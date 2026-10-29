import unittest
from pathlib import Path
ROOT=Path(__file__).resolve().parents[1]
def read(f):return (ROOT/f).read_text(encoding='utf-8')
class Release17VisibleFixes(unittest.TestCase):
 def test_language_picker_has_own_popup_filter_and_41_origins(self):
  source=read('src/searchable_language_combo.cpp')
  for token in ('Qt::Popup','m_filter->setPlaceholderText','void SearchableLanguageCombo::filterItems(',
                'm_popup->show()','setCurrentIndex(index)'):
   self.assertIn(token,source)
  self.assertIn('m_language=new SearchableLanguageCombo(panel);',read('src/main_window.cpp'))
  self.assertIn('OriginLanguages::available(',read('src/main_window.cpp'))
  self.assertIn('filterKeys=QStringList()',read('src/driver_workspace.cpp'))
  self.assertIn('Sidebar::applyOriginLanguage(',read('src/sidebar.cpp'))
 def test_win32_popup_rounded_with_alpha_and_paint(self):
  pop=read('src/premium_popover.cpp')
  for token in ('setAttribute(Qt::WA_TranslucentBackground,true)', 'drawRoundedRect(', 'Qt::NoDropShadowWindowHint','m_dark?QColor'):
   self.assertIn(token,pop)
  self.assertNotIn('setMask(QRegion(',pop)
  self.assertIn('m_installPopup->setDark(dark)',read('src/driver_workspace.cpp'))
 def test_metrics_and_controls_have_separate_rows(self):
  ui=read('src/driver_workspace.cpp')
  self.assertIn('QVBoxLayout *summaryLayout=new QVBoxLayout(summary)',ui)
  self.assertIn('summaryActions->addWidget(m_installOptions)',ui)
  self.assertIn('summaryActions->addWidget(m_all)',ui)
  self.assertIn('summaryActions->addWidget(m_none)',ui)
  self.assertIn('summaryRow->addWidget(card)',ui)
 def test_build_and_portable_verification_reject_stale_exe(self):
  build=read('tools/integration/build_native_engine.ps1')
  launch=read('tools/verify_r17_and_launch.ps1')
  pack=read('tools/package_offline_portable.ps1')
  self.assertIn("releaseTag='SDIO-MODERN'",build)
  self.assertIn("$m.releaseTag -ne 'SDIO-MODERN'",launch)
  self.assertIn('Get-FileHash',launch)
  self.assertIn('FileMinorPart -ne 21',build)
  self.assertIn('FileMinorPart -ne 21',launch)
  self.assertIn('Offline EXE is different/stale',launch)
  self.assertIn('-NoLaunch',read('01_BUILD_ENGINE_MATCHING.cmd'))
  self.assertIn("$m.releaseTag -ne 'SDIO-MODERN'",pack)
  self.assertIn('Existing language/theme preferences preserved',pack)
  self.assertIn('05_VERIFY_R17_AND_LAUNCH.cmd',read('RELEASE17_IMPLEMENTED_VS_PENDING_AR.md'))
 def test_no_false_claim_real_install_or_rename_origin(self):
  self.assertIn('SDIO_MODERN_ENABLE_EXPERIMENTAL_INSTALL',read('src/engine_bridge.cpp'))
  self.assertIn('Matching-only builds cannot modify Windows drivers',read('src/installation_workspace.cpp'))
  self.assertIn('Snappy Driver Installer',read('src/title_bar.cpp'))
  self.assertNotIn('R17 UI UPDATE',read('src/title_bar.cpp'))
  self.assertIn('FILEVERSION 0,21,3,0',read('resources.rc'))
  self.assertIn('UI © 2026 Tarek Sadek',read('src/sidebar.cpp'))
 def test_build_files_include_new_picker_and_cpp_is_not_faked(self):
  for unit in ('src/searchable_language_combo.cpp','src/searchable_language_combo.h'):
   self.assertTrue((ROOT/unit).exists())
   self.assertIn(unit,read('SDIO_Modern_Qt.pro'))
  self.assertIn('src/searchable_language_combo.cpp',read('CMakeLists.txt'))
  self.assertIn('Script script;script.initEngine(paths.forceReindex?L"reindex":L"")',read('native/origin_native.cpp'))
 def test_ui_speed_deferred_expensive_diagnostics(self):
  self.assertIn('QTimer::singleShot(250,this,[this](){if(!m_engine->isBusy())updateDiagnostics();});',read('src/main_window.cpp'))
  self.assertIn('Origin scan: %1 ms; Qt list: %2 ms',read('src/main_window.cpp'))
  self.assertIn('bool hasRecommended=false;',read('native/origin_native.cpp'))
  self.assertIn('if(c.recommended)hasRecommended=true;',read('native/origin_native.cpp'))
  self.assertIn('QRegularExpressionMatchIterator match=regex.globalMatch(source);',read('src/origin_languages.cpp'))
if __name__=='__main__':unittest.main()
