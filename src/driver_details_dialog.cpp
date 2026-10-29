#include "driver_details_dialog.h"
#include "ui_language.h"
#include "searchable_language_combo.h"
#include "vector_icons.h"
#include "theme.h"
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

class ComparisonCard : public QFrame {
public:ComparisonCard(const QColor &accent,QWidget *parent):QFrame(parent),m_accent(accent){setSizePolicy(QSizePolicy::Preferred,QSizePolicy::Maximum);}
protected:void paintEvent(QPaintEvent*) Q_DECL_OVERRIDE {
    const bool dark=Theme::isDark(this);QPainter p(this);p.setRenderHint(QPainter::Antialiasing);const QRectF area=QRectF(rect()).adjusted(.5,.5,-.5,-.5);QPainterPath clip;clip.addRoundedRect(area,15,15);p.setClipPath(clip);
    QLinearGradient fill(0,0,0,height());const QColor base=Theme::color("surface",dark);auto mix=[base,this](qreal a){return QColor(qRound(base.red()*(1-a)+m_accent.red()*a),qRound(base.green()*(1-a)+m_accent.green()*a),qRound(base.blue()*(1-a)+m_accent.blue()*a));};fill.setColorAt(0,mix(dark?.13:.065));fill.setColorAt(.65,base);p.fillPath(clip,fill);p.fillRect(QRectF(0,0,width(),3),m_accent);p.setClipping(false);p.setBrush(Qt::NoBrush);p.setPen(QPen(Theme::color("border",dark),1));p.drawRoundedRect(area,15,15);
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
    v->setObjectName(QString::fromLatin1(name));
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
    QLabel *caption=addText(heading,"detailHeading",card);caption->setStyleSheet(QStringLiteral("color:%1;background:transparent;font-size:11px;font-weight:700;").arg(headingInk.name()));title->addWidget(caption,1);box->addLayout(title);
    QLabel *version=addText(d.version.isEmpty()?QStringLiteral("—"):d.version,"detailVersion",card);version->setProperty("keepLtr",true);version->setProperty("skipTranslation",true);version->setLayoutDirection(Qt::LeftToRight);version->setAlignment(Qt::AlignAbsolute|(UiLanguage::isRtl()?Qt::AlignRight:Qt::AlignLeft));box->addWidget(version);
    if(!technical){if(!installed && d.engineToken && d.matchStars){QHBoxLayout *rating=new QHBoxLayout;rating->addWidget(new MatchStars(d.matchStars,card));rating->addStretch();box->addLayout(rating);}else box->addSpacing(22);}
    QGridLayout *fields=new QGridLayout();
    fields->setHorizontalSpacing(17);fields->setVerticalSpacing(11);
    addRow(fields,0,tr("Date","التاريخ"),d.date,card);
    addRow(fields,1,tr("Provider","المزوّد"),d.provider,card);
    addRow(fields,2,QStringLiteral("INF"),d.inf,card);
    fields->setColumnStretch(0,0);fields->setColumnStretch(1,1);
    if(!technical){
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
    icon->setPixmap(VectorIcons::pixmap(VectorIcons::forDeviceClass(entry.device.deviceClass),QColor("#18A9B6"),36));
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
    QTabWidget *tabs=new QTabWidget(body);tabs->setObjectName(QStringLiteral("comparisonTabs"));
    auto page=[tabs](const QString &title)->QWidget*{
        QScrollArea *scroll=new QScrollArea(tabs);scroll->setObjectName(QStringLiteral("dialogScroll"));
        scroll->viewport()->setObjectName(QStringLiteral("comparisonViewport"));
        scroll->setWidgetResizable(true);scroll->setFrameShape(QFrame::NoFrame);
        QWidget *contents=new QWidget(scroll);contents->setObjectName(QStringLiteral("comparisonPage"));
        scroll->setWidget(contents);tabs->addTab(scroll,title);return contents;
    };
    QWidget *overview=page(arabic?QString::fromUtf8("المقارنة"):QStringLiteral("Comparison"));
    QHBoxLayout *compare=new QHBoxLayout(overview);compare->setContentsMargins(0,12,0,0);compare->setSpacing(14);
    QFrame *installedCard=entry.candidates.isEmpty()?0:candidatePanel(entry.candidates[0],demo,entry.sdioDriverPresent,overview,false,arabic);
    if(installedCard)compare->addWidget(installedCard,1,Qt::AlignTop);
    QWidget *candidateHost=new QWidget(overview);candidateHost->setObjectName(QStringLiteral("candidateHost"));candidateHost->setSizePolicy(QSizePolicy::Preferred,QSizePolicy::Maximum);QVBoxLayout *candidateBox=new QVBoxLayout(candidateHost);candidateBox->setContentsMargins(0,0,0,0);candidateBox->setAlignment(Qt::AlignTop);
    compare->addWidget(candidateHost,1,Qt::AlignTop);
    QWidget *hardware=page(arabic?QString::fromUtf8("الجهاز"):QStringLiteral("Device"));
    QGridLayout *hw=new QGridLayout(hardware);hw->setContentsMargins(16,18,16,18);hw->setHorizontalSpacing(18);hw->setVerticalSpacing(14);
    addRow(hw,0,QStringLiteral("Device instance ID"),entry.device.instanceId,hardware);
    addRow(hw,1,QStringLiteral("Hardware ID"),entry.device.hardwareId,hardware);
    addRow(hw,2,QStringLiteral("Windows PnP problem"),entry.device.hasProblem?UiLanguage::text(QStringLiteral("Code %1"),QString::fromUtf8("رمز %1")).arg(entry.device.problemCode):UiLanguage::text(QStringLiteral("None reported"),QString::fromUtf8("لا توجد مشكلات مسجّلة")),hardware);
    hw->setRowStretch(3,1);
    QWidget *technical=page(arabic?QString::fromUtf8("بيانات المحرك"):QStringLiteral("Engine details"));
    QVBoxLayout *technicalBox=new QVBoxLayout(technical);technicalBox->setContentsMargins(0,12,0,0);
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
