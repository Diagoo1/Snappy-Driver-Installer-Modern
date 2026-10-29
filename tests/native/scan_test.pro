TEMPLATE = app
TARGET = native-scan-test
QT = core
CONFIG += console release c++11
CONFIG -= debug debug_and_release app_bundle
DEFINES += QT_NO_CAST_FROM_ASCII
SOURCES += scan_main.cpp ../../src/scan_report.cpp ../../src/origin_worker.cpp ../../native/plan_validation.cpp
HEADERS += ../../src/origin_worker.h ../../src/scan_report.h ../../src/offline_paths.h
include(../../build-origin-install-test-static/origin-link.pri)
win32:QMAKE_LFLAGS += -static -static-libgcc -static-libstdc++
