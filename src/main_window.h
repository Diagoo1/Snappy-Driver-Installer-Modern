#ifndef SDIOMODERN_MAIN_WINDOW_H
#define SDIOMODERN_MAIN_WINDOW_H

#include <QWidget>
#include <QVector>
#include <QMap>
#include "device_types.h"
#include "online_packages.h"
#include "../native/origin_api.h"
class QWidget;
class QLabel;
class QStackedWidget;
class QComboBox;
class QPushButton;
class QSettings;
class QCloseEvent;
class QEvent;
class QByteArray;
class TitleBar;
class Sidebar;
class EngineBridge;
class DriverCatalog;
class DriverWorkspace;
class InstallationWorkspace;
class ToggleSwitch;
class ToolsDrawer;
class QResizeEvent;
class ScanOverlay;
class QElapsedTimer;

class MainWindow : public QWidget
{
    Q_OBJECT
public:
    explicit MainWindow(bool demo,QWidget *parent=0);
    ~MainWindow();
    void beginScan();

protected:
    void dragEnterEvent(class QDragEnterEvent*) Q_DECL_OVERRIDE;
    void dropEvent(class QDropEvent*) Q_DECL_OVERRIDE;
    void closeEvent(QCloseEvent *event) Q_DECL_OVERRIDE;
    void resizeEvent(QResizeEvent *event) Q_DECL_OVERRIDE;
    void changeEvent(QEvent *event) Q_DECL_OVERRIDE;
    void paintEvent(QPaintEvent *event) Q_DECL_OVERRIDE;
#ifdef Q_OS_WIN
    bool nativeEvent(const QByteArray &eventType,void *message,long *result) Q_DECL_OVERRIDE;
#endif
private slots:
    void onInventoryReady(const QVector<DeviceInfo> &devices,const QString &status);
    void navigate(int index);
    void openToolsMenu();
    void dispatchTool(const QString &id);
    void openSnapshot();
    void loadSnapshot();
    void loadSnapshotFile(const QString &file);
    void openExtract();
    void openOptions();
    void beginInstallation();
    void chooseDriversFolder();
    void chooseIndexesFolder();
    void onNativeSnapshot(const sdiomodern::Snapshot &snapshot);
    void onNativeProgress(const sdiomodern::Event &event);
    void onNativeFinished(const sdiomodern::InstallReport &report);
    void onEngineError(const QString &reason);
    void updateLanguage(int index);
    void updateDark(bool enabled);
    void refreshSystemTheme();
    void updateReduceMotion(bool enabled);

private:
    void checkLibraryUpdates();
    void evaluateLibrarySchedule(bool startup=false);
    QString updateLibraryFamily()const;
    QWidget *createLibraryUpdateSettings();
    void refreshLibraryNotice();
    QStringList matchingPackages()const;
    void openOnlinePackages(const QString &package=QString(),bool forceRefresh=false);
    void importTheme();
    void exportTheme();
    QWidget *createDiagnostics();
    QWidget *createOffline();
    QWidget *createSettings();
    QWidget *createAbout();
    QWidget *newPage();
    void addDiagnosticCard(class QGridLayout *layout,int row,int col,const QString &icon,
                           const QString &label,const QString &key);
    void updateDiagnostics();
    void translateDiagnostics();
    void renderDiagnostics();
    void updateOffline();
    void applyTheme();
    void translateUi();
    void saveSettings();
    QString t(const char *english,const char *arabic) const;
    QLabel *translatedLabel(const char *english,const char *arabic,const char *role,QWidget *parent);
    QPushButton *translatedButton(const char *english,const char *arabic,const char *role,QWidget *parent);
    QString regValue(const QString &path,const QString &value) const;
    QString firstByClass(const QString &needle) const;

    sdiomodern::InstallReport m_lastInstallation;
    QString m_lastEngineError;
    QString m_pendingInf;
    bool m_locatePendingInf=false;
    bool m_demo,m_arabic,m_dark,m_reduceMotion,m_closeWhenIdle;
    int m_currentPage;
    QString m_driversPath,m_indexesPath,m_toolsPath;
    QSettings *m_settings;
    Sidebar *m_sidebar;
    TitleBar *m_titleBar;
    ToolsDrawer *m_toolsDrawer;
    QStackedWidget *m_pages;
    EngineBridge *m_engine;
    ScanOverlay *m_scanOverlay;
    QElapsedTimer *m_scanClock;
    DriverCatalog *m_catalog;
    DriverWorkspace *m_drivers;
    InstallationWorkspace *m_installation;
    QLabel *m_diagnosticsTitle,*m_diagnosticsSubtitle,*m_offlineTitle,*m_offlineStatus,*m_driversFolder,*m_indexesFolder,*m_toolsFolder;
    QLabel *m_settingsTitle,*m_settingsHint,*m_langLabel,*m_darkLabel,*m_motionLabel,*m_langNotice;
    class OnlineWorker *m_updateCheck=0;
    bool m_updateSessionChecked=false;
    OnlinePackages::Catalog m_updateCatalog;
    QUrl m_updateCatalogSource;
    QString m_updateError;
    class SharingRestore *m_sharingRestore=0;
    QLabel *m_themeName;
    QString m_languageCode,m_themeMode;
    class QCheckBox *m_autoTheme=0;
    QComboBox *m_language,*m_targetVersion,*m_targetArchitecture;
    ToggleSwitch *m_darkSwitch,*m_motionSwitch;
    QPushButton *m_driversBrowse,*m_indexesBrowse,*m_probeButton,*m_onlineButton;
    QLabel *m_probeStatus,*m_probeHint;
    QMap<QString,QString> m_diagnosticFacts;
    class QProgressBar *m_memoryUsageBar=0;
    class WindowMotion *m_windowMotion=0;
    QMap<QString,QLabel*> m_diagValues;
    QMap<QString,QLabel*> m_diagCaptions,m_diagSections;
};
#endif
