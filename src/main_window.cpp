#include "main_window.h"
#include "drop_target.h"
#include "ui_scale.h"
#include <QMimeData>
#include <QDragEnterEvent>
#include <QDropEvent>
#include <QDesktopWidget>
#include <QStandardItemModel>
#include "ui_language.h"
#include "restart_prompt.h"
#include "sharing_restore.h"
#include "app_update.h"
#include <QLineEdit>
#include "sidebar.h"
#include "title_bar.h"
#include "tools_drawer.h"
#include "engine_bridge.h"
#include "driver_catalog.h"
#include "driver_workspace.h"
#include "installation_workspace.h"
#include "ui_elements.h"
#include "vector_icons.h"
#include "system_profile.h"
#include "origin_languages.h"
#include "offline_paths.h"
#include "appearance.h"
#include "window_motion.h"
#include <QWindowStateChangeEvent>
#include <QStyle>
#include "support_report.h"
#include "online_dialog.h"
#include <QCheckBox>
#include <QSaveFile>
#include <QSignalBlocker>
#include <QJsonDocument>
#include "theme.h"
#include <QApplication>
#include <QCloseEvent>
#include <QDialog>
#include <QMessageBox>
#include <QPushButton>
#include <QToolButton>
#include <QLabel>
#include <QFrame>
#include <QProgressBar>
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QGridLayout>
#include <QScrollArea>
#include <QDirIterator>
#include <QStandardPaths>
#include <QUuid>
#include <QDesktopServices>
#include <QStackedWidget>
#include <QComboBox>
#include "searchable_language_combo.h"
#include <QListView>
#include <QStyledItemDelegate>
#include <QStyleOptionViewItem>
#include <QSettings>
#include "app_settings.h"
#include "about_page.h"
#include "portable_library.h"
#include "automation_plan.h"
#include <QFileDialog>
#include <QDir>
#include <QFileInfo>
#include <QFile>
#include <QElapsedTimer>
#include "scan_overlay.h"
#include <QDesktopServices>
#include <QProcess>
#include <QMenu>
#include <QCursor>
#include <QPropertyAnimation>
#include <QGraphicsOpacityEffect>
#include <QSysInfo>
#include <QUrl>
#include <QEvent>
#include <QResizeEvent>
#include <QFont>
#include <QTimer>
#include <QUuid>
#include <QPainter>
#include <QPainterPath>
#include <QStyle>
#include <exception>
#ifdef Q_OS_WIN
#include <windows.h>
#include <windowsx.h>
#endif

static QLabel *newLabel(const QString &text,const char *name,QWidget *parent)
{
    QLabel *v=new QLabel(text,parent);
    v->setObjectName(QString::fromLatin1(name));
    v->setWordWrap(true);
    return v;
}
static QPushButton *newButton(const QString &name,const char *style,QWidget *parent)
{
    QPushButton *b=new ActionButton(name,parent);
    b->setObjectName(QString::fromLatin1(style));
    b->setCursor(Qt::PointingHandCursor);
    return b;
}

