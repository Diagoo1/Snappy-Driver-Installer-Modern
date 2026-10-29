TEMPLATE = app
TARGET = ui-interaction-test
QT += core gui widgets svg testlib
CONFIG += console release c++11
CONFIG -= app_bundle debug debug_and_release
DEFINES += QT_NO_CAST_FROM_ASCII
APP_ROOT = $$clean_path($$PWD/../..)
INCLUDEPATH += $$APP_ROOT/src $$APP_ROOT/native
SOURCES += $$files($$APP_ROOT/src/*.cpp)
SOURCES -= $$APP_ROOT/src/main.cpp $$APP_ROOT/src/scan_report.cpp
SOURCES += $$PWD/interaction_test.cpp $$PWD/publication_gallery.cpp $$APP_ROOT/native/plan_validation.cpp $$APP_ROOT/native/origin_unavailable.cpp
HEADERS += $$files($$APP_ROOT/src/*.h) $$APP_ROOT/native/origin_api.h
RESOURCES += $$APP_ROOT/icons.qrc
win32:LIBS += -lsetupapi -lcfgmgr32 -ladvapi32 -lole32 -lwininet
win32:QMAKE_LFLAGS += -static -static-libgcc -static-libstdc++
QTPLUGIN += qwindows qsvgicon

!isEmpty(SDIO_TORRENT_PRI):include($$SDIO_TORRENT_PRI)
