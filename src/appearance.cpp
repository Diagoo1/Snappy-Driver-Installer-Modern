#include "ui_font.h"
#include "appearance.h"
#include "ui_language.h"
#include "theme.h"
#include "vector_icons.h"
#include "sidebar.h"
#include "ui_elements.h"
#include <QApplication>
#include <QAbstractButton>
#include <QCheckBox>
#include <QPainter>
#include <QPainterPath>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QPushButton>
#include <QEvent>
#include <QShowEvent>
#include <QEasingCurve>
#include <QScrollArea>
#include <QComboBox>
#include <QStyleOptionButton>
#include <QCursor>
#include <QMouseEvent>
#include <QLinearGradient>
#include <QStyleOptionMenuItem>
#include <QLineEdit>
#include <QTextEdit>
#include <QPlainTextEdit>
#include <QContextMenuEvent>
#include <QClipboard>
#include <QScopedPointer>

TitleButton::TitleButton(QWidget *parent):QToolButton(parent),m_hover(0),m_animation(this,"hoverAmount",this){
    m_animation.setDuration(155);m_animation.setEasingCurve(QEasingCurve::OutCubic);
}
void TitleButton::animateHover(qreal value){
    m_animation.stop();
    if(qApp->property("reduceMotion").toBool()){setHoverAmount(value);return;}
    m_animation.setStartValue(m_hover);m_animation.setEndValue(value);m_animation.start();
}
void TitleButton::enterEvent(QEvent *event){if(isEnabled())animateHover(1);QToolButton::enterEvent(event);}
void TitleButton::leaveEvent(QEvent *event){animateHover(0);QToolButton::leaveEvent(event);}
void TitleButton::changeEvent(QEvent *event){if(event->type()==QEvent::EnabledChange && !isEnabled()){m_animation.stop();setHoverAmount(0);}QToolButton::changeEvent(event);}
void TitleButton::paintEvent(QPaintEvent*){
    QPainter painter(this);painter.setRenderHint(QPainter::Antialiasing);
    const bool close=objectName().contains(QStringLiteral("Close"),Qt::CaseInsensitive);
    const QRectF target=QRectF(rect()).adjusted(7,10,-7,-10);
    QColor color=close?QColor("#DC5363"):QColor(Qt::white);
    const bool keyboardFocus=hasFocus() && window()->testAttribute(Qt::WA_KeyboardFocusChange);
    color.setAlpha(qRound((close?175:25)*(isDown()?1:qMax(m_hover,keyboardFocus?.45:0.0))));
    painter.setPen(Qt::NoPen);painter.setBrush(color);painter.drawRoundedRect(target,10,10);
    const QSize size=iconSize();const QRect symbol((width()-size.width())/2,(height()-size.height())/2,size.width(),size.height());
    icon().paint(&painter,symbol,Qt::AlignCenter,isEnabled()?QIcon::Normal:QIcon::Disabled);
}

static bool darkSurface(QWidget *widget){
    return Theme::isDark(widget);
}
QPainterPath windowClip(QWidget *widget,qreal radius){
    const QPoint origin=widget->mapTo(widget->window(),QPoint());
    QPainterPath path;path.addRoundedRect(QRectF(widget->window()->rect()).translated(-origin.x(),-origin.y()),
        widget->window()->isMaximized()?0:radius,widget->window()->isMaximized()?0:radius);return path;
}
ActionButton::ActionButton(const QString &text,QWidget *parent):QPushButton(text,parent),m_hover(0),m_animation(this,"hoverAmount",this){
    setCursor(Qt::PointingHandCursor);setMinimumHeight(38);m_animation.setDuration(155);m_animation.setEasingCurve(QEasingCurve::OutCubic);
}
QSize ActionButton::sizeHint()const{const QSize native=QPushButton::sizeHint();return QSize(qMax(native.width(),fontMetrics().width(text())+32+(icon().isNull()?0:iconSize().width()+9)),qMax(38,native.height()));}
void ActionButton::animate(qreal value){m_animation.stop();if(qApp->property("reduceMotion").toBool()){setHoverAmount(value);return;}
    m_animation.setStartValue(m_hover);m_animation.setEndValue(value);m_animation.start();}