MainWindow::MainWindow(bool demo,QWidget *parent)
    : QWidget(parent),m_demo(demo),m_arabic(false),m_dark(false),m_reduceMotion(false),m_closeWhenIdle(false),
      m_currentPage(0),m_settings(0),m_sidebar(0),m_titleBar(0),m_toolsDrawer(0),m_pages(0),
      m_engine(new EngineBridge(this)),m_scanOverlay(0),m_scanClock(new QElapsedTimer),
      m_catalog(new DriverCatalog(this)),
      m_drivers(0),m_installation(0),m_diagnosticsTitle(0),m_diagnosticsSubtitle(0),m_offlineTitle(0),
      m_offlineStatus(0),m_driversFolder(0),m_indexesFolder(0),m_toolsFolder(0),m_settingsTitle(0),
      m_settingsHint(0),m_langLabel(0),m_darkLabel(0),m_motionLabel(0),m_langNotice(0),
      m_language(0),m_darkSwitch(0),m_motionSwitch(0),m_driversBrowse(0),m_indexesBrowse(0),
      m_probeButton(0),m_onlineButton(0),m_probeStatus(0),m_probeHint(0)
{
    setWindowFlags(Qt::Window | Qt::FramelessWindowHint);
    setAttribute(Qt::WA_TranslucentBackground);
    setObjectName(QStringLiteral("root"));
    setWindowTitle(QStringLiteral("Snappy Driver Installer"));
    resize(1400,840);
    setMinimumSize(950,640);setAcceptDrops(true);
    const QString root=QCoreApplication::applicationDirPath();
    m_driversPath=QDir(root).filePath(QStringLiteral("drivers"));
    m_indexesPath=QDir(root).filePath(QStringLiteral("indexes"));
    m_toolsPath=QDir(root).filePath(QStringLiteral("tools"));
    m_settings=createAppSettings(root,this);
    const QByteArray sessionData=qgetenv("SDIO_LEGACY_SESSION");qunsetenv("SDIO_LEGACY_SESSION");
    const QJsonObject legacySession=sessionData.size()<=65536?QJsonDocument::fromJson(sessionData).object():QJsonObject();
    try{const QJsonObject saved=legacySession.contains(QStringLiteral("themeDocument"))?legacySession.value(QStringLiteral("themeDocument")).toObject():QJsonDocument::fromJson(m_settings->value(QStringLiteral("theme/document")).toByteArray()).object();if(!saved.isEmpty())Theme::setDocument(saved);}catch(const std::exception&){Theme::setDocument(QJsonObject());}
    m_catalog->setIgnoredHardwareIds(m_settings->value(QStringLiteral("ignoredHardwareIds")).toStringList());
    connect(m_catalog,&DriverCatalog::ignoredChanged,this,[this](const QStringList &ids){m_settings->setValue(QStringLiteral("ignoredHardwareIds"),ids);m_settings->sync();});
    m_languageCode=(legacySession.contains(QStringLiteral("lang"))?legacySession.value(QStringLiteral("lang")).toString():m_settings->value(QStringLiteral("language"),QStringLiteral("english")).toString()).toLower();
    if(m_languageCode==QStringLiteral("en"))m_languageCode=QStringLiteral("english");
    if(m_languageCode==QStringLiteral("ar"))m_languageCode=QStringLiteral("arabic");
    if(m_languageCode==QStringLiteral("hebrew"))m_languageCode=QStringLiteral("english");
    m_arabic=m_languageCode==QStringLiteral("arabic");
    UiLanguage::setLanguage(m_languageCode);
    m_dark=m_settings->value(QStringLiteral("dark"),false).toBool();
    m_reduceMotion=m_settings->value(QStringLiteral("reduceMotion"),false).toBool();
    // Restore motion once for this upgrade; keep subsequent accessibility choices.
    if(m_settings->value(QStringLiteral("motionProfile"),0).toInt()<2){
        m_reduceMotion=false;m_settings->setValue(QStringLiteral("reduceMotion"),false);
        m_settings->setValue(QStringLiteral("motionProfile"),2);
    }
    qApp->setProperty("reduceMotion",m_reduceMotion);
    const QString siblingDrivers=m_driversPath,siblingIndexes=m_indexesPath;
    const QString savedDrivers=m_settings->value(QStringLiteral("driversPath")).toString();
    const QString savedIndexes=m_settings->value(QStringLiteral("indexesPath")).toString();
    // Running next to an SDIO Offline tree takes precedence over stale settings
    // from previous copies. Never search arbitrary disk locations.
    if(!(QDir(siblingDrivers).exists() && QDir(siblingIndexes).exists())){
        if(QDir(savedDrivers).exists())m_driversPath=savedDrivers;
        if(QDir(savedIndexes).exists())m_indexesPath=savedIndexes;
    }


    if(legacySession.contains(QStringLiteral("drivers")))m_driversPath=legacySession.value(QStringLiteral("drivers")).toString();
    if(legacySession.contains(QStringLiteral("indexes")))m_indexesPath=legacySession.value(QStringLiteral("indexes")).toString();
    QVBoxLayout *mainLayout=new QVBoxLayout(this);
    mainLayout->setContentsMargins(0,0,0,0);
    mainLayout->setSpacing(0);
    m_titleBar=new TitleBar(this);
    mainLayout->addWidget(m_titleBar);
    QWidget *container=new QWidget(this);
    QHBoxLayout *body=new QHBoxLayout(container);
    body->setContentsMargins(0,0,0,0);
    body->setSpacing(0);
    m_sidebar=new Sidebar(container);
    m_sidebar->setAnimations(!m_reduceMotion);
    body->addWidget(m_sidebar);
    m_pages=new QStackedWidget(container);
    m_drivers=new DriverWorkspace(m_catalog,m_pages);
    m_drivers->setInstallationPreferences(m_settings->value(QStringLiteral("installation/restorePointRequired"),true).toBool(),m_settings->value(QStringLiteral("installation/rebootAfter"),false).toBool());
    m_installation=new InstallationWorkspace(m_pages);
    m_pages->addWidget(m_drivers);           // 0
    m_pages->addWidget(createDiagnostics()); // 1
    m_pages->addWidget(createOffline());     // 2
    m_pages->addWidget(createSettings());    // 3
    m_pages->addWidget(m_installation);      // 4
    m_pages->addWidget(createAbout());       // 5
    body->addWidget(m_pages,1);
    mainLayout->addWidget(container,1);
    // Draw the tools overlay above the workspace (below the titlebar).
    m_scanOverlay=new ScanOverlay(m_pages);
    m_scanOverlay->setGeometry(m_pages->rect());
    m_scanOverlay->hide();
    m_toolsDrawer=new ToolsDrawer(this);
    m_toolsDrawer->setGeometry(0,m_titleBar->height(),width(),height()-m_titleBar->height());
    m_toolsDrawer->setMotionEnabled(!m_reduceMotion);
    connect(m_toolsDrawer,&ToolsDrawer::actionRequested,this,&MainWindow::dispatchTool);
    connect(m_installation,&InstallationWorkspace::systemProtectionRequested,this,[this](){dispatchTool(QStringLiteral("system-protection"));});

    connect(m_sidebar,&Sidebar::pageRequested,this,&MainWindow::navigate);
    connect(m_titleBar,&TitleBar::refreshRequested,this,&MainWindow::beginScan);
    connect(m_titleBar,&TitleBar::sidebarRequested,m_sidebar,&Sidebar::toggleCollapsed);
    connect(m_titleBar,&TitleBar::menuRequested,this,&MainWindow::openToolsMenu);
    m_windowMotion=new WindowMotion(this);
    connect(m_titleBar,&TitleBar::minimizeRequested,this,[this](){m_windowMotion->minimize();});
    connect(m_titleBar,&TitleBar::closeRequested,this,&MainWindow::close);
    connect(m_titleBar,&TitleBar::maximizeRequested,this,[this](){
        m_windowMotion->toggleMaximized();
    });
    connect(m_engine,&EngineBridge::inventoryReady,this,&MainWindow::onInventoryReady);
    connect(m_engine,&EngineBridge::nativeSnapshotReady,this,&MainWindow::onNativeSnapshot);
    connect(m_engine,&EngineBridge::nativeProgress,this,&MainWindow::onNativeProgress);
    connect(m_engine,&EngineBridge::nativeInstallationFinished,this,&MainWindow::onNativeFinished);
    connect(m_engine,&EngineBridge::engineError,this,&MainWindow::onEngineError);
    connect(m_installation,&InstallationWorkspace::stopRequested,m_engine,&EngineBridge::requestStopAfterCurrent);
    connect(m_engine,&EngineBridge::snapshotSaved,this,[this](const QString &file){
        m_scanOverlay->finish();m_titleBar->setRefreshEnabled(true);m_drivers->setBusy(false);m_probeButton->setEnabled(true);
        m_probeStatus->setText(QStringLiteral("Original SDIO snapshot saved: ")+file);
        if(m_closeWhenIdle)QTimer::singleShot(0,this,SLOT(close()));
    });
    connect(m_engine,&EngineBridge::toolFinished,this,[this](const QString &message){
        m_scanOverlay->finish();
        m_titleBar->setRefreshEnabled(true);m_drivers->setBusy(false);m_probeButton->setEnabled(true);
        m_drivers->setStatus(message);m_probeStatus->setText(message);
        if(!m_pendingInf.isEmpty()){
            const QString wanted=m_pendingInf;m_pendingInf.clear();QDirIterator files(message,QStringList()<<QStringLiteral("*.inf"),QDir::Files,QDirIterator::Subdirectories);QString path;
            while(files.hasNext()){const QString found=files.next();if(QFileInfo(found).fileName().compare(wanted,Qt::CaseInsensitive)==0){path=found;break;}}
            if(path.isEmpty())projectMessage(this,t("INF file not found","لم يُعثر على ملف INF"),message);
            else if(m_locatePendingInf)QProcess::startDetached(QStringLiteral("explorer.exe"),QStringList()<<(QStringLiteral("/select,")+QDir::toNativeSeparators(path)));
            else QProcess::startDetached(QStringLiteral("notepad.exe"),QStringList()<<path);
            return;
        }
        if(m_closeWhenIdle)QTimer::singleShot(0,this,SLOT(close()));
        else projectMessage(this,t("Operation completed","اكتملت العملية"),message);
    });
    connect(m_drivers,&DriverWorkspace::scanRequested,this,&MainWindow::beginScan);
    connect(m_drivers,&DriverWorkspace::installationRequested,this,&MainWindow::beginInstallation);
    connect(m_drivers,&DriverWorkspace::utilitiesRequested,this,&MainWindow::openToolsMenu);
    connect(m_drivers,&DriverWorkspace::snapshotRequested,this,&MainWindow::openSnapshot);
    connect(m_drivers,&DriverWorkspace::extractRequested,this,&MainWindow::openExtract);
    connect(m_drivers,&DriverWorkspace::packageUpdatesRequested,this,[this](){openOnlinePackages();});connect(m_drivers,&DriverWorkspace::packageUpdateCheckRequested,this,[this](){checkLibraryUpdates();});
    connect(m_drivers,&DriverWorkspace::packageDownloadRequested,this,[this](const QString &package){openOnlinePackages(package);});
    connect(m_drivers,&DriverWorkspace::candidateExtractionRequested,this,[this](int row,int candidate){
        if(m_engine->isBusy() || !m_catalog->native() || row<0 || row>=m_catalog->size())return;
        const DriverEntry &entry=m_catalog->entryRef(row);
        if(candidate<=0 || candidate>=entry.candidates.size() || !entry.candidates[candidate].valid || !entry.candidates[candidate].local)return;
        sdiomodern::InstallPlan plan;plan.generation=m_catalog->nativeSnapshot().generation;plan.confirmed=true;plan.createRestorePoint=false;plan.allowAlternatives=true;
        sdiomodern::Selection selection;selection.deviceInstanceId=entry.device.instanceId.toStdWString();selection.candidateToken=entry.candidates[candidate].engineToken;plan.selections.push_back(selection);
        const QString folder=QFileDialog::getExistingDirectory(this,t("Extract driver files","استخراج ملفات التعريف"));if(folder.isEmpty())return;
        m_drivers->setBusy(true);m_titleBar->setRefreshEnabled(false);m_probeButton->setEnabled(false);
        m_scanOverlay->startOperation(t("Extracting driver files","استخراج ملفات التعريفات"),entry.device.name,m_reduceMotion);
        m_engine->requestExtraction(plan,folder);
    });
    connect(m_drivers,&DriverWorkspace::optionsRequested,this,&MainWindow::openOptions);
    connect(m_drivers,&DriverWorkspace::infViewRequested,this,[this](int row,int candidate,bool locate){
        if(m_engine->isBusy() || row<0 || row>=m_catalog->size())return;const DriverEntry entry=m_catalog->entry(row);if(candidate<0 || candidate>=entry.candidates.size())return;
        const DriverCandidate data=entry.candidates[candidate];if(data.inf.isEmpty())return;
        if(candidate==0){
            const QString infRoot=QDir(qEnvironmentVariableIsSet("SystemRoot")?QString::fromLocal8Bit(qgetenv("SystemRoot")):QStringLiteral("C:/Windows")).filePath(QStringLiteral("INF"));
            const QString filename=QFileInfo(QDir::fromNativeSeparators(data.inf)).fileName();const QString path=QDir(infRoot).filePath(filename);
            if(!filename.endsWith(QStringLiteral(".inf"),Qt::CaseInsensitive) || !QFileInfo(path).isFile())return;
            if(locate)QProcess::startDetached(QStringLiteral("explorer.exe"),QStringList()<<(QStringLiteral("/select,")+QDir::toNativeSeparators(path)));else QProcess::startDetached(QStringLiteral("notepad.exe"),QStringList()<<path);return;
        }
        if(!data.valid || !data.local || !m_catalog->nativeSnapshot().generation)return;
        sdiomodern::InstallPlan plan;plan.generation=m_catalog->nativeSnapshot().generation;plan.confirmed=true;plan.allowAlternatives=true;plan.createRestorePoint=false;
        sdiomodern::Selection selection;selection.deviceInstanceId=entry.device.instanceId.toStdWString();selection.candidateToken=data.engineToken;plan.selections.push_back(selection);
        const QString folder=QDir(QStandardPaths::writableLocation(QStandardPaths::TempLocation)).filePath(QStringLiteral("SDIO-view-")+QUuid::createUuid().toString());if(!QDir().mkpath(folder))return;
        m_pendingInf=QFileInfo(QDir::fromNativeSeparators(data.inf)).fileName();m_locatePendingInf=locate;m_drivers->setBusy(true);m_titleBar->setRefreshEnabled(false);m_probeButton->setEnabled(false);
        m_scanOverlay->startOperation(t("Preparing driver details","تجهيز بيانات التعريف"),t("Extracting the INF for viewing. No driver will be installed.","استخراج INF للعرض فقط، دون تثبيت أي تعريف."),m_reduceMotion);m_engine->requestExtraction(plan,folder);
    });
    connect(m_installation,&InstallationWorkspace::backRequested,this,[this](){navigate(0);if(m_lastInstallation.restoreFailed)QTimer::singleShot(0,m_drivers,&DriverWorkspace::openInstallOptions);});
    connect(m_installation,&InstallationWorkspace::simulationFinished,this,[this](bool reboot){
        if(!reboot) return;
        projectMessage(this,t("Reboot required — preview","مطلوب إعادة تشغيل — معاينة"),
            t("This is a demonstration. No reboot or driver installation will occur.","هذه محاكاة فقط، ولن يحدث تثبيت تعريفات أو إعادة تشغيل."),false,m_arabic);
    });
    connect(m_installation,&InstallationWorkspace::activeChanged,this,[this](bool active){
        m_titleBar->setStatus(active?(m_demo?QStringLiteral("DEMO - NO CHANGES"):
            QStringLiteral("NATIVE INSTALLATION - DO NOT INTERRUPT")):
            (m_demo?QStringLiteral("DEMO - READ ONLY"):QStringLiteral("ORIGIN NATIVE CORE")));
    });
    connect(m_driversBrowse,&QPushButton::clicked,this,&MainWindow::chooseDriversFolder);
    connect(m_indexesBrowse,&QPushButton::clicked,this,&MainWindow::chooseIndexesFolder);
    connect(m_probeButton,&QPushButton::clicked,this,&MainWindow::beginScan);
    m_language->setProperty("languageSelector",true);
    connect(m_language,SIGNAL(currentIndexChanged(int)),this,SLOT(updateLanguage(int)));
    connect(m_darkSwitch,&ToggleSwitch::toggled,this,&MainWindow::updateDark);
    connect(m_motionSwitch,&ToggleSwitch::toggled,this,&MainWindow::updateReduceMotion);

    m_language->blockSignals(true);
    const int localeIndex=m_language->findData(m_languageCode);
    m_language->setCurrentIndex(localeIndex<0?0:localeIndex);
    m_language->blockSignals(false);
    m_darkSwitch->blockSignals(true);m_darkSwitch->setChecked(m_dark);m_darkSwitch->blockSignals(false);
    m_darkSwitch->setPosition(m_dark?1.0:0.0);
    m_motionSwitch->blockSignals(true);m_motionSwitch->setChecked(m_reduceMotion);m_motionSwitch->blockSignals(false);
    m_motionSwitch->setPosition(m_reduceMotion?1.0:0.0);
    m_darkSwitch->setMotionEnabled(!m_reduceMotion);
    m_motionSwitch->setMotionEnabled(!m_reduceMotion);
    m_drivers->setDemo(m_demo);
    if(!legacySession.isEmpty()){
        sdiomodern::Paths options;options.drivers=m_driversPath.toStdWString();options.indexes=m_indexesPath.toStdWString();options.logDirectory=QDir(root).filePath(QStringLiteral("logs")).toStdWString();options.targetVersion=legacySession.value(QStringLiteral("targetVersion")).toInt();options.targetArchitecture=legacySession.value(QStringLiteral("targetArchitecture")).toInt();options.disableInstall=legacySession.value(QStringLiteral("disableInstall")).toBool();options.filterServicePacks=legacySession.value(QStringLiteral("filterServicePacks")).toBool();options.failSafe=legacySession.value(QStringLiteral("failSafe")).toBool();options.deleteExtraInf=legacySession.value(QStringLiteral("deleteExtraInf")).toBool();options.dpInst=legacySession.value(QStringLiteral("dpInst")).toBool();options.keepTempFiles=legacySession.value(QStringLiteral("keepTempFiles")).toBool();options.keepUnpackedIndex=legacySession.value(QStringLiteral("keepUnpackedIndex")).toBool();options.noLog=legacySession.value(QStringLiteral("noLog")).toBool();m_engine->setSessionOptions(options);
        {QSignalBlocker v(m_targetVersion),a(m_targetArchitecture);m_targetVersion->setCurrentIndex(qMax(0,m_targetVersion->findData(options.targetVersion)));m_targetArchitecture->setCurrentIndex(qMax(0,m_targetArchitecture->findData(options.targetArchitecture)));}
        m_drivers->applyLegacyDisplay(legacySession.value(QStringLiteral("filters")).toInt(-1),legacySession.value(QStringLiteral("expertmode")).toBool(),legacySession.value(QStringLiteral("showdrpnames1")).toBool() || legacySession.value(QStringLiteral("showdrpnames2")).toBool());
        if(legacySession.value(QStringLiteral("wndwx")).toInt()>0 && legacySession.value(QStringLiteral("wndwy")).toInt()>0){resize(qMax(950,legacySession.value(QStringLiteral("wndwx")).toInt()),qMax(640,legacySession.value(QStringLiteral("wndwy")).toInt()));setProperty("legacyWindowSize",true);}
    }
    m_installation->setAvailability(m_engine->canInstall(),m_demo);
    m_drivers->setAnimations(!m_reduceMotion);
    applyTheme();
    translateUi();
    updateDiagnostics();
    updateOffline();
    if(!m_demo && m_settings->value(QStringLiteral("transfer/keepSharing"),true).toBool()){
        OnlinePackages::setSharingEnabled(true);OnlinePackages::TransferSettings limits;limits.keepSharing=true;limits.uploadKiB=m_settings->value(QStringLiteral("transfer/uploadKiB"),64).toInt();limits.connections=m_settings->value(QStringLiteral("transfer/connections"),40).toInt();limits.port=m_settings->value(QStringLiteral("transfer/port"),0).toInt();
        m_sharingRestore=new SharingRestore(QDir(m_driversPath).filePath(QStringLiteral(".sdio-downloads")),limits,this);m_sharingRestore->start();
    }
    m_updateCheck=new OnlineWorker(this);connect(m_updateCheck,&QThread::finished,this,[this](){
        const QUrl wanted=OnlinePackages::officialMetadataUrl(libraryFamily(m_driversPath,resolveIndexDirectory(m_indexesPath,m_driversPath)));
        if(m_updateCheck->metadataUrl!=wanted || m_updateCheck->drivers!=m_driversPath){refreshLibraryNotice();if(!m_demo && m_settings->value(QStringLiteral("updates/checkAtStartup"),true).toBool())QTimer::singleShot(0,this,[this](){checkLibraryUpdates();});return;}
        m_updateError=m_updateCheck->error;if(m_updateError.isEmpty()){m_updateCatalog=m_updateCheck->catalog;m_updateCatalogSource=wanted;}refreshLibraryNotice();
    });
    if(!m_demo && m_settings->value(QStringLiteral("updates/checkAtStartup"),true).toBool())QTimer::singleShot(1800,this,[this](){checkLibraryUpdates();});

}
MainWindow::~MainWindow() {if(m_updateCheck){m_updateCheck->stop.store(true);if(m_updateCheck->isRunning()){m_updateCheck->disconnect(this);m_updateCheck->setParent(0);connect(m_updateCheck,&QThread::finished,m_updateCheck,&QObject::deleteLater);}}OnlinePackages::stopSharing();delete m_sharingRestore;delete m_scanClock;}
void MainWindow::paintEvent(QPaintEvent *)
{
    QPainter painter(this);painter.setRenderHint(QPainter::Antialiasing);
    painter.setPen(Qt::NoPen);QPainterPath frame;frame.addRoundedRect(QRectF(rect()),isMaximized()?0:20,isMaximized()?0:20);
    const QRect header=m_titleBar->geometry();const QRect body(0,header.bottom()+1,width(),height()-header.height());
    const QRect side(m_sidebar->mapTo(this,QPoint()),m_sidebar->size());
    QRegion content(body);if(m_sidebar->isVisible())content-=QRegion(side);
    painter.save();painter.setClipRegion(content);painter.setBrush(Theme::color("background",m_dark));painter.drawPath(frame);painter.restore();
    QLinearGradient top(UiLanguage::isRtl()?width():0,0,UiLanguage::isRtl()?0:width(),0);top.setColorAt(0,Theme::color("headerStart"));top.setColorAt(.64,Theme::color("headerMiddle"));top.setColorAt(1,Theme::color("headerEnd"));
    painter.save();painter.setClipRect(header);painter.setBrush(top);painter.drawPath(frame);painter.restore();
    if(m_sidebar->isVisible()){QLinearGradient gradient(0,side.top(),0,side.bottom());gradient.setColorAt(0,Theme::color("sidebarStart"));gradient.setColorAt(.72,Theme::color("sidebarMiddle"));gradient.setColorAt(1,Theme::color("sidebarEnd"));painter.save();painter.setClipRect(side);painter.setBrush(gradient);painter.drawPath(frame);painter.restore();}
}
void MainWindow::resizeEvent(QResizeEvent *event)
{
    QWidget::resizeEvent(event);
    if(m_toolsDrawer && m_titleBar)
        m_toolsDrawer->setGeometry(0,m_titleBar->height(),width(),qMax(0,height()-m_titleBar->height()));
    if(m_scanOverlay)m_scanOverlay->setGeometry(m_pages->rect());
}
void MainWindow::changeEvent(QEvent *e)
{
    QWidget::changeEvent(e);
    if(e->type()==QEvent::WindowStateChange && m_titleBar){
        const bool restored=static_cast<QWindowStateChangeEvent*>(e)->oldState().testFlag(Qt::WindowMinimized) && !isMinimized();
        const bool cornersChanged=property("maximized").toBool()!=isMaximized();
        m_titleBar->setMaximized(isMaximized());setProperty("maximized",isMaximized());
        // A window-state change must not rebuild the entire application QSS.
        // Only page corners depend on the maximized property.
        if(cornersChanged){for(int i=0;m_pages && i<m_pages->count();++i){QWidget *page=m_pages->widget(i);page->style()->unpolish(page);page->style()->polish(page);page->update();}m_sidebar->update();update();}
        if(restored && m_windowMotion)m_windowMotion->restoredFromMinimized();
    }
}
#ifdef Q_OS_WIN
bool MainWindow::nativeEvent(const QByteArray &eventType,void *message,long *result)
{
    if(eventType==QByteArray("windows_generic_MSG") && !isMaximized() && !isFullScreen()){
        MSG *msg=static_cast<MSG*>(message);
        if(msg && msg->message==WM_NCHITTEST){
            RECT nativeFrame={};GetWindowRect(reinterpret_cast<HWND>(winId()),&nativeFrame);
            const qreal scale=devicePixelRatioF();
            QPoint pos(qRound((GET_X_LPARAM(msg->lParam)-nativeFrame.left)/scale),qRound((GET_Y_LPARAM(msg->lParam)-nativeFrame.top)/scale));
            const int edge=7;
            bool left=pos.x()>=0&&pos.x()<edge,right=pos.x()>=width()-edge&&pos.x()<width();
            bool top=pos.y()>=0&&pos.y()<edge,bottom=pos.y()>=height()-edge&&pos.y()<height();
            if(top&&left){*result=HTTOPLEFT;return true;}
            if(top&&right){*result=HTTOPRIGHT;return true;}
            if(bottom&&left){*result=HTBOTTOMLEFT;return true;}
            if(bottom&&right){*result=HTBOTTOMRIGHT;return true;}
            if(left){*result=HTLEFT;return true;}
            if(right){*result=HTRIGHT;return true;}
            if(top){*result=HTTOP;return true;}
            if(bottom){*result=HTBOTTOM;return true;}
        }
    }
    return QWidget::nativeEvent(eventType,message,result);
}
#endif
void MainWindow::dragEnterEvent(QDragEnterEvent *event){
    if(m_engine->isBusy() || m_installation->isActive() || m_scanOverlay->isActive())return;
    const DropTarget::Kind kind=DropTarget::classify(event->mimeData()->urls());
    if(kind==DropTarget::DriverFolder || (kind==DropTarget::SnapshotFile && !m_demo && m_engine->hasSdioMatcher()))event->acceptProposedAction();
}
void MainWindow::dropEvent(QDropEvent *event){
    if(m_engine->isBusy() || m_installation->isActive() || m_scanOverlay->isActive())return;
    QString path;const DropTarget::Kind kind=DropTarget::classify(event->mimeData()->urls(),&path);
    if(kind==DropTarget::DriverFolder){m_catalog->invalidateNative();m_driversPath=path;m_settings->setValue(QStringLiteral("driversPath"),path);m_settings->sync();updateOffline();event->acceptProposedAction();if(!m_demo)beginScan();}
    else if(kind==DropTarget::SnapshotFile && !m_demo && m_engine->hasSdioMatcher()){event->acceptProposedAction();loadSnapshotFile(path);}
}
void MainWindow::closeEvent(QCloseEvent *event)
{
    if(m_demo){if(m_installation->isActive())m_installation->stopDemo();event->accept();return;}
    if(!m_engine->isBusy() && !m_installation->isActive()){event->accept();return;}
    // Never terminate a native Windows driver operation or its worker thread.
    event->ignore();
    if(projectMessage(this,t("Stop after the current operation and close?","التوقف بعد العملية الحالية وإغلاق البرنامج؟"),
        t("The active Windows operation must finish safely. Continue to cancel the remaining queue and close when the current operation finishes. Cancel keeps the application open.",
          "يلزم إنهاء عملية ويندوز الحالية بأمان. متابعة تلغي باقي قائمة الانتظار وتغلق البرنامج بعد إنهاء العملية الحالية. إلغاء يبقي البرنامج مفتوحًا."),true,m_arabic)){
        m_closeWhenIdle=true;m_engine->requestStopAfterCurrent();
    }
}
QString MainWindow::t(const char *en,const char *ar) const
{return UiLanguage::text(QString::fromUtf8(en),QString::fromUtf8(ar));}
QLabel *MainWindow::translatedLabel(const char *en,const char *ar,const char *role,QWidget *parent){QLabel *label=newLabel(t(en,ar),role,parent);label->setProperty("englishCopy",QString::fromUtf8(en));label->setProperty("arabicCopy",QString::fromUtf8(ar));return label;}
QPushButton *MainWindow::translatedButton(const char *en,const char *ar,const char *role,QWidget *parent){QPushButton *button=newButton(t(en,ar),role,parent);button->setProperty("englishCopy",QString::fromUtf8(en));button->setProperty("arabicCopy",QString::fromUtf8(ar));return button;}
QWidget *MainWindow::newPage(){QWidget *p=new QWidget(m_pages);p->setObjectName(QStringLiteral("page"));return p;}
void MainWindow::beginScan()
{
    if(m_engine->isBusy() || m_installation->isActive())return;
    if(!m_demo && !m_engine->hasSdioMatcher()){
        onEngineError(QStringLiteral("This UI-only build has no linked Origin core. Build the native integration target; no demonstration data will be substituted."));return;
    }
    if(!m_demo && (!QDir(m_driversPath).exists() || !QDir(m_indexesPath).exists())){
        navigate(2);m_probeStatus->setText(QStringLiteral("Choose existing drivers and indexes folders, then click Scan with native Origin. No driver archives are bundled with the application."));return;
    }
    if(!m_demo)m_catalog->invalidateNative();
    m_engine->configure(m_driversPath,m_indexesPath,QDir(QCoreApplication::applicationDirPath()).filePath(QStringLiteral("logs")));
    m_scanClock->restart();
    m_scanOverlay->start(m_arabic?QString::fromUtf8("جارٍ فحص الأجهزة والتعريفات..."):
        QStringLiteral("Scanning devices and offline driver indexes..."),m_reduceMotion);
    m_drivers->setBusy(true);m_titleBar->setRefreshEnabled(false);m_probeButton->setEnabled(false);
    m_drivers->setStatus(m_demo?QStringLiteral("Preparing explicit demo fixtures..."):
                        QStringLiteral("Native Origin: reading devices, indexes and actual driver matches..."));
    m_engine->requestInventory(m_demo);
}
void MainWindow::onInventoryReady(const QVector<DeviceInfo> &devices,const QString &status)
{
    m_catalog->replace(devices,m_demo);
    if(m_scanOverlay)m_scanOverlay->finish();
    m_drivers->setBusy(false);
    m_titleBar->setRefreshEnabled(true);
    m_probeButton->setEnabled(true);
    m_drivers->setStatus(m_demo?t("DEMO only · Candidate scores and versions are illustrative; no driver changes.",
       "معاينة فقط · الإصدارات والدرجات توضيحية ولا يتم تغيير أي تعريف."):
       t("Windows PnP inventory only · SDIO matching/installation is not connected.",
         "عرض أجهزة ويندوز فقط · لم يتم ربط محرك المطابقة والتثبيت بعد."));
    Q_UNUSED(status);
    QTimer::singleShot(250,this,[this](){if(!m_engine->isBusy())updateDiagnostics();});
}
void MainWindow::navigate(int index)
{
    if(m_scanOverlay && m_scanOverlay->isActive())return;
    if(m_installation->isActive() && index!=4)return;
    if(index==4 && !m_demo && !m_engine->canInstall())index=0;
    if(index<0 || index>=m_pages->count() || index==m_currentPage) return;
    m_currentPage=index;
    m_pages->setCurrentIndex(index);
    m_pages->currentWidget()->setProperty("standalone",index==4);
    m_pages->currentWidget()->style()->unpolish(m_pages->currentWidget());
    m_pages->currentWidget()->style()->polish(m_pages->currentWidget());
    m_sidebar->setCurrentPage(index);
    m_sidebar->setVisible(index!=4);
    m_titleBar->setWorkflowLocked(index==4);
    if(m_toolsDrawer && m_toolsDrawer->isOpened())m_toolsDrawer->closeDrawer();
    if(m_reduceMotion)return;
    QWidget *page=m_pages->currentWidget();
    if(page->graphicsEffect())page->setGraphicsEffect(0);
    QGraphicsOpacityEffect *effect=new QGraphicsOpacityEffect(page);
    effect->setOpacity(0.55);page->setGraphicsEffect(effect);
    QPropertyAnimation *fade=new QPropertyAnimation(effect,"opacity",effect);
    fade->setDuration(155);fade->setStartValue(0.55);fade->setEndValue(1.0);
    connect(fade,&QPropertyAnimation::finished,page,[page,effect](){
        QTimer::singleShot(0,page,[page,effect](){if(page->graphicsEffect()==effect)page->setGraphicsEffect(0);});
    });
    fade->start(QAbstractAnimation::DeleteWhenStopped);
}
void MainWindow::beginInstallation()
{
    if(m_engine->isBusy() || m_installation->isActive() || !m_catalog->selectedDrivers())return;
    const QStringList selected=m_drivers->selectedNames();if(selected.isEmpty())return;
    if(m_demo){navigate(4);m_installation->beginDemo(selected,m_drivers->createRestorePoint(),m_drivers->rebootAfter());return;}
    if(!m_engine->canInstall()){onEngineError(QStringLiteral("This build supports matching only. Experimental native installation was not enabled during the build."));return;}
    sdiomodern::InstallPlan plan=m_catalog->buildPlan(m_drivers->createRestorePoint());
    bool alternative=false;
    const sdiomodern::Snapshot &snap=m_catalog->nativeSnapshot();
    for(std::size_t i=0;i<plan.selections.size();++i)
        for(std::size_t j=0;j<snap.devices.size();++j)
            for(std::size_t k=0;k<snap.devices[j].candidates.size();++k){
                const sdiomodern::Candidate &c=snap.devices[j].candidates[k];
                if(c.token==plan.selections[i].candidateToken && !c.recommended)alternative=true;
            }
    const bool elevated=UiScale::administrator();
    if(!confirmDriverInstallation(this,selected,plan.createRestorePoint,alternative,m_drivers->rebootAfter(),elevated,m_arabic,m_catalog->selectedReinstalls()>0))return;
    if(!elevated){
        saveSettings();const QString error=UiScale::restartElevated();
        if(error.isEmpty()){QCoreApplication::quit();return;}
        projectMessage(this,t("Administrator access","صلاحيات المسؤول"),t("The administrator restart was cancelled or could not start. Your selection is unchanged. Windows error: %1","تم إلغاء إعادة الفتح كمسؤول أو تعذّر بدؤها. لم يتغير تحديدك. خطأ ويندوز: %1").arg(error),false,m_arabic);return;
    }
    plan.confirmed=true;plan.allowAlternatives=alternative;
    try{sdiomodern::validatePlan(snap,plan);}catch(const std::exception &e){onEngineError(QString::fromLocal8Bit(e.what()));return;}
    QStringList ids;for(std::size_t i=0;i<plan.selections.size();++i)ids<<QString::fromStdWString(plan.selections[i].deviceInstanceId);
    navigate(4);m_drivers->setBusy(true);m_titleBar->setRefreshEnabled(false);
    m_installation->beginNative(selected,ids,plan.createRestorePoint);
    if(!m_engine->requestInstallation(plan))m_installation->failNative(QStringLiteral("Native installation could not start."));
}
void MainWindow::openSnapshot()
{
    if(m_demo || !m_catalog->native() || m_engine->isBusy()){
        projectMessage(this,QStringLiteral("Save native snapshot"),QStringLiteral("Complete a native Origin scan first."));return;
    }
    const QString f=QFileDialog::getSaveFileName(this,QStringLiteral("Save original SDIO snapshot"),QStringLiteral("machine.snp"),QStringLiteral("SDIO snapshot (*.snp)"));
    if(f.isEmpty())return;
    m_scanOverlay->startOperation(t("Saving snapshot","حفظ لقطة الجهاز"),t("Writing the original SDIO snapshot...","جارٍ حفظ لقطة SDIO الأصلية..."),m_reduceMotion);
    m_drivers->setBusy(true);m_titleBar->setRefreshEnabled(false);m_probeButton->setEnabled(false);m_engine->requestSnapshot(f);
}
void MainWindow::openExtract()
{
    if(m_engine->isBusy() || m_demo)return;
    if(!m_catalog->native() || !m_catalog->selectedDrivers()){
        projectMessage(this,t("Extract drivers","استخراج التعريفات"),
            t("Scan, then select the local drivers to extract. No installation is performed.","افحص ثم حدد التعريفات المحلية المطلوب استخراجها. لا يتم تثبيت أي تعريف."));return;
    }
    const QString folder=QFileDialog::getExistingDirectory(this,t("Choose extraction folder","اختر مجلد الاستخراج"));
    if(folder.isEmpty())return;
    m_drivers->setBusy(true);m_titleBar->setRefreshEnabled(false);m_probeButton->setEnabled(false);
    m_scanOverlay->startOperation(t("Extracting driver files","استخراج ملفات التعريفات"),t("Preparing selected local packages...","جارٍ تجهيز الحزم المحلية المحددة..."),m_reduceMotion);
    m_engine->requestExtraction(m_catalog->buildPlan(false,true,true),folder);
}
void MainWindow::loadSnapshot()
{
    if(m_demo || !m_engine->hasSdioMatcher() || m_engine->isBusy())return;
    if(!QDir(m_driversPath).exists() || !QDir(m_indexesPath).exists()){
        navigate(2);return;
    }
    const QString file=QFileDialog::getOpenFileName(this,t("Load SDIO snapshot (read only)","تحميل لقطة SDIO للعرض فقط"),QString(),QStringLiteral("SDIO snapshot (*.snp)"));
    if(file.isEmpty())return;loadSnapshotFile(file);
}
void MainWindow::loadSnapshotFile(const QString &file){
    if(m_demo || m_engine->isBusy() || m_installation->isActive() || !m_engine->hasSdioMatcher())return;
    m_catalog->invalidateNative();
    m_engine->configure(m_driversPath,m_indexesPath,QDir(QCoreApplication::applicationDirPath()).filePath(QStringLiteral("logs")));
    m_scanClock->restart();m_scanOverlay->start(t("Matching saved snapshot...","جارٍ فحص اللقطة المحفوظة..."),m_reduceMotion);
    m_drivers->setBusy(true);m_titleBar->setRefreshEnabled(false);m_probeButton->setEnabled(false);
    m_engine->requestLoadSnapshot(file);
}
void MainWindow::openOptions(){navigate(3);}
void MainWindow::openToolsMenu()
{
    const bool idle=!m_engine->isBusy() && !m_installation->isActive();
    const QString unavailable=t("Finish the active operation or perform a native scan first.","أكمل العملية الحالية أو افحص بالمحرك الأصلي أولًا.");
    m_toolsDrawer->setActionAvailable(QStringLiteral("reindex"),idle && !m_demo && m_engine->hasSdioMatcher(),unavailable);
    m_toolsDrawer->setActionAvailable(QStringLiteral("index-report"),idle && !m_demo && m_engine->hasSdioMatcher(),unavailable);
    m_toolsDrawer->setActionAvailable(QStringLiteral("updates"),idle && !m_demo,unavailable);
    m_toolsDrawer->setActionAvailable(QStringLiteral("report"),idle,unavailable);
    m_toolsDrawer->setActionAvailable(QStringLiteral("snapshot"),idle && !m_demo && m_catalog->native(),unavailable);
    m_toolsDrawer->setActionAvailable(QStringLiteral("load-snapshot"),idle && !m_demo && m_engine->hasSdioMatcher(),unavailable);
    m_toolsDrawer->setActionAvailable(QStringLiteral("extract"),idle && !m_demo && m_catalog->selectedDrivers()>0,
        t("Select local drivers after scanning to extract their files.","حدد التعريفات المحلية بعد الفحص لاستخراج ملفاتها."));
    m_toolsDrawer->setActionAvailable(QStringLiteral("restore"),idle && !m_demo && m_engine->canInstall(),
        t("Requires a native installation build and an idle engine.","يتطلب نسخة تدعم التثبيت الفعلي وانتهاء العملية الحالية."));
    const QString windows=QString::fromLocal8Bit(qgetenv("SystemRoot"));
    const bool dism=QFileInfo(QDir(windows).filePath(QStringLiteral("System32/dism.exe"))).isFile() ||
        QFileInfo(QDir(windows).filePath(QStringLiteral("Sysnative/dism.exe"))).isFile();
    m_toolsDrawer->setActionAvailable(QStringLiteral("backup"),idle && !m_demo && dism && QSysInfo::windowsVersion()>=QSysInfo::WV_WINDOWS8_1,
        t("Windows 8.1 or later with DISM driver export is required.","تتطلب هذه الخاصية ويندوز 8.1 فأحدث مع DISM لتصدير التعريفات."));
    m_toolsDrawer->toggle();
}
void MainWindow::dispatchTool(const QString &id)
{
    if(id==QStringLiteral("report-bug")){BugReport::show(this);return;}
    if(id==QStringLiteral("updates")){openOnlinePackages();return;}
    if(id==QStringLiteral("offline")){navigate(2);return;}
    if(id==QStringLiteral("snapshot")){openSnapshot();return;}
    if(id==QStringLiteral("load-snapshot")){loadSnapshot();return;}
    if(id==QStringLiteral("extract")){openExtract();return;}
    if(id==QStringLiteral("restore")){
        if(m_demo || !m_engine->canInstall() || m_engine->isBusy())return;
        if(!projectMessage(this,t("Create restore point","إنشاء نقطة استعادة"),
            t("Create a Windows restore point now? Administrator rights and enabled System Protection are required.","إنشاء نقطة استعادة الآن؟ يتطلب صلاحيات المسؤول وتفعيل حماية النظام."),
            true,m_arabic))return;
        m_engine->configure(m_driversPath,m_indexesPath,QDir(QCoreApplication::applicationDirPath()).filePath(QStringLiteral("logs")));
        m_drivers->setBusy(true);m_titleBar->setRefreshEnabled(false);m_probeButton->setEnabled(false);
        m_scanOverlay->startOperation(t("Creating restore point","إنشاء نقطة استعادة"),t("Waiting for Windows System Protection...","في انتظار حماية نظام ويندوز..."),m_reduceMotion);
        m_engine->requestRestorePoint();return;
    }
    if(id==QStringLiteral("backup")){
        if(m_demo || m_engine->isBusy())return;
        const QString folder=QFileDialog::getExistingDirectory(this,
            t("Export installed third-party drivers (DISM; Windows 8.1+)","نسخ تعريفات الطرف الثالث المثبتة — DISM لويندوز 8.1 فأحدث"));
        if(folder.isEmpty())return;
        const QString output=QDir(folder).filePath(QStringLiteral("SDIO-backup-")+QUuid::createUuid().toString());
        if(!QDir().mkpath(output)){projectMessage(this,t("Backup","النسخ الاحتياطي"),t("Cannot create backup folder.","تعذّر إنشاء مجلد النسخ."));return;}
        m_engine->configure(m_driversPath,m_indexesPath,QDir(QCoreApplication::applicationDirPath()).filePath(QStringLiteral("logs")));
        m_drivers->setBusy(true);m_titleBar->setRefreshEnabled(false);m_probeButton->setEnabled(false);
        m_scanOverlay->startOperation(t("Backing up installed drivers","نسخ التعريفات المثبتة"),t("Windows is exporting third-party driver packages...","ويندوز يصدّر حزم تعريفات الطرف الثالث..."),m_reduceMotion);
        m_engine->requestBackup(output);return;
    }
    if(id==QStringLiteral("settings")){navigate(3);return;}
    if(id==QStringLiteral("license")){
        const QString file=QDir(QCoreApplication::applicationDirPath()).filePath(QStringLiteral("LICENSE-GPL-3.0.txt"));
        if(QFileInfo(file).isFile())QDesktopServices::openUrl(QUrl::fromLocalFile(file));
        else projectMessage(this,t("License","الترخيص"),
          t("GPLv3 license is included in the source package.","رخصة GPLv3 موجودة ضمن ملفات السورس."));
        return;
    }
    if(id==QStringLiteral("reindex") || id==QStringLiteral("index-report")){
        if(m_demo || m_engine->isBusy() || m_installation->isActive() || !m_engine->hasSdioMatcher())return;
        QString folder;if(id==QStringLiteral("index-report")){folder=QFileDialog::getExistingDirectory(this,t("Export original index text","تصدير الفهرس النصي الأصلي"));if(folder.isEmpty())return;}
        m_catalog->invalidateNative();m_engine->configure(m_driversPath,m_indexesPath,QDir(QCoreApplication::applicationDirPath()).filePath(QStringLiteral("logs")));
        m_scanClock->restart();m_scanOverlay->startOperation(t("Index maintenance","صيانة الفهارس"),t("Working with the original index engine. Large libraries can take some time.","جارٍ العمل بمحرك الفهرسة الأصلي. قد تستغرق المكتبات الكبيرة بعض الوقت."),m_reduceMotion);
        m_drivers->setBusy(true);m_titleBar->setRefreshEnabled(false);m_probeButton->setEnabled(false);m_engine->requestIndexMaintenance(id==QStringLiteral("reindex"),folder);return;
    }
    if(id==QStringLiteral("automation")){
        if(m_engine->isBusy())return;
        QJsonObject plan;try{plan=automationPlan(*m_catalog,m_drivers->createRestorePoint());}catch(const std::exception &e){projectMessage(this,t("Select drivers first","حدد التعريفات أولًا"),QString::fromUtf8(e.what()));return;}
        const QString path=QFileDialog::getSaveFileName(this,t("Export automation plan","تصدير خطة الأتمتة"),QStringLiteral("SDIO-install-plan.json"),QStringLiteral("JSON (*.json)"));if(path.isEmpty())return;QSaveFile file(path);const QByteArray bytes=QJsonDocument(plan).toJson();if(!file.open(QIODevice::WriteOnly) || file.write(bytes)!=bytes.size() || !file.commit()){projectMessage(this,t("Cannot save plan","تعذر حفظ الخطة"),file.errorString());return;}
        projectMessage(this,t("Plan exported","تم تصدير الخطة"),t("The plan pins the exact devices and driver versions. Validate it with --validate-plan before intentional unattended installation. No installation was started.","الخطة تحدد الأجهزة وإصدارات التعريفات بدقة. تحقق منها باستخدام --validate-plan قبل التثبيت غير التفاعلي المقصود. لم يبدأ أي تثبيت."));return;
    }
    if(id==QStringLiteral("report")){
        if(m_engine->isBusy())return;
        const QString file=QFileDialog::getSaveFileName(this,t("Save local support report","حفظ تقرير الدعم محليًا"),QStringLiteral("SDIO-support.json"),QStringLiteral("JSON (*.json)"));
        if(file.isEmpty())return;
        QSaveFile out(file);const QByteArray bytes=QJsonDocument(supportReport(*m_catalog,m_lastInstallation,m_lastEngineError)).toJson();
        if(!out.open(QIODevice::WriteOnly) || out.write(bytes)!=bytes.size() || !out.commit()){
            projectMessage(this,t("Report could not be saved","تعذّر حفظ التقرير"),out.errorString(),false,m_arabic);return;
        }
        projectMessage(this,t("Report saved","تم حفظ التقرير"),
            t("The report contains device IDs, matching data and operation results. It was saved locally; nothing was uploaded.","التقرير يحتوي على معرّفات الأجهزة وبيانات المطابقة ونتائج العمليات. تم حفظه محليًا دون إرسال أي بيانات.")+QStringLiteral("\n")+file,false,m_arabic);
        return;
    }

#ifdef Q_OS_WIN
    // Windows tools are OS control-panel shortcuts, not SDIO driver operations.
    if(id==QStringLiteral("system-protection")){
        const QDir windows(QString::fromLocal8Bit(qgetenv("SystemRoot")));QString panel=windows.filePath(QStringLiteral("Sysnative/SystemPropertiesProtection.exe"));if(!QFileInfo(panel).isFile())panel=windows.filePath(QStringLiteral("System32/SystemPropertiesProtection.exe"));
        const bool ok=QFileInfo(panel).isFile()?QProcess::startDetached(panel):QProcess::startDetached(windows.filePath(QStringLiteral("System32/control.exe")),QStringList()<<QStringLiteral("sysdm.cpl,,4"));
        if(!ok)projectMessage(this,t("Cannot open tool","تعذّر فتح الأداة"),t("Open System Protection from Windows System Properties.","افتح حماية النظام من خصائص ويندوز."));return;
    }
    const QStringList ids=QStringList()<<QStringLiteral("uninstall")<<QStringLiteral("device-manager")
      <<QStringLiteral("system-properties")<<QStringLiteral("display-settings")
      <<QStringLiteral("power-options")<<QStringLiteral("network-connections")
      <<QStringLiteral("computer-management")<<QStringLiteral("control-panel")
      <<QStringLiteral("disk-management")<<QStringLiteral("task-manager")
      <<QStringLiteral("command-line");
    const QStringList apps=QStringList()<<QStringLiteral("appwiz.cpl")<<QStringLiteral("devmgmt.msc")
      <<QStringLiteral("sysdm.cpl")<<QStringLiteral("desk.cpl")
      <<QStringLiteral("powercfg.cpl")<<QStringLiteral("ncpa.cpl")
      <<QStringLiteral("compmgmt.msc")<<QStringLiteral("control.exe")
      <<QStringLiteral("diskmgmt.msc")<<QStringLiteral("taskmgr.exe")
      <<QStringLiteral("cmd.exe");
    const int index=ids.indexOf(id);
    if(index<0)return;
    const QString app=apps.at(index);
    bool ok=false;
    if(app.endsWith(QStringLiteral(".msc")))ok=QProcess::startDetached(QStringLiteral("mmc.exe"),QStringList()<<app);
    else if(app.endsWith(QStringLiteral(".cpl")))ok=QProcess::startDetached(QStringLiteral("control.exe"),QStringList()<<app);
    else ok=QProcess::startDetached(app);
    if(!ok)projectMessage(this,t("Cannot open tool","تعذّر فتح الأداة"),app);
#else
    projectMessage(this,QStringLiteral("Windows only"),QStringLiteral("Native Windows utilities require Windows."));
#endif
}

