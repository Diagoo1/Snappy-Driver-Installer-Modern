#include "installation_workspace.h"
#include "vector_icons.h"
#include "ui_language.h"
#include "appearance.h"
#include "theme.h"
#include "ui_font.h"
#include <QPainterPath>
#include <QResizeEvent>
#include <QListWidget>

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGridLayout>
#include <QProgressBar>
#include <QLabel>
#include <QPushButton>
#include <QTreeWidget>
#include <QHeaderView>
#include <QTimer>
#include <QFrame>
#include <QFont>
#include <QStyle>
#include <QAbstractItemView>
#include <QStyledItemDelegate>
#include <QPainter>

// Draw the band once, with the same rounded gradient and icon hierarchy as the driver table.
class InstallationHeader : public QHeaderView {
public:explicit InstallationHeader(QWidget *parent):QHeaderView(Qt::Horizontal,parent){setFixedHeight(60);viewport()->setAutoFillBackground(false);}
protected:void paintEvent(QPaintEvent*) Q_DECL_OVERRIDE {
    QPainter p(viewport());p.setRenderHint(QPainter::Antialiasing);const bool dark=Theme::isDark(this),rtl=layoutDirection()==Qt::RightToLeft;
    p.fillRect(viewport()->rect(),Theme::color("surface",dark));const QRectF band=QRectF(viewport()->rect()).adjusted(.5,3.5,-.5,-3.5);QLinearGradient gradient(band.topLeft(),band.topRight());gradient.setColorAt(0,Theme::color("headerStart",dark));gradient.setColorAt(1,Theme::color("headerEnd",dark));p.setPen(Qt::NoPen);p.setBrush(gradient);p.drawRoundedRect(band,12,12);
    QPainterPath clip;clip.addRoundedRect(band,12,12);p.setClipPath(clip);QFont face(UiFont::family());face.setPixelSize(14);face.setWeight(QFont::DemiBold);p.setFont(face);const char *symbols[]={"chip","check-circle","clock"};
    for(int section=0;section<count();++section){const QRect area(sectionViewportPosition(section),4,sectionSize(section),height()-8);if(section>0){const int x=rtl?area.right():area.left();p.setPen(QPen(QColor(255,255,255,35),1));p.drawLine(QPointF(x,band.top()+12),QPointF(x,band.bottom()-12));}const QString label=p.fontMetrics().elidedText(model()->headerData(section,Qt::Horizontal).toString(),Qt::ElideRight,qMax(0,area.width()-44));const int textWidth=p.fontMetrics().width(label),total=textWidth+26;const int left=section==0?(rtl?area.right()-total-14:area.left()+14):area.center().x()-total/2;VectorIcons::icon(QString::fromLatin1(symbols[section%3]),QColor("#B6F3ED"),20).paint(&p,QRect(rtl?left+total-20:left,area.center().y()-10,20,20));p.setPen(Qt::white);p.drawText(QRect(rtl?left:left+26,area.top(),textWidth,area.height()),Qt::AlignAbsolute|(rtl?Qt::AlignRight:Qt::AlignLeft)|Qt::AlignVCenter,label);}
}
};
class InstallationTree : public QTreeWidget {
public:explicit InstallationTree(QWidget *parent):QTreeWidget(parent){setHeader(new InstallationHeader(this));}
protected:void drawRow(QPainter *p,const QStyleOptionViewItem &option,const QModelIndex &index)const Q_DECL_OVERRIDE {QTreeWidget::drawRow(p,option,index);p->save();p->setPen(QPen(Theme::color("border",Theme::isDark(this)),1));p->drawLine(QPointF(8,option.rect.bottom()+.5),QPointF(viewport()->width()-8,option.rect.bottom()+.5));p->restore();}
    void resizeEvent(QResizeEvent *event) Q_DECL_OVERRIDE {QTreeWidget::resizeEvent(event);header()->resizeSection(1,qBound(140,viewport()->width()/4,260));header()->resizeSection(2,110);}
};
static QString elapsedText(qint64 milliseconds){const qint64 seconds=qMax(qint64(0),milliseconds)/1000;return QStringLiteral("%1:%2:%3").arg(seconds/3600,2,10,QLatin1Char('0')).arg(seconds/60%60,2,10,QLatin1Char('0')).arg(seconds%60,2,10,QLatin1Char('0'));}

