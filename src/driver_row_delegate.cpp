#include "ui_font.h"
#include "vector_icons.h"
#include "driver_row_delegate.h"
#include <QPainter>
#include <QPainterPath>
#include <QStyle>
#include <QApplication>
#include <QFontMetrics>
#include <QIcon>
#include <QTreeWidget>
#include <QtMath>

DriverRowDelegate::DriverRowDelegate(QObject *parent) : QStyledItemDelegate(parent) {}

static QPainterPath starPath(const QPointF &center,qreal outer,qreal inner)
{
    QPainterPath path;
    for(int i=0;i<10;++i){
        const qreal r=i%2 ? inner : outer;
        const qreal angle=-3.14159265358979323846/2.0+i*3.14159265358979323846/5.0;
        const QPointF vertex(center.x()+r*qCos(angle),center.y()+r*qSin(angle));
        if(i==0) path.moveTo(vertex);else path.lineTo(vertex);
    }
    path.closeSubpath();
    return path;
}

void DriverRowDelegate::paint(QPainter *p,const QStyleOptionViewItem &option,
                             const QModelIndex &index) const
{
    const bool child=index.sibling(index.row(),0).data(Qt::UserRole+1).toInt()>=0,rtl=option.direction==Qt::RightToLeft;
    const int align=Qt::AlignAbsolute|(rtl?Qt::AlignRight:Qt::AlignLeft);
    const int col=index.column();
    if(index.data(Qt::UserRole+13).toBool()){p->save();p->setRenderHint(QPainter::Antialiasing);const bool dark=option.palette.color(QPalette::Base).lightness()<130;const QRect area=option.rect.adjusted(2,5,-2,-5);p->setPen(Qt::NoPen);p->setBrush(dark?QColor("#244555"):QColor("#EAF4F6"));p->drawRoundedRect(area,9,9);QFont face=option.font;face.setFamily(UiFont::family());face.setPixelSize(13);face.setWeight(QFont::DemiBold);p->setFont(face);p->setPen(dark?QColor("#A9E4DE"):QColor("#206477"));const QString category=index.data(Qt::UserRole+14).toString();VectorIcons::icon(VectorIcons::forDeviceClass(category),dark?QColor("#7AD4CA"):QColor("#168D9B"),18).paint(p,QRect(rtl?area.right()-30:area.left()+12,area.center().y()-9,18,18));p->drawText(area.adjusted(rtl?12:38,0,rtl?-38:-12,0),align|Qt::AlignVCenter,index.data().toString());p->restore();return;}
    QStyleOptionViewItem rowOption(option);
    // The tree paints a single rounded full-row surface, including branches and actions.
    rowOption.state &= ~(QStyle::State_MouseOver|QStyle::State_Selected|QStyle::State_HasFocus);
    if(col==3){
        QStyleOptionViewItem opt(rowOption);opt.state &= ~QStyle::State_HasFocus;
        opt.text.clear();opt.icon=QIcon();opt.features=QStyleOptionViewItem::None;
        (option.widget?option.widget->style():QApplication::style())->drawControl(QStyle::CE_ItemViewItem,&opt,p,option.widget);
        if(child || (index.data(Qt::CheckStateRole).isNull() && index.data(Qt::UserRole+12).toBool())){
            const bool dark=option.palette.color(QPalette::Base).lightness()<130,selected=index.sibling(index.row(),0).data(Qt::UserRole+2).toBool();const QRect slot=option.rect.adjusted(rtl?34:5,0,rtl?-5:-34,0);QFont face=option.font;face.setFamily(UiFont::family());face.setPointSizeF(9);face.setWeight(QFont::Medium);const QFontMetrics metrics(face);const QString label=metrics.elidedText(index.data().toString(),Qt::ElideRight,slot.width()-20);const int width=qMin(slot.width(),metrics.width(label)+20);const QRect tag(slot.center().x()-width/2,slot.center().y()-14,width,28);p->save();p->setRenderHint(QPainter::Antialiasing);p->setPen(Qt::NoPen);QColor fill=selected?QColor("#13A98D"):option.palette.color(QPalette::Mid);fill.setAlpha(selected?(dark?45:20):(dark?15:9));p->setBrush(fill);p->drawRoundedRect(tag,14,14);p->setFont(face);p->setPen(selected?(dark?QColor("#77DCC9"):QColor("#168974")):option.palette.color(QPalette::Mid));p->drawText(tag,Qt::AlignCenter,label);p->restore();
        }else         if(index.data(Qt::CheckStateRole).isNull() && index.data(Qt::UserRole+12).toBool()){
            const bool dark=option.palette.color(QPalette::Base).lightness()<130;const QRect action=option.rect.adjusted(rtl?36:7,22,rtl?-7:-36,-22);p->save();p->setRenderHint(QPainter::Antialiasing);p->setPen(QPen(dark?QColor("#426474"):QColor("#C9DDE3"),1));p->setBrush(dark?QColor("#234858"):QColor("#EDF6F8"));p->drawRoundedRect(action,9,9);p->setPen(option.palette.color(QPalette::Text));QFont face=option.font;face.setFamily(UiFont::family());face.setWeight(QFont::Medium);p->setFont(face);p->drawText(action.adjusted(6,0,-6,0),Qt::AlignCenter,option.fontMetrics.elidedText(index.data().toString(),Qt::ElideRight,action.width()-12));p->restore();
        }else {
        opt= rowOption;opt.state &= ~QStyle::State_HasFocus;opt.rect.adjust(rtl?32:0,0,rtl?0:-32,0);
        QStyledItemDelegate::paint(p,opt,index);}
        p->save();p->setRenderHint(QPainter::Antialiasing);p->setPen(Qt::NoPen);p->setBrush(option.palette.color(QPalette::Mid));
        for(int i=-1;i<=1;++i)p->drawEllipse(QPointF(rtl?option.rect.left()+16:option.rect.right()-16,option.rect.center().y()+i*5),1.6,1.6);
        p->restore();return;
    }
    p->save();
    const QRect r=option.rect;
    const QWidget *widget=option.widget;
    QStyleOptionViewItem background(rowOption);
    background.state &= ~QStyle::State_HasFocus;
    background.text.clear();
    background.icon=QIcon();
    (widget?widget->style():QApplication::style())->drawControl(QStyle::CE_ItemViewItem,&background,p,widget);
    p->setRenderHint(QPainter::Antialiasing,true);
    const QColor ink=option.palette.color(QPalette::Text);
    const QColor muted=option.palette.color(QPalette::Mid);
    const bool dark=option.palette.color(QPalette::Base).lightness()<130;

    if(col==0){
        p->setLayoutDirection(Qt::LeftToRight);
        const QStringList lines=index.data(Qt::DisplayRole).toString().split(QLatin1Char('\n'));
        if(!child){
            const QIcon icon=qvariant_cast<QIcon>(index.data(Qt::DecorationRole));
            if(!icon.isNull())icon.paint(p,QRect(rtl?r.right()-40:r.x()+10,r.y()+20,30,30));
            const int x=r.left()+(rtl?11:53);
            const int w=qMax(40,r.width()-65);
            QFont title=option.font;title.setFamily(UiFont::family());
            title.setPointSizeF(11.0);title.setWeight(QFont::DemiBold);
            p->setFont(title);p->setPen(ink);
            p->drawText(QRect(x,r.y()+9,w,29),align|Qt::AlignVCenter,
                        QFontMetrics(title).elidedText(lines.value(0),Qt::ElideRight,w));
            QFont sub=option.font;sub.setPointSizeF(10.0);sub.setWeight(QFont::Normal);
            p->setFont(sub);p->setPen(muted);
            p->drawText(QRect(x,r.y()+38,w,21),align|Qt::AlignVCenter,
                        QFontMetrics(sub).elidedText(lines.value(1),Qt::ElideRight,w));
            p->drawText(QRect(x,r.y()+60,w,19),align|Qt::AlignVCenter,
                        QFontMetrics(sub).elidedText(lines.value(2),Qt::ElideRight,w));
            if(lines.size()>3){
                QFont idFont=sub;idFont.setPointSizeF(9.0);p->setFont(idFont);
                p->drawText(QRect(x,r.y()+80,w,18),align|Qt::AlignVCenter,
                            QFontMetrics(idFont).elidedText(lines.value(3),Qt::ElideMiddle,w));
            }
        }else{
            const bool chosen=index.data(Qt::UserRole+2).toBool();
            const int cx=rtl?r.right()-18:r.x()+18, cy=r.y()+r.height()/2;
            const QColor green("#17B786");
            p->setPen(QPen(chosen?green:(dark?QColor("#708E9A"):QColor("#B9C9D0")),1.7));
            p->setBrush(Qt::NoBrush);
            p->drawEllipse(QPointF(cx,cy),8,8);
            if(chosen){p->setPen(Qt::NoPen);p->setBrush(green);p->drawEllipse(QPointF(cx,cy),4,4);}
            const int x=r.x()+(rtl?10:40);
            const int w=qMax(34,r.width()-51);
            QFont primary=option.font;primary.setPointSizeF(10.0);primary.setWeight(QFont::DemiBold);
            p->setFont(primary);p->setPen(ink);
            p->drawText(QRect(x,r.y()+14,w,25),align|Qt::AlignVCenter,
                        QFontMetrics(primary).elidedText(index.data(Qt::DisplayRole).toString(),Qt::ElideRight,w));
            QFont sub=option.font;sub.setPointSizeF(9.0);
            p->setFont(sub);p->setPen(muted);
            p->drawText(QRect(x,r.y()+42,w,21),align|Qt::AlignVCenter,
                        QFontMetrics(sub).elidedText(index.data(Qt::UserRole+3).toString(),Qt::ElideMiddle,w));
        }
    }else if(col==1){
        const QString status=index.data(Qt::DisplayRole).toString();
        const QString code=index.data(Qt::UserRole+6).toString();
        const bool recommended=code==QStringLiteral("Reinstall") || code==QStringLiteral("Upgrade") || status.contains(QStringLiteral("Recommended"),Qt::CaseInsensitive) ||
                status.contains(QStringLiteral("Update"),Qt::CaseInsensitive);
        const bool warning=code==QStringLiteral("Missing") || code==QStringLiteral("Downgrade") || status.contains(QStringLiteral("warning"),Qt::CaseInsensitive);
        const bool current=code==QStringLiteral("Installed") || status.contains(QStringLiteral("Current"),Qt::CaseInsensitive) ||
                status.contains(QStringLiteral("Installed"),Qt::CaseInsensitive);
        const QColor accent=recommended?QColor("#159F7B"):(warning?QColor("#C98D36"):(current?QColor("#548697"):QColor("#718997")));
        const QColor fill=dark?QColor(accent.red()/3+21,accent.green()/3+27,accent.blue()/3+35):
                               QColor(233,247,244);
        QFont f=option.font;f.setPointSizeF(9);f.setWeight(QFont::DemiBold);
        const int w=qMax(45,qMin(r.width()-19,qMax(72,QFontMetrics(f).width(status)+28)));
        const int x=rtl?r.right()-w-9:r.x()+9;
        const int h=child?29:32;
        const int y=r.y()+(r.height()-h)/2;
        p->setPen(Qt::NoPen);
        p->setBrush(warning?(dark?QColor("#4E4230"):QColor("#FFF4E6")):
                     (recommended?fill: (dark?QColor("#294655"):QColor("#EFF4F6"))));
        p->drawRoundedRect(QRectF(x,y,w,h),h/2.0,h/2.0);
        p->setFont(f);p->setPen(warning?QColor("#C88C36"):accent);
        p->drawText(QRect(x+9,y,w-18,h),Qt::AlignVCenter|Qt::AlignHCenter,
                    QFontMetrics(f).elidedText(status,Qt::ElideRight,w-18));
    }else if(col==2){
        const int rating=qBound(0,index.data(Qt::UserRole+2).toInt(),5);
        if(rating>0){
            const qreal step=18.7,star=7.7;
            const int total=static_cast<int>(5*step);
            const qreal start=r.left()+qMax(10,(r.width()-total)/2);
            const qreal centerY=r.center().y()-7;
            for(int i=0;i<5;++i){
                p->setPen(Qt::NoPen);
                p->setBrush(i<rating?QColor("#D7A03B"):(dark?QColor("#526474"):QColor("#CFD9DD")));
                p->drawPath(starPath(QPointF(start+i*step+8,centerY),star,3.5));
            }
            QFont tiny=option.font;tiny.setPointSizeF(9.0);p->setFont(tiny);p->setPen(muted);
            p->drawText(QRect(r.left()+4,r.center().y()+6,r.width()-8,18),
                        Qt::AlignCenter,index.data(Qt::DisplayRole).toString());
        }else{
            QFont f=option.font;f.setPointSizeF(9.0);p->setFont(f);p->setPen(muted);
            p->drawText(r.adjusted(7,0,-7,0),Qt::AlignCenter,
                        QFontMetrics(f).elidedText(index.data(Qt::DisplayRole).toString(),Qt::ElideRight,r.width()-14));
        }
    }
    p->restore();
}
