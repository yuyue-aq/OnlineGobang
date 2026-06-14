#include "OnlineGobang.h"
#include "loginwidget.h"
#include "lobbywidget.h"
#include "gamewidget.h"
#include "networkmanager.h"
#include <QStackedWidget>
#include <QMessageBox>

OnlineGobang::OnlineGobang(QWidget *parent)
    : QMainWindow(parent)
    , m_network(new NetworkManager(this))
    , m_loginWidget(nullptr)
    , m_lobbyWidget(nullptr)
    , m_gameWidget(nullptr)
    , m_userId(0)
    , m_inRoom(false)
{
    // 创建中央堆叠控件
    m_stackedWidget = new QStackedWidget(this);
    setCentralWidget(m_stackedWidget);

    // 创建页面
    m_loginWidget = new LoginWidget(this);
    m_lobbyWidget = new LobbyWidget(this);
    m_gameWidget = new GameWidget(this);

    m_stackedWidget->addWidget(m_loginWidget);  // index 0
    m_stackedWidget->addWidget(m_lobbyWidget);  // index 1
    m_stackedWidget->addWidget(m_gameWidget);   // index 2

    m_stackedWidget->setCurrentIndex(0);

    setupConnections();

    setWindowTitle("联机五子棋");
    resize(960, 700);
}

OnlineGobang::~OnlineGobang() {}

void OnlineGobang::setupConnections()
{
    // 登录界面信号
    connect(m_loginWidget, &LoginWidget::connectRequested,
            this, &OnlineGobang::onConnectRequested);

    // 网络信号
    connect(m_network, &NetworkManager::connected,
            this, &OnlineGobang::onConnected);
    connect(m_network, &NetworkManager::disconnected,
            this, &OnlineGobang::onDisconnected);
    connect(m_network, &NetworkManager::errorOccurred,
            this, &OnlineGobang::onNetworkError);
    connect(m_network, &NetworkManager::loginResult,
            this, &OnlineGobang::onLoginResult);
    connect(m_network, &NetworkManager::roomListReceived,
            this, &OnlineGobang::onRoomListReceived);
    connect(m_network, &NetworkManager::playerListReceived,
            this, &OnlineGobang::onPlayerListReceived);
    connect(m_network, &NetworkManager::playerJoinedRoom,
            this, &OnlineGobang::onPlayerJoinedRoom);
    connect(m_network, &NetworkManager::createRoomResult,
            this, &OnlineGobang::onCreateRoomResult);
    connect(m_network, &NetworkManager::joinRoomResult,
            this, &OnlineGobang::onJoinRoomResult);
    connect(m_network, &NetworkManager::gameStarted,
            this, &OnlineGobang::onGameStarted);
    connect(m_network, &NetworkManager::turnChanged,
            this, &OnlineGobang::onTurnChanged);
    connect(m_network, &NetworkManager::moveResult,
            this, &OnlineGobang::onMoveResult);
    connect(m_network, &NetworkManager::moveNotify,
            this, &OnlineGobang::onMoveNotify);
    connect(m_network, &NetworkManager::gameOver,
            this, &OnlineGobang::onGameOver);
    connect(m_network, &NetworkManager::chatReceived,
            this, &OnlineGobang::onChatReceived);
    connect(m_network, &NetworkManager::opponentLeft,
            this, &OnlineGobang::onOpponentLeft);
    connect(m_network, &NetworkManager::undoRequested,
            this, &OnlineGobang::onUndoRequested);
    connect(m_network, &NetworkManager::undoResult,
            this, &OnlineGobang::onUndoResult);
    connect(m_network, &NetworkManager::restartRequested,
            this, &OnlineGobang::onRestartRequested);
    connect(m_network, &NetworkManager::readyNotifyReceived,
            this, &OnlineGobang::onReadyNotifyReceived);
    connect(m_network, &NetworkManager::restartReplyReceived,
            this, &OnlineGobang::onRestartReplyReceived);

    // 大厅界面信号
    connect(m_lobbyWidget, &LobbyWidget::createRoomRequested,
            this, &OnlineGobang::onCreateRoomRequested);
    connect(m_lobbyWidget, &LobbyWidget::joinRoomRequested,
            this, &OnlineGobang::onJoinRoomRequested);

    // 游戏界面信号
    connect(m_gameWidget, &GameWidget::moveRequested,
            this, &OnlineGobang::onMoveRequested);
    connect(m_gameWidget, &GameWidget::readyRequested,
            this, &OnlineGobang::onReadyRequested);
    connect(m_gameWidget, &GameWidget::surrenderRequested,
            this, &OnlineGobang::onSurrenderRequested);
    connect(m_gameWidget, &GameWidget::undoRequested,
            this, &OnlineGobang::onUndoRequestedByMe);
    connect(m_gameWidget, &GameWidget::undoReplyRequested,
            this, &OnlineGobang::onUndoReplyRequested);
    connect(m_gameWidget, &GameWidget::restartRequested,
            this, &OnlineGobang::onRestartRequestedByMe);
    connect(m_gameWidget, &GameWidget::restartReplyRequested,
            this, &OnlineGobang::onRestartReplyRequested);
    connect(m_gameWidget, &GameWidget::leaveRoomRequested,
            this, &OnlineGobang::onLeaveRoomRequested);
    connect(m_gameWidget, &GameWidget::chatRequested,
            this, &OnlineGobang::onChatRequested);
}

