#include "ui_scale.h"
#include "app_settings.h"
#include <QSettings>
#include <QJsonDocument>
#include <QJsonObject>
#include <QScopedPointer>
#include <QByteArray>
#include <cstring>
#ifdef Q_OS_WIN
#include <windows.h>
#endif
namespace UiScale {
static int effective=100;
int fitPercent(int requested,const QSize &available){
    const int maximum=qMin((available.width()-24)*100/950,(available.height()-24)*100/640);
    return qBound(50,qMin(qBound(50,requested,200),qMax(50,maximum/5*5)),200);
}
void prepareEnvironment(){
    QScopedPointer<QSettings> settings(createAppSettings(QString(),0,false));
    bool overrideValid=false;const int overridePercent=qgetenv("SDIO_ZOOM_PERCENT_OVERRIDE").toInt(&overrideValid);
    const QByteArray session=qgetenv("SDIO_LEGACY_SESSION");const QJsonObject legacy=session.size()<=65536?QJsonDocument::fromJson(session).object():QJsonObject();
    bool legacyValid=false;const int oldScale=legacy.value(QStringLiteral("scale")).toString().toInt(&legacyValid);
    const int requested=qBound(50,overrideValid?overridePercent:legacyValid?qRound(oldScale*100./256):settings->value(QStringLiteral("ui/scalePercent"),100).toInt(),200);
    QByteArray original=qgetenv("SDIO_BASE_SCALE_FACTOR");if(original.isEmpty())original=qgetenv("QT_SCALE_FACTOR");
    bool valid=false;double base=original.toDouble(&valid);if(!valid || base<.5 || base>4)base=1;
    effective=requested;
#ifdef Q_OS_WIN
    RECT work={};if(SystemParametersInfoW(SPI_GETWORKAREA,0,&work,0)){
        HDC screen=GetDC(0);const int dpi=screen?GetDeviceCaps(screen,LOGPIXELSX):96;if(screen)ReleaseDC(0,screen);
        const double system=qMax(1.,dpi/96.0)*base;
        effective=fitPercent(requested,QSize(qRound((work.right-work.left)/system),qRound((work.bottom-work.top)/system)));
    }
#endif
    qputenv("SDIO_BASE_SCALE_FACTOR",QByteArray::number(base,'f',4));
    qputenv("QT_SCALE_FACTOR",QByteArray::number(base*effective/100.,'f',4));
}
int effectivePercent(){return effective;}
bool restartAfterParent(int argc,char **argv){
    for(int i=1;i<argc;++i)if(std::strcmp(argv[i],"--restart-after")==0){
        if(i+1>=argc)return false;bool ok=false;const quint32 pid=QByteArray(argv[i+1]).toUInt(&ok);if(!ok || !pid)return false;
#ifdef Q_OS_WIN
        HANDLE parent=OpenProcess(SYNCHRONIZE,FALSE,pid);if(!parent)return false;const DWORD status=WaitForSingleObject(parent,60000);CloseHandle(parent);if(status!=WAIT_OBJECT_0)return false;
#endif
    }return true;
}
}
