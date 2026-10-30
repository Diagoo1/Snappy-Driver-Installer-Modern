#ifndef SDIO_ONLINE_DIALOG_H
#define SDIO_ONLINE_DIALOG_H
#include "appearance.h"
#include "online_packages.h"
#include <QThread>
#include <QElapsedTimer>
class QTreeWidget;
class QLabel;
class QPushButton;
class QProgressBar;
class QSpinBox;
class QCheckBox;
class OnlineWorker : public QThread {
    Q_OBJECT
public:
    explicit OnlineWorker(QObject *parent=0):QThread(parent),checking(true),stop(false){}
    bool checking;
    std::atomic<bool> stop;
    OnlinePackages::Catalog catalog;
    QByteArray imported;
    QVector<int> selected;
    QString drivers,indexes,cache,error;
    QUrl metadataUrl=OnlinePackages::officialMetadataUrl();
    QStringList published;
signals:
    void activity(const QString &message,qint64 verified,qint64 total,int published);
    void transferStats(qint64 received,qint64 downloadRate,qint64 uploadRate,int peers);
    void fileProgress(int index,qint64 verified,qint64 total);
protected:
    void run() Q_DECL_OVERRIDE;
};
class OnlineDialog : public RoundedDialog {
    Q_OBJECT
public:
    OnlineDialog(const QString &drivers,const QString &indexes,const QStringList &needed,bool arabic,QWidget *parent=0);
    ~OnlineDialog();
    void reject() Q_DECL_OVERRIDE;
    QString lastError()const{return m_worker->error;}
    bool filesChanged()const{return m_changed;}
    void setCatalog(const OnlinePackages::Catalog &catalog);
    void refreshCatalog(){check();}
    OnlinePackages::Catalog catalog()const{return m_catalog;}
    QUrl catalogSource()const{return m_worker->imported.isEmpty()?m_worker->metadataUrl:QUrl();}
protected:
    void closeEvent(QCloseEvent*) Q_DECL_OVERRIDE;
private:
    QString trText(const char *english,const char *arabic)const;
    void check(bool importFile=false);
    void download();
    void completed();
    void updateSelection();
    void setWorking(bool running);
    void filterRows();
    QVector<int> selection()const;
    bool m_arabic,m_changed,m_closePending;
    QStringList m_needed;
    OnlineWorker *m_worker;
    OnlinePackages::Catalog m_catalog;
    QVector<OnlinePackages::Availability> m_availability;
    QTreeWidget *m_files;
    QLabel *m_status,*m_summary;
    QPushButton *m_check,*m_import,*m_indexes,*m_neededButton,*m_none,*m_download,*m_pause,*m_close;
    QProgressBar *m_progress;
    QWidget *m_transferPanel;
    QCheckBox *m_sharing;
    QLabel *m_sharingStatus;
    QSpinBox *m_downloadLimit,*m_uploadLimit,*m_connections,*m_port;
    QCheckBox *m_updatesOnly,*m_hideMissing;
    QPushButton *m_all,*m_network;
    class QLineEdit *m_search;
    QLabel *m_received,*m_speed,*m_eta,*m_peers,*m_upload,*m_verifiedLabel,*m_sourceLabel;
    qint64 m_verified=0,m_total=0;
    QElapsedTimer m_telemetryClock;
    class QTimer *m_rateExpiry;
};
#endif
