#include "gobangboard.h"
#include <QPainter>
#include <QPainterPath>
#include <QMouseEvent>
#include "protocol.h"

GobangBoard::GobangBoard(QWidget *parent)
    : QWidget(parent)
    , m_myColor(ChessColor::NONE)
    , m_currentTurn(ChessColor::NONE)
    , m_lastMoveRow(-1)
    , m_lastMoveCol(-1)
    , m_hasLastMove(false)
    , m_hoverRow(-1)
    , m_hoverCol(-1)
    , m_clickEnabled(false)
    , m_margin(30)
    , m_cellSize(36)
    , m_stoneRadius(15)
{
    memset(m_board, 0, sizeof(m_board));
    setMouseTracking(true);
    setMinimumSize(560, 560);
}

void GobangBoard::setBoard(const int board[15][15])
{
    memcpy(m_board, board, sizeof(m_board));
    update();
}

void GobangBoard::setMyColor(int color)
{
    m_myColor = color;
    update();
}

void GobangBoard::setCurrentTurn(int color)
{
    m_currentTurn = color;
    m_clickEnabled = (color == m_myColor && m_myColor != ChessColor::NONE);
    update();
}

void GobangBoard::setLastMove(int row, int col)
{
    m_lastMoveRow = row;
    m_lastMoveCol = col;
    m_hasLastMove = true;
    update();
}

void GobangBoard::clearLastMove()
{
    m_hasLastMove = false;
    m_lastMoveRow = -1;
    m_lastMoveCol = -1;
    update();
}

void GobangBoard::setWinLine(const QVector<QPair<int,int>> &line)
{
    m_winLine = line;
    update();
}

void GobangBoard::clearWinLine()
{
    m_winLine.clear();
    update();
}

void GobangBoard::resetBoard()
{
    memset(m_board, 0, sizeof(m_board));
    m_myColor = ChessColor::NONE;
    m_currentTurn = ChessColor::NONE;
    m_hasLastMove = false;
    m_winLine.clear();
    m_hoverRow = -1;
    m_hoverCol = -1;
    m_clickEnabled = false;
    update();
}

void GobangBoard::setEnabledClick(bool enabled)
{
    m_clickEnabled = enabled;
}

QSize GobangBoard::minimumSizeHint() const
{
    return QSize(560, 560);
}

QSize GobangBoard::sizeHint() const
{
    return QSize(600, 600);
}

void GobangBoard::paintEvent(QPaintEvent *event)
{
    Q_UNUSED(event)

    // 根据控件大小计算绘制参数
    int w = width();
    int h = height();
    int size = qMin(w, h);
    m_margin = size / 18;
    m_cellSize = (size - 2 * m_margin) / (BOARD_SIZE - 1);
    m_stoneRadius = m_cellSize * 2 / 5;

    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);

    drawBackground(painter);
    drawGrid(painter);
    drawStarPoints(painter);
    drawStones(painter);
    if (m_hasLastMove) drawLastMoveMarker(painter);
    if (!m_winLine.isEmpty()) drawWinLine(painter);
    if (m_hoverRow >= 0) drawHoverStone(painter);
}

void GobangBoard::drawBackground(QPainter &painter)
{
    // 木纹色背景
    painter.fillRect(rect(), QColor(220, 179, 92));
}

void GobangBoard::drawGrid(QPainter &painter)
{
    QPen pen(Qt::black, 1);
    painter.setPen(pen);

    for (int i = 0; i < BOARD_SIZE; i++) {
        auto [x1, y1] = boardToPixel(i, 0);
        auto [x2, y2] = boardToPixel(i, BOARD_SIZE - 1);
        painter.drawLine(x1, y1, x2, y2);

        auto [x3, y3] = boardToPixel(0, i);
        auto [x4, y4] = boardToPixel(BOARD_SIZE - 1, i);
        painter.drawLine(x3, y3, x4, y4);
    }

    // 坐标标注
    painter.setPen(QColor(80, 50, 20));
    QFont font = painter.font();
    font.setPointSize(8);
    painter.setFont(font);

    for (int i = 0; i < BOARD_SIZE; i++) {
        auto [x, y] = boardToPixel(i, 0);
        painter.drawText(x - 4, y - m_cellSize / 2 - 2, QString(QChar('A' + i)));
        auto [x2, y2] = boardToPixel(0, i);
        painter.drawText(x2 - m_cellSize / 2 - 16, y2 + 4, QString::number(BOARD_SIZE - i));
    }
}

void GobangBoard::drawStarPoints(QPainter &painter)
{
    painter.setBrush(Qt::black);
    painter.setPen(Qt::NoPen);

    int starPoints[] = {3, 7, 11};
    for (int r : starPoints) {
        for (int c : starPoints) {
            auto [x, y] = boardToPixel(r, c);
            painter.drawEllipse(QPoint(x, y), 3, 3);
        }
    }
}

