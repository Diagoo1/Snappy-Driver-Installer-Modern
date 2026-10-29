# مسار EXE واحد وتقليل الحجم

## الوضع الحالي
المكتبات الأساسية للمحرك تُبنى Static بواسطة سكربت الدمج. أما Qt المثبتة عندك من الحزمة الرسمية فهي Dynamic عادةً. لذلك نجاح بناء المحرك لا يجعل التطبيق ملفًا واحدًا تلقائيًا.

`03_BUILD_SINGLE_FILE.cmd` يتوقف إذا كانت Qt غير ثابتة. هذا مقصود، ولا يتم ضغط DLLs في حاوية وإخفاؤها عنك.

## ما يحتاجه البناء الثابت
يجب بناء Qt 5.6.3 من السورس بنفس MinGW x86 المستخدم للمحرك:
QtBase + QtSvg، Release، بدون WebEngine أو Multimedia أو OpenGL/ANGLE غير المستخدم.
يُضمَّن plugin منصة Windows ومحرك SVG والموارد.
تُفحص اعتماديات PE بعد الربط، ولا يُسمح بتسمية الناتج «ملفًا واحدًا» إذا ما زال يحتاج Qt أو GCC DLLs خارجية.

هذه خطوات إعداد للمطور وليست خطوات منفذة في بيئة هذه الحزمة:
```bat
rem Use a short path with no spaces and the matching MinGW compiler.
rem Review Qt's license before confirming it.
cd C:\QtStaticSrc\qtbase
configure.bat -static -release -opensource -confirm-license ^
  -platform win32-g++ -prefix C:\QtStatic563 ^
  -no-opengl -no-icu -no-openssl ^
  -nomake examples -nomake tests
mingw32-make -j2
mingw32-make install

cd C:\QtStaticSrc\qtsvg
C:\QtStatic563\bin\qmake.exe qtsvg.pro
mingw32-make -j2
mingw32-make install

set QT_QMAKE=C:\QtStatic563\bin\qmake.exe
set QT_MINGW_BIN=C:\Qt\Qt5.6.3\Tools\mingw492_32\bin
cd C:\SDIO-Modern
03_BUILD_SINGLE_FILE.cmd
```
خيارات configure وتوافق أدوات البناء القديمة تحتاج التحقق على جهاز البناء. دعم XP لا يثبت بمجرد وضع رقم subsystem في PE؛ يجب فحص API imports وتشغيله على XP فعليًا.

لبناء Static مع التثبيت التجريبي بعد تجهيز Qt:
```powershell
powershell.exe -NoProfile -ExecutionPolicy Bypass -File tools\integration\build_native_engine.ps1 -SingleFile -EnableExperimentalInstall -NoLaunch
```

## الحجم
لم يتم إنتاج EXE نهائي هنا، لذلك لا توجد قيمة حجم مقاسة أو ضمان 15–35 MB.
BUILD_MANIFEST الناتج بعد البناء يسجل حجم EXE وبصمته، ويُحفظ تقرير imports.
بناء Release، وحذف الوحدات غير المستخدمة، وfunction sections/GC، وعدم إضافة محرك متصفح هي وسائل تقليل الحجم المستخدمة.
لا يُستخدم UPX لإخفاء الحجم، ولا يُطلب تعطيل الحماية.

حزم التعريفات والفهارس خارج EXE. عند الحاجة لتثبيت x64 من واجهة x86 قد يُستخرج المساعد المدمج إلى مجلد مؤقت ويعمل كعملية قصيرة.

## الترخيص
لا يكتمل تسليم توزيع عام بمجرد نسخ EXE. يجب حفظ إشعارات تراخيص Qt/Boost/WebP/7-Zip/SDIO وتوفير السورس المطابق ومتطلبات التوزيع المناسبة. لا تعتبر سكربت تنزيل المصادر وحده إثباتًا كافيًا للوفاء بكل متطلبات توزيع نسخة ثابتة.
