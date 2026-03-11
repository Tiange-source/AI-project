#ifndef TITLEBAR_H
#define TITLEBAR_H

#include <QWidget>
#include <QLabel>
#include <QPushButton>

/**
 * @brief 自定义标题栏类
 *
 * 用于无边框窗口的标题栏，支持拖动和窗口控制
 */
class TitleBar : public QWidget
{
    Q_OBJECT

public:
    explicit TitleBar(QWidget* parent = nullptr);
    ~TitleBar();

    /**
     * @brief 设置标题文本
     * @param title 标题
     */
    void setTitle(const QString& title);

    /**
     * @brief 设置图标
     * @param icon 图标路径
     */
    void setIcon(const QString& icon);

protected:
    /**
     * @brief 鼠标双击事件
     */
    void mouseDoubleClickEvent(QMouseEvent* event) override;

    /**
     * @brief 鼠标按下事件
     */
    void mousePressEvent(QMouseEvent* event) override;

    /**
     * @brief 鼠标移动事件
     */
    void mouseMoveEvent(QMouseEvent* event) override;

    /**
     * @brief 鼠标释放事件
     */
    void mouseReleaseEvent(QMouseEvent* event) override;

    /**
     * @brief 事件过滤器
     */
    bool eventFilter(QObject* watched, QEvent* event) override;

signals:
    /**
     * @brief 最小化信号
     */
    void minimizeClicked();

    /**
     * @brief 最大化/还原信号
     */
    void maximizeClicked(bool maximized);

    /**
     * @brief 关闭信号
     */
    void closeClicked();

private slots:
    /**
     * @brief 更新按钮样式
     */
    void updateButtons();

    /**
     * @brief 最小化按钮点击
     */
    void onMinimizeClicked();

    /**
     * @brief 最大化按钮点击
     */
    void onMaximizeClicked();

    /**
     * @brief 关闭按钮点击
     */
    void onCloseClicked();

private:
    /**
     * @brief 初始化UI
     */
    void initUI();

    /**
     * @brief 连接信号槽
     */
    void connectSignals();

private:
    QLabel* iconLabel_;
    QLabel* titleLabel_;
    QPushButton* minimizeButton_;
    QPushButton* maximizeButton_;
    QPushButton* closeButton_;

    bool isDragging_;
    QPoint dragPosition_;
    bool isMaximized_;
};

#endif // TITLEBAR_H