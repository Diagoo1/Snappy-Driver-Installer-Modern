#include "ui_language.h"
#include "ui_font.h"
#include "theme.h"
#include "appearance.h"
#include "driver_workspace.h"
#include "driver_catalog.h"
#include "origin_languages.h"
#include "driver_details_dialog.h"
#include "premium_popover.h"
#include "ui_elements.h"
#include "vector_icons.h"
#include <QVBoxLayout>
#include <QCoreApplication>
#include <QHBoxLayout>
#include <QGridLayout>
#include <QTreeWidget>
#include <QHeaderView>
#include <QLineEdit>
#include <QLabel>
#include <QPushButton>
#include <QCheckBox>
#include <QToolButton>
#include <QSignalBlocker>
#include <QTimer>
#include <QResizeEvent>
#include <QFont>
#include <QVariant>
#include <QFrame>
#include <QPalette>
#include <QSet>
#include <QMap>
#include <QScrollBar>
#include <QAbstractItemView>
#include <QMenu>
#include <QApplication>
#include <QClipboard>
#include <QDesktopServices>
#include <QUrl>
#include <QCursor>
#include <QMouseEvent>
#include <QPainter>
#include <QItemSelectionModel>
#include <QKeyEvent>
#include <QElapsedTimer>
#include <QTreeWidgetItemIterator>
#include <QEasingCurve>
#include <functional>
#include <algorithm>
#include "driver_row_delegate.h"

// Paint the entire header once so native section backgrounds cannot leave square corners.
class DriverHeader : public QHeaderView {
public:explicit DriverHeader(QWidget *parent):QHeaderView(Qt::Horizontal,parent){setFixedHeight(64);viewport()->setAutoFillBackground(false);viewport()->setAttribute(Qt::WA_OpaquePaintEvent,false);}
protected:void paintEvent(QPaintEvent*) Q_DECL_OVERRIDE {
    QPainter p(viewport());p.setRenderHint(QPainter::Antialiasing);const bool dark=Theme::isDark(this),rtl=layoutDirection()==Qt::RightToLeft;
    p.fillRect(viewport()->rect(),Theme::color("surface",dark));
    const QRectF band=QRectF(viewport()->rect()).adjusted(.5,3.5,-.5,-3.5);QLinearGradient fill(band.topLeft(),band.topRight());fill.setColorAt(0,Theme::color("headerStart",dark));fill.setColorAt(1,Theme::color("headerEnd",dark));p.setPen(Qt::NoPen);p.setBrush(fill);p.drawRoundedRect(band,12,12);
    QPainterPath clip;clip.addRoundedRect(band,12,12);p.setClipPath(clip);
    QFont face=font();face.setFamily(UiFont::family());face.setPixelSize(15);face.setWeight(QFont::DemiBold);p.setFont(face);
    const char *symbols[]={"monitor","shield","layers","check-circle"};
    for(int section=0;section<count();++section){
        if(isSectionHidden(section))continue;const QRect area(sectionViewportPosition(section),4,sectionSize(section),height()-8);
        if(section>0){const int x=rtl?area.right():area.left();p.setPen(QPen(QColor(255,255,255,35),1));p.drawLine(QPointF(x,band.top()+11),QPointF(x,band.bottom()-11));}
        const QString label=p.fontMetrics().elidedText(model()->headerData(section,Qt::Horizontal).toString(),Qt::ElideRight,qMax(0,area.width()-52));
        const int textWidth=p.fontMetrics().width(label),total=textWidth+28;
        const int left=section==0?(rtl?area.right()-total-14:area.left()+14):area.center().x()-total/2;
        VectorIcons::icon(QString::fromLatin1(symbols[section%4]),QColor("#B6F3ED"),22).paint(&p,QRect(rtl?left+total-22:left,area.center().y()-11,22,22));
        p.setPen(Qt::white);p.drawText(QRect(rtl?left:left+28,area.top(),textWidth,area.height()),Qt::AlignAbsolute|(rtl?Qt::AlignRight:Qt::AlignLeft)|Qt::AlignVCenter,label);
    }
}
};

// Populate before QTreeView measures the opening animation, including branch clicks.
class DriverTree : public QTreeWidget {
public:
    explicit DriverTree(QWidget *parent):QTreeWidget(parent),m_arrowTimer(this),m_branchPress(false),m_suppressRelease(false){
        m_clock.start();m_arrowTimer.setInterval(16);
        connect(&m_arrowTimer,&QTimer::timeout,this,[this](){for(auto it=m_arrows.begin();it!=m_arrows.end();){if(m_clock.elapsed()-it.value().started>=160)it=m_arrows.erase(it);else ++it;}if(m_arrows.isEmpty()){m_arrowTimer.stop();setProperty("branchAnimationRunning",false);}viewport()->update();});
        connect(this,&QTreeWidget::itemExpanded,this,[this](QTreeWidgetItem *item){animateArrow(item,true);});
        connect(this,&QTreeWidget::itemCollapsed,this,[this](QTreeWidgetItem *item){animateArrow(item,false);});
        connect(model(),&QAbstractItemModel::modelReset,this,[this](){stopArrowMotion();});
    }
    void setMotionEnabled(bool enabled){setAnimated(enabled);if(!enabled)stopArrowMotion();}
    qreal arrowAngle(int row,qreal fallback)const {auto it=m_arrows.constFind(row);if(it==m_arrows.constEnd())return fallback;const qreal progress=qBound(qreal(0),(m_clock.elapsed()-it.value().started)/160.0,qreal(1));return it.value().from+(it.value().to-it.value().from)*QEasingCurve(QEasingCurve::OutCubic).valueForProgress(progress);}
    std::function<void(QTreeWidgetItem*)> prepare,details;
    std::function<void(QTreeWidgetItem*,const QPoint&)> actions;
protected:
    void mouseMoveEvent(QMouseEvent *event) Q_DECL_OVERRIDE {
        QTreeWidgetItem *item=itemAt(event->pos());
        setProperty("hoverRow",item?item->data(0,Qt::UserRole).toInt():-1);
        setProperty("hoverCandidate",item?item->data(0,Qt::UserRole+1).toInt():-2);
        viewport()->setCursor(item?Qt::PointingHandCursor:Qt::ArrowCursor);viewport()->update();QTreeWidget::mouseMoveEvent(event);
    }
    void leaveEvent(QEvent *event) Q_DECL_OVERRIDE {setProperty("hoverRow",-1);viewport()->update();QTreeWidget::leaveEvent(event);}
    void drawRow(QPainter *p,const QStyleOptionViewItem &option,const QModelIndex &index)const Q_DECL_OVERRIDE {
        const bool hovered=property("hoverRow").toInt()==index.data(Qt::UserRole).toInt() && property("hoverCandidate").toInt()==index.data(Qt::UserRole+1).toInt();
        const bool category=index.data(Qt::UserRole+13).toBool(),dark=Theme::isDark(this);
        // Paint categories across the branch gutter too, so the chevron and label
        // share one rounded surface and one hover state in either direction.
        if(category || hovered || selectionModel()->isSelected(index)){p->save();p->setRenderHint(QPainter::Antialiasing);p->setPen(Qt::NoPen);p->setBrush(category?(dark?(hovered?QColor("#2D5262"):QColor("#244555")):(hovered?QColor("#DDEEF2"):QColor("#EAF4F6"))):Theme::color("elevated",dark));const int inset=category?5:3;p->drawRoundedRect(QRectF(2,option.rect.top()+inset,viewport()->width()-4,option.rect.height()-2*inset),category?13:12,category?13:12);p->restore();}
        QStyleOptionViewItem clean(option);clean.state &= ~(QStyle::State_MouseOver|QStyle::State_Selected|QStyle::State_HasFocus);QTreeWidget::drawRow(p,clean,index);
    }
    void drawBranches(QPainter *p,const QRect &rect,const QModelIndex &index)const Q_DECL_OVERRIDE {
        const bool dark=palette().color(QPalette::Base).lightness()<130;
        if(index.data(Qt::UserRole+1).toInt()<0 && model()->hasChildren(index)){
            p->save();p->setRenderHint(QPainter::Antialiasing);p->translate(QPoint(layoutDirection()==Qt::RightToLeft?rect.left()+indentation()/2:rect.right()-indentation()/2,rect.center().y()));const qreal angle=arrowAngle(index.data(Qt::UserRole).toInt(),isExpanded(index)?90:0);p->rotate(layoutDirection()==Qt::RightToLeft?180-angle:angle);
            VectorIcons::paint(p,QStringLiteral("chevron-right"),QRectF(-8,-8,16,16),dark?QColor("#A4C6D0"):QColor("#668594"));p->restore();
        }
    }
    void mousePressEvent(QMouseEvent *event) Q_DECL_OVERRIDE {
        if(event->button()==Qt::LeftButton && prepare)prepare(itemAt(event->pos()));
        QTreeWidgetItem *item=itemAt(event->pos());
        if(event->button()==Qt::LeftButton && item && item->data(0,Qt::UserRole+1).toInt()<0 && columnAt(event->pos().x())==0){const QRect row=visualItemRect(item);const bool branch=layoutDirection()==Qt::RightToLeft?event->pos().x()>row.right():event->pos().x()<row.left();if(branch){m_branchPress=true;item->setExpanded(!item->isExpanded());event->accept();return;}}
        QTreeWidget::mousePressEvent(event);
    }
    void mouseDoubleClickEvent(QMouseEvent *event) Q_DECL_OVERRIDE {
        if(event->button()==Qt::LeftButton){QTreeWidgetItem *item=itemAt(event->pos());const bool dots=columnAt(event->pos().x())==3 && (layoutDirection()==Qt::RightToLeft?event->pos().x()<header()->sectionViewportPosition(3)+32:event->pos().x()>=header()->sectionViewportPosition(3)+columnWidth(3)-32);if(item && details && !dots){m_suppressRelease=true;if(item->data(0,Qt::UserRole+13).toBool())item->setExpanded(!item->isExpanded());else details(item);event->accept();return;}}
        QTreeWidget::mouseDoubleClickEvent(event);
    }
    void mouseReleaseEvent(QMouseEvent *event) Q_DECL_OVERRIDE {
        if(m_suppressRelease || m_branchPress){m_suppressRelease=false;m_branchPress=false;event->accept();return;}
        if(event->button()==Qt::LeftButton && columnAt(event->pos().x())==3 &&
            (layoutDirection()==Qt::RightToLeft?event->pos().x()<header()->sectionViewportPosition(3)+32:event->pos().x()>=header()->sectionViewportPosition(3)+columnWidth(3)-32)){
            if(actions)actions(itemAt(event->pos()),viewport()->mapToGlobal(event->pos()));
            event->accept();return;
        }
        setProperty("clickX",event->pos().x());
        QTreeWidget::mouseReleaseEvent(event);
    }
    void keyPressEvent(QKeyEvent *event) Q_DECL_OVERRIDE {
        if((event->key()==Qt::Key_Right || event->key()==Qt::Key_Plus) && prepare)prepare(currentItem());
        QTreeWidget::keyPressEvent(event);
    }
private:
    struct ArrowMotion {qreal from,to;qint64 started;};
    QElapsedTimer m_clock;QTimer m_arrowTimer;QMap<int,ArrowMotion> m_arrows;bool m_branchPress,m_suppressRelease;
    void stopArrowMotion(){m_arrowTimer.stop();m_arrows.clear();setProperty("branchAnimationRunning",false);viewport()->update();}
    void animateArrow(QTreeWidgetItem *item,bool open){if(!item || item->data(0,Qt::UserRole+1).toInt()>=0)return;if(!isAnimated() || qApp->property("reduceMotion").toBool()){stopArrowMotion();return;}const int row=item->data(0,Qt::UserRole).toInt();ArrowMotion motion;motion.from=arrowAngle(row,open?0:90);motion.to=open?90:0;motion.started=m_clock.elapsed();m_arrows.insert(row,motion);setProperty("branchAnimationRunning",true);m_arrowTimer.start();viewport()->update();}
};

