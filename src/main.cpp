#include "mainwindow.h"
#include <QApplication>
#include <QTextCodec>

int main(int argc, char* argv[])
{
    QApplication app(argc, argv);

    // 设置应用程序信息
    app.setApplicationName("GomokuClient");
    app.setApplicationVersion("1.0.0");
    app.setOrganizationName("GomokuProject");

    // 创建并显示主窗口
    MainWindow window;
    window.show();

    return app.exec();
}