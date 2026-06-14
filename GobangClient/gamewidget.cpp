#include "gamewidget.h"
#include "ui_gamewidget.h"
#include "gobangboard.h"
#include <QMessageBox>

GameWidget::GameWidget(QWidget *parent)
    : QWidget(parent)
    , ui(new Ui::GameWidget)
    , m_board(nullptr)
    , m_myColor(ChessColor::NONE)
    , m_currentTurn(ChessColor::NONE)
    , m_gameStarted(false)
    , m_gameOver(false)
{
    ui->setupUi(this);

    // 创建棋盘控件并插入布局
    m_board = new GobangBoard(this);
    ui->boardContainer->layout()->addWidget(m_board);

    memset(m_boardData, 0, sizeof(m_boardData));

    // 连接信号
    connect(m_board, &GobangBoard::cellClicked, this, &GameWidget::onCellClicked);
    connect(ui->readyButton, &QPushButton::clicked, this, &GameWidget::onReadyClicked);
    connect(ui->surrenderButton, &QPushButton::clicked, this, &GameWidget::onSurrenderClicked);
    connect(ui->undoButton, &QPushButton::clicked, this, &GameWidget::onUndoClicked);
    connect(ui->restartButton, &QPushButton::clicked, this, &GameWidget::onRestartClicked);
    connect(ui->leaveButton, &QPushButton::clicked, this, &GameWidget::onLeaveClicked);
    connect(ui->chatSendButton, &QPushButton::clicked, this, &GameWidget::onSendChat);
    connect(ui->chatInput, &QLineEdit::returnPressed, this, &GameWidget::onSendChat);

    // 初始按钮状态
    ui->surrenderButton->setEnabled(false);
    ui->undoButton->setEnabled(false);
    ui->restartButton->setEnabled(false);
    ui->readyButton->setEnabled(true);
    ui->turnLabel->setText("等待准备...");
}

GameWidget::~GameWidget()
{
    delete ui;
}

void GameWidget::setMyColor(int color)
{
    m_myColor = color;
    ui->myColorLabel->setText(color == ChessColor::BLACK ? "⚫ 黑方" :
                               color == ChessColor::WHITE ? "⚪ 白方" : "");
}

void GameWidget::setMyNickname(const QString &nickname)
{
    m_myNickname = nickname;
    ui->myNameLabel->setText(nickname);
}

void GameWidget::setOpponentNickname(const QString &nickname)
{
    m_opponentNickname = nickname;
    ui->opponentNameLabel->setText(nickname);
}

void GameWidget::onGameStart(const QString &blackId, const QString &whiteId, int yourColor)
{
    m_gameStarted = true;
    m_gameOver = false;
    memset(m_boardData, 0, sizeof(m_boardData));
    m_board->resetBoard();
    m_board->setBoard(m_boardData);
    m_board->setMyColor(yourColor);
    setMyColor(yourColor);

    // 判断对手昵称
    if (yourColor == ChessColor::BLACK) {
        setOpponentNickname(whiteId);
    } else {
        setOpponentNickname(blackId);
    }

    m_currentTurn = ChessColor::BLACK;
    m_board->setCurrentTurn(ChessColor::BLACK);

    ui->readyButton->setEnabled(false);
    ui->surrenderButton->setEnabled(true);
    ui->restartButton->setEnabled(false);
    ui->leaveButton->setEnabled(false);

    updateTurnLabel();
    appendChatMessage("系统", "游戏开始！黑方先行。");
}

void GameWidget::onTurnChanged(int color)
{
    m_currentTurn = color;
    m_board->setCurrentTurn(color);
    updateTurnLabel();
}

void GameWidget::onMoveNotify(int row, int col, int color)
{
    m_boardData[row][col] = color;
    m_board->setBoard(m_boardData);
    m_board->setLastMove(row, col);

    // 检查是否是自己的回合，控制悔棋按钮
    ui->undoButton->setEnabled(m_gameStarted && !m_gameOver);
}

void GameWidget::onGameOver(int winner, const QString &reason, const QJsonArray &winLine)
{
    m_gameOver = true;
    m_board->setEnabledClick(false);

    // 显示胜利连线
    if (!winLine.isEmpty()) {
        QVector<QPair<int,int>> line;
        for (int i = 0; i < winLine.size(); i++) {
            QJsonArray pt = winLine[i].toArray();
            if (pt.size() >= 2) {
                line.append({pt[0].toInt(), pt[1].toInt()});
            }
        }
        m_board->setWinLine(line);
    }

    ui->surrenderButton->setEnabled(false);
    ui->undoButton->setEnabled(false);
    ui->restartButton->setEnabled(true);
    ui->leaveButton->setEnabled(true);

    QString winnerName;
    if (winner == ChessColor::NONE) {
        winnerName = "平局";
        appendChatMessage("系统", "游戏结束，平局！");
    } else {
        bool iWin = (winner == m_myColor);
        winnerName = iWin ? "你赢了！" : "你输了...";
        QString reasonText;
        if (reason == "five_in_a_row") reasonText = "五连珠";
        else if (reason == "surrender") reasonText = "对手认输";
        else if (reason == "opponent_left") reasonText = "对手掉线";
        else reasonText = reason;
        appendChatMessage("系统", QString("游戏结束 - %1 (%2)").arg(winnerName).arg(reasonText));
    }

    ui->turnLabel->setText(winnerName);
}