class ReinstallNotice : public QWidget {
public:explicit ReinstallNotice(QWidget *parent):QWidget(parent){setMinimumHeight(68);setSizePolicy(QSizePolicy::Expanding,QSizePolicy::Minimum);}
protected:void paintEvent(QPaintEvent*) Q_DECL_OVERRIDE {QPainter p(this);p.setRenderHint(QPainter::Antialiasing);const bool dark=Theme::isDark(this);const QRectF area=QRectF(rect()).adjusted(.5,.5,-.5,-.5);p.setBrush(dark?QColor("#344339"):QColor("#FFF8E9"));p.setPen(QPen(dark?QColor("#87734A"):QColor("#EBD5A9"),1));p.drawRoundedRect(area,13,13);}
};

static QLabel *label(const QString &text,const char *objectName,QWidget *parent)
{
    QLabel *v=new QLabel(text,parent);
    v->setObjectName(QString::fromLatin1(objectName));
    return v;
}
static QPushButton *actionButton(const QString &text, const char *objectName, QWidget *parent)
{
    QPushButton *v=new ActionButton(text,parent);
    v->setObjectName(QString::fromLatin1(objectName));
    v->setCursor(Qt::PointingHandCursor);
    return v;
}

DriverWorkspace::DriverWorkspace(DriverCatalog *catalog,QWidget *parent)
  : QWidget(parent),m_catalog(catalog),m_arabic(false),m_dark(false),m_busy(false),m_demo(false),m_rebuilding(false),m_clickRow(-1),m_clickCandidate(-1),m_clickPriorChoice(-1),m_clickPriorChecked(false)
{
    setObjectName(QStringLiteral("page"));
    QVBoxLayout *root=new QVBoxLayout(this);
    root->setContentsMargins(28,20,28,16);
    root->setSpacing(10);

    QHBoxLayout *heading=new QHBoxLayout();
    QVBoxLayout *titleBox=new QVBoxLayout();
    titleBox->setSpacing(3);
    m_title=label(QStringLiteral("Drivers for this computer"),"pageTitle",this);
    m_subtitle=label(QStringLiteral("Driver matching, alternatives and selection"),"subText",this);
    titleBox->addWidget(m_title);titleBox->addWidget(m_subtitle);
    heading->addLayout(titleBox,1);
    m_tools=new CommandToolButton(this);
    m_tools->setObjectName(QStringLiteral("secondaryTool"));
    m_tools->setIcon(VectorIcons::icon(QStringLiteral("wrench"),QColor("#10AAA0"),18));
    m_tools->setIconSize(QSize(18,18));
    m_tools->setText(QStringLiteral("Tools"));
    m_tools->setToolButtonStyle(Qt::ToolButtonTextBesideIcon);
    m_tools->setCursor(Qt::PointingHandCursor);
    heading->addWidget(m_tools);
    connect(m_tools,&QToolButton::clicked,this,&DriverWorkspace::utilitiesRequested);
    root->addLayout(heading);

    // One primary action: Scan -> Install (N) when a demo selection exists.
    // Everything else lives in quieter secondary controls below the hero.
    QWidget *bar=new SurfaceCard(this);
    bar->setObjectName(QStringLiteral("softSurface"));
    m_controlBar=bar;
    QHBoxLayout *hero=new QHBoxLayout(bar);
    hero->setContentsMargins(18,13,18,13);
    hero->setSpacing(28);
    m_scan=new GradientButton(QStringLiteral("Scan this computer"),bar);
    m_scan->setMinimumHeight(70);
    m_scan->setMinimumWidth(285);
    m_scan->setMaximumWidth(590);
    m_scan->setSizePolicy(QSizePolicy::Expanding,QSizePolicy::Fixed);
    hero->addWidget(m_scan,3);

    QWidget *optionBox=new QWidget(bar);
    QVBoxLayout *toggleOptions=new QVBoxLayout(optionBox);
    toggleOptions->setContentsMargins(0,0,0,0);
    toggleOptions->setSpacing(8);
    QHBoxLayout *installedLine=new QHBoxLayout(); installedLine->setSpacing(11);
    m_showInstalled=new ToggleSwitch(optionBox);
    m_showInstalled->setChecked(true);m_showInstalled->setPosition(1.0);
    m_installedLabel=label(QStringLiteral("View installed and alternative drivers"),"subText",optionBox);
    installedLine->addWidget(m_showInstalled);installedLine->addWidget(m_installedLabel,1);
    toggleOptions->addLayout(installedLine);
    QHBoxLayout *moreLine=new QHBoxLayout();moreLine->setSpacing(11);
    m_showMore=new ToggleSwitch(optionBox);
    m_moreLabel=label(QStringLiteral("View additional information"),"subText",optionBox);
    moreLine->addWidget(m_showMore);moreLine->addWidget(m_moreLabel,1);
    toggleOptions->addLayout(moreLine);
    optionBox->setMinimumWidth(240);
    hero->addWidget(optionBox,2);
    root->addWidget(bar);

    // Lightweight status belongs to the heading, not another full-width row.
    m_info=label(QStringLiteral("DEMO — matches are illustrative. No changes to your system."),"demoText",this);
    m_info->setWordWrap(true);
    titleBox->addWidget(m_info);
    m_reinstallTimer=new QTimer(this);m_reinstallTimer->setObjectName(QStringLiteral("reinstallNoticeTimer"));m_reinstallTimer->setSingleShot(true);m_reinstallTimer->setInterval(3000);connect(m_reinstallTimer,&QTimer::timeout,this,[this](){m_reinstallNotice->hide();});
    m_reinstallNotice=new ReinstallNotice(this);m_reinstallNotice->setObjectName(QStringLiteral("reinstallNotice"));
    QHBoxLayout *noticeLayout=new QHBoxLayout(m_reinstallNotice);noticeLayout->setContentsMargins(16,12,16,12);noticeLayout->setSpacing(12);
    QLabel *noticeIcon=new QLabel(m_reinstallNotice);noticeIcon->setPixmap(VectorIcons::icon(QStringLiteral("shield"),QColor("#B78A35"),24).pixmap(24,24));noticeLayout->addWidget(noticeIcon,0,Qt::AlignVCenter);
    m_reinstallText=label(QString(),"reinstallNoticeText",m_reinstallNotice);m_reinstallText->setWordWrap(true);m_reinstallText->setTextFormat(Qt::PlainText);noticeLayout->addWidget(m_reinstallText,1);m_reinstallNotice->hide();root->insertWidget(1,m_reinstallNotice);

    m_packageNotice=new SurfaceCard(this);m_packageNotice->setObjectName(QStringLiteral("packageUpdateNotice"));m_packageNotice->setProperty("tone",QStringLiteral("violet"));QHBoxLayout *packageLine=new QHBoxLayout(m_packageNotice);packageLine->setContentsMargins(16,10,16,12);packageLine->setSpacing(12);
    QLabel *packageIcon=new QLabel(m_packageNotice);packageIcon->setPixmap(VectorIcons::pixmap(QStringLiteral("download"),QColor("#08A99D"),24));packageLine->addWidget(packageIcon);QVBoxLayout *packageCopy=new QVBoxLayout;packageCopy->setSpacing(3);m_packageTitle=label(QString(),"sectionTitle",m_packageNotice);m_packageDetail=label(QString(),"subText",m_packageNotice);m_packageDetail->setWordWrap(true);m_packageTitle->setProperty("skipTranslation",true);m_packageDetail->setProperty("skipTranslation",true);packageCopy->addWidget(m_packageTitle);packageCopy->addWidget(m_packageDetail);packageLine->addLayout(packageCopy,1);
    m_packageCheck=actionButton(UiLanguage::text(QStringLiteral("Check updates")),"outlineButton",m_packageNotice);m_packageCheck->setObjectName(QStringLiteral("checkPackageUpdates"));m_packageCheck->setIcon(VectorIcons::icon(QStringLiteral("refresh"),QColor("#08A99D"),18));m_packageBrowse=actionButton(UiLanguage::text(QStringLiteral("View updates")),"primaryButton",m_packageNotice);m_packageBrowse->setObjectName(QStringLiteral("viewPackageUpdates"));m_packageBrowse->setProperty("primaryAction",true);packageLine->addWidget(m_packageCheck);packageLine->addWidget(m_packageBrowse);root->insertWidget(1,m_packageNotice);
    connect(m_packageCheck,&QPushButton::clicked,this,&DriverWorkspace::packageUpdateCheckRequested);connect(m_packageBrowse,&QPushButton::clicked,this,&DriverWorkspace::packageUpdatesRequested);refreshPackageNotice();

    // A single summary/action strip: no competing control panels around the hero.
    QWidget *summary=new QWidget(this);
    summary->setObjectName(QStringLiteral("driverSummaryStrip"));
    QVBoxLayout *summaryLayout=new QVBoxLayout(summary);
    summaryLayout->setContentsMargins(0,3,0,2);
    summaryLayout->setSpacing(9);
    QHBoxLayout *summaryRow=new QHBoxLayout();
    summaryRow->setContentsMargins(0,0,0,0);
    summaryRow->setSpacing(10);
    summaryLayout->addLayout(summaryRow);
    QHBoxLayout *summaryActions=new QHBoxLayout();
    summaryActions->setContentsMargins(4,0,4,0);
    summaryActions->setSpacing(12);

    m_stats=label(QStringLiteral("0 devices"),"driverStats",summary);
    m_stats->setVisible(false); // kept as an accessible text summary for diagnostics
    auto statCard=[summary,summaryRow](const QString &icon,const QString &caption,
                                         const QString &tone,QLabel **number)->void {
        QFrame *card=new SurfaceCard(summary);
        card->setObjectName(QStringLiteral("metricCard"));
        card->setProperty("tone",tone);
        card->setMinimumWidth(132);
        card->setMaximumWidth(176);
        QHBoxLayout *line=new QHBoxLayout(card);
        line->setContentsMargins(12,7,14,7);line->setSpacing(9);
        QLabel *symbol=new QLabel(card);
        symbol->setPixmap(VectorIcons::pixmap(icon,QColor(tone==QStringLiteral("amber")?"#B77921":
            (tone==QStringLiteral("green")?"#008F76":"#1975A7")),20));
        symbol->setFixedSize(24,24);
        line->addWidget(symbol);
        QVBoxLayout *valueBox=new QVBoxLayout();valueBox->setSpacing(1);
        *number=label(QStringLiteral("0"),"metricValue",card);
        QLabel *text=label(caption,"metricCaption",card);
        valueBox->addWidget(*number);valueBox->addWidget(text);
        line->addLayout(valueBox,1);
        summaryRow->addWidget(card);
    };
    statCard(QStringLiteral("computer"),QStringLiteral("Recommended"),QStringLiteral("blue"),&m_devicesNumber);
    statCard(QStringLiteral("alert"),QStringLiteral("Warnings"),QStringLiteral("amber"),&m_warningsNumber);
    statCard(QStringLiteral("check-circle"),QStringLiteral("Selected"),QStringLiteral("green"),&m_selectedNumber);
    summaryRow->addStretch(1);

    // Safety options remain editable before the demo install but live in one menu.
    m_installOptions=new CommandToolButton(summary);
    m_installOptions->setObjectName(QStringLiteral("quietTool"));
    m_installOptions->setText(QStringLiteral("Install options"));
    m_installOptions->setToolButtonStyle(Qt::ToolButtonTextBesideIcon);
    m_installOptions->setIcon(VectorIcons::icon(QStringLiteral("shield"),QColor("#10AAA0"),18));
    m_installPopup=new PremiumPopover(this);
    m_installHeading=m_installPopup->addHeading(QStringLiteral("INSTALL PREFERENCES"));
    m_restore=m_installPopup->addOption(QStringLiteral("Create restore point"),true);
    m_reboot=m_installPopup->addOption(QStringLiteral("Restart automatically if required"),false);
    QPushButton *reinstallHelp=m_installPopup->addLink(QStringLiteral("How to reinstall a driver"));
    connect(reinstallHelp,&QPushButton::clicked,this,[this](){m_installPopup->hide();projectMessage(this,UiLanguage::text(QStringLiteral("Reinstall")),UiLanguage::text(QStringLiteral("To reinstall the same version, expand the device and select its installed version. The exact version must have an eligible local 7z package; Windows' installed-driver information alone is not an installation package. If it is absent, add or download that version's package and scan again. Turn off Recommended view in Match filters to browse installed devices without a recommended update. Create restore point in Install options applies to reinstallation too. Windows System Protection must be enabled; a required restore-point failure stops installation.")),false,m_arabic);});
    connect(m_installOptions,&QToolButton::clicked,this,[this](){
        m_installPopup->toggleAt(m_installOptions);
    });
    summaryActions->addWidget(m_installOptions);
    

    m_all=actionButton(QStringLiteral("Select all"),"textLinkButton",summary);
    m_none=actionButton(QStringLiteral("Select none"),"textLinkButton",summary);
    m_all->setIcon(VectorIcons::icon(QStringLiteral("check-circle"),QColor("#10AAA0"),18));m_none->setIcon(VectorIcons::icon(QStringLiteral("close"),QColor("#10AAA0"),18));
    summaryActions->addWidget(m_all);
    summaryActions->addWidget(m_none);
    summaryRow->addLayout(summaryActions);
    root->addWidget(summary);

    // ONE search/filter toolbar directly above the full-width driver list.


    m_filters=new CommandToolButton(this);
    m_filters->setObjectName(QStringLiteral("secondaryTool"));
    m_filters->setText(QStringLiteral("Match filters"));
    m_filters->setToolButtonStyle(Qt::ToolButtonTextBesideIcon);
    m_filters->setIcon(VectorIcons::icon(QStringLiteral("sliders"),QColor("#10AAA0"),18));
    const QList<QAbstractButton*> commands=QList<QAbstractButton*>()<<m_tools<<m_installOptions<<m_all<<m_none<<m_filters;
    for(QAbstractButton *command:commands){command->setObjectName(QStringLiteral("workspaceCommand"));command->setProperty("workspaceCommand",true);command->setFixedHeight(40);command->setIconSize(QSize(18,18));QFont face=command->font();face.setFamily(UiFont::family());face.setPixelSize(13);face.setWeight(QFont::Medium);command->setFont(face);}
    m_filterPopup=new PremiumPopover(this);
    m_filterViewHeading=m_filterPopup->addHeading(QStringLiteral("VIEW OPTIONS"));
    m_expert=m_filterPopup->addOption(QStringLiteral("Expert mode"),true);
    m_smartView=m_filterPopup->addOption(QStringLiteral("Recommended view"),true);m_smartView->setObjectName(QStringLiteral("recommendedView"));
    m_smartView->setToolTip(UiLanguage::text(QStringLiteral("Show eligible local recommendations and your selections. Turn off to browse every match, including unavailable alternatives.")));
    connect(m_smartView,&QCheckBox::toggled,this,[this](bool checked){if(checked){QSignalBlocker versions(m_filterVersions);m_filterVersions->setChecked(false);}rebuild();});
    m_onlyProblems=m_filterPopup->addOption(QStringLiteral("Only problems"),false);
    m_filterVersions=m_filterPopup->addOption(QStringLiteral("Filter alternative versions"),false);connect(m_filterVersions,&QCheckBox::toggled,this,[this](bool checked){if(checked){QSignalBlocker block(m_smartView);m_smartView->setChecked(false);}rebuild();});
    m_showIgnored=m_filterPopup->addOption(QStringLiteral("Show ignored devices"),false);
    connect(m_showIgnored,&QCheckBox::toggled,this,&DriverWorkspace::rebuild);
    m_filterPopup->addDivider();
    buildFilterMenu();
    m_filterPopup->setCompact();
    connect(m_filters,&QToolButton::clicked,this,[this](){
        m_filterPopup->toggleAt(m_filters);
    });


    QHBoxLayout *filterDetails=new QHBoxLayout();
    filterDetails->setSpacing(9);
    m_listTitle=label(QStringLiteral("Driver versions"),"sectionTitle",this);
    filterDetails->addWidget(m_listTitle);
    filterDetails->addStretch(1);

    filterDetails->addWidget(m_filters);
    m_search=new QLineEdit(this);
    m_search->setObjectName(QStringLiteral("searchField"));
    m_search->setPlaceholderText(QStringLiteral("Search devices..."));
    m_search->setToolTip(QStringLiteral("Search device names, manufacturers, classes or hardware IDs"));
    m_search->setClearButtonEnabled(true);
    m_search->setMinimumWidth(200);
    m_search->setMaximumWidth(380);
    m_search->setSizePolicy(QSizePolicy::Expanding,QSizePolicy::Fixed);
    filterDetails->addWidget(m_search);
    QWidget *listToolbar=new QWidget(this);listToolbar->setObjectName(QStringLiteral("softSurface"));
    filterDetails->setContentsMargins(14,8,14,8);listToolbar->setLayout(filterDetails);
    SurfaceCard *driverCard=new SurfaceCard(this);driverCard->setObjectName(QStringLiteral("driverTableCard"));QVBoxLayout *driverLayout=new QVBoxLayout(driverCard);driverLayout->setContentsMargins(12,12,12,16);driverLayout->setSpacing(8);listToolbar->setObjectName(QStringLiteral("driverTableToolbar"));driverLayout->addWidget(listToolbar);root->addWidget(driverCard,1);

    DriverTree *tree=new DriverTree(this);m_tree=tree;
    tree->prepare=[this](QTreeWidgetItem *item){populateCandidates(item);};
    tree->actions=[this](QTreeWidgetItem *item,const QPoint &point){showActions(item,point);};
    m_tree->setObjectName(QStringLiteral("driverTree"));
    m_tree->setMouseTracking(true);m_tree->setProperty("hoverRow",-1);
    m_tree->setColumnCount(4);m_tree->setHeader(new DriverHeader(m_tree));
    m_tree->setHeaderLabels(QStringList()<<QStringLiteral("Device / installed version")
                                          <<QStringLiteral("Status")<<QStringLiteral("Priority")<<QStringLiteral("Actions"));
    m_tree->setRootIsDecorated(true);
    m_tree->setIndentation(36);
    m_tree->setAlternatingRowColors(false);
    m_tree->setMouseTracking(true);
    m_tree->setUniformRowHeights(false);
    m_tree->setWordWrap(true);
    m_tree->setSelectionMode(QAbstractItemView::SingleSelection);
    m_tree->setVerticalScrollMode(QAbstractItemView::ScrollPerPixel);
    m_tree->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    m_tree->setAnimated(true);
    m_tree->setExpandsOnDoubleClick(false);
    m_tree->header()->setStretchLastSection(false);
    for(int column=1;column<4;++column)m_tree->headerItem()->setTextAlignment(column,Qt::AlignCenter);
    m_tree->header()->setHighlightSections(false);m_tree->headerItem()->setToolTip(2,UiLanguage::text(QStringLiteral("Match priority")));
    m_tree->setItemDelegate(new DriverRowDelegate(m_tree));
    m_tree->header()->setSectionResizeMode(0,QHeaderView::Stretch);
    m_tree->header()->setSectionResizeMode(1,QHeaderView::Fixed);
    m_tree->header()->setSectionResizeMode(2,QHeaderView::Fixed);
    m_tree->header()->setSectionResizeMode(3,QHeaderView::Fixed);
    m_tree->header()->resizeSection(1,164);
    m_tree->header()->resizeSection(2,166);
    m_tree->header()->resizeSection(3,130);
    QTimer::singleShot(0,this,[this](){adjustColumns();});
    m_tree->setMinimumHeight(170);
    driverLayout->addWidget(m_tree,1);

    // An itemChanged signal can originate inside Qt's checkbox delegate. Never
    // delete that item/model index by clearing the tree on the same call stack.
    // Coalesce bulk changes and rebuild after the input event has returned.
    m_catalogRefresh=new QTimer(this);m_catalogRefresh->setSingleShot(true);
    m_catalogRefresh->setInterval(0);
    connect(m_catalogRefresh,&QTimer::timeout,this,&DriverWorkspace::rebuild);
    connect(m_catalog,&DriverCatalog::changed,this,[this](){m_catalogRefresh->start();});
    connect(m_catalog,&DriverCatalog::changed,this,&DriverWorkspace::updateCount);
    connect(m_tree,&QTreeWidget::itemChanged,this,&DriverWorkspace::handleChanged);
    connect(m_tree,&QTreeWidget::itemClicked,this,&DriverWorkspace::handleClick);
    tree->details=[this](QTreeWidgetItem *item){showItemDetails(item,0);};
    m_searchDebounce=new QTimer(this);
    m_searchDebounce->setSingleShot(true);
    m_searchDebounce->setInterval(135);
    connect(m_search,&QLineEdit::textChanged,m_searchDebounce,static_cast<void(QTimer::*)()>(&QTimer::start));
    connect(m_searchDebounce,&QTimer::timeout,this,&DriverWorkspace::rebuild);
    connect(m_tree,&QTreeWidget::itemExpanded,this,&DriverWorkspace::populateCandidates);
    m_tree->setContextMenuPolicy(Qt::CustomContextMenu);
    connect(m_tree,&QWidget::customContextMenuRequested,this,[this](const QPoint &point){
        QTreeWidgetItem *item=m_tree->itemAt(point);if(item)showActions(item,m_tree->viewport()->mapToGlobal(point));
    });
    connect(m_onlyProblems,&QCheckBox::toggled,this,&DriverWorkspace::rebuild);
    connect(m_showInstalled,&ToggleSwitch::toggled,this,&DriverWorkspace::rebuild);
    connect(m_showMore,&ToggleSwitch::toggled,this,&DriverWorkspace::rebuild);
    connect(m_expert,&QCheckBox::toggled,this,&DriverWorkspace::rebuild);
    connect(m_all,&QPushButton::clicked,this,[this](){m_catalog->selectAll(true);});
    connect(m_none,&QPushButton::clicked,this,[this](){m_catalog->selectAll(false);});
    connect(m_scan,&QPushButton::clicked,this,[this](){
        if(m_busy)return;
        if((m_demo || m_catalog->nativeInstallEnabled()) && m_catalog->selectedDrivers()>0) emit installationRequested();
        else emit scanRequested();
    });
    connect(m_restore,&QCheckBox::toggled,this,&DriverWorkspace::updateCount);
    connect(m_reboot,&QCheckBox::toggled,this,&DriverWorkspace::updateCount);
    updateCount();
}

