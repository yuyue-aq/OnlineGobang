#include "server.h"
#include "clientsession.h"
#include "gameroom.h"
#include <QDateTime>
#include <QDebug>

GobangServer::GobangServer(quint16 port, QObject *parent)
    : QObject(parent)
    , m_tcpServer(new QTcpServer(this))
    , m_nextUserId(1)
    , m_nextRoomNum(1)
    , m_port(port)
{
}

GobangServer::~GobangServer()
{
    m_tcpServer->close();
    qDeleteAll(m_rooms);
    qDeleteAll(m_clients);
}

bool GobangServer::start()
{
    if (!m_tcpServer->listen(QHostAddress::Any, m_port)) {
        qCritical() << "Failed to start server:" << m_tcpServer->errorString();
        return false;
    }

    connect(m_tcpServer, &QTcpServer::newConnection, this, &GobangServer::onNewConnection);

    qInfo() << "Gobang Server started on port" << m_port;
    return true;
}

void GobangServer::onNewConnection()
{
    while (m_tcpServer->hasPendingConnections()) {
        QTcpSocket *socket = m_tcpServer->nextPendingConnection();
        int userId = m_nextUserId++;

        ClientSession *session = new ClientSession(socket, userId, this);
        m_clients[userId] = session;

        connect(session, &ClientSession::messageReceived,
                this, &GobangServer::onMessageReceived);
        connect(session, &ClientSession::disconnected,
                this, &GobangServer::onClientDisconnected);

        qInfo() << "Client connected, userId:" << userId
                << "from:" << socket->peerAddress().toString();
    }
}

void GobangServer::onClientDisconnected(int clientId)
{
    ClientSession *session = m_clients.take(clientId);
    if (!session) return;

    qInfo() << "Client disconnected, userId:" << clientId
            << "nickname:" << session->nickname();

    // 如果在房间中，处理离开
    QString roomId = session->roomId();
    if (!roomId.isEmpty() && m_rooms.contains(roomId)) {
        GameRoom *room = m_rooms[roomId];
        room->handlePlayerLeave(session);

        // 如果房间为空，删除房间
        if (room->isEmpty()) {
            m_rooms.remove(roomId);
            delete room;
            broadcastRoomList();
        }
    }

    session->deleteLater();
    broadcastPlayerList();
    broadcastRoomList();
}

void GobangServer::onMessageReceived(int clientId, const QJsonObject &msg)
{
    MsgType type = Protocol::parseType(msg);

    switch (type) {
    case MsgType::Login:        handleLogin(clientId, msg);        break;
    case MsgType::CreateRoom:   handleCreateRoom(clientId, msg);   break;
    case MsgType::JoinRoom:     handleJoinRoom(clientId, msg);     break;
    case MsgType::LeaveRoom:    handleLeaveRoom(clientId, msg);    break;
    case MsgType::Ready:        handleReady(clientId, msg);        break;
    case MsgType::Move:         handleMove(clientId, msg);         break;
    case MsgType::Chat:         handleChat(clientId, msg);         break;
    case MsgType::Surrender:    handleSurrender(clientId, msg);    break;
    case MsgType::UndoRequest:  handleUndoRequest(clientId, msg);  break;
    case MsgType::UndoReply:    handleUndoReply(clientId, msg);    break;
    case MsgType::Restart:      handleRestartRequest(clientId, msg); break;
    case MsgType::RestartReply: handleRestartReply(clientId, msg); break;
    default:
        qWarning() << "Unknown message type from client" << clientId;
        break;
    }
}

void GobangServer::handleLogin(int clientId, const QJsonObject &msg)
{
    ClientSession *session = m_clients.value(clientId);
    if (!session) return;

    QString nickname = msg["nickname"].toString().trimmed();
    if (nickname.isEmpty()) {
        session->sendMessage(Protocol::loginResp(false, "nickname_empty"));
        return;
    }

    // 检查昵称是否重复
    for (auto *c : m_clients) {
        if (c != session && c->nickname() == nickname) {
            session->sendMessage(Protocol::loginResp(false, "nickname_duplicate"));
            return;
        }
    }

    session->setNickname(nickname);
    session->sendMessage(Protocol::loginResp(true, "", clientId));

    qInfo() << "User logged in:" << nickname << "(id:" << clientId << ")";

    broadcastPlayerList();
    broadcastRoomList();
}

void GobangServer::handleCreateRoom(int clientId, const QJsonObject &msg)
{
    ClientSession *session = m_clients.value(clientId);
    if (!session) return;

    if (session->nickname().isEmpty()) {
        session->sendMessage(Protocol::createRoomResp(false));
        return;
    }

    if (!session->roomId().isEmpty()) {
        session->sendMessage(Protocol::createRoomResp(false));
        return;
    }

    QString roomId = generateRoomId();
    QString roomName = session->nickname() + "的房间";
    GameRoom *room = new GameRoom(roomId, roomName, session, this);

    room->addPlayer(session);
    m_rooms[roomId] = room;

    session->sendMessage(Protocol::createRoomResp(true, roomId));

    qInfo() << "Room created:" << roomId << "by" << session->nickname();

    broadcastRoomList();
}

