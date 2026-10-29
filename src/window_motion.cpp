#include "window_motion.h"
#include <QWidget>
#include <QApplication>
#include <QDesktopWidget>
#include <QPainter>
#include <QEasingCurve>
#include <QEvent>
#include <QtMath>
#ifdef Q_OS_WIN
#include <windows.h>
#endif
class MotionSnapshot : public QWidget {
public:MotionSnapshot():QWidget(0,Qt::Tool|Qt::FramelessWindowHint|Qt::NoDropShadowWindowHint|Qt::WindowDoesNotAcceptFocus|Qt::WindowTransparentForInput){setAttribute(Qt::WA_TranslucentBackground);setAttribute(Qt::WA_ShowWithoutActivating);setAttribute(Qt::WA_TransparentForMouseEvents);setObjectName(QStringLiteral("windowMotionSnapshot"));}
    QImage image;
protected:void paintEvent(QPaintEvent*) Q_DECL_OVERRIDE {QPainter p(this);p.setRenderHint(QPainter::SmoothPixmapTransform);p.drawImage(rect(),image);}
};
WindowMotion::WindowMotion(QWidget *window):QObject(window),m_window(window),m_proxy(new MotionSnapshot){
    setObjectName(QStringLiteral("windowMotion"));qApp->installEventFilter(this);m_timer.setInterval(33);connect(&m_timer,&QTimer::timeout,this,[this](){tick();});
    connect(qApp,&QApplication::aboutToQuit,this,[this](){finish();});
}
WindowMotion::~WindowMotion(){m_timer.stop();delete m_proxy;}
bool WindowMotion::eventFilter(QObject *watched,QEvent *event){
    if(m_busy && (event->type()==QEvent::MouseButtonPress || event->type()==QEvent::MouseButtonRelease || event->type()==QEvent::MouseButtonDblClick || event->type()==QEvent::KeyPress)){
        QWidget *widget=qobject_cast<QWidget*>(watched);if(widget && (widget==m_window || m_window->isAncestorOf(widget)))return true;
    }
    return QObject::eventFilter(watched,event);
}
QImage WindowMotion::capture(){
    // Render directly into a bounded image, never a full-resolution screen grab.
    QElapsedTimer cost;cost.start();const QSize size=m_window->size().scaled(QSize(960,640),Qt::KeepAspectRatio);
    QImage image(size,QImage::Format_ARGB32_Premultiplied);if(image.isNull())return image;image.fill(Qt::transparent);
    QPainter painter(&image);painter.scale(qreal(size.width())/m_window->width(),qreal(size.height())/m_window->height());
    m_window->render(&painter,QPoint(),QRegion(),QWidget::DrawWindowBackground|QWidget::DrawChildren);painter.end();
    m_window->setProperty("motionCapturePixels",size.width()*size.height());m_window->setProperty("motionCaptureMs",cost.elapsed());return image;
}
QRect WindowMotion::taskbarTarget()const{
    const QRect work=QApplication::desktop()->availableGeometry(m_window);const int x=qBound(work.left()+40,m_window->geometry().center().x(),work.right()-40);
    return QRect(x-40,work.bottom()-2,80,12);
}
void WindowMotion::begin(const QImage &image,const QRect &from,const QRect &to,bool minimizing){
    m_busy=true;m_minimizing=minimizing;m_frames=0;m_from=from;m_to=to;m_proxy->image=image;
    m_window->setProperty("windowMotionActive",true);m_proxy->setGeometry(from);m_proxy->show();m_proxy->raise();m_clock.start();m_timer.start();
}
void WindowMotion::finish(){
    m_timer.stop();m_proxy->hide();m_proxy->image=QImage();
    if(m_busy){m_window->setWindowOpacity(1);m_window->setProperty("motionFrames",m_frames);m_window->setProperty("windowMotionActive",false);}
    m_busy=false;
}
void WindowMotion::tick(){
    const qreal progress=qMin(1.,m_clock.elapsed()/165.);if(progress>=1 || m_frames>=5){finish();return;}
    const qreal t=QEasingCurve(QEasingCurve::OutCubic).valueForProgress(progress);
    auto blend=[t](int a,int b){return qRound(a+(b-a)*t);};
    m_proxy->setGeometry(blend(m_from.x(),m_to.x()),blend(m_from.y(),m_to.y()),qMax(1,blend(m_from.width(),m_to.width())),qMax(1,blend(m_from.height(),m_to.height())));
    ++m_frames;
}
void WindowMotion::minimize(){
    if(m_busy)return;
    if(qApp->property("reduceMotion").toBool()){m_window->showMinimized();return;}
    const QRect from=m_window->geometry();const QImage image=capture();m_window->showMinimized();if(!image.isNull())begin(image,from,taskbarTarget(),true);
}
void WindowMotion::toggleMaximized(){
    if(m_busy)return;
    if(qApp->property("reduceMotion").toBool()){if(m_window->isMaximized())m_window->showNormal();else m_window->showMaximized();return;}
    const QRect from=m_window->geometry();const QImage image=capture();if(image.isNull()){if(m_window->isMaximized())m_window->showNormal();else m_window->showMaximized();return;}m_busy=true;m_window->setWindowOpacity(0);
    if(m_window->isMaximized())m_window->showNormal();else m_window->showMaximized();
    // Let the window manager settle the destination once; no nested event loop.
    QTimer::singleShot(0,this,[this,image,from](){begin(image,from,m_window->geometry(),false);});
}
void WindowMotion::restoredFromMinimized(){
    if(m_busy || qApp->property("reduceMotion").toBool())return;
    m_busy=true;m_window->setWindowOpacity(0);
    QTimer::singleShot(0,this,[this](){const QImage image=capture();if(image.isNull()){finish();return;}begin(image,taskbarTarget(),m_window->geometry(),false);});
}
