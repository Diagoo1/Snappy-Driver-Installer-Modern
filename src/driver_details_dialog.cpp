#include "driver_details_dialog.h"
#include "ui_language.h"
#include "searchable_language_combo.h"
#include "vector_icons.h"
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
    QFrame *card=new QFrame(parent);
    card->setObjectName(QStringLiteral("detailSection"));
    QVBoxLayout *box=new QVBoxLayout(card);
    box->setContentsMargins(19,17,19,18);
    box->setSpacing(11);
    QString heading=d.status==QStringLiteral("installed")?
           (verified?tr("INSTALLED DRIVER","التعريف المثبت"):tr("CURRENT DRIVER","التعريف الحالي")):
           (d.status==QStringLiteral("recommended")?(demo?tr("RECOMMENDED - DEMO","موصى به — توضيحي"):tr("RECOMMENDED DRIVER","التعريف الموصى به")):(demo?tr("ALTERNATIVE - DEMO","بديل — توضيحي"):tr("AVAILABLE DRIVER","تعريف متاح")));
    box->addWidget(addText(heading,"detailHeading",card));
    QGridLayout *fields=new QGridLayout();
    fields->setHorizontalSpacing(17);fields->setVerticalSpacing(11);
    addRow(fields,0,tr("Version","الإصدار"),d.version,card);
    addRow(fields,1,tr("Date","التاريخ"),d.date,card);
    addRow(fields,2,tr("Provider","المزوّد"),d.provider,card);
    addRow(fields,3,QStringLiteral("INF"),d.inf,card);
    if(!technical){
        addRow(fields,4,tr("Availability","التوفر"),d.status==QStringLiteral("installed")?tr("Installed on this computer","مثبت على هذا الجهاز"):
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
    resize(qMin(1000,usable.width()-36),qMin(670,usable.height()-36));
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

    QHBoxLayout *pickerRow=new QHBoxLayout();
    pickerRow->addWidget(addText(arabic?QString::fromUtf8("الإصدار المتاح"):QStringLiteral("Available version"),"mutedText",body));
    QComboBox *picker=new SearchableLanguageCombo(body);picker->setObjectName(QStringLiteral("candidatePicker"));
    for(int i=1;i<entry.candidates.size();++i){
        const DriverCandidate &candidate=entry.candidates[i];
        picker->addItem(candidate.version+QStringLiteral("  ·  ")+candidate.date+
            (candidate.recommended?(arabic?QString::fromUtf8("  ·  موصى به"):QStringLiteral("  ·  Recommended")):QString()),i);
    }
    picker->setEnabled(picker->count()>0);
    if(!picker->count())picker->addItem(arabic?QString::fromUtf8("لا توجد إصدارات متاحة"):QStringLiteral("No available candidates"),-1);
    pickerRow->addWidget(picker,1);content->addLayout(pickerRow);
    QTabWidget *tabs=new QTabWidget(body);tabs->setObjectName(QStringLiteral("comparisonTabs"));
    auto page=[tabs](const QString &title)->QWidget*{
        QScrollArea *scroll=new QScrollArea(tabs);scroll->setObjectName(QStringLiteral("dialogScroll"));
        scroll->setWidgetResizable(true);scroll->setFrameShape(QFrame::NoFrame);
        QWidget *contents=new QWidget(scroll);contents->setStyleSheet(QStringLiteral("background:transparent;"));
        scroll->setWidget(contents);tabs->addTab(scroll,title);return contents;
    };
    QWidget *overview=page(arabic?QString::fromUtf8("المقارنة"):QStringLiteral("Comparison"));
    QHBoxLayout *compare=new QHBoxLayout(overview);compare->setContentsMargins(0,12,0,0);compare->setSpacing(14);
    if(!entry.candidates.isEmpty())compare->addWidget(candidatePanel(entry.candidates[0],demo,entry.sdioDriverPresent,overview,false,arabic),1,Qt::AlignTop);
    QWidget *candidateHost=new QWidget(overview);QVBoxLayout *candidateBox=new QVBoxLayout(candidateHost);candidateBox->setContentsMargins(0,0,0,0);
    compare->addWidget(candidateHost,1,Qt::AlignTop);
    QWidget *hardware=page(arabic?QString::fromUtf8("الجهاز"):QStringLiteral("Device"));
    QGridLayout *hw=new QGridLayout(hardware);hw->setContentsMargins(16,18,16,18);hw->setHorizontalSpacing(18);hw->setVerticalSpacing(14);
    addRow(hw,0,QStringLiteral("Device instance ID"),entry.device.instanceId,hardware);
    addRow(hw,1,QStringLiteral("Hardware ID"),entry.device.hardwareId,hardware);
    addRow(hw,2,QStringLiteral("Windows PnP problem"),entry.device.hasProblem?UiLanguage::text(QStringLiteral("Code %1"),QString::fromUtf8("رمز %1")).arg(entry.device.problemCode):UiLanguage::text(QStringLiteral("None reported"),QString::fromUtf8("لا توجد مشكلات مسجّلة")),hardware);
    hw->setRowStretch(3,1);
    QWidget *technical=page(arabic?QString::fromUtf8("بيانات المحرك"):QStringLiteral("Engine details"));
    QVBoxLayout *technicalBox=new QVBoxLayout(technical);technicalBox->setContentsMargins(0,12,0,0);
    auto update=[entry,demo,arabic,candidateBox,candidateHost,technicalBox,technical,picker](){
        while(QLayoutItem *item=candidateBox->takeAt(0)){delete item->widget();delete item;}
        while(QLayoutItem *item=technicalBox->takeAt(0)){delete item->widget();delete item;}
        const int index=picker->itemData(picker->currentIndex()).toInt();
        if(index>0 && index<entry.candidates.size()){
            candidateBox->addWidget(candidatePanel(entry.candidates[index],demo,entry.sdioDriverPresent,candidateHost,false,arabic));
            technicalBox->addWidget(candidatePanel(entry.candidates[index],demo,entry.sdioDriverPresent,technical,true,arabic));
        }
        technicalBox->addStretch(1);
    };
    connect(picker,static_cast<void(QComboBox::*)(int)>(&QComboBox::currentIndexChanged),this,[update](int){update();});
    update();
    content->addWidget(tabs,1);
    main->addWidget(body,1);
}
