#ifndef SERVER_H
#define SERVER_H

#include <QObject>
#include <QTcpServer>
#include <QHash>
#include "protocol.h"

class ClientSession;
class GameRoom;

class GobangServer : public QObject
{
    Q_OBJECT

public:
    explicit GobangServer(quint16 port = 9527, QObject *parent = nullptr);
    ~GobangServer();

    bool start();

private slots:
    void onNewConnection();
    void onClientDisconnected(int clientId);
    void onMessageReceived(int clientId, const QJsonObject &msg);

private:
    // 消息处理函数
    void handleLogin(int clientId, const QJsonObject &msg);
    void handleCreateRoom(int clientId, const QJsonObject &msg);
    void handleJoinRoom(int clientId, const QJsonObject &msg);
    void handleLeaveRoom(int clientId, const QJsonObject &msg);
    void handleReady(int clientId, const QJsonObject &msg);
    void handleMove(int clientId, const QJsonObject &msg);
    void handleChat(int clientId, const QJsonObject &msg);
    void handleSurrender(int clientId, const QJsonObject &msg);
    void handleUndoRequest(int clientId, const QJsonObject &msg);
    void handleUndoReply(int clientId, const QJsonObject &msg);
    void handleRestartRequest(int clientId, const QJsonObject &msg);
    void handleRestartReply(int clientId, const QJsonObject &msg);

    // 广播
    void broadcastRoomList();
    void broadcastPlayerList();

    // 辅助
    QString generateRoomId();

    QTcpServer *m_tcpServer;
    QHash<int, ClientSession*> m_clients;     // userId -> session
    QHash<QString, GameRoom*> m_rooms;         // roomId -> room
    int m_nextUserId;
    int m_nextRoomNum;
    quint16 m_port;
};

#endif // SERVER_H
