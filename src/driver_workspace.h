#ifndef SDIOMODERN_DRIVER_WORKSPACE_H
#define SDIOMODERN_DRIVER_WORKSPACE_H

#include <QWidget>
#include <QElapsedTimer>
#include <QVector>
#include <QStringList>
#include "device_types.h"

class DriverCatalog;
struct DriverCandidate;
class QTreeWidget;
class QTreeWidgetItem;
class QLabel;
class QLineEdit;
class QPushButton;
class QCheckBox;
class QToolButton;
class PremiumPopover;
class ToggleSwitch;
class GradientButton;
class QResizeEvent;
class QTimer;

class DriverWorkspace : public QWidget
{
    Q_OBJECT
public:
    explicit DriverWorkspace(DriverCatalog *catalog, QWidget *parent = 0);
    void setArabic(bool arabic);
    void applyOriginLanguage(const QString &directory,const QString &code);
    void setDark(bool dark);
    void setDemo(bool demo);
    void setBusy(bool busy);
    void setAnimations(bool enabled);
    void setStatus(const QString &text);
    void showEntry(int row);
    int selectedDrivers() const;
    int selectedItems() const;
    QStringList selectedNames() const;
    bool createRestorePoint() const;
    bool rebootAfter() const;
    void openInstallOptions();
    void setInstallationPreferences(bool restorePoint,bool rebootAfter);
    void applyLegacyDisplay(int filters,bool expert,bool packageNames);

signals:
    void scanRequested();
    void installationRequested();
    void utilitiesRequested();
    void snapshotRequested();
    void extractRequested();
    void packageDownloadRequested(const QString &package);
    void candidateExtractionRequested(int row,int candidate);
    void infViewRequested(int row,int candidate,bool locate);
    void optionsRequested();

protected:
    void resizeEvent(QResizeEvent *event) Q_DECL_OVERRIDE;

private slots:
    void rebuild();
    void populateCandidates(QTreeWidgetItem *item);
    void handleClick(QTreeWidgetItem *item,int column);
    void handleChanged(QTreeWidgetItem *item,int column);
    void showItemDetails(QTreeWidgetItem *item,int column);
    void updateCount();

private:
    void showActions(QTreeWidgetItem *item,const QPoint &position);
    bool matchesFilters(int index) const;
    bool matchesCandidate(const DriverCandidate &candidate) const;
    void buildFilterMenu();
    void adjustColumns();
    void refreshCopy();
    void updateDemoNotice();
    int rowOf(QTreeWidgetItem *item) const;
    DriverCatalog *m_catalog;
    bool m_arabic,m_dark,m_busy,m_demo,m_rebuilding;
    int m_clickRow,m_clickCandidate,m_clickPriorChoice;bool m_clickPriorChecked;QElapsedTimer m_clickTime;
    QString m_originLocale;
    QWidget *m_controlBar,*m_reinstallNotice;
    QLabel *m_reinstallText;
    QLabel *m_title,*m_subtitle,*m_info,*m_stats,*m_listTitle;
    QLabel *m_devicesNumber,*m_warningsNumber,*m_selectedNumber;
    QTreeWidget *m_tree;
    QLineEdit *m_search;
    QTimer *m_searchDebounce,*m_catalogRefresh;
    GradientButton *m_scan;
    QPushButton *m_all,*m_none;
    QCheckBox *m_restore,*m_reboot,*m_onlyProblems,*m_expert,*m_showIgnored,*m_filterVersions,*m_smartView;
    ToggleSwitch *m_showInstalled,*m_showMore;
    QLabel *m_installedLabel,*m_moreLabel;
    QToolButton *m_filters,*m_tools,*m_installOptions;
    PremiumPopover *m_filterPopup,*m_installPopup;
    QLabel *m_filterViewHeading,*m_filterFoundHeading,*m_filterAbsentHeading,*m_installHeading;
    QPushButton *m_filterReset;
    QVector<QCheckBox*> m_matchFilters;
};

#endif
