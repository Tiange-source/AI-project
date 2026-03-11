#include "boardwidget.h"
#include <QPainter>
#include <QMouseEvent>
#include <QDebug>

BoardWidget::BoardWidget(QWidget* parent)
    : QWidget(parent)
    , hasHover_(false)
    , enabled_(true)
    , lastMoveRow_(-1)
    , lastMoveCol_(-1)
{
    // 初始化棋盘
    reset();

    // 设置鼠标跟踪
    setMouseTracking(true);

    // 设置最小大小
    setMinimumSize(BOARD_SIZE * GRID_SIZE + BOARD_OFFSET * 2 + 60,
                   BOARD_SIZE * GRID_SIZE + BOARD_OFFSET * 2 + 60);

    // 设置背景颜色
    setStyleSheet("background-color: #DEB887;");
}

BoardWidget::~BoardWidget()
{
}

void BoardWidget::reset()
{
    for (int i = 0; i < BOARD_SIZE; ++i) {
        for (int j = 0; j < BOARD_SIZE; ++j) {
            board_[i][j] = 0;
        }
    }
    lastMoveRow_ = -1;
    lastMoveCol_ = -1;
    update();
}

void BoardWidget::setPiece(int row, int col, int player)
{
    if (isValidPosition(row, col) && board_[row][col] == 0) {
        board_[row][col] = player;
        lastMoveRow_ = row;
        lastMoveCol_ = col;
        update();
    }
}

void BoardWidget::setEnabled(bool enabled)
{
    enabled_ = enabled;
    update();
}

int BoardWidget::getPiece(int row, int col) const
{
    if (isValidPosition(row, col)) {
        return board_[row][col];
    }
    return 0;
}

void BoardWidget::paintEvent(QPaintEvent* event)
{
    Q_UNUSED(event);

    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);

    // 绘制背景
    painter.fillRect(rect(), QColor(222, 184, 135));

    // 绘制棋盘元素
    drawGrid(painter);
    drawStars(painter);
    drawPieces(painter);
    drawHover(painter);
    drawLabels(painter);
}

void BoardWidget::drawGrid(QPainter& painter)
{
    QPen gridPen(QColor(0, 0, 0));
    gridPen.setWidth(1);
    painter.setPen(gridPen);

    // 绘制横线
    for (int i = 0; i < BOARD_SIZE; ++i) {
        QPoint start(BOARD_OFFSET, BOARD_OFFSET + i * GRID_SIZE);
        QPoint end(BOARD_OFFSET + (BOARD_SIZE - 1) * GRID_SIZE, BOARD_OFFSET + i * GRID_SIZE);
        painter.drawLine(start, end);
    }

    // 绘制竖线
    for (int i = 0; i < BOARD_SIZE; ++i) {
        QPoint start(BOARD_OFFSET + i * GRID_SIZE, BOARD_OFFSET);
        QPoint end(BOARD_OFFSET + i * GRID_SIZE, BOARD_OFFSET + (BOARD_SIZE - 1) * GRID_SIZE);
        painter.drawLine(start, end);
    }
}

void BoardWidget::drawStars(QPainter& painter)
{
    // 星位点坐标：(3,3), (3,9), (9,9), (15,9), (15,3)
    int stars[][2] = {{3, 3}, {3, 9}, {9, 9}, {15, 9}, {15, 3}};

    painter.setPen(Qt::NoPen);
    painter.setBrush(QColor(0, 0, 0));

    for (int i = 0; i < 5; ++i) {
        int row = stars[i][0];
        int col = stars[i][1];
        QPoint center = gridToScreen(row, col);
        painter.drawEllipse(center, 4, 4);
    }
}

