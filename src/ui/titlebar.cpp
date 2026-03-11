#include "titlebar.h"
#include <QHBoxLayout>
#include <QEvent>
#include <QMouseEvent>
#include <QDebug>

TitleBar::TitleBar(QWidget* parent)
    : QWidget(parent)
    , iconLabel_(nullptr)
    , titleLabel_(nullptr)
    , minimizeButton_(nullptr)
    , maximizeButton_(nullptr)
    , closeButton_(nullptr)
    , isDragging_(false)
    , isMaximized_(false)
{
    initUI();
    connectSignals();

    // 设置鼠标跟踪
    setMouseTracking(true);

    // 安装事件过滤器
    installEventFilter(this);
}

TitleBar::~TitleBar()
{
}

void TitleBar::setTitle(const QString& title)
{
    if (titleLabel_) {
        titleLabel_->setText(title);
    }
}

void TitleBar::setIcon(const QString& icon)
{
    if (iconLabel_) {
        iconLabel_->setPixmap(QPixmap(icon).scaled(32, 32, Qt::KeepAspectRatio, Qt::SmoothTransformation));
    }
}

void TitleBar::initUI()
{
    // 创建控件
    iconLabel_ = new QLabel(this);
    titleLabel_ = new QLabel(this);
    minimizeButton_ = new QPushButton(this);
    maximizeButton_ = new QPushButton(this);
    closeButton_ = new QPushButton(this);

    // 设置图标
    iconLabel_->setFixedSize(32, 32);
    iconLabel_->setStyleSheet("background: transparent;");

    // 设置标题
    titleLabel_->setText("网络联机五子棋");
    titleLabel_->setStyleSheet("QLabel { color: #ffffff; font-size: 14px; font-weight: 600; background: transparent; }");

    // 设置按钮
    minimizeButton_->setFixedSize(40, 32);
    minimizeButton_->setText("─");
    minimizeButton_->setStyleSheet(R"(
        QPushButton {
            background: transparent;
            color: #ffffff;
            border: none;
            font-size: 16px;
            font-weight: bold;
        }
        QPushButton:hover {
            background: rgba(255, 255, 255, 0.1);
        }
        QPushButton:pressed {
            background: rgba(255, 255, 255, 0.2);
        }
    )");

    maximizeButton_->setFixedSize(40, 32);
    maximizeButton_->setText("□");
    maximizeButton_->setStyleSheet(R"(
        QPushButton {
            background: transparent;
            color: #ffffff;
            border: none;
            font-size: 16px;
            font-weight: bold;
        }
        QPushButton:hover {
            background: rgba(255, 255, 255, 0.1);
        }
        QPushButton:pressed {
            background: rgba(255, 255, 255, 0.2);
        }
    )");

    closeButton_->setFixedSize(40, 32);
    closeButton_->setText("✕");
    closeButton_->setStyleSheet(R"(
        QPushButton {
            background: transparent;
            color: #ffffff;
            border: none;
            font-size: 16px;
            font-weight: bold;
            border-top-right-radius: 8px;
        }
        QPushButton:hover {
            background: #e74c3c;
        }
        QPushButton:pressed {
            background: #c0392b;
        }
    )");

    // 布局
    QHBoxLayout* layout = new QHBoxLayout(this);
    layout->setContentsMargins(10, 0, 0, 0);
    layout->setSpacing(10);
    layout->addWidget(iconLabel_);
    layout->addWidget(titleLabel_);
    layout->addStretch();
    layout->addWidget(minimizeButton_);
    layout->addWidget(maximizeButton_);
    layout->addWidget(closeButton_);
    layout->setStretch(1, 1);

    // 设置固定高度
    setFixedHeight(40);

    // 设置样式
    setStyleSheet(R"(
        TitleBar {
            background: qlineargradient(x1:0, y1:0, x2:1, y2:0,
                                        stop:0 #667eea, stop:1 #764ba2);
        }
    )");
}

void TitleBar::connectSignals()
{
    connect(minimizeButton_, &QPushButton::clicked, this, &TitleBar::onMinimizeClicked);
    connect(maximizeButton_, &QPushButton::clicked, this, &TitleBar::onMaximizeClicked);
    connect(closeButton_, &QPushButton::clicked, this, &TitleBar::onCloseClicked);
}

void TitleBar::mouseDoubleClickEvent(QMouseEvent* event)
{
    if (event->button() == Qt::LeftButton) {
        onMaximizeClicked();
    }
    QWidget::mouseDoubleClickEvent(event);
}

void TitleBar::mousePressEvent(QMouseEvent* event)
{
    if (event->button() == Qt::LeftButton) {
        isDragging_ = true;
        dragPosition_ = event->globalPos() - frameGeometry().topLeft();
        event->accept();
    }
    QWidget::mousePressEvent(event);
}

void TitleBar::mouseMoveEvent(QMouseEvent* event)
{
    if (isDragging_ && event->buttons() & Qt::LeftButton) {
        if (parentWidget()) {
            parentWidget()->move(event->globalPos() - dragPosition_);
            event->accept();
        }
    }
    QWidget::mouseMoveEvent(event);
}

void TitleBar::mouseReleaseEvent(QMouseEvent* event)
{
    isDragging_ = false;
    QWidget::mouseReleaseEvent(event);
}

bool TitleBar::eventFilter(QObject* watched, QEvent* event)
{
    if (event->type() == QEvent::MouseButtonDblClick) {
        QMouseEvent* mouseEvent = static_cast<QMouseEvent*>(event);
        if (mouseEvent->button() == Qt::LeftButton) {
            onMaximizeClicked();
            return true;
        }
    }
    return QWidget::eventFilter(watched, event);
}

void TitleBar::updateButtons()
{
    if (isMaximized_) {
        maximizeButton_->setText("❐");
    } else {
        maximizeButton_->setText("□");
    }
}

void TitleBar::onMinimizeClicked()
{
    emit minimizeClicked();
}

void TitleBar::onMaximizeClicked()
{
    isMaximized_ = !isMaximized_;
    updateButtons();
    emit maximizeClicked(isMaximized_);
}

void TitleBar::onCloseClicked()
{
    emit closeClicked();
}