void GobangServer::handleJoinRoom(int clientId, const QJsonObject &msg)
{
    ClientSession *session = m_clients.value(clientId);
    if (!session) return;

    QString roomId = msg["roomId"].toString();

    if (session->roomId().isEmpty() == false) {
        session->sendMessage(Protocol::joinRoomResp(false, "already_in_room"));
        return;
    }

    GameRoom *room = m_rooms.value(roomId);
    if (!room) {
        session->sendMessage(Protocol::joinRoomResp(false, "room_not_found"));
        return;
    }

    if (room->isFull()) {
        session->sendMessage(Protocol::joinRoomResp(false, "room_full"));
        return;
    }

    if (room->isGameStarted()) {
        session->sendMessage(Protocol::joinRoomResp(false, "game_already_started"));
        return;
    }

    room->addPlayer(session);
    session->sendMessage(Protocol::joinRoomResp(true));

    // 通知房间内的另一方
    ClientSession *other = room->player1();
    if (other && other != session) {
        other->sendMessage(Protocol::playerJoinedRoom(session->nickname()));
    }

    qInfo() << session->nickname() << "joined room" << roomId;

    broadcastRoomList();
}

void GobangServer::handleLeaveRoom(int clientId, const QJsonObject &msg)
{
    ClientSession *session = m_clients.value(clientId);
    if (!session) return;

    QString roomId = session->roomId();
    if (roomId.isEmpty()) return;

    GameRoom *room = m_rooms.value(roomId);
    if (!room) return;

    if (room->isGameStarted()) {
        // 游戏中离开，通过 handlePlayerLeave 处理（判对手赢）
        room->handlePlayerLeave(session);
    } else {
        room->removePlayer(session);
    }

    // 如果房间为空，删除
    if (room->isEmpty()) {
        m_rooms.remove(roomId);
        delete room;
    }

    broadcastRoomList();
}

void GobangServer::handleReady(int clientId, const QJsonObject &msg)
{
    ClientSession *session = m_clients.value(clientId);
    if (!session) return;

    GameRoom *room = m_rooms.value(session->roomId());
    if (!room) return;

    room->handleReady(session);
}

void GobangServer::handleMove(int clientId, const QJsonObject &msg)
{
    ClientSession *session = m_clients.value(clientId);
    if (!session) return;

    GameRoom *room = m_rooms.value(session->roomId());
    if (!room) return;

    int row = msg["row"].toInt(-1);
    int col = msg["col"].toInt(-1);
    room->handleMove(session, row, col);
}

void GobangServer::handleChat(int clientId, const QJsonObject &msg)
{
    ClientSession *session = m_clients.value(clientId);
    if (!session) return;

    GameRoom *room = m_rooms.value(session->roomId());
    if (!room) return;

    QString message = msg["message"].toString();
    room->handleChat(session, message);
}

void GobangServer::handleSurrender(int clientId, const QJsonObject &msg)
{
    ClientSession *session = m_clients.value(clientId);
    if (!session) return;

    GameRoom *room = m_rooms.value(session->roomId());
    if (!room) return;

    room->handleSurrender(session);
}

void GobangServer::handleUndoRequest(int clientId, const QJsonObject &msg)
{
    ClientSession *session = m_clients.value(clientId);
    if (!session) return;

    GameRoom *room = m_rooms.value(session->roomId());
    if (!room) return;

    room->handleUndoRequest(session);
}

void GobangServer::handleUndoReply(int clientId, const QJsonObject &msg)
{
    ClientSession *session = m_clients.value(clientId);
    if (!session) return;

    GameRoom *room = m_rooms.value(session->roomId());
    if (!room) return;

    bool agree = msg["agree"].toBool(false);
    room->handleUndoReply(session, agree);
}

void GobangServer::handleRestartRequest(int clientId, const QJsonObject &msg)
{
    ClientSession *session = m_clients.value(clientId);
    if (!session) return;

    GameRoom *room = m_rooms.value(session->roomId());
    if (!room) return;

    room->handleRestartRequest(session);
}

void GobangServer::handleRestartReply(int clientId, const QJsonObject &msg)
{
    ClientSession *session = m_clients.value(clientId);
    if (!session) return;

    GameRoom *room = m_rooms.value(session->roomId());
    if (!room) return;

    bool agree = msg["agree"].toBool(false);
    room->handleRestartReply(session, agree);
}

void GobangServer::broadcastRoomList()
{
    QJsonArray rooms;
    for (auto *room : m_rooms) {
        rooms.append(room->roomInfo());
    }
    QJsonObject msg = Protocol::roomList(rooms);

    for (auto *client : m_clients) {
        if (!client->nickname().isEmpty()) {
            client->sendMessage(msg);
        }
    }
}

void GobangServer::broadcastPlayerList()
{
    QJsonArray players;
    for (auto *client : m_clients) {
        if (!client->nickname().isEmpty()) {
            QJsonObject p;
            p["id"] = client->userId();
            p["nickname"] = client->nickname();
            players.append(p);
        }
    }
    QJsonObject msg = Protocol::playerList(players);

    for (auto *client : m_clients) {
        if (!client->nickname().isEmpty()) {
            client->sendMessage(msg);
        }
    }
}

QString GobangServer::generateRoomId()
{
    return QString("room_%1").arg(m_nextRoomNum++);
}