void MainWindow::addDiagnosticCard(QGridLayout *grid,int row,int col,const QString &icon,
    const QString &caption,const QString &key)
{
    QFrame *card=new SurfaceCard(this);
    card->setObjectName(QStringLiteral("diagnosticCard"));
    const QString group=grid->property("diagnosticGroup").toString();
    card->setProperty("tone",group);card->setMinimumHeight(102);
    const QColor accent=group==QStringLiteral("system")?QColor("#309AC6"):group==QStringLiteral("performance")?QColor("#14AA9D"):group==QStringLiteral("firmware")?QColor("#9B80CB"):QColor("#CC9743");
    QHBoxLayout *line=new QHBoxLayout(card);
    line->setContentsMargins(15,14,14,14);line->setSpacing(12);
    QLabel *symbol=new QLabel(card);
    symbol->setFixedSize(40,40);symbol->setObjectName(QStringLiteral("diagnosticSymbol"));symbol->setProperty("tone",group);
    symbol->setPixmap(VectorIcons::pixmap(icon,accent,24));
    symbol->setAlignment(Qt::AlignCenter);
    line->addWidget(symbol,0,Qt::AlignTop);
    QVBoxLayout *texts=new QVBoxLayout();texts->setSpacing(5);
    QLabel *heading=newLabel(caption,"diagCaption",card);
    texts->addWidget(heading);
    QLabel *value=newLabel(QStringLiteral("—"),"diagnosticValue",card);
    value->setTextInteractionFlags(Qt::TextSelectableByMouse);value->setTextFormat(Qt::PlainText);value->setProperty("skipTranslation",true);value->setProperty("diagnosticKey",key);
    value->setMinimumWidth(95);
    texts->addWidget(value,1);
    if(key==QStringLiteral("memoryload")){m_memoryUsageBar=new QProgressBar(card);m_memoryUsageBar->setObjectName(QStringLiteral("diagnosticUsage"));m_memoryUsageBar->setRange(0,100);m_memoryUsageBar->setFixedHeight(6);m_memoryUsageBar->setTextVisible(false);m_memoryUsageBar->hide();texts->addWidget(m_memoryUsageBar);}
    line->addLayout(texts,1);
    grid->addWidget(card,row,col);
    m_diagValues.insert(key,value);
    m_diagCaptions.insert(key,heading);
}
QWidget *MainWindow::createDiagnostics()
{
    QWidget *page=newPage();
    QVBoxLayout *root=new QVBoxLayout(page);
    root->setContentsMargins(28,21,28,15);root->setSpacing(10);
    AccentCard *hero=new AccentCard(page);hero->setObjectName(QStringLiteral("diagnosticsHero"));QHBoxLayout *heroRow=new QHBoxLayout(hero);heroRow->setContentsMargins(23,19,23,19);heroRow->setSpacing(17);
    QLabel *heroIcon=new QLabel(hero);heroIcon->setPixmap(VectorIcons::pixmap(QStringLiteral("activity"),QColor("#BBF9F0"),34));heroRow->addWidget(heroIcon);
    QVBoxLayout *heroText=new QVBoxLayout;heroText->setSpacing(5);
    m_diagnosticsTitle=newLabel(QStringLiteral("System diagnostics"),"diagnosticsHeroTitle",hero);heroText->addWidget(m_diagnosticsTitle);
    m_diagnosticsSubtitle=newLabel(QStringLiteral("Motherboard, firmware, components and Windows information · local / read only"),"diagnosticsHeroSubtitle",hero);heroText->addWidget(m_diagnosticsSubtitle);heroRow->addLayout(heroText,1);root->addWidget(hero);
    QScrollArea *scroll=new QScrollArea(page);
    scroll->setObjectName(QStringLiteral("diagnosticScroll"));
    scroll->setWidgetResizable(true);
    scroll->setFrameShape(QFrame::NoFrame);
    scroll->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    QWidget *body=new QWidget(scroll);
    body->setObjectName(QStringLiteral("diagnosticBody"));
    QVBoxLayout *content=new QVBoxLayout(body);
    content->setContentsMargins(0,5,9,15);
    content->setSpacing(11);

    // Three fluid columns make full use of the content width, without fixed pixel tables.
    auto section=[&](const QString &key,const QString &title,const QString &icon)->QGridLayout*{
        QWidget *headingRow=new QWidget(body);headingRow->setObjectName(QStringLiteral("diagnosticSectionHeader"));headingRow->setProperty("tone",key);
        QHBoxLayout *bar=new QHBoxLayout(headingRow);
        bar->setContentsMargins(0,6,0,0);
        bar->setSpacing(9);
        QLabel *symbol=new QLabel(headingRow);
        symbol->setFixedSize(22,22);
        symbol->setPixmap(VectorIcons::pixmap(icon,QColor("#08AFA9"),20));
        bar->addWidget(symbol);
        QLabel *heading=newLabel(title,"diagnosticSectionTitle",headingRow);
        m_diagSections.insert(key,heading);
        bar->addWidget(heading);bar->addStretch(1);
        content->addWidget(headingRow);
        QGridLayout *grid=new QGridLayout();grid->setProperty("diagnosticGroup",key);
        grid->setSpacing(11);
        grid->setContentsMargins(0,0,0,10);
        for(int c=0;c<3;++c)grid->setColumnStretch(c,1);
        content->addLayout(grid);
        return grid;
    };
    QGridLayout *system=section(QStringLiteral("system"),QStringLiteral("Windows & computer"),QStringLiteral("monitor"));
    addDiagnosticCard(system,0,0,QStringLiteral("monitor"),QStringLiteral("Operating system"),QStringLiteral("os"));
    addDiagnosticCard(system,0,1,QStringLiteral("layers"),QStringLiteral("Build / Windows release"),QStringLiteral("build"));
    addDiagnosticCard(system,0,2,QStringLiteral("chip"),QStringLiteral("System architecture"),QStringLiteral("architecture"));
    addDiagnosticCard(system,1,0,QStringLiteral("computer"),QStringLiteral("Computer name"),QStringLiteral("computer"));
    addDiagnosticCard(system,1,1,QStringLiteral("computer"),QStringLiteral("System manufacturer"),QStringLiteral("maker"));
    addDiagnosticCard(system,1,2,QStringLiteral("computer"),QStringLiteral("Model"),QStringLiteral("model"));

    QGridLayout *compute=section(QStringLiteral("performance"),QStringLiteral("Processor & memory"),QStringLiteral("chip"));
    addDiagnosticCard(compute,0,0,QStringLiteral("chip"),QStringLiteral("Processor"),QStringLiteral("cpu"));
    addDiagnosticCard(compute,0,1,QStringLiteral("settings"),QStringLiteral("Logical processors"),QStringLiteral("logical"));
    addDiagnosticCard(compute,0,2,QStringLiteral("activity"),QStringLiteral("Reported CPU clock"),QStringLiteral("clock"));
    addDiagnosticCard(compute,1,0,QStringLiteral("storage"),QStringLiteral("Installed RAM"),QStringLiteral("memory"));
    addDiagnosticCard(compute,1,1,QStringLiteral("storage"),QStringLiteral("Available RAM"),QStringLiteral("memoryfree"));
    addDiagnosticCard(compute,1,2,QStringLiteral("activity"),QStringLiteral("Memory usage"),QStringLiteral("memoryload"));

    QGridLayout *firmware=section(QStringLiteral("firmware"),QStringLiteral("Motherboard & firmware"),QStringLiteral("settings"));
    addDiagnosticCard(firmware,0,0,QStringLiteral("chip"),QStringLiteral("Motherboard model"),QStringLiteral("board"));
    addDiagnosticCard(firmware,0,1,QStringLiteral("computer"),QStringLiteral("Motherboard manufacturer"),QStringLiteral("boardmaker"));
    addDiagnosticCard(firmware,0,2,QStringLiteral("info"),QStringLiteral("Motherboard revision"),QStringLiteral("boardversion"));
    addDiagnosticCard(firmware,1,0,QStringLiteral("settings"),QStringLiteral("BIOS version"),QStringLiteral("bios"));
    addDiagnosticCard(firmware,1,1,QStringLiteral("chip"),QStringLiteral("BIOS vendor"),QStringLiteral("biosvendor"));
    addDiagnosticCard(firmware,1,2,QStringLiteral("activity"),QStringLiteral("BIOS release date"),QStringLiteral("biosdate"));
    addDiagnosticCard(firmware,2,0,QStringLiteral("shield"),QStringLiteral("Secure Boot"),QStringLiteral("secureboot"));
    addDiagnosticCard(firmware,2,1,QStringLiteral("activity"),QStringLiteral("System uptime"),QStringLiteral("uptime"));
    addDiagnosticCard(firmware,2,2,QStringLiteral("battery"),QStringLiteral("Power state"),QStringLiteral("power"));

    QGridLayout *hardware=section(QStringLiteral("hardware"),QStringLiteral("Devices & connectivity"),QStringLiteral("layers"));
    addDiagnosticCard(hardware,0,0,QStringLiteral("monitor"),QStringLiteral("Graphics adapters"),QStringLiteral("graphics"));
    addDiagnosticCard(hardware,0,1,QStringLiteral("monitor"),QStringLiteral("Screen resolution"),QStringLiteral("resolution"));
    addDiagnosticCard(hardware,0,2,QStringLiteral("storage"),QStringLiteral("Storage controllers / drives"),QStringLiteral("storage"));
    addDiagnosticCard(hardware,1,0,QStringLiteral("storage"),QStringLiteral("Windows system volume"),QStringLiteral("volume"));
    addDiagnosticCard(hardware,1,1,QStringLiteral("network"),QStringLiteral("Network adapters"),QStringLiteral("network"));
    addDiagnosticCard(hardware,1,2,QStringLiteral("network"),QStringLiteral("Wireless interfaces"),QStringLiteral("wireless"));
    addDiagnosticCard(hardware,2,0,QStringLiteral("audio"),QStringLiteral("Audio devices"),QStringLiteral("audio"));
    addDiagnosticCard(hardware,2,1,QStringLiteral("bluetooth"),QStringLiteral("Bluetooth devices"),QStringLiteral("bluetooth"));
    addDiagnosticCard(hardware,2,2,QStringLiteral("usb"),QStringLiteral("USB controllers"),QStringLiteral("usb"));
    addDiagnosticCard(hardware,3,0,QStringLiteral("battery"),QStringLiteral("Battery / ACPI"),QStringLiteral("battery"));
    addDiagnosticCard(hardware,3,1,QStringLiteral("shield"),QStringLiteral("TPM (PnP device)"),QStringLiteral("tpm"));
    addDiagnosticCard(hardware,3,2,QStringLiteral("activity"),QStringLiteral("Devices / PnP warnings"),QStringLiteral("issues"));
    addDiagnosticCard(hardware,4,0,QStringLiteral("monitor"),QStringLiteral("Display refresh rate"),QStringLiteral("refresh"));
    content->addStretch(1);
    scroll->setWidget(body);
    root->addWidget(scroll,1);
    return page;
}

