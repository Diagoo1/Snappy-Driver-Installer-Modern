import re
import unittest
from pathlib import Path
ROOT=Path(__file__).resolve().parents[1]
def read(p):return (ROOT/p).read_text(encoding='utf-8')
class Release16Wiring(unittest.TestCase):
    def test_name_copyright_scope_and_unique_portable_executable(self):
        self.assertIn('Snappy Driver Installer',read('src/title_bar.cpp'))
        self.assertIn('UI by Tarek Sadek',read('src/title_bar.cpp'))
        self.assertIn('UI © 2026 Tarek Sadek',read('src/sidebar.cpp'))
        self.assertIn('Original SDIO contributors',read('resources.rc'))
        self.assertIn('GPL-3.0-or-later',read('resources.rc'))
        self.assertIn('TARGET = SnappyDriverInstaller-TS',read('SDIO_Modern_Qt.pro'))
        self.assertIn('SnappyDriverInstaller-TS.exe',read('tools/package_offline_portable.ps1'))
        self.assertNotIn("$dest=Join-Path $OfflineRoot 'SDI_R2600.exe'",read('tools/package_offline_portable.ps1'))
    def test_overlay_is_actual_progress_not_fake_progress(self):
        overlay=read('src/scan_overlay.cpp')
        self.assertIn('parentWidget()->grab()',overlay)
        self.assertIn('small.scaled(source.size()',overlay)
        self.assertIn('QPainter p(this)',overlay)
        self.assertIn('m_timer->setInterval(33)',overlay)
        self.assertIn('for(int i=0;i<6;++i)',overlay)
        ui=read('src/main_window.cpp')
        for token in ('m_scanOverlay->start(', 'm_scanOverlay->setStage(', 'm_scanOverlay->finish()'):
            self.assertIn(token,ui)
        main=read('src/main.cpp')
        self.assertIn('const bool noSplash = args.contains(QStringLiteral("--no-splash"));',main)
        self.assertNotIn('|| !demo',main)
        self.assertIn('Preparing your workspace...',read('src/ui_elements.cpp'))
    def test_lazy_candidates_expands_only_when_needed(self):
        ui=read('src/driver_workspace.cpp')
        part=ui.split('void DriverWorkspace::rebuild()')[1].split('void DriverWorkspace::populateCandidates(')[0]
        self.assertNotIn('new QTreeWidgetItem(root)',part)
        self.assertIn('setChildIndicatorPolicy(QTreeWidgetItem::ShowIndicator)',part)
        self.assertIn('populateCandidates(root)',part)
        self.assertIn('connect(m_tree,&QTreeWidget::itemExpanded',ui)
        self.assertIn('m_searchDebounce->setInterval(135)',ui)
        self.assertIn('m_catalog->entryRef(row)',ui)
        self.assertIn('QMap<QString,QIcon> rowIcons',ui)
        self.assertIn('m_entries.reserve(static_cast<int>(snapshot.devices.size()))',read('src/driver_catalog.cpp'))
    def test_original_matcher_with_read_only_cache(self):
        native=read('native/origin_native.cpp')
        self.assertIn('Script script;script.initEngine(paths.forceReindex?L"reindex":L"")',native)
        self.assertIn('packageChecks[c.packagePath]',native)
        self.assertIn('if(!check.stamped)',native)
        self.assertIn('if(eligible && !check.pathChecked)',native)
        self.assertIn('GetTickCount()-originStart',native)
        self.assertIn('GetTickCount()-copyStart',native)
        self.assertIn('safePackageName(c.packageName)',native)
        self.assertIn('validatePlan(m->snapshot,plan)',native)
    def test_no_live_driver_install_in_matching_profile(self):
        self.assertIn('SDIO_MODERN_ENABLE_EXPERIMENTAL_INSTALL',read('src/engine_bridge.cpp'))
        self.assertIn("profile -ne 'matching'",read('tools/package_offline_portable.ps1'))
        self.assertIn("$EnableExperimentalInstall",read('tools/integration/build_native_engine.ps1'))
    def test_qmake_cmake_sources_and_headers_exist(self):
        for unit in ('src/scan_overlay.cpp','src/scan_overlay.h'):
            self.assertTrue((ROOT/unit).is_file())
            self.assertIn(unit,read('SDIO_Modern_Qt.pro'))
            self.assertIn(unit,read('CMakeLists.txt'))
if __name__=='__main__': unittest.main()
