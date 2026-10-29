#include <QApplication>
#include <cstdio>
#include <QCoreApplication>
#include <QFont>
#include <QTimer>
#include <QTextCodec>
#include <QDesktopWidget>
#include <QSettings>
#include <QDir>
#include "main_window.h"
#include "ui_font.h"
#include "ui_scale.h"
#include "legacy_script.h"
#include "ui_language.h"
#include "ui_elements.h"
#include "device_types.h"
#include "scan_report.h"
#include "online_packages.h"
#include "automation_plan.h"
#include "single_instance.h"
#include "appearance.h"
#include "app_settings.h"
#include "app_update.h"
#include <QScopedPointer>
#include <cstring>
#ifdef Q_OS_WIN
#include <windows.h>
#include <objbase.h>
struct GuiComScope {HRESULT result;GuiComScope():result(CoInitializeEx(0,COINIT_APARTMENTTHREADED)){}~GuiComScope(){if(SUCCEEDED(result))CoUninitialize();}};
#endif

int main(int argc,char **argv)
{
    for(int i=1;i<argc;++i)if(std::strcmp(argv[i],"--apply-update")==0)return AppUpdate::apply(argc,argv);
    bool report=false,online=false,automation=false;
    for(int i=1;i<argc;++i){
        if(std::strcmp(argv[i],"--version")==0){
            fprintf(stdout,"Snappy Driver Installer | UI by Tarek Sadek\n");return 0;
        }
        if(std::strcmp(argv[i],"--scan-report")==0)report=true;
        if(std::strcmp(argv[i],"--online-report")==0)online=true;
        if(std::strcmp(argv[i],"--automation-plan")==0)automation=true;
    }
    if(!UiScale::restartAfterParent(argc,argv))return 4;
    SingleInstance instance;
    if(report || online || automation || Legacy::requested(argc,argv)){
        if(instance.alreadyRunning() || !instance.available())return 4;
        if(Legacy::requested(argc,argv))return Legacy::run(argc,argv);
        return automation?runAutomationReport(argc,argv):online?OnlinePackages::runReport(argc,argv):runScanReport(argc,argv);
    }
    // Device-independent Qt layout and sharp SVG rasterization for HiDPI Windows.
    QApplication::setAttribute(Qt::AA_UseHighDpiPixmaps);
    QApplication::setAttribute(Qt::AA_EnableHighDpiScaling);
    UiScale::prepareEnvironment();
#ifdef Q_OS_WIN
    GuiComScope guiCom;
#ifdef SDIO_ORIGIN_LINKED
    const std::uint32_t restoreCallbacks=sdiomodern::initializeRestoreCallbacks();
#endif
#endif
    QApplication app(argc,argv);
#if defined(Q_OS_WIN) && defined(SDIO_ORIGIN_LINKED)
    if(app.arguments().contains(QStringLiteral("--gui-security-probe"))){fprintf(stdout,"{\"guiCom\":%lu,\"restoreCallbacks\":%lu,\"installationRequested\":false}\n",static_cast<unsigned long>(guiCom.result),static_cast<unsigned long>(restoreCallbacks));return FAILED(guiCom.result) || restoreCallbacks?1:0;}
#endif
    // Verifiable release identity: scripts can reject stale executables.
    if(app.arguments().contains(QStringLiteral("--version"))) {
        fprintf(stdout,"Snappy Driver Installer | UI by Tarek Sadek\n");
        fflush(stdout);
        return 0;
    }
    app.setApplicationName(QStringLiteral("Snappy Driver Installer"));
    app.setWindowIcon(QIcon(QStringLiteral(":/sdio/assets/tarek-sadek-logo.png")));
    app.setOrganizationName(QStringLiteral("TarekSadek"));
    app.setQuitOnLastWindowClosed(true);
    UiFont::applyLanguage(QStringLiteral("english"));
    installInterfaceMotion();
    if(instance.alreadyRunning() || !instance.available()){
        QScopedPointer<QSettings> settings(createAppSettings(app.applicationDirPath(),0,false));
        const QString language=settings->value(QStringLiteral("language"),QStringLiteral("english")).toString();
        const bool arabic=language==QStringLiteral("arabic") || language==QStringLiteral("ar");
        UiLanguage::setLanguage(arabic?QStringLiteral("arabic"):language);
        instance.activateExisting();
        projectMessage(0,arabic?QString::fromUtf8("البرنامج مفتوح بالفعل"):QStringLiteral("Already running"),
            instance.alreadyRunning()?(arabic?QString::fromUtf8("Snappy Driver Installer مفتوح بالفعل. ارجع للنافذة الحالية لمتابعة العمل؛ لا يمكن تشغيل نسختين في نفس الوقت."):
            QStringLiteral("Snappy Driver Installer is already open. Return to the existing window to continue; a second session cannot be started.")):
            (arabic?QString::fromUtf8("تعذّر إنشاء جلسة تشغيل آمنة. أغلق النسخة الحالية وأعد المحاولة."):QStringLiteral("Cannot reserve an application session. Close the existing copy and try again.")),false,arabic);
        return instance.alreadyRunning()?0:4;
    }
    qRegisterMetaType<QVector<DeviceInfo> >("QVector<DeviceInfo>");
    const QStringList args = app.arguments();
    const bool demo = args.contains(QStringLiteral("--demo"));
    const bool noSplash = args.contains(QStringLiteral("--no-splash"));

    MainWindow window(demo);
    instance.registerWindow(&window);
    // Fit and center on 1024x768 and other small displays; the board remains responsive.
    const QRect usable=QApplication::desktop()->availableGeometry(&window);
    if(!window.property("legacyWindowSize").toBool())window.resize(qMin(1400,qMax(window.minimumWidth(),usable.width()-24)),
                  qMin(840,qMax(window.minimumHeight(),usable.height()-24)));
    window.move(usable.center()-window.rect().center());
    if (noSplash) {
        window.show();
        QTimer::singleShot(0,&window,[&window]() { window.beginScan(); });
    } else {
        Splash *splash = new Splash();
        QObject::connect(splash,&Splash::finished,&window,[&window,splash]() {
            window.show();
            window.beginScan();
            splash->close();
            splash->deleteLater();
        });
        splash->begin();
    }
    return app.exec();
}
