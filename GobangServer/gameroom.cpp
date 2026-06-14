#include "gameroom.h"
#include "clientsession.h"
#include <QJsonObject>
#include <QJsonArray>

GameRoom::GameRoom(const QString &roomId, const QString &roomName,
                   ClientSession *creator, QObject *parent)
    : QObject(parent)
    , m_roomId(roomId)
    , m_roomName(roomName)
    , m_player1(nullptr)
    , m_player2(nullptr)
    , m_currentTurn(ChessColor::NONE)
    , m_gameStarted(false)
    , m_gameOver(false)
    , m_player1Ready(false)
    , m_player2Ready(false)
    , m_undoPending(false)
    , m_undoRequesterId(-1)
    , m_restartPending(false)
    , m_restartRequesterId(-1)
{
    memset(m_board, 0, sizeof(m_board));
}

GameRoom::~GameRoom() {}

int GameRoom::playerCount() const
{
    int count = 0;
    if (m_player1) count++;
    if (m_player2) count++;
    return count;
}

bool GameRoom::addPlayer(ClientSession *player)
{
    if (isFull()) return false;

    if (!m_player1) {
        m_player1 = player;
    } else {
        m_player2 = player;
    }
    player->setRoomId(m_roomId);
    return true;
}

void GameRoom::removePlayer(ClientSession *player)
{
    if (m_player1 == player) {
        m_player1 = nullptr;
        m_player1Ready = false;
    } else if (m_player2 == player) {
        m_player2 = nullptr;
        m_player2Ready = false;
    }
    player->setRoomId({});
}

void GameRoom::handleReady(ClientSession *player)
{
    if (m_gameStarted) return;

    if (player == m_player1) {
        m_player1Ready = true;
    } else if (player == m_player2) {
        m_player2Ready = true;
    }

    // 通知对手
    ClientSession *opponent = getOpponent(player);
    if (opponent) {
        sendToPlayer(opponent, Protocol::readyNotify(player->nickname()));
    }

    // 双方都准备则开始
    if (m_player1Ready && m_player2Ready && m_player1 && m_player2) {
        startGame();
    }
}

void GameRoom::handleMove(ClientSession *player, int row, int col)
{
    if (!m_gameStarted || m_gameOver) {
        sendToPlayer(player, Protocol::moveResp(false, "game_not_started"));
        return;
    }

    int color = getPlayerColor(player);
    if (color == ChessColor::NONE) {
        sendToPlayer(player, Protocol::moveResp(false, "not_in_game"));
        return;
    }

    if (color != m_currentTurn) {
        sendToPlayer(player, Protocol::moveResp(false, "not_your_turn"));
        return;
    }

    if (row < 0 || row >= BOARD_SIZE || col < 0 || col >= BOARD_SIZE) {
        sendToPlayer(player, Protocol::moveResp(false, "out_of_bounds"));
        return;
    }

    if (m_board[row][col] != ChessColor::NONE) {
        sendToPlayer(player, Protocol::moveResp(false, "position_occupied"));
        return;
    }

    // 落子
    m_board[row][col] = color;
    m_moveHistory.append({row, col, color});
    sendToPlayer(player, Protocol::moveResp(true));

    // 通知双方
    broadcastMessage(Protocol::moveNotify(row, col, color));

    // 检查胜负
    if (checkWin(row, col, color)) {
        m_gameOver = true;
        // 构造胜利连线
        QJsonArray winLine;
        // checkWin 已确认胜利，构造五连坐标
        int dirs[4][2] = {{0,1},{1,0},{1,1},{1,-1}};
        for (int d = 0; d < 4; d++) {
            int dr = dirs[d][0], dc = dirs[d][1];
            int count = 1;
            QJsonArray line;
            line.append(QJsonArray{row, col});
            // 正方向
            for (int i = 1; i < 5; i++) {
                int r = row + dr * i, c = col + dc * i;
                if (r >= 0 && r < BOARD_SIZE && c >= 0 && c < BOARD_SIZE && m_board[r][c] == color) {
                    count++;
                    line.append(QJsonArray{r, c});
                } else break;
            }
            // 反方向
            for (int i = 1; i < 5; i++) {
                int r = row - dr * i, c = col - dc * i;
                if (r >= 0 && r < BOARD_SIZE && c >= 0 && c < BOARD_SIZE && m_board[r][c] == color) {
                    count++;
                    line.prepend(QJsonArray{r, c});
                } else break;
            }
            if (count >= 5) {
                winLine = line;
                break;
            }
        }
        broadcastMessage(Protocol::gameOver(color, "five_in_a_row", winLine));
        return;
    }

    if (checkDraw()) {
        m_gameOver = true;
        broadcastMessage(Protocol::gameOver(ChessColor::NONE, "draw"));
        return;
    }

    // 切换回合
    m_currentTurn = (color == ChessColor::BLACK) ? ChessColor::WHITE : ChessColor::BLACK;
    broadcastMessage(Protocol::turnNotify(m_currentTurn));
}

