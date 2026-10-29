# Fix 6 — إصلاح أخطاء الربط الأخيرة على Qt 5.6.3 / MinGW 4.9.2

## نتيجة لوج Windows بتاريخ 2026-10-28

انتهى بناء 7-Zip وBoost وWebP وSDIO C++ وواجهة Qt من دون أخطاء تجميع توقف العملية؛ توقفت عند رابط `SDIO-Modern-Qt.exe` بسبب:

```
undefined reference to `CLSID_WbemLocator'
undefined reference to `XZ_SIG'
undefined reference to `Xz_Free'
```

## 1) WMI — CLSID_WbemLocator

المصدر الأصلي `source/baseboard.cpp` يستدعي `CoCreateInstance(CLSID_WbemLocator, ...)`. مكتبة `wbemuuid.a` المتاحة مع MinGW 4.9.2 لا توفر هذا الرمز، رغم وجود `-lwbemuuid` بالفعل في خيارات الرابط. لذلك تضيف `patches.json` معرف GUID موثقًا محليًا في نسخة البناء `vendor/origin-patched/source/baseboard.cpp` ثم تستخدمه في الاستدعاء، مع الإبقاء على `IID_IWbemLocator` وباقي COM الأصلي. لم نحذف استعلام اللوحة الأم أو نقلّص وظائف الفحص. قيمة GUID: `{4590F811-1D3A-11D0-891F-00AA004B2E24}`.

## 2) 7-Zip — ملف Xz.c مفقود من قائمة البناء

`project/7zip/lib.cbp` لا يضم الملف الموجود `external/SevenZ/build/C/Xz.c`، الذي يعرّف `XZ_SIG` و`Xz_Free`. الآن `build_native_engine.ps1` يتحقق من بصمة هذا الملف المضمّن في نسخة SDIO الأصلية، ويرفض التكرار ثم يضيفه في قائمة بناء مكتبة `sdio_7z` قبل الربط. لا يحتاج تنزيل جديد.

## الدقة والأمان

- ثبّت `source-lock.json` بصمتي Git Blob للملفين الأصليين قبل تطبيق التعديلات، ويوقف التحضير العملية إذا اختلفت الملفات.
- يبقى مجلد `vendor/origin-upstream` سليمًا دون تعديل؛ التغيير الخاص بـ WMI يُطبق على `vendor/origin-patched` فقط، مع منع تعديل خوارزميتي `matcher.cpp` و`indexing.cpp`.
- **هذا إصلاح Linker مبني على اللوج والسورس، وليس شهادة بأن EXE اشتغل على Windows.** لا يفعّل تثبيت التعريفات الحقيقي أو يزيل طلب الموافقة.

## تطبيق الإصلاح الصغير

فك ZIP فوق مجلد `SDIO_Modern_Origin_Integration_Candidate` وافق على استبدال الملفات بنفس المسارات فقط. لا تمسح `.source-cache` أو `vendor` أو `build-origin-matching`. شغّل `01_BUILD_ENGINE_MATCHING.cmd`. إذا اكتمل، اختبر الفحص والمطابقة مع حزم SDIO Offline الرسمية قبل أي اختبار تثبيت.

## مراجع كود المصدر

- https://github.com/IDisposable/sdio-go/blob/25468bdb37d46a72ab4e8bf037b4e1a28ffec70e/SDIO-baseline/source/baseboard.cpp
- https://github.com/IDisposable/sdio-go/blob/25468bdb37d46a72ab4e8bf037b4e1a28ffec70e/SDIO-baseline/external/SevenZ/build/C/Xz.c
- https://github.com/IDisposable/sdio-go/blob/25468bdb37d46a72ab4e8bf037b4e1a28ffec70e/SDIO-baseline/project/7zip/lib.cbp
- https://github.com/KDE/kdewin/blob/master/wbem/wbemuuid.cpp