QWidget *MainWindow::createAbout(){return new AboutPage(m_arabic,m_pages);}
QWidget *MainWindow::createOffline()
{
    QWidget *page=newPage();QVBoxLayout *outer=new QVBoxLayout(page);outer->setContentsMargins(0,0,0,20);
    QScrollArea *scroll=new QScrollArea(page);scroll->setWidgetResizable(true);scroll->setFrameShape(QFrame::NoFrame);scroll->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);outer->addWidget(scroll);
    QWidget *body=new QWidget(scroll);QVBoxLayout *layout=new QVBoxLayout(body);layout->setContentsMargins(30,26,30,26);layout->setSpacing(17);
    AccentCard *hero=new AccentCard(body);QHBoxLayout *heroRow=new QHBoxLayout(hero);heroRow->setContentsMargins(25,25,25,25);heroRow->setSpacing(20);
    QLabel *symbol=new QLabel(hero);symbol->setPixmap(VectorIcons::pixmap(QStringLiteral("layers"),QColor("#9CF3E8"),45));heroRow->addWidget(symbol);
    QVBoxLayout *heroCopy=new QVBoxLayout;heroCopy->setSpacing(9);m_offlineTitle=translatedLabel("Driver packages & updates","حزم التعريفات والتحديثات","pageTitle",hero);m_offlineTitle->setStyleSheet(QStringLiteral("color:white;background:transparent;font-size:25px;font-weight:600;"));heroCopy->addWidget(m_offlineTitle);
    QLabel *intro=translatedLabel("Your offline library, ready when you need it.","مكتبة تعريفاتك، جاهزة وقت احتياجك.","subText",hero);intro->setStyleSheet(QStringLiteral("color:#C0EBEC;background:transparent;font-size:13px;"));heroCopy->addWidget(intro);heroRow->addLayout(heroCopy,1);layout->addWidget(hero);
    m_offlineStatus=newLabel(QString(),"libraryStatus",body);m_offlineStatus->setStyleSheet(QStringLiteral("color:#138E8C;background:transparent;font-size:13px;font-weight:600;padding:7px 3px;"));layout->addWidget(m_offlineStatus);
    QHBoxLayout *folders=new QHBoxLayout;folders->setSpacing(15);
    auto folder=[&](const QString &heading,const QString &hint,const QString &icon,QLabel **path,QPushButton **browse){
        QWidget *card=new SurfaceCard(body);card->setObjectName(QStringLiteral("softSurface"));QVBoxLayout *form=new QVBoxLayout(card);form->setContentsMargins(21,21,21,21);form->setSpacing(13);
        QHBoxLayout *top=new QHBoxLayout;QLabel *mark=new QLabel(card);mark->setPixmap(VectorIcons::pixmap(icon,QColor("#00A99E"),26));top->addWidget(mark);top->addSpacing(5);top->addWidget(icon==QStringLiteral("layers")?translatedLabel("Driver archives","أرشيفات التعريفات","sectionTitle",card):translatedLabel("Device indexes","فهارس الأجهزة","sectionTitle",card),1);form->addLayout(top);Q_UNUSED(heading);
        form->addWidget(icon==QStringLiteral("layers")?translatedLabel("The .7z packages used for installation.","حزم .7z المستخدمة في التثبيت.","subText",card):translatedLabel("Small files that help find matching versions.","ملفات صغيرة للعثور على الإصدارات المتوافقة.","subText",card));Q_UNUSED(hint);*path=newLabel(QString(),"bodyText",card);(*path)->setMinimumHeight(52);(*path)->setWordWrap(true);(*path)->setTextInteractionFlags(Qt::TextSelectableByMouse);form->addWidget(*path);
        *browse=translatedButton("Choose folder","اختيار المجلد","outlineButton",card);(*browse)->setIcon(VectorIcons::icon(QStringLiteral("folder"),QColor("#258F9E"),18));form->addWidget(*browse);folders->addWidget(card,1);
    };
    folder(t("Driver archives","أرشيفات التعريفات"),t("The .7z packages used for installation.","حزم .7z المستخدمة في التثبيت."),QStringLiteral("layers"),&m_driversFolder,&m_driversBrowse);
    folder(t("Device indexes","فهارس الأجهزة"),t("Small files that help find matching versions.","ملفات صغيرة للعثور على الإصدارات المتوافقة."),QStringLiteral("search"),&m_indexesFolder,&m_indexesBrowse);layout->addLayout(folders);
    QWidget *actions=new SurfaceCard(body);actions->setObjectName(QStringLiteral("softSurface"));QVBoxLayout *box=new QVBoxLayout(actions);box->setContentsMargins(23,20,23,21);box->setSpacing(13);
    box->addWidget(translatedLabel("Keep your library ready","جهّز مكتبتك للتحديث","sectionTitle",actions));
    m_probeHint=translatedLabel("Download indexes, scan this computer, then get the packages you need.","نزّل الفهارس، وافحص جهازك، ثم احصل على الحزم المطلوبة.","subText",actions);box->addWidget(m_probeHint);
    QHBoxLayout *buttons=new QHBoxLayout;buttons->setSpacing(12);m_onlineButton=translatedButton("Download indexes & packages...","تنزيل الفهارس والحزم...","primaryButton",actions);m_onlineButton->setMinimumHeight(46);buttons->addWidget(m_onlineButton,1);
    m_probeButton=translatedButton("Scan driver library","فحص مكتبة التعريفات","outlineButton",actions);m_probeButton->setMinimumHeight(46);buttons->addWidget(m_probeButton,1);box->addLayout(buttons);
    m_probeStatus=newLabel(QString(),"subText",actions);m_probeStatus->setWordWrap(true);box->addWidget(m_probeStatus);m_toolsFolder=newLabel(QString(),"subText",actions);m_toolsFolder->hide();
    ActionButton *portable=new ActionButton(t("Create portable USB library...","إنشاء مكتبة USB محمولة..."),actions);portable->setObjectName(QStringLiteral("textLinkButton"));portable->setProperty("englishCopy",QStringLiteral("Create portable USB library..."));portable->setProperty("arabicCopy",QString::fromUtf8("إنشاء مكتبة USB محمولة..."));box->addWidget(portable);
    connect(portable,&QPushButton::clicked,this,[this](){if(m_engine->isBusy())return;QStringList packages;for(const DriverEntry &entry:m_catalog->entries())for(const DriverCandidate &candidate:entry.candidates)if(candidate.recommended && candidate.local && !packages.contains(candidate.packageSource,Qt::CaseInsensitive))packages<<candidate.packageSource;openPortableLibrary(this,m_driversPath,resolveIndexDirectory(m_indexesPath,m_driversPath),packages,m_arabic);});
    connect(m_onlineButton,&QPushButton::clicked,this,[this](){openOnlinePackages();});QCheckBox *autoCatalog=new QCheckBox(t("Check driver pack updates at startup","فحص تحديثات حزم التعريفات عند بدء التشغيل"),actions);autoCatalog->setChecked(m_settings->value(QStringLiteral("updates/checkAtStartup"),true).toBool());box->addWidget(autoCatalog);connect(autoCatalog,&QCheckBox::toggled,this,[this](bool checked){m_settings->setValue(QStringLiteral("updates/checkAtStartup"),checked);m_settings->sync();});layout->addWidget(actions);layout->addStretch(1);scroll->setWidget(body);return page;
}
QWidget *MainWindow::createSettings()
{
    QWidget *page=newPage();
    QVBoxLayout *outer=new QVBoxLayout(page);outer->setContentsMargins(0,0,0,20);QScrollArea *scroll=new QScrollArea(page);scroll->setWidgetResizable(true);scroll->setFrameShape(QFrame::NoFrame);outer->addWidget(scroll);QWidget *body=new QWidget(scroll);body->setObjectName(QStringLiteral("settingsBody"));
    QVBoxLayout *root=new QVBoxLayout(body);
    root->setContentsMargins(28,24,28,24);root->setSpacing(18);
    AccentCard *hero=new AccentCard(body);hero->setObjectName(QStringLiteral("settingsHero"));
    QHBoxLayout *heroLayout=new QHBoxLayout(hero);heroLayout->setContentsMargins(24,18,24,18);heroLayout->setSpacing(16);
    QLabel *heroIcon=new QLabel(hero);heroIcon->setPixmap(VectorIcons::pixmap(QStringLiteral("settings"),QColor("#C4FFF4"),34));heroLayout->addWidget(heroIcon);
    QVBoxLayout *heroText=new QVBoxLayout;heroText->setSpacing(3);
    m_settingsTitle=newLabel(QStringLiteral("General Settings"),"settingsHeroTitle",hero);heroText->addWidget(m_settingsTitle);
    heroText->addWidget(translatedLabel("Make it feel like yours.","اضبط البرنامج على ذوقك.","settingsHeroSubtitle",hero));heroLayout->addLayout(heroText,1);root->addWidget(hero);
    QGridLayout *sections=new QGridLayout;sections->setSpacing(16);sections->setColumnStretch(0,1);sections->setColumnStretch(1,1);root->addLayout(sections);
    auto sectionHeading=[this](const char *en,const char *ar,const char *symbol,QWidget *card){QWidget *header=new QWidget(card);header->setObjectName(QStringLiteral("settingsSectionHeader"));QHBoxLayout *line=new QHBoxLayout(header);line->setContentsMargins(0,0,0,4);line->setSpacing(10);QLabel *icon=new QLabel(header);icon->setPixmap(VectorIcons::pixmap(QString::fromLatin1(symbol),QColor("#13A99D"),22));line->addWidget(icon);line->addWidget(translatedLabel(en,ar,"sectionTitle",header),1);return header;};
    QWidget *panel=new SurfaceCard(page);panel->setObjectName(QStringLiteral("settingsCard"));panel->setProperty("tone",QStringLiteral("teal"));
    QGridLayout *grid=new QGridLayout(panel);
    grid->setContentsMargins(24,24,24,24);
    grid->setHorizontalSpacing(18);grid->setVerticalSpacing(16);
    grid->addWidget(sectionHeading("Appearance & language","المظهر واللغة","settings",panel),0,0,1,2);
    m_langLabel=newLabel(QStringLiteral("App language"),"bodyText",panel);
    m_language=new SearchableLanguageCombo(panel);
    const QList<QPair<QString,QString> > languages=OriginLanguages::available(QCoreApplication::applicationDirPath());
    for(int i=0;i<languages.size();++i)
        m_language->addItem(languages.at(i).second,languages.at(i).first);
    m_language->setObjectName(QStringLiteral("premiumCombo"));
    // SearchableLanguageCombo replaces the legacy platform combo list.
    m_language->setFocusPolicy(Qt::ClickFocus);
    m_language->setMinimumWidth(174);
    m_darkLabel=newLabel(QStringLiteral("Dark mode"),"bodyText",panel);
    m_darkSwitch=new ToggleSwitch(panel);
    m_motionLabel=newLabel(QStringLiteral("Reduce animations"),"bodyText",panel);
    m_motionSwitch=new ToggleSwitch(panel);
    grid->addWidget(m_langLabel,1,0);grid->addWidget(m_language,1,1);
    grid->addWidget(m_darkLabel,2,0);grid->addWidget(m_darkSwitch,2,1,Qt::AlignRight);
    grid->addWidget(m_motionLabel,3,0);grid->addWidget(m_motionSwitch,3,1,Qt::AlignRight);
    m_langNotice=newLabel(QString(),"subText",panel);
    m_langNotice->setWordWrap(true);
    grid->addWidget(translatedLabel("Interface size","حجم الواجهة","bodyText",panel),4,0);
    QComboBox *zoom=new SearchableLanguageCombo(panel);zoom->setObjectName(QStringLiteral("interfaceZoom"));
    const int current=UiScale::effectivePercent();const QSize base=qApp->desktop()->availableGeometry(this).size()*current/100;
    const int levels[]={75,85,90,100,110,125,150,175,200};for(int percent:levels){zoom->addItem(QString::number(percent)+QLatin1Char('%'),percent);QStandardItemModel *model=qobject_cast<QStandardItemModel*>(zoom->model());if(model && UiScale::fitPercent(percent,base)<percent)model->item(zoom->count()-1)->setEnabled(false);}
    if(zoom->findData(current)<0)zoom->addItem(QString::number(current)+QLatin1Char('%'),current);zoom->setCurrentIndex(zoom->findData(current));grid->addWidget(zoom,4,1);
    QPushButton *applyZoom=translatedButton("Apply size & reopen","تطبيق الحجم وإعادة الفتح","outlineButton",panel);applyZoom->setObjectName(QStringLiteral("applyInterfaceZoom"));grid->addWidget(applyZoom,5,0,1,2);
    connect(applyZoom,&QPushButton::clicked,this,[this,zoom](){if(m_engine->isBusy() || m_installation->isActive())return;const int percent=zoom->currentData().toInt();if(percent==UiScale::effectivePercent())return;
        if(!projectMessage(this,t("Interface size","حجم الواجهة"),t("Reopen the application to apply the selected size? Windows display settings stay as they are.","إعادة فتح البرنامج لتطبيق الحجم المختار؟ إعدادات عرض ويندوز ستبقى كما هي."),true,m_arabic))return;
        m_settings->setValue(QStringLiteral("ui/scalePercent"),percent);m_settings->sync();QStringList arguments;arguments<<QStringLiteral("--restart-after")<<QString::number(QCoreApplication::applicationPid())<<QStringLiteral("--no-splash");if(m_demo)arguments<<QStringLiteral("--demo");if(QProcess::startDetached(QCoreApplication::applicationFilePath(),arguments))qApp->quit();
    });
    grid->addWidget(m_langNotice,6,0,1,2);
    grid->setColumnStretch(0,1);
    sections->addWidget(panel,0,0);
    QWidget *preview=new SurfaceCard(page);preview->setObjectName(QStringLiteral("settingsCard"));preview->setProperty("tone",QStringLiteral("blue"));QGridLayout *previewGrid=new QGridLayout(preview);previewGrid->setContentsMargins(24,20,24,20);previewGrid->setHorizontalSpacing(20);previewGrid->setVerticalSpacing(12);
    QWidget *previewHeading=sectionHeading("Compatibility preview","معاينة التوافق","monitor",preview);QToolButton *previewHelp=new CommandToolButton(previewHeading);previewHelp->setObjectName(QStringLiteral("compatibilityHelp"));previewHelp->setText(QStringLiteral("?"));previewHelp->setFixedSize(36,36);previewHelp->setToolTip(t("About compatibility preview","عن معاينة التوافق"));qobject_cast<QHBoxLayout*>(previewHeading->layout())->addWidget(previewHelp);previewGrid->addWidget(previewHeading,0,0,1,2);
    connect(previewHelp,&QToolButton::clicked,this,[this](){projectMessage(this,t("Compatibility preview","معاينة التوافق"),t("This view helps technicians prepare an offline driver library for another Windows version or architecture. It does not change Windows. Press Apply to scan that target; installation is disabled in preview. Restore default returns to this computer and refreshes its matches.","تساعد هذه المعاينة الفني على تجهيز مكتبة تعريفات لويندوز أو معمارية مختلفة، دون تغيير ويندوز نفسه. اضغط تطبيق لفحص الهدف المختار؛ التثبيت معطّل في المعاينة. استعادة الافتراضي ترجع لهذا الجهاز وتحدّث مطابقة تعريفاته."),false,m_arabic);});
    QLabel *previewNote=translatedLabel("For technicians: inspect matches for another Windows version or architecture when preparing an offline library. Installation stays disabled in preview.","لفني الصيانة: افحص المطابقات لويندوز أو معمارية مختلفة عند تجهيز مكتبة دون إنترنت. التثبيت معطّل أثناء المعاينة.","subText",preview);previewNote->setWordWrap(true);previewGrid->addWidget(previewNote,1,0,1,2);
    m_targetVersion=new SearchableLanguageCombo(preview);m_targetVersion->setObjectName(QStringLiteral("targetWindows"));m_targetVersion->addItem(t("Current Windows","ويندوز الحالي"),0);
    const int versions[]={51,52,60,61,62,63,100,110};const char *names[]={"Windows XP","Windows XP x64","Windows Vista","Windows 7","Windows 8","Windows 8.1","Windows 10","Windows 11"};for(int i=0;i<8;++i)m_targetVersion->addItem(QString::fromLatin1(names[i]),versions[i]);
    m_targetArchitecture=new SearchableLanguageCombo(preview);m_targetArchitecture->setObjectName(QStringLiteral("targetArchitecture"));m_targetArchitecture->addItem(t("Current architecture","المعمارية الحالية"),0);m_targetArchitecture->addItem(QStringLiteral("32-bit (x86)"),32);m_targetArchitecture->addItem(QStringLiteral("64-bit (x64)"),64);
    previewGrid->addWidget(m_targetVersion,2,0);previewGrid->addWidget(m_targetArchitecture,2,1);previewGrid->setColumnStretch(0,1);previewGrid->setColumnStretch(1,1);
    QPushButton *applyTarget=translatedButton("Apply","تطبيق","primaryButton",preview),*resetTarget=translatedButton("Restore default","استعادة الافتراضي","outlineButton",preview);applyTarget->setObjectName(QStringLiteral("applyTarget"));applyTarget->setProperty("primaryAction",true);applyTarget->setIcon(VectorIcons::icon(QStringLiteral("check"),Qt::white,18));applyTarget->setIconSize(QSize(18,18));resetTarget->setObjectName(QStringLiteral("resetTarget"));previewGrid->addWidget(applyTarget,3,0);previewGrid->addWidget(resetTarget,3,1);previewGrid->setRowStretch(4,1);sections->addWidget(preview,1,0);
    auto applyCompatibility=[this](){if(m_engine->isBusy() || m_installation->isActive())return;int version=m_targetVersion->currentData().toInt(),architecture=m_targetArchitecture->currentData().toInt();if(version==51)architecture=32;else if(version==52 || version==110)architecture=64;if(m_targetVersion->currentData().toInt()==0 && m_targetArchitecture->currentData().toInt()==0)architecture=0;
        {QSignalBlocker blocked(m_targetArchitecture);m_targetArchitecture->setCurrentIndex(m_targetArchitecture->findData(architecture));}
        if(version==m_engine->previewVersion() && architecture==m_engine->previewArchitecture() && (!m_catalog->native() || m_catalog->nativeSnapshot().generation))return;
        m_engine->setPreviewTarget(version,architecture);m_catalog->invalidateNative();beginScan();};
    connect(applyTarget,&QPushButton::clicked,this,applyCompatibility);
    connect(resetTarget,&QPushButton::clicked,this,[this,applyCompatibility](){if(m_engine->isBusy() || m_installation->isActive())return;{QSignalBlocker v(m_targetVersion),a(m_targetArchitecture);m_targetVersion->setCurrentIndex(0);m_targetArchitecture->setCurrentIndex(0);}applyCompatibility();});
    QWidget *themes=new SurfaceCard(body);themes->setObjectName(QStringLiteral("settingsCard"));themes->setProperty("tone",QStringLiteral("violet"));QVBoxLayout *themeBox=new QVBoxLayout(themes);themeBox->setContentsMargins(24,20,24,20);themeBox->setSpacing(12);
    themeBox->addWidget(sectionHeading("Community themes","ثيمات المجتمع","layers",themes));m_themeName=newLabel(Theme::document().value(QStringLiteral("name")).toString(),"bodyText",themes);themeBox->addWidget(m_themeName);
    themeBox->addWidget(translatedLabel("Import a theme, preview it, or export a starting point for your own design.","استورد ثيمًا وعاينه، أو صدّر عينة لتصميم ثيمك الخاص.","subText",themes));
    QVBoxLayout *themeButtons=new QVBoxLayout;themeButtons->setSpacing(8);QPushButton *load=translatedButton("Import & preview...","استيراد ومعاينة...","primaryButton",themes),*save=translatedButton("Export theme...","تصدير الثيم...","outlineButton",themes),*reset=translatedButton("Restore default","استعادة الافتراضي","textLinkButton",themes);themeButtons->addWidget(load);themeButtons->addWidget(save);themeButtons->addWidget(reset);themeBox->addLayout(themeButtons);sections->addWidget(themes,0,1);
    connect(load,&QPushButton::clicked,this,&MainWindow::importTheme);connect(save,&QPushButton::clicked,this,&MainWindow::exportTheme);connect(reset,&QPushButton::clicked,this,[this](){Theme::setDocument(QJsonObject());m_settings->remove(QStringLiteral("theme/document"));m_settings->sync();m_themeName->setText(Theme::document().value(QStringLiteral("name")).toString());applyTheme();});
    QWidget *updates=new SurfaceCard(body);updates->setObjectName(QStringLiteral("settingsCard"));updates->setProperty("tone",QStringLiteral("amber"));QVBoxLayout *updateBox=new QVBoxLayout(updates);updateBox->setContentsMargins(24,20,24,20);updateBox->setSpacing(12);
    updateBox->addWidget(sectionHeading("Application updates","تحديثات البرنامج","refresh",updates));updateBox->addWidget(translatedLabel("Browse Beta releases and release notes on GitHub. The update check follows stable releases.","تصفّح إصدارات البيتا والتغييرات على GitHub. فحص التحديث يتابع الإصدارات المستقرة.","subText",updates));
    QPushButton *releases=translatedButton("Browse releases","تصفّح الإصدارات","primaryButton",updates);releases->setIcon(VectorIcons::icon(QStringLiteral("github"),Qt::white,20));releases->setProperty("projectReleaseLink",true);updateBox->addWidget(releases);connect(releases,&QPushButton::clicked,this,[](){QDesktopServices::openUrl(QUrl(QStringLiteral("https://github.com/")+AppUpdate::projectRepository()+QStringLiteral("/releases")));});
    QPushButton *checkUpdate=translatedButton("Check application update...","البحث عن تحديث البرنامج...","outlineButton",updates);updateBox->addWidget(checkUpdate);updateBox->addStretch();sections->addWidget(updates,1,1);
    connect(checkUpdate,&QPushButton::clicked,this,[this](){if(m_engine->isBusy() || m_installation->isActive()){projectMessage(this,t("Operation in progress","عملية قيد التنفيذ"),t("Finish the active operation before updating the application.","أكمل العملية الحالية قبل تحديث البرنامج."));return;}AppUpdate::check(this,AppUpdate::projectRepository());});
    m_settingsHint=newLabel(QStringLiteral("Preferences are saved for your Windows account."),"subText",body);
    root->addWidget(m_settingsHint);
    root->addStretch(1);
    scroll->setWidget(body);return page;
}
void MainWindow::importTheme(){
    if(m_engine->isBusy() || m_installation->isActive())return;
    const QString path=QFileDialog::getOpenFileName(this,t("Import community theme","استيراد ثيم"),QDir(QCoreApplication::applicationDirPath()).filePath(QStringLiteral("themes")),QStringLiteral("Theme (*.sdi-theme.json *.json)"));if(path.isEmpty())return;
    const QJsonObject previous=Theme::document();
    try{
        const QJsonObject candidate=Theme::readPack(path);Theme::setDocument(candidate);applyTheme();
        RoundedDialog preview(this);preview.setStyleSheet(Theme::styleSheet(m_dark));preview.setMinimumWidth(520);QVBoxLayout *frame=new QVBoxLayout(&preview);frame->setContentsMargins(0,0,0,0);frame->setSpacing(0);frame->addWidget(new DialogHeader(t("Theme preview","معاينة الثيم"),QStringLiteral("layers"),&preview,m_arabic));
        QWidget *body=new QWidget(&preview);QVBoxLayout *layout=new QVBoxLayout(body);layout->setContentsMargins(26,24,26,24);layout->setSpacing(15);QLabel *title=newLabel(candidate.value(QStringLiteral("name")).toString(),"dialogTitle",body);title->setTextFormat(Qt::PlainText);layout->addWidget(title);
        QLabel *author=newLabel(t("Designed by: ","تصميم: ")+candidate.value(QStringLiteral("author")).toString(),"subText",body);author->setTextFormat(Qt::PlainText);layout->addWidget(author);
        AccentCard *sample=new AccentCard(body);sample->setMinimumHeight(84);layout->addWidget(sample);layout->addWidget(newLabel(t("The application behind this window shows the selected palette. Apply it to keep it, or cancel to restore your previous theme.","البرنامج خلف هذه النافذة يعرض ألوان الثيم. طبّقه للاحتفاظ به، أو ألغِ للرجوع للثيم السابق."),"bodyText",body));
        QHBoxLayout *buttons=new QHBoxLayout;ActionButton *cancel=new ActionButton(t("Cancel","إلغاء"),body),*accept=new ActionButton(t("Apply theme","تطبيق الثيم"),body);cancel->setObjectName(QStringLiteral("outlineButton"));accept->setObjectName(QStringLiteral("primaryButton"));cancel->setDefault(true);buttons->addWidget(cancel);buttons->addWidget(accept);layout->addLayout(buttons);frame->addWidget(body);connect(cancel,&QPushButton::clicked,&preview,&QDialog::reject);connect(accept,&QPushButton::clicked,&preview,&QDialog::accept);
        if(preview.exec()==QDialog::Accepted){m_settings->setValue(QStringLiteral("theme/document"),QJsonDocument(candidate).toJson(QJsonDocument::Compact));m_settings->sync();m_themeName->setText(candidate.value(QStringLiteral("name")).toString());}
        else {Theme::setDocument(previous);applyTheme();}
    }catch(const std::exception &error){Theme::setDocument(previous);applyTheme();projectMessage(this,t("Theme could not be loaded","تعذّر تحميل الثيم"),QString::fromUtf8(error.what()));}
}
void MainWindow::exportTheme(){
    QString path=QFileDialog::getSaveFileName(this,t("Export community theme","تصدير الثيم"),Theme::document().value(QStringLiteral("id")).toString()+QStringLiteral(".sdi-theme.json"),QStringLiteral("Theme (*.sdi-theme.json)"));if(path.isEmpty())return;if(!path.endsWith(QStringLiteral(".json"),Qt::CaseInsensitive))path+=QStringLiteral(".sdi-theme.json");
    try{Theme::writePack(path,Theme::document());}catch(const std::exception &error){projectMessage(this,t("Cannot save theme","تعذّر حفظ الثيم"),QString::fromUtf8(error.what()));}
}
void MainWindow::chooseDriversFolder()
{
    if(m_engine->isBusy())return;
    const QString f=QFileDialog::getExistingDirectory(this,t("Choose drivers folder","اختر مجلد التعريفات"),m_driversPath);
    if(!f.isEmpty()){m_catalog->invalidateNative();m_driversPath=f;m_settings->setValue(QStringLiteral("driversPath"),f);updateOffline();}
}
void MainWindow::chooseIndexesFolder()
{
    if(m_engine->isBusy())return;
    const QString f=QFileDialog::getExistingDirectory(this,t("Choose indexes folder","اختر مجلد الفهارس"),m_indexesPath);
    if(!f.isEmpty()){m_catalog->invalidateNative();m_indexesPath=f;m_settings->setValue(QStringLiteral("indexesPath"),f);updateOffline();}
}
void MainWindow::onNativeSnapshot(const sdiomodern::Snapshot &snapshot)
{
    if(m_scanOverlay)m_scanOverlay->setStage(m_arabic?QString::fromUtf8("تجهيز قائمة التعريفات..."):
        QStringLiteral("Preparing the driver list..."));
    const qint64 engineMs=m_scanClock->isValid()?m_scanClock->elapsed():0;
    QElapsedTimer uiTime;uiTime.start();
    m_catalog->replaceNative(snapshot);
    const qint64 listingMs=uiTime.elapsed();
    if(m_scanOverlay)m_scanOverlay->finish();
    m_drivers->setBusy(false);m_titleBar->setRefreshEnabled(true);m_probeButton->setEnabled(true);
    std::size_t candidates=0,local=0;
    for(std::size_t i=0;i<snapshot.devices.size();++i)
        for(std::size_t j=0;j<snapshot.devices[i].candidates.size();++j){++candidates;if(snapshot.devices[i].candidates[j].local)++local;}
    const QString text=QStringLiteral("Native Origin: %1 devices, %2 actual candidates, %3 local. %4")
        .arg(static_cast<qulonglong>(snapshot.devices.size())).arg(static_cast<qulonglong>(candidates)).arg(static_cast<qulonglong>(local))
        .arg(snapshot.capabilities.localInstall?QStringLiteral("Experimental installation available after explicit confirmation."):
             QStringLiteral("Matching-only build; installation locked."));
    const QString timing=QStringLiteral("Origin scan: %1 ms; Qt list: %2 ms. ").arg(engineMs).arg(listingMs);
    m_drivers->setStatus((m_engine->previewVersion() || m_engine->previewArchitecture())?t("Compatibility preview · Installation is disabled.","معاينة التوافق · التثبيت معطّل."):(m_lastInstallation.rebootRequired?t("Windows requested a restart. Some installed versions are reported only after reboot.","طلب ويندوز إعادة تشغيل الجهاز؛ بعض الإصدارات المثبّتة لا تظهر إلا بعد إعادة التشغيل."):QString()));m_probeStatus->setText(t("Scan complete. Your driver matches are ready.","اكتمل الفحص. مطابقة التعريفات جاهزة."));
    Q_UNUSED(text);Q_UNUSED(timing);
    m_titleBar->setStatus(QStringLiteral("ORIGIN NATIVE - REAL MATCHES"));refreshLibraryNotice();
    // Show scan results before a separate expensive SetupAPI diagnostics enumeration.
    // In particular, avoid blocking the very first paint of a large offline list.
    QTimer::singleShot(250,this,[this](){if(!m_engine->isBusy())updateDiagnostics();});
    if(m_closeWhenIdle)QTimer::singleShot(0,this,SLOT(close()));
}
void MainWindow::onNativeProgress(const sdiomodern::Event &event)
{
    if(m_installation->isLive())m_installation->applyNativeEvent(event);
    else {
        const QString stage=QString::fromStdWString(event.message);
        if(m_scanOverlay && m_scanOverlay->isActive())m_scanOverlay->setStage(stage);
        m_drivers->setStatus(stage);
    }
}
void MainWindow::onNativeFinished(const sdiomodern::InstallReport &report)
{
    m_lastInstallation=report;
    m_installation->finishNative(report);m_catalog->invalidateNative();
    m_drivers->setBusy(false);m_titleBar->setRefreshEnabled(true);m_probeButton->setEnabled(true);
    m_drivers->setStatus(QStringLiteral("Installation session finished. Scan again before making another selection."));
    if(report.error.empty() && report.rebootRequired && !report.cancelled && m_drivers->rebootAfter() && !m_closeWhenIdle){
        bool succeeded=!report.items.empty();for(const auto &item:report.items)if(item.outcome!=sdiomodern::Outcome::Success)succeeded=false;
        if(succeeded)restartPrompt(this);
    }
    if(m_closeWhenIdle)QTimer::singleShot(0,this,SLOT(close()));
    else if(!report.items.empty() || report.restoreFailed){QTimer *refresh=new QTimer(this);refresh->setObjectName(QStringLiteral("postInstallRefresh"));refresh->setInterval(40);connect(refresh,&QTimer::timeout,this,[this,refresh](){if(m_demo || m_closeWhenIdle){refresh->stop();refresh->deleteLater();return;}if(m_engine->isBusy())return;refresh->stop();refresh->deleteLater();m_catalog->replace(QVector<DeviceInfo>(),false);m_drivers->setBusy(true);m_titleBar->setRefreshEnabled(false);m_probeButton->setEnabled(false);m_drivers->setStatus(t("Refreshing matches after installation...","تحديث المطابقة بعد التثبيت..."));m_scanClock->restart();m_engine->requestInventory(false);});refresh->start();}
}
void MainWindow::onEngineError(const QString &reason)
{
    m_pendingInf.clear();
    m_lastEngineError=reason;
    if(m_scanOverlay)m_scanOverlay->finish();
    if(!m_demo)m_catalog->invalidateNative();
    if(m_installation->isLive())m_installation->failNative(reason);
    m_drivers->setBusy(false);m_titleBar->setRefreshEnabled(true);m_probeButton->setEnabled(true);
    m_drivers->setStatus(reason);m_probeStatus->setText(reason);m_titleBar->setStatus(QStringLiteral("ENGINE NOT READY"));
    if(!m_closeWhenIdle)projectMessage(this,QStringLiteral("Native engine stopped"),reason);
    else QTimer::singleShot(0,this,SLOT(close()));
}