class InstallationStatusDelegate : public QStyledItemDelegate {
public:explicit InstallationStatusDelegate(QObject *parent):QStyledItemDelegate(parent){}
    void paint(QPainter *p,const QStyleOptionViewItem &option,const QModelIndex &index)const Q_DECL_OVERRIDE {
        if(index.column()!=1){QStyledItemDelegate::paint(p,option,index);return;}
        const int tone=index.data(Qt::UserRole+3).toInt();const QColor ink=tone==2?QColor("#00866F"):tone==3?QColor("#C34462"):tone==1?QColor("#1477AA"):QColor("#987022");
        const bool dark=Theme::isDark(qobject_cast<QWidget*>(parent()));QColor fill=ink;fill.setAlpha(dark?55:20);QColor text=dark?ink.lighter(190):ink;
        p->save();p->setRenderHint(QPainter::Antialiasing);const QRect area=option.rect.adjusted(8,10,-8,-10);p->setPen(Qt::NoPen);p->setBrush(fill);p->drawRoundedRect(area,12,12);p->setPen(text);p->drawText(area.adjusted(7,0,-7,0),Qt::AlignCenter,option.fontMetrics.elidedText(index.data().toString(),Qt::ElideRight,area.width()-14));p->restore();
    }
};
static void queueStatus(QTreeWidgetItem *row,const QString &text){
    row->setText(1,text);row->setData(1,Qt::UserRole+4,text);const bool failed=text.startsWith(QStringLiteral("Failed"));const bool success=text==QStringLiteral("Reinstalled") || text==QStringLiteral("Installed") || text.startsWith(QStringLiteral("Installed -")) || text.startsWith(QStringLiteral("Completed"));
    const bool waiting=text==QStringLiteral("Pending") || text==QStringLiteral("Queued") || text==QStringLiteral("Not started") || text==QStringLiteral("Not installed");
    row->setData(1,Qt::UserRole+3,failed?3:success?2:waiting?0:1);
}

bool confirmDriverInstallation(QWidget *parent,const QStringList &drivers,bool restore,bool alternative,bool reboot,bool administrator,bool arabic,bool reinstall){
    auto t=[](const char *en,const char *ar){return UiLanguage::text(QString::fromUtf8(en),QString::fromUtf8(ar));};
    RoundedDialog dialog(parent);dialog.setObjectName(QStringLiteral("installationConsent"));dialog.setLayoutDirection(arabic?Qt::RightToLeft:Qt::LeftToRight);dialog.resize(620,520);
    QVBoxLayout *root=new QVBoxLayout(&dialog);root->setContentsMargins(0,0,0,18);root->addWidget(new DialogHeader(t("Confirm driver installation","تأكيد تثبيت التعريفات"),QStringLiteral("shield"),&dialog,arabic));
    QVBoxLayout *body=new QVBoxLayout;body->setContentsMargins(24,12,24,0);body->setSpacing(14);
    AccentCard *hero=new AccentCard(&dialog);QVBoxLayout *heroBox=new QVBoxLayout(hero);heroBox->setContentsMargins(20,18,20,18);
    QLabel *title=new QLabel(t("Review before installing","راجع اختياراتك قبل التثبيت"),hero);title->setStyleSheet(QStringLiteral("color:white;background:transparent;font-size:21px;font-weight:600;"));heroBox->addWidget(title);
    QLabel *subtitle=new QLabel(administrator?t("Only the selected drivers will be installed on this computer.","سيتم تثبيت التعريفات المحددة فقط على هذا الجهاز."):t("Administrator access is required. Reopen, scan and confirm again; installation will not start automatically.","يلزم فتح البرنامج كمسؤول، ثم الفحص والتأكيد من جديد. لن يبدأ التثبيت تلقائيًا."),hero);subtitle->setWordWrap(true);subtitle->setStyleSheet(QStringLiteral("color:#D4F1F0;background:transparent;"));heroBox->addWidget(subtitle);body->addWidget(hero);
    QHBoxLayout *tags=new QHBoxLayout;
    auto tag=[&](const QString &value){QLabel *label=new QLabel(value,&dialog);label->setWordWrap(true);label->setObjectName(QStringLiteral("installOptionTag"));tags->addWidget(label);};
    tag(t("Drivers: %1","التعريفات: %1").arg(drivers.size()));tag(restore?t("Restore point required","نقطة استعادة مطلوبة"):t("Restore point off","نقطة الاستعادة معطلة"));tag(reboot?t("Restart countdown enabled","عدّاد إعادة التشغيل مفعّل"):t("Manual restart","إعادة تشغيل يدوية"));body->addLayout(tags);
    QListWidget *list=new QListWidget(&dialog);list->setObjectName(QStringLiteral("installConsentList"));list->setSelectionMode(QAbstractItemView::NoSelection);list->addItems(drivers);for(int i=0;i<list->count();++i)list->item(i)->setSizeHint(QSize(0,48));body->addWidget(list,1);
    const QString warning=reinstall?t("Reinstallation selected: the matching local package will be installed again. Review the listed versions before continuing.","تم اختيار إعادة التثبيت: سيتم تثبيت الحزمة المحلية المطابقة من جديد. راجع الإصدارات قبل المتابعة."):alternative?t("Beta: alternative drivers selected. Review the versions carefully.","بيتا: تم اختيار تعريفات بديلة. راجع الإصدارات بعناية."):t("Beta: installing drivers changes Windows. Review your selection before continuing.","بيتا: تثبيت التعريفات يغيّر ويندوز. راجع اختياراتك قبل المتابعة.");
    QLabel *notice=new QLabel(warning+QLatin1Char(' ')+(restore?t("Restore-point failure stops the queue.","فشل نقطة الاستعادة يوقف التثبيت."):t("Installation will proceed without a restore point.","سيتم التثبيت دون إنشاء نقطة استعادة.")),&dialog);notice->setObjectName(QStringLiteral("installNotice"));notice->setWordWrap(true);body->addWidget(notice);
    QHBoxLayout *actions=new QHBoxLayout;actions->addStretch();ActionButton *cancel=new ActionButton(t("Cancel","إلغاء"),&dialog);cancel->setObjectName(QStringLiteral("installationCancel"));ActionButton *proceed=new ActionButton(administrator?t("Install selected drivers","تثبيت التعريفات المحددة"):t("Restart as administrator","إعادة الفتح كمسؤول"),&dialog);proceed->setObjectName(QStringLiteral("primaryButton"));actions->addWidget(cancel);actions->addWidget(proceed);body->addLayout(actions);root->addLayout(body,1);
    QObject::connect(cancel,&QPushButton::clicked,&dialog,&QDialog::reject);QObject::connect(proceed,&QPushButton::clicked,&dialog,&QDialog::accept);return dialog.exec()==QDialog::Accepted;
}

