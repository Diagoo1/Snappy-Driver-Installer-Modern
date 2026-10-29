"""Package only actual translations and report English fallbacks explicitly."""
import json,re
from pathlib import Path
from collect_catalog import collect,original,ROOT
from build_locales import TAGS,write,PLACEHOLDER

def main():
    catalog=collect();keys=catalog['messages'];folder=ROOT/'vendor/origin-patched/tools/SDIO/langs';english=original(folder/'english.txt');inverse={v:k for k,v in english.items() if v}
    index=[];coverage={}
    for name,tag in TAGS.items():
        path=ROOT/'i18n'/f'{name}.json';saved=json.loads(path.read_text('utf-8')) if path.exists() else {};messages=saved.get('messages',{});native=original(folder/(name+'.txt'))
        for key in keys:
            if key in inverse and native.get(inverse[key]):messages[key]=native[inverse[key]]
        if name=='english':messages={key:key for key in keys}
        if name=='arabic':messages.update(catalog['arabic']);messages.update({'Installed':'مثبّت','Missing':'مفقود','Upgrade':'ترقية','Downgrade':'إصدار أقدم','Invalid':'غير صالح','Ignored':'متجاهل'})
        if name=='serbian':messages.update({'Installed':'Инсталирано','Missing':'Недостаје','Upgrade':'Надоградња','Downgrade':'Старија верзија','Invalid':'Неисправно','Ignored':'Игнорисано','Settings':'Подешавања','About':'О програму','Drivers':'Управљачки програми','Diagnostics':'Дијагностика','Cancel':'Откажи','Close':'Затвори','Continue':'Настави','Apply':'Примени','Yes':'Да','No':'Не','Automatic':'Аутоматски','Download':'Преузми','Pause':'Паузирај','Stop':'Заустави','Resume':'Настави','Recommended':'Препоручено'})
        messages={key:value for key,value in messages.items() if key in keys and value.strip() and sorted(PLACEHOLDER.findall(key))==sorted(PLACEHOLDER.findall(value)) and not re.search(r'(?:ZZPH|ZXQ)\s*\d+',value,re.I)}
        missing=[key for key in keys if key not in messages]
        write(path,{'schema':1,'language':name,'tag':tag,'rtl':name in ('arabic','farsi'),'generated':'Upstream human translations, maintained Arabic, and build-time machine translation; native review recommended. Missing keys use English.','messages':messages})
        index.append({'id':name,'tag':tag,'name':native.get('STR_LANG_NAME',name),'rtl':name in ('arabic','farsi'),'messages':len(messages),'englishFallbacks':len(missing)})
        coverage[name]={'translated':len(messages),'total':len(keys),'englishFallbacks':missing}
    write(ROOT/'i18n/index.json',{'schema':1,'locales':sorted(index,key=lambda x:x['id']),'nativeReviewRequired':True})
    write(ROOT/'docs/LOCALIZATION_COVERAGE.json',coverage)
    qrc=ROOT/'icons.qrc';text=qrc.read_text('utf-8');text=re.sub(r'\s*<file>i18n/[^<]+</file>','',text)
    text=text.replace('  </qresource>',''.join('    <file>i18n/'+name+'.json</file>\n' for name in sorted(TAGS))+ '    <file>i18n/index.json</file>\n  </qresource>');qrc.write_text(text,'utf-8')
    print('Embedded locales:',len(index),'source strings:',len(keys),'English fallbacks:',{k:len(v['englishFallbacks']) for k,v in coverage.items() if v['englishFallbacks']})
if __name__=='__main__':main()
