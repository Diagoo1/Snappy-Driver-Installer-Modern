# Fix 7 — استكمال وحدات SHA-256 وCRC64 الأصلية من 7-Zip

## المشاهدة من لوج Windows بتاريخ 2026-10-28

بعد إصلاح Fix 6 وصل البناء إلى الربط النهائي لتطبيق `SDIO-Modern-Qt.exe`، لكن توقف عند:

```text
undefined reference to `Sha256_Init'
undefined reference to `Crc64Update'
```

**السبب:** سورس 7-Zip الأصلي يضم `C/Xz.c` الذي استكملناه في Fix 6، لكنه يعتمد بدوره على وحدات SHA-256 وCRC64. ملف مشروع Code::Blocks القديم `project/7zip/lib.cbp` لا يسرد هذه الوحدات. لذلك أضفنا **سورس 7-Zip الأصلي بلا تعديل** إلى قائمة الربط الداخلية، مع التحقق من بصمة Git Blob لكل ملف:

| الوحدة | الوظيفة | Git blob |
|---|---|---|
| `Sha256.c` | `Sha256_Init`, `Sha256_Update`, `Sha256_Final` | `b29f06bcd1ec75b18ac7c4c593d8cb0fc0c9403d` |
| `Sha256Opt.c` | تنفيذ SHA-256 المحسّن المعتمد على قدرة المعالج | `291e71c120b85aabb2789fe8121b680487bfa687` |
| `XzCrc64.c` | `Crc64Update`, `Crc64GenerateTable` | `a0023a6fa2c8b459f3d931f399a31f3f20225397` |
| `XzCrc64Opt.c` | تنفيذ CRC64 المحسّن | `08a419ca526349c70740853b525b1c8e64f88e72` |

مهم: لا نلغي CRC أو SHA-256 ولا نستخدم دوال وهمية، لأن سلامة فحص بيانات الأرشيف مهمة. ملحوظة: وحدات SHA-256 المحسّنة مع MinGW 4.9.2 ما زالت بحاجة إلى اختبار البناء على Windows؛ اختبارات Linux لا تثبت توافقها.

## تحسين البناء المتكرر

تم أيضًا تحديث `Make-Library` بحيث لا يغيّر تواريخ ملفات wrapper وملفات qmake `.pro` ما لم يتغير محتواها؛ يتيح هذا الاستفادة من نتائج البناء السابقة **للمكتبات التي لم تتغير مصادرها**. لكن تجهيز نسخة `origin-patched` من المصدر قد يعيد بناء أجزاء من المحرك أو 7-Zip؛ لم نعدّل شروط التحقق من سلامة المصدر للتسريع.

## تركيب الإصلاح الصغير

1. افتح مجلد `SDIO_Modern_Origin_Integration_Candidate` الحالي.
2. فك ملف `SDIO_Native_SHA256_CRC64_Fix7_Hotfix.zip` في جذر نفس المجلد، ووافق على الاستبدال.
3. لا تحذف `vendor` أو `.source-cache` أو `build-origin-matching`.
4. شغّل **`01_BUILD_ENGINE_MATCHING.cmd` فقط**.

**الحدود:** لا تتضمن هذه الحزمة EXE مبنيًا أو اختبار تشغيل Windows أو اختبار تكافؤ المطابقة أو تثبيت التعريفات، ولا تفعّل التثبيت في نسخة المطابقة الافتراضية. إن فشل البناء مرة أخرى، استخرج أول `undefined reference` أو `error:` من اللوج.

## المصدر المرجعي

- https://github.com/IDisposable/sdio-go/tree/25468bdb37d46a72ab4e8bf037b4e1a28ffec70e/SDIO-baseline/external/SevenZ/build/C
- https://github.com/IDisposable/sdio-go/blob/25468bdb37d46a72ab4e8bf037b4e1a28ffec70e/SDIO-baseline/project/7zip/lib.cbp