void DriverWorkspace::buildFilterMenu()
{
    // Native popover with generous hit targets, no platform QMenu outline.
    m_filterFoundHeading=m_filterPopup->addHeading(QStringLiteral("LOCAL PACKAGE MATCHES"));
    const QStringList found=QStringList()<<QStringLiteral("Not installed")<<QStringLiteral("Newer")
      <<QStringLiteral("Current")<<QStringLiteral("Older")<<QStringLiteral("Better match")<<QStringLiteral("Worse match");
    for(int i=0;i<found.size();++i){
        QCheckBox *option=m_filterPopup->addOption(found.at(i),i==0 || i==1 || i==4);
        m_matchFilters.append(option);
        connect(option,&QCheckBox::toggled,this,[this](){QSignalBlocker smart(m_smartView),versions(m_filterVersions);m_smartView->setChecked(false);m_filterVersions->setChecked(true);rebuild();});
    }
    m_filterPopup->addDivider();
    m_filterAbsentHeading=m_filterPopup->addHeading(QStringLiteral("NO LOCAL PACKAGE MATCH"));
    const QStringList absent=QStringList()<<QStringLiteral("Not installed")<<QStringLiteral("Unknown")<<QStringLiteral("Standard");
    for(int i=0;i<absent.size();++i){
        QCheckBox *option=m_filterPopup->addOption(absent.at(i),i==0);
        m_matchFilters.append(option);
        connect(option,&QCheckBox::toggled,this,[this](){QSignalBlocker smart(m_smartView),versions(m_filterVersions);m_smartView->setChecked(false);m_filterVersions->setChecked(true);rebuild();});
    }
    m_filterPopup->addDivider();
    m_filterPopup->addHeading(QStringLiteral("ADVANCED MATCH VISIBILITY"));
    const QStringList advanced=QStringList()<<QStringLiteral("Best match only")<<QStringLiteral("Show duplicates")<<QStringLiteral("Show invalid matches");
    for(int i=0;i<advanced.size();++i){QCheckBox *option=m_filterPopup->addOption(advanced[i],i==0);m_matchFilters.append(option);connect(option,&QCheckBox::toggled,this,[this](){QSignalBlocker smart(m_smartView),versions(m_filterVersions);m_smartView->setChecked(false);m_filterVersions->setChecked(true);rebuild();});}
    m_filterReset=m_filterPopup->addLink(QStringLiteral("Reset filters"));
    connect(m_filterReset,&QPushButton::clicked,this,[this](){
        {QSignalBlocker smart(m_smartView),versions(m_filterVersions);m_smartView->setChecked(true);m_filterVersions->setChecked(false);}
        for(int i=0;i<m_matchFilters.size();++i){
            QSignalBlocker block(m_matchFilters.at(i));
            m_matchFilters.at(i)->setChecked(i==0 || i==1 || i==4 || i==6 || i==9);
        }
        rebuild();
    });
}