void GobangBoard::drawStones(QPainter &painter)
{
    for (int r = 0; r < BOARD_SIZE; r++) {
        for (int c = 0; c < BOARD_SIZE; c++) {
            if (m_board[r][c] == ChessColor::NONE) continue;

            auto [x, y] = boardToPixel(r, c);

            // 阴影
            painter.setPen(Qt::NoPen);
            painter.setBrush(QColor(0, 0, 0, 40));
            painter.drawEllipse(QPoint(x + 2, y + 2), m_stoneRadius, m_stoneRadius);

            if (m_board[r][c] == ChessColor::BLACK) {
                // 黑子渐变
                QRadialGradient gradient(x - m_stoneRadius / 3, y - m_stoneRadius / 3,
                                         m_stoneRadius * 1.5);
                gradient.setColorAt(0, QColor(80, 80, 80));
                gradient.setColorAt(1, QColor(10, 10, 10));
                painter.setBrush(gradient);
                painter.setPen(Qt::NoPen);
                painter.drawEllipse(QPoint(x, y), m_stoneRadius, m_stoneRadius);
            } else {
                // 白子渐变
                QRadialGradient gradient(x - m_stoneRadius / 3, y - m_stoneRadius / 3,
                                         m_stoneRadius * 1.5);
                gradient.setColorAt(0, QColor(255, 255, 255));
                gradient.setColorAt(1, QColor(210, 210, 210));
                painter.setBrush(gradient);
                painter.setPen(QPen(QColor(150, 150, 150), 1));
                painter.drawEllipse(QPoint(x, y), m_stoneRadius, m_stoneRadius);
            }
        }
    }
}

void GobangBoard::drawLastMoveMarker(QPainter &painter)
{
    auto [x, y] = boardToPixel(m_lastMoveRow, m_lastMoveCol);
    int color = m_board[m_lastMoveRow][m_lastMoveCol];
    painter.setPen(Qt::NoPen);
    painter.setBrush(color == ChessColor::BLACK ? Qt::red : Qt::red);
    painter.drawEllipse(QPoint(x, y), 4, 4);
}

void GobangBoard::drawWinLine(QPainter &painter)
{
    if (m_winLine.size() < 2) return;

    QPen pen(QColor(255, 0, 0, 180), 4);
    painter.setPen(pen);

    auto [x1, y1] = boardToPixel(m_winLine.first().first, m_winLine.first().second);
    auto [x2, y2] = boardToPixel(m_winLine.last().first, m_winLine.last().second);
    painter.drawLine(x1, y1, x2, y2);
}

void GobangBoard::drawHoverStone(QPainter &painter)
{
    if (!m_clickEnabled) return;
    if (m_board[m_hoverRow][m_hoverCol] != ChessColor::NONE) return;

    auto [x, y] = boardToPixel(m_hoverRow, m_hoverCol);

    QColor stoneColor = (m_currentTurn == ChessColor::BLACK)
        ? QColor(0, 0, 0, 80) : QColor(255, 255, 255, 80);
    painter.setPen(Qt::NoPen);
    painter.setBrush(stoneColor);
    painter.drawEllipse(QPoint(x, y), m_stoneRadius, m_stoneRadius);
}

void GobangBoard::mouseMoveEvent(QMouseEvent *event)
{
    auto [row, col] = pixelToBoard(event->pos().x(), event->pos().y());
    if (row != m_hoverRow || col != m_hoverCol) {
        m_hoverRow = row;
        m_hoverCol = col;
        update();
    }
}

void GobangBoard::mousePressEvent(QMouseEvent *event)
{
    if (!m_clickEnabled) return;
    if (event->button() != Qt::LeftButton) return;

    auto [row, col] = pixelToBoard(event->pos().x(), event->pos().y());
    if (row >= 0 && row < BOARD_SIZE && col >= 0 && col < BOARD_SIZE) {
        if (m_board[row][col] == ChessColor::NONE) {
            emit cellClicked(row, col);
        }
    }
}

void GobangBoard::leaveEvent(QEvent *event)
{
    Q_UNUSED(event)
    m_hoverRow = -1;
    m_hoverCol = -1;
    update();
}

QPair<int,int> GobangBoard::pixelToBoard(int x, int y) const
{
    int col = qRound(static_cast<double>(x - m_margin) / m_cellSize);
    int row = qRound(static_cast<double>(y - m_margin) / m_cellSize);
    if (row < 0) row = 0;
    if (row >= BOARD_SIZE) row = BOARD_SIZE - 1;
    if (col < 0) col = 0;
    if (col >= BOARD_SIZE) col = BOARD_SIZE - 1;
    return {row, col};
}

QPair<int,int> GobangBoard::boardToPixel(int row, int col) const
{
    int x = m_margin + col * m_cellSize;
    int y = m_margin + row * m_cellSize;
    return {x, y};
}
