#ifndef ONLINEGOBANG_H
#define ONLINEGOBANG_H

#include <QtWidgets/QMainWindow>
#include <QStackedWidget>
#include <QJsonArray>
#include <QJsonObject>
#include "ui_OnlineGobang.h"
#include "networkmanager.h"

class LoginWidget;
class LobbyWidget;
class GameWidget;

class OnlineGobang : public QMainWindow
{
    Q_OBJECT

public:
    OnlineGobang(QWidget *parent = nullptr);
    ~OnlineGobang();

private slots:
    // 登录相关
    void onConnectRequested(const QString &host, quint16 port, const QString &nickname);
    void onLoginResult(bool success, const QString &reason, int userId);
    void onConnected();
    void onDisconnected();
    void onNetworkError(const QString &errorString);

    // 大厅相关
    void onCreateRoomRequested();
    void onCreateRoomResult(bool success, const QString &roomId);
    void onJoinRoomRequested(const QString &roomId);
    void onJoinRoomResult(bool success, const QString &reason);
    void onRoomListReceived(const QJsonArray &rooms);
    void onPlayerListReceived(const QJsonArray &players);
    void onPlayerJoinedRoom(const QString &nickname);

    // 游戏相关
    void onGameStarted(const QString &blackId, const QString &whiteId, int yourColor);
    void onTurnChanged(int color);
    void onMoveResult(bool success, const QString &reason);
    void onMoveNotify(int row, int col, int color);
    void onGameOver(int winner, const QString &reason, const QJsonArray &winLine);
    void onChatReceived(const QString &from, const QString &message);
    void onOpponentLeft(const QString &reason);
    void onUndoRequested(const QString &from);
    void onUndoResult(bool agreed, int row, int col, int color);
    void onRestartRequested(const QString &from);
    void onRestartReplyReceived(bool agreed);
    void onReadyNotifyReceived(const QString &nickname);

    // 游戏界面发出的信号
    void onMoveRequested(int row, int col);
    void onReadyRequested();
    void onSurrenderRequested();
    void onUndoRequestedByMe();
    void onUndoReplyRequested(bool agree);
    void onRestartRequestedByMe();
    void onRestartReplyRequested(bool agree);
    void onLeaveRoomRequested();
    void onChatRequested(const QString &message);

private:
    void setupConnections();
    void switchToLobby();
    void switchToGame();

    Ui::OnlineGobangClass ui;
    NetworkManager *m_network;

    LoginWidget *m_loginWidget;
    LobbyWidget *m_lobbyWidget;
    GameWidget *m_gameWidget;

    QStackedWidget *m_stackedWidget;

    QString m_nickname;
    int m_userId;
    bool m_inRoom;
};

#endif // ONLINEGOBANG_H