void MainWindow::updateOffline()
{
    refreshLibraryNotice();
    m_driversFolder->setText(m_driversPath);m_indexesFolder->setText(m_indexesPath);
    m_toolsFolder->setText(QDir(m_toolsPath).exists()?m_toolsPath:QStringLiteral("Not present (optional)"));
    const QDir drivers(m_driversPath),indexes(resolveIndexDirectory(m_indexesPath,m_driversPath));
    const int count=drivers.exists()?drivers.entryList(QStringList()<<QStringLiteral("*.7z"),QDir::Files|QDir::Hidden|QDir::System).size():0;
    const int idx=indexes.exists()?indexes.entryList(QStringList()<<QStringLiteral("*.bin"),QDir::Files|QDir::Hidden|QDir::System).size():0;
    const bool ready=drivers.exists() && indexes.exists() && count>0 && idx>0;
    m_offlineStatus->setText(t("%1 packages  ·  %2 indexes  ·  %3","%1 حزمة  ·  %2 فهرس  ·  %3").arg(count).arg(idx).arg(ready?t("Library ready","المكتبة جاهزة"):t("Choose your folders to get started","اختر المجلدات للبدء")));
    m_offlineStatus->setText(m_offlineStatus->text()+QStringLiteral(" · ")+UiLanguage::text(libraryFamily(m_driversPath,indexes.absolutePath())));
    m_probeHint->setText(t("Download indexes, scan this computer, then get the packages you need.","نزّل الفهارس، وافحص جهازك، ثم احصل على الحزم المطلوبة."));

}
QString MainWindow::regValue(const QString &path,const QString &value) const
{
#ifdef Q_OS_WIN
    QSettings reg(path,QSettings::NativeFormat);
    const QString result=reg.value(value).toString().trimmed();
    return result.isEmpty()?QStringLiteral("—"):result;
#else
    Q_UNUSED(path);Q_UNUSED(value);
    return QStringLiteral("—");
#endif
}
QString MainWindow::firstByClass(const QString &needle) const
{
    const QVector<DriverEntry> &entries=m_catalog->entries();
    for(int i=0;i<entries.size();++i){
        const DeviceInfo &d=entries.at(i).device;
        if(d.deviceClass.contains(needle,Qt::CaseInsensitive))return d.name;
    }
    return QStringLiteral("—");
}
void MainWindow::updateDiagnostics()
{
    QVector<DeviceInfo> devices;
    const QVector<DriverEntry> &entries=m_catalog->entries();
    devices.reserve(entries.size());
    for(int i=0;i<entries.size();++i)devices.append(entries.at(i).device);
    m_diagnosticFacts=SystemProfile::collect(devices,m_demo);renderDiagnostics();
}
void MainWindow::renderDiagnostics()
{
    for(QMap<QString,QLabel*>::const_iterator it=m_diagValues.constBegin();it!=m_diagValues.constEnd();++it){
        const QString raw=m_diagnosticFacts.value(it.key(),QStringLiteral("—"));
        const QString value=SystemProfile::displayValue(it.key(),raw);
        it.value()->setProperty("rawDiagnosticValue",raw);
        it.value()->setLayoutDirection(UiLanguage::isRtl()?Qt::RightToLeft:Qt::LeftToRight);
        it.value()->setAlignment(Qt::AlignAbsolute|(UiLanguage::isRtl()?Qt::AlignRight:Qt::AlignLeft)|Qt::AlignVCenter);
        it.value()->setText(value);
        it.value()->setToolTip(value);
    }
    if(m_memoryUsageBar){bool ok=false;const int usage=m_diagnosticFacts.value(QStringLiteral("memoryload")).section(QLatin1Char('%'),0,0).toInt(&ok);m_memoryUsageBar->setVisible(ok && usage>=0 && usage<=100);if(ok)m_memoryUsageBar->setValue(usage);}
}
void MainWindow::translateDiagnostics()
{
    struct Caption {const char *key;const char *en;const char *ar;};
    static const Caption captions[]={
        {"os","Operating system","نظام التشغيل"},{"build","Build / Windows release","بنية ويندوز وإصداره"},
        {"architecture","System architecture","معمارية النظام"},{"computer","Computer name","اسم الكمبيوتر"},
        {"maker","System manufacturer","الشركة المصنّعة"},{"model","Model","موديل الجهاز"},
        {"cpu","Processor","المعالج"},{"logical","Logical processors","المعالجات المنطقية"},
        {"clock","Reported CPU clock","تردد المعالج المسجّل"},{"memory","Installed RAM","الذاكرة المثبّتة"},
        {"memoryfree","Available RAM","الذاكرة المتاحة"},{"memoryload","Memory usage","استخدام الذاكرة"},
        {"board","Motherboard model","موديل اللوحة الأم"},{"boardmaker","Motherboard manufacturer","الشركة المصنّعة للوحة"},
        {"boardversion","Motherboard revision","إصدار اللوحة الأم"},{"bios","BIOS version","إصدار BIOS"},
        {"biosvendor","BIOS vendor","الشركة المصنّعة لـ BIOS"},{"biosdate","BIOS release date","تاريخ BIOS"},
        {"secureboot","Secure Boot","التمهيد الآمن"},{"uptime","System uptime","مدة التشغيل"},
        {"power","Power state","حالة الطاقة"},{"graphics","Graphics adapters","كروت الشاشة"},
        {"resolution","Screen resolution","دقة الشاشة"},{"storage","Storage controllers / drives","وحدات التخزين والمتحكمات"},
        {"volume","Windows system volume","قرص نظام ويندوز"},{"network","Network adapters","محولات الشبكة"},
        {"wireless","Wireless interfaces","الشبكات اللاسلكية"},{"audio","Audio devices","أجهزة الصوت"},
        {"bluetooth","Bluetooth devices","أجهزة بلوتوث"},{"usb","USB controllers","متحكمات USB"},
        {"battery","Battery / ACPI","البطارية وACPI"},{"tpm","TPM (PnP device)","شريحة TPM"},
        {"issues","Devices / PnP warnings","الأجهزة وتحذيرات ويندوز"},
        {"refresh","Display refresh rate","معدل تحديث الشاشة"}
    };
    for(unsigned i=0;i<sizeof(captions)/sizeof(captions[0]);++i){
        const QString key=QString::fromLatin1(captions[i].key);
        QLabel *item=m_diagCaptions.value(key,0);
        if(item)item->setText(m_arabic?QString::fromUtf8(captions[i].ar):QString::fromLatin1(captions[i].en));
    }
    struct Section {const char *key;const char *en;const char *ar;};
    static const Section sections[]={
        {"system","Windows & computer","ويندوز والكمبيوتر"},
        {"performance","Processor & memory","المعالج والذاكرة"},
        {"firmware","Motherboard & firmware","اللوحة الأم والبرامج الثابتة"},
        {"hardware","Devices & connectivity","الأجهزة والاتصال"}
    };
    for(unsigned i=0;i<sizeof(sections)/sizeof(sections[0]);++i){
        QLabel *item=m_diagSections.value(QString::fromLatin1(sections[i].key),0);
        if(item)item->setText(m_arabic?QString::fromUtf8(sections[i].ar):QString::fromLatin1(sections[i].en));
    }
    if(m_diagnosticsSubtitle)m_diagnosticsSubtitle->setText(m_arabic?
       QString::fromUtf8("اللوحة الأم وBIOS والمكونات وبيانات ويندوز · قراءة محلية فقط"):
       QStringLiteral("Motherboard, firmware, components and Windows information · local / read only"));
}
void MainWindow::updateLanguage(int index)
{
    if(index<0)return;
    m_languageCode=m_language->itemData(index).toString();
    m_arabic=(m_languageCode==QStringLiteral("arabic"));
    UiLanguage::setLanguage(m_languageCode);
    saveSettings();translateUi();
}
void MainWindow::updateDark(bool enabled)
{ m_dark=enabled;saveSettings();applyTheme(); }
void MainWindow::updateReduceMotion(bool enabled)
{
    m_reduceMotion=enabled;saveSettings();
    qApp->setProperty("reduceMotion",enabled);
    m_sidebar->setAnimations(!enabled);
    m_drivers->setAnimations(!enabled);
    m_toolsDrawer->setMotionEnabled(!enabled);
    m_darkSwitch->setMotionEnabled(!enabled);
    m_motionSwitch->setMotionEnabled(!enabled);
}
void MainWindow::saveSettings()
{
    m_settings->setValue(QStringLiteral("installation/restorePointRequired"),m_drivers->createRestorePoint());m_settings->setValue(QStringLiteral("installation/rebootAfter"),m_drivers->rebootAfter());
    m_settings->setValue(QStringLiteral("language"),m_languageCode);
    m_settings->setValue(QStringLiteral("dark"),m_dark);
    m_settings->setValue(QStringLiteral("reduceMotion"),m_reduceMotion);
    m_settings->sync();
}
void MainWindow::applyTheme()
{
    setProperty("darkMode",m_dark);
    setStyleSheet(Theme::styleSheet(m_dark));
    m_drivers->setDark(m_dark);
    m_darkSwitch->setDark(m_dark);
    m_motionSwitch->setDark(m_dark);
}
void MainWindow::translateUi()
{
    setWindowTitle(t("Snappy Driver Installer","مدير تعريفات الأجهزة"));
    setProperty("rtl",UiLanguage::isRtl());applyTheme();
    for(QLabel *label:findChildren<QLabel*>())if(label->property("englishCopy").isValid())label->setText(label->property(m_arabic?"arabicCopy":"englishCopy").toString());
    for(QAbstractButton *button:findChildren<QAbstractButton*>())if(button->property("englishCopy").isValid())button->setText(button->property(m_arabic?"arabicCopy":"englishCopy").toString());
    m_targetVersion->setItemText(0,t("Current Windows","ويندوز الحالي"));m_targetArchitecture->setItemText(0,t("Current architecture","المعمارية الحالية"));
    static_cast<AboutPage*>(m_pages->widget(5))->setArabic(m_arabic);
    m_sidebar->setArabic(m_arabic);
    m_sidebar->applyOriginLanguage(QCoreApplication::applicationDirPath(),m_languageCode);
    m_toolsDrawer->setArabic(m_arabic);
    m_titleBar->setArabic(m_arabic);
    m_titleBar->setStatus(m_demo?QStringLiteral("DEMO MODE"):m_engine->canInstall()?QStringLiteral("NATIVE INSTALL - TEST ONLY"):QStringLiteral("NATIVE OFFLINE MATCHER"));
    m_drivers->setArabic(m_arabic);
    m_drivers->applyOriginLanguage(QCoreApplication::applicationDirPath(),m_languageCode);
    m_installation->setArabic(m_arabic);
    m_installation->setAvailability(m_engine->canInstall(),m_demo);
    m_diagnosticsTitle->setText(t("System diagnostics","تشخيص النظام"));
    translateDiagnostics();renderDiagnostics();
    m_offlineTitle->setText(t("Driver packages & updates","حزم التعريفات والتحديثات"));
    m_probeButton->setText(t("Scan driver library","فحص مكتبة التعريفات"));
    m_onlineButton->setText(t("Download indexes & packages...","تنزيل الفهارس والحزم..."));
    m_onlineButton->setEnabled(!m_demo && !m_engine->isBusy());
    m_settingsTitle->setText(t("General settings","الإعدادات العامة"));
    m_langLabel->setText(t("App language","لغة التطبيق"));
    if(m_langNotice)m_langNotice->setText(t("All interface languages are included for offline use. Community translations can be improved in the project repository.","كل لغات الواجهة مدمجة للعمل دون إنترنت. يمكن تحسين الترجمات المجتمعية من مستودع المشروع."));
    const QString appDir=QCoreApplication::applicationDirPath();
    const QString originalLang=OriginLanguages::text(appDir,m_languageCode,QStringLiteral("STR_LANG"));
    if(!m_arabic && !originalLang.isEmpty())m_langLabel->setText(originalLang);
    m_darkLabel->setText(t("Dark mode","الوضع الداكن"));
    m_motionLabel->setText(t("Reduce animations","تقليل الحركات"));
    m_settingsHint->setText(t("Defaults are built in. Your preferences are saved in your Windows account without a configuration file beside the EXE.",
      "الإعدادات الافتراضية مدمجة. اختياراتك محفوظة لحساب ويندوز بدون ملف إعدادات بجوار EXE."));
    m_driversBrowse->setText(t("Browse...","اختيار..."));
    m_indexesBrowse->setText(t("Browse...","اختيار..."));
    updateOffline();
    UiLanguage::refresh();
}

