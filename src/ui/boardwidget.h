#ifndef BOARDWIDGET_H
#define BOARDWIDGET_H

#include <QWidget>
#include <QPainter>
#include <QMouseEvent>
#include <QPoint>

/**
 * @brief 棋盘组件
 *
 * 18x18五子棋棋盘，支持落子和显示
 */
class BoardWidget : public QWidget
{
    Q_OBJECT

public:
    explicit BoardWidget(QWidget* parent = nullptr);
    ~BoardWidget();

    /**
     * @brief 重置棋盘
     */
    void reset();

    /**
     * @brief 落子
     * @param row 行号
     * @param col 列号
     * @param player 玩家（1=黑棋，2=白棋）
     */
    void setPiece(int row, int col, int player);

    /**
     * @brief 设置是否允许落子
     * @param enabled 是否允许
     */
    void setEnabled(bool enabled);

    /**
     * @brief 获取棋盘状态
     * @param row 行号
     * @param col 列号
     * @return 棋子状态（0=空，1=黑，2=白）
     */
    int getPiece(int row, int col) const;

signals:
    /**
     * @brief 棋子点击信号
     * @param row 行号
     * @param col 列号
     */
    void pieceClicked(int row, int col);

protected:
    /**
     * @brief 绘制事件
     */
    void paintEvent(QPaintEvent* event) override;

    /**
     * @brief 鼠标移动事件
     */
    void mouseMoveEvent(QMouseEvent* event) override;

    /**
     * @brief 鼠标按下事件
     */
    void mousePressEvent(QMouseEvent* event) override;

    /**
     * @brief 鼠标离开事件
     */
    void leaveEvent(QEvent* event) override;

private:
    /**
     * @brief 绘制棋盘网格
     */
    void drawGrid(QPainter& painter);

    /**
     * @brief 绘制星位
     */
    void drawStars(QPainter& painter);

    /**
     * @brief 绘制棋子
     */
    void drawPieces(QPainter& painter);

    /**
     * @brief 绘制悬停效果
     */
    void drawHover(QPainter& painter);

    /**
     * @brief 绘制坐标标签
     */
    void drawLabels(QPainter& painter);

    /**
     * @brief 屏幕坐标转网格坐标
     */
    QPoint screenToGrid(const QPoint& pos);

    /**
     * @brief 网格坐标转屏幕坐标
     */
    QPoint gridToScreen(int row, int col);

    /**
     * @brief 检查位置是否有效
     */
    bool isValidPosition(int row, int col) const;

private:
    static const int BOARD_SIZE = 18;      // 棋盘大小18x18
    static const int GRID_SIZE = 35;       // 网格间距
    static const int BOARD_OFFSET = 30;    // 边距
    static const int CHESS_RADIUS = 16;    // 棋子半径

    int board_[BOARD_SIZE][BOARD_SIZE];    // 棋盘状态
    QPoint hoverPos_;                      // 鼠标悬停位置
    bool hasHover_;
    bool enabled_;
    int lastMoveRow_;                      // 最后一步行号
    int lastMoveCol_;                      // 最后一步列号
};

#endif // BOARDWIDGET_H