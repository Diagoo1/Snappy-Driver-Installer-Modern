#include "ui_font.h"
#include "ui_language.h"
#include "scan_overlay.h"
#include "theme.h"
#include <QLabel>
#include <QTimer>
#include <QPainter>
#include <QImage>

#include <QResizeEvent>
#include <QFont>
#include <QtMath>
#include <QPainterPath>

ScanOverlay::ScanOverlay(QWidget *parent)
    : QWidget(parent),m_title(new QLabel(this)),
      m_stage(new QLabel(this)),m_caption(new QLabel(this)),
      m_timer(new QTimer(this)),m_active(false),m_reducedMotion(false),m_frame(0)
{
    setObjectName(QStringLiteral("nativeScanOverlay"));
    setAttribute(Qt::WA_StyledBackground,true);
    setFocusPolicy(Qt::StrongFocus);
    setMouseTracking(false);
    if(parent)parent->installEventFilter(this);
    for(QLabel *label : {m_title,m_stage,m_caption}) {
        label->setAlignment(Qt::AlignCenter);
        label->setAttribute(Qt::WA_TransparentForMouseEvents);
        label->setStyleSheet(QStringLiteral("background:transparent;color:#FFFFFF;"));
    }
    QFont title(UiFont::family(),19,QFont::DemiBold);
    m_title->setFont(title);
    QFont stage(UiFont::family(),11);
    m_stage->setFont(stage);
    QFont small(UiFont::family(),9);
    m_caption->setFont(small);
    m_caption->setText(QStringLiteral("Snappy Driver Installer  |  UI: Tarek Sadek"));
    m_timer->setInterval(33);
    connect(m_timer,&QTimer::timeout,this,[this](){++m_frame;update();});
    hide();
}
void ScanOverlay::start(const QString &message,bool reducedMotion)
{
    if(!parentWidget())return;
    // Take a single snapshot before the cover is shown; never blur the entire
    // live parent tree on every animation frame.
    if(!m_active) {
        // Two-pass downsample gives a subtle frosted background without a
        // costly live blur of the complete UI at every animation frame.
        // A transparent stack has no painted window background of its own.
        // grab() would fill it from the native (often light) Window palette.
        // Composite its children over our resolved theme instead, at native DPI.
        const qreal dpr=parentWidget()->devicePixelRatioF();
        QImage source(QSize(qCeil(parentWidget()->width()*dpr),qCeil(parentWidget()->height()*dpr)),QImage::Format_ARGB32_Premultiplied);
        source.setDevicePixelRatio(dpr);source.fill(Theme::color("background",Theme::isDark(this)));
        {QPainter snapshot(&source);parentWidget()->render(&snapshot,QPoint(),QRegion(),QWidget::DrawChildren);}
        const QImage small=source.scaled(qMax(1,source.width()/10),
                                        qMax(1,source.height()/10),Qt::IgnoreAspectRatio,Qt::SmoothTransformation);
        m_snapshot=QPixmap::fromImage(small.scaled(source.size(),Qt::IgnoreAspectRatio,Qt::SmoothTransformation));
    }
    m_active=true;m_reducedMotion=reducedMotion;m_frame=0;
    const bool arabic=UiLanguage::language()==QStringLiteral("arabic");
    m_caption->setText(arabic?QString::fromUtf8("مدير تعريفات الأجهزة | واجهة طارق صادق"):QStringLiteral("Snappy Driver Installer | UI: Tarek Sadek"));
    m_title->setText(arabic?QString::fromUtf8("جارٍ فحص الجهاز والتعريفات"):QStringLiteral("Scanning devices & offline indexes"));
    m_stage->setText(UiLanguage::text(message));
    setGeometry(parentWidget()->rect());
    layoutChildren();
    show();raise();setFocus(Qt::OtherFocusReason);
    if(!reducedMotion)m_timer->start();else m_timer->stop();
    update();
}
void ScanOverlay::setStage(const QString &message)
{
    if(!m_active)return;
    m_stage->setText(UiLanguage::text(message));
}
void ScanOverlay::finish()
{
    m_active=false;m_timer->stop();m_snapshot=QPixmap();hide();
}
void ScanOverlay::resizeEvent(QResizeEvent *event)
{
    QWidget::resizeEvent(event);layoutChildren();
}
bool ScanOverlay::eventFilter(QObject *object,QEvent *event)
{
    if(object==parentWidget() && event->type()==QEvent::Resize)
        setGeometry(parentWidget()->rect());
    return QWidget::eventFilter(object,event);
}
void ScanOverlay::layoutChildren()
{
    const int cy=height()/2;
    const int left=qMax(16,(width()-840)/2);
    const int w=qMin(840,width()-32);
    m_title->setGeometry(left,cy+54,w,40);
    m_stage->setGeometry(left,cy+101,w,60);
    m_stage->setWordWrap(true);
    m_caption->setGeometry(left,height()-58,w,26);
}
void ScanOverlay::paintEvent(QPaintEvent *)
{
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing);
    // Clip against the actual outer window, not four new inner corners. Inner
    // rounding exposes white page wedges beneath the busy cover.
    const QPoint origin=mapTo(window(),QPoint(0,0));
    const QRectF windowBounds=QRectF(window()->rect()).translated(-origin.x(),-origin.y());
    const qreal windowRadius=window()->isMaximized()?0:20;
    QPainterPath clip;clip.addRoundedRect(windowBounds,windowRadius,windowRadius);p.setClipPath(clip);
    if(!m_snapshot.isNull())p.drawPixmap(rect(),m_snapshot);
    p.fillRect(rect(),QColor(5,22,35,177));
    p.setRenderHint(QPainter::Antialiasing,true);
    const QPointF center(width()/2.0,height()/2.0-38.0);
    const qreal radius=34.0;
    for(int i=0;i<6;++i){
        const qreal phase=m_reducedMotion?i*0.55:(m_frame*0.105 - i*0.39);
        const qreal angle=phase-1.5707963267948966;
        const QPointF spot(center.x()+radius*qCos(angle),center.y()+radius*qSin(angle));
        const int opacity=qBound(65,240-i*29,255);
        p.setPen(Qt::NoPen);
        p.setBrush(QColor(118,239,231,opacity));
        const qreal r=3.1+(5-i)*0.17;
        p.drawEllipse(spot,r,r);
    }
}

void ScanOverlay::startOperation(const QString &title,const QString &message,bool reducedMotion){
    start(message,reducedMotion);m_title->setText(title);
}
