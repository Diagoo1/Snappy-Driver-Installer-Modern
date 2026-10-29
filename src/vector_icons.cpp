#include "vector_icons.h"
#include <QSvgRenderer>
#include <QPainter>
#include <QPixmap>
#include <QFile>
#include <QHash>
#include <QSharedPointer>
#include <QIconEngine>
#include <QApplication>
#include <QtMath>

namespace {
QSharedPointer<QSvgRenderer> renderer(const QString &name,const QColor &color){
    static QHash<QString,QSharedPointer<QSvgRenderer> > cache;
    const QString key=name+color.name();
    if(cache.contains(key))return cache.value(key);
    QFile file(QStringLiteral(":/sdio/assets/icons/")+name+QStringLiteral(".svg"));
    QByteArray svg;if(file.open(QIODevice::ReadOnly))svg=file.readAll();
    svg.replace("#ffffff",color.name().toLatin1());
    QSharedPointer<QSvgRenderer> result(new QSvgRenderer(svg));
    if(cache.size()>360)cache.clear();cache.insert(key,result);return result;
}
class SvgIconEngine : public QIconEngine {
public:
    SvgIconEngine(const QString &name,const QColor &color):m_name(name),m_color(color){}
    QIconEngine *clone()const Q_DECL_OVERRIDE{return new SvgIconEngine(m_name,m_color);}
    void paint(QPainter *p,const QRect &r,QIcon::Mode mode,QIcon::State) Q_DECL_OVERRIDE {
        p->save();p->setOpacity(p->opacity()*m_color.alphaF()*(mode==QIcon::Disabled?.4:1.0));
        p->setRenderHint(QPainter::Antialiasing);renderer(m_name,m_color)->render(p,QRectF(r));p->restore();
    }
    QPixmap pixmap(const QSize &size,QIcon::Mode mode,QIcon::State state) Q_DECL_OVERRIDE {
        QPixmap result(size);result.fill(Qt::transparent);QPainter p(&result);paint(&p,QRect(QPoint(),size),mode,state);return result;
    }
private:
    QString m_name;QColor m_color;
};
}
QIcon VectorIcons::icon(const QString &name,const QColor &color,int logicalSize){
    Q_UNUSED(logicalSize);return QIcon(new SvgIconEngine(name,color));
}
QPixmap VectorIcons::pixmap(const QString &name,const QColor &color,int logicalSize){
    const qreal dpr=qApp?qApp->devicePixelRatio():1.0;
    QPixmap result(qCeil(logicalSize*dpr),qCeil(logicalSize*dpr));result.fill(Qt::transparent);result.setDevicePixelRatio(dpr);
    QPainter p(&result);VectorIcons::paint(&p,name,QRectF(0,0,logicalSize,logicalSize),color);return result;
}
void VectorIcons::paint(QPainter *p,const QString &name,const QRectF &rect,const QColor &color){
    if(!p || rect.isEmpty())return;
    p->save();p->setOpacity(p->opacity()*color.alphaF());p->setRenderHint(QPainter::Antialiasing);
    renderer(name,color)->render(p,rect);p->restore();
}

QString VectorIcons::forDeviceClass(const QString &original)
{
    const QString d=original.toLower();
    if (d.contains(QStringLiteral("net")) || d.contains(QStringLiteral("wifi"))) return QStringLiteral("network");
    if (d.contains(QStringLiteral("media")) || d.contains(QStringLiteral("audio")) || d.contains(QStringLiteral("sound"))) return QStringLiteral("audio");
    if (d.contains(QStringLiteral("display")) || d.contains(QStringLiteral("monitor"))) return QStringLiteral("monitor");
    if (d.contains(QStringLiteral("disk")) || d.contains(QStringLiteral("storage")) || d.contains(QStringLiteral("hdd")) || d.contains(QStringLiteral("scsi"))) return QStringLiteral("storage");
    if (d.contains(QStringLiteral("usb"))) return QStringLiteral("usb");
    if (d.contains(QStringLiteral("bluetooth"))) return QStringLiteral("bluetooth");
    if (d.contains(QStringLiteral("keyboard"))) return QStringLiteral("keyboard");
    if (d.contains(QStringLiteral("mouse")) || d.contains(QStringLiteral("pointing"))) return QStringLiteral("mouse");
    if (d.contains(QStringLiteral("battery"))) return QStringLiteral("battery");
    if (d.contains(QStringLiteral("print"))) return QStringLiteral("printer");
    if (d.contains(QStringLiteral("processor")) || d.contains(QStringLiteral("system"))) return QStringLiteral("chip");
    return QStringLiteral("computer");
}