void BoardWidget::drawPieces(QPainter& painter)
{
    for (int i = 0; i < BOARD_SIZE; ++i) {
        for (int j = 0; j < BOARD_SIZE; ++j) {
            if (board_[i][j] != 0) {
                QPoint center = gridToScreen(i, j);

                // 绘制棋子阴影
                painter.setPen(Qt::NoPen);
                painter.setBrush(QColor(0, 0, 0, 50));
                painter.drawEllipse(center.x() + 2, center.y() + 2, CHESS_RADIUS * 2, CHESS_RADIUS * 2);

                // 绘制棋子
                if (board_[i][j] == 1) {
                    // 黑棋
                    QRadialGradient blackGradient(center, CHESS_RADIUS);
                    blackGradient.setColorAt(0, QColor(80, 80, 80));
                    blackGradient.setColorAt(1, QColor(0, 0, 0));
                    painter.setBrush(blackGradient);
                } else {
                    // 白棋
                    QRadialGradient whiteGradient(center, CHESS_RADIUS);
                    whiteGradient.setColorAt(0, QColor(255, 255, 255));
                    whiteGradient.setColorAt(1, QColor(220, 220, 220));
                    painter.setBrush(whiteGradient);
                }

                painter.drawEllipse(center, CHESS_RADIUS, CHESS_RADIUS);

                // 绘制最后一步标记
                if (i == lastMoveRow_ && j == lastMoveCol_) {
                    painter.setPen(QPen(QColor(255, 0, 0), 2));
                    painter.setBrush(Qt::NoBrush);
                    painter.drawEllipse(center, CHESS_RADIUS + 3, CHESS_RADIUS + 3);
                }
            }
        }
    }
}

void BoardWidget::drawHover(QPainter& painter)
{
    if (hasHover_ && enabled_) {
        QPoint gridPos = screenToGrid(hoverPos_);
        if (isValidPosition(gridPos.x(), gridPos.y()) && board_[gridPos.x()][gridPos.y()] == 0) {
            QPoint center = gridToScreen(gridPos.x(), gridPos.y());
            painter.setPen(Qt::NoPen);
            painter.setBrush(QColor(0, 0, 0, 100));
            painter.drawEllipse(center, CHESS_RADIUS, CHESS_RADIUS);
        }
    }
}

void BoardWidget::drawLabels(QPainter& painter)
{
    painter.setPen(QColor(0, 0, 0));
    painter.setFont(QFont("Arial", 10));

    // 绘制行号（左侧）
    for (int i = 0; i < BOARD_SIZE; ++i) {
        QString label = QString::number(BOARD_SIZE - i);
        QPoint pos(BOARD_OFFSET - 25, BOARD_OFFSET + i * GRID_SIZE + 5);
        painter.drawText(pos, label);
    }

    // 绘制列号（底部）
    for (int i = 0; i < BOARD_SIZE; ++i) {
        QString label = QString::number(i + 1);
        QPoint pos(BOARD_OFFSET + i * GRID_SIZE - 5, BOARD_OFFSET + (BOARD_SIZE - 1) * GRID_SIZE + 40);
        painter.drawText(pos, label);
    }
}

void BoardWidget::mouseMoveEvent(QMouseEvent* event)
{
    hoverPos_ = event->pos();
    hasHover_ = true;
    update();
    QWidget::mouseMoveEvent(event);
}

void BoardWidget::mousePressEvent(QMouseEvent* event)
{
    if (event->button() == Qt::LeftButton && enabled_) {
        QPoint gridPos = screenToGrid(event->pos());
        if (isValidPosition(gridPos.x(), gridPos.y()) && board_[gridPos.x()][gridPos.y()] == 0) {
            emit pieceClicked(gridPos.x(), gridPos.y());
        }
    }
    QWidget::mousePressEvent(event);
}

void BoardWidget::leaveEvent(QEvent* event)
{
    Q_UNUSED(event);
    hasHover_ = false;
    hoverPos_ = QPoint(-1, -1);
    update();
}

QPoint BoardWidget::screenToGrid(const QPoint& pos)
{
    int col = qRound((pos.x() - BOARD_OFFSET) / (double)GRID_SIZE);
    int row = qRound((pos.y() - BOARD_OFFSET) / (double)GRID_SIZE);
    return QPoint(row, col);
}

QPoint BoardWidget::gridToScreen(int row, int col)
{
    return QPoint(BOARD_OFFSET + col * GRID_SIZE, BOARD_OFFSET + row * GRID_SIZE);
}

bool BoardWidget::isValidPosition(int row, int col) const
{
    return row >= 0 && row < BOARD_SIZE && col >= 0 && col < BOARD_SIZE;
}