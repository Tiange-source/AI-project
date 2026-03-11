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
SHARED_BUILD_DIR = $$PWD/../shared/build

# Protobuf配置
PROTOBUF_SOURCES = $$SHARED_BUILD_DIR/gomoku.pb.cc
PROTOBUF_HEADERS = $$SHARED_BUILD_DIR/gomoku.pb.h

# 检查Protobuf文件是否存在
!exists($$PROTOBUF_SOURCES) {
    warning("Protobuf编译文件不存在，请先编译shared/proto/gomoku.proto")
    warning("运行: cd shared/proto && ./compile.sh")
}

# 源文件
SOURCES += \
    $$SRC_DIR/main.cpp \
    $$SRC_DIR/mainwindow.cpp \
    $$SRC_DIR/network/tcpclient.cpp \
    $$SRC_DIR/network/protobufcodec.cpp \
    $$SRC_DIR/network/messagedispatcher.cpp \
    $$SRC_DIR/data/userprofile.cpp \
    $$SRC_DIR/data/roominfo.cpp

# 头文件
HEADERS += \
    $$SRC_DIR/mainwindow.h \
    $$SRC_DIR/network/tcpclient.h \
    $$SRC_DIR/network/protobufcodec.h \
    $$SRC_DIR/network/messagedispatcher.h \
    $$SRC_DIR/data/userprofile.h \
    $$SRC_DIR/data/roominfo.h

# UI文件
FORMS += \
    $$SRC_DIR/mainwindow.ui

# Protobuf文件（包含但不编译）
OTHER_FILES += \
    $$SHARED_PROTO_DIR/gomoku.proto

# 编译Protobuf文件（如果存在）
exists($$PROTOBUF_SOURCES) {
    SOURCES += $$PROTOBUF_SOURCES
    HEADERS += $$PROTOBUF_HEADERS

    # Protobuf头文件路径
    INCLUDEPATH += $$SHARED_BUILD_DIR
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

    # 设置编码
    QMAKE_CXXFLAGS += /utf-8
    QMAKE_CFLAGS += /utf-8
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