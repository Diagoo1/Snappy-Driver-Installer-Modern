#include "online_dialog.h"
#include "ui_language.h"
#include "sharing_restore.h"
#include <stdexcept>
#include "theme.h"
#include "vector_icons.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QPushButton>
#include <QTreeWidget>
#include <QHeaderView>
#include <QProgressBar>
#include <QFileDialog>
#include <QFile>
#include <QFileInfo>
#include <QDir>
#include <QCloseEvent>
#include <QApplication>
#include <QDesktopWidget>
#include <QSignalBlocker>
#include <QJsonObject>
#include <QJsonDocument>
#include <QDateTime>
#include <QLineEdit>
#include <QSpinBox>
#include <QCheckBox>
#include <QTimer>
#include <QGridLayout>
#include "app_settings.h"
#include <QScopedPointer>

void OnlineWorker::run(){
    error.clear();published.clear();
    QDir().mkpath(cache);QFile journal(QDir(cache).filePath(QStringLiteral("update-events.jsonl")));journal.open(QIODevice::WriteOnly|QIODevice::Append);
    const auto record=[&journal](const QString &message,const QString &detail=QString()){
        if(!journal.isOpen())return;QJsonObject event;event.insert(QStringLiteral("time"),QDateTime::currentDateTimeUtc().toString(Qt::ISODate));event.insert(QStringLiteral("message"),message);event.insert(QStringLiteral("detail"),detail);journal.write(QJsonDocument(event).toJson(QJsonDocument::Compact));journal.write("\n");journal.flush();
    };
    record(checking?QStringLiteral("Reading update catalog"):QStringLiteral("Starting verified package download"));
    try{
        if(stop.load())throw std::runtime_error("Download paused. Verified blocks were kept for resuming.");
        if(checking){emit activity(QStringLiteral("Reading the official update catalog..."),0,0,0);
            const QByteArray bytes=imported.isEmpty()?OnlinePackages::fetchMetadata(OnlinePackages::officialMetadataUrl(),stop):imported;
            catalog=OnlinePackages::parse(bytes);
        }else published=OnlinePackages::download(catalog,selected,drivers,indexes,cache,stop,[this,&record](const OnlinePackages::Progress &p){record(p.message,p.diagnostic);emit activity(p.message,p.verified,p.total,p.published);});
    }catch(const std::exception &e){error=QString::fromUtf8(e.what());}
    record(error.isEmpty()?QStringLiteral("Operation completed"):error);
}
static QString sizeText(qint64 size){
    if(size>=1024LL*1024*1024)return QStringLiteral("%1 GB").arg(size/(1024.0*1024*1024),0,'f',2);
    if(size>=1024*1024)return QStringLiteral("%1 MB").arg(size/(1024.0*1024),0,'f',1);
    return QStringLiteral("%1 KB").arg(qMax(1LL,size/1024));
}
QString OnlineDialog::trText(const char *en,const char *ar)const{return UiLanguage::text(QString::fromUtf8(en),QString::fromUtf8(ar));}
OnlineDialog::OnlineDialog(const QString &drivers,const QString &indexes,const QStringList &needed,bool arabic,QWidget *parent)
 :RoundedDialog(parent),m_arabic(arabic),m_changed(false),m_closePending(false),m_needed(needed),m_worker(new OnlineWorker(this)){
    setLayoutDirection(UiLanguage::isRtl()?Qt::RightToLeft:Qt::LeftToRight);
    setObjectName(QStringLiteral("onlineDialog"));setWindowTitle(trText("Package updates","تحديث حزم التعريفات"));
    setMinimumSize(740,500);const QRect usable=QApplication::desktop()->availableGeometry(parent);resize(qMin(980,usable.width()-32),qMin(720,usable.height()-32));
    m_worker->drivers=drivers;m_worker->indexes=indexes;m_worker->cache=QDir(drivers).filePath(QStringLiteral(".sdio-downloads"));
    QVBoxLayout *frame=new QVBoxLayout(this);frame->setContentsMargins(0,0,0,0);frame->setSpacing(0);
    frame->addWidget(new DialogHeader(trText("Download center","مركز التنزيل"),QStringLiteral("layers"),this,arabic));
    QWidget *body=new QWidget(this);QVBoxLayout *root=new QVBoxLayout(body);root->setContentsMargins(24,20,24,22);root->setSpacing(12);frame->addWidget(body,1);
    QLabel *title=new QLabel(trText("Driver packages & indexes","حزم التعريفات والفهارس"),this);title->setObjectName(QStringLiteral("dialogTitle"));root->addWidget(title);
    QLabel *description=new QLabel(trText("Update indexes → Scan this computer → Download matching packages. Downloads are verified before saving; no driver is installed. You can keep sharing verified blocks while the application is open.","حدّث الفهارس ← افحص الكمبيوتر ← نزّل الحزم المطابقة. يتم التحقق من الملفات قبل حفظها، دون تثبيت أي تعريف. يمكنك استمرار مشاركة الأجزاء الموثوقة أثناء فتح البرنامج."),this);description->setWordWrap(true);description->setObjectName(QStringLiteral("subText"));root->addWidget(description);
    auto button=[this](const QString &text,const char *name){QPushButton *b=new ActionButton(text,this);b->setObjectName(QString::fromLatin1(name));b->setCursor(Qt::PointingHandCursor);return b;};
    QHBoxLayout *source=new QHBoxLayout();m_check=button(trText("Check for updates","البحث عن تحديثات"),"outlineButton");m_import=button(trText("Open catalog file...","فتح ملف التحديثات..."),"textLinkButton");source->addWidget(m_check);source->addWidget(m_import);source->addStretch();QLineEdit *search=new QLineEdit(this);search->setPlaceholderText(trText("Find a package...","ابحث عن حزمة..."));search->setMaximumWidth(280);search->setClearButtonEnabled(true);source->addWidget(search,1);root->addLayout(source);
    ActionButton *transferToggle=new ActionButton(trText("Transfer settings","إعدادات النقل"),this);transferToggle->setObjectName(QStringLiteral("textLinkButton"));source->insertWidget(2,transferToggle);
    m_transferPanel=new QWidget(this);m_transferPanel->setObjectName(QStringLiteral("softSurface"));QGridLayout *transferGrid=new QGridLayout(m_transferPanel);transferGrid->setContentsMargins(14,11,14,11);transferGrid->setHorizontalSpacing(12);
    auto spin=[&](const QString &caption,int low,int high,int value,int column){QLabel *label=new QLabel(caption,m_transferPanel);label->setObjectName(QStringLiteral("subText"));transferGrid->addWidget(label,0,column);QSpinBox *input=new QSpinBox(m_transferPanel);input->setRange(low,high);input->setValue(value);input->setMinimumWidth(110);transferGrid->addWidget(input,1,column);return input;};
    QScopedPointer<QSettings> preferences(createAppSettings(QCoreApplication::applicationDirPath(),0,false));
    m_downloadLimit=spin(trText("Download KiB/s","التنزيل KiB/s"),0,102400,preferences->value(QStringLiteral("transfer/downloadKiB"),0).toInt(),0);m_downloadLimit->setSpecialValueText(trText("Unlimited","غير محدود"));
    m_uploadLimit=spin(trText("Upload KiB/s","الرفع KiB/s"),1,102400,preferences->value(QStringLiteral("transfer/uploadKiB"),64).toInt(),1);
    m_connections=spin(trText("Peer connections","اتصالات المصادر"),1,200,preferences->value(QStringLiteral("transfer/connections"),40).toInt(),2);
    m_port=spin(trText("Peer port","منفذ المصادر"),0,65535,preferences->value(QStringLiteral("transfer/port"),0).toInt(),3);m_port->setSpecialValueText(trText("Automatic","تلقائي"));
    m_port->setToolTip(trText("Automatic, or a port from 1024 to 65535.","تلقائي، أو منفذ بين 1024 و65535."));
    m_sharing=new QCheckBox(trText("Keep sharing verified downloads","استمرار مشاركة التنزيلات الموثوقة"),m_transferPanel);m_sharing->setObjectName(QStringLiteral("keepSharing"));m_sharing->setChecked(preferences->value(QStringLiteral("transfer/keepSharing"),true).toBool());transferGrid->addWidget(m_sharing,2,0,1,4);
    m_sharingStatus=new QLabel(m_transferPanel);m_sharingStatus->setObjectName(QStringLiteral("subText"));transferGrid->addWidget(m_sharingStatus,3,0,1,4);
    QTimer *sharingTimer=new QTimer(this);connect(sharingTimer,&QTimer::timeout,this,[this]{m_sharingStatus->setText(m_sharing->isChecked()?UiLanguage::text(OnlinePackages::sharingStatus()):trText("Sharing is stopped","المشاركة متوقفة"));});sharingTimer->start(1500);
    connect(m_sharing,&QCheckBox::toggled,this,[this](bool enabled){QScopedPointer<QSettings> settings(createAppSettings(QCoreApplication::applicationDirPath(),0,false));settings->setValue(QStringLiteral("transfer/keepSharing"),enabled);settings->sync();OnlinePackages::setSharingEnabled(enabled);if(enabled){OnlinePackages::TransferSettings limits;limits.keepSharing=true;limits.uploadKiB=m_uploadLimit->value();limits.connections=m_connections->value();limits.port=m_port->value()>=1024?m_port->value():0;SharingRestore *resume=new SharingRestore(m_worker->cache,limits,this);connect(resume,&QThread::finished,resume,&QObject::deleteLater);resume->start();}});
    root->addWidget(m_transferPanel);m_transferPanel->hide();connect(transferToggle,&QPushButton::clicked,this,[this](){m_transferPanel->setVisible(!m_transferPanel->isVisible());});
    m_files=new QTreeWidget(this);m_files->setObjectName(QStringLiteral("onlineFiles"));m_files->setRootIsDecorated(false);m_files->setUniformRowHeights(true);m_files->setAlternatingRowColors(true);m_files->setIndentation(0);m_files->setColumnCount(3);
    m_files->setHeaderLabels(QStringList()<<trText("PACKAGE / INDEX","الحزمة / الفهرس")<<trText("SIZE","الحجم")<<trText("LOCAL FILE","الملف المحلي"));
    m_files->header()->setSectionResizeMode(0,QHeaderView::Stretch);m_files->header()->setSectionResizeMode(1,QHeaderView::ResizeToContents);m_files->header()->setSectionResizeMode(2,QHeaderView::ResizeToContents);root->addWidget(m_files,1);
    QHBoxLayout *select=new QHBoxLayout();m_indexes=button(trText("Select indexes","تحديد الفهارس"),"textLinkButton");m_neededButton=button(trText("Packages for this scan","حزم هذا الفحص"),"textLinkButton");m_none=button(trText("Clear selection","إلغاء التحديد"),"textLinkButton");select->addWidget(m_indexes);select->addWidget(m_neededButton);select->addWidget(m_none);select->addStretch();root->addLayout(select);
    m_summary=new QLabel(this);m_summary->setObjectName(QStringLiteral("bodyText"));m_summary->setWordWrap(true);root->addWidget(m_summary);
    m_progress=new QProgressBar(this);m_progress->setObjectName(QStringLiteral("mainProgress"));m_progress->setRange(0,1000);m_progress->setValue(0);m_progress->setFormat(QStringLiteral("%p%"));root->addWidget(m_progress);
    m_status=new QLabel(trText("Check for updates to read the official catalog. A catalog file can also be opened without an Internet connection.","اضغط البحث عن تحديثات لقراءة القائمة الرسمية، أو افتح ملف التحديثات المحفوظ دون اتصال."),this);m_status->setObjectName(QStringLiteral("subText"));m_status->setWordWrap(true);m_status->setMinimumHeight(40);root->addWidget(m_status);
    QHBoxLayout *actions=new QHBoxLayout();m_pause=button(trText("Pause download","إيقاف مؤقت"),"outlineButton");m_pause->hide();m_download=button(trText("Download selected","تنزيل المحدد"),"primaryButton");m_download->setMinimumHeight(44);m_download->setMinimumWidth(190);m_close=button(trText("Close","إغلاق"),"outlineButton");actions->addWidget(m_pause);actions->addStretch();actions->addWidget(m_close);actions->addWidget(m_download);root->addLayout(actions);
    connect(m_check,&QPushButton::clicked,this,[this](){check();});connect(m_import,&QPushButton::clicked,this,[this](){check(true);});connect(m_download,&QPushButton::clicked,this,&OnlineDialog::download);
    connect(m_close,&QPushButton::clicked,this,&QWidget::close);connect(m_pause,&QPushButton::clicked,this,[this](){m_worker->stop.store(true);m_pause->setEnabled(false);m_status->setText(trText("Pausing after the current network request...","جارٍ الإيقاف بعد طلب الشبكة الحالي..."));});
    connect(m_worker,&QThread::finished,this,&OnlineDialog::completed,Qt::QueuedConnection);
    connect(m_worker,&OnlineWorker::activity,this,[this](const QString &message,qint64 verified,qint64 total,int published){m_status->setText(UiLanguage::text(message));if(published)m_changed=true;if(total>0 && verified>0){m_progress->setRange(0,1000);m_progress->setValue(static_cast<int>(verified*1000/total));}else m_progress->setRange(0,0);},Qt::QueuedConnection);
    connect(search,&QLineEdit::textChanged,this,[this](const QString &query){for(int i=0;i<m_files->topLevelItemCount();++i){QTreeWidgetItem *item=m_files->topLevelItem(i);item->setHidden(!item->text(0).contains(query,Qt::CaseInsensitive) && !item->toolTip(0).contains(query,Qt::CaseInsensitive));}});
    connect(m_files,&QTreeWidget::itemChanged,this,[this](){updateSelection();});
    connect(m_indexes,&QPushButton::clicked,this,[this](){QSignalBlocker blocked(m_files);QTreeWidgetItem *first=0;for(int i=0;i<m_files->topLevelItemCount();++i){QTreeWidgetItem *row=m_files->topLevelItem(i);const bool selected=m_catalog.files[row->data(0,Qt::UserRole).toInt()].kind==OnlinePackages::Index;row->setCheckState(0,selected?Qt::Checked:Qt::Unchecked);if(selected && !first)first=row;}if(first)m_files->scrollToItem(first,QAbstractItemView::PositionAtTop);updateSelection();});
    connect(m_none,&QPushButton::clicked,this,[this](){QSignalBlocker blocked(m_files);for(int i=0;i<m_files->topLevelItemCount();++i)m_files->topLevelItem(i)->setCheckState(0,Qt::Unchecked);updateSelection();});
    connect(m_neededButton,&QPushButton::clicked,this,[this](){QSignalBlocker blocked(m_files);int found=0;QTreeWidgetItem *first=0;for(int i=0;i<m_files->topLevelItemCount();++i){QTreeWidgetItem *row=m_files->topLevelItem(i);const OnlinePackages::File &f=m_catalog.files[row->data(0,Qt::UserRole).toInt()];const bool wanted=f.kind==OnlinePackages::Driver && m_needed.contains(f.targetName,Qt::CaseInsensitive) && !QFileInfo(QDir(m_worker->drivers).filePath(f.targetName)).isFile();row->setCheckState(0,wanted?Qt::Checked:Qt::Unchecked);if(wanted){++found;if(!first)first=row;}}if(first)m_files->scrollToItem(first,QAbstractItemView::PositionAtTop);updateSelection();
        if(!found)m_status->setText(trText("No missing packages from this scan match the catalog. Update indexes and scan again first.","لا توجد حزم ناقصة من هذا الفحص تطابق القائمة. حدّث الفهارس ثم أعد الفحص أولًا."));});
    updateSelection();
}
void OnlineDialog::setCatalog(const OnlinePackages::Catalog &catalog){
    m_catalog=catalog;m_files->blockSignals(true);m_files->clear();int indices=0,packs=0;
    for(int i=0;i<catalog.files.size();++i){const OnlinePackages::File &f=catalog.files[i];if(f.kind==OnlinePackages::Other)continue;QTreeWidgetItem *row=new QTreeWidgetItem(m_files);row->setData(0,Qt::UserRole,i);row->setText(0,f.path.section(QLatin1Char('/'),-1));row->setToolTip(0,f.path);row->setText(1,sizeText(f.size));row->setCheckState(0,Qt::Unchecked);row->setSizeHint(0,QSize(0,36));
        const QString path=QDir(f.kind==OnlinePackages::Driver?m_worker->drivers:m_worker->indexes).filePath(f.targetName);row->setText(2,QFileInfo(path).isFile()?trText("Found locally","موجود محليًا"):trText("Not present","غير موجود"));
        row->setIcon(0,VectorIcons::icon(f.kind==OnlinePackages::Driver?QStringLiteral("chip"):QStringLiteral("layers"),QColor("#00A79D"),18));if(f.kind==OnlinePackages::Driver)++packs;else ++indices;}
    m_files->blockSignals(false);m_status->setText(trText("Catalog ready: %1 indexes and %2 packages. Select what you need.","القائمة جاهزة: %1 فهرس و%2 حزمة. حدد المطلوب تنزيله.").arg(indices).arg(packs));updateSelection();
}
QVector<int> OnlineDialog::selection()const{QVector<int> selected;for(int i=0;i<m_files->topLevelItemCount();++i){QTreeWidgetItem *row=m_files->topLevelItem(i);if(row->checkState(0)==Qt::Checked)selected.append(row->data(0,Qt::UserRole).toInt());}return selected;}
void OnlineDialog::updateSelection(){
    const QVector<int> selected=selection();qint64 size=0;for(int index:selected)size+=m_catalog.files[index].size;
    qint64 transfer=0;if(!selected.isEmpty())transfer=OnlinePackages::requiredBytes(m_catalog,selected);
    m_summary->setText(trText("%1 selected · Files: %2 · Verified download: up to %3", "%1 محدد · حجم الملفات: %2 · التنزيل المطلوب للتحقق: حتى %3").arg(selected.size()).arg(sizeText(size)).arg(sizeText(transfer)));
    m_download->setEnabled(!m_worker->isRunning() && !selected.isEmpty());m_indexes->setEnabled(!m_worker->isRunning() && !m_catalog.files.isEmpty());m_neededButton->setEnabled(!m_worker->isRunning() && !m_catalog.files.isEmpty());m_none->setEnabled(!m_worker->isRunning() && !m_catalog.files.isEmpty());
}
void OnlineDialog::setWorking(bool running){m_transferPanel->setEnabled(!running);m_check->setEnabled(!running);m_import->setEnabled(!running);m_files->setEnabled(!running);m_pause->setVisible(running);m_pause->setEnabled(running);m_download->setEnabled(!running && !selection().isEmpty());m_indexes->setEnabled(!running && !m_catalog.files.isEmpty());m_neededButton->setEnabled(!running && !m_catalog.files.isEmpty());m_none->setEnabled(!running && !m_catalog.files.isEmpty());}
void OnlineDialog::check(bool importFile){
    if(m_worker->isRunning())return;m_worker->imported.clear();
    if(importFile){const QString path=QFileDialog::getOpenFileName(this,trText("Open update catalog","فتح ملف التحديثات"),QString(),QStringLiteral("Torrent catalog (*.torrent)"));if(path.isEmpty())return;QFile file(path);if(!file.open(QIODevice::ReadOnly) || file.size()<=0 || file.size()>8*1024*1024){m_status->setText(trText("Cannot read this catalog file.","تعذّر قراءة ملف التحديثات."));return;}m_worker->imported=file.readAll();}
    m_worker->checking=true;m_worker->stop.store(false);setWorking(true);m_progress->setRange(0,0);m_worker->start();
}
void OnlineDialog::download(){if(m_worker->isRunning() || selection().isEmpty())return;
    if(m_port->value()>0 && m_port->value()<1024){m_status->setText(trText("Choose an automatic port or a port from 1024 to 65535.","اختر منفذًا تلقائيًا أو بين 1024 و65535."));return;}
    m_catalog.transfer.downloadKiB=m_downloadLimit->value();m_catalog.transfer.uploadKiB=m_uploadLimit->value();m_catalog.transfer.connections=m_connections->value();m_catalog.transfer.port=m_port->value();m_catalog.transfer.keepSharing=m_sharing->isChecked();
    QScopedPointer<QSettings> preferences(createAppSettings(QCoreApplication::applicationDirPath(),0,false));preferences->setValue(QStringLiteral("transfer/downloadKiB"),m_downloadLimit->value());preferences->setValue(QStringLiteral("transfer/uploadKiB"),m_uploadLimit->value());preferences->setValue(QStringLiteral("transfer/connections"),m_connections->value());preferences->setValue(QStringLiteral("transfer/port"),m_port->value());preferences->sync();
    m_worker->catalog=m_catalog;m_worker->selected=selection();m_worker->checking=false;m_worker->stop.store(false);setWorking(true);m_progress->setRange(0,1000);m_progress->setValue(0);m_worker->start();}
