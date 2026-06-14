#ifndef PROTOCOL_H
#define PROTOCOL_H

#include <QByteArray>
#include <QDataStream>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QTcpSocket>

// 棋子颜色常量
namespace ChessColor {
    constexpr int NONE  = 0;
    constexpr int BLACK = 1;
    constexpr int WHITE = 2;
}

// 消息类型枚举
enum class MsgType : int {
    // 登录
    Login       = 1,
    LoginResp   = 2,
    // 房间
    CreateRoom     = 10,
    CreateRoomResp = 11,
    JoinRoom       = 12,
    JoinRoomResp   = 13,
    LeaveRoom      = 14,
    RoomList       = 15,
    PlayerList     = 16,
    PlayerJoinedRoom = 17,
    // 游戏流程
    Ready       = 20,
    ReadyNotify = 21,
    GameStart   = 22,
    TurnNotify  = 23,
    // 落子
    Move        = 30,
    MoveResp    = 31,
    MoveNotify  = 32,
    // 游戏结束
    GameOver    = 40,
    // 聊天
    Chat        = 50,
    ChatNotify  = 51,
    // 悔棋
    UndoRequest = 60,
    UndoNotify  = 61,
    UndoReply   = 62,
    UndoResult  = 63,
    // 认输
    Surrender   = 70,
    // 再来一局
    Restart      = 80,
    RestartNotify = 81,
    RestartReply  = 82,
    // 掉线
    OpponentLeft = 90,
    // 错误
    Error = 999
};

// 协议辅助工具类
class Protocol {
public:
    // 构造 JSON 消息
    static QJsonObject makeMessage(MsgType type, const QJsonObject &data = QJsonObject()) {
        QJsonObject msg;
        msg["type"] = static_cast<int>(type);
        if (!data.isEmpty()) {
            for (auto it = data.begin(); it != data.end(); ++it) {
                msg[it.key()] = it.value();
            }
        }
        return msg;
    }

    // 解析消息类型
    static MsgType parseType(const QJsonObject &msg) {
        if (msg.contains("type")) {
            return static_cast<MsgType>(msg["type"].toInt());
        }
        return MsgType::Error;
    }

    // 封包：QJsonObject → QByteArray（带 4 字节大端长度前缀）
    static QByteArray pack(const QJsonObject &json) {
        QByteArray body = QJsonDocument(json).toJson(QJsonDocument::Compact);
        QByteArray packet;
        QDataStream stream(&packet, QIODevice::WriteOnly);
        stream.setByteOrder(QDataStream::BigEndian);
        stream << static_cast<quint32>(body.size());
        packet.append(body);
        return packet;
    }

    // 发送消息到 socket
    static void sendMessage(QTcpSocket *socket, const QJsonObject &json) {
        if (socket && socket->state() == QAbstractSocket::ConnectedState) {
            socket->write(pack(json));
            socket->flush();
        }
    }

    // 从 socket 缓冲区尝试读取一条完整消息
    // 成功返回 QJsonObject，不完整返回空的 QJsonObject
    static QJsonObject tryReadMessage(QTcpSocket *socket, QByteArray &buffer) {
        if (!socket) return {};

        buffer.append(socket->readAll());

        if (buffer.size() < 4) return {};

        QDataStream stream(buffer);
        stream.setByteOrder(QDataStream::BigEndian);
        quint32 msgLen = 0;
        stream >> msgLen;

        if (msgLen == 0 || msgLen > 1024 * 1024) {  // 消息最大 1MB
            buffer.clear();
            return {};
        }

        if (static_cast<quint32>(buffer.size()) < 4 + msgLen) return {};

        QByteArray jsonData = buffer.mid(4, msgLen);
        buffer.remove(0, 4 + msgLen);

        QJsonParseError error;
        QJsonDocument doc = QJsonDocument::fromJson(jsonData, &error);
        if (error.error != QJsonParseError::NoError) return {};

        return doc.object();
    }

