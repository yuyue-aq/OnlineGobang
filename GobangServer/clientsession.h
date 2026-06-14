#ifndef CLIENTSESSION_H
#define CLIENTSESSION_H

#include <QObject>
#include <QTcpSocket>
#include <QJsonObject>
#include "protocol.h"

class ClientSession : public QObject
{
    Q_OBJECT

public:
    explicit ClientSession(QTcpSocket *socket, int id, QObject *parent = nullptr);
    ~ClientSession();

    int userId() const { return m_userId; }
    QString nickname() const { return m_nickname; }
    void setNickname(const QString &name) { m_nickname = name; }
    QString roomId() const { return m_roomId; }
    void setRoomId(const QString &roomId) { m_roomId = roomId; }
    bool isOnline() const { return m_socket && m_socket->state() == QAbstractSocket::ConnectedState; }

    void sendMessage(const QJsonObject &msg);

signals:
    void messageReceived(int clientId, const QJsonObject &msg);
    void disconnected(int clientId);

private slots:
    void onReadyRead();
    void onDisconnected();

private:
    QTcpSocket *m_socket;
    int m_userId;
    QString m_nickname;
    QString m_roomId;
    QByteArray m_buffer;
};

#endif // CLIENTSESSION_H