static QLabel *textLabel(const QString &s,const char *name,QWidget *parent)
{
    QLabel *v=new QLabel(s,parent);
    v->setObjectName(QString::fromLatin1(name));
    v->setWordWrap(true);
    return v;
}
InstallationWorkspace::InstallationWorkspace(QWidget *parent)
    : QWidget(parent),m_arabic(false),m_active(false),m_reboot(false),m_hasRestore(false),m_live(false),m_percent(0),m_currentStage(-1),m_timer(new QTimer(this))
{
    m_duration=0;m_clockRunning=false;m_queue=0;m_clockTimer=new QTimer(this);m_clockTimer->setInterval(1000);connect(m_clockTimer,&QTimer::timeout,this,&InstallationWorkspace::updateTimes);
    setObjectName(QStringLiteral("page"));
    QVBoxLayout *outer=new QVBoxLayout(this);
    outer->setContentsMargins(32,28,30,22);
    outer->setSpacing(15);

    QHBoxLayout *top=new QHBoxLayout();
    QVBoxLayout *titles=new QVBoxLayout();titles->setSpacing(4);
    m_title=textLabel(QStringLiteral("Installation progress"),"pageTitle",this);
    m_sub=textLabel(QStringLiteral("One-screen operation center"),"subText",this);
    titles->addWidget(m_title);titles->addWidget(m_sub);
    top->addLayout(titles,1);
    m_badge=textLabel(QStringLiteral("INSTALLATION LOCKED IN SCAN BUILD"),"simulationBadge",this);
    m_badge->setWordWrap(false);
    m_badge->hide();
    top->addWidget(m_badge);
    outer->addLayout(top);

    QWidget *mainProgress=new AccentCard(this);
    mainProgress->setObjectName(QStringLiteral("installationHero"));
    QVBoxLayout *mp=new QVBoxLayout(mainProgress);
    mp->setContentsMargins(23,18,23,18);mp->setSpacing(10);
    m_stage=textLabel(QStringLiteral("Ready for native installation when validated"),"sectionTitle",mainProgress);
    m_stage->setStyleSheet(QStringLiteral("color:white;background:transparent;font-size:20px;font-weight:600;"));
    mp->addWidget(m_stage);
    m_progress=new QProgressBar(mainProgress);
    m_progress->setObjectName(QStringLiteral("mainProgress"));
    m_progress->setRange(0,100);m_progress->setValue(0);
    m_progress->setTextVisible(true);m_progress->setFormat(QStringLiteral("%p%"));
    m_progress->setMinimumHeight(21);
    mp->addWidget(m_progress);
    m_summary=textLabel(QStringLiteral("No tasks scheduled"),"subText",mainProgress);
    m_summary->setStyleSheet(QStringLiteral("color:#D4F1F0;background:transparent;font-size:13px;"));
    QHBoxLayout *progressFooter=new QHBoxLayout;progressFooter->addWidget(m_summary,1);
    m_elapsed=textLabel(QString(),"installationElapsed",mainProgress);m_elapsed->setWordWrap(false);m_elapsed->setStyleSheet(QStringLiteral("color:#D4F1F0;background:transparent;font-size:13px;font-weight:500;"));progressFooter->addWidget(m_elapsed);mp->addLayout(progressFooter);updateTimes();
    outer->addWidget(mainProgress);

    QHBoxLayout *center=new QHBoxLayout();center->setSpacing(15);
    QWidget *milestones=new SurfaceCard(this);
    milestones->setObjectName(QStringLiteral("softSurface"));
    milestones->setMinimumWidth(218);
    milestones->setMaximumWidth(300);
    QVBoxLayout *steps=new QVBoxLayout(milestones);
    steps->setContentsMargins(18,19,18,19);steps->setSpacing(16);
    steps->addWidget(textLabel(QStringLiteral("PROCESS STAGES"),"detailHeading",milestones));
    m_stepRestore=textLabel(QStringLiteral("01  Restore point"),"stageItem",milestones);
    m_stepPrep=textLabel(QStringLiteral("02  Preparing"),"stageItem",milestones);
    m_stepInstall=textLabel(QStringLiteral("03  Installing drivers"),"stageItem",milestones);
    m_stepFinish=textLabel(QStringLiteral("04  Finalizing"),"stageItem",milestones);
    steps->addWidget(m_stepRestore);steps->addWidget(m_stepPrep);
    steps->addWidget(m_stepInstall);steps->addWidget(m_stepFinish);
    steps->addStretch(1);
    m_tip=textLabel(QStringLiteral("Matching-only builds cannot modify Windows drivers. Real installation requires a separately validated installation build."),"mutedText",milestones);
    steps->addWidget(m_tip);
    center->addWidget(milestones);

    QWidget *operations=new SurfaceCard(this);
    operations->setObjectName(QStringLiteral("softSurface"));
    QVBoxLayout *olist=new QVBoxLayout(operations);
    olist->setContentsMargins(18,18,18,15);olist->setSpacing(9);
    olist->addWidget(textLabel(QStringLiteral("INSTALLATION ITEMS"),"detailHeading",operations));
    m_queue=new InstallationTree(operations);
    m_queue->setObjectName(QStringLiteral("installQueue"));
    m_queue->setItemDelegate(new InstallationStatusDelegate(m_queue));
    m_queue->setColumnCount(3);
    m_queue->setHeaderLabels(QStringList()<<QStringLiteral("Drivers")<<QStringLiteral("Status")<<UiLanguage::text(QStringLiteral("Elapsed"),QString::fromUtf8("المنقضي")));
    m_queue->setRootIsDecorated(false);
    m_queue->setIndentation(0);
    m_queue->setUniformRowHeights(true);
    m_queue->setAlternatingRowColors(false);
    m_queue->setSelectionMode(QAbstractItemView::NoSelection);
    m_queue->setFocusPolicy(Qt::NoFocus);
    m_queue->header()->setSectionResizeMode(0,QHeaderView::Stretch);
    m_queue->header()->setSectionResizeMode(1,QHeaderView::Fixed);
    m_queue->header()->resizeSection(1,220);
    m_queue->header()->setSectionResizeMode(2,QHeaderView::Fixed);m_queue->header()->resizeSection(2,110);
    m_queue->setVerticalScrollMode(QAbstractItemView::ScrollPerPixel);
    m_queue->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    olist->addWidget(m_queue,1);
    center->addWidget(operations,1);
    outer->addLayout(center,1);

    QHBoxLayout *bottom=new QHBoxLayout();
    m_complete=textLabel(QStringLiteral("No changes are made to Windows."),"demoText",this);
    m_complete->setObjectName(QStringLiteral("installNotice"));
    bottom->addWidget(m_complete,1);
    m_stop=new QPushButton(QStringLiteral("Stop demo"),this);
    m_stop->setObjectName(QStringLiteral("outlineButton"));
    m_stop->setVisible(false);
    bottom->addWidget(m_stop);
    m_restoreHelp=new ActionButton(UiLanguage::text(QStringLiteral("System Protection"),QString::fromUtf8("حماية النظام")),this);m_restoreHelp->setObjectName(QStringLiteral("restoreHelp"));m_restoreHelp->hide();bottom->addWidget(m_restoreHelp);connect(m_restoreHelp,&QPushButton::clicked,this,&InstallationWorkspace::systemProtectionRequested);
    m_back=new QPushButton(QStringLiteral("Back to drivers"),this);
    m_back->setObjectName(QStringLiteral("primaryButton"));
    bottom->addWidget(m_back);
    outer->addLayout(bottom);
    connect(m_back,&QPushButton::clicked,this,&InstallationWorkspace::backRequested);
    connect(m_stop,&QPushButton::clicked,this,[this](){
        if(m_live){m_stop->setEnabled(false);m_stop->setText(QStringLiteral("Waiting for current driver..."));emit stopRequested();}
        else stopDemo();
    });
    m_timer->setInterval(85);
    connect(m_timer,&QTimer::timeout,this,&InstallationWorkspace::step);
}