bool DriverWorkspace::matchesCandidate(const DriverCandidate &candidate) const
{
    if(candidate.filterVisibility.size()==64){
        unsigned mask=0;for(int i=0;i<m_matchFilters.size();++i)if(m_matchFilters[i]->isChecked())mask|=1u<<i;
        return (candidate.filterVisibility[mask/64]&(quint64(1)<<(mask%64)))!=0;
    }
    if(candidate.filterVisibility.size()==8){
        unsigned mask=0;
        for(int i=0;i<qMin(9,m_matchFilters.size());++i)if(m_matchFilters.at(i)->isChecked())mask|=1u<<i;
        return (candidate.filterVisibility.at(mask/64)&(quint64(1)<<(mask%64)))!=0;
    }
    // Preserve the engine's status bits for both parent visibility and children.
    const quint32 flags[]={0x10,0x20,0x40,0x80,0x1,0x4};
    for(int k=0;k<6;++k)
        if((candidate.statusBits&flags[k]) && m_matchFilters.at(k)->isChecked())return true;
    return false;
}
bool DriverWorkspace::matchesFilters(int index) const
{
    const DriverEntry &entry=m_catalog->entryRef(index);
    if(m_catalog->isIgnored(index) && !m_showIgnored->isChecked())return false;
    const bool matched=entry.candidates.size()>1;
    if(m_onlyProblems->isChecked() && !entry.device.hasProblem && !(entry.engineDeviceStatus&0x100))return false;
    if(!m_showInstalled->isChecked() && !matched && !entry.device.hasProblem)return false;
    const QString q=m_search->text().trimmed();
    if(!q.isEmpty()){
        const QString text=entry.device.name+QLatin1Char(' ')+entry.device.manufacturer+QLatin1Char(' ')+
            entry.device.deviceClass+QLatin1Char(' ')+entry.device.hardwareId;
        if(!text.contains(q,Qt::CaseInsensitive))return false;
    }
    if(m_demo && matched)return entry.device.hasProblem?m_matchFilters.at(0)->isChecked():
        (m_matchFilters.at(1)->isChecked() || m_matchFilters.at(4)->isChecked());
    if(m_catalog->native()){
        if(m_smartView->isChecked()){
            if(entry.checked)return true;
            for(int c=1;c<entry.candidates.size();++c){const DriverCandidate &candidate=entry.candidates[c];if(candidate.valid && candidate.local && candidate.recommended)return true;}
            return false;
        }
        if(!m_filterVersions->isChecked())return true;
        for(int c=1;c<entry.candidates.size();++c)
            if(matchesCandidate(entry.candidates[c]))return true;
        if(matched)return false;
        const quint32 absent[]={0x100,0x200,0x400};
        for(int k=0;k<3;++k)if((entry.engineDeviceStatus&absent[k]) && m_matchFilters.at(k+6)->isChecked())return true;
        return entry.engineDeviceStatus==0;
    }
    return true;
}

