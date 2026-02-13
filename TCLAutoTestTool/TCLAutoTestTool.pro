QT       += core gui serialport network xml

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

CONFIG += c++17


DESTDIR = ../../bin
RC_ICONS = TCLLogo.ico
VERSION = 1.0.1.2
TARGET = TCLAutoTestTool
QMAKE_TARGET_COMPANY = "TCL泛智屏BU研发中心"
QMAKE_TARGET_PRODUCT = TCLAutoTestTool
QMAKE_TARGET_DESCRIPTION = "TCLDebugTool: Created by Qt6.10.1"
QMAKE_TARGET_COPYRIGHT = "TCL泛智屏BU研发中心 版权所有(2026.02)"
RC_LANG = 0x0004

# You can make your code fail to compile if it uses deprecated APIs.
# In order to do so, uncomment the following lTCLAutoTestTooline.
#DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0x060000    # disables all the APIs deprecated before Qt 6.0.0

# 仅Windows平台生效
win32 {
    LIBS += -luser32 -lkernel32 -lpsapi # 链接窗口/进程相关库
}

SOURCES += \
    DialogSerialportList.cpp \
    gencomport.cpp \
    main.cpp \
    MainWindow.cpp \
    tinyxml2.cpp

HEADERS += \
    DialogSerialportList.h \
    MainWindow.h \
    gencomport.h \
    tinyxml2.h

FORMS += \
    DialogSerialportList.ui \
    MainWindow.ui

TRANSLATIONS += \
    TCLAutoTestTool_zh_CN.ts
CONFIG += lrelease
CONFIG += embed_translations

# Default rules for deployment.
qnx: target.path = /tmp/$${TARGET}/bin
else: unix:!android: target.path = /opt/$${TARGET}/bin
!isEmpty(target.path): INSTALLS += target

RESOURCES += \
    res.qrc