void InstallationWorkspace::beginDemo(const QStringList &drivers,bool restorePoint,bool rebootAfter)
{
    if(m_active || drivers.isEmpty()) return;
    m_live=false;m_items=drivers;
    m_back->setText(UiLanguage::text(QStringLiteral("Back to drivers"),QString::fromUtf8("العودة للتعريفات")));m_complete->setToolTip(QString());
    m_restoreHelp->hide();
    m_complete->setProperty("tone",QString());m_complete->style()->unpolish(m_complete);m_complete->style()->polish(m_complete);
    m_progress->setRange(0,100);m_progress->setFormat(QStringLiteral("%p%"));
    m_badge->setText(QStringLiteral("SIMULATION ONLY"));
    m_stop->setText(QStringLiteral("Stop demo"));m_stop->setEnabled(true);
    m_hasRestore=restorePoint;
    if(restorePoint) m_items.prepend(QStringLiteral("Restore point (simulated)"));
    m_reboot=rebootAfter;
    m_queue->clear();
    for(int i=0;i<m_items.size();++i){
        QTreeWidgetItem *item=new QTreeWidgetItem(m_queue);
        item->setText(0,m_items.at(i));
        queueStatus(item,QStringLiteral("Queued"));
        item->setIcon(0,VectorIcons::icon(i==0 && restorePoint?QStringLiteral("restore"):QStringLiteral("chip"),QColor("#16A9AD"),18));
        item->setSizeHint(0,QSize(0,51));
    }
    m_percent=0;
    m_currentStage=-1;
    m_active=true;
    m_back->setEnabled(false);
    m_stop->setVisible(true);
    m_stage->setText(QStringLiteral("Simulating installation flow..."));
    m_complete->setText(QStringLiteral("DEMO: no restore point, install, download or reboot is performed."));
    startTiming();updateProgress();
    emit activeChanged(true);
    m_timer->start();
}
void InstallationWorkspace::step()
{
    if(!m_active || m_live) return;
    m_percent=qMin(100,m_percent+1);
    updateProgress();
    if(m_percent>=100){
        m_timer->stop();stopTiming();
        m_active=false;
        m_back->setEnabled(true);
        m_stop->setVisible(false);
        m_stage->setText(QStringLiteral("Preview complete — no system changes"));
        m_complete->setText(m_reboot?QStringLiteral("DEMO COMPLETE · A real install would now request reboot confirmation."):
                                      QStringLiteral("DEMO COMPLETE · No Windows drivers were changed."));
        emit activeChanged(false);
        emit simulationFinished(m_reboot);
    }
}
void InstallationWorkspace::updateProgress()
{
    m_progress->setValue(m_percent);
    const int total=m_items.size();
    int completed=0;
    for(int i=0;i<total;++i){
        QTreeWidgetItem *row=m_queue->topLevelItem(i);
        const int start=i*100/total;
        const int finish=(i+1)*100/total;
        const QString state=m_percent>=finish?QStringLiteral("Completed (demo)"):
           (m_percent>=start && m_percent<finish?QStringLiteral("Simulating %1%").arg(
                 qBound(0,(m_percent-start)*100/qMax(1,finish-start),99)):QStringLiteral("Queued"));
        timeRow(row,m_percent>=start && m_percent<finish);queueStatus(row,state);
        if(m_percent>=finish) {++completed;row->setForeground(1,QColor("#31B273"));}
        else if(m_percent>=start)row->setForeground(1,QColor("#159FAC"));
    }
    m_summary->setText(UiLanguage::text(QStringLiteral("%1 / %2 items completed · 0 real driver operations"))
        .arg(completed).arg(total));
    const int stage=m_percent<18 && m_hasRestore?0:(m_percent<30?1:(m_percent<95?2:3));
    if(stage!=m_currentStage){
        m_currentStage=stage;
        const QString stageText=stage==0?QStringLiteral("Creating restore point — SIMULATION"):
                 (stage==1?QStringLiteral("Preparing installation — SIMULATION"):
                  (stage==2?QStringLiteral("Installing queued drivers — SIMULATION"):
                            QStringLiteral("Finalizing — SIMULATION")));
        m_stage->setText(stageText);
        m_stepRestore->setText(m_hasRestore?QStringLiteral("01  Restore point"):
                             QStringLiteral("01  Restore point (skipped)"));
        QLabel *labels[]={m_stepRestore,m_stepPrep,m_stepInstall,m_stepFinish};
        for(int i=0;i<4;++i){
            labels[i]->setObjectName(i==stage?QStringLiteral("activeStage"):QStringLiteral("stageItem"));
            labels[i]->style()->unpolish(labels[i]);
            labels[i]->style()->polish(labels[i]);
        }
    }
}
void InstallationWorkspace::stopDemo()
{
    if(!m_active || m_live)return;
    m_timer->stop();stopTiming();m_active=false;
    m_back->setEnabled(true);m_stop->setVisible(false);
    m_stage->setText(QStringLiteral("Simulation stopped"));
    m_complete->setText(QStringLiteral("No driver changes were performed."));
    emit activeChanged(false);
}
void InstallationWorkspace::setArabic(bool arabic)
{
    m_arabic=arabic;
    m_queue->setHeaderLabels(QStringList()<<UiLanguage::text(QStringLiteral("Drivers"))<<UiLanguage::text(QStringLiteral("Status"))<<UiLanguage::text(QStringLiteral("Elapsed"),QString::fromUtf8("المنقضي")));
    updateTimes();
    m_title->setText(arabic?QString::fromUtf8("متابعة التثبيت"):QStringLiteral("Installation progress"));
    m_sub->setText(arabic?QString::fromUtf8("مراحل العملية والعناصر في شاشة واحدة"):QStringLiteral("One-screen operation center"));
    m_back->setText(arabic?QString::fromUtf8("العودة للتعريفات"):QStringLiteral("Back to drivers"));
    if(m_live){m_stop->setText(arabic?QString::fromUtf8("التوقف بعد التعريف الحالي"):QStringLiteral("Stop after current"));return;}
    m_stop->setText(arabic?QString::fromUtf8("إيقاف المحاكاة"):QStringLiteral("Stop demo"));
    if(!m_active){
        // setAvailability is called again by the window after translating.
        if(m_badge->text().contains(QStringLiteral("MATCHING")))
            m_badge->setText(arabic?QString::fromUtf8("فحص ومطابقة فقط"):QStringLiteral("MATCHING-ONLY BUILD"));
    }
}


