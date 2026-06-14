#ifndef GAMEWIDGET_H
#define GAMEWIDGET_H

#include <QWidget>
#include <QJsonArray>
#include "protocol.h"

namespace Ui {
class GameWidget;
}

class GobangBoard;

class GameWidget : public QWidget
{
    Q_OBJECT

public:
    explicit GameWidget(QWidget *parent = nullptr);
    ~GameWidget();

    void setMyColor(int color);
    void setMyNickname(const QString &nickname);
    void setOpponentNickname(const QString &nickname);
    void onGameStart(const QString &blackId, const QString &whiteId, int yourColor);
    void onTurnChanged(int color);
    void onMoveNotify(int row, int col, int color);
    void onGameOver(int winner, const QString &reason, const QJsonArray &winLine);
    void onChatReceived(const QString &from, const QString &message);
    void onOpponentLeft(const QString &reason);
    void onUndoRequested(const QString &from);
    void onUndoResult(bool agreed, int row, int col, int color);
    void onRestartRequested(const QString &from);
    void onRestartReply(bool agreed);
    void onReadyNotify(const QString &nickname);
    void resetGame();

signals:
    void moveRequested(int row, int col);
    void readyRequested();
    void surrenderRequested();
    void undoRequested();
    void undoReplyRequested(bool agree);
    void restartRequested();
    void restartReplyRequested(bool agree);
    void leaveRoomRequested();
    void chatRequested(const QString &message);

private slots:
    void onReadyClicked();
    void onSurrenderClicked();
    void onUndoClicked();
    void onRestartClicked();
    void onLeaveClicked();
    void onSendChat();
    void onCellClicked(int row, int col);

private:
    void updateTurnLabel();
    void appendChatMessage(const QString &from, const QString &message);

    Ui::GameWidget *ui;
    GobangBoard *m_board;
    int m_myColor;
    int m_currentTurn;
    QString m_myNickname;
    QString m_opponentNickname;
    bool m_gameStarted;
    bool m_gameOver;
    int m_boardData[15][15];
};

#endif // GAMEWIDGET_H