    // 便捷构造各类型消息
    static QJsonObject loginReq(const QString &nickname) {
        return makeMessage(MsgType::Login, {{"nickname", nickname}});
    }
    static QJsonObject loginResp(bool success, const QString &reason = {}, int userId = 0) {
        return makeMessage(MsgType::LoginResp, {{"success", success}, {"reason", reason}, {"userId", userId}});
    }
    static QJsonObject createRoomReq() {
        return makeMessage(MsgType::CreateRoom);
    }
    static QJsonObject createRoomResp(bool success, const QString &roomId = {}) {
        return makeMessage(MsgType::CreateRoomResp, {{"success", success}, {"roomId", roomId}});
    }
    static QJsonObject joinRoomReq(const QString &roomId) {
        return makeMessage(MsgType::JoinRoom, {{"roomId", roomId}});
    }
    static QJsonObject joinRoomResp(bool success, const QString &reason = {}) {
        return makeMessage(MsgType::JoinRoomResp, {{"success", success}, {"reason", reason}});
    }
    static QJsonObject leaveRoomReq() {
        return makeMessage(MsgType::LeaveRoom);
    }
    static QJsonObject roomList(const QJsonArray &rooms) {
        return makeMessage(MsgType::RoomList, {{"rooms", rooms}});
    }
    static QJsonObject playerList(const QJsonArray &players) {
        return makeMessage(MsgType::PlayerList, {{"players", players}});
    }
    static QJsonObject playerJoinedRoom(const QString &nickname) {
        return makeMessage(MsgType::PlayerJoinedRoom, {{"nickname", nickname}});
    }
    static QJsonObject readyReq() {
        return makeMessage(MsgType::Ready);
    }
    static QJsonObject readyNotify(const QString &nickname) {
        return makeMessage(MsgType::ReadyNotify, {{"nickname", nickname}});
    }
    static QJsonObject gameStart(const QString &blackId, const QString &whiteId, int yourColor) {
        return makeMessage(MsgType::GameStart, {{"blackId", blackId}, {"whiteId", whiteId}, {"yourColor", yourColor}});
    }
    static QJsonObject turnNotify(int color) {
        return makeMessage(MsgType::TurnNotify, {{"color", color}});
    }
    static QJsonObject moveReq(int row, int col) {
        return makeMessage(MsgType::Move, {{"row", row}, {"col", col}});
    }
    static QJsonObject moveResp(bool success, const QString &reason = {}) {
        return makeMessage(MsgType::MoveResp, {{"success", success}, {"reason", reason}});
    }
    static QJsonObject moveNotify(int row, int col, int color) {
        return makeMessage(MsgType::MoveNotify, {{"row", row}, {"col", col}, {"color", color}});
    }
    static QJsonObject gameOver(int winner, const QString &reason, const QJsonArray &winLine = QJsonArray()) {
        QJsonObject data = {{"winner", winner}, {"reason", reason}};
        if (!winLine.isEmpty()) data["winLine"] = winLine;
        return makeMessage(MsgType::GameOver, data);
    }
    static QJsonObject chatReq(const QString &message) {
        return makeMessage(MsgType::Chat, {{"message", message}});
    }
    static QJsonObject chatNotify(const QString &from, const QString &message) {
        return makeMessage(MsgType::ChatNotify, {{"from", from}, {"message", message}});
    }
    static QJsonObject surrenderReq() {
        return makeMessage(MsgType::Surrender);
    }
    static QJsonObject undoRequest() {
        return makeMessage(MsgType::UndoRequest);
    }
    static QJsonObject undoNotify(const QString &from) {
        return makeMessage(MsgType::UndoNotify, {{"from", from}});
    }
    static QJsonObject undoReply(bool agree) {
        return makeMessage(MsgType::UndoReply, {{"agree", agree}});
    }
    static QJsonObject undoResult(bool agreed, int row = -1, int col = -1, int color = 0) {
        return makeMessage(MsgType::UndoResult, {{"agreed", agreed}, {"row", row}, {"col", col}, {"color", color}});
    }
    static QJsonObject restartReq() {
        return makeMessage(MsgType::Restart);
    }
    static QJsonObject restartNotify(const QString &from) {
        return makeMessage(MsgType::RestartNotify, {{"from", from}});
    }
    static QJsonObject restartReply(bool agree) {
        return makeMessage(MsgType::RestartReply, {{"agree", agree}});
    }
    static QJsonObject restartReplyResp(bool agree) {
        return makeMessage(MsgType::RestartReply, {{"agree", agree}});
    }
    static QJsonObject opponentLeft(const QString &reason) {
        return makeMessage(MsgType::OpponentLeft, {{"reason", reason}});
    }
};

#endif // PROTOCOL_H