void GameRoom::handleSurrender(ClientSession *player)
{
    if (!m_gameStarted || m_gameOver) return;

    int color = getPlayerColor(player);
    if (color == ChessColor::NONE) return;

    m_gameOver = true;
    int winner = (color == ChessColor::BLACK) ? ChessColor::WHITE : ChessColor::BLACK;
    broadcastMessage(Protocol::gameOver(winner, "surrender"));
}

void GameRoom::handleUndoRequest(ClientSession *player)
{
    if (!m_gameStarted || m_gameOver) return;
    if (m_undoPending) return;  // 已有悔棋请求待处理

    // 检查是否有棋可悔
    if (m_moveHistory.isEmpty()) return;

    // 只有刚下完棋的对手才能被请求悔棋（即当前回合的对手上一手下的棋）
    // 简化：任何人都可以请求悔最后一手
    m_undoPending = true;
    m_undoRequesterId = player->userId();

    ClientSession *opponent = getOpponent(player);
    if (opponent) {
        sendToPlayer(opponent, Protocol::undoNotify(player->nickname()));
    }
}

void GameRoom::handleUndoReply(ClientSession *player, bool agree)
{
    if (!m_undoPending) return;
    m_undoPending = false;

    ClientSession *requester = nullptr;
    if (m_player1 && m_player1->userId() == m_undoRequesterId) requester = m_player1;
    else if (m_player2 && m_player2->userId() == m_undoRequesterId) requester = m_player2;

    if (!requester) return;

    if (agree && !m_moveHistory.isEmpty()) {
        MoveRecord last = m_moveHistory.takeLast();
        m_board[last.row][last.col] = ChessColor::NONE;
        m_currentTurn = last.color;  // 回到被悔棋方的回合

        broadcastMessage(Protocol::undoResult(true, last.row, last.col, last.color));
    } else {
        broadcastMessage(Protocol::undoResult(false));
    }
}

void GameRoom::handleRestartRequest(ClientSession *player)
{
    if (!m_gameOver) return;
    if (m_restartPending) return;

    m_restartPending = true;
    m_restartRequesterId = player->userId();

    ClientSession *opponent = getOpponent(player);
    if (opponent) {
        sendToPlayer(opponent, Protocol::restartNotify(player->nickname()));
    }
}

void GameRoom::handleRestartReply(ClientSession *player, bool agree)
{
    if (!m_restartPending) return;
    m_restartPending = false;

    ClientSession *requester = nullptr;
    if (m_player1 && m_player1->userId() == m_restartRequesterId) requester = m_player1;
    else if (m_player2 && m_player2->userId() == m_restartRequesterId) requester = m_player2;

    if (!requester) return;

    if (agree) {
        // 交换黑白方
        ClientSession *temp = m_player1;
        m_player1 = m_player2;
        m_player2 = temp;
        m_player1Ready = true;
        m_player2Ready = true;
        startGame();
    } else {
        // 通知双方再来一局被拒绝，重置准备状态
        m_player1Ready = false;
        m_player2Ready = false;
        broadcastMessage(Protocol::restartReplyResp(false));
    }
}

