"""Build-time translation of generic UI copy; generated catalogs work offline.
No runtime device data, source files, credentials or user paths are transmitted.
Existing upstream translations and maintained Arabic copy take precedence.
"""
from pathlib import Path
import concurrent.futures,json,re,time,threading,urllib.request,urllib.parse,sys
from collect_catalog import collect,original,ROOT

TAGS={'arabic':'ar','armenian':'hy','azerbaijan':'az','belarusian':'be','brazilian':'pt-BR','bulgarian':'bg','catalan':'ca','chinese_cn':'zh-CN','chinese_tw':'zh-TW','croatian':'hr','czech':'cs','danish':'da','dutch':'nl','english':'en','estonian':'et','farsi':'fa','french':'fr','georgian':'ka','german':'de','greek':'el','hungarian':'hu','indonesian':'id','italian':'it','japanese':'ja','korean':'ko','latvian':'lv','lithuanian':'lt','norwegian':'no','polish':'pl','portuguese':'pt-PT','romanian':'ro','russian':'ru','serbian':'sr','slovenian':'sl','spanish':'es','swedish':'sv','thai':'th','turkish':'tr','ukrainian':'uk','vietnamese':'vi'}
LOCK=threading.Lock();NEXT=0.
PLACEHOLDER=re.compile(r'%[1-9]\d*')
BRANDS=('Snappy Driver Installer Origin','Snappy Driver Installer','Dreams Software Collection','Tarek Sadek','Diagoo1','GitHub','Windows','INF','HWID','SDIO','JSON','DLL','USB','DISM','SHA-256')
def protect(text):
    tokens={}
    for value in list(dict.fromkeys(PLACEHOLDER.findall(text)))+list(BRANDS):
        if value not in text:continue
        token=f'ZZPH{len(tokens):03d}ZZ';tokens[token]=value;text=text.replace(value,token)
    return text,tokens
def unprotect(text,tokens,source):
    if not text.strip():raise ValueError('Empty translation')
    for token,value in tokens.items():
        if not re.search(re.escape(token),text,re.I):raise ValueError('Missing protected token')
        text=re.sub(re.escape(token),lambda _:value,text,flags=re.I)
    if sorted(PLACEHOLDER.findall(text))!=sorted(PLACEHOLDER.findall(source)):raise ValueError('Placeholder mismatch')
    if any(token.lower() in text.lower() for token in tokens):raise ValueError('Unrestored token')
    if ('<' in text or '>' in text) and not ('<' in source or '>' in source):raise ValueError('Unexpected markup')
    leading=re.match(r'^\s*',source)[0];trailing=re.search(r'\s*$',source)[0]
    return leading+text.strip()+trailing
def request(text,tag):
    global NEXT
    for attempt in range(4):
        with LOCK:
            wait=max(0,NEXT-time.monotonic());NEXT=max(NEXT,time.monotonic())+.65
        if wait:time.sleep(wait)
        url='https://translate.googleapis.com/translate_a/single?'+urllib.parse.urlencode({'client':'gtx','sl':'en','tl':tag,'dt':'t','q':text})
        try:
            with urllib.request.urlopen(urllib.request.Request(url,headers={'User-Agent':'SDIO-localization-build/1.0'}),timeout=35) as response:data=json.load(response)
            return ''.join(part[0] or '' for part in data[0])
        except Exception:
            if attempt==3:raise
            time.sleep(min(40,3**(attempt+1)))
def translate_batch(messages,tag):
    lines=[];protected=[]
    for i,source in enumerate(messages):
        value,tokens=protect(source);protected.append(tokens);lines.append(f'ZXQ{i:04d}ZX '+value.replace('\n',' '))
    answer=request('\n'.join(lines),tag)
    markers=list(re.finditer(r'ZXQ\s*(\d{4})\s*ZX',answer,re.I));found={}
    for n,marker in enumerate(markers):
        index=int(marker[1]);end=markers[n+1].start() if n+1<len(markers) else len(answer)
        if index<len(messages):
            try:found[messages[index]]=unprotect(answer[marker.end():end],protected[index],messages[index])
            except ValueError:pass
    for source,tokens in zip(messages,protected):
        if source in found:continue
        value,_=protect(source)
        try:found[source]=unprotect(request(value,tag),tokens,source)
        except ValueError:
            # Some providers drop or translate protective markers. Translate
            # prose segments separately and join the original tokens exactly.
            chunks=re.split(r'(ZZPH\d{3}ZZ)',value);parts=[]
            for chunk in chunks:
                if chunk in tokens:parts.append(tokens[chunk])
                elif not re.search(r'[A-Za-z]',chunk):parts.append(chunk)
                else:
                    translated=request(chunk,tag).strip()
                    if not translated:raise ValueError('Empty prose segment')
                    parts.append(re.match(r'^\s*',chunk)[0]+translated+re.search(r'\s*$',chunk)[0])
            found[source]=unprotect(''.join(parts),{},source)
    return found
