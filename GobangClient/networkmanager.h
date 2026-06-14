#ifndef NETWORKMANAGER_H
#define NETWORKMANAGER_H

#include <QObject>
#include <QTcpSocket>
#include <QJsonObject>
#include <QJsonArray>
#include "protocol.h"

class NetworkManager : public QObject
{
    Q_OBJECT

public:
    explicit NetworkManager(QObject *parent = nullptr);
    ~NetworkManager();

    void connectToServer(const QString &host, quint16 port);
    void disconnectFromServer();
    bool isConnected() const;

    // 便捷发送方法
    void login(const QString &nickname);
    void createRoom();
    void joinRoom(const QString &roomId);
    void leaveRoom();
    void ready();
    void sendMove(int row, int col);
    void sendChat(const QString &message);
    void surrender();
    void requestUndo();
    void replyUndo(bool agree);
    void requestRestart();
    void replyRestart(bool agree);

signals:
    void connected();
    void disconnected();
    void errorOccurred(const QString &errorString);
    void messageReceived(const QJsonObject &msg);

    // 便捷信号
    void loginResult(bool success, const QString &reason, int userId);
    void roomListReceived(const QJsonArray &rooms);
    void playerListReceived(const QJsonArray &players);
    void playerJoinedRoom(const QString &nickname);
    void createRoomResult(bool success, const QString &roomId);
    void joinRoomResult(bool success, const QString &reason);
    void gameStarted(const QString &blackId, const QString &whiteId, int yourColor);
    void turnChanged(int color);
    void moveResult(bool success, const QString &reason);
    void moveNotify(int row, int col, int color);
    void gameOver(int winner, const QString &reason, const QJsonArray &winLine);
    void chatReceived(const QString &from, const QString &message);
    void opponentLeft(const QString &reason);
    void undoRequested(const QString &from);
    void undoResult(bool agreed, int row, int col, int color);
    void restartRequested(const QString &from);
    void restartReplyReceived(bool agreed);
    void readyNotifyReceived(const QString &nickname);

private slots:
    void onConnected();
    void onDisconnected();
    void onReadyRead();
    void onError(QAbstractSocket::SocketError socketError);

private:
    void sendMessage(const QJsonObject &msg);
    void dispatchMessage(const QJsonObject &msg);

    QTcpSocket *m_socket;
    QByteArray m_buffer;
    QString m_host;
    quint16 m_port;
};

#endif // NETWORKMANAGER_H
