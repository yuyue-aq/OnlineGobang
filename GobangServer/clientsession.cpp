#include "clientsession.h"

ClientSession::ClientSession(QTcpSocket *socket, int id, QObject *parent)
    : QObject(parent)
    , m_socket(socket)
    , m_userId(id)
{
    m_socket->setParent(this);
    connect(m_socket, &QTcpSocket::readyRead, this, &ClientSession::onReadyRead);
    connect(m_socket, &QTcpSocket::disconnected, this, &ClientSession::onDisconnected);
}

ClientSession::~ClientSession()
{
    if (m_socket) {
        m_socket->close();
    }
}

void ClientSession::sendMessage(const QJsonObject &msg)
{
    Protocol::sendMessage(m_socket, msg);
}

void ClientSession::onReadyRead()
{
    QJsonObject msg = Protocol::tryReadMessage(m_socket, m_buffer);
    while (!msg.isEmpty()) {
        emit messageReceived(m_userId, msg);
        msg = Protocol::tryReadMessage(m_socket, m_buffer);
    }
}

void ClientSession::onDisconnected()
{
    emit disconnected(m_userId);
}
