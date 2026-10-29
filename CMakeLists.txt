cmake_minimum_required(VERSION 3.5)
project(SnappyDriverInstallerUi LANGUAGES CXX)
# Optional modern-host build for code review / QA. Windows XP target uses qmake + Qt 5.6.3.
set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
find_package(Qt5 5.6 REQUIRED COMPONENTS Widgets Svg)
add_executable(SnappyDriverInstaller-TS
  src/automation_plan.cpp src/automation_plan.h
  src/portable_library.cpp src/portable_library.h
  src/about_page.cpp src/about_page.h
  src/main.cpp
  src/app_settings.h
  src/online_torrent.cpp
  src/online_packages.cpp src/online_packages.h src/online_dialog.cpp src/online_dialog.h
  src/support_report.cpp src/support_report.h
  src/appearance.cpp
  src/appearance.h
  src/single_instance.cpp
  src/single_instance.h
  src/scan_report.cpp
  src/main_window.cpp
  src/scan_overlay.cpp src/searchable_language_combo.cpp
  src/system_profile.cpp
  src/pnp_probe.cpp
  src/origin_languages.cpp
  src/sidebar.cpp
  src/device_scanner.cpp
  src/engine_bridge.cpp
  src/origin_worker.cpp
  native/plan_validation.cpp
  native/origin_unavailable.cpp
  src/premium_popover.cpp
  src/device_model.cpp
  src/app_update.cpp
    src/restart_prompt.cpp
    src/legacy_script.cpp src/legacy_runtime.cpp src/legacy_script.h
    src/drop_target.cpp src/drop_target.h src/ui_scale.cpp src/ui_scale.h
    src/ui_font.cpp src/ui_font.h
    src/ui_language.cpp
    src/theme_pack.cpp
  src/theme.cpp
  src/ui_elements.cpp
  src/vector_icons.cpp
  src/driver_catalog.cpp
  src/driver_workspace.cpp
  src/driver_details_dialog.cpp
  src/installation_workspace.cpp
  src/tools_drawer.cpp
  src/driver_row_delegate.cpp
  src/title_bar.cpp
  icons.qrc
  src/main_window.h
  src/scan_overlay.h
  src/system_profile.h
  src/pnp_probe.h
  src/origin_languages.h
  src/sidebar.h
  src/device_scanner.h
  src/engine_bridge.h
  src/origin_worker.h
  native/origin_api.h
  src/premium_popover.h
  src/device_model.h
  src/ui_elements.h
  src/theme.h
  src/device_types.h
  src/vector_icons.h
  src/driver_catalog.h
  src/driver_workspace.h
  src/driver_details_dialog.h
  src/installation_workspace.h
  src/tools_drawer.h
  src/driver_row_delegate.h
  src/title_bar.h
)
target_compile_definitions(SnappyDriverInstaller-TS PRIVATE QT_NO_CAST_FROM_ASCII)
target_link_libraries(SnappyDriverInstaller-TS Qt5::Widgets Qt5::Svg)
if(WIN32)
  target_link_libraries(SnappyDriverInstaller-TS setupapi cfgmgr32 advapi32 ole32 wininet)
endif()

# This CMake target is UI-only. Native Windows integration uses the reviewed
# qmake dependency graph in tools/integration/build_native_engine.ps1.
