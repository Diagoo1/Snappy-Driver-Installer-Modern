"""Extract public UI copy only. No device data, paths or source code is sent."""
from pathlib import Path
import json,re
ROOT=Path(__file__).resolve().parents[2]
STRING=r'"(?:[^"\\]|\\.)*"'
CONCAT=r'((?:'+STRING+r'\s*)+)'
def decode(value):
    return ''.join(json.loads(token) for token in re.findall(STRING,value))
def visible(value):
    if not value or len(value)>850 or not re.search('[A-Za-z]',value):return False
    if re.fullmatch(r'[a-z0-9_.-]+\.cpl(?:,\d*)+',value,re.I):return False
    if any('\u0590'<=c<='\u08ff' for c in value):return False
    if any(x in value for x in ('{','}','background:','color:','font-size:','QWidget','QMenu::','QPushButton#','QTreeWidget')):return False
    if value.startswith((':/','http:','https:','mailto:','C:/','Software\\','SOFTWARE\\','--','^')):return False
    if re.search(r'\.(?:cpp|hpp|h|exe|dll|json|ini|log|bin|7z|snp|txt|svg|png|jpg)(?:$|\*)',value,re.I):return False
    if any(x in value for x in ('*.','%Y','%p%','USERPROFILE','SystemRoot')):return False
    if re.fullmatch(r'[a-z0-9_/-]+',value):return False
    if re.fullmatch(r'[A-Za-z0-9_.:/%#-]+',value) and not re.fullmatch(r'[A-Z][a-z]+',value):return False
    if value.startswith(('Accept:','SnappyDriverInstaller /')):return False
    if value in {'Windows','Cairo','Satoshi','Diagoo1','GitHub','INF','SDIO','HWID','JSON','DISM','USB'}:return False
    return True
def original(folder):
    return {m[0]:m[1].replace(r'\n','\n').replace(r'\"','"').strip() for m in re.findall(r'(?m)^\s*([A-Z][A-Z0-9_]*)\s*=\s*"((?:[^"\\]|\\.)*)"',folder.read_text(encoding='utf-8-sig'))}
def collect():
    messages=set();arabic={};sources={}
    paths=list((ROOT/'src').glob('*.cpp'))+[ROOT/'native/origin_native.cpp',ROOT/'native/plan_validation.cpp']
    for path in paths:
        if path.name in {'theme.cpp','theme_pack.cpp','vector_icons.cpp','legacy_script.cpp','legacy_runtime.cpp'}:continue
        text=path.read_text(encoding='utf-8-sig')
        for match in re.finditer(r'\b(?:t|tr|trText|translatedLabel|translatedButton)\s*\(\s*('+STRING+r')\s*,\s*('+STRING+r')',text):
            try:en,ar=decode(match[1]),decode(match[2])
            except ValueError:continue
            if visible(en):messages.add(en);arabic[en]=ar;sources.setdefault(en,[]).append(str(path.relative_to(ROOT)))
        for expression in (r'(?:QStringLiteral|QString::fromUtf8|QString::fromLatin1)\(\s*'+CONCAT+r'\)',r'(?:runtime_error|fail|rejectPlan)\(\s*'+CONCAT+r'\)',r'L('+STRING+r')'):
            for match in re.finditer(expression,text):
                try:value=decode(match[1])
                except ValueError:continue
                if visible(value):messages.add(value);sources.setdefault(value,[]).append(str(path.relative_to(ROOT)))
    # These short states and common dialog verbs are intentionally translated.
    messages.update(('Classic Teal','Snappy Driver Installer','Tarek Sadek','Installed','Missing','Upgrade','Downgrade','Invalid','Ignored','Cancel','Close','Continue','Apply','Browse...','Yes','No','Automatic','Unlimited','Download','Pause','Stop','Resume','Settings','About','Drivers','Diagnostics','Recommended'))
    messages.update(('Made with','by','Love'))
    messages.update(('Undo','Redo','Cut','Copy','Paste','Delete'))
    maintained=ROOT/'tools/i18n/arabic_overrides.json'
    if maintained.exists():arabic.update(json.loads(maintained.read_text(encoding='utf-8')))
    directory=ROOT/'i18n';directory.mkdir(exist_ok=True)
    data={'schema':1,'messages':sorted(messages),'arabic':arabic,'sources':{key:sorted(set(sources.get(key,[]))) for key in sorted(messages)}}
    (directory/'source-catalog.json').write_text(json.dumps(data,ensure_ascii=False,indent=2)+'\n',encoding='utf-8')
    print('UI messages:',len(messages),'characters:',sum(len(x) for x in messages),'manual Arabic:',len(arabic))
    return data
if __name__=='__main__':collect()
