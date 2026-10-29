# Phase 9 - Qt 5.6.3 build hotfix

## سبب الخطأ
أظهر سجل البناء الحقيقي على Windows 11 أن `QTemporaryDir::filePath` غير متاحة في Qt 5.6.3.
لذلك فشل تجميع `src/sdio_readonly_probe.cpp` عند السطرين 81 و82.

## الإصلاح
تم استبدال استخدام `m_workDir->filePath` بالدالة `QDir(m_workDir->path()).filePath` المتوافقة مع Qt 5.6.3.
لم يتم تغيير سكربت الفحص، ولا تشغيل SDIO، ولا تفعيل تثبيت التعريفات.

## التشغيل
1. فك هذه الحزمة **في مجلد جديد** باسم `SDIO_Modern_Qt_Phase9_FIXED`.
2. شغّل `02_RUN_REAL_SCAN.cmd` للفحص الحقيقي للقراءة فقط، أو `01_BUILD_AND_RUN_DEMO.cmd` لمعاينة الواجهة.
3. لو ظهر خطأ آخر، أرسل **أول سطر فيه `error:`** ومعه بعض الأسطر قبله وبعده من `build-win11-preview\build-win11.log`.

يمكنك بدلًا من الحزمة الجديدة تعديل السطرين في مشروعك الحالي يدويًا:
```cpp
const QString scriptPath=QDir(m_workDir->path()).filePath(QStringLiteral("readonly.txt"));
m_exportPath=QDir(m_workDir->path()).filePath(QStringLiteral("installed.txt"));
```

**حالة التحقق:** اجتاز اختبارًا بنيويًا جديدًا بجانب اختبارات السورس السابقة. لم تُجر عملية Windows Qt/MinGW Build فعلية في البيئة الحالية؛ لا نعتبر النجاح مضمونًا إلا عند تشغيل السكربت عندك.