QStringList MainWindow::matchingPackages()const{QStringList result;for(const DriverEntry &entry:m_catalog->entries())for(const DriverCandidate &candidate:entry.candidates)if(candidate.valid && candidate.engineToken && !candidate.packageSource.isEmpty() && !result.contains(candidate.packageSource,Qt::CaseInsensitive))result<<candidate.packageSource;return result;}
void MainWindow::checkLibraryUpdates(){
    if(m_demo || !m_updateCheck || m_updateCheck->isRunning())return;
    m_updateCheck->checking=true;m_updateCheck->drivers=m_driversPath;m_updateCheck->indexes=resolveIndexDirectory(m_indexesPath,m_driversPath);m_updateCheck->metadataUrl=OnlinePackages::officialMetadataUrl(libraryFamily(m_driversPath,m_updateCheck->indexes));m_updateCheck->imported.clear();m_updateCheck->stop.store(false);m_updateError.clear();m_drivers->setPackageUpdateStatus(-1,-1,m_updateCheck->metadataUrl.host().contains(QStringLiteral("sdi-tool"))?QStringLiteral("SDI"):QStringLiteral("SDIO"),QString(),true);m_updateCheck->start();
}
void MainWindow::refreshLibraryNotice(){
    const QString family=libraryFamily(m_driversPath,resolveIndexDirectory(m_indexesPath,m_driversPath));const QUrl source=OnlinePackages::officialMetadataUrl(family);const QString displaySource=family==QStringLiteral("SDI")?QStringLiteral("SDI"):QStringLiteral("SDIO");
    if(m_updateCatalog.files.isEmpty() || m_updateCatalogSource!=source){m_drivers->setPackageUpdateStatus(-1,-1,displaySource,m_updateError.isEmpty()?t("Check the official library catalog for new packages and indexes.","افحص كتالوج المكتبة الرسمي للبحث عن حزم وفهارس جديدة."):t("Could not check the catalog. Your offline library is still available; retry when connected.","تعذّر فحص الكتالوج. مكتبتك الأوفلاين ما زالت متاحة؛ أعد المحاولة عند الاتصال."),m_updateCheck && m_updateCheck->isRunning());return;}
    const auto state=OnlinePackages::availability(m_updateCatalog,m_driversPath,resolveIndexDirectory(m_indexesPath,m_driversPath),matchingPackages());int packs=0,indexes=0;for(int i=0;i<state.size();++i)if(state[i].update){if(m_updateCatalog.files[i].kind==OnlinePackages::Driver)++packs;else if(m_updateCatalog.files[i].kind==OnlinePackages::Index)++indexes;}
    m_drivers->setPackageUpdateStatus(packs,indexes,displaySource,m_updateError.isEmpty()?QString():t("Could not refresh. Showing the last catalog from this session.","تعذّر التحديث؛ تُعرض آخر قائمة من هذه الجلسة."),m_updateCheck && m_updateCheck->isRunning());
}
void MainWindow::openOnlinePackages(const QString &package){
    if(m_demo || m_engine->isBusy() || m_installation->isActive())return;
    QStringList needed=matchingPackages();if(!package.isEmpty() && !needed.contains(package,Qt::CaseInsensitive))needed<<package;
    OnlineDialog dialog(m_driversPath,resolveIndexDirectory(m_indexesPath,m_driversPath),needed,m_arabic,this);
    if(!m_updateCatalog.files.isEmpty() && m_updateCatalogSource==dialog.catalogSource())dialog.setCatalog(m_updateCatalog);else dialog.refreshCatalog();dialog.exec();
    if(!dialog.catalog().files.isEmpty() && dialog.catalogSource()==OnlinePackages::officialMetadataUrl(libraryFamily(m_driversPath,resolveIndexDirectory(m_indexesPath,m_driversPath)))){m_updateCatalog=dialog.catalog();m_updateCatalogSource=dialog.catalogSource();m_updateError.clear();}
    if(!dialog.lastError().isEmpty()){m_lastEngineError=dialog.lastError();if(dialog.catalogSource()==OnlinePackages::officialMetadataUrl(libraryFamily(m_driversPath,resolveIndexDirectory(m_indexesPath,m_driversPath))))m_updateError=dialog.lastError();}refreshLibraryNotice();
    if(dialog.filesChanged()){
        m_catalog->invalidateNative();updateOffline();
        m_drivers->setStatus(t("Refreshing matches after downloading packages...","تحديث المطابقة بعد تنزيل الحزم..."));beginScan();
    }
}
