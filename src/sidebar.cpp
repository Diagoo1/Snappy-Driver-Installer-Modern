#include "ui_font.h"
#include "sidebar.h"
#include "appearance.h"
#include "theme.h"
#include "vector_icons.h"
#include "origin_languages.h"
#include <QCoreApplication>
#include <QPainter>
#include <QPropertyAnimation>
#include <QEasingCurve>
#include <QVBoxLayout>
#include <QLabel>
#include <QFrame>
#include <QFontMetrics>
#include <QEvent>
#include <QPainterPath>

SidebarItem::SidebarItem(const QString &symbol,QWidget *parent)
  : QAbstractButton(parent),m_iconName(symbol),m_compact(false),m_motion(true),
    m_hover(0),m_animation(new QPropertyAnimation(this,"hoverAmount",this))
{
    setCheckable(true);
    setAutoExclusive(false);
    setCursor(Qt::PointingHandCursor);
    setFocusPolicy(Qt::StrongFocus);
    setFixedHeight(66);
    setSizePolicy(QSizePolicy::Expanding,QSizePolicy::Fixed);
    m_animation->setDuration(150);
    m_animation->setEasingCurve(QEasingCurve::OutCubic);
}
void SidebarItem::setCompact(bool compact) { m_compact=compact; update(); }
void SidebarItem::setMotionEnabled(bool enabled)
{
    m_motion=enabled;
    if(!enabled){m_animation->stop();setHoverAmount(underMouse()?1.0:0.0);}
}
void SidebarItem::setHoverAmount(qreal h) { m_hover=h; update(); }
void SidebarItem::enterEvent(QEvent *e)
{
    m_animation->stop();
    if (m_motion) {m_animation->setStartValue(m_hover);m_animation->setEndValue(1.0);m_animation->start();}
    else setHoverAmount(1.0);
    QAbstractButton::enterEvent(e);
}
void SidebarItem::leaveEvent(QEvent *e)
{
    m_animation->stop();
    if (m_motion) {m_animation->setStartValue(m_hover);m_animation->setEndValue(0.0);m_animation->start();}
    else setHoverAmount(0.0);
    QAbstractButton::leaveEvent(e);
}
void SidebarItem::paintEvent(QPaintEvent *)
{
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing,true);
    const bool active=isChecked(),rtl=layoutDirection()==Qt::RightToLeft;
    if (active) {
        p.setPen(Qt::NoPen);p.setBrush(Theme::color("sidebarMiddle").lighter(145));p.drawRoundedRect(rect().adjusted(8,4,-8,-4),12,12);
        p.setBrush(Theme::color("accentSoft"));
        p.drawRoundedRect(QRectF(rtl?width()-6:2,(height()-32)/2.0,4,32),2,2);
    }
    if (m_hover>0.001) {
        QColor hover=Theme::color("accentSoft");hover.setAlphaF(m_hover * (active ? 0.18 : 0.29));
        p.setPen(Qt::NoPen);p.setBrush(hover);p.drawRoundedRect(rect().adjusted(8,4,-8,-4),12,12);
    }
    if (isDown()) {p.setPen(Qt::NoPen);p.setBrush(QColor(1,22,41,42));p.drawRoundedRect(rect().adjusted(8,4,-8,-4),12,12);}
    const int iconSize=24;
    const qreal openness=qBound(0.0,(width()-84)/140.0,1.0);
    const int logicalX=qRound(30-5*openness);
    const int ix=rtl?width()-logicalX-iconSize:logicalX;
    const int iy=(height()-iconSize)/2;
    const QColor ink=active ? QColor("#FFFFFF") : QColor("#B2D1E2");
    VectorIcons::paint(&p,m_iconName,QRectF(ix,iy,iconSize,iconSize),ink);
    const qreal labelOpacity=qBound(0.0,(width()-100)/100.0,1.0);
    if (labelOpacity>.001) {
        p.setOpacity(labelOpacity);
        QFont f=font(); f.setFamily(UiFont::family()); f.setPointSize(10);
        f.setWeight(active ? QFont::DemiBold : QFont::Normal);
        p.setFont(f);
        p.setPen(ink);
        const QRect textRect(rtl?12:ix+iconSize+15,0,qMax(8,width()-(logicalX+iconSize+22)),height());
        const QString label=QFontMetrics(f).elidedText(text(),Qt::ElideRight,textRect.width());
        p.drawText(textRect,Qt::AlignAbsolute|(rtl?Qt::AlignRight:Qt::AlignLeft)|Qt::AlignVCenter,label);
    }

}