void OnlineDialog::completed(){
    setWorking(false);m_progress->setRange(0,1000);
    if(!m_worker->error.isEmpty())m_status->setText(m_worker->error+(m_changed?trText(" Some files were already saved; scan again to refresh matches."," تم حفظ بعض الملفات؛ أعد الفحص لتحديث المطابقة."):QString()));
    else if(m_worker->checking)setCatalog(m_worker->catalog);
    else {m_changed=!m_worker->published.isEmpty() || m_changed;m_progress->setValue(1000);m_status->setText(trText("%1 verified files saved. Close this window and scan again. No driver was installed.","تم حفظ %1 ملف بعد التحقق. أغلق هذه النافذة وأعد الفحص. لم يتم تثبيت أي تعريف.").arg(m_worker->published.size()));}
    if(m_closePending)reject();
}
void OnlineDialog::closeEvent(QCloseEvent *event){if(m_worker->isRunning()){m_closePending=true;m_worker->stop.store(true);m_status->setText(trText("Finishing the current request before closing...","جارٍ إنهاء طلب الشبكة الحالي قبل الإغلاق..."));event->ignore();}else event->accept();}

OnlineDialog::~OnlineDialog(){m_worker->stop.store(true);m_worker->wait();}
void OnlineDialog::reject(){if(m_worker->isRunning()){m_closePending=true;m_worker->stop.store(true);return;}RoundedDialog::reject();}
