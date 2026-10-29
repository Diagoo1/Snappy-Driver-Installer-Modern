"""Source-structure checks only; NOT Windows compilation or integration tests."""
import json
import re
import unittest
import xml.etree.ElementTree as ET
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
def text(name):
    return (ROOT / name).read_text(encoding='utf-8-sig')

class SourceWiring(unittest.TestCase):
    def test_native_target_has_no_ui_fallback(self):
        pro = text('SDIO_Modern_Qt.pro')
        self.assertIn('contains(CONFIG, origin_engine)', pro)
        self.assertIn('include($$quote($$SDIO_ORIGIN_PRI))', pro)
        self.assertIn('SOURCES += native/origin_unavailable.cpp', pro)
        self.assertIn('src/origin_worker.cpp', pro)
        self.assertIn('native/plan_validation.cpp', pro)

    def test_no_external_origin_exporter_remains(self):
        combined = '\n'.join(p.read_text() for p in (ROOT/'src').glob('*') if p.is_file())
        self.assertNotIn('SdioReadOnlyProbe', combined)
        self.assertIn('engine().writeDeviceList',text('src/legacy_runtime.cpp'))
        self.assertIn('matcher->write_device_list',text('native/origin_native.cpp'))
        self.assertNotIn('m_originExe', combined)

    def test_original_functions_used(self):
        native=text('native/origin_native.cpp')
        for call in ('script.initEngine(paths.forceReindex?L"reindex":L"")', 'matcher->getDevicematch_i', 'matcher->getHwidmatch_i',
                     'Manager::thread_install(0)', 'SRSetRestorePointW', 'END_SYSTEM_CHANGE', 'getState()->save', 'device->getClassDesc(category)'):
            self.assertIn(call, native)

    def test_native_source_is_separate_from_qt(self):
        native=text('native/origin_native.cpp')
        self.assertNotRegex(native, r'#include\s+[<"]Q[A-Z]')
        self.assertIn('SDIO_ORIGIN_LINKED',text('src/engine_bridge.cpp'))

    def test_installer_requires_explicit_build_and_runtime_consent(self):
        self.assertIn('#ifdef SDIO_MODERN_ENABLE_EXPERIMENTAL_INSTALL', text('native/origin_native.cpp'))
        self.assertIn('plan.confirmed=true',text('src/main_window.cpp'))
        self.assertIn('cancel->setDefault(true)',text('src/appearance.cpp'))
        window=text('src/main_window.cpp').split('void MainWindow::beginInstallation()',1)[1].split('void MainWindow::openSnapshot()',1)[0]
        self.assertIn('if(!confirmDriverInstallation(',window)
        self.assertLess(window.index('if(!confirmDriverInstallation('),window.index('plan.confirmed=true'))
        self.assertLess(window.index('if(!elevated){'),window.index('m_installation->beginNative('))
        self.assertIn('QDialog::Accepted',text('src/installation_workspace.cpp'))
        self.assertIn('if(!plan.confirmed)',text('native/plan_validation.cpp'))

    def test_demo_separated_from_live(self):
        bridge=text('src/engine_bridge.cpp')
        self.assertIn('if(demo){m_scanner->setDemo(true)',bridge)
        self.assertIn('m_worker->job=OriginWorker::Scan',bridge)
        self.assertIn('if(!m_active || m_live) return;',text('src/installation_workspace.cpp'))
        self.assertIn('m_timer->stop();m_live=true',text('src/installation_workspace.cpp'))

    def test_priority_stars_preserve_original_matcher_rank(self):
        self.assertIn('matchPriorityStars(entry,i)',text('src/driver_catalog.cpp'))
        self.assertIn('candidate.originalOrder<target.originalOrder',text('src/driver_catalog.cpp'))
        self.assertIn('c.rawScore=hw->getScore()',text('native/origin_native.cpp'))
        self.assertIn('matching rank, not a security rating',text('native/origin_native.cpp'))

    def test_no_unsafe_shutdown_of_worker(self):
        src='\n'.join(text(f) for f in ('src/engine_bridge.cpp','src/origin_worker.cpp','src/main_window.cpp'))
        self.assertNotIn('->terminate(',src)
        self.assertNotIn('->kill(',src)
        self.assertIn('requestStopAfterCurrent',src)
        self.assertIn('m_closeWhenIdle',src)

    def test_pending_gate_avoids_finished_signal_race(self):
        bridge=text('src/engine_bridge.cpp')
        self.assertIn('return m_pending ||',bridge)
        self.assertIn('m_pending=true;m_worker->start',bridge)
        self.assertIn('m_pending=false;',bridge)
        self.assertIn('bool m_pending;',text('src/engine_bridge.h'))

    def test_local_package_and_hwid_preflight(self):
        native=text('native/origin_native.cpp')
        for guard in ('uniquelyPresent(', 'packageWithinRoot(', 'safePackageName(',
                      'FILE_ATTRIBUTE_REPARSE_POINT', 'archive.stamp()', 'administrator()'):
            self.assertIn(guard,native)
        self.assertIn('matchingDevices!=1',text('native/plan_validation.cpp'))

    def test_engine_snapshot_is_single_use(self):
        self.assertIn('++m->generation;m->snapshot=Snapshot()',text('native/origin_native.cpp'))
        self.assertIn('m_catalog->invalidateNative()',text('src/main_window.cpp'))

    def test_queue_filters_scope_bulk_selection(self):
        self.assertIn('selectRows(',text('src/driver_workspace.cpp'))
        self.assertIn('if(recommended<0)continue;',text('src/driver_catalog.cpp'))

    def test_http_updates_are_separate_from_native_torrent_backend(self):
        self.assertIn('c.downloads=false',text('native/origin_native.cpp'))
        self.assertIn('openOnlinePackages',text('src/main_window.cpp'))
        self.assertIn('QCryptographicHash::Sha1',text('src/online_packages.cpp'))

    def test_pinned_source_and_digest_lengths(self):
        lock=json.loads(text('tools/integration/source-lock.json'))
        self.assertRegex(lock['origin']['commit'],r'^[a-f0-9]{40}$')
        self.assertRegex(lock['boost']['sha256'],r'^[a-f0-9]{64}$')
        for digest in lock['origin']['gitBlobs'].values():
            self.assertRegex(digest,r'^[a-f0-9]{40}$')
        self.assertNotIn('HEAD',lock['origin']['url'])
        self.assertIn('community mirror',lock['origin']['version'])

    def test_patches_do_not_modify_matching_algorithm(self):
        patches=json.loads(text('tools/integration/patches.json'))['changes']
        names={p['file'] for p in patches}
        self.assertNotIn('source/matcher.cpp',names)
        self.assertIn('source/indexing.cpp',names)
        self.assertTrue(all(p['old'] and p['old']!=p['new'] and p['count']>=1 for p in patches))

    def test_upstream_double_retry_guards_are_both_patched(self):
        # The pinned Origin commit contains this guard in the extraction loop
        # AND the nested pack-availability loop. Both must be bounded.
        changes=json.loads(text('tools/integration/patches.json'))['changes']
        old='                        if(!itembar->checked||installmode!=MODE_INSTALLING||tries>60)break;'
        hits=[c for c in changes if c['file']=='source/install.cpp' and c['old']==old]
        self.assertEqual(len(hits),1)
        self.assertEqual(hits[0]['count'],2)
        self.assertEqual(hits[0]['new'],old.replace('tries>60','tries>=2'))
        self.assertIn('pattern: $snippet',text('tools/integration/prepare_sources.ps1'))

    def test_no_security_prompt_autoclick_in_patched_installer(self):
        changes=json.loads(text('tools/integration/patches.json'))['changes']
        old='\n'.join(p['old'] for p in changes)
        new='\n'.join(p['new'] for p in changes)
        self.assertIn('thread_clicker',old)
        self.assertNotIn('thr->start(',new)
        self.assertIn('preserve Windows SetupAPI logs',new)

    def test_7zip_2601_missing_header_has_pinned_official_bootstrap(self):
        lock=json.loads(text('tools/integration/source-lock.json'))
        fix=lock['sevenZipBootstrap']
        self.assertEqual(fix['version'],'26.01')
        self.assertEqual(fix['upstreamRepository'],'ip7z/7zip')
        self.assertEqual(fix['upstreamCommit'],'8c63d71ff886bda90c86db28466287f977374237')
        self.assertEqual(fix['gitBlob'],'5781a957b105c6a8e397561fbcdf2e11d76183ee')
        self.assertEqual(fix['relativePath'],'external/SevenZ/build/CPP/Common/Common0.h')
        self.assertIn(fix['upstreamCommit'],fix['url'])
        self.assertTrue(fix['url'].startswith('https://raw.githubusercontent.com/ip7z/7zip/'))

    def test_7zip_header_is_only_added_to_generated_vendor_tree(self):
        p=text('tools/integration/prepare_sources.ps1')
        b=text('tools/integration/build_native_engine.ps1')
        self.assertIn('Copy-Item -LiteralPath $origin -Destination $patched -Recurse',p)
        self.assertLess(p.index('Copy-Item -LiteralPath $origin -Destination $patched -Recurse'), p.index('Copy-Item -LiteralPath $sevenCache -Destination $sevenTarget'))
        self.assertIn('Get-GitBlobHash $sevenCache',p)
        self.assertIn('Get-GitBlobHash $sevenTarget',p)
        self.assertIn('Get-GitBlobHash $sevenCommon0',b)
        self.assertIn('MY_VERSION_NUMBERS',p)
        self.assertIn('Source prepared.',p)
        self.assertNotIn('Copy-Item -LiteralPath $sevenCache -Destination $origin',p)

    def test_boost_179_uses_thread_safe_cxx11_atomic_ref_fallback(self):
        # Exact failure observed with MinGW 4.9.2: std::atomic_ref is C++20.
        build=text('tools/integration/build_native_engine.ps1')
        self.assertIn("$boostAtomicRef=Join-Path $boost 'libs\\filesystem\\src\\atomic_ref.hpp'",build)
        self.assertIn('namespace atomic_ns = boost;',build)
        self.assertIn('Boost.Filesystem atomic_ref.hpp does not expose',build)
        self.assertIn("$boostLib=Make-Library 'sdio_boost_fs'",build)
        line=next(x for x in build.splitlines() if "$boostLib=Make-Library 'sdio_boost_fs'" in x)
        self.assertIn('BOOST_FILESYSTEM_NO_CXX20_ATOMIC_REF',line)
        self.assertIn('BOOST_FILESYSTEM_SOURCE',line)
        self.assertNotIn('BOOST_FILESYSTEM_SINGLE_THREADED',line)
        self.assertNotIn('BOOST_FILESYSTEM_SINGLE_THREADED',build)
        self.assertLess(build.index('Boost.Filesystem atomic_ref.hpp does not expose'),
                        build.index("Make-Library 'sdio_7z'"))

    def test_both_compile_profiles_use_same_prepared_verified_header(self):
        b=text('tools/integration/build_native_engine.ps1')
        self.assertIn('if(-not $SkipSourceDownload)',b)
        self.assertIn('prepare_sources.ps1',b)
        self.assertIn("$sevenCommon0=Join-Path $origin 'external",b)
        self.assertIn("Make-Library 'sdio_7z'",b)
        self.assertLess(b.index('$sevenCommon0=Join-Path'),b.index("Make-Library 'sdio_7z'"))

    def test_unused_sapi_header_is_removed_from_generated_origin_only(self):
        lock=json.loads(text('tools/integration/source-lock.json'))
        self.assertEqual(lock['origin']['gitBlobs']['source/system.cpp'],
                         '8d5dc7242bc71f7cd49d246b8c4be6168c337dfd')
        changes=json.loads(text('tools/integration/patches.json'))['changes']
        fix=[c for c in changes if c['file']=='source/system.cpp']
        self.assertEqual(len(fix),1)
        self.assertEqual(fix[0]['old'],'#include <sapi.h>')
        self.assertEqual(fix[0]['count'],1)
        self.assertIn('unused',fix[0]['new'])
        prep=text('tools/integration/prepare_sources.ps1')
        self.assertIn('Copy-Item -LiteralPath $origin -Destination $patched -Recurse',prep)
        self.assertIn('Source differs from the reviewed baseline',prep)
        self.assertIn('Get-GitBlobHash',prep)

    def test_native_core_does_not_require_unavailable_sapi_import_library(self):
        build=text('tools/integration/build_native_engine.ps1')
        self.assertNotIn('-lsapi',build)
        self.assertNotIn("'sapi.dll'",build)
        self.assertIn("$systemCpp=Join-Path $origin 'source\\system.cpp'",build)
        self.assertIn('Legacy speech header was not removed',build)
        self.assertIn("$coreLib=Make-Library 'sdio_origin_core'",build)

    def test_fix6_wmi_locator_guid_is_local_and_pinned(self):
        lock=json.loads(text('tools/integration/source-lock.json'))
        self.assertEqual(lock['origin']['gitBlobs']['source/baseboard.cpp'],
                         '6f021396b96ab543f4c54a7c0eb2cca3f7b2f59c')
        patches=json.loads(text('tools/integration/patches.json'))['changes']
        fixes=[p for p in patches if p['file']=='source/baseboard.cpp']
        self.assertEqual(len(fixes),2)
        self.assertTrue(all(p['count']==1 for p in fixes))
        self.assertIn('static const CLSID kSdioWbemLocator',fixes[0]['new'])
        self.assertIn('0x4590F811, 0x1D3A, 0x11D0',fixes[0]['new'])
        self.assertIn('0x89, 0x1F, 0x00, 0xAA, 0x00, 0x4B, 0x2E, 0x24',fixes[0]['new'])
        self.assertIn('CoCreateInstance(kSdioWbemLocator',fixes[1]['new'])
        self.assertIn('IID_IWbemLocator',fixes[1]['new'])
        self.assertIn('CoCreateInstance(CLSID_WbemLocator',fixes[1]['old'])
        self.assertIn('Get-GitBlobHash $path',text('tools/integration/prepare_sources.ps1'))

    def test_fix6_7zip_xz_compilation_unit_supplies_missing_link_symbols(self):
        lock=json.loads(text('tools/integration/source-lock.json'))
        self.assertEqual(lock['origin']['gitBlobs']['external/SevenZ/build/C/Xz.c'],
                         '92bae029e60813bce196bea2915d83abb9c70769')
        build=text('tools/integration/build_native_engine.ps1')
        self.assertIn("$xzSource=Join-Path $origin 'external\\SevenZ\\build\\C\\Xz.c'",build)
        self.assertIn('Get-GitBlobHash $xzSource',build)
        self.assertIn('$seven -contains $xzFull',build)
        self.assertIn('$seven+=$xzFull',build)
        self.assertIn('Xz.c defines XZ_SIG and Xz_Free',build)
        self.assertLess(build.index('$seven+=$xzFull'),build.index("Make-Library 'sdio_7z'"))
        self.assertNotIn('-lwbem_guids_override',build)
        self.assertIn('-lwbemuuid',build)

    def test_fix14_bzip2_crc_linker_provider_is_pinned(self):
        expected='c1709170b53c413717db10bcdda6809417e0b571'
        p='external/SevenZ/build/CPP/7zip/Compress/BZip2Crc.cpp'
        lock=json.loads(text('tools/integration/source-lock.json'))
        script=text('tools/integration/build_native_engine.ps1')
        self.assertEqual(lock['origin']['gitBlobs'][p],expected)
        self.assertEqual(script.count("@{path='"+p.replace('/','\\')+"'; blob='"+expected+"'}"),1)
        self.assertLess(script.index('BZip2Crc.cpp'),script.index("Make-Library 'sdio_7z'"))

    def test_fix14_complete_source_catalog_is_pinned_and_only_missing_modules(self):
        catalog=json.loads(text('tools/integration/7zip-source-catalog.json'))
        self.assertEqual(catalog['schema'],1)
        self.assertEqual(catalog['repository'],'IDisposable/sdio-go')
        self.assertEqual(catalog['commit'],'25468bdb37d46a72ab4e8bf037b4e1a28ffec70e')
        self.assertEqual(len(catalog['files']),82)
        files={p['path']:p['blob'] for p in catalog['files']}
        self.assertEqual(len(files),len(catalog['files']))
        self.assertEqual(files['CPP/7zip/Compress/BZip2Crc.cpp'],
                         'c1709170b53c413717db10bcdda6809417e0b571')
        self.assertTrue(all(re.fullmatch(r'[0-9a-f]{40}',sha) for sha in files.values()))
        self.assertTrue(all(re.fullmatch(r'(?:C|CPP)/[A-Za-z0-9_./-]+\.(?:c|cpp)',p) for p in files))

    def test_fix14_nm_uses_real_compiled_archive_and_limits_to_pinned_sources(self):
        b=text('tools/integration/build_native_engine.ps1')
        ps=text('tools/integration/resolve_7zip.ps1')
        self.assertIn("$nm=Join-Path $mingw 'nm.exe'",b)
        self.assertIn('Find-SevenZipMissingSources',b)
        self.assertIn('for($pass=1;$pass -le 14;$pass++)',b)
        self.assertIn("Write-Utf8 $closureLog",b)
        self.assertIn("'--extern-only' '--demangle' $Archive",ps)
        self.assertIn('Get-GitBlobHash $full',ps)
        self.assertIn('origin-upstream',ps)
        self.assertIn('Get-GitBlobHash $sourceOriginal',ps)
        self.assertIn('if($found.Count -eq 1)',ps)
        self.assertIn('return $add.ToArray()',ps)
        self.assertNotIn('return ,($add.ToArray())',ps)
        self.assertIn('StdAfx|DllExports',ps)
        self.assertNotIn('Remove-Item',ps)
        self.assertNotIn('Invoke-Expression',ps)
        self.assertNotIn('Start-Process',ps)

    def test_fix14_old_driver_engine_still_gated_and_original_matcher_unchanged(self):
        b=text('tools/integration/build_native_engine.ps1')
        self.assertIn("if($EnableExperimentalInstall){$profile='install-test'}",b)
        self.assertIn("$coreLib=Make-Library 'sdio_origin_core'",b)
        self.assertIn('-Wl,--whole-archive',b)
        self.assertIn("$profile='matching'",b)
        patches=json.loads(text('tools/integration/patches.json'))['changes']
        self.assertNotIn('source/matcher.cpp',{p['file'] for p in patches})
        self.assertIn('source/indexing.cpp',{p['file'] for p in patches})

    def test_fix13_byteswap_implementation_is_pinned_and_linked_once(self):
        p='external/SevenZ/build/C/SwapBytes.c'
        sha='d382c327a0c142b4383234cbeed98a99b75a3293'
        lock=json.loads(text('tools/integration/source-lock.json'))
        build=text('tools/integration/build_native_engine.ps1')
        entry="@{path='"+p.replace('/','\\')+"'; blob='"+sha+"'}"
        self.assertEqual(lock['origin']['gitBlobs'][p],sha)
        self.assertEqual(build.count(entry),1)
        self.assertIn('z7_SwapBytes2/4',build)
        self.assertIn('z7_SwapBytesPrepare',build)
        self.assertLess(build.index(entry),build.index('$seven+=$unitFull'))
        self.assertLess(build.index('$seven+=$unitFull'),build.index("Make-Library 'sdio_7z'"))

    def test_fix13_all_pinned_linker_units_unique_and_match_source_lock(self):
        lock=json.loads(text('tools/integration/source-lock.json'))['origin']['gitBlobs']
        build=text('tools/integration/build_native_engine.ps1')
        entries=re.findall(r"@\{path='([^']+)'; blob='([0-9a-f]{40})'\}",build)
        files=[p.replace('\\','/') for p,_ in entries]
        self.assertEqual(build.count('$linkerDependencies=@('),1)
        self.assertEqual(len(files),len(set(files)))
        self.assertIn('external/SevenZ/build/C/SwapBytes.c',files)
        self.assertIn('external/SevenZ/build/C/HuffEnc.c',files)
        self.assertIn('external/SevenZ/build/CPP/7zip/Compress/BitlDecoder.cpp',files)
        self.assertGreaterEqual(len(files),10)
        for p,sha in entries:
            self.assertEqual(lock[p.replace('\\','/')],sha)
        self.assertIn('Get-GitBlobHash $unitPath',build)
        self.assertIn('Original SevenZ build list already includes',build)
        self.assertIn('$seven -contains $unitFull',build)

    def test_fix13_does_not_change_original_matcher_or_install_access(self):
        build=text('tools/integration/build_native_engine.ps1')
        changes=json.loads(text('tools/integration/patches.json'))['changes']
        modified={c['file'] for c in changes}
        self.assertNotIn('source/matcher.cpp',modified)
        self.assertIn('source/indexing.cpp',modified)
        self.assertIn('EXTRACT_ONLY PROG_VARIANT_R',build)
        self.assertIn("if($EnableExperimentalInstall){$profile='install-test'}",build)
        self.assertIn("$profile='matching'",build)
        self.assertNotIn('void z7_SwapBytes2(',build)

    def test_fix12_original_bit_reverse_table_implementation_is_pinned(self):
        p='external/SevenZ/build/CPP/7zip/Compress/BitlDecoder.cpp'
        h='1236bab4b666f3bcc8874d60be19630cb42af59c'
        lock=json.loads(text('tools/integration/source-lock.json'))
        build=text('tools/integration/build_native_engine.ps1')
        expected="@{path='"+p.replace('/','\\')+"'; blob='"+h+"'}"
        self.assertEqual(lock['origin']['gitBlobs'][p],h)
        self.assertEqual(build.count(expected),1)
        self.assertIn('NBitl::kReverseTable',build)
        self.assertIn('definition and initializer',build)
        self.assertLess(build.index(expected),build.index('$seven+=$unitFull'))
        self.assertLess(build.index('$seven+=$unitFull'),build.index("Make-Library 'sdio_7z'"))
        self.assertIn('Get-GitBlobHash $unitPath',build)

    def test_fix12_no_duplicate_original_linker_entries_and_both_sources_pinned(self):
        build=text('tools/integration/build_native_engine.ps1')
        lock=json.loads(text('tools/integration/source-lock.json'))['origin']['gitBlobs']
        self.assertEqual(build.count('$linkerDependencies=@('),1)
        entries=re.findall(r"@\{path='([^']+)'; blob='([0-9a-f]{40})'\}",build)
        paths=[p.replace('\\','/') for p,_ in entries]
        self.assertEqual(len(paths),len(set(paths)))
        self.assertEqual(len([p for p in paths if p.endswith('BitlDecoder.cpp')]),1)
        self.assertEqual(len([p for p in paths if p.endswith('HuffEnc.c')]),1)
        for p,sha in entries:
            self.assertEqual(lock[p.replace('\\','/')],sha)
        self.assertIn('Original SevenZ build list already includes',build)
        self.assertIn('SevenZ linker unit hash mismatch',build)
        self.assertIn('$seven -contains $unitFull',build)

    def test_fix12_does_not_change_matching_or_installation_permission(self):
        build=text('tools/integration/build_native_engine.ps1')
        patched=json.loads(text('tools/integration/patches.json'))['changes']
        files={group['file'] for group in patched}
        self.assertNotIn('source/matcher.cpp',files)
        self.assertIn('source/indexing.cpp',files)
        self.assertIn("$profile='matching'",build)
        self.assertIn("if($EnableExperimentalInstall){$profile='install-test'}",build)
        self.assertIn('EXTRACT_ONLY PROG_VARIANT_R',build)
        self.assertIn('runtimeTested=$false',build)
        self.assertNotIn('Byte kReverseTable[256];',build)

    def test_fix11_original_huffman_encoder_is_pinned_and_included(self):
        p='external/SevenZ/build/C/HuffEnc.c'
        h='04faf9e824fd3c1d594c4306fb83c106b6984f7c'
        lock=json.loads(text('tools/integration/source-lock.json'))
        build=text('tools/integration/build_native_engine.ps1')
        expected="@{path='"+p.replace('/','\\')+"'; blob='"+h+"'}"
        self.assertEqual(lock['origin']['gitBlobs'][p],h)
        self.assertEqual(build.count(expected),1)
        self.assertIn('Huffman_Generate',build)
        self.assertIn('already-linked HeapSort',build)
        self.assertLess(build.index(expected),build.index('$seven+=$unitFull'))
        self.assertLess(build.index(expected),build.index("Make-Library 'sdio_7z'"))
        self.assertIn('Get-GitBlobHash $unitPath',build)

    def test_fix11_huffman_unit_has_consistent_source_pin_and_no_duplicate(self):
        build=text('tools/integration/build_native_engine.ps1')
        locked=json.loads(text('tools/integration/source-lock.json'))['origin']['gitBlobs']
        self.assertEqual(build.count('$linkerDependencies=@('),1)
        self.assertIn('SevenZ linker unit hash mismatch',build)
        self.assertIn('$seven -contains $unitFull',build)
        self.assertIn('Original SevenZ build list already includes',build)
        entries=re.findall(r"@\{path='([^']+)'; blob='([0-9a-f]{40})'\}",build)
        paths=[p.replace('\\','/') for p,_ in entries]
        self.assertEqual(len(paths),len(set(paths)))
        for path,sha in entries:
            self.assertEqual(locked['external/SevenZ/build/C/HuffEnc.c'] if path.endswith('HuffEnc.c') else locked[path.replace('\\','/')],sha)

    def test_fix11_preserves_matching_and_installation_safety(self):
        build=text('tools/integration/build_native_engine.ps1')
        patches=json.loads(text('tools/integration/patches.json'))['changes']
        files={v['file'] for v in patches}
        self.assertNotIn('source/matcher.cpp',files)
        self.assertIn('source/indexing.cpp',files)
        self.assertIn("$profile='matching'",build)
        self.assertIn("if($EnableExperimentalInstall){$profile='install-test'}",build)
        self.assertIn('EXTRACT_ONLY PROG_VARIANT_R',build)
        self.assertIn('runtimeTested=$false',build)
        self.assertNotIn('void Huffman_Generate(',build)
        self.assertIn('No placeholder or fake functions are used',build)

    def test_fix10_original_lz_out_window_implementation_is_pinned(self):
        p='external/SevenZ/build/CPP/7zip/Compress/LzOutWindow.cpp'
        h='213e0a69ce56e47d7501dea4f1a8bce42929196d'
        lock=json.loads(text('tools/integration/source-lock.json'))
        b=text('tools/integration/build_native_engine.ps1')
        self.assertEqual(lock['origin']['gitBlobs'][p],h)
        self.assertEqual(b.count("@{path='"+p.replace('/','\\')+"'; blob='"+h+"'}"),1)
        self.assertIn('RAR1/2 decoders call CLzOutWindow::Init',b)
        self.assertIn('Get-GitBlobHash $unitPath',b)
        self.assertLess(b.index('LzOutWindow.cpp'),b.index("Make-Library 'sdio_7z'"))

    def test_fix10_original_ppmd_codec_definitions_are_pinned(self):
        expected={
            'PpmdDecoder.cpp':'e2399dc4b10923d8bf0b17a77557331ea6261cb1',
            'PpmdEncoder.cpp':'16a40d69f52f1278c65cf63971bac354718bd3ff',
        }
        lock=json.loads(text('tools/integration/source-lock.json'))
        b=text('tools/integration/build_native_engine.ps1')
        for name,h in expected.items():
            p='external/SevenZ/build/CPP/7zip/Compress/'+name
            self.assertEqual(lock['origin']['gitBlobs'][p],h)
            frag="@{path='"+p.replace('/','\\')+"'; blob='"+h+"'}"
            self.assertEqual(b.count(frag),1)
            self.assertLess(b.index(frag),b.index('$seven+=$unitFull'))
        self.assertEqual(b.count('$linkerDependencies=@('),1)
        self.assertIn('Pinned SevenZ linker unit hash mismatch',b)
        self.assertIn('$seven -contains $unitFull',b)
        self.assertIn('$seven+=$unitFull',b)

    def test_fix10_preserves_matching_and_installation_safety(self):
        b=text('tools/integration/build_native_engine.ps1')
        patches=json.loads(text('tools/integration/patches.json'))['changes']
        self.assertNotIn('source/matcher.cpp',[p['file'] for p in patches])
        self.assertIn('source/indexing.cpp',[p['file'] for p in patches])
        self.assertIn("$profile='matching'",b)
        self.assertIn("if($EnableExperimentalInstall){$profile='install-test'}",b)
        self.assertIn('runtimeTested=$false',b)
        self.assertIn('parityTested=$false',b)
        self.assertIn(' -Wl,--whole-archive ',b)
        self.assertIn(' -Wl,--no-whole-archive ',b)
        self.assertIn('No placeholder or fake functions are used',b)
        self.assertNotIn('void CLzOutWindow::Init(bool)',b)
        self.assertNotIn('NCompress::NPpmd::CEncoder::CEncoder()',b)
        self.assertIn('foreach($entry in $lock.origin.gitBlobs.PSObject.Properties)',text('tools/integration/prepare_sources.ps1'))

    def test_fix9_rar3_vm_constructors_link_from_original_pinned_source(self):
        lock=json.loads(text('tools/integration/source-lock.json'))
        path='external/SevenZ/build/CPP/7zip/Compress/Rar3Vm.cpp'
        blob='e6bfd7ea689bcd1fc07cbfee99fdd4678c16a282'
        self.assertEqual(lock['origin']['gitBlobs'][path],blob)
        b=text('tools/integration/build_native_engine.ps1')
        fragment="@{path='external\\SevenZ\\build\\CPP\\7zip\\Compress\\Rar3Vm.cpp'; blob='"+blob+"'}"
        self.assertEqual(b.count(fragment),1)
        self.assertIn('$linkerDependencies=@(',b)
        self.assertIn('Get-GitBlobHash $unitPath',b)
        self.assertIn('Pinned SevenZ linker unit hash mismatch',b)
        self.assertIn('$seven -contains $unitFull',b)
        self.assertLess(b.index(fragment),b.index('$seven+=$unitFull'))
        self.assertLess(b.index('$seven+=$unitFull'),b.index("Make-Library 'sdio_7z'"))
        # No VM placeholders: this new source is supplied from the validated upstream snapshot.
        self.assertNotIn('NVm::CVm::CVm()',b)
        self.assertNotIn('NCompress::NRar3::NVm::CVm::CVm()',b)

    def test_fix9_matching_profile_keeps_driver_installer_disabled(self):
        b=text('tools/integration/build_native_engine.ps1')
        self.assertIn("$profile='matching'",b)
        self.assertIn("if($EnableExperimentalInstall){$profile='install-test'}",b)
        self.assertIn('runtimeTested=$false',b)
        self.assertIn('parityTested=$false',b)
        self.assertIn(' -Wl,--whole-archive ',b)
        self.assertIn(' -Wl,--no-whole-archive ',b)
        patches=json.loads(text('tools/integration/patches.json'))['changes']
        self.assertNotIn('source/matcher.cpp',[p['file'] for p in patches])
        self.assertIn('source/indexing.cpp',[p['file'] for p in patches])
        self.assertIn('No placeholder or fake functions are used',b)

    def test_fix9_existing_pinned_sources_and_provenance_unchanged(self):
        lock=json.loads(text('tools/integration/source-lock.json'))
        hashes=lock['origin']['gitBlobs']
        for src,sha in {
           'project/7zip/lib.cbp':'781659afa87155a8459c17fc7abbf084ada5a20b',
           'external/SevenZ/build/C/Sort.c':'0c30ca8bc323ec641272c607fb5c2c48c50e03f2',
           'external/SevenZ/build/CPP/Windows/FileSystem.cpp':'02faf627b10a6722441da7e9bbc03827ec2df744',
           'external/SevenZ/build/CPP/7zip/UI/Common/ZipRegistry.cpp':'f4c27bc93feb46aea9f6f6ee363b5f091271211e',
        }.items():
            self.assertEqual(hashes[src],sha)
        self.assertIn('foreach($entry in $lock.origin.gitBlobs.PSObject.Properties)',text('tools/integration/prepare_sources.ps1'))

    def test_fix8_missing_original_7zip_linker_units_pinned(self):
        expected={
            'external/SevenZ/build/C/Sort.c':'0c30ca8bc323ec641272c607fb5c2c48c50e03f2',
            'external/SevenZ/build/CPP/Windows/FileSystem.cpp':'02faf627b10a6722441da7e9bbc03827ec2df744',
            'external/SevenZ/build/CPP/7zip/UI/Common/ZipRegistry.cpp':'f4c27bc93feb46aea9f6f6ee363b5f091271211e',
        }
        lock=json.loads(text('tools/integration/source-lock.json'))
        build=text('tools/integration/build_native_engine.ps1')
        self.assertEqual(build.count('$linkerDependencies=@('),1)
        self.assertIn('Get-GitBlobHash $unitPath',build)
        self.assertIn('$seven -contains $unitFull',build)
        self.assertIn('$seven+=$unitFull',build)
        self.assertIn('Pinned SevenZ linker unit hash mismatch',build)
        self.assertLess(build.index('$seven+=$unitFull'),build.index("Make-Library 'sdio_7z'"))
        for path,sha in expected.items():
            self.assertEqual(lock['origin']['gitBlobs'][path],sha)
            fragment="@{path='"+path.replace('/','\\')+"'; blob='"+sha+"'}"
            self.assertEqual(build.count(fragment),1,(path,fragment))

    def test_fix8_preserves_real_engine_and_no_install_in_matching_profile(self):
        build=text('tools/integration/build_native_engine.ps1')
        self.assertIn("if($EnableExperimentalInstall){$profile='install-test'}",build)
        self.assertIn("$profile='matching'",build)
        self.assertIn('runtimeTested=$false',build)
        self.assertIn('parityTested=$false',build)
        self.assertIn("$coreLib=Make-Library 'sdio_origin_core'",build)
        self.assertIn(' -Wl,--whole-archive ',build)
        patches=json.loads(text('tools/integration/patches.json'))['changes']
        self.assertNotIn('source/matcher.cpp',[p['file'] for p in patches])
        self.assertIn('source/indexing.cpp',[p['file'] for p in patches])
        self.assertNotIn('void Z7_FASTCALL HeapSort(',build)
        self.assertNotIn('CInfo::Load() {',build)
        self.assertIn('No placeholder or fake functions are used',build)

    def test_fix8_origin_sources_reverified_before_modifications(self):
        build=text('tools/integration/build_native_engine.ps1')
        prep=text('tools/integration/prepare_sources.ps1')
        self.assertIn("if(-not $SkipSourceDownload){& (Join-Path $PSScriptRoot 'prepare_sources.ps1')",build)
        self.assertIn('foreach($entry in $lock.origin.gitBlobs.PSObject.Properties)',prep)
        self.assertIn('Source differs from the reviewed baseline',prep)
        self.assertIn('origin-upstream',prep)
        self.assertIn('origin-patched',prep)
        self.assertIn('Unexpected algorithm modification',prep)

    def test_fix7_xz_sha256_crc64_units_are_all_pinned(self):
        # These four upstream files close the SHA256 and CRC64 linker symbols
        # and preserve optimized implementations (without insecure stubs).
        lock=json.loads(text('tools/integration/source-lock.json'))
        expected={
            'Sha256.c':'b29f06bcd1ec75b18ac7c4c593d8cb0fc0c9403d',
            'Sha256Opt.c':'291e71c120b85aabb2789fe8121b680487bfa687',
            'XzCrc64.c':'a0023a6fa2c8b459f3d931f399a31f3f20225397',
            'XzCrc64Opt.c':'08a419ca526349c70740853b525b1c8e64f88e72',
        }
        for file,blob in expected.items():
            self.assertEqual(lock['origin']['gitBlobs']['external/SevenZ/build/C/'+file],blob)
        build=text('tools/integration/build_native_engine.ps1')
        self.assertIn('$xzDependencies=@(',build)
        self.assertIn('Get-GitBlobHash $dependencyPath',build)
        self.assertIn("$seven+=$dependencyFull",build)
        self.assertLess(build.index('$seven+=$xzFull'),build.index('$xzDependencies=@('))
        self.assertLess(build.index('$seven+=$dependencyFull'),build.index("Make-Library 'sdio_7z'"))
        for file,blob in expected.items():
            self.assertEqual(build.count("@{name='"+file+"'; blob='"+blob+"'}"),1)
        self.assertIn('$seven -contains $dependencyFull',build)

    def test_fix7_does_not_use_sha_or_crc_stubs_or_disable_install_gates(self):
        build=text('tools/integration/build_native_engine.ps1')
        self.assertNotIn('void Sha256_Init(',build)
        self.assertNotIn('UInt64 Crc64Update(',build)
        self.assertNotIn('Z7_COMPILER_SHA256_SUPPORTED=0',build)
        self.assertIn("if($EnableExperimentalInstall){$profile='install-test'}",build)
        self.assertIn('runtimeTested=$false',build)
        self.assertIn('parityTested=$false',build)
        self.assertNotIn('-lsapi',build)
        patches=json.loads(text('tools/integration/patches.json'))['changes']
        self.assertNotIn('source/matcher.cpp',[p['file'] for p in patches])
        self.assertIn('source/indexing.cpp',[p['file'] for p in patches])

    def test_fix7_idempotent_unit_wrappers_do_not_rebuild_boost_and_webp_every_time(self):
        build=text('tools/integration/build_native_engine.ps1')
        self.assertIn('if(-not (Test-Path -LiteralPath $out -PathType Leaf)',build)
        self.assertIn('[IO.File]::ReadAllText($out) -cne $wrapperContent',build)
        self.assertIn('if(-not (Test-Path -LiteralPath $pro -PathType Leaf)',build)
        self.assertIn('[IO.File]::ReadAllText($pro) -cne $body',build)
        self.assertIn('Build-Qmake $qmake $make',build)
        self.assertIn('prepare_sources.ps1',build)
        self.assertIn('Source differs from the reviewed baseline',text('tools/integration/prepare_sources.ps1'))

    def test_fix6_keeps_original_matching_and_install_mode_separation(self):
        patches=json.loads(text('tools/integration/patches.json'))['changes']
        self.assertNotIn('source/matcher.cpp',[p['file'] for p in patches])
        self.assertIn('source/indexing.cpp',[p['file'] for p in patches])
        build=text('tools/integration/build_native_engine.ps1')
        self.assertIn('if($EnableExperimentalInstall)',build)
        self.assertIn("$profile='matching'",build)
        self.assertIn('runtimeTested=$false',build)
        self.assertIn('parityTested=$false',build)

    def test_bootstrap_preserves_original_bytes_and_checks_preconditions(self):
        prep=text('tools/integration/prepare_sources.ps1')
        self.assertIn('Get-GitBlobHash',prep)
        self.assertIn('GetEncoding(28591)',prep)
        self.assertIn('Patch precondition failed',prep)
        self.assertIn('Unexpected algorithm modification',prep)
        self.assertIn('origin-upstream',prep)
        self.assertIn('origin-patched',prep)

    def test_archives_do_not_extract_arbitrary_paths(self):
        common=text('tools/integration/common.ps1')
        self.assertIn('ZIP entry escapes the destination',common)
        self.assertIn('Symbolic links are not accepted',common)
        self.assertIn('Only HTTPS',common)
        self.assertIn('SHA256 mismatch',common)

    def test_build_does_not_capture_compiler_log_as_library_path(self):
        common=text('tools/integration/common.ps1')
        self.assertIn('@Extra | Out-Host',common)
        self.assertIn("& $Make '-j2' | Out-Host",common)

    def test_single_file_requires_static_sdk(self):
        self.assertIn('!contains(QT_CONFIG, static):error',text('SDIO_Modern_Qt.pro'))
        build=text('tools/integration/build_native_engine.ps1')
        self.assertIn('if($SingleFile -and -not $staticQt)',build)
        self.assertIn('Single-file dependency audit',build)
        self.assertIn('runtimeTested=$false',build)

    def test_helper_is_compiled_not_opaque_download(self):
        build=text('tools/integration/build_install_helper.ps1')
        self.assertIn('install64_xp.c',build)
        self.assertIn('/NODEFAULTLIB /ENTRY:helperEntry',build)
        self.assertIn('0x8664',build)
        self.assertNotIn('DownloadFile',build)

    def test_manifest_declares_modern_windows_version(self):
        ET.fromstring(text('app.manifest'))
        self.assertIn('8e0f7a12-bfb3-4fe8-b9a5-48fd50a15a9a',text('app.manifest'))
        self.assertIn('level="asInvoker"',text('app.manifest'))
        self.assertIn('1 24 "app.manifest"',text('resources.rc'))

    def test_qmake_and_cmake_declared_files_exist(self):
        for config in ('SDIO_Modern_Qt.pro','CMakeLists.txt'):
            paths=re.findall(r'\b((?:src|native)/[a-z_]+\.(?:cpp|h))',text(config))
            self.assertTrue(paths)
            for path in paths:
                self.assertTrue((ROOT/path).is_file(),(config,path))

    def test_qobject_headers_registered(self):
        pro=text('SDIO_Modern_Qt.pro');cmake=text('CMakeLists.txt')
        for h in (ROOT/'src').glob('*.h'):
            if 'Q_OBJECT' in h.read_text():
                self.assertIn('src/'+h.name,pro)
                self.assertIn('src/'+h.name,cmake)

    def test_resources_present(self):
        xml=ET.fromstring(text('icons.qrc'))
        for file in xml.iter('file'):
            self.assertTrue((ROOT/file.text).is_file(),file.text)

    def test_no_qt57_temporarydir_api_regression(self):
        for path in (ROOT/'src').glob('*.cpp'):
            self.assertNotIn('m_workDir->filePath(',path.read_text())

if __name__=='__main__':
    unittest.main(verbosity=2)