void ActionButton::enterEvent(QEvent *event){if(isEnabled())animate(1);QPushButton::enterEvent(event);}
void ActionButton::leaveEvent(QEvent *event){animate(0);QPushButton::leaveEvent(event);}
void ActionButton::paintEvent(QPaintEvent*){
    QPainter p(this);p.setRenderHint(QPainter::Antialiasing);const bool dark=darkSurface(this);
    const bool primary=objectName()==QStringLiteral("primaryButton");
    const bool quiet=objectName().contains(QStringLiteral("Link"),Qt::CaseInsensitive);
    const qreal amount=isEnabled()?(isDown()?1:m_hover):0;
    QColor base=primary?Theme::color("accent",dark):Theme::color("surface",dark);
    QColor hover=primary?Theme::color("accentHover",dark):Theme::color("elevated",dark);
    auto mix=[amount](int a,int b){return qRound(a+(b-a)*amount);};
    QColor fill(mix(base.red(),hover.red()),mix(base.green(),hover.green()),mix(base.blue(),hover.blue()));
    if(quiet)fill.setAlpha(qRound(255*amount));
    if(!isEnabled())p.setOpacity(.45);
    p.setPen(primary || quiet?Qt::NoPen:QPen(dark?QColor("#426474"):QColor("#D5E5E9"),1));p.setBrush(fill);
    p.drawRoundedRect(QRectF(rect()).adjusted(.5,.5,-.5,-.5),11,11);
    p.setPen(primary?QColor("#FFFFFF"):(dark?QColor("#E6F7FA"):QColor("#226071")));QFont face=font();face.setFamily(UiFont::family());face.setStyleHint(QFont::SansSerif);face.setStyleStrategy(QFont::PreferAntialias);p.setFont(face);
    QRect area=rect().adjusted(14,0,-14,0);
    if(!icon().isNull()){
        const int total=fontMetrics().width(text())+iconSize().width()+9;const bool rtl=layoutDirection()==Qt::RightToLeft;const int x=rtl?(width()+total)/2-iconSize().width():(width()-total)/2;
        icon().paint(&p,QRect(x,(height()-iconSize().height())/2,iconSize().width(),iconSize().height()));if(rtl)area.setRight(x-9);else area.setLeft(x+iconSize().width()+9);
        p.drawText(area,Qt::AlignAbsolute|(rtl?Qt::AlignRight:Qt::AlignLeft)|Qt::AlignVCenter,text());
    }else p.drawText(area,Qt::AlignCenter,text());
    if(hasFocus() && window()->testAttribute(Qt::WA_KeyboardFocusChange)){p.setBrush(Qt::NoBrush);p.setPen(QPen(QColor("#20BCAF"),1.3));p.drawRoundedRect(QRectF(rect()).adjusted(3,3,-3,-3),8,8);}
}
DialogHeader::DialogHeader(const QString &title,const QString &symbol,QDialog *dialog,bool arabic):QWidget(dialog),m_dragging(false){
    setFixedHeight(64);setObjectName(QStringLiteral("dialogHeader"));
    QHBoxLayout *line=new QHBoxLayout(this);line->setContentsMargins(22,0,9,0);line->setSpacing(12);
    QLabel *icon=new QLabel(this);icon->setPixmap(VectorIcons::pixmap(symbol,QColor("#BDF7F1"),25));line->addWidget(icon);
    QLabel *name=new QLabel(title,this);name->setAlignment(Qt::AlignAbsolute|(arabic?Qt::AlignRight:Qt::AlignLeft)|Qt::AlignVCenter);name->setStyleSheet(QStringLiteral("background:transparent;color:#FFFFFF;font-size:17px;font-weight:600;"));line->addWidget(name,1);
    TitleButton *close=new TitleButton(this);close->setObjectName(QStringLiteral("windowClose"));close->setIcon(VectorIcons::icon(QStringLiteral("close"),QColor("#FFFFFF"),19));close->setIconSize(QSize(19,19));close->setFixedSize(48,54);close->setCursor(Qt::PointingHandCursor);close->setToolTip(arabic?QString::fromUtf8("إغلاق"):QStringLiteral("Close"));line->addWidget(close);
    connect(close,&QToolButton::clicked,dialog,&QDialog::reject);
}
void DialogHeader::paintEvent(QPaintEvent*){/* Painted once by RoundedDialog to preserve outer alpha. */}
void DialogHeader::mousePressEvent(QMouseEvent *e){if(e->button()==Qt::LeftButton){m_dragging=true;m_offset=e->globalPos()-window()->pos();e->accept();}else QWidget::mousePressEvent(e);}
void DialogHeader::mouseMoveEvent(QMouseEvent *e){if(m_dragging && (e->buttons()&Qt::LeftButton)){window()->move(e->globalPos()-m_offset);e->accept();}else QWidget::mouseMoveEvent(e);}
void DialogHeader::mouseReleaseEvent(QMouseEvent *e){m_dragging=false;QWidget::mouseReleaseEvent(e);}
void AccentCard::paintEvent(QPaintEvent*){QPainter p(this);p.setRenderHint(QPainter::Antialiasing);QLinearGradient g(0,0,width(),height());g.setColorAt(0,Theme::color("heroStart"));g.setColorAt(.65,Theme::color("heroMiddle"));g.setColorAt(1,Theme::color("heroEnd"));p.setPen(Qt::NoPen);p.setBrush(g);p.drawRoundedRect(QRectF(rect()),20,20);p.setBrush(QColor(255,255,255,9));p.drawEllipse(QPointF(width()-55,20),125,125);p.drawEllipse(QPointF(width()-50,20),80,80);}
ProjectMenu::ProjectMenu(QWidget *parent):QMenu(parent),m_highlightY(-1),m_slide(this,"highlightY",this){
    setMinimumWidth(240);
    setStyleSheet(QStringLiteral("QMenu{background:transparent;border:0;padding:8px;font-size:13px;font-weight:400;} QMenu::item{background:transparent;padding:11px 22px;} QMenu::separator{height:1px;background:%1;margin:5px 10px;}").arg(darkSurface(this)?QStringLiteral("#3B5868"):QStringLiteral("#E2EBEF")));
    setWindowFlags(Qt::Popup|Qt::FramelessWindowHint|Qt::NoDropShadowWindowHint);setAttribute(Qt::WA_TranslucentBackground);m_slide.setDuration(120);m_slide.setEasingCurve(QEasingCurve::OutCubic);
    connect(this,&QMenu::hovered,this,[this](QAction *action){if(!action || !action->isEnabled() || action->isSeparator())return;const qreal y=actionGeometry(action).top();m_slide.stop();if(m_highlightY<0 || qApp->property("reduceMotion").toBool()){setHighlightY(y);return;}m_slide.setStartValue(m_highlightY);m_slide.setEndValue(y);m_slide.start();});
}
void ProjectMenu::paintEvent(QPaintEvent*){
    QPainter p(this);p.setRenderHint(QPainter::Antialiasing);const bool dark=darkSurface(this);p.setPen(QPen(Theme::color("border",dark),1));p.setBrush(Theme::color("surface",dark));p.drawRoundedRect(QRectF(rect()).adjusted(.5,.5,-.5,-.5),14,14);
    QAction *active=activeAction();if(active && active->isEnabled() && !active->isSeparator()){QRectF target=actionGeometry(active).adjusted(4,1,-4,-1);if(m_highlightY>=0)target.moveTop(m_highlightY+1);p.setPen(Qt::NoPen);p.setBrush(dark?QColor("#2B5264"):QColor("#E4F3EF"));p.drawRoundedRect(target,8,8);}
    for(QAction *action:actions()){
        if(!action->isVisible())continue;const QRect area=actionGeometry(action);
        if(action->isSeparator()){p.setPen(dark?QColor("#3B5868"):QColor("#E2EBEF"));p.drawLine(area.left()+10,area.center().y(),area.right()-10,area.center().y());continue;}
        QFont face=font();face.setFamily(UiFont::family());face.setStyleHint(QFont::SansSerif);face.setStyleStrategy(QFont::PreferAntialias);p.setFont(face);
        p.setPen(action->isEnabled()?(dark?QColor("#E7F3F7"):QColor("#234A58")):(dark?QColor("#91ACBA"):QColor("#8196A3")));
        const bool rtl=layoutDirection()==Qt::RightToLeft;QRect label=area.adjusted(22,0,-22,0);
        const QString text=action->text().section(QLatin1Char('\t'),0,0).remove(QLatin1Char('&'));
        const QString shortcut=action->text().contains(QLatin1Char('\t'))?action->text().section(QLatin1Char('\t'),1):action->shortcut().toString(QKeySequence::NativeText);
        if(!shortcut.isEmpty()){const int reserve=p.fontMetrics().width(shortcut)+24;QRect key=label;if(rtl){label.adjust(reserve,0,0,0);key.setRight(label.left()-12);}else{label.adjust(0,0,-reserve,0);key.setLeft(label.right()+12);}p.save();p.setPen(Theme::color("muted",dark));p.drawText(key,Qt::AlignAbsolute|(rtl?Qt::AlignLeft:Qt::AlignRight)|Qt::AlignVCenter,shortcut);p.restore();}
        p.drawText(label,Qt::AlignAbsolute|(rtl?Qt::AlignRight:Qt::AlignLeft)|Qt::AlignVCenter,text);
        if(action->isCheckable() && action->isChecked())VectorIcons::paint(&p,QStringLiteral("check"),QRectF(rtl?area.right()-19:area.left()+3,area.center().y()-7,14,14),QColor("#00A99E"));
    }
}
RoundedPanel::RoundedPanel(QWidget *p):QWidget(p){setAttribute(Qt::WA_TranslucentBackground);}
void RoundedPanel::paintEvent(QPaintEvent*){
    QPainter p(this);p.setRenderHint(QPainter::Antialiasing);
    p.setPen(QPen(Theme::color("border",darkSurface(this)),1));
    p.setBrush(Theme::color("surface",darkSurface(this)));
    p.drawRoundedRect(QRectF(rect()).adjusted(.5,.5,-.5,-.5),16,16);
}
RoundedDialog::RoundedDialog(QWidget *p):QDialog(p,Qt::Dialog|Qt::FramelessWindowHint){
    setAttribute(Qt::WA_TranslucentBackground);setModal(true);
}
void RoundedDialog::paintEvent(QPaintEvent*){
    QPainter p(this);p.setRenderHint(QPainter::Antialiasing);
    QPainterPath frame;frame.addRoundedRect(QRectF(rect()),22,22);p.setPen(Qt::NoPen);
    QWidget *header=findChild<QWidget*>(QStringLiteral("dialogHeader"));
    if(header){const QRect bar=header->geometry();p.save();p.setClipRect(QRect(0,bar.bottom()+1,width(),height()-bar.height()));p.setBrush(Theme::color("background",darkSurface(this)));p.drawPath(frame);p.restore();QLinearGradient g(0,0,width(),0);g.setColorAt(0,Theme::color("headerStart"));g.setColorAt(1,Theme::color("headerEnd"));p.save();p.setClipRect(bar);p.setBrush(g);p.drawPath(frame);p.restore();}
    else {p.setBrush(Theme::color("background",darkSurface(this)));p.drawPath(frame);}
}
void RoundedDialog::showEvent(QShowEvent *e){
    QDialog::showEvent(e);
    if(qApp->property("reduceMotion").toBool())return;
    QPropertyAnimation *fade=new QPropertyAnimation(this,"windowOpacity",this);
    fade->setDuration(180);fade->setStartValue(.0);fade->setEndValue(1.0);
    fade->setEasingCurve(QEasingCurve::OutCubic);fade->start(QAbstractAnimation::DeleteWhenStopped);
}
HoverSurface::HoverSurface(QWidget *p):QWidget(p),m_amount(0),m_animation(this,"amount",this){
    setAttribute(Qt::WA_TransparentForMouseEvents);setAttribute(Qt::WA_NoSystemBackground);
    setObjectName(QStringLiteral("motionHighlight"));
    m_animation.setDuration(180);m_animation.setEasingCurve(QEasingCurve::OutCubic);
    setGeometry(p->rect());show();
}
void HoverSurface::transition(bool entered){
    m_animation.stop();
    if(qApp->property("reduceMotion").toBool()){setAmount(entered?1:0);return;}
    m_animation.setStartValue(m_amount);m_animation.setEndValue(entered?1.0:0.0);m_animation.start();
}
void HoverSurface::paintEvent(QPaintEvent*){
    if(m_amount<.001)return;
    QPainter p(this);p.setRenderHint(QPainter::Antialiasing);
    const bool close=parentWidget()->objectName().contains(QStringLiteral("Close"),Qt::CaseInsensitive);
    const QColor tint=close?QColor("#E24F65"):QColor("#00B7AB");
    QColor fill=tint;fill.setAlpha(qRound((close?75:18)*m_amount));
    QColor border=tint;border.setAlpha(qRound(28*m_amount));
    p.setBrush(fill);p.setPen(QPen(border,1));
    QRectF area=QRectF(rect()).adjusted(1,1,-1,-1);
    if(QCheckBox *check=qobject_cast<QCheckBox*>(parentWidget())){
        QStyleOptionButton option;option.initFrom(check);option.text=check->text();
        area=check->style()->subElementRect(QStyle::SE_CheckBoxIndicator,&option,check).adjusted(-2,-2,2,2);
    }
    p.drawRoundedRect(area,5,5);
}
static bool textContextMenu(QWidget *widget,QContextMenuEvent *event){
    if(!widget || widget->contextMenuPolicy()!=Qt::DefaultContextMenu)return false;
    if(widget->parentWidget() && (qobject_cast<QTextEdit*>(widget->parentWidget()) || qobject_cast<QPlainTextEdit*>(widget->parentWidget())))widget=widget->parentWidget();
    QScopedPointer<QMenu> standard;QLabel *label=qobject_cast<QLabel*>(widget);
    if(QLineEdit *edit=qobject_cast<QLineEdit*>(widget))standard.reset(edit->createStandardContextMenu());
    else if(QTextEdit *edit=qobject_cast<QTextEdit*>(widget))standard.reset(edit->createStandardContextMenu());
    else if(QPlainTextEdit *edit=qobject_cast<QPlainTextEdit*>(widget))standard.reset(edit->createStandardContextMenu());
    else if(!label || !(label->textInteractionFlags()&(Qt::TextSelectableByMouse|Qt::TextSelectableByKeyboard)))return false;
    ProjectMenu menu(widget);menu.setObjectName(QStringLiteral("textContextMenu"));menu.setLayoutDirection(UiLanguage::isRtl()?Qt::RightToLeft:Qt::LeftToRight);
    if(standard){for(QAction *action:standard->actions()){
        const QString original=action->text(),name=original.section(QLatin1Char('\t'),0,0).remove(QLatin1Char('&'));
        const QStringList keys=QStringList()<<QStringLiteral("Undo")<<QStringLiteral("Redo")<<QStringLiteral("Cut")<<QStringLiteral("Copy")<<QStringLiteral("Paste")<<QStringLiteral("Delete")<<QStringLiteral("Select all");
        for(const QString &key:keys)if(name.compare(key,Qt::CaseInsensitive)==0){action->setText(UiLanguage::text(key)+(original.contains(QLatin1Char('\t'))?QLatin1Char('\t')+original.section(QLatin1Char('\t'),1):QString()));break;}
        menu.addAction(action);
    }}
    else {
        QAction *copy=menu.addAction(UiLanguage::text(QStringLiteral("Copy"),QString::fromUtf8("نسخ")));copy->setShortcut(QKeySequence::Copy);copy->setEnabled(label->hasSelectedText());QObject::connect(copy,&QAction::triggered,label,[label](){qApp->clipboard()->setText(label->selectedText());});
        menu.addSeparator();QAction *all=menu.addAction(UiLanguage::text(QStringLiteral("Select all"),QString::fromUtf8("تحديد الكل")));all->setShortcut(QKeySequence::SelectAll);all->setEnabled(!label->text().isEmpty());QObject::connect(all,&QAction::triggered,label,[label](){label->setSelection(0,label->text().size());});
    }
    menu.exec(event->globalPos());event->accept();return true;
}
class InterfaceMotion : public QObject {
public:
    explicit InterfaceMotion(QObject *p):QObject(p){}
    bool eventFilter(QObject *object,QEvent *event) Q_DECL_OVERRIDE {
        if(event->type()==QEvent::ContextMenu){QWidget *widget=qobject_cast<QWidget*>(object);if(widget && textContextMenu(widget,static_cast<QContextMenuEvent*>(event)))return true;}
        QWidget *button=qobject_cast<QAbstractButton*>(object);
        if(!button)button=qobject_cast<QComboBox*>(object);
        if(!button || qobject_cast<ActionButton*>(button) || qobject_cast<TitleButton*>(button) || qobject_cast<SidebarItem*>(button) ||
           qobject_cast<GradientButton*>(button) || qobject_cast<ToggleSwitch*>(button))return false;
        if(event->type()==QEvent::Polish){button->setMouseTracking(true);}
        if(event->type()==QEvent::Enter || event->type()==QEvent::Leave || event->type()==QEvent::Resize || event->type()==QEvent::MouseMove){
            HoverSurface *surface=button->findChild<HoverSurface*>(QStringLiteral("motionHighlight"),Qt::FindDirectChildrenOnly);
            if(event->type()==QEvent::Resize){if(surface)surface->setGeometry(button->rect());return false;}
            if(!button->isEnabled())return false;
            if(!surface)surface=new HoverSurface(button);
            bool entered=event->type()!=QEvent::Leave;
            if(QCheckBox *check=qobject_cast<QCheckBox*>(button)){
                QStyleOptionButton option;option.initFrom(check);option.text=check->text();
                entered=entered && check->style()->subElementRect(QStyle::SE_CheckBoxIndicator,&option,check).contains(check->mapFromGlobal(QCursor::pos()));
            }
            surface->raise();surface->transition(entered);
        }
        return false;
    }
};
void installInterfaceMotion(){qApp->installEventFilter(new InterfaceMotion(qApp));}
bool projectMessage(QWidget *parent,const QString &title,const QString &message,bool question,bool arabic){
    arabic=arabic || UiLanguage::isRtl();
    RoundedDialog dialog(parent);dialog.setObjectName(QStringLiteral("projectDialog"));dialog.setWindowTitle(title);
    dialog.setLayoutDirection(arabic?Qt::RightToLeft:Qt::LeftToRight);
    dialog.setStyleSheet(Theme::styleSheet(darkSurface(parent)));
    dialog.setMinimumWidth(480);dialog.setMaximumWidth(560);
    QVBoxLayout *frame=new QVBoxLayout(&dialog);frame->setContentsMargins(0,0,0,0);frame->setSpacing(0);
    frame->addWidget(new DialogHeader(title,question?QStringLiteral("shield"):QStringLiteral("info"),&dialog,arabic));
    QWidget *content=new QWidget(&dialog);QVBoxLayout *layout=new QVBoxLayout(content);layout->setContentsMargins(26,23,26,23);layout->setSpacing(18);
    QLabel *brand=new QLabel(arabic?QString::fromUtf8("مدير تعريفات الأجهزة · واجهة طارق صادق"):QStringLiteral("Snappy Driver Installer · UI by Tarek Sadek"),content);brand->setObjectName(QStringLiteral("detailHeading"));brand->setProperty("skipTranslation",true);brand->setAlignment(Qt::AlignAbsolute|(arabic?Qt::AlignRight:Qt::AlignLeft));layout->addWidget(brand);
    QLabel *body=new QLabel(message,content);body->setObjectName(QStringLiteral("bodyText"));body->setWordWrap(true);body->setTextFormat(Qt::PlainText);body->setAlignment(Qt::AlignAbsolute|(arabic?Qt::AlignRight:Qt::AlignLeft)|Qt::AlignTop);body->setMinimumWidth(420);
    if(message.size()>500 || message.count(QLatin1Char('\n'))>6){
        QScrollArea *scroll=new QScrollArea(content);scroll->setWidgetResizable(true);scroll->setFrameShape(QFrame::NoFrame);
        scroll->setWidget(body);scroll->setMinimumHeight(180);scroll->setMaximumHeight(320);layout->addWidget(scroll);
    }else layout->addWidget(body);
    QHBoxLayout *actions=new QHBoxLayout;actions->setSpacing(10);actions->addStretch();
    if(question){QPushButton *cancel=new ActionButton(arabic?QString::fromUtf8("إلغاء"):QStringLiteral("Cancel"),content);cancel->setObjectName(QStringLiteral("outlineButton"));cancel->setDefault(true);cancel->setFocus();actions->addWidget(cancel);QObject::connect(cancel,&QPushButton::clicked,&dialog,&QDialog::reject);}
    QPushButton *accept=new ActionButton(arabic?(question?QString::fromUtf8("متابعة"):QString::fromUtf8("حسنًا")):(question?QStringLiteral("Continue"):QStringLiteral("OK")),content);
    accept->setObjectName(QStringLiteral("primaryButton"));accept->setDefault(!question);actions->addWidget(accept);QObject::connect(accept,&QPushButton::clicked,&dialog,&QDialog::accept);
    layout->addLayout(actions);frame->addWidget(content);return dialog.exec()==QDialog::Accepted;
}
