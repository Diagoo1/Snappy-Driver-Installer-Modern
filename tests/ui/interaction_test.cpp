#include <QApplication>
#include <QUrlQuery>
#ifdef Q_OS_WIN
#include <windows.h>
#endif
#include <QSettings>
#include <QScopedPointer>
#include <QPersistentModelIndex>
#include <QTemporaryDir>
#include <QFontInfo>
#include <QLineEdit>
#include <QListWidget>
#include "portable_library.h"
#include "about_page.h"
#include "automation_plan.h"
#include "searchable_language_combo.h"
#include <QDir>
#include <QStackedWidget>
#include <QToolButton>
#include <QTest>
#include <QImage>
#include <QGraphicsOpacityEffect>
#include <QJsonDocument>
#include <QCryptographicHash>
#include <QTimer>
#include <QPushButton>
#include <QFont>
#include <QLabel>
#include <QHeaderView>
#include <QComboBox>
#include <QTabWidget>
#include <QTabBar>
#include <QTreeWidget>
#include <QTreeWidgetItemIterator>
#include <QCheckBox>
#include <QMenu>
#include <QContextMenuEvent>
#include <QScrollBar>
#include "driver_workspace.h"
#include "support_report.h"
#include "offline_paths.h"
#include "theme.h"
#include "ui_language.h"
#include "ui_font.h"
#include "legacy_script.h"
#include "drop_target.h"
#include "ui_scale.h"
#include <QMimeData>
#include <QDragEnterEvent>
#include <QDropEvent>
#include "restart_prompt.h"
#include "app_update.h"
#include "online_dialog.h"
#include "update_policy.h"
#include <QSpinBox>
#include <QDateTimeEdit>
#include "app_settings.h"
#include "scan_overlay.h"
#include <QJsonArray>
#include "ui_elements.h"
#include <cstdio>
int renderPublicationGallery(QApplication &app);
#include <stdexcept>
#include "main_window.h"
#include "sidebar.h"
#include "title_bar.h"
#include "tools_drawer.h"
#include "system_profile.h"
#include <QProgressBar>
#include "driver_catalog.h"
#include "engine_bridge.h"
#include "installation_workspace.h"
#include "appearance.h"
#include "premium_popover.h"
#include "vector_icons.h"
#include "single_instance.h"
#include "driver_details_dialog.h"

