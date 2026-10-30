#include "support_report.h"
#include <QJsonArray>
#include <QDateTime>
#include <QSysInfo>
#include "appearance.h"
#include "ui_language.h"
#include "theme.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QPlainTextEdit>
#include <QDesktopServices>
#include <QUrlQuery>
#include <QTextOption>

QString BugReport::environment(const QString &windows,const QString &architecture,const QString &language,bool dark){
    const QString pattern=UiLanguage::text(QStringLiteral("Version: 0.23.4 (Beta)\nWindows: %1\nArchitecture: %2\nLanguage: %3\nTheme: %4"),QString::fromUtf8("الإصدار: 0.23.4 (بيتا)\nويندوز: %1\nالمعمارية: %2\nاللغة: %3\nالمظهر: %4"));
    auto isolate=[](const QString &value){return QChar(0x202a)+value+QChar(0x202c);};
    const bool ar=UiLanguage::isRtl();const QString languageName=ar && language==QStringLiteral("arabic")?QString::fromUtf8("العربية"):language;
    return pattern.arg(ar?isolate(windows):windows,ar?isolate(architecture):architecture,languageName, dark?UiLanguage::text(QStringLiteral("Dark"),QString::fromUtf8("داكن")):UiLanguage::text(QStringLiteral("Light"),QString::fromUtf8("فاتح")));
}
QUrl BugReport::issueUrl(const QString &environment){
    QUrl url(QStringLiteral("https://github.com/Diagoo1/Snappy-Driver-Installer-Modern/issues/new"));
    QUrlQuery query;query.addQueryItem(QStringLiteral("template"),QStringLiteral("bug_report.yml"));
    query.addQueryItem(QStringLiteral("title"),QStringLiteral("[Bug] "));
    query.addQueryItem(QStringLiteral("environment"),environment.left(1500));url.setQuery(query);return url;
}
void BugReport::show(QWidget *parent){
    const bool ar=UiLanguage::isRtl();
    auto t=[](const char *en,const char *arabic){return UiLanguage::text(QString::fromUtf8(en),QString::fromUtf8(arabic));};
    RoundedDialog dialog(parent);dialog.setObjectName(QStringLiteral("bugReportDialog"));dialog.setWindowTitle(t("Report a bug","الإبلاغ عن مشكلة"));dialog.resize(610,470);dialog.setLayoutDirection(ar?Qt::RightToLeft:Qt::LeftToRight);
    QVBoxLayout *layout=new QVBoxLayout(&dialog);layout->setContentsMargins(0,0,0,18);layout->addWidget(new DialogHeader(dialog.windowTitle(),QStringLiteral("alert"),&dialog,ar));
    QVBoxLayout *body=new QVBoxLayout;body->setContentsMargins(24,12,24,0);body->setSpacing(14);
    QLabel *description=new QLabel(t("Review these details before opening GitHub. Describe the problem and add steps or screenshots there. A GitHub account is required; reports are public.","راجع البيانات قبل فتح GitHub، ثم اشرح المشكلة وأضف خطوات تكرارها أو صورًا هناك. يلزم حساب GitHub، والبلاغات علنية."),&dialog);description->setWordWrap(true);description->setObjectName(QStringLiteral("bodyText"));body->addWidget(description);
    const QString details=environment(QSysInfo::prettyProductName(),QSysInfo::currentCpuArchitecture(),UiLanguage::language(),Theme::isDark(parent));
    QPlainTextEdit *preview=new QPlainTextEdit(details,&dialog);preview->setObjectName(QStringLiteral("bugReportPreview"));preview->setReadOnly(true);preview->setLayoutDirection(ar?Qt::RightToLeft:Qt::LeftToRight);body->addWidget(preview,1);
    QTextOption option=preview->document()->defaultTextOption();option.setAlignment(Qt::AlignAbsolute|(ar?Qt::AlignRight:Qt::AlignLeft));option.setTextDirection(ar?Qt::RightToLeft:Qt::LeftToRight);preview->document()->setDefaultTextOption(option);
    const bool dark=Theme::isDark(parent);preview->setStyleSheet(QStringLiteral("QPlainTextEdit {background:%1;color:%2;border:1px solid %3;border-radius:12px;padding:12px;selection-background-color:%4;}").arg(Theme::color("surface",dark).name(),Theme::color("text",dark).name(),Theme::color("border",dark).name(),Theme::color("elevated",dark).name()));
    QLabel *privacy=new QLabel(t("No report is sent automatically. Hardware IDs, device names, paths and logs are not included. Check any screenshots or files you attach for private information.","لا يُرسل البلاغ تلقائيًا. لا تُرفق معرّفات الأجهزة أو أسماؤها أو المسارات أو السجلات. راجع الصور والملفات التي تضيفها للتأكد من خلوها من بياناتك الخاصة."),&dialog);privacy->setWordWrap(true);privacy->setObjectName(QStringLiteral("subText"));body->addWidget(privacy);
    QHBoxLayout *actions=new QHBoxLayout;actions->addStretch();ActionButton *cancel=new ActionButton(t("Cancel","إلغاء"),&dialog);cancel->setObjectName(QStringLiteral("bugReportCancel"));actions->addWidget(cancel);ActionButton *open=new ActionButton(t("Open GitHub","فتح GitHub"),&dialog);open->setObjectName(QStringLiteral("primaryButton"));actions->addWidget(open);body->addLayout(actions);layout->addLayout(body,1);
    QObject::connect(cancel,&QPushButton::clicked,&dialog,&QDialog::reject);
    QObject::connect(open,&QPushButton::clicked,&dialog,[&](){if(QDesktopServices::openUrl(issueUrl(details)))dialog.accept();else projectMessage(&dialog,t("Cannot open link","تعذّر فتح الرابط"),t("Open the project's Issues page in your browser to report the problem.","افتح صفحة Issues في مستودع المشروع من متصفحك للإبلاغ عن المشكلة."),false,ar);});dialog.exec();
}
QJsonObject supportReport(const DriverCatalog &catalog,const sdiomodern::InstallReport &installation,const QString &lastError){
    QJsonObject root;root.insert(QStringLiteral("schema"),1);root.insert(QStringLiteral("uiVersion"),QStringLiteral("0.23.4"));
    root.insert(QStringLiteral("ignoredHardwareIds"),QJsonArray::fromStringList(catalog.ignoredHardwareIds()));
    root.insert(QStringLiteral("createdUtc"),QDateTime::currentDateTimeUtc().toString(Qt::ISODate));
    root.insert(QStringLiteral("windows"),QSysInfo::prettyProductName());root.insert(QStringLiteral("architecture"),QSysInfo::currentCpuArchitecture());
    root.insert(QStringLiteral("demo"),catalog.demo());root.insert(QStringLiteral("native"),catalog.native());root.insert(QStringLiteral("lastError"),lastError);
    root.insert(QStringLiteral("sourceRevision"),QString::fromStdWString(catalog.nativeSnapshot().sourceRevision));
    root.insert(QStringLiteral("scanValidForInstallation"),catalog.nativeInstallEnabled());
    root.insert(QStringLiteral("automaticUpload"),false);
    QJsonObject updates;updates.insert(QStringLiteral("verifiedHttp"),true);updates.insert(QStringLiteral("automaticInstall"),false);
#ifdef SDIO_TORRENT_LINKED
    updates.insert(QStringLiteral("peerTransportLinked"),true);
#else
    updates.insert(QStringLiteral("peerTransportLinked"),false);
#endif
    root.insert(QStringLiteral("packageUpdates"),updates);
    QJsonArray devices;
    for(const DriverEntry &entry:catalog.entries()){
        QJsonObject device;device.insert(QStringLiteral("name"),entry.device.name);device.insert(QStringLiteral("instanceId"),entry.device.instanceId);
        device.insert(QStringLiteral("hardwareId"),entry.device.hardwareId);device.insert(QStringLiteral("installedVersion"),entry.device.driverVersion);
        device.insert(QStringLiteral("problemCode"),static_cast<int>(entry.device.problemCode));device.insert(QStringLiteral("selected"),entry.checked);
        device.insert(QStringLiteral("chosenCandidate"),entry.chosenCandidate);QJsonArray candidates;
        for(const DriverCandidate &candidate:entry.candidates){
            QJsonObject c;c.insert(QStringLiteral("version"),candidate.version);c.insert(QStringLiteral("date"),candidate.date);
            c.insert(QStringLiteral("inf"),candidate.inf);c.insert(QStringLiteral("package"),candidate.packageSource);
            c.insert(QStringLiteral("hardwareId"),candidate.hardwareId);c.insert(QStringLiteral("originalOrder"),candidate.originalOrder);
            c.insert(QStringLiteral("rawScore"),static_cast<double>(candidate.rawScore));c.insert(QStringLiteral("statusBits"),static_cast<double>(candidate.statusBits));
            c.insert(QStringLiteral("relativeMatchPriorityStars"),candidate.matchStars);c.insert(QStringLiteral("valid"),candidate.valid);c.insert(QStringLiteral("local"),candidate.local);c.insert(QStringLiteral("recommended"),candidate.recommended);
            candidates.append(c);
        }
        device.insert(QStringLiteral("candidates"),candidates);devices.append(device);
    }
    root.insert(QStringLiteral("devices"),devices);QJsonObject result;QJsonArray items;
    for(const sdiomodern::Result &r:installation.items){QJsonObject item;
        item.insert(QStringLiteral("deviceId"),QString::fromStdWString(r.deviceId));item.insert(QStringLiteral("name"),QString::fromStdWString(r.name));
        item.insert(QStringLiteral("outcome"),static_cast<int>(r.outcome));item.insert(QStringLiteral("message"),QString::fromStdWString(r.message));
        item.insert(QStringLiteral("nativeError"),static_cast<double>(r.nativeError));item.insert(QStringLiteral("rebootRequired"),r.rebootRequired);items.append(item);}
    result.insert(QStringLiteral("items"),items);result.insert(QStringLiteral("restoreCreated"),installation.restoreCreated);
    result.insert(QStringLiteral("restoreFailed"),installation.restoreFailed);result.insert(QStringLiteral("restoreError"),static_cast<double>(installation.restoreError));
    result.insert(QStringLiteral("cancelled"),installation.cancelled);result.insert(QStringLiteral("rebootRequired"),installation.rebootRequired);
    result.insert(QStringLiteral("error"),QString::fromStdWString(installation.error));root.insert(QStringLiteral("lastInstallation"),result);return root;
}
