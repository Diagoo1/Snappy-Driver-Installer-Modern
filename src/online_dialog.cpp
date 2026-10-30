#include "appearance.h"
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
#include <QElapsedTimer>
#include <QStorageInfo>
#include <QStyledItemDelegate>
#include <QPainter>
#include <QPainterPath>
#include <QMouseEvent>
#include <QKeyEvent>
#include "offline_paths.h"
#include "ui_font.h"

void OnlineWorker::run(){
    error.clear();published.clear();
    QFile journal;if(!checking){QDir().mkpath(cache);journal.setFileName(QDir(cache).filePath(QStringLiteral("update-events.jsonl")));journal.open(QIODevice::WriteOnly|QIODevice::Append);}
    const auto record=[&journal](const QString &message,const QString &detail=QString()){
        if(!journal.isOpen())return;QJsonObject event;event.insert(QStringLiteral("time"),QDateTime::currentDateTimeUtc().toString(Qt::ISODate));event.insert(QStringLiteral("message"),message);event.insert(QStringLiteral("detail"),detail);journal.write(QJsonDocument(event).toJson(QJsonDocument::Compact));journal.write("\n");journal.flush();
    };
    record(checking?QStringLiteral("Reading update catalog"):QStringLiteral("Starting verified package download"));
    QElapsedTimer clock;clock.start();qint64 lastTime=0,lastReceived=0,rate=0;QMap<int,qint64> shown;QString lastMessage;
    try{
        if(stop.load())throw std::runtime_error("Download paused. Verified blocks were kept for resuming.");
        if(checking){emit activity(QStringLiteral("Reading the official update catalog..."),0,0,0);
            const QByteArray bytes=imported.isEmpty()?OnlinePackages::fetchMetadata(metadataUrl,stop):imported;
            catalog=OnlinePackages::parse(bytes);
        }else published=OnlinePackages::download(catalog,selected,drivers,indexes,cache,stop,[&](const OnlinePackages::Progress &p){
            if(p.message!=lastMessage || !p.diagnostic.isEmpty()){record(p.message,p.diagnostic);lastMessage=p.message;}
            const qint64 now=clock.elapsed();if(now-lastTime>=200){const qint64 sample=qMax(qint64(0),p.received-lastReceived)*1000/qMax(qint64(1),now-lastTime);rate=rate?(rate*2+sample)/3:sample;lastTime=now;lastReceived=p.received;}
            emit transferStats(p.received,p.downloadRate>=0?p.downloadRate:rate,p.uploadRate,p.peers);emit activity(p.message,p.verified,p.total,p.published);
            for(auto it=p.fileVerified.constBegin();it!=p.fileVerified.constEnd();++it)if(shown.value(it.key(),-1)!=it.value()){shown[it.key()]=it.value();emit fileProgress(it.key(),it.value(),catalog.files[it.key()].size);}
        });
    }catch(const std::exception &e){error=QString::fromUtf8(e.what());}
    record(error.isEmpty()?QStringLiteral("Operation completed"):error);
}
static QString sizeText(qint64 size){
    if(size<1024)return QStringLiteral("%1 B").arg(qMax(qint64(0),size));
    if(size>=1024LL*1024*1024)return QStringLiteral("%1 GB").arg(size/(1024.0*1024*1024),0,'f',2);
    if(size>=1024*1024)return QStringLiteral("%1 MB").arg(size/(1024.0*1024),0,'f',1);
    return QStringLiteral("%1 KB").arg(size/1024.0,0,'f',1);
}
class DownloadBar : public QProgressBar {
public:explicit DownloadBar(QWidget *parent):QProgressBar(parent),m_tick(this),m_phase(0){m_tick.setInterval(40);connect(&m_tick,&QTimer::timeout,this,[this](){if(!isVisible() || maximum()!=0){m_tick.stop();return;}m_phase=(m_phase+1)%60;update();});}
protected:void hideEvent(QHideEvent *event) Q_DECL_OVERRIDE {m_tick.stop();QProgressBar::hideEvent(event);}
 void paintEvent(QPaintEvent*) Q_DECL_OVERRIDE {const bool busy=maximum()==0,reduced=qApp->property("reduceMotion").toBool();if(busy && !reduced && isVisible()){if(!m_tick.isActive())m_tick.start();}else m_tick.stop();QPainter p(this);p.setRenderHint(QPainter::Antialiasing);const bool dark=Theme::isDark(this);const QRectF track=QRectF(rect()).adjusted(.5,.5,-.5,-.5);QPainterPath clip;clip.addRoundedRect(track,track.height()/2,track.height()/2);p.fillPath(clip,Theme::color("elevated",dark));QRectF fill=track;if(busy){const qreal segment=track.width()*.3;fill.setLeft(reduced?(track.width()-segment)/2:(track.width()+segment)*m_phase/59-segment);fill.setWidth(segment);}else{fill.setWidth(track.width()*qBound(0.0,(value()-minimum())/qreal(qMax(1,maximum()-minimum())),1.0));if(layoutDirection()==Qt::RightToLeft)fill.moveRight(track.right());}QLinearGradient gradient(track.topLeft(),track.topRight());gradient.setColorAt(0,QColor("#167B9D"));gradient.setColorAt(1,QColor("#118676"));p.save();p.setClipPath(clip);p.fillRect(fill,gradient);p.restore();if(!busy){const QString text=QStringLiteral("%1%").arg(qRound((value()-minimum())*100.0/qMax(1,maximum()-minimum())));p.setPen(Theme::color("text",dark));p.drawText(track,Qt::AlignCenter,text);p.setClipRect(fill);p.setClipPath(clip,Qt::IntersectClip);p.setPen(Qt::white);p.drawText(track,Qt::AlignCenter,text);}}
private:QTimer m_tick;int m_phase;
};
class PackageHeader : public QHeaderView {
public:explicit PackageHeader(QWidget *parent):QHeaderView(Qt::Horizontal,parent){setFixedHeight(48);}
protected:void paintEvent(QPaintEvent*) Q_DECL_OVERRIDE {QPainter p(viewport());p.setRenderHint(QPainter::Antialiasing);const bool dark=Theme::isDark(this);p.fillRect(viewport()->rect(),Theme::color("surface",dark));QRectF band=QRectF(viewport()->rect()).adjusted(.5,3,-.5,-3);QLinearGradient gradient(band.topLeft(),band.topRight());gradient.setColorAt(0,Theme::color("headerStart",dark));gradient.setColorAt(1,Theme::color("headerEnd",dark));p.setPen(Qt::NoPen);p.setBrush(gradient);p.drawRoundedRect(band,11,11);p.setPen(Qt::white);QFont face=font();face.setFamily(UiFont::family());face.setPixelSize(12);face.setWeight(QFont::DemiBold);p.setFont(face);for(int i=0;i<count();++i){QRect area(sectionViewportPosition(i)+10,3,sectionSize(i)-20,height()-6);p.drawText(area,Qt::AlignVCenter|(i==0?(layoutDirection()==Qt::RightToLeft?Qt::AlignRight:Qt::AlignLeft):Qt::AlignHCenter),p.fontMetrics().elidedText(model()->headerData(i,Qt::Horizontal).toString(),Qt::ElideRight,area.width()));}}
};
class PackageTree : public QTreeWidget {
public:explicit PackageTree(QWidget *parent):QTreeWidget(parent){setMouseTracking(true);}
protected:void resizeEvent(QResizeEvent *event) Q_DECL_OVERRIDE {QTreeWidget::resizeEvent(event);const int widths[]={34,9,19,9,9,9,11};const int total=viewport()->width();for(int i=0;i<7;++i)header()->resizeSection(i,qMax(i==0?240:70,total*widths[i]/100));}
 void mouseMoveEvent(QMouseEvent *event) Q_DECL_OVERRIDE {QTreeWidgetItem *item=itemAt(event->pos());setProperty("hoverPackage",item?item->data(0,Qt::UserRole).toInt():-1);viewport()->update();QTreeWidget::mouseMoveEvent(event);}
 void leaveEvent(QEvent *event) Q_DECL_OVERRIDE {setProperty("hoverPackage",-1);viewport()->update();QTreeWidget::leaveEvent(event);}
 void mousePressEvent(QMouseEvent *event) Q_DECL_OVERRIDE {if(property("transferLocked").toBool() && event->button()==Qt::LeftButton){event->accept();return;}QTreeWidget::mousePressEvent(event);}
 void keyPressEvent(QKeyEvent *event) Q_DECL_OVERRIDE {if(property("transferLocked").toBool() && event->key()==Qt::Key_Space){event->accept();return;}QTreeWidget::keyPressEvent(event);}
 void drawRow(QPainter *p,const QStyleOptionViewItem &option,const QModelIndex &index)const Q_DECL_OVERRIDE {if(property("hoverPackage").isValid() && property("hoverPackage").toInt()==index.data(Qt::UserRole).toInt()){p->save();p->setRenderHint(QPainter::Antialiasing);p->setPen(Qt::NoPen);p->setBrush(Theme::color("elevated",Theme::isDark(this)));p->drawRoundedRect(QRectF(2,option.rect.top()+3,viewport()->width()-4,option.rect.height()-6),11,11);p->restore();}QStyleOptionViewItem clean(option);clean.state &= ~(QStyle::State_MouseOver|QStyle::State_Selected|QStyle::State_HasFocus);QTreeWidget::drawRow(p,clean,index);}
};
class PackageDelegate : public QStyledItemDelegate {
public:explicit PackageDelegate(QObject *parent):QStyledItemDelegate(parent){}
 void paint(QPainter *p,const QStyleOptionViewItem &option,const QModelIndex &index)const Q_DECL_OVERRIDE {
    if(index.column()!=2 && index.column()!=6){QStyleOptionViewItem clean(option);clean.state &= ~(QStyle::State_MouseOver|QStyle::State_Selected|QStyle::State_HasFocus);QStyledItemDelegate::paint(p,clean,index);return;}
    p->save();p->setRenderHint(QPainter::Antialiasing);const bool dark=Theme::isDark(option.widget);const QRectF track=QRectF(option.rect).adjusted(8,15,-8,-15);p->setPen(Qt::NoPen);p->setBrush(Theme::color("elevated",dark));p->drawRoundedRect(track,9,9);
    if(index.column()==2){const qreal ratio=qBound(0.0,index.data(Qt::UserRole+1).toDouble(),1.0);if(ratio>0){QPainterPath clip;clip.addRoundedRect(track,9,9);p->setClipPath(clip);QLinearGradient g(track.topLeft(),track.topRight());g.setColorAt(0,QColor("#2CB7C5"));g.setColorAt(1,QColor("#32CBB0"));p->setBrush(g);QRectF fill=track;fill.setWidth(track.width()*ratio);if(option.direction==Qt::RightToLeft)fill.moveRight(track.right());p->drawRect(fill);p->setClipping(false);}}
    QFont face=option.font;face.setPixelSize(12);face.setWeight(QFont::Medium);p->setFont(face);p->setPen(Theme::color("text",dark));p->drawText(track,Qt::AlignCenter,index.data().toString());p->restore();
 }
};
QString OnlineDialog::trText(const char *en,const char *ar)const{return UiLanguage::text(QString::fromUtf8(en),QString::fromUtf8(ar));}
OnlineDialog::OnlineDialog(const QString &drivers,const QString &indexes,const QStringList &needed,bool arabic,QWidget *parent)
 :RoundedDialog(parent),m_arabic(arabic),m_changed(false),m_closePending(false),m_needed(needed),m_worker(new OnlineWorker(this)){
    setLayoutDirection(UiLanguage::isRtl()?Qt::RightToLeft:Qt::LeftToRight);
    setProperty("darkMode",Theme::isDark(parent));
    setObjectName(QStringLiteral("onlineDialog"));setWindowTitle(trText("Package updates","تحديث حزم التعريفات"));
    const QRect usable=QApplication::desktop()->availableGeometry(parent);setMinimumSize(qMin(920,usable.width()-32),qMin(580,usable.height()-32));resize(qMin(1180,usable.width()-32),qMin(980,usable.height()-48));
    m_worker->metadataUrl=OnlinePackages::officialMetadataUrl(libraryFamily(drivers,indexes));m_worker->drivers=drivers;m_worker->indexes=indexes;m_worker->cache=QDir(drivers).filePath(QStringLiteral(".sdio-downloads"));
    QVBoxLayout *frame=new QVBoxLayout(this);frame->setContentsMargins(0,0,0,0);frame->setSpacing(0);
    frame->addWidget(new DialogHeader(trText("Download center","مركز التنزيل"),QStringLiteral("layers"),this,arabic));
    QWidget *body=new QWidget(this);QVBoxLayout *root=new QVBoxLayout(body);root->setContentsMargins(24,14,24,16);root->setSpacing(8);frame->addWidget(body,1);
    QLabel *title=new QLabel(trText("Driver packages & indexes","حزم التعريفات والفهارس"),this);title->setObjectName(QStringLiteral("dialogTitle"));title->hide();
    QLabel *description=new QLabel(trText("Update indexes → Scan this computer → Download matching packages. Downloads are verified before saving; no driver is installed. You can keep sharing verified blocks while the application is open.","حدّث الفهارس ← افحص الكمبيوتر ← نزّل الحزم المطابقة. يتم التحقق من الملفات قبل حفظها، دون تثبيت أي تعريف. يمكنك استمرار مشاركة الأجزاء الموثوقة أثناء فتح البرنامج."),this);description->setWordWrap(true);description->setObjectName(QStringLiteral("subText"));root->addWidget(description);
    auto button=[this](const QString &text,const char *name){QPushButton *b=new ActionButton(text,this);b->setObjectName(QString::fromLatin1(name));b->setCursor(Qt::PointingHandCursor);return b;};
    QHBoxLayout *source=new QHBoxLayout();m_check=button(trText("Check for updates","البحث عن تحديثات"),"outlineButton");m_import=button(trText("Open catalog file...","فتح ملف التحديثات..."),"textLinkButton");source->addWidget(m_check);source->addWidget(m_import);source->addStretch();QLineEdit *search=new QLineEdit(this);m_search=search;search->setPlaceholderText(trText("Find a package...","ابحث عن حزمة..."));search->setMaximumWidth(280);search->setClearButtonEnabled(true);source->addWidget(search,1);root->addLayout(source);
    ActionButton *transferToggle=new ActionButton(trText("Transfer settings","إعدادات النقل"),this);transferToggle->setObjectName(QStringLiteral("textLinkButton"));source->insertWidget(2,transferToggle);
    m_transferPanel=new SurfaceCard(this);m_transferPanel->setObjectName(QStringLiteral("softSurface"));QGridLayout *transferGrid=new QGridLayout(m_transferPanel);transferGrid->setContentsMargins(14,11,14,11);transferGrid->setHorizontalSpacing(12);
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
    QLabel *sourceName=new QLabel(trText("Official catalog: %1 · Driver packs and indexes", "الكتالوج الرسمي: %1 · حزم التعريفات والفهارس").arg(m_worker->metadataUrl.host().contains(QStringLiteral("sdi-tool"))?QStringLiteral("SDI"):QStringLiteral("SDIO")),this);m_sourceLabel=sourceName;sourceName->setObjectName(QStringLiteral("bodyText"));root->addWidget(sourceName);
    m_updatesOnly=new QCheckBox(trText("Updates only","التحديثات فقط"),this);m_updatesOnly->setObjectName(QStringLiteral("updatesOnly"));m_updatesOnly->setChecked(true);m_hideMissing=new QCheckBox(trText("Hide missing packages","إخفاء الحزم غير الموجودة محليًا"),this);m_hideMissing->setObjectName(QStringLiteral("hideMissingPackages"));QHBoxLayout *visibility=new QHBoxLayout;visibility->addWidget(m_updatesOnly);visibility->addWidget(m_hideMissing);visibility->addStretch();root->addLayout(visibility);
    m_files=new PackageTree(this);m_files->setObjectName(QStringLiteral("onlineFiles"));m_files->setRootIsDecorated(false);m_files->setUniformRowHeights(true);m_files->setAlternatingRowColors(false);m_files->setIndentation(0);m_files->setColumnCount(7);m_files->setItemDelegate(new PackageDelegate(m_files));m_files->setHeader(new PackageHeader(m_files));
    m_files->setHeaderLabels(QStringList()<<trText("PACKAGE / INDEX","الحزمة / الفهرس")<<trText("SIZE","الحجم")<<trText("PROGRESS","التقدم")<<trText("NEW","الجديد")<<trText("LOCAL","المحلي")<<trText("THIS PC","هذا الجهاز")<<trText("STATUS","الحالة"));
    m_files->header()->setMinimumSectionSize(60);for(int column=0;column<7;++column)m_files->header()->setSectionResizeMode(column,QHeaderView::Fixed);m_files->setMinimumHeight(qMin(300,qMax(170,usable.height()-570)));root->addWidget(m_files,1);
    QHBoxLayout *select=new QHBoxLayout();m_all=button(trText("Select all","تحديد الكل"),"outlineButton");m_indexes=button(trText("Select indexes","تحديد الفهارس"),"outlineButton");m_neededButton=button(trText("Packages for this scan","حزم هذا الفحص"),"outlineButton");m_network=button(trText("Network only","الشبكة فقط"),"outlineButton");m_none=button(trText("Clear selection","إلغاء التحديد"),"outlineButton");const QList<QPushButton*> presets=QList<QPushButton*>()<<m_all<<m_indexes<<m_neededButton<<m_network<<m_none;for(QPushButton *b:presets)select->addWidget(b);root->addLayout(select);
    m_summary=new QLabel(this);m_summary->setObjectName(QStringLiteral("bodyText"));m_summary->setWordWrap(true);root->addWidget(m_summary);
    QWidget *transfer=new SurfaceCard(this);transfer->setObjectName(QStringLiteral("softSurface"));transfer->setProperty("tone",QStringLiteral("blue"));QVBoxLayout *transferBody=new QVBoxLayout(transfer);transferBody->setContentsMargins(18,14,18,14);transferBody->setSpacing(10);QHBoxLayout *metrics=new QHBoxLayout;
    auto metric=[&](const QString &caption,const QString &name,const QString &symbol,const QColor &color){QWidget *tile=new QWidget(transfer);tile->setObjectName(QStringLiteral("transferTile"));QVBoxLayout *box=new QVBoxLayout(tile);box->setContentsMargins(9,5,9,5);box->setSpacing(4);QHBoxLayout *titleRow=new QHBoxLayout;titleRow->setSpacing(7);QLabel *icon=new QLabel(tile);icon->setPixmap(VectorIcons::pixmap(symbol,color,16));titleRow->addWidget(icon);QLabel *title=new QLabel(caption,tile);title->setObjectName(QStringLiteral("subText"));titleRow->addWidget(title,1);QLabel *value=new QLabel(QStringLiteral("—"),tile);value->setObjectName(name);value->setProperty("transferMetric",true);box->addLayout(titleRow);box->addWidget(value);metrics->addWidget(tile,1);return value;};
    m_speed=metric(trText("Download speed","سرعة التنزيل"),QStringLiteral("downloadSpeed"),QStringLiteral("activity"),QColor("#25A8C0"));m_eta=metric(trText("Time remaining","الوقت المتبقي"),QStringLiteral("downloadEta"),QStringLiteral("clock"),QColor("#A38AD6"));m_received=metric(trText("Received","تم استلامه"),QStringLiteral("downloadReceived"),QStringLiteral("download"),QColor("#1AAE9F"));m_peers=metric(trText("Peers","المصادر"),QStringLiteral("downloadPeers"),QStringLiteral("network"),QColor("#3A9FD0"));m_upload=metric(trText("Upload speed","سرعة الرفع"),QStringLiteral("uploadSpeed"),QStringLiteral("upload"),QColor("#C79B49"));transferBody->addLayout(metrics);
    m_progress=new DownloadBar(transfer);m_progress->setObjectName(QStringLiteral("mainProgress"));m_progress->setRange(0,1000);m_progress->setValue(0);m_progress->setFormat(QStringLiteral("%p%"));m_progress->setFixedHeight(24);transferBody->addWidget(m_progress);m_verifiedLabel=new QLabel(trText("Choose the packages you want to download","اختر الحزم المطلوب تنزيلها"),transfer);m_verifiedLabel->setObjectName(QStringLiteral("subText"));transferBody->addWidget(m_verifiedLabel);root->addWidget(transfer);
    m_status=new QLabel(trText("Check for updates to read the official catalog. A catalog file can also be opened without an Internet connection.","اضغط البحث عن تحديثات لقراءة القائمة الرسمية، أو افتح ملف التحديثات المحفوظ دون اتصال."),this);m_status->setObjectName(QStringLiteral("subText"));m_status->setWordWrap(true);m_status->setMinimumHeight(18);root->addWidget(m_status);
    QHBoxLayout *actions=new QHBoxLayout();m_pause=button(trText("Pause download","إيقاف مؤقت"),"outlineButton");m_pause->hide();m_download=button(trText("Download selected","تنزيل المحدد"),"primaryButton");m_download->setMinimumHeight(44);m_download->setMinimumWidth(190);m_close=button(trText("Close","إغلاق"),"outlineButton");actions->addWidget(m_pause);actions->addStretch();actions->addWidget(m_close);actions->addWidget(m_download);root->addLayout(actions);
    connect(m_check,&QPushButton::clicked,this,[this](){check();});connect(m_import,&QPushButton::clicked,this,[this](){check(true);});connect(m_download,&QPushButton::clicked,this,&OnlineDialog::download);
    connect(m_close,&QPushButton::clicked,this,&QWidget::close);connect(m_pause,&QPushButton::clicked,this,[this](){m_worker->stop.store(true);m_pause->setEnabled(false);m_status->setText(trText("Pausing after the current network request...","جارٍ الإيقاف بعد طلب الشبكة الحالي..."));});
    connect(m_worker,&QThread::finished,this,&OnlineDialog::completed,Qt::QueuedConnection);
    connect(m_worker,&OnlineWorker::activity,this,[this](const QString &message,qint64 verified,qint64 total,int published){m_status->setText(UiLanguage::text(message));if(published)m_changed=true;m_verified=verified;m_total=total;m_verifiedLabel->setText(trText("Verified: %1 / %2 · %3 file(s) saved","تم التحقق: %1 / %2 · تم حفظ %3 ملفات").arg(sizeText(verified),sizeText(total)).arg(published));if(total>0){m_progress->setRange(0,1000);m_progress->setValue(static_cast<int>(verified*1000/total));}else m_progress->setRange(0,0);},Qt::QueuedConnection);
    connect(search,&QLineEdit::textChanged,this,[this](){filterRows();});connect(m_updatesOnly,&QCheckBox::toggled,this,[this](){filterRows();});connect(m_hideMissing,&QCheckBox::toggled,this,[this](){filterRows();});
    connect(m_files,&QTreeWidget::itemChanged,this,[this](){updateSelection();});
    connect(m_indexes,&QPushButton::clicked,this,[this](){QSignalBlocker blocked(m_files);QTreeWidgetItem *first=0;for(int i=0;i<m_files->topLevelItemCount();++i){QTreeWidgetItem *row=m_files->topLevelItem(i);const bool selected=!row->isHidden() && m_catalog.files[row->data(0,Qt::UserRole).toInt()].kind==OnlinePackages::Index;row->setCheckState(0,selected?Qt::Checked:Qt::Unchecked);if(selected && !first)first=row;}if(first)m_files->scrollToItem(first,QAbstractItemView::PositionAtTop);updateSelection();});
    connect(m_none,&QPushButton::clicked,this,[this](){QSignalBlocker blocked(m_files);for(int i=0;i<m_files->topLevelItemCount();++i)m_files->topLevelItem(i)->setCheckState(0,Qt::Unchecked);updateSelection();});
    connect(m_neededButton,&QPushButton::clicked,this,[this](){QSignalBlocker blocked(m_files);int found=0;QTreeWidgetItem *first=0;for(int i=0;i<m_files->topLevelItemCount();++i){QTreeWidgetItem *row=m_files->topLevelItem(i);const OnlinePackages::File &f=m_catalog.files[row->data(0,Qt::UserRole).toInt()];const bool wanted=!row->isHidden() && f.kind==OnlinePackages::Driver && m_availability[row->data(0,Qt::UserRole).toInt()].needed && m_availability[row->data(0,Qt::UserRole).toInt()].update;row->setCheckState(0,wanted?Qt::Checked:Qt::Unchecked);if(wanted){++found;if(!first)first=row;}}if(first)m_files->scrollToItem(first,QAbstractItemView::PositionAtTop);updateSelection();
        if(!found)m_status->setText(trText("No missing packages from this scan match the catalog. Update indexes and scan again first.","لا توجد حزم ناقصة من هذا الفحص تطابق القائمة. حدّث الفهارس ثم أعد الفحص أولًا."));});
    m_rateExpiry=new QTimer(this);m_rateExpiry->setInterval(500);connect(m_rateExpiry,&QTimer::timeout,this,[this](){if(!m_worker->isRunning()){m_rateExpiry->stop();return;}if(m_telemetryClock.isValid() && m_telemetryClock.elapsed()>1500){m_speed->setText(QStringLiteral("0 B/s"));m_eta->setText(QStringLiteral("—"));}});
    connect(m_worker,&OnlineWorker::transferStats,this,[this](qint64 received,qint64 speed,qint64 upload,int peers){m_telemetryClock.restart();m_received->setText(sizeText(received));m_speed->setText(sizeText(speed)+QStringLiteral("/s"));m_upload->setText(sizeText(upload)+QStringLiteral("/s"));m_peers->setText(QString::number(peers));if(speed>0 && m_total>m_verified){const qint64 seconds=(m_total-m_verified)/speed;m_eta->setText(QStringLiteral("%1:%2:%3").arg(seconds/3600,2,10,QLatin1Char('0')).arg(seconds/60%60,2,10,QLatin1Char('0')).arg(seconds%60,2,10,QLatin1Char('0')));}else m_eta->setText(QStringLiteral("—"));},Qt::QueuedConnection);
    connect(m_worker,&OnlineWorker::fileProgress,this,[this](int index,qint64 verified,qint64 total){for(int row=0;row<m_files->topLevelItemCount();++row){QTreeWidgetItem *item=m_files->topLevelItem(row);if(item->data(0,Qt::UserRole).toInt()!=index)continue;const qreal ratio=total?qBound(0.0,verified/qreal(total),1.0):1;item->setData(2,Qt::UserRole+1,ratio);item->setText(2,QStringLiteral("%1%").arg(qRound(ratio*100)));}},Qt::QueuedConnection);
    connect(m_all,&QPushButton::clicked,this,[this](){QSignalBlocker blocked(m_files);for(int i=0;i<m_files->topLevelItemCount();++i){QTreeWidgetItem *row=m_files->topLevelItem(i);row->setCheckState(0,!row->isHidden()?Qt::Checked:Qt::Unchecked);}updateSelection();});
    connect(m_network,&QPushButton::clicked,this,[this](){QSignalBlocker blocked(m_files);for(int i=0;i<m_files->topLevelItemCount();++i){QTreeWidgetItem *row=m_files->topLevelItem(i);const int index=row->data(0,Qt::UserRole).toInt();row->setCheckState(0,!row->isHidden() && m_availability[index].network && m_availability[index].update?Qt::Checked:Qt::Unchecked);}updateSelection();});
    updateSelection();
}
void OnlineDialog::setCatalogSource(const QUrl &source){if(m_worker->isRunning() || !m_catalog.files.isEmpty())return;m_worker->metadataUrl=source;m_sourceLabel->setText(trText("Official catalog: %1 · Driver packs and indexes", "الكتالوج الرسمي: %1 · حزم التعريفات والفهارس").arg(source.host().contains(QStringLiteral("sdi-tool"))?QStringLiteral("SDI"):QStringLiteral("SDIO")));}
void OnlineDialog::setCatalog(const OnlinePackages::Catalog &catalog){
    m_catalog=catalog;m_availability=OnlinePackages::availability(catalog,m_worker->drivers,m_worker->indexes,m_needed);m_files->blockSignals(true);m_files->clear();int indices=0,packs=0;
    for(int i=0;i<catalog.files.size();++i){const OnlinePackages::File &f=catalog.files[i];if(f.kind==OnlinePackages::Other)continue;QTreeWidgetItem *row=new QTreeWidgetItem(m_files);row->setData(0,Qt::UserRole,i);row->setText(0,f.path.section(QLatin1Char('/'),-1));row->setToolTip(0,f.path);row->setText(1,sizeText(f.size));row->setCheckState(0,Qt::Unchecked);row->setSizeHint(0,QSize(0,50));
        const OnlinePackages::Availability &state=m_availability[i];row->setText(2,QStringLiteral("0%"));row->setData(2,Qt::UserRole+1,0.0);row->setText(3,state.available.isEmpty()?QStringLiteral("—"):state.available);row->setText(4,state.current.isEmpty()?QStringLiteral("—"):state.current);row->setText(5,f.kind==OnlinePackages::Index?QStringLiteral("—"):m_needed.isEmpty()?trText("Scan first","افحص أولًا"):state.needed?trText("Yes","نعم"):trText("No","لا"));row->setText(6,state.local?trText("Current","محدّث"):state.missing?trText("Missing","غير موجود"):state.update?trText("Update","تحديث"):trText("Newer locally","أحدث محليًا"));

        row->setIcon(0,VectorIcons::icon(f.kind==OnlinePackages::Driver?QStringLiteral("chip"):QStringLiteral("layers"),QColor("#00A79D"),18));if(f.kind==OnlinePackages::Driver)++packs;else ++indices;}
    m_files->blockSignals(false);filterRows();m_status->setText(trText("Catalog ready: %1 indexes and %2 packages. Select what you need.","القائمة جاهزة: %1 فهرس و%2 حزمة. حدد المطلوب تنزيله.").arg(indices).arg(packs));updateSelection();
}
QVector<int> OnlineDialog::selection()const{QVector<int> selected;for(int i=0;i<m_files->topLevelItemCount();++i){QTreeWidgetItem *row=m_files->topLevelItem(i);if(row->checkState(0)==Qt::Checked)selected.append(row->data(0,Qt::UserRole).toInt());}return selected;}
void OnlineDialog::updateSelection(){
    const QVector<int> selected=selection();qint64 size=0;for(int index:selected)size+=m_catalog.files[index].size;
    qint64 transfer=0;if(!selected.isEmpty())transfer=OnlinePackages::requiredBytes(m_catalog,selected);
    m_summary->setText(trText("%1 selected · Files: %2 · Verified download: up to %3", "%1 محدد · حجم الملفات: %2 · التنزيل المطلوب للتحقق: حتى %3").arg(selected.size()).arg(sizeText(size)).arg(sizeText(transfer)));
    QString spacePath=m_worker->drivers;for(int i=0;i<32 && !QDir(spacePath).exists();++i){const QString parent=QFileInfo(spacePath).absolutePath();if(parent==spacePath)break;spacePath=parent;}const QStorageInfo volume(spacePath);int hidden=0;for(int i=0;i<m_files->topLevelItemCount();++i)if(m_files->topLevelItem(i)->isHidden() && m_files->topLevelItem(i)->checkState(0)==Qt::Checked)++hidden;if(hidden)m_summary->setText(m_summary->text()+trText(" · %1 selected hidden by filters"," · %1 تحديدات مخفية بالفلاتر").arg(hidden));
    m_summary->setText(m_summary->text()+trText(" · Free space: %1"," · المساحة المتاحة: %1").arg(volume.isValid() && volume.bytesAvailable()>=0?sizeText(volume.bytesAvailable()):QStringLiteral("—")));
    m_all->setEnabled(!m_worker->isRunning() && !m_catalog.files.isEmpty());m_network->setEnabled(m_all->isEnabled());
    m_download->setEnabled(!m_worker->isRunning() && !selected.isEmpty());m_indexes->setEnabled(!m_worker->isRunning() && !m_catalog.files.isEmpty());m_neededButton->setEnabled(!m_worker->isRunning() && !m_catalog.files.isEmpty() && !m_needed.isEmpty());m_none->setEnabled(!m_worker->isRunning() && !m_catalog.files.isEmpty());
}
void OnlineDialog::setWorking(bool running){m_all->setEnabled(!running && !m_catalog.files.isEmpty());m_network->setEnabled(m_all->isEnabled());m_updatesOnly->setEnabled(!running);m_hideMissing->setEnabled(!running);m_transferPanel->setEnabled(!running);m_check->setEnabled(!running);m_import->setEnabled(!running);m_files->setProperty("transferLocked",running);m_pause->setVisible(running);m_pause->setEnabled(running);m_download->setEnabled(!running && !selection().isEmpty());m_indexes->setEnabled(!running && !m_catalog.files.isEmpty());m_neededButton->setEnabled(!running && !m_catalog.files.isEmpty() && !m_needed.isEmpty());m_none->setEnabled(!running && !m_catalog.files.isEmpty());}
void OnlineDialog::filterRows(){
    QSignalBlocker blocked(m_files);const QString query=m_search->text().trimmed();for(int i=0;i<m_files->topLevelItemCount();++i){QTreeWidgetItem *row=m_files->topLevelItem(i);const int index=row->data(0,Qt::UserRole).toInt();const auto &state=m_availability[index];const bool hidden=(!query.isEmpty() && !row->text(0).contains(query,Qt::CaseInsensitive)) || (m_updatesOnly->isChecked() && !state.update) || (m_hideMissing->isChecked() && state.missing);row->setHidden(hidden);}updateSelection();
}
void OnlineDialog::check(bool importFile){
    if(m_worker->isRunning())return;m_worker->imported.clear();
    if(importFile){const QString path=QFileDialog::getOpenFileName(this,trText("Open update catalog","فتح ملف التحديثات"),QString(),QStringLiteral("Torrent catalog (*.torrent)"));if(path.isEmpty())return;QFile file(path);if(!file.open(QIODevice::ReadOnly) || file.size()<=0 || file.size()>8*1024*1024){m_status->setText(trText("Cannot read this catalog file.","تعذّر قراءة ملف التحديثات."));return;}m_worker->imported=file.readAll();m_sourceLabel->setText(trText("Imported catalog: %1","كتالوج مستورد: %1").arg(QFileInfo(path).fileName()));}
    m_catalog=OnlinePackages::Catalog();m_availability.clear();m_files->clear();updateSelection();if(!importFile)m_sourceLabel->setText(trText("Official catalog: %1 · Driver packs and indexes", "الكتالوج الرسمي: %1 · حزم التعريفات والفهارس").arg(m_worker->metadataUrl.host().contains(QStringLiteral("sdi-tool"))?QStringLiteral("SDI"):QStringLiteral("SDIO")));m_worker->checking=true;m_worker->stop.store(false);setWorking(true);m_progress->setRange(0,0);m_worker->start();
}
void OnlineDialog::download(){if(m_worker->isRunning() || selection().isEmpty())return;
    if(m_port->value()>0 && m_port->value()<1024){m_status->setText(trText("Choose an automatic port or a port from 1024 to 65535.","اختر منفذًا تلقائيًا أو بين 1024 و65535."));return;}
    m_catalog.transfer.downloadKiB=m_downloadLimit->value();m_catalog.transfer.uploadKiB=m_uploadLimit->value();m_catalog.transfer.connections=m_connections->value();m_catalog.transfer.port=m_port->value();m_catalog.transfer.keepSharing=m_sharing->isChecked();
    QScopedPointer<QSettings> preferences(createAppSettings(QCoreApplication::applicationDirPath(),0,false));preferences->setValue(QStringLiteral("transfer/downloadKiB"),m_downloadLimit->value());preferences->setValue(QStringLiteral("transfer/uploadKiB"),m_uploadLimit->value());preferences->setValue(QStringLiteral("transfer/connections"),m_connections->value());preferences->setValue(QStringLiteral("transfer/port"),m_port->value());preferences->sync();
    m_telemetryClock.start();m_rateExpiry->start();m_verified=0;m_total=OnlinePackages::requiredBytes(m_catalog,selection());m_received->setText(QStringLiteral("0 B"));m_speed->setText(QStringLiteral("0 B/s"));m_eta->setText(QStringLiteral("—"));m_worker->catalog=m_catalog;m_worker->selected=selection();m_worker->checking=false;m_worker->stop.store(false);setWorking(true);m_progress->setRange(0,1000);m_progress->setValue(0);m_worker->start();}
