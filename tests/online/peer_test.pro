TEMPLATE=app
TARGET=peer-tests
QT=core
CONFIG+=console release c++11
CONFIG-=debug debug_and_release app_bundle
DEFINES+=QT_NO_CAST_FROM_ASCII
SOURCES+=peer_test.cpp ../../src/online_packages.cpp ../../src/online_torrent.cpp
isEmpty(SDIO_TORRENT_PRI):SDIO_TORRENT_PRI=$$PWD/../../build-torrent-static/torrent-link.pri
include($$SDIO_TORRENT_PRI)
win32:LIBS+=-lwininet
win32:QMAKE_LFLAGS+=-static -static-libgcc -static-libstdc++
