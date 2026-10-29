#ifndef SDIO_SHARING_RESTORE_H
#define SDIO_SHARING_RESTORE_H
#include "online_packages.h"
#include <QThread>
class SharingRestore : public QThread {
public:
    SharingRestore(const QString &root,const OnlinePackages::TransferSettings &settings,QObject *parent):QThread(parent),stop(false),m_root(root),m_settings(settings){}
    ~SharingRestore(){stop.store(true);wait();}
    std::atomic<bool> stop;
protected:
    void run() Q_DECL_OVERRIDE {OnlinePackages::resumeSharing(m_root,m_settings,stop);}
private:QString m_root;OnlinePackages::TransferSettings m_settings;
};
#endif
