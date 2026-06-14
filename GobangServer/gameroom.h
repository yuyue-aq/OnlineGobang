#ifndef GAMEROOM_H
#define GAMEROOM_H

#include <QObject>
#include <QVector>
#include "protocol.h"

class ClientSession;

// 落子记录
struct MoveRecord {
    int row;
    int col;
    int color; // BLACK or WHITE
};

class GameRoom : public QObject
{
    Q_OBJECT

public:
    explicit GameRoom(const QString &roomId, const QString &roomName,
                     ClientSession *creator, QObject *parent = nullptr);
    ~GameRoom();

    QString roomId() const { return m_roomId; }
    QString roomName() const { return m_roomName; }
    int playerCount() const;
    bool isFull() const { return playerCount() >= 2; }
    bool isEmpty() const { return playerCount() == 0; }
    bool isGameStarted() const { return m_gameStarted; }
    ClientSession *player1() const { return m_player1; }
    ClientSession *player2() const { return m_player2; }

    // 玩家管理
    bool addPlayer(ClientSession *player);
    void removePlayer(ClientSession *player);

    // 游戏操作
    void handleReady(ClientSession *player);
    void handleMove(ClientSession *player, int row, int col);
    void handleSurrender(ClientSession *player);
    void handleUndoRequest(ClientSession *player);
    void handleUndoReply(ClientSession *player, bool agree);
    void handleRestartRequest(ClientSession *player);
    void handleRestartReply(ClientSession *player, bool agree);
    void handleChat(ClientSession *player, const QString &message);

    // 对手离开处理
    void handlePlayerLeave(ClientSession *player);

    // 获取房间信息（用于广播）
    QJsonObject roomInfo() const;

private:
    void startGame();
    void resetGame();
    bool checkWin(int row, int col, int color);
    bool checkDraw();
    void broadcastMessage(const QJsonObject &msg);
    void sendToPlayer(ClientSession *player, const QJsonObject &msg);
    ClientSession *getOpponent(ClientSession *player) const;
    int getPlayerColor(ClientSession *player) const;

    QString m_roomId;
    QString m_roomName;
    ClientSession *m_player1;  // 黑方
    ClientSession *m_player2;  // 白方

    // 游戏状态
    static const int BOARD_SIZE = 15;
    int m_board[15][15];
    int m_currentTurn;         // 当前回合颜色
    bool m_gameStarted;
    bool m_gameOver;
    QVector<MoveRecord> m_moveHistory;

    // 准备状态
    bool m_player1Ready;
    bool m_player2Ready;

    // 悔棋请求状态
    bool m_undoPending;
    int m_undoRequesterId;     // 发起悔棋的玩家 ID

    // 再来一局请求状态
    bool m_restartPending;
    int m_restartRequesterId;
};

#endif // GAMEROOM_H
