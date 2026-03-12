QT       += core gui network

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

CONFIG += c++11 utf8_source

# 定义版本号
VERSION = 1.0.0
DEFINES += APP_VERSION=\"$$VERSION\"

# 源代码目录
SRC_DIR = $$PWD/src
BUILD_DIR = $$PWD/build
PROTO_DIR = $$PWD/protocol
SHARED_PROTO_DIR = $$PWD/../shared/proto

# Protobuf路径
PROTOBUF_ROOT = $$PWD/../protobuf2112
PROTOBUF_INCLUDE = $$PROTOBUF_ROOT/include
PROTOBUF_LIB = $$PROTOBUF_ROOT/lib
PROTOBUF_LIB_FILE = $$PROTOBUF_LIB/libprotobuf.a
PROTOBUF_COMPILER = $$PROTOBUF_ROOT/protoc.exe

# Protobuf编译输出
PROTOBUF_SOURCES = $$BUILD_DIR/gomoku.pb.cc
PROTOBUF_HEADERS = $$BUILD_DIR/gomoku.pb.h

# 检查Protobuf文件是否存在
!exists($$PROTOBUF_SOURCES) {
    warning("Protobuf编译文件不存在")
    warning("请运行: $$PROTOBUF_COMPILER --proto_path=shared/proto --cpp_out=build shared/proto/gomoku.proto")
}

# 源文件
SOURCES += \
    $$SRC_DIR/main.cpp \
    $$SRC_DIR/mainwindow.cpp \
    $$SRC_DIR/ui/titlebar.cpp \
    $$SRC_DIR/ui/logindialog.cpp \
    $$SRC_DIR/ui/offlinegamedialog.cpp \
    $$SRC_DIR/ui/boardwidget.cpp \
    $$SRC_DIR/network/windowssocket.cpp \
    $$SRC_DIR/network/protobufcodec.cpp \
    $$SRC_DIR/network/messagedispatcher.cpp \
    $$SRC_DIR/logic/gamecontroller.cpp \
    $$SRC_DIR/logic/aiengine.cpp \
    $$SRC_DIR/data/userprofile.cpp \
    $$SRC_DIR/data/roominfo.cpp

# 头文件
HEADERS += \
    $$SRC_DIR/mainwindow.h \
    $$SRC_DIR/ui/titlebar.h \
    $$SRC_DIR/ui/logindialog.h \
    $$SRC_DIR/ui/offlinegamedialog.h \
    $$SRC_DIR/ui/boardwidget.h \
    $$SRC_DIR/network/windowssocket.h \
    $$SRC_DIR/network/protobufcodec.h \
    $$SRC_DIR/network/messagedispatcher.h \
    $$SRC_DIR/logic/gamecontroller.h \
    $$SRC_DIR/logic/aiengine.h \
    $$SRC_DIR/data/userprofile.h \
    $$SRC_DIR/data/roominfo.h

# UI文件
FORMS += \
    $$SRC_DIR/mainwindow.ui \
    $$SRC_DIR/ui/logindialog.ui \
    $$SRC_DIR/ui/offlinegamedialog.ui

# 资源文件
RESOURCES += \
    $$SRC_DIR/resources.qrc

# Protobuf文件（包含但不编译）
OTHER_FILES += \
    $$SHARED_PROTO_DIR/gomoku.proto \
    $$SRC_DIR/styles.qss

# 编译Protobuf文件（如果存在）
exists($$PROTOBUF_SOURCES) {
    SOURCES += $$PROTOBUF_SOURCES
    HEADERS += $$PROTOBUF_HEADERS

    # Protobuf头文件路径
    INCLUDEPATH += $$BUILD_DIR
    INCLUDEPATH += $$PROTOBUF_INCLUDE

    # 链接Protobuf库
    LIBS += -L$$PROTOBUF_LIB -lprotobuf
}

# 默认规则用于部署
qnx: target.path = /tmp/$${TARGET}/bin
else: unix:!android: target.path = /opt/$${TARGET}/bin
!isEmpty(target.path): INSTALLS += target

# Windows特定配置
win32 {
    # 设置输出目录
    CONFIG(debug, debug|release) {
        DESTDIR = $$BUILD_DIR/debug
        OBJECTS_DIR = $$BUILD_DIR/debug/obj
        MOC_DIR = $$BUILD_DIR/debug/moc
        RCC_DIR = $$BUILD_DIR/debug/rcc
        UI_DIR = $$BUILD_DIR/debug/ui
    } else {
        DESTDIR = $$BUILD_DIR/release
        OBJECTS_DIR = $$BUILD_DIR/release/obj
        MOC_DIR = $$BUILD_DIR/release/moc
        RCC_DIR = $$BUILD_DIR/release/rcc
        UI_DIR = $$BUILD_DIR/release/ui
    }

    # 设置编码（MinGW使用-exec-charset和-input-charset）
    QMAKE_CXXFLAGS += -finput-charset=UTF-8 -fexec-charset=UTF-8
    QMAKE_CFLAGS += -finput-charset=UTF-8 -fexec-charset=UTF-8
    
    # 链接Windows Socket库
    LIBS += -lws2_32
} else {
    # Unix/Linux/Mac配置
    DESTDIR = $$BUILD_DIR
    OBJECTS_DIR = $$BUILD_DIR/obj
    MOC_DIR = $$BUILD_DIR/moc
    RCC_DIR = $$BUILD_DIR/rcc
    UI_DIR = $$BUILD_DIR/ui
}

# 包含路径
INCLUDEPATH += $$SRC_DIR \
               $$SRC_DIR/network \
               $$SRC_DIR/ui \
               $$SRC_DIR/logic \
               $$SRC_DIR/data