void DriverWorkspace::rebuild()
{
    if (m_rebuilding) return;
    m_rebuilding=true;
    QSignalBlocker blocked(m_tree);
    QSet<int> expanded;QSet<QString> collapsedCategories;
    for(QTreeWidgetItemIterator it(m_tree);*it;++it){QTreeWidgetItem *item=*it;if(item->data(0,Qt::UserRole+13).toBool()){if(!item->isExpanded())collapsedCategories.insert(item->data(0,Qt::UserRole+14).toString());}else if(item->data(0,Qt::UserRole+1).toInt()==-1 && item->isExpanded())expanded.insert(item->data(0,Qt::UserRole).toInt());}
    const int scroll=m_tree->verticalScrollBar()->value();
    const bool animated=m_tree->isAnimated();
    m_tree->setAnimated(false);
    m_tree->setUpdatesEnabled(false);
    m_tree->clear();
    QMap<QString,QTreeWidgetItem*> categories;
    QMap<QString,QIcon> rowIcons; // avoid rasterizing the same class icon hundreds of times
    const QStringList categoryOrder=QStringList()<<QStringLiteral("Graphics")<<QStringLiteral("Chipset and system")<<QStringLiteral("Audio")<<QStringLiteral("Network")<<QStringLiteral("Storage")<<QStringLiteral("Bluetooth")<<QStringLiteral("USB")<<QStringLiteral("Input devices")<<QStringLiteral("Cameras and biometrics")<<QStringLiteral("Other devices");
    QVector<int> orderedRows;for(int row=0;row<m_catalog->size();++row)orderedRows.append(row);std::stable_sort(orderedRows.begin(),orderedRows.end(),[this,categoryOrder](int a,int b){const DeviceInfo &x=m_catalog->entryRef(a).device,&y=m_catalog->entryRef(b).device;return categoryOrder.indexOf(VectorIcons::deviceCategory(x.deviceClass,x.name))<categoryOrder.indexOf(VectorIcons::deviceCategory(y.deviceClass,y.name));});
    for (int row:orderedRows) {
        if (!matchesFilters(row)) continue;
        const DriverEntry &d=m_catalog->entryRef(row);
        const QString category=VectorIcons::deviceCategory(d.device.deviceClass,d.device.name);
        if(!categories.contains(category)){QTreeWidgetItem *group=new QTreeWidgetItem(m_tree);group->setData(0,Qt::UserRole,-10-categories.size());group->setData(0,Qt::UserRole+1,-2);group->setData(0,Qt::UserRole+13,true);group->setData(0,Qt::UserRole+14,category);group->setFlags(Qt::ItemIsEnabled);group->setSizeHint(0,QSize(0,64));group->setFirstColumnSpanned(true);categories.insert(category,group);}
        QTreeWidgetItem *root=new QTreeWidgetItem(categories.value(category));
        root->setData(0,Qt::UserRole,row);
        root->setData(0,Qt::UserRole+1,-1);
        QString rowText=d.device.name+QStringLiteral("\n")+
                d.device.manufacturer+QStringLiteral("  ·  ")+d.device.deviceClass+
                QStringLiteral("\n")+(d.device.driverVersion.isEmpty()?
                   UiLanguage::text(QStringLiteral("Installed version unavailable")):
                   UiLanguage::text(QStringLiteral("Installed"))+QStringLiteral(": ")+d.device.driverVersion);
        if(m_showMore->isChecked() && !d.device.hardwareId.isEmpty())
            rowText+=QStringLiteral("\n")+d.device.hardwareId;
        root->setText(0,rowText);
        root->setToolTip(0,d.device.name+QStringLiteral("\n")+d.device.hardwareId+
                            QStringLiteral("\nDouble-click for details"));
        const QString iconName=VectorIcons::forDeviceClass(d.device.deviceClass+QStringLiteral(" ")+d.device.name);
        const QString iconKey=iconName+(d.device.hasProblem?QStringLiteral("-warning"):QStringLiteral("-normal"));
        if(!rowIcons.contains(iconKey))
            rowIcons.insert(iconKey,VectorIcons::icon(iconName,d.device.hasProblem?QColor("#C88935"):QColor("#22A5D9"),32));
        root->setIcon(0,rowIcons.value(iconKey));
        const QString status=driverStatus(d);
        const auto statusText=[this](const QString &code){if(!m_arabic)return code;if(code==QStringLiteral("Installed"))return QString::fromUtf8("مثبّت");if(code==QStringLiteral("Missing"))return QString::fromUtf8("مفقود");if(code==QStringLiteral("Upgrade"))return QString::fromUtf8("ترقية");if(code==QStringLiteral("Downgrade"))return QString::fromUtf8("إصدار أقدم");if(code==QStringLiteral("Invalid"))return QString::fromUtf8("غير صالح");return code;};
        root->setText(1,UiLanguage::text(statusText(status)));root->setData(1,Qt::UserRole+6,status);
        if(m_catalog->isIgnored(row)){root->setText(1,m_arabic?QString::fromUtf8("متجاهل"):QStringLiteral("Ignored"));root->setData(1,Qt::UserRole+6,QStringLiteral("Ignored"));}
        int rated=d.chosenCandidate;
        if(rated<=0 || rated>=d.candidates.size()){
            rated=-1;
            for(int i=1;i<d.candidates.size();++i)if(d.candidates[i].valid && (rated<=0 || d.candidates[i].originalOrder<d.candidates[rated].originalOrder))rated=i;
        }
        const int stars=m_demo?5:(rated>0 && rated<d.candidates.size()?d.candidates[rated].matchStars:0);
        root->setText(2,m_demo?UiLanguage::text(QStringLiteral("DEMO priority")):
            (stars?UiLanguage::text(QStringLiteral("%1 versions")).arg(qMax(0,d.candidates.size()-1)):UiLanguage::text(QStringLiteral("No valid match"))));
        root->setData(2,Qt::UserRole+2,stars);
        root->setToolTip(2,m_arabic?QString::fromUtf8("أولوية نسبية من ترتيب Origin بين البدائل الصالحة للجهاز. خمس نجوم للأعلى أولوية. ليست تقييم أمان أو جودة. يتبع الصف الإصدار المختار."):
            QStringLiteral("Relative priority from the unfiltered valid Origin order for this device. Five stars = highest priority. Not a quality or security rating. The device row follows the selected version."));
        root->setText(3,UiLanguage::text(d.checked?QStringLiteral("Selected"):QStringLiteral("Select")));
        if (m_catalog->canQueue(row)) root->setCheckState(3,d.checked?Qt::Checked:Qt::Unchecked);
        else root->setText(3,UiLanguage::text(QStringLiteral("Details")));
        root->setData(3,Qt::UserRole+12,!m_catalog->canQueue(row));
        const QColor subdued=m_dark?QColor("#A8C1CD"):QColor("#738897");
        root->setForeground(2,subdued);
        root->setForeground(1,d.device.hasProblem?QColor("#C68B42"):(m_demo?QColor("#08A695"):subdued));
        QFont parentFont=root->font(0);parentFont.setPointSize(10);parentFont.setWeight(QFont::DemiBold);
        root->setFont(0,parentFont);
        root->setSizeHint(0,QSize(0,m_showMore->isChecked()?115:96));
        root->setToolTip(3,UiLanguage::text(QStringLiteral("Double-click for comparison and hardware details")));
        for(int column=1;column<4;++column)root->setText(column,UiLanguage::text(root->text(column)));
        if(m_expert->isChecked() && !d.candidates.isEmpty()) {
            root->setChildIndicatorPolicy(QTreeWidgetItem::ShowIndicator);
            if(expanded.contains(row)){
                populateCandidates(root);
                root->setExpanded(true);
            }
        }

    }
    for(auto it=categories.begin();it!=categories.end();++it){it.value()->setText(0,UiLanguage::text(it.key())+QStringLiteral("  ·  ")+QString::number(it.value()->childCount()));it.value()->setExpanded(!collapsedCategories.contains(it.key()));}
    m_rebuilding=false;
    m_tree->setUpdatesEnabled(true);
    updateCount();
    // Clearing the model resets the scrollbar range. Measure the rebuilt rows
    // and notice layout before restoring the position, otherwise Qt clamps it
    // to zero while the new scroll range is still pending.
    layout()->activate();
    m_tree->doItemsLayout();
    m_tree->verticalScrollBar()->setValue(scroll);
    m_tree->setAnimated(animated);
}

void DriverWorkspace::populateCandidates(QTreeWidgetItem *root)
{
    if(!root || root->data(0,Qt::UserRole+1).toInt()!=-1 || !m_expert->isChecked() || root->data(0,Qt::UserRole+4).toBool())return;
    const int row=root->data(0,Qt::UserRole).toInt();
    if(row<0 || row>=m_catalog->size())return;
    const DriverEntry &d=m_catalog->entryRef(row);
    QSignalBlocker blockedSignals(m_tree);
    m_tree->setUpdatesEnabled(false);
    const QColor subdued=m_dark?QColor("#A8C1CD"):QColor("#738897");
    for(int c=0;c<d.candidates.size();++c) {
        const DriverCandidate &candidate=d.candidates.at(c);
                if(c>0 && c==reinstallCandidateIndex(d))continue;
                if(m_catalog->native() && c>0 && m_smartView->isChecked() && !(d.checked && c==d.chosenCandidate) && !(candidate.valid && candidate.local && candidate.recommended))continue;
                if(m_catalog->native() && c>0 && m_filterVersions->isChecked() && !(d.checked && c==d.chosenCandidate) && !matchesCandidate(candidate))continue;
                if(m_catalog->native() && c==0 && !d.sdioDriverPresent)continue;
                if (c==0 && !m_showInstalled->isChecked()) continue;
                if (m_demo) {
                    if(c==0 && !m_matchFilters.at(2)->isChecked())continue;
                    if(c==1 && !m_matchFilters.at(1)->isChecked() && !m_matchFilters.at(4)->isChecked())continue;
                    if(c>=2 && !m_matchFilters.at(3)->isChecked() && !m_matchFilters.at(5)->isChecked())continue;
                }
                QTreeWidgetItem *child=new QTreeWidgetItem(root);
                child->setData(0,Qt::UserRole,row);
                child->setData(0,Qt::UserRole+1,c);
                child->setText(0,candidate.version+QStringLiteral("  /  ")+candidate.date);
                child->setData(0,Qt::UserRole+2,c==0?(d.checked && d.chosenCandidate==reinstallCandidateIndex(d)):(d.checked && c==d.chosenCandidate));
                child->setData(0,Qt::UserRole+3,candidate.hardwareId);
                const QString status=driverStatus(d,c);QString translated=status;
                if(m_arabic){if(status==QStringLiteral("Installed"))translated=QString::fromUtf8("مثبّت");else if(status==QStringLiteral("Missing"))translated=QString::fromUtf8("مفقود");else if(status==QStringLiteral("Upgrade"))translated=QString::fromUtf8("ترقية");else if(status==QStringLiteral("Downgrade"))translated=QString::fromUtf8("إصدار أقدم");else if(status==QStringLiteral("Invalid"))translated=QString::fromUtf8("غير صالح");}
                child->setText(1,UiLanguage::text(translated));child->setData(1,Qt::UserRole+6,status);
                child->setText(2,m_demo?QStringLiteral("DEMO priority"):
                    (c==0?QStringLiteral("Installed"):(candidate.matchStars?QStringLiteral("Origin #%1").arg(candidate.originalOrder):QStringLiteral("Ineligible"))));
                child->setData(2,Qt::UserRole+2,candidate.matchStars);
                child->setText(3,UiLanguage::text(c==0?(d.checked && isReinstallCandidate(d,d.chosenCandidate)?QStringLiteral("Selected"):reinstallCandidateIndex(d)>0?QStringLiteral("Reinstall"):QStringLiteral("Installed")):
                    (!m_demo && !candidate.valid?QStringLiteral("Ineligible"):
                     (!m_demo && !candidate.local?QStringLiteral("Package missing"):
                      (!m_demo && !m_catalog->nativeInstallEnabled()?QStringLiteral("Read only"):
                       (c==d.chosenCandidate && d.checked?QStringLiteral("Selected"):QStringLiteral("Choose")))))));
                child->setData(3,Qt::UserRole+12,m_catalog->canQueue(row) && (c==0?reinstallCandidateIndex(d)>0:(m_demo || (candidate.valid && candidate.local))));
                child->setToolTip(2,candidate.matchReason);
                child->setForeground(1,candidate.status==QStringLiteral("recommended")?QColor("#2BAE6B"):subdued);
                child->setForeground(2,candidate.matchStars?QColor("#D49B38"):subdued);
                for(int column=1;column<4;++column)child->setText(column,UiLanguage::text(child->text(column)));
                child->setSizeHint(0,QSize(0,78));
                child->setToolTip(0,QStringLiteral("%1\n%2\n%3").arg(candidate.version,candidate.hardwareId,candidate.packageSource));
    }
    root->setData(0,Qt::UserRole+4,true);
    m_tree->setUpdatesEnabled(true);
}

