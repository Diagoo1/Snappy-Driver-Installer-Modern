# بناء نسخة Windows XP → 11 بواجهة Qt واحدة

**الهدف**: Win32 / x86، Qt 5.6.3 Widgets، مع build ثابت (Static) وملف تشغيل مستقل بقدر الإمكان.

## نسخة التطوير الأولى (تعمل بوجود Qt DLLs)

يمكن فتح `SDIO_Modern_Qt.pro` في Qt Creator المتوافق مع Qt 5.6.3 وتجميعها باستخدام `Release` وKit **x86**. هذه النسخة تختبر الواجهة فقط وقد تحتاج Qt DLLs وplugins بجوار EXE؛ لا نسميها Portable EXE واحدة.

## إصدار XP المستقل المستهدف

1. جهّز Visual Studio 2013 (x86) وWindows SDK 7.1A الخاص بالاستهداف القديم. ابدأ موجه أوامر x86 بأدوات تطوير XP المناسبة.
2. أنشئ **بناء Qt 5.6.3 Static x86** من السورس الرسمي مع `-target xp`، باتباع [توثيق Qt 5.6 للـ XP](https://typeerror.org/docs/qt~5.6/windows-issues) وتحقق من توافق Visual Studio والإعدادات. راجع `configure -help` الخاص بنفس نسخة Qt قبل الاعتماد على خيارات البناء.
3. افتح موجّه الأدوات الذي يحتوي على `qmake.exe` الخاص **بنسخة Qt الثابتة** و`nmake.exe`، ثم شغّل `tools\build_windows_xp_x86.bat` من المشروع.
4. افحص DLL imports الفعلية؛ EXE لازم لا يستورد `Qt5Core.dll` أو `Qt5Gui.dll` أو `Qt5Widgets.dll` إذا كان هدفنا ملفًا واحدًا.
5. جرّب الملف على **Windows XP SP3 x86** بدون Qt مثبت، وعلى Windows 7 و10 و11، وداخل نظام لا يحتوي على تعريف كارت شاشة OEM.

مثال لتجميع Qt نفسه (إرشادي فقط؛ حسب إعداد SDK/Compiler عندك):

```bat
configure -opensource -confirm-license -release -static -target xp -nomake tests -nomake examples
nmake
```

**مهم**: الإصدار 5.6.3 قديم وغير مدعوم أمنيًا. Qt 5.6 يذكر XP كـ deployment only وليس بيئة CI حديثة. هذا المشروع لا يدّعي أن اختبار XP وWindows 11 قد نجح فعليًا. تشغيل تطبيق Win32 x86 على Windows 11 x64 مرجح، لكنه يحتاج اختبارًا.

## ملاحظات خاصة بتطبيق تعريفات

- استخدم **Qt Widgets** وQPainter software raster دون Qt Quick/WebEngine/OpenGL الإجباري؛ أجهزة ما قبل تثبيت التعريفات قد لا توفر تسريعًا رسوميًا جيدًا.
- لا يمكن إعطاء صلاحيات التثبيت لأي إصدار نهائي دون مراجعة Installer وUAC، XP مختلف تمامًا عن Windows 7–11.
- النسخة الحالية تستخدم `SetupAPI` و`CfgMgr32` قراءة فقط، وعند اختيار --demo لا تستعلم عن أجهزة Windows.
- المعمارية x86 موحدة لسهولة دعم XP، لكن حدود الذاكرة تحتاج اختبارًا مع فهارس حزم التعريفات الكبيرة.
- ملف `SDIO-Modern.ini` يُحفظ بجانب EXE؛ احفظه في مجلد قابل للكتابة لتحقيق Portable settings. 

## الترخيص

SDIO Origin خاضع GPLv3؛ استخدم Qt بمسار ترخيص متوافق (مثل GPLv3 عند توافره) وأرفق السورس وإشعارات التراخيص عند التوزيع. بناء Qt بشكل Static يتطلب مراجعة حقوق إعادة الربط والتوزيع حسب ترخيص Qt المختار.
