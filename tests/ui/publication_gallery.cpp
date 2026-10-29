#include <QApplication>
#include <QDir>
#include <QFile>
#include <QJsonDocument>
#include <QJsonArray>
#include <QJsonObject>
#include <QSettings>
#include <QLabel>
#include <QLineEdit>
#include <QComboBox>
#include <QTreeWidget>
#include <QTabWidget>
#include <QStackedWidget>
#include <QScrollArea>
#include <QScrollBar>
#include <QTimer>
#include <QTest>
#include <QProgressBar>
#include <QContextMenuEvent>
#include <QMenu>
#include "main_window.h"
#include "driver_catalog.h"
#include "driver_workspace.h"
#include "driver_details_dialog.h"
#include "installation_workspace.h"
#include "tools_drawer.h"
#include "scan_overlay.h"
#include "online_dialog.h"
#include "ui_elements.h"
#include "ui_language.h"
#include "app_settings.h"
#include "theme.h"
#include "support_report.h"
#include <QPlainTextEdit>
#include <QPushButton>
#include "../online/fixture.h"
#include <stdexcept>
#include <cstdio>

int renderPublicationGallery(QApplication &app){
    const int flag=app.arguments().indexOf(QStringLiteral("--publication-gallery"));
    const QString output=flag>=0?app.arguments().value(flag+1):QString();
    if(output.isEmpty() || !QDir().mkpath(output))return 2;
    QSettings *settings=createAppSettings(app.applicationDirPath(),0,false);settings->clear();settings->sync();
    MainWindow window(true);window.resize(1280,860);window.show();QTest::qWait(700);
    QComboBox *language=window.findChild<QComboBox*>(QStringLiteral("premiumCombo"));
    DriverCatalog *catalog=window.findChild<DriverCatalog*>();DriverWorkspace *board=window.findChild<DriverWorkspace*>();QStackedWidget *pages=window.findChild<QStackedWidget*>();
    QVector<DeviceInfo> devices;const QStringList names=QStringList()<<QStringLiteral("Demo graphics adapter")<<QStringLiteral("Demo audio controller")<<QStringLiteral("Demo Ethernet adapter")<<QStringLiteral("Demo Wi-Fi adapter")<<QStringLiteral("Demo USB controller")<<QStringLiteral("Demo Bluetooth adapter")<<QStringLiteral("Demo storage controller");
    const QStringList classes=QStringList()<<QStringLiteral("Display")<<QStringLiteral("Media")<<QStringLiteral("Net")<<QStringLiteral("Net")<<QStringLiteral("USB")<<QStringLiteral("Bluetooth")<<QStringLiteral("SCSIAdapter");
    for(int i=0;i<names.size();++i){DeviceInfo d;d.name=names[i];d.manufacturer=QStringLiteral("Demo hardware manufacturer");d.deviceClass=classes[i];d.hardwareId=QStringLiteral("DEMO\\DEVICE_%1").arg(i+1);d.instanceId=QStringLiteral("DEMO\\DEVICE_%1\\PUBLIC-GALLERY").arg(i+1);d.driverVersion=QStringLiteral("1.0.0.%1").arg(i+1);devices.append(d);}catalog->replace(devices,true);catalog->setSelected(0,true);catalog->setSelected(2,true);
    QJsonArray manifest;
    for(int ar=0;ar<2;++ar)for(int dark=0;dark<2;++dark){
        QObject captureScope; // Cancel any outstanding capture callbacks before leaving this variant.
        language->setCurrentIndex(language->findData(ar?QStringLiteral("arabic"):QStringLiteral("english")));QMetaObject::invokeMethod(&window,"updateDark",Q_ARG(bool,dark==1));QMetaObject::invokeMethod(&window,"updateReduceMotion",Q_ARG(bool,true));UiLanguage::refresh();QTest::qWait(120);
        const QString variant=QStringLiteral("%1-%2").arg(ar?QStringLiteral("ar"):QStringLiteral("en"),dark?QStringLiteral("dark"):QStringLiteral("light"));QDir().mkpath(QDir(output).filePath(variant));
        auto sanitize=[&](){
            QMap<QString,QString> facts;facts[QStringLiteral("os")]=QStringLiteral("Windows 11 Pro");facts[QStringLiteral("build")]=QStringLiteral("24H2");facts[QStringLiteral("architecture")]=QStringLiteral("64-bit (x64)");facts[QStringLiteral("computer")]=QStringLiteral("DEMO-PC");facts[QStringLiteral("maker")]=QStringLiteral("Demo manufacturer");facts[QStringLiteral("model")]=QStringLiteral("Demo workstation");facts[QStringLiteral("cpu")]=QStringLiteral("Demo 8-core processor");facts[QStringLiteral("logical")]=QStringLiteral("16");facts[QStringLiteral("clock")]=QStringLiteral("3.2 GHz");facts[QStringLiteral("memory")]=QStringLiteral("16 GiB");facts[QStringLiteral("memoryfree")]=QStringLiteral("10 GiB");facts[QStringLiteral("memoryload")]=QStringLiteral("38%");
            for(QLabel *label:window.findChildren<QLabel*>()){
                const QString key=label->property("diagnosticKey").toString();if(!key.isEmpty()){const QString value=facts.value(key,ar?QString::fromUtf8("بيانات توضيحية"):QStringLiteral("Demo data"));label->setText(value);label->setProperty("rawDiagnosticValue",value);label->setToolTip(value);}
                else if(label->text().contains(QStringLiteral("C:/Users/"),Qt::CaseInsensitive) || label->text().contains(QStringLiteral("C:\\Users\\"),Qt::CaseInsensitive) || label->text().contains(QStringLiteral("build-review")))label->setText(QStringLiteral("./drivers · ./indexes"));
            }
            for(QProgressBar *bar:window.findChildren<QProgressBar*>(QStringLiteral("diagnosticUsage")))bar->setValue(38);
        };
        auto shot=[&](QWidget *widget,const QString &name,bool installation=false){UiLanguage::refresh();sanitize();QTest::qWait(35);const QString relative=variant+QLatin1Char('/')+name+QStringLiteral(".png");if(!widget->grab().save(QDir(output).filePath(relative)))throw std::runtime_error("Gallery image could not be saved.");QJsonObject item;item.insert(QStringLiteral("file"),relative);item.insert(QStringLiteral("locale"),ar?QStringLiteral("ar"):QStringLiteral("en"));item.insert(QStringLiteral("theme"),dark?QStringLiteral("dark"):QStringLiteral("light"));item.insert(QStringLiteral("sampleData"),true);item.insert(QStringLiteral("installationSimulation"),installation);manifest.append(item);};
        auto page=[&](int index){QMetaObject::invokeMethod(&window,"navigate",Q_ARG(int,index));QTest::qWait(120);sanitize();};
        page(0);shot(&window,QStringLiteral("01-drivers"));QTreeWidget *tree=board->findChild<QTreeWidget*>(QStringLiteral("driverTree"));if(tree && tree->topLevelItemCount()){tree->expandItem(tree->topLevelItem(0));QTest::qWait(50);}shot(&window,QStringLiteral("02-driver-versions"));if(tree)tree->collapseAll();
        page(1);shot(&window,QStringLiteral("03-diagnostics"));
        page(2);shot(&window,QStringLiteral("04-driver-packages"));
        page(3);shot(&window,QStringLiteral("05-settings"));
        for(QScrollArea *scroll:pages->widget(3)->findChildren<QScrollArea*>())scroll->verticalScrollBar()->setValue(scroll->verticalScrollBar()->maximum());shot(&window,QStringLiteral("06-settings-advanced"));for(QScrollArea *scroll:pages->widget(3)->findChildren<QScrollArea*>())scroll->verticalScrollBar()->setValue(0);
        page(5);shot(&window,QStringLiteral("07-about"));
        QTimer::singleShot(90,&captureScope,[&](){for(QWidget *widget:QApplication::topLevelWidgets())if(widget->objectName()==QStringLiteral("bugReportDialog")){QPlainTextEdit *preview=widget->findChild<QPlainTextEdit*>(QStringLiteral("bugReportPreview"));if(preview)preview->setPlainText(BugReport::environment(QStringLiteral("Windows 11 (demo)"),QStringLiteral("x86_64"),ar?QStringLiteral("arabic"):QStringLiteral("english"),dark==1));shot(widget,QStringLiteral("18-report-bug"));widget->close();return;}});BugReport::show(&window);
        page(0);ToolsDrawer *tools=window.findChild<ToolsDrawer*>();tools->setMotionEnabled(false);tools->openDrawer();shot(&window,QStringLiteral("08-tools"));tools->closeDrawer();
        for(int elevated=0;elevated<2;++elevated){QTimer::singleShot(80,&captureScope,[&,elevated](){for(QWidget *widget:QApplication::topLevelWidgets())if(widget->objectName()==QStringLiteral("installationConsent")){shot(widget,elevated?QStringLiteral("19-installation-review"):QStringLiteral("20-administrator-review"));widget->close();return;}});confirmDriverInstallation(&window,QStringList()<<QStringLiteral("Demo graphics adapter · 2.0.0"),true,false,false,elevated==1,ar==1);}
        {DriverDetailsDialog details(catalog->entry(0),true,ar,&window);details.show();QTest::qWait(60);QTabWidget *tabs=details.findChild<QTabWidget*>();shot(&details,QStringLiteral("09-comparison"));tabs->setCurrentIndex(1);shot(&details,QStringLiteral("10-device"));QLabel *value=details.findChild<QLabel*>(QStringLiteral("hardwareIdentityValue"));value->setSelection(0,value->text().size());QTimer::singleShot(70,&details,[&](){QMenu *menu=qobject_cast<QMenu*>(app.activePopupWidget());if(menu){menu->setActiveAction(menu->actions().first());shot(menu,QStringLiteral("17-text-menu"));menu->close();}});QContextMenuEvent context(QContextMenuEvent::Mouse,QPoint(12,12),value->mapToGlobal(QPoint(12,12)));QApplication::sendEvent(value,&context);tabs->setCurrentIndex(2);shot(&details,QStringLiteral("11-engine-details"));details.close();}
        {OnlineDialog online(QStringLiteral("./drivers"),QStringLiteral("./indexes"),QStringList(),ar,&window);online.setProperty("darkMode",dark==1);online.setStyleSheet(Theme::styleSheet(dark==1));Fixture fixture;online.setCatalog(OnlinePackages::parse(fixture.torrent));online.show();QTest::qWait(60);shot(&online,QStringLiteral("12-download-catalog"));online.close();}
        page(0);QMetaObject::invokeMethod(&window,"beginInstallation");InstallationWorkspace *installer=window.findChild<InstallationWorkspace*>();for(QTimer *timer:installer->findChildren<QTimer*>())timer->stop();for(int i=0;i<45;++i)QMetaObject::invokeMethod(installer,"step");shot(&window,QStringLiteral("13-installation-simulation"),true);for(int i=0;i<55;++i)QMetaObject::invokeMethod(installer,"step");shot(&window,QStringLiteral("14-installation-complete-simulation"),true);installer->stopDemo();page(0);
        ScanOverlay *overlay=static_cast<ScanOverlay*>(window.findChild<QWidget*>(QStringLiteral("nativeScanOverlay")));overlay->start(ar?QString::fromUtf8("معاينة توضيحية للفحص"):QStringLiteral("Demonstration of scan progress"),true);shot(&window,QStringLiteral("15-scan-preview"));overlay->finish();
        page(4);installer->beginNative(QStringList()<<QStringLiteral("Demo graphics adapter"),QStringList()<<QStringLiteral("DEMO-ID"),true);sdiomodern::InstallReport blocked;blocked.restoreFailed=true;blocked.restoreError=1058;blocked.error=ar?L"حماية النظام معطّلة (Windows 1058). لم يبدأ تثبيت أي تعريف. مثال توضيحي.":L"System Protection is disabled (Windows 1058). No driver installation started. Illustrative example.";installer->finishNative(blocked);shot(&window,QStringLiteral("21-restore-error-example"),true);page(0);
        {Splash splash;splash.begin();QTest::qWait(250);shot(&splash,QStringLiteral("16-startup"));splash.close();}
    }
    QJsonObject result;result.insert(QStringLiteral("schema"),1);result.insert(QStringLiteral("realInstallationPerformed"),false);result.insert(QStringLiteral("privateMachineDataIncluded"),false);result.insert(QStringLiteral("images"),manifest);QFile file(QDir(output).filePath(QStringLiteral("manifest.json")));if(!file.open(QIODevice::WriteOnly))return 3;file.write(QJsonDocument(result).toJson());settings->clear();settings->sync();delete settings;std::printf("Publication gallery: %d sanitized images; installation screenshots are simulations.\n",manifest.size());return 0;
}
