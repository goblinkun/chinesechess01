// SPDX-License-Identifier: GPL-3.0-or-later
// SPDX-FileCopyrightText: 2019-2026 XMuli & Contributors
// SPDX-GitHub: https://github.com/XMuli/ChineseChess
// SPDX-Author: XMuli <xmulitech@gmail.com>

#ifndef MACHINEGAME_H
#define MACHINEGAME_H

#include "ChessBoard.h"
#include "ChessStep.h"
#include "XiangqiEngine.h"
#include <QVector>

enum class AIDifficulty { Easy = 0, Medium = 1, Hard = 2 };

class MachineGame : public ChessBoard
{
public:
    MachineGame(AIDifficulty diff = AIDifficulty::Medium);
    ~MachineGame();

    void saveStep(int moveID, int checkedID, int row, int col, QVector<ChessStep*>& steps);
    void getAllMoves(QVector<ChessStep*>& steps, bool forRed);

    void mouseReleaseEvent(QMouseEvent *ev) override;

    void fakeMove(ChessStep* step);
    void unFakeMove(ChessStep* step);
    int calcScore();
    int alphaBeta(int depth, int alpha, int beta, bool isMaximizing);
    ChessStep* getBestMove();
    void machineChooseAndMovePieces();

    int getSearchDepth() const;
    bool isAiRed() const { return m_bAiIsRed; }
    void setAiIsRed(bool red) { m_bAiIsRed = red; }

private:
    int getThinkingTime() const;                    // 难度 → 引擎思考时间(毫秒)
    void onEngineBestMove(const QString &uciMove);  // 引擎返回最佳走法
    void setupEngine();                             // 查找并启动皮卡鱼引擎

protected:
    AIDifficulty m_difficulty;
    bool m_bAiIsRed = false;  // AI 默认执黑

    XiangqiEngine *m_engine = nullptr;
    bool m_engineThinking = false;
};

#endif // MACHINEGAME_H
