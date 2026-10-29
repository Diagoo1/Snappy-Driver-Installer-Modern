# Snappy Driver Installer - Qt 5.6-compatible UI modified by Tarek Sadek
# Target: Qt 5.6.3 + C++11 + Windows XP x86 through Windows 11 (pending VM validation)
QT += core gui widgets svg
CONFIG += c++11
CONFIG -= app_bundle
TEMPLATE = app
TARGET = SnappyDriverInstaller-TS
DEFINES += QT_NO_CAST_FROM_ASCII

SOURCES += \
    src/online_torrent.cpp \
    src/online_packages.cpp \
    src/online_dialog.cpp \
    src/support_report.cpp \
    src/appearance.cpp src/window_motion.cpp \
    src/single_instance.cpp \
    src/scan_report.cpp \
    src/automation_plan.cpp \
    src/portable_library.cpp \
    src/about_page.cpp \
    src/main.cpp \
    src/main_window.cpp \
    src/scan_overlay.cpp \
    src/searchable_language_combo.cpp \
    src/system_profile.cpp \
    src/pnp_probe.cpp \
    src/origin_languages.cpp \
    src/sidebar.cpp \
    src/device_scanner.cpp \
    src/engine_bridge.cpp \
    src/origin_worker.cpp \
    native/plan_validation.cpp \
    src/premium_popover.cpp \
    src/device_model.cpp \
    src/app_update.cpp \
    src/restart_prompt.cpp \
    src/legacy_script.cpp \
    src/legacy_runtime.cpp \
    src/drop_target.cpp \
    src/ui_scale.cpp \
    src/ui_font.cpp \
    src/ui_language.cpp \
    src/theme_pack.cpp \
    src/theme.cpp \
    src/ui_elements.cpp \
    src/vector_icons.cpp \
    src/driver_catalog.cpp \
    src/driver_workspace.cpp \
    src/driver_details_dialog.cpp \
    src/installation_workspace.cpp \
    src/tools_drawer.cpp \
    src/driver_row_delegate.cpp \
    src/title_bar.cpp
HEADERS += \
    src/online_packages.h \
    src/online_dialog.h \
    src/support_report.h \
    src/appearance.h src/window_motion.h \
    src/single_instance.h \
    src/scan_report.h \
    src/main_window.h \
    src/scan_overlay.h \
    src/searchable_language_combo.h \
    src/system_profile.h \
    src/pnp_probe.h \
    src/origin_languages.h \
    src/sidebar.h \
    src/device_scanner.h \
    src/engine_bridge.h \
    src/origin_worker.h \
    native/origin_api.h \
    src/premium_popover.h \
    src/device_model.h \
    src/theme.h \
    src/ui_elements.h \
    src/device_types.h \
    src/vector_icons.h \
    src/driver_catalog.h \
    src/driver_workspace.h \
    src/driver_details_dialog.h \
    src/installation_workspace.h \
    src/tools_drawer.h \
    src/driver_row_delegate.h \
    src/title_bar.h

RESOURCES += icons.qrc

win32:LIBS += -lsetupapi -lcfgmgr32 -ladvapi32 -lole32 -lwininet
win32:RC_FILE = resources.rc
win32:QMAKE_LFLAGS += -Wl,--major-subsystem-version,5,--minor-subsystem-version,1
win32:QMAKE_LFLAGS += -Wl,--major-os-version,5,--minor-os-version,1

# Qt 5.6.3's static build must use the XP deployment target when building for XP.
# Linking against dynamic Qt will NOT produce the promised single-file portable executable.

# No implicit fallback: a UI-only build cannot claim to contain Origin.
contains(CONFIG, origin_engine) {
    !win32:error(The native Origin engine target requires Windows.)
    isEmpty(SDIO_ORIGIN_PRI):error(Use tools/integration/build_native_engine.ps1 to build the actual engine first.)
    !exists($$SDIO_ORIGIN_PRI):error(The generated native linker configuration does not exist.)
    include($$quote($$SDIO_ORIGIN_PRI))
    include($$PWD/build-torrent-static/torrent-link.pri)
} else {
    SOURCES += native/origin_unavailable.cpp
}
contains(CONFIG, single_file) {
    !contains(QT_CONFIG, static):error(A static Qt SDK is required for a single-file build.)
    QMAKE_LFLAGS += -static -static-libgcc -static-libstdc++ -Wl,--gc-sections
    QMAKE_CFLAGS_RELEASE += -ffunction-sections -fdata-sections
    QMAKE_CXXFLAGS_RELEASE += -ffunction-sections -fdata-sections
    QTPLUGIN += qwindows qsvgicon
    DEFINES += SDIO_STATIC_QT
}
