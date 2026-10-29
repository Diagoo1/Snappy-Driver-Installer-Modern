from pathlib import Path
p=Path(__file__).resolve().parents[1]
s=(p/'tools'/'build_win11_preview.ps1').read_text(encoding='utf-8-sig')
for x in ['QT_QMAKE','QT_MINGW_BIN','QT_VERSION','QMAKE_XSPEC','win32-g','mingw32-make','windeployqt','--demo','dist-win11-preview','build-win11.log']:
 assert x in s, x
assert (p/'01_BUILD_AND_RUN_DEMO.cmd').is_file()
assert (p/'02_RUN_REAL_SCAN.cmd').is_file()
print('QuickBuild source audit passed.')