void DriverWorkspace::handleChanged(QTreeWidgetItem *item,int column)
{
    if(m_rebuilding || m_busy || !item || item->data(0,Qt::UserRole+1).toInt()!=-1 || column!=3) return;
    const int row=item->data(0,Qt::UserRole).toInt();
    if (!m_catalog->canQueue(row)) return;
    m_catalog->setSelected(row,item->checkState(3)==Qt::Checked);
}
void DriverWorkspace::handleClick(QTreeWidgetItem *item,int column)
{
    if (!item || m_rebuilding || m_busy) return;
    if(item->data(0,Qt::UserRole+13).toBool()){if(column==0)item->setExpanded(!item->isExpanded());return;}
    const int row=rowOf(item);
    const int candidate=item->data(0,Qt::UserRole+1).toInt();
    if(candidate>=0 && item->parent() && m_catalog->canQueue(row)){
        const DriverEntry &entry=m_catalog->entryRef(row);m_clickRow=row;m_clickCandidate=candidate;m_clickPriorChoice=entry.chosenCandidate;m_clickPriorChecked=entry.checked;m_clickTime.start();const int resolved=candidate==0?reinstallCandidateIndex(entry):candidate;
        if(resolved>0 && entry.checked && entry.chosenCandidate==resolved)m_catalog->setSelected(row,false);
        else if(!m_catalog->chooseCandidate(row,candidate) && candidate==0)projectMessage(this,UiLanguage::text(QStringLiteral("Reinstall"),QString::fromUtf8("إعادة تثبيت")),UiLanguage::text(QStringLiteral("Reinstall requires a matching local driver package. Download or add that package, then scan again."),QString::fromUtf8("إعادة التثبيت تتطلب حزمة تعريف محلية مطابقة. أضف الحزمة أو نزّلها، ثم أعد الفحص.")),false,m_arabic);
    }else if (candidate<0 && column==0){
        populateCandidates(item);
        item->setExpanded(!item->isExpanded());
    }else if (candidate<0 && column==3 && !m_catalog->canQueue(row)) {
        showEntry(row);
    }
}
void DriverWorkspace::showItemDetails(QTreeWidgetItem *item,int)
{
    if(!item)return;const int row=rowOf(item),candidate=item->data(0,Qt::UserRole+1).toInt();
    if(row==m_clickRow && candidate==m_clickCandidate && m_clickTime.isValid() && m_clickTime.elapsed()<=QApplication::doubleClickInterval()){
        if(m_clickPriorChoice>0)m_catalog->chooseCandidate(row,m_clickPriorChoice);
        if(!m_clickPriorChecked)m_catalog->setSelected(row,false);
    }
    m_clickRow=-1;showEntry(row);
}
int DriverWorkspace::rowOf(QTreeWidgetItem *item) const
{
    if (!item) return -1;
    return item->data(0,Qt::UserRole).toInt();
}
void DriverWorkspace::showEntry(int row)
{
    if(row<0 || row>=m_catalog->size()) return;
    DriverDetailsDialog dialog(m_catalog->entry(row),m_demo,m_arabic,this);
    dialog.exec();
}
void DriverWorkspace::updateCount()
{
    refreshPackageNotice();
    const int selected=m_catalog->selectedDrivers();
    const int reinstalls=m_catalog->selectedReinstalls();
    QString signature;for(int row=0;row<m_catalog->size();++row){const DriverEntry &entry=m_catalog->entryRef(row);if(entry.checked && isReinstallCandidate(entry,entry.chosenCandidate))signature+=QString::number(row)+QLatin1Char(':')+QString::number(entry.chosenCandidate)+QLatin1Char(';');}if(!signature.isEmpty())signature+=m_restore->isChecked()?QLatin1Char('1'):QLatin1Char('0');
    if(signature!=m_reinstallSignature){m_reinstallSignature=signature;m_reinstallTimer->stop();m_reinstallNotice->setVisible(!signature.isEmpty() && !m_busy);if(!signature.isEmpty() && !m_busy)m_reinstallTimer->start();}
    if(reinstalls==0 || m_busy){m_reinstallTimer->stop();m_reinstallNotice->hide();}
    if(reinstalls>0){
        const QString heading=UiLanguage::text(QStringLiteral("Reinstallation selected: %1"),QString::fromUtf8("إعادة تثبيت: %1")).arg(reinstalls);
        const QString protection=UiLanguage::text(m_restore->isChecked()?QStringLiteral("A restore point is required before reinstalling. If creation fails, installation stops."):QStringLiteral("Restore point is off. Enable it in Install options before continuing."));
        m_reinstallText->setText(heading+QLatin1Char('\n')+protection);
    }

    int recommendedCount=0;
    for(int row=0;row<m_catalog->size();++row){if(m_catalog->isIgnored(row))continue;const DriverEntry &entry=m_catalog->entryRef(row);bool recommended=m_demo && entry.selectable;for(const DriverCandidate &candidate:entry.candidates)if(candidate.recommended && candidate.valid && candidate.local)recommended=true;if(recommended)++recommendedCount;}
    m_devicesNumber->setText(QString::number(recommendedCount));
    m_warningsNumber->setText(QString::number(m_catalog->problems()));
    m_selectedNumber->setText(QString::number(selected));
    const int count=selected;
    m_stats->setText(m_arabic?QString::fromUtf8("%1 جهاز   ·   %2 تحذير   ·   %3 محدد")
          .arg(m_catalog->size()).arg(m_catalog->problems()).arg(selected)
        :QStringLiteral("%1 devices   ·   %2 warnings   ·   %3 selected")
          .arg(m_catalog->size()).arg(m_catalog->problems()).arg(selected));
    if(!m_demo && m_catalog->sdioVerifiedCount()>0)
        m_stats->setText(m_stats->text()+
            (m_arabic?QString::fromUtf8("   ·   %1 موثّق من SDIO").arg(m_catalog->sdioVerifiedCount()):
             QStringLiteral("   ·   %1 SDIO verified").arg(m_catalog->sdioVerifiedCount())));
    const QString countHint=m_arabic?QString::fromUtf8("عدد الأجهزة التي لها إصدار موصى به وفق المطابقة. بعض الحزم قد تحتاج تنزيلًا. عدد كل مكونات Windows موجود في التشخيص."):QStringLiteral("Devices with an engine-recommended match. Some packages may need downloading. The complete Windows component count is shown in Diagnostics.");
    m_devicesNumber->setToolTip(countHint);m_devicesNumber->parentWidget()->setToolTip(countHint);
    if(QLabel *caption=m_devicesNumber->parentWidget()->findChild<QLabel*>(QStringLiteral("metricCaption")))caption->setText(m_arabic?QString::fromUtf8("ترشيحات"):QStringLiteral("Recommended"));
    m_warningsNumber->setToolTip(m_arabic?QString::fromUtf8("أجهزة بها مشكلات"):QStringLiteral("PnP warnings"));
    m_selectedNumber->setToolTip(m_arabic?QString::fromUtf8("التعريفات المحددة"):QStringLiteral("Selected drivers"));
    if(QLabel *caption=m_warningsNumber->parentWidget()->findChild<QLabel*>(QStringLiteral("metricCaption")))caption->setText(m_arabic?QString::fromUtf8("تحذيرات"):QStringLiteral("Warnings"));
    if(QLabel *caption=m_selectedNumber->parentWidget()->findChild<QLabel*>(QStringLiteral("metricCaption")))caption->setText(m_arabic?QString::fromUtf8("محدد"):QStringLiteral("Selected"));
    m_installOptions->setText(m_arabic ? QString::fromUtf8("خيارات التثبيت") : QStringLiteral("Install options"));
    m_installOptions->setToolTip(m_arabic?
        QString::fromUtf8("نقطة الاستعادة: %1 · إعادة التشغيل: %2").arg(m_restore->isChecked()?QString::fromUtf8("نعم"):QString::fromUtf8("لا"),m_reboot->isChecked()?QString::fromUtf8("نعم"):QString::fromUtf8("لا")):
        QStringLiteral("Restore point: %1 · Reboot: %2").arg(m_restore->isChecked()?QStringLiteral("On"):QStringLiteral("Off"),m_reboot->isChecked()?QStringLiteral("On"):QStringLiteral("Off")));
    const bool canInstall=m_demo || m_catalog->nativeInstallEnabled();
    const bool installMode=canInstall && selected>0;
    m_scan->setActionState(m_busy?(m_arabic?QString::fromUtf8("جارٍ فحص الأجهزة..."):QStringLiteral("Scanning devices...")):
       installMode?(m_arabic?QString::fromUtf8("تثبيت (%1)").arg(count):QStringLiteral("Install (%1)").arg(count)):
       (m_arabic?QString::fromUtf8("فحص هذا الكمبيوتر"):QStringLiteral("Scan this computer")),
       installMode?QStringLiteral("install"):QStringLiteral("scan"));
    m_scan->setToolTip(installMode?
      (m_arabic?QString::fromUtf8("%1 تعريفات محددة، وخطوة الاستعادة %2. محاكاة فقط.").arg(selected).arg(m_restore->isChecked()?QString::fromUtf8("مفعّلة"):QString::fromUtf8("غير مفعّلة")):
       QStringLiteral("%1 driver(s) selected; restore point %2. Demo simulation only.").arg(selected).arg(m_restore->isChecked()?QStringLiteral("on"):QStringLiteral("off"))):
      (m_arabic?QString::fromUtf8("فحص الأجهزة؛ لن يتم تثبيت أي تعريف."):QStringLiteral("Scan Windows devices; no drivers will be installed.")));
    if(!m_demo)m_scan->setToolTip(installMode?
        QStringLiteral("%1 drivers and %2 restore-point task(s). Requires confirmation and native preflight checks.").arg(selected).arg(m_restore->isChecked()?1:0):
        QStringLiteral("Scan using the original in-process Origin matcher. No simulated candidates."));
    m_scan->setEnabled(!m_busy);
    m_all->setToolTip(UiLanguage::text(QStringLiteral("Select every eligible local Recommended driver, including devices hidden by search or filters."),QString::fromUtf8("تحديد كل ترشيحات المحرك المؤهلة والمتوفرة محليًا، بما فيها الأجهزة المخفية بالبحث أو الفلاتر.")));
    m_none->setToolTip(UiLanguage::text(QStringLiteral("Clear every selection, including hidden devices and manually chosen versions."),QString::fromUtf8("إلغاء كل التحديدات، بما فيها الأجهزة المخفية والإصدارات المختارة يدويًا.")));
    m_all->setEnabled(canInstall && !m_busy);
    m_none->setEnabled(m_catalog->size()>0 && !m_busy);
    m_restore->setEnabled(!m_busy);
    m_reboot->setEnabled(!m_busy);
    m_filters->setEnabled((m_demo || m_catalog->native()) && !m_busy);
    m_tree->setEnabled(!m_busy);
}
void DriverWorkspace::setPackageUpdateStatus(int packages,int indexes,const QString &source,const QString &message,bool checking){m_packageCount=packages;m_packageIndexes=indexes;m_packageSource=source;m_packageMessage=message;m_packageChecking=checking;refreshPackageNotice();}
void DriverWorkspace::refreshPackageNotice(){
    if(!m_packageNotice)return;
    m_packageNotice->setVisible(m_packageAllowed && (!m_demo || m_packageCount>=0) && (m_packageChecking || m_packageCount!=0 || m_packageIndexes!=0 || !m_packageMessage.isEmpty()));
    m_packageTitle->setText(m_packageChecking?UiLanguage::text(QStringLiteral("Checking driver pack updates..."),QString::fromUtf8("جارٍ فحص تحديثات حزم التعريفات...")):m_packageCount>=0?UiLanguage::text(QStringLiteral("Updates available · %1 driver packs · %2 indexes"),QString::fromUtf8("تحديثات متاحة · %1 حزمة تعريفات · %2 فهرس")).arg(m_packageCount).arg(m_packageIndexes):UiLanguage::text(QStringLiteral("Driver pack updates"),QString::fromUtf8("تحديثات حزم التعريفات")));
    m_packageDetail->setText(m_packageMessage.isEmpty()?UiLanguage::text(QStringLiteral("%1 catalog · Review packages before downloading. Installation is a separate step."),QString::fromUtf8("كتالوج %1 · راجع الحزم قبل التنزيل. التثبيت خطوة منفصلة.")).arg(m_packageSource.isEmpty()?QStringLiteral("SDI / SDIO"):m_packageSource):UiLanguage::text(m_packageMessage));
    m_packageCheck->setEnabled(!m_packageChecking && !m_demo);m_packageBrowse->setEnabled(!m_busy && !m_demo);
}
void DriverWorkspace::setStatus(const QString &status)
{ m_info->setText(status);m_info->setVisible(!status.trimmed().isEmpty()); }
void DriverWorkspace::openInstallOptions(){if(!m_busy)m_installOptions->click();}
void DriverWorkspace::setInstallationPreferences(bool restorePoint,bool rebootAfter){QSignalBlocker restoreSignals(m_restore),rebootSignals(m_reboot);m_restore->setChecked(restorePoint);m_reboot->setChecked(rebootAfter);updateCount();}
void DriverWorkspace::setBusy(bool busy)
{ m_busy=busy;updateCount(); }
void DriverWorkspace::setAnimations(bool enabled)
{
    static_cast<DriverTree*>(m_tree)->setMotionEnabled(enabled);
    m_scan->setMotionEnabled(enabled);
    m_showInstalled->setMotionEnabled(enabled);
    m_showMore->setMotionEnabled(enabled);
    m_installPopup->setMotionEnabled(enabled);
    m_filterPopup->setMotionEnabled(enabled);
}
void DriverWorkspace::setDemo(bool demo)
{
    m_demo=demo;
    m_filters->setEnabled(demo || m_catalog->native());
    updateDemoNotice();
    rebuild();
}
void DriverWorkspace::updateDemoNotice()
{
    setStatus(m_demo?(m_arabic?QString::fromUtf8("معاينة ببيانات توضيحية، دون تعديل أي تعريف."):QStringLiteral("Interface preview with sample data. No drivers are changed.")):QString());
}
void DriverWorkspace::setDark(bool dark)
{m_dark=dark;m_showInstalled->setDark(dark);m_showMore->setDark(dark);
 m_installPopup->setDark(dark);m_filterPopup->setDark(dark);rebuild();}
