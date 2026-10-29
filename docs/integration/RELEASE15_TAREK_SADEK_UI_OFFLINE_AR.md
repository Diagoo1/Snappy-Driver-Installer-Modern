# Release 15 — Tarek Sadek Driver Suite (source handoff)

النسخة مبنية فوق Fix 14 وتحافظ على نفس سورس SDIO Origin ومراحل التحقق من البصمات.

## التنفيذ

- اسم نافذة التطبيق و EXE و Windows VERSIONINFO: **Tarek Sadek Driver Suite**.
- استخدام اللوجو المرفوع في الهيدر، شاشة البداية، ملف ICO، وأيقونة EXE.
- حقوق مساهمات الواجهة والهوية الجديدة: (c) 2026 Tarek Sadek. تنويه: السورس الأصلي وجميع أعمال المؤلفين الأصليين تخضع لتراخيصها (GPL-3.0-or-later وغيرها) ولا يجوز إزالة نسبتها.
- Popover بخلفية شفافة وحدود ظاهرة فقط على الكارد الداخلي، ظل محسّن ومسافات أكثر.
- 3 إحصائيات مستقلة بالأيقونات والألوان لعدد الأجهزة وتحذيرات PnP والتحديد، نابعة من نفس model الحقيقي.
- قارئ ترجمة SDIO Origin من ملفات `langs/*.txt` الأصلية؛ build ينسخ ملفات الأصل الـ 41 بدون اختلاق ترجمات. الإنجليزية والعربية متاحتان للواجهة الحديثة، أما بقية اللغات فيتم استخدام نصوص الأصل حيث توجد دلالات مطابقة ويتبقى نصوص Qt المخصصة بالإنجليزية.
- صفحة التثبيت لا تظهر في قائمة التنقل في Build `matching`، ولا يتم تفعيل التثبيت بالقوة. الوضع demo متاح فقط بوسيط التشغيل `--demo`، ووضع `install-test` منفصل ومشروط.
- Diagnostics: تم جلب أجهزة PnP الحاضرة مباشرة من Windows SetupAPI، وليس فقط الأجهزة التي وجد لها محرك SDIO باك، ومن ضمنها Wi-Fi/Bluetooth/Display/Storage/USB/Audio/Battery/TPM. الغائب يظهر «Not detected among present PnP devices» وليس «الجهاز غير موجود قطعًا». إضافة قياس معدل تحديث الشاشة من Win32، واستخدام بيانات الطاقة والنظام الحالية. لا تتم عمليات كتابة أو تثبيت ضمن التشخيص.
- رصد `drivers/indexes/tools` بجانب EXE تلقائيًا؛ المجلد tools اختياري، وتظل عملية التحميل عبر الإنترنت غير مدعومة.
- سكربت `04_PACKAGE_OFFLINE_PORTABLE.cmd` ينسخ EXE الحالي بعد بناء ناجح ومعاه DLLs/Qt plugins وملفات اللغات إلى المسار المحدد، دون نسخ driver packs أو ملفات SDIO الأصلية.

## تشغيل على Windows 11 مع Qt 5.6.3 / MinGW 4.9.2

1. فك هوتفكس Release 15 على نفس مجلد المشروع (Fix 14) وافق على الاستبدال، مع الاحتفاظ بـ `.source-cache` و `vendor` و `build-origin-matching`.
2. شغّل `01_BUILD_ENGINE_MATCHING.cmd`، وتأكد من `Integrated build produced:` في اللوج، ووجود `dist-origin-matching/TarekSadekDriverSuite.exe`.
3. لتجهيز نسخة بجوار حزمك الأوفلاين، شغّل `04_PACKAGE_OFFLINE_PORTABLE.cmd`. سيستخدم افتراضيًا `D:\Prog\Snappy Driver Installer 1.26.0 (R2600)`. يمكن تعديل مسار OfflineRoot في السكربت لو مختلف.
4. افتح `TarekSadekDriverSuite.exe` من نفس مجلد `drivers` و`indexes`، وراجع Offline packages، ثم نفّذ Scan only، واحفظ اللوج `logs/native-events.jsonl` عند وجود مشكلة.

**لا تستخدم Build الفحص للتثبيت الحقيقي**. Build `02_BUILD_INSTALLATION_TEST.cmd` هو منفصل وغير معتمد على جهازك الأساسي؛ يجب إجراء اختبارات الأمان والمطابقة على آلة معزولة أولاً.

## حدود التحقق

- تم تنفيذ اختبارات source wiring و native C++ على Linux، بدون Windows Qt toolchain. ملفات ZIP لا تحتوي EXE Windows مجمّعًا.
- النجاح على Windows، صحة المطابقة، وعمل Installer / استعادة النظام لم يتم التحقق منهم بهذه الحزمة.
- لا تزال بعض نصوص الواجهة الحديثة غير مترجمة لكل اللغات الـ41. 
