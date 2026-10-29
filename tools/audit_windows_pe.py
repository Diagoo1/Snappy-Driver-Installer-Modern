#!/usr/bin/env python3
"""Minimal read-only PE inspector, stdlib only.

Checks x86 PE + imports to flag accidental Qt DLL or browser dependencies.
This does not replace a Windows XP VM test or a malware scanner.
"""
import argparse
import struct
from pathlib import Path

def inspect(path, with_symbols=False):
    data = Path(path).read_bytes()
    if len(data) < 0x100 or data[:2] != b'MZ':
        raise ValueError('Not an MZ executable')
    pe = struct.unpack_from('<I', data, 0x3c)[0]
    if data[pe:pe+4] != b'PE\0\0':
        raise ValueError('Missing PE signature')
    machine, number, _ts, _symbols, _num_symbols, optsz, flags = struct.unpack_from('<HHIIIHH', data, pe+4)
    opt = pe+24
    magic = struct.unpack_from('<H', data, opt)[0]
    if magic not in (0x10b, 0x20b):
        raise ValueError('Invalid optional PE header')
    is32 = magic == 0x10b
    data_dir = opt+(96 if is32 else 112)
    import_rva, _ = struct.unpack_from('<II',data,data_dir+8)
    sections = []
    for i in range(number):
        o = opt+optsz+i*40
        name = data[o:o+8].split(b'\0')[0].decode('ascii','replace')
        vsize, va, size, offset = struct.unpack_from('<IIII',data,o+8)
        sections.append((name,va,max(vsize,size),offset))
    def offset_of(rva):
        for _,va,sz,pos in sections:
            if va <= rva < va+sz:
                return pos+rva-va
        raise ValueError('Unmapped PE import RVA')
    imports = []
    symbols = []
    if import_rva:
        o = offset_of(import_rva)
        for j in range(4096):
            original, timestamp, forwarder, name_rva, first = struct.unpack_from('<IIIII',data,o+j*20)
            if not (original or timestamp or forwarder or name_rva or first): break
            n = offset_of(name_rva)
            end = data.find(b'\0',n,n+200)
            if end < 0: raise ValueError('Unterminated import name')
            imports.append(data[n:end].decode('ascii','replace'))
            thunk = offset_of(original or first)
            width = 4 if is32 else 8
            for k in range(65536):
                value = struct.unpack_from('<I' if is32 else '<Q', data, thunk+k*width)[0]
                if not value: break
                if value & (1 << (width*8-1)): continue  # ordinal import
                start = offset_of(value)+2  # IMAGE_IMPORT_BY_NAME hint
                end = data.find(b'\0',start,start+512)
                if end < 0: raise ValueError('Unterminated imported symbol')
                symbols.append((imports[-1],data[start:end].decode('ascii','replace')))
    return (machine,imports,symbols) if with_symbols else (machine,imports)

def main():
    p=argparse.ArgumentParser()
    p.add_argument('exe',type=Path)
    p.add_argument('--require-x86', action='store_true')
    p.add_argument('--single-exe',action='store_true')
    p.add_argument('--check-xp-imports',action='store_true')
    args=p.parse_args()
    machine,imports,symbols=inspect(args.exe,with_symbols=True)
    print('Machine:',{0x14c:'x86',0x8664:'x64'}.get(machine,hex(machine)))
    print('Imports:',', '.join(imports))
    errors=[]
    if args.require_x86 and machine != 0x14c:
        errors.append('This is not Win32 x86.')
    forbidden = ['qt5core','qt5gui','qt5widgets','qt6','webview2','msedge','chrome',
                 'libstdc++','libgcc','libwinpthread','vcruntime','msvcp']
    if args.single_exe:
        for name in imports:
            if any(name.lower().startswith(f) for f in forbidden):
                errors.append('Not standalone or contains forbidden browser dependency: '+name)
    if args.check_xp_imports:
        # This targeted loader audit is deliberately not an exhaustive XP certificate.
        newer = {'GetTickCount64','CancelIoEx','InitializeCriticalSectionEx',
                 'GetFileInformationByHandleEx','SetFileInformationByHandle',
                 'GetQueuedCompletionStatusEx','GetFinalPathNameByHandleW',
                 'GetFinalPathNameByHandleA','GetSystemTimePreciseAsFileTime',
                 'InitializeConditionVariable','SleepConditionVariableCS',
                 'SleepConditionVariableSRW','WakeConditionVariable',
                 'WakeAllConditionVariable','InitializeSRWLock',
                 'AcquireSRWLockExclusive','AcquireSRWLockShared',
                 'ReleaseSRWLockExclusive','ReleaseSRWLockShared',
                 'InitOnceExecuteOnce','GetDpiForWindow','SetThreadDpiAwarenessContext'}
        for dll,name in symbols:
            if name in newer: errors.append('Post-XP static import: '+dll+'!'+name)
        for dll in imports:
            if dll.lower() in {'dwmapi.dll','bcrypt.dll','ucrtbase.dll'} or dll.lower().startswith(('api-ms-','ext-ms-')):
                errors.append('Post-XP static DLL dependency: '+dll)
        if not errors: print('No known post-XP static imports in',len(symbols),'imported functions.')
    for error in errors: print('ERROR:',error)
    if errors: raise SystemExit(1)
    print('PE audit passed. Runtime compatibility still needs VM testing.')

if __name__=='__main__':main()
