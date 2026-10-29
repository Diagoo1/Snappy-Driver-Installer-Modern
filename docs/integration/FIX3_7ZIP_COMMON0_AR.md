# SDIO Modern — Fix 3: Missing official 7-Zip header

## سبب خطأ البناء

النسخة المضمنة من مكتبة 7-Zip في المستودع الذي نعتمد عليه تحمل الإصدار 26.01، لكنها **لا تحتوي** على `external/SevenZ/build/CPP/Common/Common0.h` رغم أن `CPP/Common/Common.h` يستدعيه مباشرة. لذلك المشكلة ليست مجرد `INCLUDEPATH` ناقص؛ يجب إكمال الملف المفقود.

## التصحيح

- `prepare_sources.ps1` يسترجع **هذا الهيدر فقط** من مستودع 7-Zip الرسمي `ip7z/7zip` عند commit محدد، داخل مجلد الكاش؛ لا يحمّل executable.
- يتحقق من رقم إصدار 7-Zip في `C/7zVersion.h` أولًا، ثم يتحقق من Git blob SHA1 للملف `5781a957b105c6a8e397561fbcdf2e11d76183ee` قبل استخدامه.
- ينسخ الهيدر إلى `vendor/origin-patched` فقط. نسختك `vendor/origin-upstream` والنسخة الرسمية Offline لا يتم تعديلهما.
- `build_native_engine.ps1` يفحص وجود الهيدر وبصمته قبل بدء 7-Zip compilation.
- يحتفظ بالكاش المُنزَّل سابقًا من Origin وBoost وWebP. الجديد ملف مصدر صغير فقط (~10KB).

مرجع الهيدر: `https://github.com/ip7z/7zip/blob/26.01/CPP/Common/Common0.h`

## تجربة الإصلاح

1. فك ملف Hotfix داخل **جذر** المشروع الحالي، واستبدل ملفات `tools/integration`.
2. أعد تشغيل `01_BUILD_ENGINE_MATCHING.cmd`.
3. لو ظهر خطأ جديد، أرسل **أول** سطر `fatal error:` أو `error:` مع 5 أسطر قبله وبعده، من `build-origin-matching/build-native.log`.
4. لا تشغل ملف `02_BUILD_INSTALLATION_TEST.cmd` قبل اجتياز بناء وفحص المطابقة، ولا تختبر التثبيت الحقيقي على جهازك الأساسي.

## حدود التحقق

فحصنا تطابق مسار الملف وقيمة Git Blob المعلنة في commit رسمي، وشغلنا اختبارات السورس. **لم تتم تجربة بناء MinGW/Qt على Windows في هذه البيئة**؛ قد تظهر مشكلات تجميع لاحقة في المحرك.
