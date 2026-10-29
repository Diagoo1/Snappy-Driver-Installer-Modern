#ifndef SDIOMODERN_INSTALLATION_WORKSPACE_H
#define SDIOMODERN_INSTALLATION_WORKSPACE_H
#include <QWidget>
#include <QStringList>
#include "../native/origin_api.h"
class QLabel;
class QProgressBar;
class QTreeWidget;
class QPushButton;
class QTimer;

// Separate explicit demo and native-event paths. Real progress is never timer-generated.
class InstallationWorkspace : public QWidget
{
    Q_OBJECT
public:
    explicit InstallationWorkspace(QWidget *parent=0);
    void setArabic(bool arabic);
    void setAvailability(bool canInstall,bool demo);
    void beginDemo(const QStringList &drivers,bool restorePoint,bool rebootAfter);
    bool isActive() const {return m_active;}
    void stopDemo();
    bool isLive() const { return m_live; }
    void beginNative(const QStringList &names,const QStringList &ids,bool restore);
    void applyNativeEvent(const sdiomodern::Event &event);
    void finishNative(const sdiomodern::InstallReport &report);
    void failNative(const QString &reason);

signals:
    void backRequested();
    void stopRequested();
    void activeChanged(bool active);
    void simulationFinished(bool rebootRequested);

private slots:
    void step();

private:
    void updateProgress();
    bool m_arabic,m_active,m_reboot,m_hasRestore,m_live;
    int m_percent;
    int m_currentStage;
    QTimer *m_timer;
    QStringList m_items;
    QLabel *m_title,*m_sub,*m_summary,*m_stage,*m_tip,*m_complete,*m_badge;
    QLabel *m_stepRestore,*m_stepPrep,*m_stepInstall,*m_stepFinish;
    QProgressBar *m_progress;
    QTreeWidget *m_queue;
    QPushButton *m_back,*m_stop;
};
#endif
