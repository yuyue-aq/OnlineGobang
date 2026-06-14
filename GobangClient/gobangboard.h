#ifndef GOBANGBOARD_H
#define GOBANGBOARD_H

#include <QWidget>
#include <QVector>

class GobangBoard : public QWidget
{
    Q_OBJECT

public:
    static const int BOARD_SIZE = 15;

    explicit GobangBoard(QWidget *parent = nullptr);

    void setBoard(const int board[15][15]);
    void setMyColor(int color);
    void setCurrentTurn(int color);
    void setLastMove(int row, int col);
    void clearLastMove();
    void setWinLine(const QVector<QPair<int,int>> &line);
    void clearWinLine();
    void resetBoard();
    void setEnabledClick(bool enabled);

    QSize minimumSizeHint() const override;
    QSize sizeHint() const override;

signals:
    void cellClicked(int row, int col);

protected:
    void paintEvent(QPaintEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event) override;
    void mousePressEvent(QMouseEvent *event) override;
    void leaveEvent(QEvent *event) override;

private:
    void drawBackground(QPainter &painter);
    void drawGrid(QPainter &painter);
    void drawStarPoints(QPainter &painter);
    void drawStones(QPainter &painter);
    void drawLastMoveMarker(QPainter &painter);
    void drawWinLine(QPainter &painter);
    void drawHoverStone(QPainter &painter);

    QPair<int,int> pixelToBoard(int x, int y) const;
    QPair<int,int> boardToPixel(int row, int col) const;

    int m_board[15][15];
    int m_myColor;
    int m_currentTurn;
    int m_lastMoveRow;
    int m_lastMoveCol;
    bool m_hasLastMove;
    QVector<QPair<int,int>> m_winLine;
    int m_hoverRow;
    int m_hoverCol;
    bool m_clickEnabled;

    // 绘制参数
    int m_margin;
    int m_cellSize;
    int m_stoneRadius;
};

#endif // GOBANGBOARD_H
