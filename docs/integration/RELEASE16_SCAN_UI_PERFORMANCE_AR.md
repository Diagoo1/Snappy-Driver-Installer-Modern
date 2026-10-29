# Release 16 — Snappy Driver Installer (UI by Tarek Sadek)

- اسم المنتج عاد إلى **Snappy Driver Installer**؛ التخصيصات © 2026 Tarek Sadek، ويظل محرك SDIO الأصلي ومكتبـاته بحقوق مؤلفيها وتراخيصها الأصلية (GPL-3.0-or-later وغيرها).
- ملف EXE الخاص بتعديلنا اسمه **SnappyDriverInstaller-TS.exe** حتى لا يتعارض مع SDI_R2600.exe وSDI_x64_R2600.exe.
- تم إخفاء شاشة البداية الزائفة في نمط الفحص الحقيقي؛ ويُعرض غطاء كامل للشاشة بخلفية ضبابية ونقاط متحركة حتى ينتهي الفحص والتحضير.
- بطء واجهة تعريفات Offline: صفوف البدائل لا تُنشأ حتى يفتح المستخدم تفاصيل الجهاز؛ كاش لأيقونات فئات الأجهزة؛ البحث مؤجل 135ms؛ عدم نسخ DriverEntry بلا داع؛ والحجز المسبق لذاكرة المصفوفات.
- بطء محرك Offline: التحقق من وجود الحزمة والموقع الآمن لها أصبح يُحسب مرة واحدة لكل مسار package في الفحص الواحد، لا لكل تطابق، مع بقاء فحص سلامة الحزم عند التثبيت في المسار المخصص له. زمن Engine وزمن قائمة النتائج يُعرضان في حالة الفحص.
- ثمة 41 ملف لغة من SDIO الأصلي؛ الواجهة المخصصة مترجمة جزئيًا فقط في اللغات الأخرى، لذا لا نعد بتغطية كاملة لـ41 لغة.
- **لا توجد ميزة تثبيت حقيقية مفعّلة في إصدار matching-only**. هذا الإصدار مخصص للفحص والمطابقة فقط.
- الاختبارات البنيوية على Linux لا تعادل اختبار تشغيل Qt 5.6 / MinGW 4.9 على Windows، لذلك يرجى بناء البرنامج مجددًا على Windows ثم اختبار السرعة ومطابقة النتائج.

## خطوات التنصيب فوق المشروع
1. فك R16 Patch في المجلد الموجود عندك `SDIO_Modern_Origin_Integration_Candidate` واستبدل الملفات.
2. لا تحذف vendor أو .source-cache أو build-origin-matching.
3. شغّل `01_BUILD_ENGINE_MATCHING.cmd`.
4. تأكد من إنشاء `dist-origin-matching\SnappyDriverInstaller-TS.exe`، وأن الهيدر يظهر `UI by Tarek Sadek · R16`.
5. شغّل `04_PACKAGE_OFFLINE_PORTABLE.cmd` لإنشاء نسخة بجانب `drivers` و`indexes` بداخل `D:\Prog\Snappy Driver Installer 1.26.0 (R2600)`.
6. افتح **الملف الجديد** SnappyDriverInstaller-TS.exe، لا ملف R15 القديم، ولا SDI_R2600.exe الأصلي.
7. لو كان الفحص بطيئًا، قارِن زمن Origin index / PnP scan في `logs/native-events.jsonl` وزمن `list preparation` المعروض أسفل العنوان.
