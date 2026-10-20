// SPDX-License-Identifier: GPL-3.0-or-later
// SPDX-FileCopyrightText: 2019-2026 XMuli & Contributors
// SPDX-GitHub: https://github.com/XMuli/ChineseChess

#ifndef XIANGQIENGINE_H
#define XIANGQIENGINE_H

#include <QObject>
#include <QProcess>
#include <QString>
#include <QStringList>
#include <QVector>

// 引擎多路分析结果中的一条主变
struct EngineLine {
    int multipv = 0;         // 主变序号(1..N)
    int scoreCp = 0;         // 分数(厘兵 cp)
    bool scoreIsMate = false;// 是否为绝杀分
    int matePlies = 0;       // 绝杀步数(scoreIsMate 为 true 时有效)
    QStringList pv;          // 主变走法(UCI 坐标, 如 "h2e2")
};

// 皮卡鱼（Pikafish）引擎封装：通过 UCI 协议与引擎子进程通信。
// 用法：start() 启动引擎并加载 NNUE 权重，然后调用 analyze() 请求分析，
// 引擎返回的最佳走法通过 bestMove 信号发出（形如 "h2e2"）。
class XiangqiEngine : public QObject
{
    Q_OBJECT
public:
    explicit XiangqiEngine(QObject *parent = nullptr);
    ~XiangqiEngine() override;

    bool start(const QString &enginePath, const QString &evalFilePath = QString());
    void stop() noexcept;
    bool isReady() const { return m_bIsReady; }

    // 分析指定局面（FEN），在 movetimeMs 毫秒后返回最佳走法
    void analyze(const QString &fen, int movetimeMs);

    // 多路分析：返回最好的 numLines 个着法及其各自主变（通过 analysisLines 信号发出）
    void analyzeMultiPv(const QString &fen, int movetimeMs, int numLines = 5);

    // 在 applicationDirPath()/engine 或 applicationDirPath() 下查找引擎二进制；找不到返回空串
    static QString findEngineBinary();

signals:
    void bestMove(const QString &uciMove);
    void analysisLines(const QVector<EngineLine> &lines);
    void engineError(const QString &message);

private slots:
    void onReadyRead();

private:
    void sendCommand(const QString &cmd);
    void parseInfoLine(const QString &s);

    QProcess m_process;
    QString m_evalFilePath;   // NNUE 权重文件路径（可选）
    bool m_bIsReady = false;  // 是否已完成 uci/ready 握手
    bool m_bWaitingReady = false;  // 是否有一个待执行的 analyze 请求
    QString m_pendingFen;           // 待分析的局面
    int m_pendingTime = 0;          // 待分析的时间（毫秒）
    bool m_pendingMultiPv = false;  // 待执行请求是否为多路分析
    bool m_collectingLines = false; // 是否正在收集 multipv 行
    QVector<EngineLine> m_lines;    // 收集到的多路结果
};

#endif // XIANGQIENGINE_H