static QTreeWidgetItem *deviceItem(QTreeWidget *tree,int row=0){for(QTreeWidgetItemIterator it(tree);*it;++it)if((*it)->data(0,Qt::UserRole+1).toInt()==-1 && (*it)->data(0,Qt::UserRole).toInt()==row)return *it;return 0;}
static void collapseDevices(QTreeWidget *tree){for(QTreeWidgetItemIterator it(tree);*it;++it)if((*it)->data(0,Qt::UserRole+1).toInt()==-1)(*it)->setExpanded(false);}
static int checks=0;
static void check(bool result,const char *message){if(!result)throw std::runtime_error(message);++checks;std::printf("PASS %d: %s\n",checks,message);std::fflush(stdout);}
class DpiOpacityProbe : public QGraphicsOpacityEffect {
public:explicit DpiOpacityProbe(QObject *parent):QGraphicsOpacityEffect(parent),sourceDpr(0){}qreal sourceDpr;QSize physicalSize;
protected:void draw(QPainter *painter) Q_DECL_OVERRIDE {QPoint offset;const QPixmap source=sourcePixmap(Qt::DeviceCoordinates,&offset,QGraphicsEffect::NoPad);sourceDpr=source.devicePixelRatio();physicalSize=source.size();QGraphicsOpacityEffect::draw(painter);}
};
static QJsonObject probeFractionalRendering(const QString &name){
    QWidget host;host.resize(420,128);host.setAutoFillBackground(true);QPalette background=host.palette();background.setColor(QPalette::Window,QColor("#075266"));host.setPalette(background);
    QFont face(QStringLiteral("Segoe UI"));face.setPixelSize(12);face.setStyleStrategy(QFont::PreferQuality);face.setHintingPreference(QFont::PreferFullHinting);
    QWidget *reference=new QWidget(&host),*animated=new QWidget(&host);reference->setGeometry(10,8,400,44);animated->setGeometry(10,72,400,44);
    auto contents=[&](QWidget *group,int alpha){QLabel *text=new QLabel(QStringLiteral("Find the right match for your PC."),group);text->setGeometry(0,0,330,44);text->setAlignment(Qt::AlignRight|Qt::AlignVCenter);text->setFont(face);QPalette palette=text->palette();QColor ink("#C1DEE2");ink.setAlpha(alpha);palette.setColor(QPalette::WindowText,ink);text->setPalette(palette);QLabel *icon=new QLabel(group);icon->setGeometry(354,12,20,20);QColor tint("#E3F4F6");tint.setAlpha(alpha);icon->setPixmap(VectorIcons::pixmap(QStringLiteral("refresh"),tint,20));};
    contents(reference,140);contents(animated,255);DpiOpacityProbe *effect=new DpiOpacityProbe(animated);effect->setOpacity(140./255.);animated->setGraphicsEffect(effect);host.show();QTest::qWait(80);
    const QString suffix=QString::number(qRound(host.devicePixelRatioF()*100));const QString imagePath=QDir(QCoreApplication::applicationDirPath()).filePath(QStringLiteral("dpi-")+name+QLatin1Char('-')+suffix+QStringLiteral(".png"));host.grab().save(imagePath);
    const QPixmap svg=VectorIcons::icon(QStringLiteral("refresh"),QColor(Qt::white),20).pixmap(QSize(20,20));QJsonObject result;result.insert(QStringLiteral("deviceDpr"),host.devicePixelRatioF());result.insert(QStringLiteral("effectSourceDpr"),effect->sourceDpr);result.insert(QStringLiteral("sourcePhysicalWidth"),effect->physicalSize.width());result.insert(QStringLiteral("expectedPhysicalWidth"),qRound(animated->width()*host.devicePixelRatioF()));result.insert(QStringLiteral("svgPhysicalWidth"),svg.width());result.insert(QStringLiteral("svgDpr"),svg.devicePixelRatio());result.insert(QStringLiteral("image"),imagePath);
    QFile report(QDir(QCoreApplication::applicationDirPath()).filePath(QStringLiteral("dpi-")+name+QLatin1Char('-')+suffix+QStringLiteral(".json")));if(report.open(QIODevice::WriteOnly))report.write(QJsonDocument(result).toJson());return result;
}
int main(int argc,char **argv){
#ifdef Q_OS_WIN
    SetErrorMode(SEM_FAILCRITICALERRORS|SEM_NOGPFAULTERRORBOX|SEM_NOOPENFILEERRORBOX);
#endif
    QApplication::setAttribute(Qt::AA_EnableHighDpiScaling);
    QApplication::setAttribute(Qt::AA_UseHighDpiPixmaps);
    for(int i=1;i<argc;++i)if(QByteArray(argv[i])==QByteArray("--zoom-probe"))UiScale::prepareEnvironment();
    QApplication app(argc,argv);app.setQuitOnLastWindowClosed(false);app.setOrganizationName(QStringLiteral("SDIO-UI-Regression"));UiFont::applyLanguage(QStringLiteral("english"));installInterfaceMotion();
    if(app.arguments().contains(QStringLiteral("--publication-gallery")))return renderPublicationGallery(app);
    if(app.arguments().contains(QStringLiteral("--zoom-probe"))){QWidget probe;probe.resize(400,200);probe.show();QTest::qWait(30);QJsonObject report;report.insert(QStringLiteral("effectivePercent"),UiScale::effectivePercent());report.insert(QStringLiteral("deviceDpr"),probe.devicePixelRatioF());const QJsonObject rendering=probeFractionalRendering(QStringLiteral("zoom"));report.insert(QStringLiteral("rendering"),rendering);std::printf("%s\n",QJsonDocument(report).toJson(QJsonDocument::Compact).constData());return 0;}
    if(app.arguments().contains(QStringLiteral("--live-update-probe"))){std::atomic<bool> stop(false);const auto release=AppUpdate::latest(AppUpdate::projectRepository(),stop);const QString target=QDir(app.applicationDirPath()).filePath(QStringLiteral("verified-public-release.exe"));AppUpdate::downloadVerified(release,target,stop);QFile binary(target);if(!binary.open(QIODevice::ReadOnly))return 2;QJsonObject report;report.insert(QStringLiteral("version"),release.version);report.insert(QStringLiteral("sha256"),QString::fromLatin1(QCryptographicHash::hash(binary.readAll(),QCryptographicHash::Sha256).toHex()));report.insert(QStringLiteral("githubDigest"),QString::fromLatin1(release.sha256));report.insert(QStringLiteral("applied"),false);std::printf("%s\n",QJsonDocument(report).toJson(QJsonDocument::Compact).constData());return 0;}
    if(app.arguments().contains(QStringLiteral("--dpi-probe-before")) || app.arguments().contains(QStringLiteral("--dpi-probe-after"))){const QJsonObject result=probeFractionalRendering(app.arguments().contains(QStringLiteral("--dpi-probe-before"))?QStringLiteral("before"):QStringLiteral("after"));std::printf("%s\n",QJsonDocument(result).toJson(QJsonDocument::Compact).constData());return 0;}
    if(app.arguments().contains(QStringLiteral("--theme-preview"))){QScopedPointer<QSettings> preferences(createAppSettings(app.applicationDirPath(),0,false));preferences->clear();preferences->sync();Theme::setDocument(Theme::readPack(QDir(app.applicationDirPath()).filePath(QStringLiteral("../../themes/copper-night.sdi-theme.json"))));MainWindow sample(true);DeviceInfo previewDevice;previewDevice.name=QStringLiteral("Sample device");previewDevice.manufacturer=QStringLiteral("Hardware manufacturer");previewDevice.deviceClass=QStringLiteral("Display");previewDevice.hardwareId=QStringLiteral("DEMO-ONLY");previewDevice.driverVersion=QStringLiteral("1.0");sample.findChild<DriverCatalog*>()->replace(QVector<DeviceInfo>()<<previewDevice,true);sample.resize(1120,760);sample.findChild<GradientButton*>()->setMotionEnabled(false);sample.show();QTest::qWait(300);sample.grab().save(QDir(app.applicationDirPath()).filePath(QStringLiteral("copper-night-light.png")));QMetaObject::invokeMethod(&sample,"updateDark",Q_ARG(bool,true));QTest::qWait(100);sample.grab().save(QDir(app.applicationDirPath()).filePath(QStringLiteral("copper-night-dark.png")));preferences->clear();preferences->sync();std::printf("Community theme previews rendered\n");return 0;}
    if(app.arguments().contains(QStringLiteral("--language-smoke")) || app.arguments().contains(QStringLiteral("--language-smoke-all"))){MainWindow sample(true);DeviceInfo smokeDevice;smokeDevice.name=QStringLiteral("LANGUAGE TEST DEVICE");smokeDevice.hardwareId=QStringLiteral("TEST-LOCALIZATION");smokeDevice.driverVersion=QStringLiteral("1.0");sample.findChild<DriverCatalog*>()->replace(QVector<DeviceInfo>()<<smokeDevice,true);sample.show();QTest::qWait(60);QComboBox *selector=sample.findChild<QComboBox*>(QStringLiteral("premiumCombo"));for(int i=0;i<selector->count();++i){if(!app.arguments().contains(QStringLiteral("--language-smoke-all")) && selector->itemData(i).toString()!=QStringLiteral("arabic"))continue;std::printf("Locale %s\n",selector->itemData(i).toString().toUtf8().constData());std::fflush(stdout);selector->setCurrentIndex(i);QTest::qWait(40);}std::printf("Language smoke passed\n");return 0;}
    try {
        const QJsonObject dpi=probeFractionalRendering(QStringLiteral("regression"));
        check(qAbs(dpi.value(QStringLiteral("effectSourceDpr")).toDouble()-dpi.value(QStringLiteral("deviceDpr")).toDouble())<.01,"animated text and icons retain the fractional display scale");
        check(dpi.value(QStringLiteral("sourcePhysicalWidth")).toInt()>=dpi.value(QStringLiteral("expectedPhysicalWidth")).toInt(),"animation caches contain full physical pixel resolution");
        check(dpi.value(QStringLiteral("svgPhysicalWidth")).toInt()>=qRound(20*dpi.value(QStringLiteral("deviceDpr")).toDouble()),"SVG icons are rasterized at native device resolution");
        check(qAbs(dpi.value(QStringLiteral("svgDpr")).toDouble()-dpi.value(QStringLiteral("deviceDpr")).toDouble())<.01,"SVG icon metadata preserves the fractional display scale");
        const wchar_t *lock=L"Local\\SDIO.TarekSadek.UIRegressionTest";
        {SingleInstance first(lock);check(first.available() && !first.alreadyRunning(),"primary lock");
         SingleInstance second(lock);check(second.alreadyRunning(),"duplicate lock across instances");}
        {SingleInstance recovered(lock);check(!recovered.alreadyRunning(),"lock released without stale files");}
        DriverEntry priority;DriverCandidate installed;priority.candidates.append(installed);
        for(int i=1;i<=10;++i){DriverCandidate candidate;candidate.originalOrder=i;candidate.valid=true;candidate.rawScore=0x1234+i;priority.candidates.append(candidate);}
        check(matchPriorityStars(priority,0)==0,"installed driver has no relative priority rating");
        check(matchPriorityStars(priority,1)==5 && matchPriorityStars(priority,2)==5,"top priority band follows original order");
        check(matchPriorityStars(priority,3)==4 && matchPriorityStars(priority,10)==1,"lower priority bands follow original order");
        priority.candidates[1].valid=false;check(matchPriorityStars(priority,1)==0,"ineligible candidates never receive priority stars");
        check(priority.candidates[3].rawScore==0x1237,"priority presentation does not modify raw engine scores");
        DriverEntry state;state.sdioVerified=true;state.sdioDriverPresent=true;state.candidates.resize(2);state.candidates[1].valid=true;state.candidates[1].recommended=true;state.chosenCandidate=1;
        check(driverStatus(state)==QStringLiteral("Installed"),"short status reflects an unchanged installed driver");state.candidates[1].statusBits=0x20;check(driverStatus(state)==QStringLiteral("Upgrade"),"short status uses native upgrade bit");state.candidates[1].statusBits=0x80;check(driverStatus(state)==QStringLiteral("Downgrade"),"short status uses native downgrade bit");state.sdioDriverPresent=false;check(driverStatus(state)==QStringLiteral("Missing"),"short status reports a missing installed driver");check(driverStatus(state,0)==QStringLiteral("Installed"),"installed version row stays Installed");
        Theme::validate(Theme::defaultDocument());check(true,"built-in theme meets documented contrast requirements");
        QTemporaryDir library;check(library.isValid(),"library detection fixtures use an isolated folder");QDir().mkpath(library.path()+QStringLiteral("/drivers"));QDir().mkpath(library.path()+QStringLiteral("/indexes/SDI"));QDir().mkpath(library.path()+QStringLiteral("/indexes/SDIO"));
        auto fixtureFile=[](const QString &path,const QByteArray &bytes){QFile file(path);if(!file.open(QIODevice::WriteOnly) || file.write(bytes)!=bytes.size())throw std::runtime_error("Cannot create library fixture");};
        const QByteArray supported=QByteArray::fromHex("53445705020000")+QByteArray("fixture");const QString sdiIndex=library.path()+QStringLiteral("/indexes/SDI/DP_Demo.bin"),sdioIndex=library.path()+QStringLiteral("/indexes/SDIO/DP_Other.bin");fixtureFile(sdiIndex,supported);fixtureFile(sdioIndex,supported);fixtureFile(library.path()+QStringLiteral("/drivers/DP_Demo.7z"),QByteArray("fixture"));
        check(resolveIndexDirectory(library.path()+QStringLiteral("/indexes"),library.path()+QStringLiteral("/drivers")).endsWith(QStringLiteral("/SDI")),"mixed index roots prefer the cache matching the selected driver packs");
        fixtureFile(library.path()+QStringLiteral("/SDIO_x64.exe"),QByteArray("fixture"));check(resolveIndexDirectory(library.path()+QStringLiteral("/indexes"),library.path()+QStringLiteral("/drivers")).endsWith(QStringLiteral("/SDIO")),"SDIO layout detection selects its own index subtree");
        check(compatibleLibraryIndex(sdiIndex),"native cache signatures accept the supported SDI and SDIO versions");fixtureFile(sdioIndex,QByteArray::fromHex("53445706020000"));check(!compatibleLibraryIndex(sdioIndex),"unknown index formats are rejected before native parsing");
        QTemporaryDir classic;QDir().mkpath(classic.path()+QStringLiteral("/Index/SDI"));fixtureFile(classic.path()+QStringLiteral("/Index/SDI/DP_Test.bin"),supported);check(resolveIndexDirectory(classic.path()+QStringLiteral("/indexes"),classic.path()+QStringLiteral("/Drivers")).endsWith(QStringLiteral("/Index/SDI")),"classic SDI Index directory is found when the default indexes path is absent");QFile::remove(classic.path()+QStringLiteral("/Index/SDI/DP_Test.bin"));check(libraryFamily(classic.path()+QStringLiteral("/Drivers"),classic.path()+QStringLiteral("/Index"))==QStringLiteral("SDI") && resolveIndexDirectory(classic.path()+QStringLiteral("/indexes"),classic.path()+QStringLiteral("/Drivers")).endsWith(QStringLiteral("/Index/SDI")),"an empty SDI library still selects its own index subtree and catalog family");
        const QString managed=prepareLibraryIndexes(library.path()+QStringLiteral("/drivers"),library.path()+QStringLiteral("/indexes/SDI"));check(QFileInfo(QDir(managed).filePath(QStringLiteral("DP_Demo.bin"))).isFile() && !managed.startsWith(library.path()),"native scans use a private copied cache without rewriting library indexes");
        InstallationWorkspace blocked;blocked.beginNative(QStringList()<<QStringLiteral("Example"),QStringList()<<QStringLiteral("DEMO-ID"),true);sdiomodern::InstallReport restoreFailure;restoreFailure.restoreFailed=true;restoreFailure.restoreError=1058;restoreFailure.error=L"System Protection disabled (1058)";blocked.finishNative(restoreFailure);check(blocked.findChild<QPushButton*>(QStringLiteral("restoreHelp"))->isHidden()==false && blocked.findChild<QProgressBar*>()->value()==0,"restore failure offers System Protection and never shows completed driver progress");
        const QString safeEnvironment=BugReport::environment(QStringLiteral("Windows 11"),QStringLiteral("x86_64"),QStringLiteral("arabic"),true);
        const QUrl bugUrl=BugReport::issueUrl(safeEnvironment);const QUrlQuery bugQuery(bugUrl);
        check(bugUrl.scheme()==QStringLiteral("https") && bugUrl.host()==QStringLiteral("github.com") && bugUrl.path()==QStringLiteral("/Diagoo1/Snappy-Driver-Installer-Modern/issues/new"),"bug reports open the authorized repository over HTTPS");
        check(bugQuery.queryItemValue(QStringLiteral("template"))==QStringLiteral("bug_report.yml") && bugQuery.queryItemValue(QStringLiteral("environment"),QUrl::FullyDecoded)==safeEnvironment,"issue form prefill preserves the reviewed environment");
        check(!safeEnvironment.contains(QStringLiteral("hardware"),Qt::CaseInsensitive) && !safeEnvironment.contains(QStringLiteral("instance"),Qt::CaseInsensitive) && !safeEnvironment.contains(QStringLiteral("Users")),"automatic bug context omits hardware IDs and user paths");
        QTimer::singleShot(60,[](){for(QWidget *widget:QApplication::topLevelWidgets())if(widget->objectName()==QStringLiteral("bugReportDialog")){QPushButton *cancel=widget->findChild<QPushButton*>(QStringLiteral("bugReportCancel"));if(cancel)cancel->click();}});
        BugReport::show(0);check(true,"bug report preview can close without opening a browser or sending data");
        for(int elevated=0;elevated<2;++elevated){QTimer::singleShot(60,[](){for(QWidget *widget:QApplication::topLevelWidgets())if(widget->objectName()==QStringLiteral("installationConsent"))if(QPushButton *cancel=widget->findChild<QPushButton*>(QStringLiteral("installationCancel")))cancel->click();});check(!confirmDriverInstallation(0,QStringList()<<QStringLiteral("Example device · 1.2.3"),true,false,false,elevated==1,false),"installation and elevation previews can cancel without a Windows action");}
        bool reinstallWarningVisible=false;QTimer::singleShot(60,[&](){for(QWidget *widget:QApplication::topLevelWidgets())if(widget->objectName()==QStringLiteral("installationConsent")){QLabel *notice=widget->findChild<QLabel*>(QStringLiteral("installNotice"));reinstallWarningVisible=notice && notice->text().contains(QStringLiteral("Reinstallation selected"));widget->findChild<QPushButton*>(QStringLiteral("installationCancel"))->click();}});confirmDriverInstallation(0,QStringList()<<QStringLiteral("Reinstall · Example"),false,false,false,true,false,true);check(reinstallWarningVisible,"reinstallation requires a visible operation-specific confirmation before execution");
        const QJsonObject copper=Theme::readPack(QDir(app.applicationDirPath()).filePath(QStringLiteral("../../themes/copper-night.sdi-theme.json")));Theme::setDocument(copper);check(Theme::document().value(QStringLiteral("id")).toString()==QStringLiteral("copper-night"),"community sample theme imports without style code");Theme::setDocument(QJsonObject());
        QJsonObject unreadable=copper,palette=copper.value(QStringLiteral("light")).toObject();palette.insert(QStringLiteral("text"),palette.value(QStringLiteral("surface")));unreadable.insert(QStringLiteral("light"),palette);bool rejectedTheme=false;try{Theme::validate(unreadable);}catch(const std::exception&){rejectedTheme=true;}check(rejectedTheme,"theme import rejects unreadable text contrast");
        check(AppUpdate::newer(QStringLiteral("v0.20.0"),QStringLiteral("0.19.0")) && !AppUpdate::newer(QStringLiteral("0.18.9"),QStringLiteral("0.19.0")),"application updater compares semantic versions numerically");
        QJsonObject release;release.insert(QStringLiteral("draft"),false);release.insert(QStringLiteral("prerelease"),false);release.insert(QStringLiteral("tag_name"),QStringLiteral("v0.20.0"));release.insert(QStringLiteral("html_url"),QStringLiteral("https://github.com/test/project/releases/tag/v0.20.0"));QJsonObject asset;asset.insert(QStringLiteral("name"),QStringLiteral("SnappyDriverInstaller-TS.exe"));asset.insert(QStringLiteral("size"),24000000);asset.insert(QStringLiteral("browser_download_url"),QStringLiteral("https://github.com/test/project/releases/download/v0.20.0/SnappyDriverInstaller-TS.exe"));asset.insert(QStringLiteral("digest"),QStringLiteral("sha256:")+QString(64,QLatin1Char('a')));release.insert(QStringLiteral("assets"),QJsonArray()<<asset);check(AppUpdate::parseRelease(release,QStringLiteral("test/project")).size==24000000,"updater binds the expected executable to the configured repository and GitHub digest");asset.remove(QStringLiteral("digest"));release.insert(QStringLiteral("assets"),QJsonArray()<<asset);bool rejectedUpdate=false;try{AppUpdate::parseRelease(release,QStringLiteral("test/project"));}catch(const std::exception&){rejectedUpdate=true;}check(rejectedUpdate,"updater rejects assets without a SHA-256 digest");
        QTemporaryDir updateArea;const QString oldExe=QDir(updateArea.path()).filePath(QStringLiteral("old.exe")),newExe=QDir(updateArea.path()).filePath(QStringLiteral("new.exe"));QByteArray oldBytes(2048,'a');oldBytes[0]='M';oldBytes[1]='Z';oldBytes[60]=char(128);oldBytes[61]=oldBytes[62]=oldBytes[63]=0;oldBytes.replace(128,6,QByteArray::fromHex("504500004c01"));QByteArray newBytes=oldBytes;newBytes[1024]='b';QFile oldFile(oldExe),newFile(newExe);check(oldFile.open(QIODevice::WriteOnly) && newFile.open(QIODevice::WriteOnly),"update fixtures are isolated temporary files");oldFile.write(oldBytes);newFile.write(newBytes);oldFile.close();newFile.close();bool rejectedReplacement=false;try{AppUpdate::replaceVerified(oldExe,newExe,QByteArray(64,'0'));}catch(const std::exception&){rejectedReplacement=true;}oldFile.open(QIODevice::ReadOnly);check(rejectedReplacement && oldFile.readAll()==oldBytes,"failed update verification preserves the existing executable");oldFile.close();AppUpdate::replaceVerified(oldExe,newExe,QCryptographicHash::hash(newBytes,QCryptographicHash::Sha256).toHex());oldFile.open(QIODevice::ReadOnly);check(oldFile.readAll()==newBytes,"verified update replaces the temporary fixture atomically without execution");oldFile.close();
        bool restartInvoked=false;QTimer::singleShot(80,[](){for(QWidget *widget:QApplication::topLevelWidgets())if(widget->objectName()==QStringLiteral("restartPrompt")){QPushButton *cancel=widget->findChild<QPushButton*>(QStringLiteral("restartCancel"));if(cancel)cancel->click();}});check(!restartPrompt(0,60,[&]{restartInvoked=true;return QString();}) && !restartInvoked,"automatic restart can be cancelled before any OS action");check(restartPrompt(0,1,[&]{restartInvoked=true;return QString();}) && restartInvoked,"countdown invokes injected restart action exactly after expiry");
        SingleInstance *primary=new SingleInstance(L"Local\\SDIO.TarekSadek.DuplicateDialogTest");
        SingleInstance duplicate(L"Local\\SDIO.TarekSadek.DuplicateDialogTest");delete primary;
        SingleInstance restarted(L"Local\\SDIO.TarekSadek.DuplicateDialogTest");
        check(!restarted.alreadyRunning(),"duplicate notification cannot retain the exited primary session guard");
        QScopedPointer<QSettings> testSettings(createAppSettings(app.applicationDirPath(),0,false));
        testSettings->clear();testSettings->sync();
        const QString legacySettings=QDir(app.applicationDirPath()).filePath(QStringLiteral("SnappyDriverInstaller-TS.ini"));
        QSettings settings(legacySettings,QSettings::IniFormat);
        settings.clear();settings.setValue(QStringLiteral("reduceMotion"),true);settings.sync();
        MainWindow window(true);window.resize(1120,760);window.show();QTest::qWait(60);
        testSettings->sync();
        check(!QFile::exists(legacySettings),"legacy sidecar removed after successful registry migration");
        check(testSettings->value(QStringLiteral("motionProfile")).toInt()==2,"migrated settings share the native user store");
        {QScopedPointer<QSettings> reopened(createAppSettings(app.applicationDirPath()));
         check(reopened->value(QStringLiteral("reduceMotion")).toBool()==false,"preferences survive reopening without a sidecar");}
        // Settle initial font shaping and widget polish before timing a fade.
        QTest::qWait(250);
        TitleBar *dpiHeader=window.findChild<TitleBar*>();QLabel *phrase=dpiHeader->findChild<QLabel*>(QStringLiteral("headerStatus"));QGraphicsOpacityEffect *phraseEffect=qobject_cast<QGraphicsOpacityEffect*>(phrase->graphicsEffect());QTimer *phraseTimer=dpiHeader->findChild<QTimer*>(QStringLiteral("headerPhraseTimer"));QMetaObject::invokeMethod(phraseTimer,"timeout",Qt::DirectConnection);QTest::qWait(45);check(phraseEffect && phraseEffect->opacity()>0 && phraseEffect->opacity()<1,"header text fade remains animated");phraseEffect->setOpacity(.55);dpiHeader->grab().save(QDir(app.applicationDirPath()).filePath(QStringLiteral("header-mid-fade-")+QString::number(qRound(window.devicePixelRatioF()*100))+QStringLiteral(".png")));QTest::qWait(350);check(qAbs(phraseEffect->opacity()-1)<.001,"header text returns to full opacity after its animation");
        TitleButton *closeButton=0;
        for(TitleButton *button:window.findChildren<TitleButton*>())if(button->objectName()==QStringLiteral("windowClose"))closeButton=button;
        check(closeButton!=0,"title bar uses dedicated vector hover buttons");
        QEvent hoverEnter(QEvent::Enter);QApplication::sendEvent(closeButton,&hoverEnter);QTest::qWait(50);
        check(closeButton->hoverAmount()>0 && closeButton->hoverAmount()<1,"close hover transitions smoothly");
        check(!closeButton->findChild<HoverSurface*>(),"title icon is not covered by generic hover overlay");
        QTest::qWait(160);window.findChild<TitleBar*>()->grab().save(QDir(app.applicationDirPath()).filePath(QStringLiteral("title-hover.png")));
        QTest::mouseMove(&window,QPoint(window.width()/2,window.height()/2));
        QEvent hoverLeave(QEvent::Leave);QApplication::sendEvent(closeButton,&hoverLeave);
        for(int frame=0;frame<24 && closeButton->hoverAmount()>0;++frame)QTest::qWait(25);
        check(closeButton->hoverAmount()==0,"close hover resets on leave");
        check(!app.property("reduceMotion").toBool(),"upgrade restores requested animation default");
        Sidebar *sidebar=window.findChild<Sidebar*>();check(sidebar!=0,"sidebar exists");
        const QList<SidebarItem*> buttons=sidebar->findChildren<SidebarItem*>();
        check(buttons.size()==5,"five navigation destinations including About");
        for(SidebarItem *button:buttons)check(button->property("pageIndex").toInt()!=4,"installer is absent from navigation");
        sidebar->setCurrentPage(1);QTest::qWait(45);
        sidebar->grab().save(QDir(app.applicationDirPath()).filePath(QStringLiteral("sidebar-active-motion.png")));
        check(sidebar->activeY()>buttons[0]->y() && sidebar->activeY()<buttons[1]->y(),"selected sidebar surface and marker visibly travel between destinations");
        QTest::qWait(220);check(qAbs(sidebar->activeY()-buttons[1]->y())<1,"selected sidebar surface settles on its destination");
        QEvent navEnter(QEvent::Enter),navLeave(QEvent::Leave);QApplication::sendEvent(buttons[0],&navEnter);QTest::qWait(200);QApplication::sendEvent(buttons[0],&navLeave);QApplication::sendEvent(buttons[1],&navEnter);QTest::qWait(45);
        sidebar->grab().save(QDir(app.applicationDirPath()).filePath(QStringLiteral("sidebar-hover-motion.png")));
        check(sidebar->hoverY()>buttons[0]->y() && sidebar->hoverY()<buttons[1]->y() && sidebar->hoverOpacity()>0,"one shared hover surface slides visibly between sidebar items");
        {QApplication::sendEvent(buttons[1],&navLeave);QApplication::sendEvent(buttons[2],&navEnter);QTest::qWait(200);const QImage frame=sidebar->grab().toImage();const qreal ratio=frame.width()/qreal(sidebar->width());const QColor active=QColor::fromRgba(frame.pixel(qRound(16*ratio),qRound((buttons[1]->y()+30)*ratio))),hover=QColor::fromRgba(frame.pixel(qRound(16*ratio),qRound((buttons[2]->y()+30)*ratio)));check(active.alpha()>240 && hover.alpha()<60 && hover.alpha()>0 && qAbs(sidebar->activeY()-buttons[1]->y())<1,"hover stays quiet while the selected page keeps its full background and marker");}

        QApplication::sendEvent(buttons[1],&navLeave);QApplication::sendEvent(buttons[3],&navEnter);QTest::qWait(25);QApplication::sendEvent(buttons[3],&navLeave);QApplication::sendEvent(buttons[2],&navEnter);QTest::qWait(220);
        check(qAbs(sidebar->hoverY()-buttons[2]->y())<1,"rapid sidebar hovering retargets to the last item without queued movement");
        sidebar->setAnimations(false);sidebar->setCurrentPage(3);QApplication::sendEvent(buttons[2],&navLeave);QApplication::sendEvent(buttons[3],&navEnter);
        check(qAbs(sidebar->activeY()-buttons[3]->y())<1 && qAbs(sidebar->hoverY()-buttons[3]->y())<1 && sidebar->hoverOpacity()==1,"disabling motion snaps both shared sidebar highlights immediately");
        sidebar->toggleCollapsed();QTest::qWait(20);
        check(qAbs(sidebar->activeY()-buttons[3]->y())<1 && qAbs(sidebar->hoverY()-buttons[3]->y())<1,"compact navigation realigns both highlights after the footer changes the bottom tabs' positions");
        sidebar->grab().save(QDir(app.applicationDirPath()).filePath(QStringLiteral("sidebar-compact-motion.png")));sidebar->toggleCollapsed();
        QApplication::sendEvent(buttons[3],&navLeave);sidebar->setAnimations(true);sidebar->setCurrentPage(0);QTest::qWait(220);
        for(int click=0;click<10;++click){QTest::mouseClick(buttons[0],Qt::LeftButton);QTest::qWait(5);}check(buttons[0]->isChecked() && !buttons[1]->isChecked(),"repeated mouse clicks on the active tab preserve its selected state and bold type");
        QTest::keyClick(buttons[0],Qt::Key_Space);check(buttons[0]->isChecked(),"keyboard activation cannot toggle the current page off");
        sidebar->toggleCollapsed();QTest::qWait(60);
        check(sidebar->width()>84 && sidebar->width()<224,"sidebar animates through intermediate widths");
        QTest::qWait(300);check(sidebar->width()==84,"sidebar reaches collapsed width");
        sidebar->toggleCollapsed();QTest::qWait(320);
        buttons.first()->setHoverAmount(0);QEvent enter(QEvent::Enter);QApplication::sendEvent(buttons.first(),&enter);QTest::qWait(50);
        check(buttons.first()->hoverAmount()>0 && buttons.first()->hoverAmount()<1,"navigation hover interpolates");
        QMetaObject::invokeMethod(&window,"navigate",Q_ARG(int,1));
        QStackedWidget *pages=window.findChild<QStackedWidget*>();
        check(pages->currentIndex()==1 && pages->currentWidget()->graphicsEffect(),"page transition has a running effect");QTest::qWait(220);
        for(int i=0;i<40 && pages->currentWidget()->graphicsEffect();++i)QTest::qWait(25);
        check(!pages->currentWidget()->graphicsEffect(),"page transition releases its effect");
        QMetaObject::invokeMethod(&window,"navigate",Q_ARG(int,0));QTest::qWait(220);
        std::printf("Resolved UI font: %s\n",QFontInfo(window.font()).family().toUtf8().constData());
        QMetaObject::invokeMethod(&window,"navigate",Q_ARG(int,5));QTest::qWait(240);
        QLabel *developer=pages->widget(5)->findChild<QLabel*>(QStringLiteral("developerName"));
        check(developer && developer->text()==QStringLiteral("Tarek Sadek"),"About page presents the verified developer identity");
        const QList<ActionButton*> aboutActions=pages->widget(5)->findChildren<ActionButton*>();bool equalActions=aboutActions.size()==5;for(ActionButton *button:aboutActions)equalActions=equalActions && qAbs(button->width()-aboutActions.first()->width())<=1 && !button->icon().isNull() && !button->text().isEmpty();check(equalActions,"About actions have equal widths and an icon beside every label");
        window.grab().save(QDir(app.applicationDirPath()).filePath(QStringLiteral("about-page.png")));
        QMetaObject::invokeMethod(&window,"navigate",Q_ARG(int,2));QTest::qWait(240);
        window.grab().save(QDir(app.applicationDirPath()).filePath(QStringLiteral("packages-page.png")));
        QMetaObject::invokeMethod(&window,"navigate",Q_ARG(int,0));QTest::qWait(240);
        const QImage normal=window.grab().toImage();normal.save(QDir(app.applicationDirPath()).filePath(QStringLiteral("main.png")));
        check(qAlpha(normal.pixel(0,0))<30,"window corner is transparent, not a square crop");
        bool smooth=false;
        for(int y=0;y<24 && !smooth;++y)for(int x=0;x<24;++x){int alpha=qAlpha(normal.pixel(x,y));if(alpha>0 && alpha<255)smooth=true;}
        check(smooth,"window curve contains antialiased alpha coverage");
        ScanOverlay *cover=static_cast<ScanOverlay*>(window.findChild<QWidget*>(QStringLiteral("nativeScanOverlay")));
        check(cover && cover->parentWidget()==pages,"loading cover belongs only to content pages");
        cover->start(QStringLiteral("Checking content-only loading overlay"));QTest::qWait(80);
        check(cover->geometry()==pages->rect(),"loading cover exactly follows content bounds");
        const QRect covered(cover->mapTo(&window,QPoint()),cover->size());
        check(!covered.intersects(QRect(sidebar->mapTo(&window,QPoint()),sidebar->size())),"sidebar is outside the loading cover");
        TitleBar *visibleTitle=window.findChild<TitleBar*>();
        check(!covered.intersects(QRect(visibleTitle->mapTo(&window,QPoint()),visibleTitle->size())),"title controls are outside the loading cover");
        sidebar->toggleCollapsed();QTest::qWait(320);
        check(cover->geometry()==pages->rect(),"content cover follows animated sidebar resizing");
        const QImage coveredImage=window.grab().toImage();
        coveredImage.save(QDir(app.applicationDirPath()).filePath(QStringLiteral("loading-content.png")));
        const qreal imageScale=coveredImage.width()/qreal(window.width());
        const QPoint contentOrigin=cover->mapTo(&window,QPoint(0,0));
        bool noWhiteGaps=true;
        const QPoint localCorners[]={QPoint(1,1),QPoint(cover->width()-2,1),QPoint(1,cover->height()-2)};
        for(const QPoint &local:localCorners){const QPoint logical=contentOrigin+local;
            const QColor color=QColor::fromRgba(coveredImage.pixel(qRound(logical.x()*imageScale),qRound(logical.y()*imageScale)));
            if(color.red()>200 && color.green()>200 && color.blue()>200)noWhiteGaps=false;
        }
        check(noWhiteGaps,"content loading cover has no exposed white wedges at inner corners");
        QMetaObject::invokeMethod(&window,"navigate",Q_ARG(int,1));
        check(pages->currentIndex()==0,"busy content cannot switch to another page");
        cover->finish();sidebar->toggleCollapsed();QTest::qWait(320);
        const QPixmap icon=VectorIcons::pixmap(QStringLiteral("chip"),QColor("#00ADA3"),32);
        check(icon.width()>=32*icon.devicePixelRatio(),"label icon retains full DPI resolution");
        bool safeDefault=false;
        QTimer::singleShot(40,[&](){
            QDialog *dialog=qobject_cast<QDialog*>(app.activeModalWidget());
            if(dialog){for(QPushButton *button:dialog->findChildren<QPushButton*>())
                if(button->isDefault() && button->text()==QStringLiteral("Cancel"))safeDefault=true;
                dialog->reject();}
        });
        check(!projectMessage(&window,QStringLiteral("Confirmation test"),QStringLiteral("No system operation will be started."),true),"confirmation can be cancelled");
        check(safeDefault,"confirmation defaults to cancellation");
        QTimer::singleShot(230,[&](){
            QDialog *dialog=qobject_cast<QDialog*>(app.activeModalWidget());
            if(dialog){dialog->grab().save(QDir(app.applicationDirPath()).filePath(QStringLiteral("already-running.png")));dialog->accept();}
        });
        projectMessage(&window,QStringLiteral("Already running"),QStringLiteral("Snappy Driver Installer is already open. Return to the existing window to continue; a second session cannot be started."));
        PremiumPopover popup(&window);popup.addHeading(QStringLiteral("Rounded popup"));popup.addOption(QStringLiteral("Smooth edges"),true);
        popup.openAt(buttons.first());QTest::qWait(180);
        const QImage popupImage=popup.grab().toImage();popupImage.save(QDir(app.applicationDirPath()).filePath(QStringLiteral("popup.png")));
        check(qAlpha(popupImage.pixel(0,0))<30,"popup uses alpha corners");popup.hide();
        DriverCatalog *catalog=window.findChild<DriverCatalog*>();QVector<DeviceInfo> devices;DeviceInfo device;
        device.hardwareId=QStringLiteral("PCI\\VEN_TEST&DEV_DEMO");device.name=QStringLiteral("TEST DEVICE - DEMONSTRATION ONLY");devices.append(device);catalog->replace(devices,true);catalog->selectAll(true);
        QTest::qWait(20);
        DriverWorkspace *workspace=window.findChild<DriverWorkspace*>();
        QTreeWidget *tree=workspace->findChild<QTreeWidget*>(QStringLiteral("driverTree"));
        QList<QAbstractButton*> workspaceCommands;for(QAbstractButton *button:workspace->findChildren<QAbstractButton*>())if(button->property("workspaceCommand").toBool())workspaceCommands.append(button);
        check(workspaceCommands.size()==5,"workspace exposes exactly five consistently styled secondary commands");
        bool consistentCommands=true;for(QAbstractButton *button:workspaceCommands)if(button->height()!=workspaceCommands.first()->height() || button->font().pixelSize()!=13 || button->icon().isNull() || button->iconSize()!=QSize(18,18))consistentCommands=false;check(consistentCommands,"workspace commands share height, typography and vector icon sizing");
        const QImage headerImage=tree->header()->viewport()->grab().toImage();const qreal headerDpr=headerImage.devicePixelRatio();const QColor headerCorner=QColor::fromRgba(headerImage.pixel(qRound(1*headerDpr),qRound(4*headerDpr)));const QColor cardSurface=Theme::color("surface",false);check(qAbs(headerCorner.red()-cardSurface.red())<5 && qAbs(headerCorner.green()-cardSurface.green())<5,"header outer corners blend into the card instead of a native grey rectangle");
        check(tree->isAnimated(),"candidate opening and closing animations enabled");
        QTreeWidgetItem *root=deviceItem(tree);check(root!=0,"demo driver row exists");
        check(root->parent() && root->parent()->data(0,Qt::UserRole+13).toBool(),"devices are nested below a distinct collapsible category header");root->parent()->setExpanded(false);catalog->setSelected(0,true);QTest::qWait(40);root=deviceItem(tree);check(!root->parent()->isExpanded() && catalog->selectedDrivers()==1,"category collapse survives selection updates without clearing driver choices");root->parent()->setExpanded(true);catalog->setSelected(0,false);QTest::qWait(40);root=deviceItem(tree);
        bool handCursors=true;for(QAbstractButton *button:workspaceCommands)if(button->isEnabled() && button->cursor().shape()!=Qt::PointingHandCursor)handCursors=false;check(handCursors,"enabled workspace buttons consistently use the hand cursor");
        tree->setProperty("hoverRow",0);tree->setProperty("hoverCandidate",-1);tree->viewport()->repaint();const QImage roundedRow=tree->viewport()->grab().toImage();const qreal rowDpr=roundedRow.devicePixelRatio();const int rowTop=tree->visualItemRect(root).top();auto rowPixel=[&](int x,int y){return QColor::fromRgba(roundedRow.pixel(qRound(x*rowDpr),qRound(y*rowDpr)));};const QColor rowCorner=rowPixel(3,rowTop+4),rowCenter=rowPixel(tree->viewport()->width()/2,rowTop+5);check(qAbs(rowCorner.red()-cardSurface.red())<5 && qAbs(rowCorner.green()-cardSurface.green())<5 && (qAbs(rowCenter.red()-rowCorner.red())>5 || qAbs(rowCenter.green()-rowCorner.green())>5),"full-row hover has rounded outer corners while filling the center continuously");tree->setProperty("hoverRow",-1);tree->viewport()->update();
        catalog->toggleIgnored(0);check(catalog->isIgnored(0) && !catalog->canQueue(0),"ignored hardware cannot be queued");
        testSettings->sync();check(testSettings->value(QStringLiteral("ignoredHardwareIds")).toStringList().contains(device.hardwareId),"ignored hardware preferences are persisted");
        QTest::qWait(20);check(tree->topLevelItemCount()==0,"ignored hardware is hidden from the default list");
        catalog->toggleIgnored(0);catalog->selectAll(true);QTest::qWait(20);root=deviceItem(tree);
        check(root && catalog->canQueue(0),"ignored hardware can be restored without rescanning");
        const QPersistentModelIndex editingIndex(tree->model()->index(root->parent()->indexOfChild(root),3,tree->model()->index(tree->indexOfTopLevelItem(root->parent()),0)));
        root->setCheckState(3,Qt::Unchecked);
        check(editingIndex.isValid(),"selection must not delete an item while its check event is on the stack");
        QTest::qWait(20);root=deviceItem(tree);
        catalog->setSelected(0,true);QTest::qWait(40);check(workspace->findChild<GradientButton*>()->text()==QStringLiteral("Install (1)"),"install action counts one driver without counting the restore point");catalog->setSelected(0,false);QTest::qWait(20);
        for(int i=0;i<200;++i){
            root=deviceItem(tree);tree->setCurrentItem(root,3);
            QTest::keyClick(tree,Qt::Key_Space);QTest::qWait(1);
        }
        check(catalog->selectedDrivers()==0,"200 real checkbox keyboard toggles remain stable and consistent");
        QPushButton *selectAll=0,*selectNone=0;
        for(QPushButton *button:workspace->findChildren<QPushButton*>()){
            if(button->text()==QStringLiteral("Select all"))selectAll=button;
            if(button->text()==QStringLiteral("Select none"))selectNone=button;
        }
        check(selectAll && selectNone,"bulk selection controls are available for regression test");
        for(int i=0;i<100;++i){selectAll->click();selectNone->click();QTest::qWait(1);}
        check(catalog->selectedDrivers()==0,"100 select-all and clear cycles remain stable");
        QLineEdit *selectionSearch=workspace->findChild<QLineEdit*>(QStringLiteral("searchField"));selectionSearch->setText(QStringLiteral("NO-SUCH-DEVICE"));QTest::qWait(180);selectAll->click();QTest::qWait(30);check(tree->topLevelItemCount()==0 && catalog->selectedDrivers()==1 && catalog->entryRef(0).chosenCandidate==1,"select-all chooses Recommended even when search hides the device");
        QMetaObject::invokeMethod(&window,"navigate",Q_ARG(int,1));QTest::qWait(170);QMetaObject::invokeMethod(&window,"navigate",Q_ARG(int,0));QTest::qWait(170);selectNone->click();QTest::qWait(30);check(catalog->selectedDrivers()==0,"clear-all removes hidden marks after leaving and returning to the page");selectionSearch->clear();QTest::qWait(180);
        for(int i=0;i<100;++i){
            root=deviceItem(tree);
            const int checkX=tree->header()->sectionViewportPosition(3)+12;
            QTest::mouseClick(tree->viewport(),Qt::LeftButton,Qt::NoModifier,QPoint(checkX,tree->visualItemRect(root).center().y()));QTest::qWait(1);
        }
        check(catalog->selectedDrivers()==0,"100 real checkbox mouse toggles remain stable and consistent");
        bool alternativesAccepted=true;
        for(int i=0;i<60;++i){alternativesAccepted=catalog->chooseCandidate(0,i%2+1) && alternativesAccepted;QTest::qWait(1);}
        check(alternativesAccepted,"repeated alternative selection accepted");
        check(catalog->entryRef(0).chosenCandidate==2,"alternative selection remains consistent after repeated changes");
        catalog->selectAll(true);QTest::qWait(20);root=deviceItem(tree);
        QTest::mouseClick(tree->viewport(),Qt::LeftButton,Qt::NoModifier,tree->visualItemRect(root).topLeft()+QPoint(80,25));QTest::qWait(350);
        check(root->isExpanded() && root->childCount()>0,"first expansion populates candidates before animation");
        const QPoint branchPoint(tree->visualItemRect(root).left()-tree->indentation()/2,tree->visualItemRect(root).center().y());QTest::mouseClick(tree->viewport(),Qt::LeftButton,Qt::NoModifier,branchPoint);QTest::qWait(40);check(!root->isExpanded() && tree->property("branchAnimationRunning").toBool(),"one branch click closes the list and starts its finite chevron rotation");QTest::qWait(200);QTest::mouseClick(tree->viewport(),Qt::LeftButton,Qt::NoModifier,branchPoint);QTest::qWait(220);check(root->isExpanded() && !tree->property("branchAnimationRunning").toBool(),"one branch click reopens the list and the arrow timer stops after settling");workspace->setAnimations(false);root->setExpanded(false);check(!tree->isAnimated() && !tree->property("branchAnimationRunning").toBool(),"reduced motion takes the immediate list and arrow path");workspace->setAnimations(true);root->setExpanded(true);QTest::qWait(220);
        catalog->setSelected(0,false);QTest::qWait(40);root=deviceItem(tree);QTreeWidgetItem *alternativeRow=0;for(int c=0;c<root->childCount();++c)if(root->child(c)->data(0,Qt::UserRole+1).toInt()>0){alternativeRow=root->child(c);break;}if(!alternativeRow)throw std::runtime_error("No eligible visible alternative in the click fixture");const int wideChoice=alternativeRow->data(0,Qt::UserRole+1).toInt();const QRect alternativeRect=tree->visualItemRect(alternativeRow);QTest::mouseClick(tree->viewport(),Qt::LeftButton,Qt::NoModifier,QPoint(tree->header()->sectionViewportPosition(1)+tree->columnWidth(1)/2,alternativeRect.center().y()));QTest::qWait(40);check(catalog->entryRef(0).chosenCandidate==wideChoice && catalog->selectedDrivers()==1,"clicking an alternative's status cell selects its radio choice");root=deviceItem(tree);QTreeWidgetItem *sameChoice=0;for(int c=0;c<root->childCount();++c)if(root->child(c)->data(0,Qt::UserRole+1).toInt()==wideChoice)sameChoice=root->child(c);if(!sameChoice)throw std::runtime_error("Selected row not visible");const QPoint choicePoint(tree->header()->sectionViewportPosition(1)+tree->columnWidth(1)/2,tree->visualItemRect(sameChoice).center().y());QTest::mouseClick(tree->viewport(),Qt::LeftButton,Qt::NoModifier,choicePoint);QTest::qWait(40);check(catalog->selectedDrivers()==0,"clicking the selected version row again clears its radio and mark");
        catalog->chooseCandidate(0,wideChoice);QTest::qWait(40);QTest::mouseClick(tree->viewport(),Qt::LeftButton,Qt::NoModifier,choicePoint);QTest::qWait(20);bool doubleDetails=false;QObject detailProbe;QTimer::singleShot(80,&detailProbe,[&](){for(QWidget *top:QApplication::topLevelWidgets())if(top->objectName()==QStringLiteral("driverDetailsDialog")){doubleDetails=true;top->close();return;}});QTest::mouseDClick(tree->viewport(),Qt::LeftButton,Qt::NoModifier,choicePoint);QTest::mouseRelease(tree->viewport(),Qt::LeftButton,Qt::NoModifier,choicePoint);QTest::qWait(100);check(doubleDetails && catalog->selectedDrivers()==1,"double-click opens details after a selection rebuild and preserves the pre-click choice");
        catalog->chooseCandidate(0,2);QTest::qWait(40);root=deviceItem(tree);
        bool menuReady=false,menuHoverVisible=false;
        QTimer menuProbe;menuProbe.setInterval(30);QObject::connect(&menuProbe,&QTimer::timeout,[&](){QMenu *menu=qobject_cast<QMenu*>(app.activePopupWidget());if(!menu)for(QWidget *top:QApplication::topLevelWidgets())if(QMenu *candidate=qobject_cast<QMenu*>(top))if(candidate->isVisible()){menu=candidate;break;}if(menu){menuProbe.stop();menuReady=menu->actions().size()>=8;menu->setActiveAction(menu->actions().first());QTest::mouseMove(menu,menu->actionGeometry(menu->actions().first()).center());QTest::qWait(150);menu->setActiveAction(menu->actions().first());QTest::qWait(160);menu->repaint();const QImage menuImage=menu->grab().toImage();menuImage.save(QDir(app.applicationDirPath()).filePath(QStringLiteral("actions-menu.png")));const qreal ratio=menuImage.width()/qreal(menu->width());const QRect first=menu->actionGeometry(menu->activeAction()?menu->activeAction():menu->actions().first());const QColor hoverPixel=QColor::fromRgba(menuImage.pixel(qRound((first.left()+8)*ratio),qRound(first.center().y()*ratio)));menuHoverVisible=hoverPixel.green()>hoverPixel.red()+8 && hoverPixel.blue()>hoverPixel.red()+5;menu->close();}});menuProbe.start();
        const QPoint dots(tree->header()->sectionViewportPosition(3)+tree->columnWidth(3)-16,tree->visualItemRect(root).center().y());
        QTest::mouseClick(tree->viewport(),Qt::LeftButton,Qt::NoModifier,dots);
        check(menuReady,"three-dot button opens per-driver actions");
        check(menuHoverVisible,"project action menu shows its hover background behind clear text");QTest::qWait(60);window.activateWindow();QApplication::setActiveWindow(&window);QTest::qWait(40);
        window.grab().save(QDir(app.applicationDirPath()).filePath(QStringLiteral("workspace.png")));
        const QList<QCheckBox*> checksList=workspace->findChildren<QCheckBox*>();QCheckBox *expert=0;
        for(QCheckBox *box:checksList)if(box->text()==QStringLiteral("Expert mode"))expert=box;
        check(expert!=0,"expert option exists");
        for(QToolButton *tool:workspace->findChildren<QToolButton*>())if(tool->text()==QStringLiteral("Match filters"))tool->click();
        QTest::qWait(200);if(!expert->isVisible()){window.activateWindow();QApplication::setActiveWindow(&window);QTest::qWait(60);for(QToolButton *tool:workspace->findChildren<QToolButton*>())if(tool->text()==QStringLiteral("Match filters"))tool->click();QTest::qWait(200);}check(expert->isVisible(),"advanced view options are accessible inside filters");
        QTest::mouseMove(expert,QPoint(expert->width()-4,expert->height()/2));QTest::qWait(220);
        HoverSurface *hover=expert->findChild<HoverSurface*>();check(!hover || hover->amount()==0,"checkbox text does not receive hover highlight");
        expert->window()->grab().save(QDir(app.applicationDirPath()).filePath(QStringLiteral("filters.png")));
        expert->window()->hide();
        sdiomodern::InstallReport operation;sdiomodern::Result operationResult;operationResult.nativeError=5;operationResult.outcome=sdiomodern::Outcome::Failure;operation.items.push_back(operationResult);
        const QJsonObject exported=supportReport(*catalog,operation,QStringLiteral("test failure"));
        check(exported.value(QStringLiteral("devices")).toArray().size()==1,"support export includes actual catalog devices");
        check(exported.value(QStringLiteral("lastInstallation")).toObject().value(QStringLiteral("items")).toArray().size()==1,"support export contains per-device operation results");
        check(!exported.value(QStringLiteral("automaticUpload")).toBool(),"support export remains local");
        DriverEntry many=catalog->entry(0);for(int i=0;i<80;++i)many.candidates.append(many.candidates[1]);
        DriverDetailsDialog details(many,true,false,&window);details.show();QTest::qWait(220);
        const QImage detailImage=details.grab().toImage();detailImage.save(QDir(app.applicationDirPath()).filePath(QStringLiteral("details.png")));
        QComboBox *picker=details.findChild<QComboBox*>(QStringLiteral("candidatePicker"));
        check(picker && picker->count()==many.candidates.size()-1,"all alternative versions accessible in picker");
        QTabWidget *fullTabs=details.findChild<QTabWidget*>();QTabBar *fullBar=details.findChild<QTabBar*>();check(fullBar && qAbs(fullBar->width()-fullTabs->width())<=2 && qAbs(fullBar->tabRect(0).united(fullBar->tabRect(2)).width()-fullBar->width())<=4 && !fullTabs->tabIcon(0).isNull() && !fullTabs->tabIcon(1).isNull() && !fullTabs->tabIcon(2).isNull(),"detail tabs fill the available width and all have vector icons");
        QLabel *identityValue=details.findChild<QLabel*>(QStringLiteral("hardwareIdentityValue"));check(identityValue && identityValue->font().weight()==QFont::Normal,"device identity values use regular body type instead of heading weight");
        check(details.findChild<QTabWidget*>()->count()==3,"comparison separates overview, hardware, engine metadata");
        picker->setCurrentIndex(70);QTest::qWait(20);
        picker->showPopup();QTest::qWait(100);
        QLineEdit *versionSearch=picker->findChild<QLineEdit*>(QStringLiteral("languageSearch"));
        check(versionSearch!=0,"version picker uses the searchable project popup");
        QWidget *versionPopup=app.activePopupWidget();check(versionPopup && versionPopup->findChild<QListWidget*>(QStringLiteral("languageChoices")),"version choices use the project list rather than a native combo window");
        versionPopup->grab().save(QDir(app.applicationDirPath()).filePath(QStringLiteral("version-list.png")));
        details.grab().save(QDir(app.applicationDirPath()).filePath(QStringLiteral("version-picker.png")));
        QTest::keyClick(versionSearch,Qt::Key_Escape);picker->hidePopup();QTest::qWait(20);
        check(details.findChildren<QLabel*>().size()<65,"candidate switch keeps bounded detail widgets for large catalogs");
        check(qAlpha(detailImage.pixel(0,0))<30,"comparison dialog has smooth transparent corners");details.close();
        QMetaObject::invokeMethod(&window,"beginInstallation");QTest::qWait(70);
        InstallationWorkspace *installation=window.findChild<InstallationWorkspace*>();
        check(pages->currentIndex()==4 && installation->isActive(),"demo enters dedicated installation stage");
        check(sidebar->isHidden(),"navigation is hidden during installation");
        const QList<QToolButton*> titleButtons=window.findChild<TitleBar*>()->findChildren<QToolButton*>();
        for(QToolButton *button:titleButtons)if(button->objectName()==QStringLiteral("titleAction"))check(!button->isEnabled(),"installation locks toolbar actions");
        QMetaObject::invokeMethod(&window,"navigate",Q_ARG(int,3));check(pages->currentIndex()==4,"active installation cannot navigate away");
        QTest::qWait(180);window.grab().save(QDir(app.applicationDirPath()).filePath(QStringLiteral("installation.png")));
        installation->stopDemo();QMetaObject::invokeMethod(&window,"navigate",Q_ARG(int,0));
        check(!sidebar->isHidden(),"navigation returns after leaving completed stage");

        installation->beginNative(QStringList()<<QStringLiteral("TEST ONLY"),QStringList()<<QStringLiteral("TEST-ID"),true);
        sdiomodern::Event event;event.stage=sdiomodern::Stage::RestorePoint;installation->applyNativeEvent(event);
        QLabel *activeStep=installation->findChild<QLabel*>(QStringLiteral("activeStage"));check(activeStep && activeStep->text().startsWith(QStringLiteral("01")),"restore stage follows real engine event");
        event.stage=sdiomodern::Stage::Install;installation->applyNativeEvent(event);activeStep=installation->findChild<QLabel*>(QStringLiteral("activeStage"));
        check(activeStep && activeStep->text().startsWith(QStringLiteral("03")),"installation stage follows real engine event");
        installation->finishNative(operation);activeStep=installation->findChild<QLabel*>(QStringLiteral("activeStage"));
        check(activeStep && activeStep->text().startsWith(QStringLiteral("04")),"completion stage follows actual report");
        window.close();
        sdiomodern::Snapshot nativeFixture;nativeFixture.generation=8;nativeFixture.sourceRevision=L"UI-TEST-ENGINE";nativeFixture.capabilities.nativeCore=true;nativeFixture.capabilities.matching=true;nativeFixture.capabilities.localInstall=true;
        sdiomodern::Device nativeDevice;nativeDevice.name=L"NATIVE MODEL TEST - NOT A REAL DEVICE";nativeDevice.instanceId=L"TEST-ID";nativeDevice.hasInstalledDriver=true;nativeDevice.installed.version=L"1.0";
        for(int i=1;i<=12;++i){sdiomodern::Candidate candidate;candidate.filterVisibility.assign(64,i==12?std::uint64_t(0):~std::uint64_t(0));candidate.token=i;candidate.originalOrder=i;candidate.valid=true;candidate.local=true;candidate.recommended=i==1;candidate.statusBits=0x20;candidate.rawScore=0xff2000+i;candidate.version=L"2.0";candidate.hardwareId=L"TEST-HARDWARE";candidate.packageName=L"DP_Test.7z";candidate.inf=L"fixture.inf";nativeDevice.candidates.push_back(candidate);}
        nativeFixture.devices.push_back(nativeDevice);DriverCatalog nativeCatalog;nativeCatalog.replaceNative(nativeFixture);
        sdiomodern::Snapshot reinstallFixture=nativeFixture;reinstallFixture.devices[0].hardwareIds.push_back(L"TEST-HARDWARE");reinstallFixture.devices[0].candidates.resize(1);reinstallFixture.devices[0].candidates[0].statusBits=0x041;reinstallFixture.devices[0].candidates[0].version=L"1.0";DriverCatalog reinstallCatalog;reinstallCatalog.replaceNative(reinstallFixture);
        reinstallCatalog.selectAll(true);check(reinstallCatalog.selectedDrivers()==1,"bulk selection follows an eligible engine recommendation even when it is a reinstall");
        check(reinstallCatalog.chooseCandidate(0,0) && reinstallCatalog.selectedReinstalls()==1 && driverStatus(reinstallCatalog.entryRef(0))==QStringLiteral("Reinstall"),"installed row resolves to a matching local native reinstall candidate");
        DriverEntry differentCurrent=reinstallCatalog.entry(0);differentCurrent.candidates[1].version=QStringLiteral("2.0");check(!isReinstallCandidate(differentCurrent,1),"a different version with the same driver date is not mislabeled as a reinstall");
        const auto reinstallPlan=reinstallCatalog.buildPlan(false,true,true);check(sdiomodern::validatePlan(reinstallFixture,reinstallPlan).size()==1 && reinstallPlan.selections[0].candidateToken==1,"reinstallation reaches the normal validated engine plan with the exact local token");
        reinstallFixture.devices[0].candidates[0].local=false;reinstallCatalog.replaceNative(reinstallFixture);check(!reinstallCatalog.chooseCandidate(0,0),"installed metadata without its local archive cannot be reinstalled");
        InstallationWorkspace reinstallPreview;reinstallPreview.beginNative(QStringList()<<QStringLiteral("Reinstall · TEST"),QStringList()<<QStringLiteral("REINSTALL-ID"),false);sdiomodern::Event reinstallEvent;reinstallEvent.stage=sdiomodern::Stage::Install;reinstallEvent.outcome=sdiomodern::Outcome::Running;reinstallEvent.deviceId=L"REINSTALL-ID";reinstallPreview.applyNativeEvent(reinstallEvent);QTreeWidget *reinstallQueue=reinstallPreview.findChild<QTreeWidget*>(QStringLiteral("installQueue"));check(reinstallQueue && reinstallQueue->topLevelItem(0)->text(1)==QStringLiteral("Reinstalling"),"active queue distinguishes reinstalling from first installation");sdiomodern::InstallReport reinstallReport;sdiomodern::Result reinstallResult;reinstallResult.deviceId=L"REINSTALL-ID";reinstallResult.outcome=sdiomodern::Outcome::Success;reinstallReport.items.push_back(reinstallResult);reinstallPreview.finishNative(reinstallReport);check(reinstallQueue->topLevelItem(0)->text(1)==QStringLiteral("Reinstalled"),"reinstall completion keeps its operation label");
        check(nativeCatalog.chooseCandidate(0,1),"automation fixture has one explicitly selected driver");
        nativeCatalog.chooseCandidate(0,12);nativeCatalog.selectAll(true);check(nativeCatalog.entryRef(0).chosenCandidate==1 && nativeCatalog.selectedDrivers()==1,"select-all replaces a manual alternative with the engine Recommended candidate");
        sdiomodern::Snapshot noRecommendation=nativeFixture;noRecommendation.devices[0].candidates.resize(1);noRecommendation.devices[0].candidates[0].recommended=false;DriverCatalog noRecommendationCatalog;noRecommendationCatalog.replaceNative(noRecommendation);noRecommendationCatalog.chooseCandidate(0,1);noRecommendationCatalog.selectAll(true);check(noRecommendationCatalog.selectedDrivers()==0,"select-all does not retain a non-recommended manual choice when no recommendation exists");
        const QJsonObject exportedPlan=automationPlan(nativeCatalog,false);
        const sdiomodern::InstallPlan resolvedPlan=resolveAutomationPlan(nativeFixture,exportedPlan,false);
        check(!resolvedPlan.confirmed && !resolvedPlan.createRestorePoint && resolvedPlan.selections.size()==1 && resolvedPlan.selections[0].candidateToken==1,"automation descriptor validation never implies installation consent");
        QJsonObject changedPlan=exportedPlan;QJsonArray changedSelections=changedPlan.value(QStringLiteral("selections")).toArray();QJsonObject changedSelection=changedSelections[0].toObject();changedSelection.insert(QStringLiteral("version"),QStringLiteral("changed"));changedSelections[0]=changedSelection;changedPlan.insert(QStringLiteral("selections"),changedSelections);
        bool rejectedAutomation=false;try{resolveAutomationPlan(nativeFixture,changedPlan,false);}catch(const std::exception&){rejectedAutomation=true;}check(rejectedAutomation,"automation rejects a changed driver identity");
        changedPlan=exportedPlan;changedSelections=changedPlan.value(QStringLiteral("selections")).toArray();changedSelections.append(changedSelections[0]);changedPlan.insert(QStringLiteral("selections"),changedSelections);rejectedAutomation=false;try{resolveAutomationPlan(nativeFixture,changedPlan,false);}catch(const std::exception&){rejectedAutomation=true;}check(rejectedAutomation,"automation rejects duplicate device selections");
        sdiomodern::Snapshot previewFixture=nativeFixture;previewFixture.capabilities.localInstall=false;rejectedAutomation=false;try{resolveAutomationPlan(previewFixture,exportedPlan,false);}catch(const std::exception&){rejectedAutomation=true;}check(rejectedAutomation,"automation rejects read-only compatibility preview results");
        check(nativeCatalog.entryRef(0).candidates[1].matchStars==5 && nativeCatalog.entryRef(0).candidates[12].matchStars==1,"copied native candidates receive derived relative priority");
        check(nativeCatalog.entryRef(0).candidates[12].rawScore==nativeFixture.devices[0].candidates[11].rawScore,"native score copied exactly without star conversion changing it");
        DriverWorkspace nativeBoard(&nativeCatalog);nativeBoard.setDemo(false);nativeBoard.setStyleSheet(Theme::styleSheet(false));nativeBoard.resize(1100,760);nativeBoard.show();QTest::qWait(50);
        QTreeWidget *nativeTree=nativeBoard.findChild<QTreeWidget*>(QStringLiteral("driverTree"));QTreeWidgetItem *nativeRoot=deviceItem(nativeTree);
        check(nativeRoot && nativeRoot->data(2,Qt::UserRole+2).toInt()==5,"native device row displays actual chosen candidate priority");
        QTest::mouseClick(nativeTree->viewport(),Qt::LeftButton,Qt::NoModifier,nativeTree->visualItemRect(nativeRoot).topLeft()+QPoint(85,25));QTest::qWait(300);
        nativeBoard.grab().save(QDir(app.applicationDirPath()).filePath(QStringLiteral("native-priority.png")));
        QCheckBox *smartView=nativeBoard.findChild<QCheckBox*>(QStringLiteral("recommendedView"));
        check(smartView && smartView->isChecked() && nativeRoot->childCount()==2,"recommended view defaults to the eligible recommendation plus installed metadata");
        smartView->setChecked(false);QTest::qWait(40);nativeRoot=deviceItem(nativeTree);
        check(nativeRoot->childCount()==13,"all twelve native candidates plus installed metadata are shown without the device filter hiding versions");QCheckBox *versionFilter=0;for(QCheckBox *box:nativeBoard.findChildren<QCheckBox*>())if(box->text()==QStringLiteral("Filter alternative versions"))versionFilter=box;check(versionFilter && !versionFilter->isChecked(),"alternative-version filtering is explicit and off by default");
        versionFilter->setChecked(true);QTest::qWait(40);nativeRoot=deviceItem(nativeTree);check(nativeRoot->childCount()==12,"opting into version filtering applies the original visibility mask without losing the full catalog");versionFilter->setChecked(false);QTest::qWait(40);nativeRoot=deviceItem(nativeTree);
        nativeBoard.applyLegacyDisplay(0,false,false);check(!smartView->isChecked() && versionFilter->isChecked(),"explicit legacy filters switch out of recommended view");
        smartView->setChecked(true);check(!versionFilter->isChecked(),"returning to recommended view clears custom version filtering");smartView->setChecked(false);QTest::qWait(30);nativeRoot=deviceItem(nativeTree);
        DriverDetailsDialog archiveDetails(nativeCatalog.entry(0),false,false,&window);archiveDetails.show();QTest::qWait(30);bool archiveShown=false;for(QLabel *label:archiveDetails.findChildren<QLabel*>())if(label->text()==QStringLiteral("DP_Test.7z"))archiveShown=true;check(archiveShown,"comparison details identify the actual 7z package");archiveDetails.close();
        check(VectorIcons::forDeviceClass(QStringLiteral("System Intel Smart Sound"))==QStringLiteral("audio") && VectorIcons::forDeviceClass(QStringLiteral("Camera"))==QStringLiteral("camera") && VectorIcons::forDeviceClass(QStringLiteral("Display"))==QStringLiteral("monitor"),"device icons distinguish sound, camera and graphics components");
        reinstallFixture.devices[0].candidates[0].local=true;reinstallCatalog.replaceNative(reinstallFixture);reinstallCatalog.chooseCandidate(0,0);
        DriverWorkspace reinstallBoard(&reinstallCatalog);reinstallBoard.setStyleSheet(Theme::styleSheet(false));reinstallBoard.resize(1000,760);reinstallBoard.show();QTest::qWait(40);
        check(reinstallBoard.createRestorePoint() && reinstallCatalog.buildPlan(reinstallBoard.createRestorePoint(),true,true).createRestorePoint,"reinstallation uses the same required-restore-point engine plan");
        QWidget *notice=reinstallBoard.findChild<QWidget*>(QStringLiteral("reinstallNotice"));QLabel *noticeText=reinstallBoard.findChild<QLabel*>(QStringLiteral("reinstallNoticeText"));
        check(notice && notice->isVisible() && notice->height()>=68 && noticeText && noticeText->text().contains(QStringLiteral("restore point")),"reinstall selection has a padded card with the actual restore requirement");
        reinstallBoard.grab().save(QDir(app.applicationDirPath()).filePath(QStringLiteral("reinstall-notice.png")));
        reinstallBoard.setInstallationPreferences(false,false);check(noticeText->text().contains(QStringLiteral("off")),"reinstall notice reflects disabling the restore-point requirement");
        QTest::qWait(3100);check(!notice->isVisible(),"reinstall warning expires after three seconds");reinstallBoard.setInstallationPreferences(false,false);check(!notice->isVisible(),"ordinary refreshes do not re-show an expired reinstall notice");
        reinstallCatalog.selectAll(false);QTest::qWait(30);check(!notice->isVisible(),"reinstall notice clears with the selection");reinstallBoard.close();
        smartView->setChecked(true);QTest::qWait(30);nativeCatalog.chooseCandidate(0,12);QTest::qWait(30);nativeRoot=deviceItem(nativeTree);
        bool selectedVisible=false;for(int i=0;i<nativeRoot->childCount();++i)if(nativeRoot->child(i)->data(0,Qt::UserRole+1).toInt()==12)selectedVisible=true;
        check(selectedVisible,"recommended view keeps a manually selected alternative visible");smartView->setChecked(false);QTest::qWait(30);
        check(nativeCatalog.chooseCandidate(0,12),"native alternative can be explicitly selected in model test");QTest::qWait(50);
        check(deviceItem(nativeTree)->data(2,Qt::UserRole+2).toInt()==1,"device row priority follows new selection rather than old recommendation");
        sdiomodern::Snapshot longFixture=nativeFixture;longFixture.devices.clear();
        for(int i=0;i<48;++i){sdiomodern::Device device=nativeDevice;device.name=L"SCROLL FIXTURE "+std::to_wstring(i);device.instanceId=L"SCROLL-ID-"+std::to_wstring(i);longFixture.devices.push_back(device);}
        DriverCatalog longCatalog;longCatalog.replaceNative(longFixture);DriverWorkspace longBoard(&longCatalog);longBoard.setDemo(false);longBoard.setAnimations(false);longBoard.resize(1080,650);longBoard.show();QTest::qWait(40);
        QTreeWidget *longTree=longBoard.findChild<QTreeWidget*>(QStringLiteral("driverTree"));longTree->scrollToItem(deviceItem(longTree,30),QAbstractItemView::PositionAtCenter);QTest::qWait(30);
        const int beforeSelection=longTree->verticalScrollBar()->value();QTreeWidgetItem *scrolledDevice=deviceItem(longTree,30);
        const int selectX=longTree->header()->sectionViewportPosition(3)+12;
        QTest::mouseClick(longTree->viewport(),Qt::LeftButton,Qt::NoModifier,QPoint(selectX,longTree->visualItemRect(scrolledDevice).center().y()));QTest::qWait(80);
        check(beforeSelection>1000 && longCatalog.entryRef(30).checked && qAbs(longTree->verticalScrollBar()->value()-beforeSelection)<=2,"selecting a driver far down the list preserves its scroll position after rebuilding");
        bool stableScroll=true;for(int i=0;i<12;++i){longCatalog.setSelected(30,i%2==0);QTest::qWait(15);stableScroll=stableScroll && qAbs(longTree->verticalScrollBar()->value()-beforeSelection)<=2;}
        check(stableScroll,"repeated selection changes do not jump a scrolled list back to the top");
        check(longTree->indentation()==36,"branch gutter gives chevrons an inset inside the rounded surface");longBoard.close();
        sdiomodern::Snapshot missingFixture=nativeFixture;missingFixture.devices[0].candidates.resize(1);missingFixture.devices[0].candidates[0].local=false;
        DriverCatalog missingCatalog;missingCatalog.replaceNative(missingFixture);
        check(missingCatalog.entryRef(0).candidates[1].matchStars==5,"missing package retains its real match priority");
        DriverWorkspace missingBoard(&missingCatalog);QTreeWidget *missingTree=missingBoard.findChild<QTreeWidget*>(QStringLiteral("driverTree"));
        check(!deviceItem(missingTree),"recommended view hides devices whose only recommendation needs an absent package");
        missingBoard.findChild<QCheckBox*>(QStringLiteral("recommendedView"))->setChecked(false);check(deviceItem(missingTree),"turning off recommended view exposes unavailable matches for inspection");
        check(!missingCatalog.canQueue(0) && !missingCatalog.chooseCandidate(0,1),"missing package cannot enter the installer queue");
        OnlinePackages::Catalog onlineCatalog;onlineCatalog.name=QStringLiteral("SDIO_Update");onlineCatalog.pieceLength=16384;onlineCatalog.total=12000;
        OnlinePackages::File indexFile;indexFile.path=QStringLiteral("indexes/SDIO/DP_Test.bin");indexFile.targetName=QStringLiteral("_P_Test.bin");indexFile.kind=OnlinePackages::Index;indexFile.size=2000;onlineCatalog.files.append(indexFile);
        OnlinePackages::File packFile;packFile.path=QStringLiteral("drivers/DP_Missing.7z");packFile.targetName=QStringLiteral("DP_Missing.7z");packFile.kind=OnlinePackages::Driver;packFile.offset=2000;packFile.size=10000;onlineCatalog.files.append(packFile);
        OnlineDialog online(QDir(app.applicationDirPath()).filePath(QStringLiteral("test-drivers")),QDir(app.applicationDirPath()).filePath(QStringLiteral("test-indexes")),QStringList()<<QStringLiteral("DP_Missing.7z"),false,&nativeBoard);
        online.setCatalogSource(OnlinePackages::officialMetadataUrl(QStringLiteral("SDI")));check(online.catalogSource().host()==QStringLiteral("sdi-tool.org"),"standalone download center accepts the saved SDI source before loading metadata");
        online.setProperty("darkMode",false);online.setStyleSheet(Theme::styleSheet(false));online.setCatalog(onlineCatalog);online.show();QTest::qWait(200);
        QTreeWidget *onlineFiles=online.findChild<QTreeWidget*>();check(onlineFiles && onlineFiles->topLevelItemCount()==2,"online dialog lists approved indexes and packages");
        for(QPushButton *button:online.findChildren<QPushButton*>())if(button->text()==QStringLiteral("Select indexes"))button->click();
        check(onlineFiles->topLevelItem(0)->checkState(0)==Qt::Checked && onlineFiles->topLevelItem(1)->checkState(0)==Qt::Unchecked,"index selection does not download driver packages");
        for(QPushButton *button:online.findChildren<QPushButton*>())if(button->text()==QStringLiteral("Packages for this scan"))button->click();
        check(onlineFiles->topLevelItem(0)->checkState(0)==Qt::Unchecked && onlineFiles->topLevelItem(1)->checkState(0)==Qt::Checked,"matching package selection uses the native package identity");
        check(!online.catalogCheckedAt().isValid(),"opening cached metadata does not postpone the next scheduled network check");
        check(onlineFiles->columnCount()==7,"download center exposes size, progress, available/local versions, relevance and status");
        check(onlineFiles->columnWidth(2)>=onlineFiles->viewport()->width()*0.16 && onlineFiles->columnWidth(0)<onlineFiles->viewport()->width()*0.42,"download table allocates a readable progress column and bounds package-name width");
        OnlineWorker *transferWorker=online.findChild<OnlineWorker*>();QMetaObject::invokeMethod(transferWorker,"activity",Q_ARG(QString,QStringLiteral("fixture download")),Q_ARG(qint64,4000),Q_ARG(qint64,10000),Q_ARG(int,0));QMetaObject::invokeMethod(transferWorker,"transferStats",Q_ARG(qint64,5000),Q_ARG(qint64,2000),Q_ARG(qint64,100),Q_ARG(int,3));QMetaObject::invokeMethod(transferWorker,"fileProgress",Q_ARG(int,1),Q_ARG(qint64,5000),Q_ARG(qint64,10000));QTest::qWait(30);
        check(online.findChild<QLabel*>(QStringLiteral("downloadSpeed"))->text().contains(QStringLiteral("/s")) && online.findChild<QLabel*>(QStringLiteral("downloadPeers"))->text()==QStringLiteral("3") && online.findChild<QLabel*>(QStringLiteral("downloadEta"))->text()!=QStringLiteral("—"),"transfer telemetry updates measured speed, peers and ETA");
        check(onlineFiles->topLevelItem(1)->text(2)==QStringLiteral("50%"),"per-file verified bytes update the row progress");
        online.findChild<QCheckBox*>(QStringLiteral("hideMissingPackages"))->setChecked(true);check(onlineFiles->topLevelItem(1)->isHidden(),"hide-missing filter removes absent packages from the list");online.findChild<QCheckBox*>(QStringLiteral("hideMissingPackages"))->setChecked(false);
        nativeBoard.setPackageUpdateStatus(7,2,QStringLiteral("SDI"));check(nativeBoard.findChild<QWidget*>(QStringLiteral("packageUpdateNotice"))->isVisible(),"available library updates have a visible driver-page notification");nativeBoard.setPackageUpdatesVisible(false);check(!nativeBoard.findChild<QWidget*>(QStringLiteral("packageUpdateNotice"))->isVisible(),"suppressed home notices stay hidden even with available updates");nativeBoard.setPackageUpdatesVisible(true);check(nativeBoard.findChild<QWidget*>(QStringLiteral("packageUpdateNotice"))->isVisible(),"explicit session results can reveal available updates again");nativeBoard.setPackageUpdateStatus(0,0,QStringLiteral("SDI"));check(!nativeBoard.findChild<QWidget*>(QStringLiteral("packageUpdateNotice"))->isVisible(),"a current library does not display a false update notification");
        online.grab().save(QDir(app.applicationDirPath()).filePath(QStringLiteral("online.png")));
        online.setProperty("darkMode",true);online.setStyleSheet(Theme::styleSheet(true));online.grab().save(QDir(app.applicationDirPath()).filePath(QStringLiteral("online-dark.png")));
        online.close();
        QTemporaryDir copyArea;check(copyArea.isValid(),"portable copy test has an isolated destination");
        const QString sourcePath=QDir(copyArea.path()).filePath(QStringLiteral("source.bin"));QFile sourceFile(sourcePath);check(sourceFile.open(QIODevice::WriteOnly),"portable fixture is writable");sourceFile.write("portable fixture bytes");sourceFile.close();
        QVector<PortableLibrary::Entry> copyFiles;PortableLibrary::Entry copyEntry;copyEntry.source=sourcePath;copyEntry.relative=QStringLiteral("indexes/SDIO/fixture.bin");copyFiles.append(copyEntry);std::atomic<bool> copyStop(false);
        const QString destination=QDir(copyArea.path()).filePath(QStringLiteral("portable"));PortableLibrary::copy(copyFiles,destination,copyStop,std::function<void(qint64,qint64)>());
        QFile copied(QDir(destination).filePath(copyEntry.relative));check(copied.open(QIODevice::ReadOnly) && copied.readAll()==QByteArray("portable fixture bytes"),"portable copy publishes verified file bytes");copied.close();
        copyFiles[0].relative=QStringLiteral("../escape.bin");bool rejectedCopy=false;try{PortableLibrary::copy(copyFiles,destination,copyStop,std::function<void(qint64,qint64)>());}catch(const std::exception&){rejectedCopy=true;}
        check(rejectedCopy && !QFile::exists(QDir(copyArea.path()).filePath(QStringLiteral("escape.bin"))),"portable copy rejects traversal without publishing outside its folder");
        nativeBoard.close();
        window.show();QTest::qWait(80);
        QComboBox *language=window.findChild<QComboBox*>(QStringLiteral("premiumCombo"));const int arabicIndex=language->findData(QStringLiteral("arabic"));
        check(arabicIndex>=0,"Arabic is available for live interface verification");language->setCurrentIndex(arabicIndex);QTest::qWait(100);
        QMetaObject::invokeMethod(&window,"navigate",Q_ARG(int,2));QTest::qWait(250);
        bool translatedLibrary=false;for(QLabel *label:pages->widget(2)->findChildren<QLabel*>())if(label->text()==QString::fromUtf8("أرشيفات التعريفات"))translatedLibrary=true;
        check(translatedLibrary,"new library cards translate immediately without restarting");
        window.grab().save(QDir(app.applicationDirPath()).filePath(QStringLiteral("packages-arabic.png")));
        QMetaObject::invokeMethod(&window,"updateDark",Q_ARG(bool,true));QMetaObject::invokeMethod(&window,"navigate",Q_ARG(int,5));QTest::qWait(250);
        window.grab().save(QDir(app.applicationDirPath()).filePath(QStringLiteral("about-arabic-dark.png")));
        {
            QComboBox *zoom=window.findChild<QComboBox*>(QStringLiteral("interfaceZoom"));
            check(zoom && zoom->property("projectCombo").toBool(),"interface zoom shares the themed project combo styling");
            check(zoom && Theme::isDark(zoom),"combo theme follows the dark application instead of the Windows palette");
            if(zoom){zoom->showPopup();QTest::qWait(180);QWidget *popup=app.activePopupWidget();
                check(popup && popup->property("darkMode").toBool(),"dark zoom popup paints a dark rounded frame");
                QListWidget *choices=popup?popup->findChild<QListWidget*>(QStringLiteral("languageChoices")):0;
                bool sameFlags=choices && choices->count()==zoom->count();for(int i=0;choices && i<choices->count();++i)sameFlags=sameFlags && bool(choices->item(i)->flags()&Qt::ItemIsEnabled)==bool(zoom->model()->flags(zoom->model()->index(i,0))&Qt::ItemIsEnabled);
                check(sameFlags,"popup preserves disabled zoom levels from the source model");
                check(popup && !popup->findChild<QLineEdit*>(QStringLiteral("languageSearch"))->isVisible(),"short percentage choices omit the unnecessary search field");
                if(popup)popup->grab().save(QDir(app.applicationDirPath()).filePath(QStringLiteral("zoom-dark-premium.png")));zoom->hidePopup();
            }
            DriverEntry sample=many;sample.sdioVerified=true;sample.sdioDriverPresent=true;sample.device.name=QStringLiteral("Thunderbolt(TM) Controller - 1134");sample.device.manufacturer=QStringLiteral("Intel(R) Corporation");sample.device.deviceClass=QStringLiteral("System devices");sample.candidates.resize(3);sample.candidates[0].version=QStringLiteral("1.41.1420.0");sample.candidates[0].status=QStringLiteral("installed");sample.candidates[1].version=QStringLiteral("1.41.1423.0");sample.candidates[1].status=QStringLiteral("recommended");sample.candidates[1].engineToken=1;sample.candidates[1].matchStars=5;sample.candidates[1].originalOrder=1;sample.candidates[1].local=true;
            bool versionStable=true,starStable=true,roundedTextMenus=true,correctMenuColors=true,selectAllWorks=true;
            for(int mode=0;mode<2;++mode)for(int rtl=0;rtl<2;++rtl){
                language->setCurrentIndex(language->findData(rtl?QStringLiteral("arabic"):QStringLiteral("english")));QMetaObject::invokeMethod(&window,"updateDark",Q_ARG(bool,mode==1));QMetaObject::invokeMethod(&window,"navigate",Q_ARG(int,5));QTest::qWait(240);
                const QString suffix=QStringLiteral("%1-%2").arg(mode?QStringLiteral("dark"):QStringLiteral("light"),rtl?QStringLiteral("arabic"):QStringLiteral("english"));
                window.grab().save(QDir(app.applicationDirPath()).filePath(QStringLiteral("premium-about-")+suffix+QStringLiteral(".png")));
                DriverDetailsDialog redesigned(sample,false,rtl,&window);redesigned.show();QTest::qWait(220);
                redesigned.grab().save(QDir(app.applicationDirPath()).filePath(QStringLiteral("premium-comparison-")+suffix+QStringLiteral(".png")));
                redesigned.findChild<QTabWidget*>()->setCurrentIndex(1);QTest::qWait(30);redesigned.grab().save(QDir(app.applicationDirPath()).filePath(QStringLiteral("premium-device-")+suffix+QStringLiteral(".png")));redesigned.findChild<QTabWidget*>()->setCurrentIndex(0);
                QLabel *identity=redesigned.findChild<QLabel*>(QStringLiteral("hardwareIdentityValue"));identity->setSelection(0,1);
                QTimer::singleShot(40,[&](){QMenu *menu=qobject_cast<QMenu*>(app.activePopupWidget());if(!menu){roundedTextMenus=false;return;}const QImage pixels=menu->grab().toImage();const qreal dpr=pixels.width()/qreal(menu->width());roundedTextMenus=roundedTextMenus && qobject_cast<ProjectMenu*>(menu) && qAlpha(pixels.pixel(0,0))<30;const QColor surface=QColor::fromRgba(pixels.pixel(qRound(menu->width()/2*dpr),qRound(5*dpr))),expected=Theme::color("surface",mode==1);correctMenuColors=correctMenuColors && qAbs(surface.red()-expected.red())<5 && qAbs(surface.green()-expected.green())<5 && qAbs(surface.blue()-expected.blue())<5;menu->actions().last()->trigger();menu->close();});
                QContextMenuEvent context(QContextMenuEvent::Mouse,QPoint(8,8),identity->mapToGlobal(QPoint(8,8)));QApplication::sendEvent(identity,&context);selectAllWorks=selectAllWorks && identity->selectedText()==identity->text();
                for(QLabel *value:redesigned.findChildren<QLabel*>(QStringLiteral("detailVersion")))versionStable=versionStable && value->property("keepLtr").toBool() && value->layoutDirection()==Qt::LeftToRight;
                QWidget *stars=redesigned.findChild<QWidget*>(QStringLiteral("comparisonStars"));starStable=starStable && stars && stars->property("matchPriority").toInt()==5;
                redesigned.close();
            }
            check(versionStable,"comparison redesign preserves left-to-right technical versions in both interface directions");
            check(starStable,"comparison stars retain the selected engine candidate's match priority");
            check(roundedTextMenus,"text context menus use rounded transparent project chrome in both languages and themes");
            check(correctMenuColors,"text context menus paint the actual light or dark surface rather than black inherited backgrounds");
            check(selectAllWorks,"project text context menus preserve select-all behavior without changing the technical value");
            language->setCurrentIndex(arabicIndex);QMetaObject::invokeMethod(&window,"updateDark",Q_ARG(bool,true));
        }
        bool cleanBranding=true;for(QLabel *label:window.findChildren<QLabel*>())if(label->text().contains(QStringLiteral("Release 17")) || label->text().contains(QStringLiteral("R17 UI UPDATE")))cleanBranding=false;
        check(cleanBranding,"local release labels are absent throughout the interface");
        language->setCurrentIndex(language->findData(QStringLiteral("english")));QMetaObject::invokeMethod(&window,"updateDark",Q_ARG(bool,false));QTest::qWait(100);
        check(UiFont::embeddedFontsAvailable(),"all six embedded font weights register successfully");
        check(QFontInfo(qApp->font()).family()==QStringLiteral("Satoshi"),"English resolves to the embedded Dreams font");
        bool stableNames=true,stableBranches=true,visibleBranches=true,stableChecks=true,correctLabels=true;
        const int selectedBefore=nativeCatalog.selectedDrivers();nativeBoard.show();nativeBoard.setAnimations(false);
        for(int cycle=0;cycle<24;++cycle){
            const bool ar=cycle%2==0;language->setCurrentIndex(language->findData(ar?QStringLiteral("arabic"):QStringLiteral("english")));nativeBoard.setStyleSheet(Theme::styleSheet(false));nativeBoard.setArabic(ar);QTest::qWait(25);UiLanguage::refresh();
            QTreeWidgetItem *item=deviceItem(nativeTree);
            stableNames=stableNames && item && item->text(0).section(QLatin1Char('\n'),0,0)==QStringLiteral("NATIVE MODEL TEST - NOT A REAL DEVICE");
            const QString expectedAction=UiLanguage::text(nativeCatalog.canQueue(0)?(nativeCatalog.entryRef(0).checked?QStringLiteral("Selected"):QStringLiteral("Select")):QStringLiteral("Details"));
            correctLabels=correctLabels && item->text(3)==expectedAction && item->text(2).contains(ar?QString::fromUtf8("إصدارات"):QStringLiteral("versions"));
            collapseDevices(nativeTree);QTest::qWait(1);
            const int x=ar?nativeTree->visualItemRect(item).right()+nativeTree->indentation()/2:nativeTree->visualItemRect(item).left()-nativeTree->indentation()/2;
            QTest::mouseClick(nativeTree->viewport(),Qt::LeftButton,Qt::NoModifier,QPoint(x,nativeTree->visualItemRect(item).center().y()));QTest::qWait(2);
            stableBranches=stableBranches && item->isExpanded() && item->childCount()>0;
            const QImage branchImage=nativeTree->viewport()->grab().toImage();const qreal scale=branchImage.width()/qreal(nativeTree->viewport()->width());const int y=nativeTree->visualItemRect(item).center().y();int ink=0;
            for(int py=qRound((y-8)*scale);py<=qRound((y+8)*scale);++py)for(int px=qRound((x-8)*scale);px<=qRound((x+8)*scale);++px){const QColor color=QColor::fromRgba(branchImage.pixel(px,py));if(color.red()<170 && color.green()<190 && color.blue()<205)++ink;}
            visibleBranches=visibleBranches && ink>=3;
            stableChecks=stableChecks && nativeCatalog.selectedDrivers()==selectedBefore;
        }
        check(stableNames,"24 live language switches preserve original English hardware names");
        check(stableBranches,"branch buttons open the alternatives after every LTR and RTL switch");
        check(visibleBranches,"branch chevrons remain visibly painted after all 24 language switches");
        check(stableChecks,"language changes and branch clicks preserve driver selections");
        check(correctLabels,"Arabic details and version counts retain their correct meanings");
        language->setCurrentIndex(arabicIndex);nativeBoard.setStyleSheet(Theme::styleSheet(false));nativeBoard.setArabic(true);QTest::qWait(40);nativeBoard.grab().save(QDir(app.applicationDirPath()).filePath(QStringLiteral("native-arabic.png")));nativeBoard.close();
        check(QFontInfo(qApp->font()).family()==QStringLiteral("Cairo"),"Arabic resolves to embedded Cairo instead of a system fallback");
        QMetaObject::invokeMethod(&window,"navigate",Q_ARG(int,5));QTest::qWait(250);UiLanguage::refresh();
        QLabel *author=window.findChild<QLabel*>(QStringLiteral("developerName"));
        check(author && author->text()==QString::fromUtf8("طارق صادق") && (author->alignment()&Qt::AlignRight) && (author->alignment()&Qt::AlignAbsolute),"Arabic author identity and alignment are explicit");
        check(window.findChild<QLabel*>(QStringLiteral("headerTitle"))->text()==QString::fromUtf8("مدير تعريفات الأجهزة"),"Arabic project identity is localized in the header");
        window.grab().save(QDir(app.applicationDirPath()).filePath(QStringLiteral("about-arabic.png")));
        QMetaObject::invokeMethod(&window,"navigate",Q_ARG(int,3));QTest::qWait(250);UiLanguage::refresh();
        check(window.findChildren<QWidget*>(QStringLiteral("settingsCard")).size()==4,"settings groups use four distinct project cards");
        {SearchableLanguageCombo shortCombo;shortCombo.addItems(QStringList()<<QStringLiteral("Automatic")<<QStringLiteral("SDI")<<QStringLiteral("SDIO"));shortCombo.resize(380,44);shortCombo.show();QTest::mouseClick(&shortCombo,Qt::LeftButton,Qt::NoModifier,QPoint(360,22));QTest::qWait(200);QWidget *popup=shortCombo.findChild<QWidget*>(QStringLiteral("languagePopup"));check(popup && popup->isVisible() && popup->height()<=160,"three-item dropdown fits its rows without an empty lower section");if(popup)popup->hide();shortCombo.close();}
        QComboBox *librarySource=window.findChild<QComboBox*>(QStringLiteral("libraryUpdateSource")),*librarySchedule=window.findChild<QComboBox*>(QStringLiteral("libraryUpdateSchedule"));QSpinBox *libraryFrequency=window.findChild<QSpinBox*>(QStringLiteral("libraryUpdateFrequency"));QDateTimeEdit *libraryDate=window.findChild<QDateTimeEdit*>(QStringLiteral("libraryUpdateDate"));
        check(librarySource && librarySource->count()==3 && librarySchedule && librarySchedule->count()==6,"settings expose independent automatic/SDI/SDIO sources and six check schedules");
        librarySchedule->setCurrentIndex(librarySchedule->findData(QStringLiteral("interval")));check(!libraryFrequency->isHidden() && libraryDate->isHidden(),"hourly schedule exposes frequency and hides date controls");libraryFrequency->setValue(12);librarySource->setCurrentIndex(librarySource->findData(QStringLiteral("SDI")));window.findChild<QPushButton*>(QStringLiteral("applyLibraryUpdateSchedule"))->click();QScopedPointer<QSettings> updatePreferences(createAppSettings(QCoreApplication::applicationDirPath(),0,false));check(updatePreferences->value(QStringLiteral("updates/hours")).toInt()==12 && updatePreferences->value(QStringLiteral("updates/source"))==QStringLiteral("SDI"),"Apply persists selected catalog and schedule without starting a demo transfer");
        librarySchedule->setCurrentIndex(librarySchedule->findData(QStringLiteral("date")));check(!libraryDate->isHidden() && libraryFrequency->isHidden(),"date schedule exposes local date/time instead of frequency");
        const QDateTime scheduleNow=QDateTime::fromString(QStringLiteral("2026-10-30T12:00:00Z"),Qt::ISODate);QSettings policy(QDir(app.applicationDirPath()).filePath(QStringLiteral("update-policy-test.ini")),QSettings::IniFormat);policy.clear();policy.setValue(QStringLiteral("updates/source"),QStringLiteral("SDI"));check(UpdatePolicy::family(policy,QStringLiteral("SDIO"))==QStringLiteral("SDI"),"explicit catalog overrides auto detection");policy.setValue(QStringLiteral("updates/source"),QStringLiteral("auto"));check(UpdatePolicy::family(policy,QStringLiteral("Automatic"))==QStringLiteral("SDIO"),"standalone automatic source has a defined SDIO fallback");
        policy.setValue(QStringLiteral("updates/mode"),QStringLiteral("startup"));check(UpdatePolicy::due(policy,QStringLiteral("SDI"),scheduleNow,true) && !UpdatePolicy::due(policy,QStringLiteral("SDI"),scheduleNow,false),"startup checks run once per launch rather than every timer tick");policy.setValue(QStringLiteral("updates/mode"),QStringLiteral("interval"));policy.setValue(QStringLiteral("updates/hours"),12);UpdatePolicy::succeeded(policy,QStringLiteral("SDI"),scheduleNow.addSecs(-11*3600));check(!UpdatePolicy::due(policy,QStringLiteral("SDI"),scheduleNow,false) && UpdatePolicy::due(policy,QStringLiteral("SDI"),scheduleNow.addSecs(3600),false),"hourly checks use elapsed successful-check time");policy.setValue(UpdatePolicy::key(QStringLiteral("SDI"),"success"),scheduleNow.addSecs(-13*3600));policy.setValue(UpdatePolicy::key(QStringLiteral("SDI"),"attempt"),scheduleNow);check(!UpdatePolicy::due(policy,QStringLiteral("SDI"),scheduleNow.addSecs(60),false),"failed background checks have a bounded retry cooldown");policy.remove(UpdatePolicy::key(QStringLiteral("SDI"),"attempt"));policy.setValue(QStringLiteral("updates/mode"),QStringLiteral("launches"));policy.setValue(QStringLiteral("updates/launches"),3);policy.setValue(QStringLiteral("updates/launchesSinceCheck"),2);check(!UpdatePolicy::due(policy,QStringLiteral("SDI"),scheduleNow,true),"launch-count schedule waits for the configured threshold");policy.setValue(QStringLiteral("updates/launchesSinceCheck"),3);check(UpdatePolicy::due(policy,QStringLiteral("SDI"),scheduleNow,true) && !UpdatePolicy::due(policy,QStringLiteral("SDI"),scheduleNow,false),"launch threshold is checked at startup only");policy.setValue(QStringLiteral("updates/mode"),QStringLiteral("date"));policy.setValue(QStringLiteral("updates/date"),scheduleNow.addSecs(-60));check(UpdatePolicy::due(policy,QStringLiteral("SDI"),scheduleNow,false),"overdue scheduled date is eligible after reopening");UpdatePolicy::succeeded(policy,QStringLiteral("SDI"),scheduleNow);check(!UpdatePolicy::due(policy,QStringLiteral("SDI"),scheduleNow,false),"a completed date does not trigger repeatedly");policy.setValue(QStringLiteral("updates/mode"),QStringLiteral("manual"));check(!UpdatePolicy::due(policy,QStringLiteral("SDIO"),scheduleNow,true),"manual mode disables all automatic checks");policy.setValue(QStringLiteral("updates/mode"),QStringLiteral("never"));check(!UpdatePolicy::due(policy,QStringLiteral("SDI"),scheduleNow,true) && !UpdatePolicy::showNotice(QStringLiteral("never"),true),"Never disables automatic checks and hides even checked-session home notices");check(!UpdatePolicy::showNotice(QStringLiteral("manual"),false) && UpdatePolicy::showNotice(QStringLiteral("manual"),true),"Manual hides the notice on a fresh launch and permits explicit session results");policy.clear();policy.sync();QFile::remove(policy.fileName());

        window.grab().save(QDir(app.applicationDirPath()).filePath(QStringLiteral("settings-arabic.png")));
        QMetaObject::invokeMethod(&window,"updateDark",Q_ARG(bool,true));QTest::qWait(60);window.grab().save(QDir(app.applicationDirPath()).filePath(QStringLiteral("settings-arabic-dark.png")));
        QMetaObject::invokeMethod(&window,"updateDark",Q_ARG(bool,false));
        bool updateHeader=false,updateCompact=false;
        QTimer::singleShot(230,[&](){QDialog *dialog=qobject_cast<QDialog*>(app.activeModalWidget());if(dialog){updateHeader=dialog->findChild<QWidget*>(QStringLiteral("dialogHeader"))!=0;updateCompact=dialog->height()<350;dialog->grab().save(QDir(app.applicationDirPath()).filePath(QStringLiteral("update-arabic.png")));dialog->accept();}});
        AppUpdate::check(&window,QString());
        check(updateHeader && updateCompact,"unpublished update channel uses a compact branded dialog");
        language->setCurrentIndex(language->findData(QStringLiteral("english")));QTest::qWait(80);UiLanguage::refresh();
        window.grab().save(QDir(app.applicationDirPath()).filePath(QStringLiteral("settings-english.png")));
        QTimer::singleShot(230,[&](){QDialog *dialog=qobject_cast<QDialog*>(app.activeModalWidget());if(dialog){dialog->grab().save(QDir(app.applicationDirPath()).filePath(QStringLiteral("update-english.png")));dialog->accept();}});AppUpdate::check(&window,QString());
        check(language->findData(QStringLiteral("hebrew"))<0,"removed language is absent from the settings selector");
        QFile removedCatalog(QStringLiteral(":/sdio/i18n/hebrew.json"));check(!removedCatalog.exists(),"removed language catalog is not embedded in the executable");
        bool exclusiveLanguage=true,exclusiveVersion=true,drawerDirections=true,drawerDismiss=true,diagnosticCopy=true;
        ToolsDrawer *tools=window.findChild<ToolsDrawer*>();tools->setMotionEnabled(false);
        SearchableLanguageCombo versionFixture;versionFixture.setObjectName(QStringLiteral("candidatePicker"));for(int i=0;i<12;++i)versionFixture.addItem(QString::number(i));versionFixture.show();
        for(int cycle=0;cycle<8;++cycle){
            const bool ar=cycle%2==0;language->setCurrentIndex(language->findData(ar?QStringLiteral("arabic"):QStringLiteral("english")));QTest::qWait(35);
            language->showPopup();QTest::qWait(30);QLineEdit *search=language->findChild<QLineEdit*>(QStringLiteral("languageSearch"));exclusiveLanguage=exclusiveLanguage && search && search->placeholderText()==(ar?QString::fromUtf8("ابحث عن لغة..."):QStringLiteral("Search languages..."));language->hidePopup();
            static_cast<QComboBox*>(&versionFixture)->showPopup();QTest::qWait(30);QLineEdit *versionFilter=versionFixture.findChild<QLineEdit*>(QStringLiteral("languageSearch"));exclusiveVersion=exclusiveVersion && versionFilter->placeholderText()==(ar?QString::fromUtf8("ابحث عن إصدار..."):QStringLiteral("Search versions..."));static_cast<QComboBox*>(&versionFixture)->hidePopup();
            QMetaObject::invokeMethod(&window,"navigate",Q_ARG(int,1));QTest::qWait(250);UiLanguage::refresh();
            tools->openDrawer();QTest::qWait(30);QWidget *panel=tools->findChild<QWidget*>(QStringLiteral("toolsDrawerPanel"));drawerDirections=drawerDirections && (ar?panel->x()==12:panel->geometry().right()==tools->width()-13);
            if(cycle<2){window.grab().save(QDir(app.applicationDirPath()).filePath(ar?QStringLiteral("tools-arabic.png"):QStringLiteral("tools-english.png")));}
            QTest::mouseClick(tools,Qt::LeftButton,Qt::NoModifier,QPoint(ar?tools->width()-20:20,tools->height()/2));drawerDismiss=drawerDismiss && !tools->isOpened();
            const auto values=pages->widget(1)->findChildren<QLabel*>(QStringLiteral("diagnosticValue"));
            for(QLabel *value:values){const QString raw=value->property("rawDiagnosticValue").toString();const QString key=value->property("diagnosticKey").toString();diagnosticCopy=diagnosticCopy && value->text()==SystemProfile::displayValue(key,raw);if(ar && key==QStringLiteral("uptime"))diagnosticCopy=diagnosticCopy && value->text().contains(QString::fromUtf8("ساعة"));}
        }
        versionFixture.close();tools->setMotionEnabled(true);
        check(exclusiveLanguage,"language search shows only the selected UI language across eight switches");
        check(exclusiveVersion,"version search shows only the selected UI language across eight switches");
        check(drawerDirections,"tools drawer opens from the left in Arabic and right in English");
        check(drawerDismiss,"clicking outside closes the tools drawer in both directions");
        check(diagnosticCopy,"diagnostics translate structured live values without changing hardware data");
        UiLanguage::setLanguage(QStringLiteral("arabic"));
        check(SystemProfile::displayValue(QStringLiteral("cpu"),QStringLiteral("Intel Core (12)"))==QStringLiteral("Intel Core (12)"),"diagnostics preserve opaque device and vendor identifiers");
        check(SystemProfile::displayValue(QStringLiteral("memoryload"),QStringLiteral("35% in use"))==QString::fromUtf8("35% قيد الاستخدام"),"memory usage is localized before inserting its measured number");
        check(SystemProfile::displayValue(QStringLiteral("refresh"),QStringLiteral("144 Hz (primary display)"))==QString::fromUtf8("144 هرتز (الشاشة الرئيسية)"),"display refresh suffix follows the selected language");
        UiLanguage::setLanguage(QStringLiteral("english"));
        check(SystemProfile::displayValue(QStringLiteral("memoryload"),QStringLiteral("35% in use"))==QStringLiteral("35% in use"),"diagnostic English copy is restored without reverse translation guessing");
        language->setCurrentIndex(arabicIndex);QTest::qWait(50);QMetaObject::invokeMethod(&window,"navigate",Q_ARG(int,1));QTest::qWait(250);window.grab().save(QDir(app.applicationDirPath()).filePath(QStringLiteral("diagnostics-arabic.png")));
        check(pages->widget(1)->findChild<QWidget*>(QStringLiteral("diagnosticsHero"))!=0,"diagnostics has the project gradient header");
        QSet<QString> tones;for(QFrame *card:pages->widget(1)->findChildren<QFrame*>(QStringLiteral("diagnosticCard")))tones.insert(card->property("tone").toString());check(tones.size()==4,"diagnostic cards use distinct section accents");
        QProgressBar *usage=pages->widget(1)->findChild<QProgressBar*>(QStringLiteral("diagnosticUsage"));bool measuredBar=false;for(QLabel *value:pages->widget(1)->findChildren<QLabel*>(QStringLiteral("diagnosticValue")))if(value->property("diagnosticKey").toString()==QStringLiteral("memoryload"))measuredBar=usage && usage->value()==value->property("rawDiagnosticValue").toString().section(QLatin1Char('%'),0,0).toInt();check(measuredBar,"memory bar displays the actual measured Windows memory load");
        QMetaObject::invokeMethod(&window,"updateDark",Q_ARG(bool,true));QTest::qWait(60);window.grab().save(QDir(app.applicationDirPath()).filePath(QStringLiteral("diagnostics-arabic-dark.png")));QMetaObject::invokeMethod(&window,"updateDark",Q_ARG(bool,false));
        language->setCurrentIndex(language->findData(QStringLiteral("english")));QTest::qWait(60);window.grab().save(QDir(app.applicationDirPath()).filePath(QStringLiteral("diagnostics-english.png")));
        language->showPopup();QTest::qWait(180);QWidget *languagePopup=app.activePopupWidget();if(languagePopup)languagePopup->grab().save(QDir(app.applicationDirPath()).filePath(QStringLiteral("language-search-english.png")));language->hidePopup();
        UiLanguage::setLanguage(QStringLiteral("hebrew"));check(UiLanguage::language()==QStringLiteral("english") && qApp->layoutDirection()==Qt::LeftToRight,"an obsolete removed-language preference falls back to English");
        const Legacy::Program legacy=Legacy::parse(QByteArray("echo start\ninit\nonerror goto failed\ncmd must-not-run\n:failed\nend\n"));
        QStringList called;const int legacyCode=Legacy::execute(legacy,[&called](const Legacy::Command &command){called<<command.name;return command.name==QStringLiteral("init")?7:0;});
        check(legacyCode==7 && called==(QStringList()<<QStringLiteral("echo")<<QStringLiteral("init")),"legacy onerror/goto keeps failure status and skips unexecuted shell effects");
        bool invalidScript=false;try{Legacy::parse(QByteArray("install\nunknown-command\n"));}catch(const std::exception&){invalidScript=true;}check(invalidScript,"the entire script is validated before any install or command can run");
        invalidScript=false;try{Legacy::parse(QByteArray(":same\n:same\nend\n"));}catch(const std::exception&){invalidScript=true;}check(invalidScript,"legacy duplicate labels are rejected");
        invalidScript=false;try{const auto loop=Legacy::parse(QByteArray(":loop\ngoto loop\n"));Legacy::execute(loop,[](const Legacy::Command&){return 0;},10);}catch(const std::exception&){invalidScript=true;}check(invalidScript,"legacy backward goto has a bounded execution budget");
        const auto parameters=Legacy::parse(QByteArray("drpdir \"%1\"\necho %0\n"),QStringList()<<QStringLiteral("script.txt")<<QStringLiteral("folder with spaces"));check(parameters.commands[0].arguments.value(0)==QStringLiteral("folder with spaces") && parameters.commands[1].arguments.value(0)==QStringLiteral("script.txt"),"legacy parameters preserve quoted paths and script parameter zero");
        const auto infSuccess=sdiomodern::decodeLegacyInfResult(1,true);check(infSuccess.outcome==sdiomodern::Outcome::Success && infSuccess.nativeError==0 && infSuccess.rebootRequired,"original INF helper success code 1 maps to success and its real restart flag");
        const auto infFailure=sdiomodern::decodeLegacyInfResult(0,true);check(infFailure.outcome==sdiomodern::Outcome::Failure && infFailure.nativeError!=0 && !infFailure.rebootRequired,"original INF failure cannot be reported as success or trigger a restart");
        const auto infDenied=sdiomodern::decodeLegacyInfResult(5,false);check(infDenied.outcome==sdiomodern::Outcome::Failure && infDenied.nativeError==5,"original INF error numbers survive the compatibility boundary");
        QTemporaryDir configArea;const QString cfgPath=QDir(configArea.path()).filePath(QStringLiteral("legacy.cfg"));QFile cfg(cfgPath);cfg.open(QIODevice::WriteOnly);cfg.write("# comment\n-nogui\n-drv:\"folder with spaces\"\n");cfg.close();const auto cfgArgs=Legacy::expandConfiguration(QStringList()<<QStringLiteral("app")<<(QStringLiteral("-cfg:")+cfgPath));check(cfgArgs.contains(QStringLiteral("-nogui")) && cfgArgs.contains(QStringLiteral("-drv:folder with spaces")),"legacy CFG import preserves comments and quoted option paths");
#ifdef Q_OS_WIN
        RECT frame={};const HWND frameWindow=reinterpret_cast<HWND>(window.winId());GetWindowRect(frameWindow,&frame);const qreal windowScale=window.devicePixelRatioF();const LRESULT border=SendMessageW(frameWindow,WM_NCHITTEST,0,MAKELPARAM(frame.left+qRound(2*windowScale),frame.top+qRound(window.height()/2*windowScale)));check(border==HTLEFT,"custom window resize hit testing follows fractional interface scaling");
#endif
        check(!Legacy::describe(parameters).value(QStringLiteral("executed")).toBool(),"script validation metadata never claims system execution");
        const auto allCommands=Legacy::parse(QByteArray("init reindex\nactivetorrent 1\nselect missing newer better\ncheckupdates\nget app\nget indexes\nget driverpacks selected\nget everything\ninstall\nsnapshot test.snp\nloadsnapshot test.snp\nunloadsnapshot\nrestorepoint Test\nwritedevicelist devices.txt\nlogdir logs\ndrpdir drivers\nindexdir indexes\nextractdir extracted\ntorrentport 5555\nkeeptempfiles on\necho hello\ndebug off\nlogging on\ncmd echo test\nverbose 0\nenableinstall off\npause\nreboot ifneeded\nrunlatest\nend\n"));
        int callbacks=0;Legacy::execute(allCommands,[&callbacks](const Legacy::Command&){++callbacks;return 0;});check(callbacks==29,"all legacy operational command forms dispatch through injected effects without installing or rebooting");
        QTemporaryDir dropArea;const QString snapshotDrop=QDir(dropArea.path()).filePath(QStringLiteral("sample.snp"));QFile snapFile(snapshotDrop);snapFile.open(QIODevice::WriteOnly);snapFile.write("fixture");snapFile.close();
        check(DropTarget::classify(QList<QUrl>()<<QUrl::fromLocalFile(dropArea.path()))==DropTarget::DriverFolder,"drop recognizes one existing driver folder");
        check(DropTarget::classify(QList<QUrl>()<<QUrl::fromLocalFile(snapshotDrop))==DropTarget::SnapshotFile,"drop recognizes a bounded local snapshot");
        check(DropTarget::classify(QList<QUrl>()<<QUrl(QStringLiteral("https://example.com/sample.snp")))==DropTarget::Rejected,"drop rejects remote URLs rather than downloading or running them");
        check(DropTarget::classify(QList<QUrl>()<<QUrl::fromLocalFile(snapshotDrop)<<QUrl::fromLocalFile(dropArea.path()))==DropTarget::Rejected,"multiple dropped paths cannot change the library ambiguously");
        QFile scriptDrop(QDir(dropArea.path()).filePath(QStringLiteral("dangerous.txt")));scriptDrop.open(QIODevice::WriteOnly);scriptDrop.write("reboot\n");scriptDrop.close();check(DropTarget::classify(QList<QUrl>()<<QUrl::fromLocalFile(scriptDrop.fileName()))==DropTarget::Rejected,"drag and drop never launches legacy scripts or arbitrary files");
        QMimeData dropMime;dropMime.setUrls(QList<QUrl>()<<QUrl::fromLocalFile(dropArea.path()));QDragEnterEvent drag(QPoint(120,120),Qt::CopyAction,&dropMime,Qt::LeftButton,Qt::NoModifier);QApplication::sendEvent(&window,&drag);check(drag.isAccepted(),"window accepts an idle local driver-folder drag");
        QDropEvent drop(QPointF(120,120),Qt::CopyAction,&dropMime,Qt::LeftButton,Qt::NoModifier);QApplication::sendEvent(&window,&drop);check(drop.isAccepted(),"driver folder drop reaches the actual library-path handler");
        QScopedPointer<QSettings> droppedSettings(createAppSettings(app.applicationDirPath(),0,false));check(droppedSettings->value(QStringLiteral("driversPath")).toString()==QFileInfo(dropArea.path()).canonicalFilePath(),"folder drop saves the chosen portable library without installing anything");
        check(UiScale::fitPercent(150,QSize(1920,1080))==150,"independent interface zoom is available on a sufficiently large screen");
        const int fitted=UiScale::fitPercent(200,QSize(1024,768));check(950*fitted/100<=1000 && 640*fitted/100<=744,"interface zoom is bounded to fit the work area");
        check(UiScale::fitPercent(900,QSize(4000,4000))==200,"independent zoom rejects excessive requested scaling");
        QComboBox *interfaceZoom=window.findChild<QComboBox*>(QStringLiteral("interfaceZoom"));check(interfaceZoom && interfaceZoom->findData(125)>=0,"settings exposes independent interface sizing");
        bool maintenanceRows=false,exportRow=false;for(QToolButton *tool:tools->findChildren<QToolButton*>()){if(tool->property("toolId").toString()==QStringLiteral("reindex"))maintenanceRows=true;if(tool->property("toolId").toString()==QStringLiteral("index-report"))exportRow=true;}check(maintenanceRows && exportRow,"tools expose real index rebuilding and original text export");
        check(AppUpdate::projectRepository()==QStringLiteral("Diagoo1/Snappy-Driver-Installer-Modern"),"the public update channel is bound to the user-selected project repository");
        check(UiLanguage::available().size()==40,"all 40 UI languages are embedded without external language folders");
        const QStringList rtlLocales=QStringList()<<QStringLiteral("arabic")<<QStringLiteral("farsi");
        for(const QString &locale:rtlLocales){language->setCurrentIndex(language->findData(locale));QTest::qWait(30);UiLanguage::refresh();check(qApp->layoutDirection()==Qt::RightToLeft,"RTL language changes global widget flow");check(UiLanguage::text(QStringLiteral("Installed"))!=QStringLiteral("Installed"),"short state is translated in RTL locales");}
        language->setCurrentIndex(language->findData(QStringLiteral("french")));QTest::qWait(30);check(UiLanguage::text(QStringLiteral("Installed"))!=QStringLiteral("Installed"),"modern UI text is localized beyond Arabic and English");language->setCurrentIndex(language->findData(QStringLiteral("english")));UiLanguage::refresh();
        QMetaObject::invokeMethod(&window,"navigate",Q_ARG(int,3));QTest::qWait(180);QComboBox *targetWindows=window.findChild<QComboBox*>(QStringLiteral("targetWindows")),*targetArchitecture=window.findChild<QComboBox*>(QStringLiteral("targetArchitecture"));EngineBridge *previewBridge=window.findChild<EngineBridge*>();check(targetWindows && targetArchitecture && qAbs(targetWindows->geometry().top()-targetArchitecture->geometry().top())<=2 && targetWindows->geometry().left()!=targetArchitecture->geometry().left(),"target Windows and architecture share a compact settings row");targetWindows->setCurrentIndex(targetWindows->findData(61));targetArchitecture->setCurrentIndex(targetArchitecture->findData(32));QTest::qWait(30);check(previewBridge->previewVersion()==0 && previewBridge->previewArchitecture()==0,"editing target controls alone does not invalidate or apply a compatibility preview");window.findChild<QPushButton*>(QStringLiteral("applyTarget"))->click();QTest::qWait(550);check(previewBridge->previewVersion()==61 && previewBridge->previewArchitecture()==32,"Apply commits the chosen target through an explicit action");window.findChild<QPushButton*>(QStringLiteral("resetTarget"))->click();QTest::qWait(550);check(previewBridge->previewVersion()==0 && previewBridge->previewArchitecture()==0 && catalog->size()>0,"Restore default restores the live target and refreshes inventory without a manual scan");
        bool previewExplained=false;QObject helpProbe;QTimer::singleShot(60,&helpProbe,[&](){for(QWidget *top:QApplication::topLevelWidgets())if(top->objectName()==QStringLiteral("projectDialog")){for(QLabel *label:top->findChildren<QLabel*>())if(label->text().contains(QStringLiteral("technicians")))previewExplained=true;top->close();return;}});window.findChild<QToolButton*>(QStringLiteral("compatibilityHelp"))->click();check(previewExplained,"compatibility help opens the project-styled explanation");
        QProgressBar gradientBar;gradientBar.setObjectName(QStringLiteral("mainProgress"));gradientBar.setStyleSheet(Theme::styleSheet(false));gradientBar.setRange(0,100);gradientBar.setValue(100);gradientBar.setTextVisible(false);gradientBar.setFixedSize(300,14);gradientBar.show();QTest::qWait(30);const QImage gradientImage=gradientBar.grab().toImage();const qreal gradientDpr=gradientImage.width()/qreal(gradientBar.width());const QColor gradientLeft=QColor::fromRgba(gradientImage.pixel(qRound(60*gradientDpr),qRound(7*gradientDpr))),gradientRight=QColor::fromRgba(gradientImage.pixel(qRound(240*gradientDpr),qRound(7*gradientDpr)));check(qAbs(gradientLeft.green()-gradientRight.green())+qAbs(gradientLeft.blue()-gradientRight.blue())>15,"progress fill renders a visible gradient rather than one flat color");gradientBar.close();
        check(AboutMotion::heartbeatScale(110)>1.17 && AboutMotion::heartbeatScale(405)>1.11,"signature heart has two distinct heartbeat peaks");
        check(AboutMotion::heartbeatScale(700)==1 && AboutMotion::heartbeatScale(1500)==1 && qAbs(AboutMotion::heartbeatScale(1910)-AboutMotion::heartbeatScale(110))<.001,"heartbeat rests between pairs and repeats using elapsed time");
        QMetaObject::invokeMethod(&window,"navigate",Q_ARG(int,5));QTest::qWait(230);
        QWidget *signature=window.findChild<QWidget*>(QStringLiteral("developerSignature"));QStringList signatureWords;for(QLabel *word:signature->findChildren<QLabel*>())signatureWords<<word->text();
        check(signatureWords.contains(QStringLiteral("Made with")) && signatureWords.contains(QStringLiteral("by")) && signature->findChild<QWidget*>(QStringLiteral("signatureHeart")),"developer signature places the heart between made-with and by above the author name");
        qApp->setProperty("reduceMotion",false);
        for(int i=0;i<5;++i)QMetaObject::invokeMethod(window.findChild<TitleBar*>(),"maximizeRequested");QTest::qWait(100);
        check(window.isMaximized() && window.windowOpacity()>.99,"repeated native window-state requests settle without hiding the live interface");
        check(window.property("windowMotionBackend").toString()==QStringLiteral("native-window-manager") && !window.property("motionCapturePixels").isValid(),"window transitions delegate to Windows without raster capture or per-frame geometry work");
        QMetaObject::invokeMethod(window.findChild<TitleBar*>(),"maximizeRequested");QTest::qWait(300);check(!window.isMaximized() && window.windowOpacity()>.99,"restore from maximized completes with the live interface visible");
        QMetaObject::invokeMethod(window.findChild<TitleBar*>(),"minimizeRequested");QTest::qWait(280);QElapsedTimer minimizeWait;minimizeWait.start();while(!window.isMinimized() && minimizeWait.elapsed()<1200)QTest::qWait(20);if(!window.isMinimized())std::printf("Minimize diagnostic: Qt state=%d visible=%d modal=%s\n",int(window.windowState()),int(window.isVisible()),app.activeModalWidget()?app.activeModalWidget()->objectName().toUtf8().constData():"none");check(window.isMinimized() && !window.property("windowMotionActive").toBool(),"minimize reaches the real window-manager state after its bounded transition");
        window.showNormal();QTest::qWait(300);bool ghostVisible=false;for(QWidget *top:app.topLevelWidgets())if(top->objectName()==QStringLiteral("windowMotionSnapshot") && top->isVisible())ghostVisible=true;
        check(window.isVisible() && window.windowOpacity()>.99 && !ghostVisible,"taskbar restore leaves the live window visible without a snapshot overlay");
        qApp->setProperty("reduceMotion",true);QMetaObject::invokeMethod(window.findChild<TitleBar*>(),"maximizeRequested");QTest::qWait(40);check(window.isMaximized() && !window.property("windowMotionActive").toBool(),"reduced-motion preference takes the direct native window-state path");window.showNormal();qApp->setProperty("reduceMotion",false);
        Splash splash;splash.begin();QTest::qWait(500);splash.grab().save(QDir(app.applicationDirPath()).filePath(QStringLiteral("splash.png")));
        check(splash.progress()>0 && splash.progress()<100,"startup progress animation runs");splash.close();
        workspace->setInstallationPreferences(false,false);QMetaObject::invokeMethod(&window,"updateDark",Q_ARG(bool,false));testSettings->sync();check(!testSettings->value(QStringLiteral("installation/restorePointRequired"),true).toBool() && !testSettings->value(QStringLiteral("installation/rebootAfter"),true).toBool(),"explicit installation preferences survive settings persistence for administrator restart");
        std::printf("%d UI and instance checks passed; no system installation was performed.\n",checks);
        testSettings->clear();testSettings->sync();
        return 0;
    }catch(const std::exception &error){std::fprintf(stderr,"UI test failed: %s\n",error.what());return 1;}
}