Sidebar::Sidebar(QWidget *parent)
  : QWidget(parent),m_slide(new QPropertyAnimation(this,"menuWidth",this)),
    m_layout(new QVBoxLayout(this)),m_footer(0),m_collapsed(false),m_arabicEnabled(false),
    m_animationsEnabled(true),m_labelsHidden(false),m_current(0)
{
    setObjectName(QStringLiteral("sidebar"));
    setFixedWidth(224);
    m_layout->setContentsMargins(0,18,0,10);
    m_layout->setSpacing(0);
    makeButton(0,QStringLiteral("Drivers"),QString::fromUtf8("التعريفات"),QStringLiteral("wrench"));
    makeButton(1,QStringLiteral("Diagnostics"),QString::fromUtf8("تشخيص الجهاز"),QStringLiteral("activity"));
    makeButton(2,QStringLiteral("Driver packages"),QString::fromUtf8("حزم التعريفات"),QStringLiteral("layers"));
    m_layout->addStretch(1);
    QFrame *sep=new QFrame(this);
    sep->setFixedHeight(1);
    sep->setStyleSheet(QStringLiteral("background:#276A8D;border:none;"));
    m_layout->addWidget(sep);
    makeButton(3,QStringLiteral("Settings"),QString::fromUtf8("الإعدادات"),QStringLiteral("settings"));
    makeButton(5,QStringLiteral("About"),QString::fromUtf8("حول البرنامج"),QStringLiteral("info"));
    m_footer=new QLabel(QStringLiteral("UI © 2026 Tarek Sadek"),this);
    m_footer->setObjectName(QStringLiteral("sidebarFooter"));
    m_footer->setAlignment(Qt::AlignCenter);
    m_footer->setFixedHeight(31);
    m_layout->addWidget(m_footer);
    m_slide->setDuration(270);
    m_slide->setEasingCurve(QEasingCurve::OutCubic);
    updateActive();
}

SidebarItem *Sidebar::makeButton(int index,const QString &en,const QString &ar,const QString &symbol)
{
    SidebarItem *button=new SidebarItem(symbol,this);
    button->setText(en);
    button->setProperty("pageIndex",index);
    button->setToolTip(en);
    m_buttons.append(button);
    m_english.append(en);
    m_arabic.append(ar);
    connect(button,&SidebarItem::clicked,this,[this,index](){updateActive();emit pageRequested(index);});
    m_layout->addWidget(button);
    return button;
}

void Sidebar::paintEvent(QPaintEvent *)
{
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing);
    // The top-level window paints the sidebar surface once at the outer edge.
    p.setPen(QColor(255,255,255,18));
    const int x=layoutDirection()==Qt::RightToLeft?0:width()-1;
    p.drawLine(x,0,x,height());
}
int Sidebar::menuWidth() const {return width();}
void Sidebar::setMenuWidth(int value)
{
    setFixedWidth(value);
    const bool compact=value<144;
    if (compact!=m_labelsHidden) {
        m_labelsHidden=compact;
        if (m_footer) m_footer->setVisible(!compact);
        refreshLabels();
    }
}
void Sidebar::toggleCollapsed()
{
    m_collapsed=!m_collapsed;
    m_slide->stop();
    if (m_animationsEnabled) {
        m_slide->setStartValue(width());
        m_slide->setEndValue(m_collapsed ? 84 : 224);
        m_slide->start();
    } else setMenuWidth(m_collapsed ? 84 : 224);
}
void Sidebar::setAnimations(bool enabled)
{
    m_animationsEnabled=enabled;
    if(!enabled){
        m_slide->stop();
        setMenuWidth(m_collapsed?84:224);
    }
    for (int i=0;i<m_buttons.size();++i) m_buttons.at(i)->setMotionEnabled(enabled);
}
void Sidebar::setCurrentPage(int page)
{m_current=page;updateActive();}
void Sidebar::setArabic(bool arabic)
{m_arabicEnabled=arabic;m_footer->setText(arabic?QString::fromUtf8("واجهة طارق صادق © 2026"):QStringLiteral("UI © 2026 Tarek Sadek"));m_footer->setProperty("skipTranslation",true);refreshLabels();}
void Sidebar::applyOriginLanguage(const QString &directory,const QString &code)
{
    if(code==QStringLiteral("english") || code==QStringLiteral("arabic"))return;
    // Don't replace a page title with an unrelated upstream label. Only map
    // exact shared concepts: installation, diagnostics and settings.
    for(int i=0;i<m_buttons.size();++i){
        const int page=m_buttons.at(i)->property("pageIndex").toInt();
        QString key;
        if(page==4)key=QStringLiteral("STR_INSTALL");
        else if(page==1)key=QStringLiteral("STR_SHOW_SYSINFO");
        else if(page==3)key=QStringLiteral("STR_OPTIONS");
        if(key.isEmpty())continue;
        const QString text=OriginLanguages::text(directory,code,key);
        if(text.isEmpty())continue;
        m_buttons[i]->setText(text);m_buttons[i]->setToolTip(text);
    }
}
void Sidebar::refreshLabels()
{
    for (int i=0;i<m_buttons.size();++i) {
        const QString label=m_arabicEnabled?m_arabic.at(i):m_english.at(i);
        SidebarItem *button=m_buttons.at(i);
        button->setText(label);
        button->setToolTip(label);
        button->setCompact(m_labelsHidden);
    }
}
void Sidebar::updateActive()
{
    for (int i=0;i<m_buttons.size();++i) {
        m_buttons.at(i)->setChecked(m_buttons.at(i)->property("pageIndex").toInt()==m_current);
        m_buttons.at(i)->update();
    }
}

void Sidebar::setInstallationAvailable(bool enabled)
{
    for(int i=0;i<m_buttons.size();++i)
        if(m_buttons.at(i)->property("pageIndex").toInt()==4)
            m_buttons.at(i)->setVisible(enabled);
}