// ===== 登录相关 =====

void OnlineGobang::onConnectRequested(const QString &host, quint16 port, const QString &nickname)
{
    m_nickname = nickname;
    m_network->connectToServer(host, port);
}

void OnlineGobang::onConnected()
{
    // 连接成功后发送登录请求
    m_network->login(m_nickname);
}

void OnlineGobang::onDisconnected()
{
    m_loginWidget->setConnecting(false);
    m_loginWidget->setStatus("已断开连接", true);
    m_stackedWidget->setCurrentIndex(0);
    m_inRoom = false;
}

void OnlineGobang::onNetworkError(const QString &errorString)
{
    m_loginWidget->setConnecting(false);
    m_loginWidget->setStatus("连接失败: " + errorString, true);
}

void OnlineGobang::onLoginResult(bool success, const QString &reason, int userId)
{
    if (success) {
        m_userId = userId;
        m_loginWidget->setStatus("登录成功！", false);
        switchToLobby();
    } else {
        m_loginWidget->setConnecting(false);
        QString msg;
        if (reason == "nickname_empty") msg = "昵称不能为空";
        else if (reason == "nickname_duplicate") msg = "昵称已被使用";
        else msg = "登录失败: " + reason;
        m_loginWidget->setStatus(msg, true);
    }
}

// ===== 大厅相关 =====

void OnlineGobang::switchToLobby()
{
    m_stackedWidget->setCurrentIndex(1);
    m_inRoom = false;
}

void OnlineGobang::onCreateRoomRequested()
{
    m_network->createRoom();
}

void OnlineGobang::onCreateRoomResult(bool success, const QString &roomId)
{
    if (success) {
        switchToGame();
        m_gameWidget->resetGame();
        m_gameWidget->setMyNickname(m_nickname);
        m_inRoom = true;
    } else {
        QMessageBox::warning(this, "创建房间", "创建房间失败");
    }
}

void OnlineGobang::onJoinRoomRequested(const QString &roomId)
{
    m_network->joinRoom(roomId);
}

void OnlineGobang::onJoinRoomResult(bool success, const QString &reason)
{
    if (success) {
        switchToGame();
        m_gameWidget->resetGame();
        m_gameWidget->setMyNickname(m_nickname);
        m_inRoom = true;
    } else {
        QString msg;
        if (reason == "room_not_found") msg = "房间不存在";
        else if (reason == "room_full") msg = "房间已满";
        else if (reason == "game_already_started") msg = "游戏已开始";
        else if (reason == "already_in_room") msg = "你已在房间中";
        else msg = "加入失败: " + reason;
        QMessageBox::warning(this, "加入房间", msg);
    }
}

