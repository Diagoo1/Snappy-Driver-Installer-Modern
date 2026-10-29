"""Build Cairo static instances; copy unmodified official Satoshi static fonts.

Input downloads live in build-review/dreams-font. Satoshi must be obtained from
Fontshare by each source builder; its raw font files are not GPL source assets.
"""
from pathlib import Path
import hashlib,json,shutil
from fontTools.ttLib import TTFont
from fontTools.varLib.instancer import instantiateVariableFont
ROOT=Path(__file__).resolve().parents[2]
source=ROOT/'build-review/dreams-font'
destination=ROOT/'assets/fonts'
for weight,style in [(400,'Regular'),(500,'Medium'),(700,'Bold')]:
    font=TTFont(source/'Cairo-variable.ttf')
    axes={a.axisTag:(weight if a.axisTag=='wght' else 0 if a.axisTag=='slnt' else a.defaultValue) for a in font['fvar'].axes}
    font=instantiateVariableFont(font,axes,inplace=True)
    for platform,encoding,language in [(3,1,0x409),(1,0,0)]:
        for key,value in [(1,'Cairo'),(2,style),(4,'Cairo '+style),(6,'Cairo-'+style),(16,'Cairo'),(17,style)]:
            font['name'].setName(value,key,platform,encoding,language)
    font['OS/2'].usWeightClass=weight
    font['OS/2'].fsSelection=(font['OS/2'].fsSelection & ~(32|64)) | (32 if weight==700 else 64 if weight==400 else 0)
    font['head'].macStyle=1 if weight==700 else 0
    font.save(destination/('Cairo-'+style+'.ttf'))
    shutil.copyfile(source/('satoshi/Satoshi_Complete/Fonts/WEB/fonts/Satoshi-'+style+'.ttf'),destination/('Satoshi-'+style+'.ttf'))
shutil.copyfile(source/'satoshi/Satoshi_Complete/License/FFL.txt',destination/'Satoshi-FFL.txt')
report={'arabic':{'family':'Cairo','source':'https://github.com/google/fonts/tree/main/ofl/cairo','license':'SIL OFL 1.1','preparation':'Static 400/500/700 instances for Qt 5.6 and older Windows'},'english':{'family':'Satoshi','source':'https://api.fontshare.com/v2/fonts/download/satoshi','license':'ITF FFL 2.0','preparation':'Official static TTF files, unmodified. Download directly from Fontshare for source builds; do not redistribute standalone fonts.'},'files':{p.name:hashlib.sha256(p.read_bytes()).hexdigest() for p in destination.glob('*') if p.name.startswith(('Cairo-','Satoshi-'))}}
(destination/'PROVENANCE.json').write_text(json.dumps(report,indent=2)+'\n',encoding='utf-8')
print(json.dumps(report,indent=2))