void InstallationWorkspace::beginNative(const QStringList &names,const QStringList &ids,bool restore)
{
    if(m_active || names.isEmpty() || names.size()!=ids.size())return;
    m_timer->stop();m_live=true;m_active=true;m_hasRestore=restore;m_queue->clear();
    m_back->setText(UiLanguage::text(QStringLiteral("Back to drivers"),QString::fromUtf8("العودة للتعريفات")));m_complete->setToolTip(QString());
    m_restoreHelp->hide();
    m_complete->setProperty("tone",QString());m_complete->style()->unpolish(m_complete);m_complete->style()->polish(m_complete);
    m_badge->setText(UiLanguage::text(QStringLiteral("Live installation · Beta"),QString::fromUtf8("تثبيت فعلي · بيتا")));
    m_tip->setText(QStringLiteral("The current Windows driver operation is not interruptible. Stop cancels the remaining queue after the active driver returns."));
    for(int i=0;i<names.size();++i){
        QTreeWidgetItem *row=new QTreeWidgetItem(m_queue);row->setText(0,names.at(i));queueStatus(row,QStringLiteral("Pending"));
        row->setData(0,Qt::UserRole,ids.at(i));row->setData(0,Qt::UserRole+10,names.at(i).startsWith(UiLanguage::text(QStringLiteral("Reinstall"),QString::fromUtf8("إعادة تثبيت"))+QStringLiteral(" · ")));row->setSizeHint(0,QSize(0,51));
        row->setIcon(0,VectorIcons::icon(QStringLiteral("chip"),QColor("#16A9AD"),18));
    }
    m_stepRestore->setText(restore?QStringLiteral("01  Restore point"):QStringLiteral("01  Restore point (skipped)"));
    m_stepPrep->setText(QStringLiteral("02  Validation & extraction"));m_stepInstall->setText(QStringLiteral("03  Installing drivers"));m_stepFinish->setText(QStringLiteral("04  Results"));
    m_currentStage=-1;
    m_items=names;m_back->setEnabled(false);m_stop->setVisible(true);m_stop->setEnabled(true);m_stop->setText(QStringLiteral("Stop after current"));
    m_progress->setRange(0,names.size());m_progress->setValue(0);m_progress->setFormat(QChar(0x202a)+QStringLiteral("%v / %m")+QChar(0x202c));
    m_stage->setText(QStringLiteral("Validating selected candidates and local packages"));
    m_summary->setText(QStringLiteral("0 / %1 drivers completed").arg(names.size()));
    m_complete->setText(QStringLiteral("Results and errors below come from the native installer, not a simulation."));startTiming();emit activeChanged(true);
}
void InstallationWorkspace::applyNativeEvent(const sdiomodern::Event &e)
{
    if(!m_live || !m_active)return;
    m_stage->setText(QString::fromStdWString(e.message));
    const int stage=e.stage==sdiomodern::Stage::RestorePoint?0:(e.stage==sdiomodern::Stage::Install?2:(e.stage==sdiomodern::Stage::Finished?3:1));
    m_currentStage=stage;QLabel *steps[]={m_stepRestore,m_stepPrep,m_stepInstall,m_stepFinish};
    for(int i=0;i<4;++i){steps[i]->setObjectName(i==stage?QStringLiteral("activeStage"):QStringLiteral("stageItem"));steps[i]->style()->unpolish(steps[i]);steps[i]->style()->polish(steps[i]);}
    const int total=static_cast<int>(e.total);
    if(total>0){m_progress->setRange(0,total);m_progress->setValue(static_cast<int>(e.done));}
    const QString id=QString::fromStdWString(e.deviceId);
    QString status;
    switch(e.outcome){
    case sdiomodern::Outcome::Success:status=QStringLiteral("Installed");break;
    case sdiomodern::Outcome::Failure:status=QStringLiteral("Failed (%1)").arg(e.nativeError);break;
    case sdiomodern::Outcome::Skipped:status=QStringLiteral("Skipped");break;
    case sdiomodern::Outcome::Cancelled:status=QStringLiteral("Cancelled");break;
    case sdiomodern::Outcome::Pending:status=QStringLiteral("Pending");break;
    default:status=e.stage==sdiomodern::Stage::Extract?QStringLiteral("Extracting"):QStringLiteral("Working...");break;
    }
    for(int i=0;i<m_queue->topLevelItemCount();++i){QTreeWidgetItem *row=m_queue->topLevelItem(i);
        if(!id.isEmpty() && row->data(0,Qt::UserRole).toString().compare(id,Qt::CaseInsensitive)==0){if(row->data(0,Qt::UserRole+10).toBool() && e.stage==sdiomodern::Stage::Install){if(e.outcome==sdiomodern::Outcome::Success)status=QStringLiteral("Reinstalled");else if(e.outcome==sdiomodern::Outcome::Running)status=QStringLiteral("Reinstalling");}if(e.stage==sdiomodern::Stage::Extract || e.stage==sdiomodern::Stage::Install)timeRow(row,e.outcome==sdiomodern::Outcome::Running);queueStatus(row,status);row->setToolTip(1,QString::fromStdWString(e.message));}}
    m_summary->setText(QStringLiteral("%1 / %2 drivers processed").arg(static_cast<qulonglong>(e.done)).arg(static_cast<qulonglong>(e.total)));
}
void InstallationWorkspace::finishNative(const sdiomodern::InstallReport &report)
{
    auto t=[](const char *en,const char *ar){return UiLanguage::text(QString::fromUtf8(en),QString::fromUtf8(ar));};
    if(!m_live)return;int ok=0,failed=0,skipped=0;
    for(std::size_t i=0;i<report.items.size();++i){const sdiomodern::Result &result=report.items[i];
        QString status;
        if(result.outcome==sdiomodern::Outcome::Success){++ok;status=result.rebootRequired?QStringLiteral("Installed - restart"):QStringLiteral("Installed");}
        else if(result.outcome==sdiomodern::Outcome::Failure){++failed;status=QStringLiteral("Failed (%1)").arg(result.nativeError);}
        else{++skipped;status=QStringLiteral("Not installed");}
        for(int j=0;j<m_queue->topLevelItemCount();++j){QTreeWidgetItem *row=m_queue->topLevelItem(j);
            if(row->data(0,Qt::UserRole).toString().compare(QString::fromStdWString(result.deviceId),Qt::CaseInsensitive)==0){if(row->data(0,Qt::UserRole+10).toBool() && result.outcome==sdiomodern::Outcome::Success)status=QStringLiteral("Reinstalled");queueStatus(row,status);row->setToolTip(1,QString::fromStdWString(result.message));}}
    }
    for(int j=0;j<m_queue->topLevelItemCount();++j){QTreeWidgetItem *row=m_queue->topLevelItem(j);
        if(row->data(1,Qt::UserRole+4).toString()==QStringLiteral("Pending")){queueStatus(row,QStringLiteral("Not started"));++skipped;}}
    stopTiming();m_active=false;m_live=false;m_stop->setVisible(false);m_back->setEnabled(true);
    const int finalStage=report.restoreFailed && !report.error.empty()?0:3;
    QLabel *steps[]={m_stepRestore,m_stepPrep,m_stepInstall,m_stepFinish};for(int i=0;i<4;++i){steps[i]->setObjectName(i==finalStage?QStringLiteral("activeStage"):QStringLiteral("stageItem"));steps[i]->style()->unpolish(steps[i]);steps[i]->style()->polish(steps[i]);}
    m_stage->setText(report.cancelled?QStringLiteral("Stopped safely after the active operation"):
      (failed || !report.error.empty()?QStringLiteral("Completed with errors"):QStringLiteral("Native installation finished")));
    m_summary->setText(QStringLiteral("Installed: %1 | Failed: %2 | Not installed: %3").arg(ok).arg(failed).arg(skipped));
    m_complete->setText(!report.error.empty()?QString::fromStdWString(report.error):
        (report.rebootRequired?QStringLiteral("Windows restart is required. Restart manually after saving your work."):
         QStringLiteral("Scan again to verify the current Windows driver state.")));
    if(report.restoreFailed && !report.restoreCreated && !report.error.empty()){
        m_complete->setToolTip(QString::fromStdWString(report.error));m_back->setText(t("Review install options","مراجعة خيارات التثبيت"));
        if(report.restoreError==1058)m_complete->setText(t("System Protection is disabled (Windows 1058). Enable it for the Windows drive, or review your restore-point option before retrying.","حماية النظام معطّلة (Windows 1058). فعّلها لقرص ويندوز، أو راجع خيار نقطة الاستعادة قبل المحاولة مجددًا."));
        m_stage->setText(UiLanguage::text(QStringLiteral("Installation blocked: restore point"),QString::fromUtf8("التثبيت متوقف: نقطة الاستعادة")));m_restoreHelp->show();m_complete->setProperty("tone",QStringLiteral("error"));m_complete->style()->unpolish(m_complete);m_complete->style()->polish(m_complete);m_progress->setValue(0);
    }else m_progress->setValue(ok+failed+skipped);emit activeChanged(false);
}
void InstallationWorkspace::failNative(const QString &reason)
{
    if(!m_live)return;stopTiming();m_active=false;m_live=false;m_stop->setVisible(false);m_back->setEnabled(true);
    m_stage->setText(UiLanguage::text(QStringLiteral("Installation needs attention"),QString::fromUtf8("التثبيت يحتاج مراجعة")));m_complete->setText(reason);m_complete->setProperty("tone",QStringLiteral("error"));m_complete->style()->unpolish(m_complete);m_complete->style()->polish(m_complete);
    for(int i=0;i<m_queue->topLevelItemCount();++i){QTreeWidgetItem *row=m_queue->topLevelItem(i);
        if(row->data(1,Qt::UserRole+4).toString()==QStringLiteral("Pending"))queueStatus(row,QStringLiteral("Not started"));}
    emit activeChanged(false);
}