void GameWidget::onChatReceived(const QString &from, const QString &message)
{
    appendChatMessage(from, message);
}

void GameWidget::onOpponentLeft(const QString &reason)
{
    appendChatMessage("系统", QString("对手已离开 (%1)").arg(reason));
    ui->leaveButton->setEnabled(true);
}

void GameWidget::onUndoRequested(const QString &from)
{
    int ret = QMessageBox::question(this, "悔棋请求",
        QString("%1 请求悔棋，是否同意？").arg(from),
        QMessageBox::Yes | QMessageBox::No, QMessageBox::No);
    emit undoReplyRequested(ret == QMessageBox::Yes);
}

void GameWidget::onUndoResult(bool agreed, int row, int col, int color)
{
    if (agreed && row >= 0 && col >= 0) {
        m_boardData[row][col] = ChessColor::NONE;
        m_board->setBoard(m_boardData);
        m_board->clearLastMove();
        appendChatMessage("系统", "悔棋成功");
    } else {
        appendChatMessage("系统", "悔棋被拒绝");
    }
}

void GameWidget::onRestartRequested(const QString &from)
{
    int ret = QMessageBox::question(this, "再来一局",
        QString("%1 请求再来一局，是否同意？").arg(from),
        QMessageBox::Yes | QMessageBox::No, QMessageBox::No);
    emit restartReplyRequested(ret == QMessageBox::Yes);
}

void GameWidget::onRestartReply(bool agreed)
{
    if (!agreed) {
        appendChatMessage("系统", "再来一局请求被拒绝");
        ui->readyButton->setEnabled(true);
        ui->readyButton->setText("准备");
    }
}

void GameWidget::onReadyNotify(const QString &nickname)
{
    appendChatMessage("系统", QString("%1 已准备").arg(nickname));
}

void GameWidget::resetGame()
{
    m_gameStarted = false;
    m_gameOver = false;
    m_myColor = ChessColor::NONE;
    m_currentTurn = ChessColor::NONE;
    memset(m_boardData, 0, sizeof(m_boardData));
    m_board->resetBoard();

    ui->readyButton->setEnabled(true);
    ui->surrenderButton->setEnabled(false);
    ui->undoButton->setEnabled(false);
    ui->restartButton->setEnabled(false);
    ui->leaveButton->setEnabled(true);
    ui->turnLabel->setText("等待准备...");
    ui->myColorLabel->setText("");
    ui->opponentNameLabel->setText("等待对手加入...");
    ui->chatDisplay->clear();
}

void GameWidget::updateTurnLabel()
{
    if (!m_gameStarted) {
        ui->turnLabel->setText("等待准备...");
        return;
    }

    bool myTurn = (m_currentTurn == m_myColor);
    if (myTurn) {
        ui->turnLabel->setText("🎯 轮到你了");
        ui->turnLabel->setStyleSheet("color: green; font-weight: bold; font-size: 14px;");
    } else {
        ui->turnLabel->setText("对手思考中...");
        ui->turnLabel->setStyleSheet("color: gray; font-size: 14px;");
    }
}

void GameWidget::appendChatMessage(const QString &from, const QString &message)
{
    ui->chatDisplay->append(QString("<b>%1:</b> %2").arg(from).arg(message));
}

void GameWidget::onReadyClicked()
{
    ui->readyButton->setEnabled(false);
    ui->readyButton->setText("已准备");
    emit readyRequested();
}

void GameWidget::onSurrenderClicked()
{
    int ret = QMessageBox::question(this, "认输",
        "确定要认输吗？", QMessageBox::Yes | QMessageBox::No, QMessageBox::No);
    if (ret == QMessageBox::Yes) {
        emit surrenderRequested();
    }
}

void GameWidget::onUndoClicked()
{
    emit undoRequested();
}

void GameWidget::onRestartClicked()
{
    emit restartRequested();
}

void GameWidget::onLeaveClicked()
{
    emit leaveRoomRequested();
}

void GameWidget::onSendChat()
{
    QString text = ui->chatInput->text().trimmed();
    if (text.isEmpty()) return;
    ui->chatInput->clear();
    emit chatRequested(text);
}

void GameWidget::onCellClicked(int row, int col)
{
    if (m_gameStarted && !m_gameOver && m_currentTurn == m_myColor) {
        emit moveRequested(row, col);
    }
}
