include(smoke.pro)
TARGET=download-peer-smoke
isEmpty(SDIO_TORRENT_PRI):SDIO_TORRENT_PRI=$$PWD/../../build-torrent-static/torrent-link.pri
include($$SDIO_TORRENT_PRI)
