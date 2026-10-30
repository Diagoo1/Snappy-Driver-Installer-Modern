#include "driver_details_dialog.h"
#include "ui_language.h"
#include "searchable_language_combo.h"
#include "vector_icons.h"
#include "theme.h"
#include "ui_font.h"
#include <QTabBar>
#include <QCursor>
#include <QResizeEvent>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGridLayout>
#include <QScrollArea>
#include <QFrame>
#include <QLabel>
#include <QToolButton>
#include <QPushButton>
#include <QFont>
#include <QMouseEvent>
#include <QPainter>
#include <QLinearGradient>
#include <QPainterPath>
#include <QComboBox>
#include <QTabWidget>
#include <QApplication>
#include <QDesktopWidget>
#include <QtMath>

class EqualTabBar : public QTabBar {
public:explicit EqualTabBar(QWidget *parent):QTabBar(parent){QFont face=QApplication::font();face.setFamily(UiFont::family());face.setWeight(QFont::DemiBold);setFont(face);setDrawBase(false);}
protected:void paintEvent(QPaintEvent*) Q_DECL_OVERRIDE {QPainter p(this);p.setRenderHint(QPainter::Antialiasing);const bool dark=Theme::isDark(this),rtl=layoutDirection()==Qt::RightToLeft;for(int i=0;i<count();++i){const QRect area=tabRect(i).adjusted(4,1,-4,-8);const bool chosen=i==currentIndex(),hovered=underMouse() && tabAt(mapFromGlobal(QCursor::pos()))==i;QColor fill=Theme::color(chosen || hovered?"elevated":"surface",dark);p.setPen(QPen(chosen?Theme::color("link",dark):Theme::color("border",dark),1));p.setBrush(fill);p.drawRoundedRect(area,11,11);QFont face=QApplication::font();face.setFamily(UiFont::family());face.setPointSizeF(10);face.setWeight(chosen?QFont::DemiBold:QFont::Normal);p.setFont(face);p.setPen(Theme::color("text",dark));const QString label=p.fontMetrics().elidedText(tabText(i),Qt::ElideRight,area.width()-42);const int total=p.fontMetrics().width(label)+28;const int left=area.center().x()-total/2;tabIcon(i).paint(&p,QRect(rtl?left+total-19:left,area.center().y()-9,19,19));p.drawText(QRect(rtl?left:left+28,area.top(),total-28,area.height()),Qt::AlignCenter,label);}}
    QSize tabSizeHint(int index)const Q_DECL_OVERRIDE {QSize size=QTabBar::tabSizeHint(index);size.setWidth(qMax(1,width()/qMax(1,count())));return size;}
};
class FullWidthTabs : public QTabWidget {
public:explicit FullWidthTabs(QWidget *parent):QTabWidget(parent){setTabBar(new EqualTabBar(this));tabBar()->setExpanding(true);tabBar()->setUsesScrollButtons(false);setIconSize(QSize(19,19));}
protected:void resizeEvent(QResizeEvent *event) Q_DECL_OVERRIDE {QTabWidget::resizeEvent(event);tabBar()->setFixedWidth(width());}
};

class ComparisonCard : public SurfaceCard {
public:ComparisonCard(const QColor &accent,QWidget *parent):SurfaceCard(parent),m_accent(accent){
    setSizePolicy(QSizePolicy::Preferred,QSizePolicy::Maximum);
    setProperty("tone",accent.blue()>accent.green()?QStringLiteral("violet"):accent.red()>accent.green()?QStringLiteral("amber"):accent.blue()>accent.red() && accent.blue()>accent.green()*.95?QStringLiteral("blue"):QStringLiteral("green"));
    if(accent==QColor("#469DCD"))setProperty("tone",QStringLiteral("blue"));
}
protected:void paintEvent(QPaintEvent*) Q_DECL_OVERRIDE {
    SurfaceCard::paintEvent(0);QPainter painter(this);painter.setRenderHint(QPainter::Antialiasing);
    const QRectF face=QRectF(rect()).adjusted(2,2,-2,-7);QPainterPath clip;clip.addRoundedRect(face,14,14);painter.setClipPath(clip);
    QLinearGradient tint(0,face.top(),0,qMin(qreal(160),face.bottom()));QColor color=m_accent;color.setAlpha(Theme::isDark(this)?17:10);tint.setColorAt(0,color);color.setAlpha(0);tint.setColorAt(1,color);painter.fillPath(clip,tint);
}
private:QColor m_accent;
};

