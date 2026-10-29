#include "installation_workspace.h"
#include "vector_icons.h"
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
    top->addWidget(m_badge);
    outer->addLayout(top);

    QWidget *mainProgress=new QWidget(this);
    mainProgress->setObjectName(QStringLiteral("softSurface"));
    QVBoxLayout *mp=new QVBoxLayout(mainProgress);
    mp->setContentsMargins(23,18,23,18);mp->setSpacing(10);
    m_stage=textLabel(QStringLiteral("Ready for native installation when validated"),"sectionTitle",mainProgress);
    mp->addWidget(m_stage);
    m_progress=new QProgressBar(mainProgress);
    m_progress->setObjectName(QStringLiteral("mainProgress"));
    m_progress->setRange(0,100);m_progress->setValue(0);
    m_progress->setTextVisible(true);m_progress->setFormat(QStringLiteral("%p%"));
    m_progress->setMinimumHeight(21);
    mp->addWidget(m_progress);
    m_summary=textLabel(QStringLiteral("No tasks scheduled"),"subText",mainProgress);
    mp->addWidget(m_summary);
    outer->addWidget(mainProgress);

    QHBoxLayout *center=new QHBoxLayout();center->setSpacing(15);
    QWidget *milestones=new QWidget(this);
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

    QWidget *operations=new QWidget(this);
    operations->setObjectName(QStringLiteral("softSurface"));
    QVBoxLayout *olist=new QVBoxLayout(operations);
    olist->setContentsMargins(18,18,18,15);olist->setSpacing(9);
    olist->addWidget(textLabel(QStringLiteral("INSTALLATION ITEMS"),"detailHeading",operations));
    m_queue=new QTreeWidget(operations);
    m_queue->setObjectName(QStringLiteral("installQueue"));
    m_queue->setColumnCount(2);
    m_queue->setHeaderLabels(QStringList()<<QStringLiteral("ITEM")<<QStringLiteral("STATUS"));
    m_queue->setRootIsDecorated(false);
    m_queue->setIndentation(0);
    m_queue->setUniformRowHeights(true);
    m_queue->setAlternatingRowColors(false);
    m_queue->setSelectionMode(QAbstractItemView::NoSelection);
    m_queue->setFocusPolicy(Qt::NoFocus);
    m_queue->header()->setSectionResizeMode(0,QHeaderView::Stretch);
    m_queue->header()->setSectionResizeMode(1,QHeaderView::Fixed);
    m_queue->header()->resizeSection(1,175);
    m_queue->setVerticalScrollMode(QAbstractItemView::ScrollPerPixel);
    m_queue->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    olist->addWidget(m_queue,1);
    center->addWidget(operations,1);
    outer->addLayout(center,1);

    QHBoxLayout *bottom=new QHBoxLayout();
    m_complete=textLabel(QStringLiteral("No changes are made to Windows."),"demoText",this);
    bottom->addWidget(m_complete,1);
    m_stop=new QPushButton(QStringLiteral("Stop demo"),this);
    m_stop->setObjectName(QStringLiteral("outlineButton"));
    m_stop->setVisible(false);
    bottom->addWidget(m_stop);
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
        item->setText(1,QStringLiteral("Queued"));
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
    updateProgress();
    emit activeChanged(true);
    m_timer->start();
}
void InstallationWorkspace::step()
{
    if(!m_active || m_live) return;
    m_percent=qMin(100,m_percent+1);
    updateProgress();
    if(m_percent>=100){
        m_timer->stop();
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
        row->setText(1,state);
        if(m_percent>=finish) {++completed;row->setForeground(1,QColor("#31B273"));}
        else if(m_percent>=start)row->setForeground(1,QColor("#159FAC"));
    }
    m_summary->setText(QStringLiteral("%1 / %2 items completed · 0 real driver operations")
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
    m_timer->stop();m_active=false;
    m_back->setEnabled(true);m_stop->setVisible(false);
    m_stage->setText(QStringLiteral("Simulation stopped"));
    m_complete->setText(QStringLiteral("No driver changes were performed."));
    emit activeChanged(false);
}
void InstallationWorkspace::setArabic(bool arabic)
{
    m_arabic=arabic;
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
    m_badge->setText(QStringLiteral("REAL SYSTEM OPERATION - TEST BUILD"));
    m_tip->setText(QStringLiteral("The current Windows driver operation is not interruptible. Stop cancels the remaining queue after the active driver returns."));
    for(int i=0;i<names.size();++i){
        QTreeWidgetItem *row=new QTreeWidgetItem(m_queue);row->setText(0,names.at(i));row->setText(1,QStringLiteral("Pending"));
        row->setData(0,Qt::UserRole,ids.at(i));row->setSizeHint(0,QSize(0,51));
        row->setIcon(0,VectorIcons::icon(QStringLiteral("chip"),QColor("#16A9AD"),18));
    }
    m_stepRestore->setText(restore?QStringLiteral("01  Restore point"):QStringLiteral("01  Restore point (skipped)"));
    m_stepPrep->setText(QStringLiteral("02  Validation & extraction"));m_stepInstall->setText(QStringLiteral("03  Installing drivers"));m_stepFinish->setText(QStringLiteral("04  Results"));
    m_currentStage=-1;
    m_items=names;m_back->setEnabled(false);m_stop->setVisible(true);m_stop->setEnabled(true);m_stop->setText(QStringLiteral("Stop after current"));
    m_progress->setRange(0,names.size());m_progress->setValue(0);m_progress->setFormat(QStringLiteral("%v / %m"));
    m_stage->setText(QStringLiteral("Validating selected candidates and local packages"));
    m_summary->setText(QStringLiteral("0 / %1 drivers completed").arg(names.size()));
    m_complete->setText(QStringLiteral("Results and errors below come from the native installer, not a simulation."));emit activeChanged(true);
}
void InstallationWorkspace::applyNativeEvent(const sdiomodern::Event &e)
{
    if(!m_live)return;
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
        if(!id.isEmpty() && row->data(0,Qt::UserRole).toString().compare(id,Qt::CaseInsensitive)==0){row->setText(1,status);row->setToolTip(1,QString::fromStdWString(e.message));}}
    m_summary->setText(QStringLiteral("%1 / %2 drivers processed").arg(static_cast<qulonglong>(e.done)).arg(static_cast<qulonglong>(e.total)));
}
void InstallationWorkspace::finishNative(const sdiomodern::InstallReport &report)
{
    if(!m_live)return;int ok=0,failed=0,skipped=0;
    for(std::size_t i=0;i<report.items.size();++i){const sdiomodern::Result &result=report.items[i];
        QString status;
        if(result.outcome==sdiomodern::Outcome::Success){++ok;status=result.rebootRequired?QStringLiteral("Installed - restart"):QStringLiteral("Installed");}
        else if(result.outcome==sdiomodern::Outcome::Failure){++failed;status=QStringLiteral("Failed (%1)").arg(result.nativeError);}
        else{++skipped;status=QStringLiteral("Not installed");}
        for(int j=0;j<m_queue->topLevelItemCount();++j){QTreeWidgetItem *row=m_queue->topLevelItem(j);
            if(row->data(0,Qt::UserRole).toString().compare(QString::fromStdWString(result.deviceId),Qt::CaseInsensitive)==0){row->setText(1,status);row->setToolTip(1,QString::fromStdWString(result.message));}}
    }
    for(int j=0;j<m_queue->topLevelItemCount();++j){QTreeWidgetItem *row=m_queue->topLevelItem(j);
        if(row->text(1)==QStringLiteral("Pending")){row->setText(1,QStringLiteral("Not started"));++skipped;}}
    m_active=false;m_live=false;m_stop->setVisible(false);m_back->setEnabled(true);
    QLabel *steps[]={m_stepRestore,m_stepPrep,m_stepInstall,m_stepFinish};for(int i=0;i<4;++i){steps[i]->setObjectName(i==3?QStringLiteral("activeStage"):QStringLiteral("stageItem"));steps[i]->style()->unpolish(steps[i]);steps[i]->style()->polish(steps[i]);}
    m_stage->setText(report.cancelled?QStringLiteral("Stopped safely after the active operation"):
      (failed || !report.error.empty()?QStringLiteral("Completed with errors"):QStringLiteral("Native installation finished")));
    m_summary->setText(QStringLiteral("Installed: %1 | Failed: %2 | Not installed: %3").arg(ok).arg(failed).arg(skipped));
    m_complete->setText(!report.error.empty()?QString::fromStdWString(report.error):
        (report.rebootRequired?QStringLiteral("Windows restart is required. Restart manually after saving your work."):
         QStringLiteral("Scan again to verify the current Windows driver state.")));
    m_progress->setValue(ok+failed+skipped);emit activeChanged(false);
}
void InstallationWorkspace::failNative(const QString &reason)
{
    if(!m_live)return;m_active=false;m_live=false;m_stop->setVisible(false);m_back->setEnabled(true);
    m_stage->setText(QStringLiteral("Native operation stopped"));m_complete->setText(reason);
    for(int i=0;i<m_queue->topLevelItemCount();++i){QTreeWidgetItem *row=m_queue->topLevelItem(i);
        if(row->text(1)==QStringLiteral("Pending"))row->setText(1,QStringLiteral("Not started"));}
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