void GameRoom::handleChat(ClientSession *player, const QString &message)
{
    ClientSession *opponent = getOpponent(player);
    if (opponent) {
        sendToPlayer(opponent, Protocol::chatNotify(player->nickname(), message));
    }
}

void GameRoom::handlePlayerLeave(ClientSession *player)
{
    if (!m_gameStarted || m_gameOver) {
        removePlayer(player);
        return;
    }

    // 游戏中离开，判对手胜利
    m_gameOver = true;
    int winner = getPlayerColor(player) == ChessColor::BLACK ? ChessColor::WHITE : ChessColor::BLACK;

    ClientSession *opponent = getOpponent(player);
    removePlayer(player);

    if (opponent) {
        sendToPlayer(opponent, Protocol::opponentLeft("disconnect"));
        sendToPlayer(opponent, Protocol::gameOver(winner, "opponent_left"));
    }
}

QJsonObject GameRoom::roomInfo() const
{
    QJsonObject info;
    info["roomId"] = m_roomId;
    info["roomName"] = m_roomName;
    info["playerCount"] = playerCount();
    info["gameStarted"] = m_gameStarted;
    return info;
}

void GameRoom::startGame()
{
    memset(m_board, 0, sizeof(m_board));
    m_moveHistory.clear();
    m_currentTurn = ChessColor::BLACK;
    m_gameStarted = true;
    m_gameOver = false;
    m_undoPending = false;
    m_restartPending = false;

    // player1 = 黑方, player2 = 白方
    sendToPlayer(m_player1, Protocol::gameStart(
        m_player1->nickname(), m_player2->nickname(), ChessColor::BLACK));
    sendToPlayer(m_player2, Protocol::gameStart(
        m_player1->nickname(), m_player2->nickname(), ChessColor::WHITE));
}

void GameRoom::resetGame()
{
    memset(m_board, 0, sizeof(m_board));
    m_moveHistory.clear();
    m_currentTurn = ChessColor::NONE;
    m_gameStarted = false;
    m_gameOver = false;
    m_player1Ready = false;
    m_player2Ready = false;
    m_undoPending = false;
    m_restartPending = false;
}

bool GameRoom::checkWin(int row, int col, int color)
{
    int dirs[4][2] = {{0,1}, {1,0}, {1,1}, {1,-1}};
    for (int d = 0; d < 4; d++) {
        int count = 1;
        int dr = dirs[d][0], dc = dirs[d][1];
        // 正方向
        for (int i = 1; i < 5; i++) {
            int r = row + dr * i, c = col + dc * i;
            if (r >= 0 && r < BOARD_SIZE && c >= 0 && c < BOARD_SIZE && m_board[r][c] == color)
                count++;
            else break;
        }
        // 反方向
        for (int i = 1; i < 5; i++) {
            int r = row - dr * i, c = col - dc * i;
            if (r >= 0 && r < BOARD_SIZE && c >= 0 && c < BOARD_SIZE && m_board[r][c] == color)
                count++;
            else break;
        }
        if (count >= 5) return true;
    }
    return false;
}

bool GameRoom::checkDraw()
{
    for (int r = 0; r < BOARD_SIZE; r++)
        for (int c = 0; c < BOARD_SIZE; c++)
            if (m_board[r][c] == ChessColor::NONE) return false;
    return true;
}

void GameRoom::broadcastMessage(const QJsonObject &msg)
{
    if (m_player1) m_player1->sendMessage(msg);
    if (m_player2) m_player2->sendMessage(msg);
}

void GameRoom::sendToPlayer(ClientSession *player, const QJsonObject &msg)
{
    if (player) player->sendMessage(msg);
}

ClientSession *GameRoom::getOpponent(ClientSession *player) const
{
    if (player == m_player1) return m_player2;
    if (player == m_player2) return m_player1;
    return nullptr;
}

int GameRoom::getPlayerColor(ClientSession *player) const
{
    if (player == m_player1) return ChessColor::BLACK;
    if (player == m_player2) return ChessColor::WHITE;
    return ChessColor::NONE;
}