void OnlineDialog::completed(){
    m_rateExpiry->stop();setWorking(false);m_progress->setRange(0,1000);
    if(!m_worker->checking && m_worker->stop.load())m_download->setText(trText("Resume download","استكمال التنزيل"));else m_download->setText(trText("Download selected","تنزيل المحدد"));
    if(!m_worker->error.isEmpty())m_status->setText(m_worker->error+(m_changed?trText(" Some files were already saved; scan again to refresh matches."," تم حفظ بعض الملفات؛ أعد الفحص لتحديث المطابقة."):QString()));
    else if(m_worker->checking){m_catalogCheckedAt=QDateTime::currentDateTimeUtc();setCatalog(m_worker->catalog);}
    else {m_speed->setText(QStringLiteral("0 B/s"));m_eta->setText(QStringLiteral("—"));m_changed=!m_worker->published.isEmpty() || m_changed;m_progress->setValue(1000);m_status->setText(trText("%1 verified files saved. Close this window to refresh driver matches. No driver was installed.","تم حفظ %1 ملف بعد التحقق. أغلق هذه النافذة لتحديث مطابقة التعريفات. لم يتم تثبيت أي تعريف.").arg(m_worker->published.size()));}
    if(!m_worker->checking){for(const QString &saved:m_worker->published)for(int i=0;i<m_files->topLevelItemCount();++i){QTreeWidgetItem *row=m_files->topLevelItem(i);const int index=row->data(0,Qt::UserRole).toInt();if(QFileInfo(saved).fileName()==m_catalog.files[index].targetName)row->setText(6,trText("Saved","تم الحفظ"));}}
    if(m_closePending)reject();
}
void OnlineDialog::closeEvent(QCloseEvent *event){if(m_worker->isRunning()){m_closePending=true;m_worker->stop.store(true);m_status->setText(trText("Finishing the current request before closing...","جارٍ إنهاء طلب الشبكة الحالي قبل الإغلاق..."));event->ignore();}else event->accept();}

OnlineDialog::~OnlineDialog(){m_worker->stop.store(true);m_worker->wait();}
void OnlineDialog::reject(){if(m_worker->isRunning()){m_closePending=true;m_worker->stop.store(true);return;}RoundedDialog::reject();}
