#include "networkmanager.h"
#include <QJsonArray>

NetworkManager::NetworkManager(QObject *parent)
    : QObject(parent)
    , m_socket(new QTcpSocket(this))
    , m_port(0)
{
    connect(m_socket, &QTcpSocket::connected, this, &NetworkManager::onConnected);
    connect(m_socket, &QTcpSocket::disconnected, this, &NetworkManager::onDisconnected);
    connect(m_socket, &QTcpSocket::readyRead, this, &NetworkManager::onReadyRead);
    connect(m_socket, &QTcpSocket::errorOccurred, this, &NetworkManager::onError);
}

NetworkManager::~NetworkManager()
{
    disconnectFromServer();
}

void NetworkManager::connectToServer(const QString &host, quint16 port)
{
    m_host = host;
    m_port = port;
    m_buffer.clear();
    m_socket->connectToHost(host, port);
}

void NetworkManager::disconnectFromServer()
{
    if (m_socket->state() != QAbstractSocket::UnconnectedState) {
        m_socket->disconnectFromHost();
    }
}

bool NetworkManager::isConnected() const
{
    return m_socket->state() == QAbstractSocket::ConnectedState;
}

void NetworkManager::login(const QString &nickname)
{
    sendMessage(Protocol::loginReq(nickname));
}

void NetworkManager::createRoom()
{
    sendMessage(Protocol::createRoomReq());
}

void NetworkManager::joinRoom(const QString &roomId)
{
    sendMessage(Protocol::joinRoomReq(roomId));
}

void NetworkManager::leaveRoom()
{
    sendMessage(Protocol::leaveRoomReq());
}

void NetworkManager::ready()
{
    sendMessage(Protocol::readyReq());
}

void NetworkManager::sendMove(int row, int col)
{
    sendMessage(Protocol::moveReq(row, col));
}

void NetworkManager::sendChat(const QString &message)
{
    sendMessage(Protocol::chatReq(message));
}

void NetworkManager::surrender()
{
    sendMessage(Protocol::surrenderReq());
}

void NetworkManager::requestUndo()
{
    sendMessage(Protocol::undoRequest());
}

void NetworkManager::replyUndo(bool agree)
{
    sendMessage(Protocol::undoReply(agree));
}

void NetworkManager::requestRestart()
{
    sendMessage(Protocol::restartReq());
}

void NetworkManager::replyRestart(bool agree)
{
    sendMessage(Protocol::restartReply(agree));
}

void NetworkManager::onConnected()
{
    emit connected();
}

void NetworkManager::onDisconnected()
{
    m_buffer.clear();
    emit disconnected();
}

void NetworkManager::onReadyRead()
{
    QJsonObject msg = Protocol::tryReadMessage(m_socket, m_buffer);
    while (!msg.isEmpty()) {
        dispatchMessage(msg);
        emit messageReceived(msg);
        msg = Protocol::tryReadMessage(m_socket, m_buffer);
    }
}

void NetworkManager::onError(QAbstractSocket::SocketError socketError)
{
    Q_UNUSED(socketError)
    emit errorOccurred(m_socket->errorString());
}

void NetworkManager::sendMessage(const QJsonObject &msg)
{
    Protocol::sendMessage(m_socket, msg);
}

void NetworkManager::dispatchMessage(const QJsonObject &msg)
{
    MsgType type = Protocol::parseType(msg);

    switch (type) {
    case MsgType::LoginResp:
        emit loginResult(msg["success"].toBool(), msg["reason"].toString(), msg["userId"].toInt());
        break;
    case MsgType::RoomList:
        emit roomListReceived(msg["rooms"].toArray());
        break;
    case MsgType::PlayerList:
        emit playerListReceived(msg["players"].toArray());
        break;
    case MsgType::PlayerJoinedRoom:
        emit playerJoinedRoom(msg["nickname"].toString());
        break;
    case MsgType::CreateRoomResp:
        emit createRoomResult(msg["success"].toBool(), msg["roomId"].toString());
        break;
    case MsgType::JoinRoomResp:
        emit joinRoomResult(msg["success"].toBool(), msg["reason"].toString());
        break;
    case MsgType::GameStart:
        emit gameStarted(msg["blackId"].toString(), msg["whiteId"].toString(), msg["yourColor"].toInt());
        break;
    case MsgType::TurnNotify:
        emit turnChanged(msg["color"].toInt());
        break;
    case MsgType::MoveResp:
        emit moveResult(msg["success"].toBool(), msg["reason"].toString());
        break;
    case MsgType::MoveNotify:
        emit moveNotify(msg["row"].toInt(), msg["col"].toInt(), msg["color"].toInt());
        break;
    case MsgType::GameOver:
        emit gameOver(msg["winner"].toInt(), msg["reason"].toString(), msg["winLine"].toArray());
        break;
    case MsgType::ChatNotify:
        emit chatReceived(msg["from"].toString(), msg["message"].toString());
        break;
    case MsgType::OpponentLeft:
        emit opponentLeft(msg["reason"].toString());
        break;
    case MsgType::UndoNotify:
        emit undoRequested(msg["from"].toString());
        break;
    case MsgType::UndoResult:
        emit undoResult(msg["agreed"].toBool(), msg["row"].toInt(), msg["col"].toInt(), msg["color"].toInt());
        break;
    case MsgType::RestartNotify:
        emit restartRequested(msg["from"].toString());
        break;
    case MsgType::ReadyNotify:
        emit readyNotifyReceived(msg["nickname"].toString());
        break;
    case MsgType::RestartReply:
        emit restartReplyReceived(msg["agree"].toBool(false));
        break;
    default:
        break;
    }
}
