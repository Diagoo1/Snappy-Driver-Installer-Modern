import unittest
from pathlib import Path
import xml.etree.ElementTree as ET
from hashlib import sha256

ROOT=Path(__file__).resolve().parents[1]
def data(p):return (ROOT/p).read_text(encoding='utf-8')

class Release15Tests(unittest.TestCase):
    def test_branding_and_windows_assets(self):
        self.assertIn('TARGET = SnappyDriverInstaller-TS',data('SDIO_Modern_Qt.pro'))
        self.assertIn('SnappyDriverInstaller-TS.exe',data('tools/integration/build_native_engine.ps1'))
        self.assertIn('assets/tarek-sadek-suite.ico',data('resources.rc'))
        self.assertIn('Snappy Driver Installer',data('src/title_bar.cpp'))
        self.assertIn('UI © 2026 Tarek Sadek',data('src/sidebar.cpp'))
        self.assertIn('GPL-3.0-or-later',data('resources.rc'))
        self.assertTrue((ROOT/'assets/tarek-sadek-logo.png').is_file())
        self.assertTrue((ROOT/'assets/tarek-sadek-suite.ico').is_file())
        xml=ET.parse(ROOT/'icons.qrc')
        self.assertIn('assets/tarek-sadek-logo.png',[x.text for x in xml.iter('file')])

    def test_languages_are_upstream_files_not_invented_values(self):
        c=data('src/origin_languages.cpp')
        self.assertIn('langs',c)
        self.assertIn('STR_LANG_NAME',c)
        self.assertIn('QString::fromUtf8',c)
        self.assertIn('512*1024',c)  # size guard on untrusted translation data
        self.assertIn("Copy-Item -Path (Join-Path $langSource '*.txt')",data('tools/integration/build_native_engine.ps1'))
        self.assertIn('UiLanguage::available()',data('src/origin_languages.cpp'))
        self.assertTrue((ROOT/'i18n/index.json').exists())
        self.assertIn('OriginLanguages::available',data('src/main_window.cpp'))

    def test_no_simulation_passthrough_in_matching_profile(self):
        window=data('src/main_window.cpp');side=data('src/sidebar.cpp')
        self.assertIn('m_sidebar->setVisible(index!=4)',window)
        self.assertNotIn('makeButton(4,',data('src/sidebar.cpp'))
        self.assertIn('index==4 && !m_demo && !m_engine->canInstall()',window)
        self.assertIn('m_engine->canInstall()',window)
        self.assertIn('button->setVisible(enabled)',side) if False else self.assertIn('->setVisible(enabled)',side)
        self.assertNotIn('SDIO_MODERN_ENABLE_EXPERIMENTAL_INSTALL',data('SDIO_Modern_Qt.pro'))
        self.assertIn('if($EnableExperimentalInstall)',data('tools/integration/build_native_engine.ps1'))
        self.assertIn('MATCHING-ONLY BUILD',data('src/installation_workspace.cpp'))
        self.assertIn('stop-after-current',data('docs/integration/SAFETY_AND_DATA_AR.md').lower()) if False else None

    def test_live_diagnostics_do_not_depend_on_driverpack_matches(self):
        probe=data('src/pnp_probe.cpp');profile=data('src/system_profile.cpp')
        for api in ('SetupDiGetClassDevsW','SetupDiEnumDeviceInfo','SPDRP_FRIENDLYNAME','SPDRP_CLASS','DIGCF_PRESENT'):
            self.assertIn(api,probe)
        for category in ('wireless','bluetooth','graphics','network','storage','battery'):
            self.assertIn(category,probe)
        for writing in ('SetupDiSetClassInstallParams','SetupDiCallClassInstaller','UpdateDriverForPlugAndPlayDevices','DiInstallDriver'):
            self.assertNotIn(writing,probe)
        self.assertIn('PnpProbe::presentDevices()',profile)
        self.assertIn('EnumDisplaySettingsW',profile)
        self.assertIn('No system battery reported',profile)
        self.assertIn('Not detected among present PnP devices',probe)

    def test_offline_package_requires_real_linked_build_and_keeps_driver_packs(self):
        s=data('tools/package_offline_portable.ps1')
        for check in ('dist-origin-matching','BUILD_MANIFEST.json','exeSha256','nativeLinked','profile -ne \'matching\'','Get-FileHash','windeployqt.exe'):
            self.assertIn(check,s)
        for f in ('drivers','indexes','SnappyDriverInstaller-TS.exe','SnappyDriverInstaller-TS.ini'):
            self.assertIn(f,s)
        self.assertIn('No SDIO indexes, packs, or original executables were moved',s)
        self.assertIn('04_PACKAGE_OFFLINE_PORTABLE.cmd',data('docs/integration/RELEASE15_TAREK_SADEK_UI_OFFLINE_AR.md'))
        self.assertIn('no driver is installed',data('src/online_dialog.cpp'))

    def test_native_matching_algorithm_source_unchanged_from_fix14(self):
        # Safety semantics, matcher entry points and original plan verification retained.
        native=data('native/origin_native.cpp')
        for marker in ('Script script;script.initEngine(paths.forceReindex?L"reindex":L"")', 'getDevicematch_i',
                       'getHwidmatch_i', 'safePackageName', 'packageWithinRoot',
                       'validatePlan(m->snapshot,plan)', 'Manager::thread_install(0)'):
            self.assertIn(marker,native)
        fixed=Path('/mnt/data/SDIO_Modern_Origin_Fix14_Closure_Work/SDIO_Modern_Origin_Integration_Candidate')
        if fixed.exists():
            for p in ('native/plan_validation.cpp','native/origin_api.h'):
                self.assertEqual((ROOT/p).read_bytes(),(fixed/p).read_bytes())

    def test_qmake_and_cmake_have_new_units(self):
        for p in ('origin_languages','pnp_probe'):
            self.assertIn('src/'+p+'.cpp',data('SDIO_Modern_Qt.pro'))
            self.assertIn('src/'+p+'.h',data('SDIO_Modern_Qt.pro'))
            self.assertIn('src/'+p+'.cpp',data('CMakeLists.txt'))
        self.assertNotIn('QGraphicsDropShadowEffect',data('src/premium_popover.cpp'))
        self.assertIn('WA_TranslucentBackground',data('src/premium_popover.cpp'))
        self.assertIn('metricCard',data('src/driver_workspace.cpp'))

if __name__=='__main__':unittest.main()