def write(path,data):
    temporary=path.with_suffix(path.suffix+'.tmp');temporary.write_text(json.dumps(data,ensure_ascii=False,indent=2)+'\n',encoding='utf-8');temporary.replace(path)
def run_locale(locale,tag,catalog,native,english):
    destination=ROOT/'i18n'/f'{locale}.json';saved=json.loads(destination.read_text(encoding='utf-8')) if destination.exists() else {}
    translated=saved.get('messages',{});keys=catalog['messages'];shared={}
    translated={k:v for k,v in translated.items() if v.strip() and not re.search(r'(?:ZZPH|ZXQ)\s*\d+',v,re.I) and sorted(PLACEHOLDER.findall(v))==sorted(PLACEHOLDER.findall(k))}
    inverse={value:key for key,value in english.items() if value}
    for source in keys:
        key=inverse.get(source)
        if key and native.get(key):shared[source]=native[key]
    translated.update(shared)
    if locale=='arabic':translated.update(catalog['arabic']);translated.update({'Installed':'مثبّت','Missing':'مفقود','Upgrade':'ترقية','Downgrade':'إصدار أقدم','Invalid':'غير صالح','Ignored':'متجاهل'})
    translated.update(json.loads((ROOT/'tools/i18n/signature_translations.json').read_text('utf-8')).get(locale,{}))
    translated.update(dict(zip(('Undo','Redo','Cut','Copy','Paste','Delete'),json.loads((ROOT/'tools/i18n/edit_translations.json').read_text('utf-8'))[locale])))
    if locale=='english':translated={key:key for key in keys}
    missing=[key for key in keys if not translated.get(key)]
    def persist():write(destination,{'schema':1,'language':locale,'tag':tag,'rtl':locale in {'arabic','farsi'},'generated':'Upstream human translations, maintained Arabic, and build-time machine translation; native review recommended.','messages':{key:translated[key] for key in keys if key in translated}})
    batches=[];batch=[];length=0
    for source in missing:
        if batch and length+len(source)>2700:batches.append(batch);batch=[];length=0
        batch.append(source);length+=len(source)+15
    if batch:batches.append(batch)
    for batch in batches:translated.update(translate_batch(batch,tag));persist()
    persist();assert all(translated.get(key) for key in keys)
    print(f'{locale}: {len(keys)} / {len(keys)} UI messages; upstream={len(shared)}',flush=True)
    return {'id':locale,'tag':tag,'name':native.get('STR_LANG_NAME',locale),'rtl':locale in {'arabic','farsi'},'messages':len(keys)}
def main():
    catalog=collect();folder=ROOT/'vendor/origin-patched/tools/SDIO/langs';english=original(folder/'english.txt')
    selected={name:tag for name,tag in TAGS.items() if len(sys.argv)==1 or name in sys.argv[1:]}
    results=[];errors=[]
    with concurrent.futures.ThreadPoolExecutor(max_workers=3) as pool:
        futures={pool.submit(run_locale,name,tag,catalog,original(folder/(name+'.txt')),english):name for name,tag in selected.items()}
        for future in concurrent.futures.as_completed(futures):
            try:results.append(future.result())
            except Exception as error:errors.append({'locale':futures[future],'error':type(error).__name__+': '+str(error)});print('FAILED',futures[future],type(error).__name__,str(error),flush=True)
    if len(selected)==len(TAGS):write(ROOT/'i18n/index.json',{'schema':1,'locales':sorted(results,key=lambda x:x['id']),'failures':errors})
    if errors:raise SystemExit(1)
if __name__=='__main__':main()