void DriverWorkspace::setArabic(bool arabic)
{
    m_arabic=arabic;
    m_title->setText(arabic?QString::fromUtf8("تعريفات هذا الكمبيوتر"):QStringLiteral("Drivers for this computer"));
    m_subtitle->setText(arabic?QString::fromUtf8("التعريفات المثبتة والبدائل والمقارنة"):QStringLiteral("Installed drivers, alternatives and comparison"));
    m_tools->setText(arabic?QString::fromUtf8("الأدوات"):QStringLiteral("Tools"));
    m_all->setText(arabic?QString::fromUtf8("تحديد الكل"):QStringLiteral("Select all"));
    m_none->setText(arabic?QString::fromUtf8("إلغاء التحديد"):QStringLiteral("Select none"));
    m_installHeading->setText(arabic?QString::fromUtf8("خيارات التثبيت"):QStringLiteral("INSTALL PREFERENCES"));
    m_filterViewHeading->setText(arabic?QString::fromUtf8("خيارات العرض"):QStringLiteral("VIEW OPTIONS"));
    m_filterFoundHeading->setText(arabic?QString::fromUtf8("متوافق مع الحزم المحلية"):QStringLiteral("LOCAL PACKAGE MATCHES"));
    m_filterAbsentHeading->setText(arabic?QString::fromUtf8("لا يوجد تطابق محلي"):QStringLiteral("NO LOCAL PACKAGE MATCH"));
    m_filterReset->setText(arabic?QString::fromUtf8("إعادة ضبط الفلاتر"):QStringLiteral("Reset filters"));
    const QStringList foundAr=QStringList()<<QString::fromUtf8("غير مثبت")<<QString::fromUtf8("أحدث")
      <<QString::fromUtf8("الحالي")<<QString::fromUtf8("أقدم")<<QString::fromUtf8("مطابقة أفضل")<<QString::fromUtf8("مطابقة أضعف")
      <<QString::fromUtf8("غير مثبت")<<QString::fromUtf8("غير معروف")<<QString::fromUtf8("قياسي");
    const QStringList foundEn=QStringList()<<QStringLiteral("Not installed")<<QStringLiteral("Newer")
      <<QStringLiteral("Current")<<QStringLiteral("Older")<<QStringLiteral("Better match")<<QStringLiteral("Worse match")
      <<QStringLiteral("Not installed")<<QStringLiteral("Unknown")<<QStringLiteral("Standard");
    for(int i=0;i<qMin(9,m_matchFilters.size());++i)
        m_matchFilters[i]->setText(arabic?foundAr.at(i):foundEn.at(i));
    m_matchFilters[9]->setText(arabic?QString::fromUtf8("أفضل مطابقة فقط"):QStringLiteral("Best match only"));
    m_matchFilters[10]->setText(arabic?QString::fromUtf8("إظهار المكررات"):QStringLiteral("Show duplicates"));
    m_matchFilters[11]->setText(arabic?QString::fromUtf8("إظهار المطابقات غير الصالحة"):QStringLiteral("Show invalid matches"));
    m_showIgnored->setText(arabic?QString::fromUtf8("إظهار الأجهزة المتجاهلة"):QStringLiteral("Show ignored devices"));
    m_restore->setText(arabic?QString::fromUtf8("إنشاء نقطة استعادة"):QStringLiteral("Create restore point"));
    m_reboot->setText(arabic?QString::fromUtf8("إعادة التشغيل تلقائيًا عند الحاجة"):QStringLiteral("Restart automatically if required"));
    m_onlyProblems->setText(arabic?QString::fromUtf8("المشكلات فقط"):QStringLiteral("Only problems"));
    m_expert->setText(arabic?QString::fromUtf8("الوضع المتقدم"):QStringLiteral("Expert mode"));
    m_filters->setText(arabic?QString::fromUtf8("فلاتر المطابقة"):QStringLiteral("Match filters"));
    m_listTitle->setText(arabic?QString::fromUtf8("إصدارات التعريفات"):QStringLiteral("Driver versions"));
    m_installedLabel->setText(arabic?QString::fromUtf8("إظهار المثبت والتعريفات البديلة"):QStringLiteral("View installed and alternative drivers"));
    m_moreLabel->setText(arabic?QString::fromUtf8("إظهار معلومات إضافية"):QStringLiteral("View additional information"));
    m_search->setPlaceholderText(arabic?QString::fromUtf8("ابحث عن جهاز..."):QStringLiteral("Search devices..."));
    m_tree->setHeaderLabels(arabic?QStringList()<<QString::fromUtf8("الجهاز / التعريف المثبّت")<<QString::fromUtf8("الحالة")<<QString::fromUtf8("أولوية المطابقة")<<QString::fromUtf8("الإجراءات"):
                             QStringList()<<QStringLiteral("Device / installed version")<<QStringLiteral("Status")<<QStringLiteral("Priority")<<QStringLiteral("Actions"));
    rebuild();
    m_tree->doItemsLayout();adjustColumns();m_tree->viewport()->update();
    updateCount();
}
int DriverWorkspace::selectedDrivers() const{return m_catalog->selectedDrivers();}
void DriverWorkspace::applyLegacyDisplay(int filters,bool expert,bool packageNames){
    if(filters>=0){QSignalBlocker smart(m_smartView),versions(m_filterVersions);m_smartView->setChecked(false);m_filterVersions->setChecked(true);}
    if(filters>=0)for(int i=0;i<m_matchFilters.size();++i){QSignalBlocker block(m_matchFilters[i]);m_matchFilters[i]->setChecked((unsigned(filters>>1)&(1u<<i))!=0);}
    if(expert)m_expert->setChecked(true);if(packageNames)m_showMore->setChecked(true);rebuild();
}
int DriverWorkspace::selectedItems() const {const int n=selectedDrivers();return n>0?n+(m_restore->isChecked()?1:0):0;}
QStringList DriverWorkspace::selectedNames() const {return m_catalog->selectedNames();}
bool DriverWorkspace::createRestorePoint() const {return m_restore->isChecked();}
bool DriverWorkspace::rebootAfter() const {return m_reboot->isChecked();}