class MatchStars : public QWidget {
public:MatchStars(int stars,QWidget *parent):QWidget(parent),m_stars(qBound(0,stars,5)){setObjectName(QStringLiteral("comparisonStars"));setFixedSize(112,22);setProperty("matchPriority",m_stars);}
protected:void paintEvent(QPaintEvent*) Q_DECL_OVERRIDE {
    QPainter p(this);p.setRenderHint(QPainter::Antialiasing);p.setPen(Qt::NoPen);
    for(int s=0;s<5;++s){const int slot=layoutDirection()==Qt::RightToLeft?4-s:s;QPainterPath star;for(int point=0;point<10;++point){const qreal angle=(-90+point*36)*3.141592653589793/180;const qreal radius=point%2?3.7:8;const QPointF pos(11+slot*22+qCos(angle)*radius,11+qSin(angle)*radius);if(point)star.lineTo(pos);else star.moveTo(pos);}star.closeSubpath();p.setBrush(s<m_stars?QColor("#DCAF56"):Theme::color("border",Theme::isDark(this)));p.drawPath(star);}
}
private:int m_stars;
};

static QLabel *addText(const QString &text,const char *name,QWidget *parent)
{
    QLabel *v=new QLabel(text,parent);
    v->setObjectName(QString::fromLatin1(name));QFont face=QApplication::font();face.setFamily(UiFont::family());face.setWeight(QFont::Normal);v->setFont(face);
    v->setWordWrap(true);
    v->setTextInteractionFlags(Qt::TextSelectableByMouse);
    return v;
}
static void addRow(QGridLayout *grid,int row,const QString &name,const QString &value,QWidget *parent)
{
    grid->addWidget(addText(name,"mutedText",parent),row,0);
    QLabel *v=addText(value.isEmpty()?QStringLiteral("—"):value,"bodyText",parent);
    v->setMinimumWidth(100);
    v->setProperty("keepLtr",true);v->setProperty("skipTranslation",true);v->setLayoutDirection(Qt::LeftToRight);
    grid->addWidget(v,row,1);
}
static QFrame *candidatePanel(const DriverCandidate &d,bool demo,bool verified,QWidget *parent,bool technical=false,bool arabic=false)
{
    auto tr=[arabic](const char *en,const char *ar){return UiLanguage::text(QString::fromUtf8(en),QString::fromUtf8(ar));};
    const bool installed=d.status==QStringLiteral("installed"),recommended=d.status==QStringLiteral("recommended");
    const QColor accent=installed?QColor("#469DCD"):recommended?QColor("#23B9A6"):QColor("#C6A25C");
    QFrame *card=new ComparisonCard(accent,parent);
    card->setObjectName(QStringLiteral("detailSection"));
    card->setProperty("candidateRole",installed?QStringLiteral("installed"):recommended?QStringLiteral("recommended"):QStringLiteral("alternative"));
    QVBoxLayout *box=new QVBoxLayout(card);
    box->setContentsMargins(21,23,21,22);
    box->setSpacing(16);
    QString heading=d.status==QStringLiteral("installed")?
           (verified?tr("INSTALLED DRIVER","التعريف المثبت"):tr("CURRENT DRIVER","التعريف الحالي")):
           (d.status==QStringLiteral("recommended")?(demo?tr("RECOMMENDED - DEMO","موصى به — توضيحي"):tr("RECOMMENDED DRIVER","التعريف الموصى به")):(demo?tr("ALTERNATIVE - DEMO","بديل — توضيحي"):tr("AVAILABLE DRIVER","تعريف متاح")));
    QHBoxLayout *title=new QHBoxLayout;QLabel *symbol=new QLabel(card);symbol->setPixmap(VectorIcons::pixmap(installed?QStringLiteral("monitor"):QStringLiteral("shield"),accent,23));title->addWidget(symbol);
    const QColor headingInk=Theme::isDark(parent)?accent.lighter(135):accent.darker(140);
    QLabel *caption=addText(heading,"detailHeading",card);caption->setStyleSheet(QStringLiteral("color:%1;background:transparent;font-size:13px;font-weight:600;").arg(headingInk.name()));title->addWidget(caption,1);box->addLayout(title);
    QLabel *version=addText(d.version.isEmpty()?QStringLiteral("—"):d.version,"detailVersion",card);version->setProperty("keepLtr",true);version->setProperty("skipTranslation",true);version->setLayoutDirection(Qt::LeftToRight);version->setAlignment(Qt::AlignAbsolute|(UiLanguage::isRtl()?Qt::AlignRight:Qt::AlignLeft));box->addWidget(version);
    if(!technical){if(!installed && d.engineToken && d.matchStars){QHBoxLayout *rating=new QHBoxLayout;rating->addWidget(new MatchStars(d.matchStars,card));rating->addStretch();box->addLayout(rating);}else box->addSpacing(22);}
    QGridLayout *fields=new QGridLayout();
    fields->setHorizontalSpacing(17);fields->setVerticalSpacing(11);
    addRow(fields,0,tr("Date","التاريخ"),d.date,card);
    addRow(fields,1,tr("Provider","المزوّد"),d.provider,card);
    addRow(fields,2,QStringLiteral("INF"),d.inf,card);
    fields->setColumnStretch(0,0);fields->setColumnStretch(1,1);
    if(!technical){
        addRow(fields,4,tr("Driver pack (.7z)","حزمة التعريف (.7z)"),d.packageSource.isEmpty()?tr("No matching archive in this library","لا توجد حزمة مطابقة في هذه المكتبة"):d.packageSource,card);
        addRow(fields,3,tr("Availability","التوفر"),d.status==QStringLiteral("installed")?tr("Installed on this computer","مثبت على هذا الجهاز"):
            (demo?tr("Demo candidate","إصدار توضيحي"):(d.local?tr("Local package present","الحزمة متوفرة محليًا"):tr("Package missing","الحزمة غير متوفرة"))),card);
        if(d.engineToken){
            addRow(fields,5,tr("Match priority","أولوية المطابقة"),d.matchStars?tr("%1 / 5 · Origin order #%2","الأولوية %1 من 5 · الترتيب الأصلي %2").arg(d.matchStars).arg(d.originalOrder):tr("Not eligible","غير مؤهل"),card);
        }
    }else{
        addRow(fields,4,tr("Section","القسم"),d.section,card);
        addRow(fields,5,tr("Hardware ID","معرّف الجهاز"),d.hardwareId,card);
        addRow(fields,6,tr("Package source","مصدر الحزمة"),d.packageSource,card);
        if(d.engineToken){
            addRow(fields,7,tr("Original order","الترتيب الأصلي"),QStringLiteral("#%1").arg(d.originalOrder),card);
            addRow(fields,8,tr("Original match score","درجة المطابقة الأصلية"),QStringLiteral("0x%1").arg(d.rawScore,8,16,QLatin1Char('0')),card);
            addRow(fields,9,tr("Original status flags","علامات الحالة الأصلية"),QStringLiteral("0x%1").arg(d.statusBits,0,16),card);
            addRow(fields,10,tr("Eligible","أهلية التثبيت"),d.valid?tr("Yes, subject to install preflight","نعم، بعد التحقق قبل التثبيت"):tr("No","لا"),card);
        }
    }
    box->addLayout(fields);
    box->addStretch(1);
    return card;
}
DriverDetailsDialog::DriverDetailsDialog(const DriverEntry &entry,bool demo,bool arabic,QWidget *parent)
    : RoundedDialog(parent)
{
    setWindowFlags(Qt::Dialog|Qt::FramelessWindowHint);
    setModal(true);
    setObjectName(QStringLiteral("driverDetailsDialog"));
    setWindowTitle(arabic?QString::fromUtf8("مقارنة التعريفات"):QStringLiteral("Driver comparison"));
    setMinimumSize(680,490);
    const QRect usable=QApplication::desktop()->availableGeometry(parent);
    resize(qMin(1000,usable.width()-36),qMin(630,usable.height()-36));
    QVBoxLayout *main=new QVBoxLayout(this);
    main->setContentsMargins(0,0,0,0);
    main->setSpacing(0);
    main->addWidget(new DialogHeader(arabic?QString::fromUtf8("مقارنة التعريفات"):QStringLiteral("Driver comparison"),QStringLiteral("chip"),this,arabic));

    QWidget *body=new QWidget(this);
    body->setObjectName(QStringLiteral("comparisonBody"));
    QVBoxLayout *content=new QVBoxLayout(body);
    content->setContentsMargins(22,18,22,20);
    content->setSpacing(13);
    QHBoxLayout *header=new QHBoxLayout();
    QLabel *icon=new QLabel(body);
    icon->setPixmap(VectorIcons::pixmap(VectorIcons::forDeviceClass(entry.device.deviceClass+QStringLiteral(" ")+entry.device.name),QColor("#18A9B6"),36));
    header->addWidget(icon);
    QVBoxLayout *headText=new QVBoxLayout();
    headText->setSpacing(4);
    headText->addWidget(addText(entry.device.name,"dialogTitle",body));
    headText->addWidget(addText(entry.device.manufacturer+QStringLiteral(" · ")+entry.device.deviceClass,"mutedText",body));
    header->addLayout(headText,1);
    content->addLayout(header);
    QLabel *status=addText(demo?QStringLiteral("DEMO DATA — candidate versions, dates and ratings are illustrative. No driver is installed."):
        entry.sdioVerified?QStringLiteral("Data copied from the in-process SDIO Origin matcher. Original order and raw score are shown; they are not security ratings. Windows installation is subject to build capabilities and preflight validation."):
        QStringLiteral("Windows PnP data only. Available drivers, INF match score and alternatives need the SDIO Origin matcher."),"demoText",body);
    if(demo || !entry.sdioVerified)content->addWidget(status);else status->hide();

    QWidget *pickerSurface=new QWidget(body);pickerSurface->setObjectName(QStringLiteral("comparisonPickerSurface"));QHBoxLayout *pickerRow=new QHBoxLayout(pickerSurface);pickerRow->setContentsMargins(14,10,10,10);pickerRow->setSpacing(16);
    pickerRow->addWidget(addText(arabic?QString::fromUtf8("الإصدار المتاح"):QStringLiteral("Available version"),"mutedText",body));
    QComboBox *picker=new SearchableLanguageCombo(body);picker->setObjectName(QStringLiteral("candidatePicker"));
    for(int i=1;i<entry.candidates.size();++i){
        const DriverCandidate &candidate=entry.candidates[i];
        picker->addItem(candidate.version+QStringLiteral("  ·  ")+candidate.date+
            (candidate.recommended?(arabic?QString::fromUtf8("  ·  موصى به"):QStringLiteral("  ·  Recommended")):QString()),i);
    }
    picker->setEnabled(picker->count()>0);
    if(!picker->count())picker->addItem(arabic?QString::fromUtf8("لا توجد إصدارات متاحة"):QStringLiteral("No available candidates"),-1);
    pickerRow->addWidget(picker,1);content->addWidget(pickerSurface);
    QTabWidget *tabs=new FullWidthTabs(body);tabs->setObjectName(QStringLiteral("comparisonTabs"));
    auto page=[tabs](const QString &title)->QWidget*{
        QScrollArea *scroll=new QScrollArea(tabs);scroll->setObjectName(QStringLiteral("dialogScroll"));
        scroll->viewport()->setObjectName(QStringLiteral("comparisonViewport"));
        scroll->setWidgetResizable(true);scroll->setFrameShape(QFrame::NoFrame);
        QWidget *contents=new QWidget(scroll);contents->setObjectName(QStringLiteral("comparisonPage"));
        scroll->setWidget(contents);tabs->addTab(scroll,title);return contents;
    };
    QWidget *overview=page(arabic?QString::fromUtf8("المقارنة"):QStringLiteral("Comparison"));
    QHBoxLayout *compare=new QHBoxLayout(overview);compare->setContentsMargins(0,12,0,0);compare->setSpacing(14);
    DriverCandidate installedData=entry.candidates.isEmpty()?DriverCandidate():entry.candidates[0];const int reinstallSource=reinstallCandidateIndex(entry);if(reinstallSource>0)installedData.packageSource=entry.candidates[reinstallSource].packageSource;
    QFrame *installedCard=entry.candidates.isEmpty()?0:candidatePanel(installedData,demo,entry.sdioDriverPresent,overview,false,arabic);
    if(installedCard)compare->addWidget(installedCard,1,Qt::AlignTop);
    QWidget *candidateHost=new QWidget(overview);candidateHost->setObjectName(QStringLiteral("candidateHost"));candidateHost->setSizePolicy(QSizePolicy::Preferred,QSizePolicy::Maximum);QVBoxLayout *candidateBox=new QVBoxLayout(candidateHost);candidateBox->setContentsMargins(0,0,0,0);candidateBox->setAlignment(Qt::AlignTop);
    compare->addWidget(candidateHost,1,Qt::AlignTop);
    QWidget *hardware=page(arabic?QString::fromUtf8("الجهاز"):QStringLiteral("Device"));
    QVBoxLayout *hw=new QVBoxLayout(hardware);hw->setContentsMargins(0,12,0,0);hw->setSpacing(13);
    const QStringList hardwareTitles=QStringList()<<UiLanguage::text(QStringLiteral("Device instance ID"),QString::fromUtf8("معرّف مثيل الجهاز"))<<UiLanguage::text(QStringLiteral("Hardware ID"),QString::fromUtf8("معرّف الجهاز"))<<UiLanguage::text(QStringLiteral("Windows PnP problem"),QString::fromUtf8("حالة الجهاز في ويندوز"));
    const QStringList hardwareValues=QStringList()<<entry.device.instanceId<<entry.device.hardwareId<<(entry.device.hasProblem?UiLanguage::text(QStringLiteral("Code %1"),QString::fromUtf8("رمز %1")).arg(entry.device.problemCode):UiLanguage::text(QStringLiteral("None reported"),QString::fromUtf8("لا توجد مشكلات مسجّلة")));
    for(int i=0;i<3;++i){const QColor tone=i==0?QColor("#469DCD"):i==1?QColor("#9C88D1"):entry.device.hasProblem?QColor("#D7A14C"):QColor("#23B9A6");QFrame *card=new ComparisonCard(tone,hardware);card->setObjectName(QStringLiteral("hardwareIdentityCard"));QVBoxLayout *stack=new QVBoxLayout(card);stack->setContentsMargins(20,18,20,18);stack->setSpacing(11);QHBoxLayout *caption=new QHBoxLayout;QLabel *mark=new QLabel(card);mark->setPixmap(VectorIcons::pixmap(i==2?QStringLiteral("shield"):QStringLiteral("chip"),tone,23));caption->addWidget(mark);caption->addWidget(addText(hardwareTitles[i],"sectionTitle",card),1);stack->addLayout(caption);QLabel *value=addText(hardwareValues[i].isEmpty()?QStringLiteral("—"):hardwareValues[i],"bodyText",card);value->setObjectName(QStringLiteral("hardwareIdentityValue"));value->setStyleSheet(QStringLiteral("background:transparent;color:%1;font-size:13px;font-weight:400;").arg(Theme::color("text",Theme::isDark(hardware)).name()));value->setProperty("keepLtr",i<2);value->setProperty("skipTranslation",true);if(i<2)value->setLayoutDirection(Qt::LeftToRight);value->setAlignment(Qt::AlignAbsolute|(arabic?Qt::AlignRight:Qt::AlignLeft));stack->addWidget(value);hw->addWidget(card);}
    hw->addStretch(1);
    QWidget *technical=page(arabic?QString::fromUtf8("بيانات المحرك"):QStringLiteral("Engine details"));
    QVBoxLayout *technicalBox=new QVBoxLayout(technical);technicalBox->setContentsMargins(0,12,0,0);
    const char *tabIcons[]={"layers","chip","settings"};for(int i=0;i<3;++i)tabs->setTabIcon(i,VectorIcons::icon(QString::fromLatin1(tabIcons[i]),Theme::color("link",Theme::isDark(parent)),19));
    auto update=[entry,demo,arabic,candidateBox,candidateHost,technicalBox,technical,picker,installedCard](){
        while(QLayoutItem *item=candidateBox->takeAt(0)){delete item->widget();delete item;}
        while(QLayoutItem *item=technicalBox->takeAt(0)){delete item->widget();delete item;}
        const int index=picker->itemData(picker->currentIndex()).toInt();
        if(index>0 && index<entry.candidates.size()){
            QFrame *available=candidatePanel(entry.candidates[index],demo,entry.sdioDriverPresent,candidateHost,false,arabic);candidateBox->addWidget(available);
            if(installedCard){installedCard->setMinimumHeight(0);const int height=qMax(installedCard->sizeHint().height(),available->sizeHint().height());installedCard->setMinimumHeight(height);available->setMinimumHeight(height);}
            technicalBox->addWidget(candidatePanel(entry.candidates[index],demo,entry.sdioDriverPresent,technical,true,arabic));
        }
        technicalBox->addStretch(1);
    };
    connect(picker,static_cast<void(QComboBox::*)(int)>(&QComboBox::currentIndexChanged),this,[update](int){update();});
    update();
    content->addWidget(tabs,1);
    main->addWidget(body,1);
}
