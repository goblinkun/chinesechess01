// SPDX-License-Identifier: GPL-3.0-or-later
// SPDX-FileCopyrightText: 2019-2026 XMuli & Contributors
// SPDX-GitHub: https://github.com/XMuli/ChineseChess

#include "XiangqiEngine.h"

#include <QCoreApplication>
#include <QFileInfo>
#include <QStringList>

XiangqiEngine::XiangqiEngine(QObject *parent)
    : QObject(parent)
{
    connect(&m_process, &QProcess::readyReadStandardOutput, this, &XiangqiEngine::onReadyRead);
    connect(&m_process, &QProcess::errorOccurred, this, [this](QProcess::ProcessError err) {
        if (err == QProcess::FailedToStart)
            emit engineError(QStringLiteral("引擎启动失败: %1").arg(m_process.program()));
    });
}

XiangqiEngine::~XiangqiEngine()
{
    stop();
}

bool XiangqiEngine::start(const QString &enginePath, const QString &evalFilePath)
{
    m_evalFilePath = evalFilePath;
    m_bIsReady = false;
    m_bWaitingReady = false;
    m_pendingMultiPv = false;
    m_collectingLines = false;
    m_lines.clear();

    m_process.start(enginePath, QStringList());
    if (!m_process.waitForStarted(3000)) {
        emit engineError(QStringLiteral("引擎启动失败: %1").arg(enginePath));
        return false;
    }

    sendCommand("uci");
    return true;
}

void XiangqiEngine::stop() noexcept
{
    if (m_process.state() != QProcess::NotRunning) {
        sendCommand("quit");
        m_process.waitForFinished(500);
        m_process.kill();
        m_process.waitForFinished(500);
    }
    m_bIsReady = false;
    m_bWaitingReady = false;
}

void XiangqiEngine::analyze(const QString &fen, int movetimeMs)
{
    if (!m_bIsReady || m_process.state() != QProcess::Running) {
        // 尚未就绪：缓存请求，待握手完成后执行
        m_pendingFen = fen;
        m_pendingTime = movetimeMs;
        m_bWaitingReady = true;
        m_pendingMultiPv = false;
        return;
    }

    sendCommand("position fen " + fen);
    sendCommand(QStringLiteral("go movetime %1").arg(movetimeMs));
}

void XiangqiEngine::analyzeMultiPv(const QString &fen, int movetimeMs, int numLines)
{
    Q_UNUSED(numLines);   // 引擎已通过 setoption MultiPV 配置为 5 路
    if (!m_bIsReady || m_process.state() != QProcess::Running) {
        m_pendingFen = fen;
        m_pendingTime = movetimeMs;
        m_bWaitingReady = true;
        m_pendingMultiPv = true;
        return;
    }

    m_lines.clear();
    m_collectingLines = true;
    sendCommand("position fen " + fen);
    sendCommand(QStringLiteral("go movetime %1").arg(movetimeMs));
}

QString XiangqiEngine::findEngineBinary()
{
    const QString appDir = QCoreApplication::applicationDirPath();
    for (const QString &name : { QStringLiteral("engine/pikafish"),
                                 QStringLiteral("pikafish"),
                                 QStringLiteral("engine/pikafish.exe"),
                                 QStringLiteral("pikafish.exe") }) {
        const QString p = appDir + QLatin1Char('/') + name;
        if (QFileInfo::exists(p))
            return p;
    }
    return QString();
}

// 解析 "info ... multipv N ... score cp|mate V ... pv m1 m2 ..." 行
void XiangqiEngine::parseInfoLine(const QString &s)
{
    const QStringList t = s.split(' ', Qt::SkipEmptyParts);
    EngineLine line;
    bool inPv = false;
    for (int i = 1; i < t.size(); ++i) {
        const QString &w = t.at(i);
        if (w == "multipv" && i + 1 < t.size()) {
            line.multipv = t.at(++i).toInt();
        } else if (w == "score" && i + 1 < t.size()) {
            const QString &kind = t.at(++i);
            if (i + 1 < t.size()) {
                const int v = t.at(++i).toInt();
                if (kind == "mate") { line.scoreIsMate = true; line.matePlies = v; }
                else if (kind == "cp") { line.scoreCp = v; }
            }
        } else if (w == "pv") {
            inPv = true;
        } else if (inPv) {
            line.pv.append(w);
        }
    }

    // 仅保留真正的搜索主变行（multipv >= 1），忽略 "info string ..." 等
    if (line.multipv <= 0)
        return;

    // 同一 multipv 序号只保留最新一条
    for (auto &e : m_lines) {
        if (e.multipv == line.multipv) { e = line; return; }
    }
    m_lines.append(line);
}

void XiangqiEngine::onReadyRead()
{
    while (m_process.canReadLine()) {
        const QByteArray line = m_process.readLine().trimmed();
        if (line.isEmpty())
            continue;

        const QString s = QString::fromLatin1(line);

        if (s == "uciok") {
            m_bIsReady = true;
            if (!m_evalFilePath.isEmpty())
                sendCommand("setoption name EvalFile value " + m_evalFilePath);
            sendCommand("setoption name MultiPV value 5");
            sendCommand("isready");
        } else if (s == "readyok") {
            if (m_bWaitingReady) {
                m_bWaitingReady = false;
                if (m_pendingMultiPv) {
                    m_lines.clear();
                    m_collectingLines = true;
                    m_pendingMultiPv = false;
                }
                sendCommand("position fen " + m_pendingFen);
                sendCommand(QStringLiteral("go movetime %1").arg(m_pendingTime));
            }
        } else if (s.startsWith("info")) {
            if (m_collectingLines)
                parseInfoLine(s);
        } else if (s.startsWith("bestmove")) {
            if (m_collectingLines) {
                m_collectingLines = false;
                emit analysisLines(m_lines);
            }
            const QStringList parts = s.split(' ', Qt::SkipEmptyParts);
            if (parts.size() >= 2)
                emit bestMove(parts.at(1));
        }
    }
}

void XiangqiEngine::sendCommand(const QString &cmd)
{
    if (m_process.state() == QProcess::Running) {
        m_process.write((cmd + '\n').toLatin1());
    }
}