void InstallationWorkspace::setAvailability(bool canInstall,bool demo)
{
    if(m_active)return;
    if(demo){
        m_badge->setText(QStringLiteral("EXPLICIT DEMO"));
        m_tip->setText(QStringLiteral("Demonstration mode was explicitly requested. No Windows changes."));
        return;
    }
    if(canInstall){
        m_badge->setText(QStringLiteral("NATIVE INSTALL - TEST ONLY"));
        m_stage->setText(QStringLiteral("No installation queued"));
        m_tip->setText(QStringLiteral("Real Windows installer is enabled ONLY in the opt-in install-test build. Use an isolated machine."));
    } else {
        m_badge->setText(QStringLiteral("MATCHING-ONLY BUILD"));
        m_stage->setText(QStringLiteral("Native installation not enabled"));
        m_tip->setText(QStringLiteral("This matching build cannot install drivers. Use a separate isolated installation-test build."));
    }
    m_complete->setText(QStringLiteral("No changes have been made to Windows."));
}

void InstallationWorkspace::startTiming(){m_clock.restart();m_duration=0;m_clockRunning=true;for(int i=0;i<m_queue->topLevelItemCount();++i){QTreeWidgetItem *row=m_queue->topLevelItem(i);row->setData(0,Qt::UserRole+11,QVariant());row->setData(0,Qt::UserRole+12,qint64(0));row->setText(2,QString::fromUtf8("—"));row->setTextAlignment(2,Qt::AlignCenter);row->setToolTip(2,UiLanguage::text(QStringLiteral("Measured processing time; waiting in the queue is excluded."),QString::fromUtf8("وقت المعالجة المقاس؛ لا يشمل الانتظار في القائمة.")));}updateTimes();m_clockTimer->start();}
void InstallationWorkspace::timeRow(QTreeWidgetItem *row,bool running){if(!m_clockRunning)return;const QVariant started=row->data(0,Qt::UserRole+11);if(running && !started.isValid())row->setData(0,Qt::UserRole+11,m_clock.elapsed());else if(!running && started.isValid()){row->setData(0,Qt::UserRole+12,row->data(0,Qt::UserRole+12).toLongLong()+m_clock.elapsed()-started.toLongLong());row->setData(0,Qt::UserRole+11,QVariant());row->setText(2,elapsedText(row->data(0,Qt::UserRole+12).toLongLong()));}if(running)row->setText(2,elapsedText(row->data(0,Qt::UserRole+12).toLongLong()+m_clock.elapsed()-row->data(0,Qt::UserRole+11).toLongLong()));}
void InstallationWorkspace::updateTimes(){if(m_clockRunning)m_duration=m_clock.elapsed();m_elapsed->setText(UiLanguage::text(QStringLiteral("Total elapsed: %1"),QString::fromUtf8("الوقت الإجمالي: %1")).arg(QChar(0x202a)+elapsedText(m_duration)+QChar(0x202c)));for(int i=0;i<(m_queue?m_queue->topLevelItemCount():0);++i){QTreeWidgetItem *row=m_queue->topLevelItem(i);if(m_clockRunning && row->data(0,Qt::UserRole+11).isValid())row->setText(2,elapsedText(row->data(0,Qt::UserRole+12).toLongLong()+m_clock.elapsed()-row->data(0,Qt::UserRole+11).toLongLong()));}}
void InstallationWorkspace::stopTiming(){if(!m_clockRunning)return;for(int i=0;i<m_queue->topLevelItemCount();++i)timeRow(m_queue->topLevelItem(i),false);m_duration=m_clock.elapsed();m_clockRunning=false;m_clockTimer->stop();updateTimes();}