void DriverWorkspace::resizeEvent(QResizeEvent *event)
{
    QWidget::resizeEvent(event);
    QTimer::singleShot(0,this,[this](){adjustColumns();});
}

void DriverWorkspace::adjustColumns()
{
    if(!m_tree || !m_tree->header())return;
    const int available=m_tree->viewport()->width();
    if(available<150)return;
    // Stretch the device name into ALL spare room; keep other columns proportional.
    // Horizontal scrolling stays disabled for the entire board.
    const int status=qBound(122,static_cast<int>(available*0.17),196);
    const int rating=qBound(140,static_cast<int>(available*0.175),194);
    const int action=qBound(148,static_cast<int>(available*0.15),174);
    m_tree->header()->resizeSection(1,status);
    m_tree->header()->resizeSection(2,rating);
    m_tree->header()->resizeSection(3,action);
}

void DriverWorkspace::applyOriginLanguage(const QString &directory,const QString &code)
{
    m_originLocale=code;
    if(code==QStringLiteral("english") || code==QStringLiteral("arabic"))return;
    const QStringList keys=QStringList()<<QStringLiteral("STR_EXPERT")<<QStringLiteral("STR_SELECT_ALL")
       <<QStringLiteral("STR_SELECT_NONE")<<QStringLiteral("STR_TOOLS")<<QStringLiteral("STR_OPTIONS_BTN");
    const QStringList values=QStringList()<<m_expert->text()<<m_all->text()<<m_none->text()
       <<m_tools->text()<<m_installOptions->text();
    for(int i=0;i<keys.size();++i){
        const QString value=OriginLanguages::text(directory,code,keys.at(i));
        if(value.isEmpty())continue; // UI-specific controls have English/Arabic fallbacks.
        switch(i){
            case 0:m_expert->setText(value);break;
            case 1:m_all->setText(value);break;
            case 2:m_none->setText(value);break;
            case 3:m_tools->setText(value);break;
            case 4:m_installOptions->setText(value);break;
        }
    }
    // Apply native SDIO terminology to the corresponding nine filtering states.
    const QStringList filterKeys=QStringList()<<QStringLiteral("STR_SHOW_MISSING")
       <<QStringLiteral("STR_SHOW_NEWER")<<QStringLiteral("STR_SHOW_CURRENT")
       <<QStringLiteral("STR_SHOW_OLD")<<QStringLiteral("STR_SHOW_BETTER")
       <<QStringLiteral("STR_SHOW_WORSE_RANK")<<QStringLiteral("STR_SHOW_NF_MISSING")
       <<QStringLiteral("STR_SHOW_NF_UNKNOWN")<<QStringLiteral("STR_SHOW_NF_STANDARD");
    for(int i=0;i<filterKeys.size() && i<m_matchFilters.size();++i){
        const QString text=OriginLanguages::text(directory,code,filterKeys.at(i));
        if(!text.isEmpty())m_matchFilters[i]->setText(text);
    }
    const QString installText=OriginLanguages::text(directory,code,QStringLiteral("STR_INSTALL"));
    if(!installText.isEmpty())m_restore->setToolTip(installText);
}

void DriverWorkspace::showActions(QTreeWidgetItem *item,const QPoint &position)
{
    if(!item || m_busy)return;
    const int row=rowOf(item);if(row<0 || row>=m_catalog->size())return;
    const DriverEntry entry=m_catalog->entry(row);
    const int candidate=item->data(0,Qt::UserRole+1).toInt();
    ProjectMenu menu(this);menu.setLayoutDirection(layoutDirection());menu.setObjectName(QStringLiteral("driverActions"));
    const quint64 menuGeneration=m_catalog->nativeSnapshot().generation;
    QAction *details=menu.addAction(m_arabic?QString::fromUtf8("مقارنة التعريفات"):QStringLiteral("Compare drivers"));
    QAction *versions=menu.addAction(m_arabic?QString::fromUtf8("إظهار / إخفاء الإصدارات"):QStringLiteral("Show / hide versions"));
    versions->setEnabled(m_expert->isChecked());
    menu.addSeparator();
    QAction *queue=menu.addAction(m_arabic?QString::fromUtf8("تحديد للتثبيت"):QStringLiteral("Select for installation"));
    const int actionCandidate=candidate==0?reinstallCandidateIndex(entry):candidate;
    const bool eligible=m_catalog->canQueue(row) && (candidate<0 || (actionCandidate>0 && ((entry.candidates[actionCandidate].valid && entry.candidates[actionCandidate].local) || m_demo)));
    queue->setEnabled(eligible);
    QAction *install=menu.addAction(m_arabic?QString::fromUtf8("تثبيت التعريفات المحددة..."):QStringLiteral("Install selected drivers..."));
    install->setEnabled(eligible);
    QAction *getPackage=menu.addAction(m_arabic?QString::fromUtf8("تنزيل الحزمة الناقصة..."):QStringLiteral("Download missing package..."));
    getPackage->setEnabled(!m_demo && m_catalog->native() && candidate>0 && !entry.candidates[candidate].local && !entry.candidates[candidate].packageSource.isEmpty());
    int extractIndex=candidate>0?candidate:entry.chosenCandidate;
    if(extractIndex<=0)for(int i=1;i<entry.candidates.size();++i)if(entry.candidates[i].valid && entry.candidates[i].local){extractIndex=i;break;}
    QAction *extract=menu.addAction(m_arabic?QString::fromUtf8("استخراج ملفات هذا الإصدار..."):QStringLiteral("Extract this version..."));
    extract->setEnabled(candidate!=0 && !m_demo && m_catalog->native() && m_catalog->nativeSnapshot().generation && extractIndex>0 && extractIndex<entry.candidates.size() && entry.candidates[extractIndex].valid && entry.candidates[extractIndex].local);
    QAction *openInf=menu.addAction(m_arabic?QString::fromUtf8("عرض ملف INF"):QStringLiteral("View INF file"));
    QAction *locateInf=menu.addAction(m_arabic?QString::fromUtf8("فتح مكان ملف INF"):QStringLiteral("Show INF in folder"));
    const bool canOpenInf=!m_demo && (candidate==0?!entry.candidates[0].inf.isEmpty():extract->isEnabled());openInf->setEnabled(canOpenInf);locateInf->setEnabled(canOpenInf);
    menu.addSeparator();
    const QString hardware=candidate>=0 && !entry.candidates[candidate].hardwareId.isEmpty()?entry.candidates[candidate].hardwareId:entry.device.hardwareId;
    QAction *copy=menu.addAction(m_arabic?QString::fromUtf8("نسخ Hardware ID"):QStringLiteral("Copy hardware ID"));copy->setEnabled(!hardware.isEmpty());
    QAction *search=menu.addAction(m_arabic?QString::fromUtf8("البحث عن Hardware ID على الويب"):QStringLiteral("Search hardware ID on the web"));search->setEnabled(!hardware.isEmpty());
    menu.addSeparator();
    QAction *ignore=menu.addAction(m_catalog->isIgnored(row)?(m_arabic?QString::fromUtf8("إلغاء تجاهل الجهاز"):QStringLiteral("Stop ignoring this device")):(m_arabic?QString::fromUtf8("تجاهل هذا الجهاز"):QStringLiteral("Ignore this device")));
    ignore->setEnabled(!entry.device.hardwareId.isEmpty());
    QAction *restoreIgnored=menu.addAction(m_arabic?QString::fromUtf8("استعادة جميع الأجهزة المتجاهلة"):QStringLiteral("Restore all ignored devices"));restoreIgnored->setEnabled(!m_catalog->ignoredHardwareIds().isEmpty());
    QAction *chosen=menu.exec(position);
    if(row>=m_catalog->size() || m_catalog->nativeSnapshot().generation!=menuGeneration || m_catalog->entryRef(row).device.instanceId!=entry.device.instanceId)return;
    if(chosen==details)showEntry(row);
    else if(chosen==versions){for(QTreeWidgetItemIterator it(m_tree);*it;++it){QTreeWidgetItem *root=*it;if(root->data(0,Qt::UserRole+1).toInt()==-1 && root->data(0,Qt::UserRole).toInt()==row){populateCandidates(root);root->setExpanded(!root->isExpanded());break;}}}
    else if(chosen==queue || chosen==install){
        if(candidate>=0){if(!m_catalog->chooseCandidate(row,candidate))return;}else if(!m_catalog->setSelected(row,true))return;
        if(chosen==install)emit installationRequested();
    }else if(chosen==getPackage)emit packageDownloadRequested(entry.candidates[candidate].packageSource);
    else if(chosen==extract)emit candidateExtractionRequested(row,extractIndex);
    else if(chosen==openInf || chosen==locateInf)emit infViewRequested(row,candidate==0?0:extractIndex,chosen==locateInf);
    else if(chosen==copy)QApplication::clipboard()->setText(hardware);
    else if(chosen==search)QDesktopServices::openUrl(QUrl(QStringLiteral("https://www.google.com/search?q=")+QString::fromLatin1(QUrl::toPercentEncoding(hardware+QStringLiteral(" driver")))));
    else if(chosen==ignore)m_catalog->toggleIgnored(row);
    else if(chosen==restoreIgnored)m_catalog->setIgnoredHardwareIds(QStringList());
}
