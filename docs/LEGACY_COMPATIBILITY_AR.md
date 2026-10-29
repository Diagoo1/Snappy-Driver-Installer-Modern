# توافق السكربتات والأوامر — 0.21.0

يوجد مفسّر للصيغة القديمة بجانب خطط JSON. يستدعي المحرك المدمج، وليس برنامجًا أصليًا خارجيًا. المرجع مصدر SDIO المثبت؛ قد تختلف فروع SDI الأخرى.

## التحقق والتنفيذ

```
SnappyDriverInstaller-TS.exe --validate-script plan.txt --legacy-report validation.json
SnappyDriverInstaller-TS.exe -script:plan.txt parameter1 --legacy-report result.json
```

الأول يحلل البرنامج دون أوامر أو تثبيت أو تنزيل أو إعادة تشغيل. الثاني ينفذ الملف الصريح. يدعم UTF-8 وUTF-16 والاقتباس، `%NAME%` للبيئة، `%0` لمسار السكربت و`%1` إلى `%9` للمعاملات. يفحص العلامات والقفزات قبل التنفيذ، ويحد حجم الملفات ودوران التعليمات.

الأوامر: `init [reindex]`, `activetorrent`, `select`, `checkupdates`, `get`, `install`, `snapshot`, `loadsnapshot`, `unloadsnapshot`, `restorepoint`, `writedevicelist`, `logdir`, `drpdir`, `indexdir`, `extractdir`, `torrentport`, `keeptempfiles`, `echo`, `debug`, `logging`, `cmd`, `verbose`, `enableinstall`, `pause`, `reboot [ifneeded]`, `runlatest`, `onerror`, `goto`, `end`.

`select` يستخدم حالات وترتيب المحرك الأصلي. `get app` و`runlatest` يخصان مشروعنا. `cmd` و`install` و`reboot` لها أثر فعلي عند تنفيذ السكربت؛ وضع التحقق لا ينفذها. اللقطة ومعاينة نظام آخر تمنعان التثبيت. فشل نقطة الاستعادة يوقف الخطة إلا مع `-nostop` الصريح.

مثال قراءة فقط (أنشئ مجلد الإخراج أولًا):

```
drpdir "D:\DriverLibrary\drivers"
indexdir "D:\DriverLibrary\indexes"
enableinstall off
init
snapshot "D:\Reports\machine.snp"
writedevicelist "D:\Reports\devices.txt"
end
```

## خيارات التشغيل

تشمل `-script:`, `-cfg:`, `-drp_dir:`, `-index_dir:`, `-log_dir:`, `-output_dir:`, `-reindex`, `-index_hr`, `-getdevicelist:`, `-ls:`, `-a:`, `-v:`, `-filters:`, `-autoinstall`, `-autoupdate`, `-autoclose`, `-nogui`, `-extractdir:`, `-disableinstall`, `-restorepoint`, `-norestorepoint`, `-nostop`, `-failsafe`, `-filtersp`, `-dpinstmode`, `-delextrainfs`, `-keeptempfiles`, `-keepunpackedindex`, `-nologfile`, `-verbose:`, `-finish_cmd:`, `-finishrb_cmd:`, `-finishup_cmd:` وخيارات التورنت. ضع الخيار كله بين اقتباسين إذا احتوى مسافات.

`-install HWID INF` يستخدم التثبيت الأصلي بعد التحقق من جهاز حالي واحد مطابق وصلاحية المسؤول. `-save-installed-id[:file]` يسجل معرّفات التثبيت؛ `-HWIDInstalled:needle[=file]` استعلام قراءة يعيد 1 عند العثور و0 عند الغياب كما في الأصل. `-7z` يمرر أمر الأرشيف إلى المكتبة المدمجة.

خيارات اللغة والمكتبة ومعاينة النظام والتكبير تنتقل لجلسة الواجهة الحديثة. `-preservecfg` يمنع حفظ خيارات الجلسة. مقياس `-scale` القديم 256 يعني 100% ويخضع لقيود الشاشة. `-theme` يقبل JSON الخاص بالمشروع، وليس ثيم الصور القديم.

خيارات الرسم القديم وشكل أسماء الحزم والتنبيهات القديمة تُقبل للتوافق مع ملفات التشغيل، لكن بعضها ليس له تأثير مرئي مستقل في تصميمنا. المشاركة تتوقف عند إغلاق البرنامج.

اختُبر تدفق السكربتات ببدائل وهمية للعمليات المؤثرة، وأوامر قراءة وصيانة بالمحرك. التثبيت الحقيقي وDPInst ونقطة الاستعادة وأوامر النظام المؤثرة تحتاج جهاز تجارب.
