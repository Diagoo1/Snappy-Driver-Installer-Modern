# Fix 4 — توافق Boost.Filesystem مع Qt 5.6.3 / MinGW 4.9.2

## سبب التوقف

تأكدنا من لوج Windows الذي أرسله المستخدم أن مكتبة 7-Zip اكتمل بناؤها، ثم بدأ Boost.Filesystem 1.79، وفشل الملف `libs/filesystem/src/atomic_tools.hpp` عند `atomic_ns::atomic_ref<T>` لأن `src/atomic_ref.hpp` يختار افتراضيًا مساحة الأسماء `std`، و`std::atomic_ref` ليست متاحة في GCC 4.9.2 مع C++11.

## الإصلاح

- تعديل مسار **بناء Boost.Filesystem فقط** داخل `tools/integration/build_native_engine.ps1`: إضافة `BOOST_FILESYSTEM_NO_CXX20_ATOMIC_REF` إلى `DEFINES` الخاصة بمكتبة `sdio_boost_fs`.
- Boost 1.79 يدعم هذا الخيار رسميًا في `libs/filesystem/src/atomic_ref.hpp`: يستبدل `namespace atomic_ns = std;` بـ `namespace atomic_ns = boost;` ويستخدم `boost/atomic/atomic_ref.hpp` مع الحفاظ على التزامن الصحيح.
- فحص مسبق في السكربت للتأكد من وجود الهيدر ومسار Boost.Atomic، والتوقف برسالة واضحة لو نسخة Boost تغيّرت، بدل تجميع غير موثوق.
- **لا تستخدم** `BOOST_FILESYSTEM_SINGLE_THREADED` لأنه يلغي العمليات الذرية، ولا تعدل ملفات vendor، ولا ترقي المترجم بمعزل عن Qt.
- التعديل لا يؤثر على `SDIO Origin Matcher` أو Installer، ولا يفعّل التثبيت.

## طريقة التثبيت على مجلدك الحالي

1. فك ملف `SDIO_Native_Boost_Atomic_Fix4_Hotfix.zip` في جذر المجلد الحالي `SDIO_Modern_Origin_Integration_Candidate` مع السماح باستبدال الملفات بنفس المسارات.
2. احتفظ بمجلدات `.source-cache` و`vendor` و`build-origin-matching`، فهي تحتوي على ملفات تحمّلت واتجمّعت بنجاح.
3. شغّل `01_BUILD_ENGINE_MATCHING.cmd`، فقط.
4. إذا ظهر خطأ جديد، أرسل أول `error:` أو `fatal error:` مع بضعة أسطر حوله.

## ملاحظات التحقق

طابقنا خيار الـ macro مع نسخة `boostorg/filesystem` ذات العلامة `boost-1.79.0` الرسمية، وأضفنا اختبارًا بنيويًا للكود. لم نجرِ تجميع Windows Qt في هذه البيئة. ما زال المشروع Integration Candidate، وليس إصدار تعريفات نهائيًا.

المصدر: https://github.com/boostorg/filesystem/blob/boost-1.79.0/src/atomic_ref.hpp