void OnlineGobang::onRoomListReceived(const QJsonArray &rooms)
{
    if (m_stackedWidget->currentIndex() == 1) {
        m_lobbyWidget->updateRoomList(rooms);
    }
}

void OnlineGobang::onPlayerListReceived(const QJsonArray &players)
{
    if (m_stackedWidget->currentIndex() == 1) {
        m_lobbyWidget->updatePlayerList(players);
    }
}

void OnlineGobang::onPlayerJoinedRoom(const QString &nickname)
{
    m_gameWidget->setOpponentNickname(nickname);
}

// ===== 游戏相关 =====

void OnlineGobang::switchToGame()
{
    m_stackedWidget->setCurrentIndex(2);
}

void OnlineGobang::onGameStarted(const QString &blackId, const QString &whiteId, int yourColor)
{
    m_gameWidget->onGameStart(blackId, whiteId, yourColor);
}

void OnlineGobang::onTurnChanged(int color)
{
    m_gameWidget->onTurnChanged(color);
}

void OnlineGobang::onMoveResult(bool success, const QString &reason)
{
    if (!success) {
        QString msg;
        if (reason == "not_your_turn") msg = "还没轮到你";
        else if (reason == "position_occupied") msg = "该位置已有棋子";
        else if (reason == "out_of_bounds") msg = "超出棋盘范围";
        else if (reason == "game_not_started") msg = "游戏未开始";
        else msg = "落子失败: " + reason;
        statusBar()->showMessage(msg, 3000);
    }
}

void OnlineGobang::onMoveNotify(int row, int col, int color)
{
    m_gameWidget->onMoveNotify(row, col, color);
}

void OnlineGobang::onGameOver(int winner, const QString &reason, const QJsonArray &winLine)
{
    m_gameWidget->onGameOver(winner, reason, winLine);
}

void OnlineGobang::onChatReceived(const QString &from, const QString &message)
{
    m_gameWidget->onChatReceived(from, message);
}

void OnlineGobang::onOpponentLeft(const QString &reason)
{
    m_gameWidget->onOpponentLeft(reason);
}

void OnlineGobang::onUndoRequested(const QString &from)
{
    m_gameWidget->onUndoRequested(from);
}

void OnlineGobang::onUndoResult(bool agreed, int row, int col, int color)
{
    m_gameWidget->onUndoResult(agreed, row, col, color);
}

void OnlineGobang::onRestartRequested(const QString &from)
{
    m_gameWidget->onRestartRequested(from);
}

void OnlineGobang::onRestartReplyReceived(bool agreed)
{
    m_gameWidget->onRestartReply(agreed);
}

void OnlineGobang::onReadyNotifyReceived(const QString &nickname)
{
    m_gameWidget->onReadyNotify(nickname);
}

// ===== 游戏界面发出的请求 =====

void OnlineGobang::onMoveRequested(int row, int col)
{
    m_network->sendMove(row, col);
}

void OnlineGobang::onReadyRequested()
{
    m_network->ready();
}

void OnlineGobang::onSurrenderRequested()
{
    m_network->surrender();
}

void OnlineGobang::onUndoRequestedByMe()
{
    m_network->requestUndo();
}

void OnlineGobang::onUndoReplyRequested(bool agree)
{
    m_network->replyUndo(agree);
}

void OnlineGobang::onRestartRequestedByMe()
{
    m_network->requestRestart();
}

void OnlineGobang::onRestartReplyRequested(bool agree)
{
    m_network->replyRestart(agree);
}

void OnlineGobang::onLeaveRoomRequested()
{
    m_network->leaveRoom();
    m_gameWidget->resetGame();
    switchToLobby();
    m_inRoom = false;
}

void OnlineGobang::onChatRequested(const QString &message)
{
    m_network->sendChat(message);
    m_gameWidget->onChatReceived(m_nickname, message);
}
