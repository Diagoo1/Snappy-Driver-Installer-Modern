#include "about_page.h"
#include "appearance.h"
#include "vector_icons.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QScrollArea>
#include <QPainter>
#include <QTimer>
#include <QApplication>
#include <QDesktopServices>
#include <QUrl>
#include <QtMath>

class DeveloperPortrait : public QWidget {
public:explicit DeveloperPortrait(QWidget *p):QWidget(p){setFixedSize(92,92);}
protected:void paintEvent(QPaintEvent*) Q_DECL_OVERRIDE {QPainter p(this);p.setRenderHint(QPainter::Antialiasing);QPainterPath clip;clip.addEllipse(QRectF(4,4,84,84));p.setClipPath(clip);p.drawPixmap(rect(),QPixmap(QStringLiteral(":/sdio/assets/developer-avatar.jpg")));p.setClipping(false);p.setPen(QPen(QColor("#33C9BA"),3));p.setBrush(Qt::NoBrush);p.drawEllipse(QRectF(3,3,86,86));}
};
class Heartbeat : public QWidget {
public:explicit Heartbeat(QWidget *parent):QWidget(parent),m_frame(0){setFixedSize(32,32);QTimer *timer=new QTimer(this);timer->setInterval(33);connect(timer,&QTimer::timeout,this,[this](){if(isVisible() && !qApp->property("reduceMotion").toBool()){++m_frame;update();}});timer->start();}
protected:void paintEvent(QPaintEvent*) Q_DECL_OVERRIDE {QPainter p(this);p.setRenderHint(QPainter::Antialiasing);const qreal phase=(m_frame%60)/60.0;const qreal scale=qApp->property("reduceMotion").toBool()?1:1+.13*qMax(0.0,qSin(phase*4*3.14159265))*qExp(-phase*4);p.translate(16,16);p.scale(scale,scale);QPainterPath heart;heart.moveTo(0,9);heart.cubicTo(-18,-2,-10,-15,0,-6);heart.cubicTo(10,-15,18,-2,0,9);p.setPen(Qt::NoPen);p.setBrush(QColor("#ED7589"));p.drawPath(heart);}
private:int m_frame;
};
AboutPage::AboutPage(bool arabic,QWidget *parent):QWidget(parent),m_scroll(new QScrollArea(this)){
    setObjectName(QStringLiteral("page"));QVBoxLayout *root=new QVBoxLayout(this);root->setContentsMargins(0,0,0,20);
    m_scroll->setWidgetResizable(true);m_scroll->setFrameShape(QFrame::NoFrame);m_scroll->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);root->addWidget(m_scroll);setArabic(arabic);
}
void AboutPage::setArabic(bool ar){
    auto t=[ar](const char *en,const char *arabic){return ar?QString::fromUtf8(arabic):QString::fromUtf8(en);};
    QWidget *previous=m_scroll->takeWidget();if(previous)previous->deleteLater();
    QWidget *body=new QWidget;body->setObjectName(QStringLiteral("aboutBody"));QVBoxLayout *box=new QVBoxLayout(body);box->setContentsMargins(34,27,34,30);box->setSpacing(17);
    auto text=[ar](const QString &value,const char *name,QWidget *parent){QLabel *v=new QLabel(value,parent);v->setObjectName(QString::fromLatin1(name));v->setWordWrap(true);v->setAlignment(Qt::AlignAbsolute|(ar?Qt::AlignRight:Qt::AlignLeft)|Qt::AlignVCenter);v->setTextFormat(Qt::PlainText);return v;};
    box->addWidget(text(t("About the project","حول المشروع"),"pageTitle",body));
    box->addWidget(text(t("Thoughtful design. Practical tools. Built for your Windows PC.","تصميم بعناية، وأدوات عملية لجهازك."),"subText",body));
    AccentCard *hero=new AccentCard(body);QHBoxLayout *heroLayout=new QHBoxLayout(hero);heroLayout->setContentsMargins(26,25,26,25);heroLayout->setSpacing(22);
    QLabel *logo=new QLabel(hero);QPixmap mark(QStringLiteral(":/sdio/assets/tarek-sadek-logo.png"));const qreal dpr=logo->devicePixelRatioF();mark=mark.scaled(qRound(80*dpr),qRound(80*dpr),Qt::KeepAspectRatio,Qt::SmoothTransformation);mark.setDevicePixelRatio(dpr);logo->setPixmap(mark);heroLayout->addWidget(logo);
    QVBoxLayout *heroText=new QVBoxLayout;QLabel *name=text(t("Snappy Driver Installer","مدير تعريفات الأجهزة"),"heroName",hero);name->setStyleSheet(QStringLiteral("color:white;background:transparent;font-size:25px;font-weight:600;"));heroText->addWidget(name);
    QLabel *description=text(t("Find the right driver. Keep the process clear.","اعثر على التعريف المناسب، بخطوات واضحة."),"heroDescription",hero);description->setStyleSheet(QStringLiteral("color:#BDE7E8;background:transparent;font-size:13px;"));heroText->addWidget(description);heroLayout->addLayout(heroText,1);box->addWidget(hero);
    QWidget *developer=new QWidget(body);developer->setObjectName(QStringLiteral("softSurface"));QHBoxLayout *profile=new QHBoxLayout(developer);profile->setContentsMargins(25,23,25,23);profile->setSpacing(22);profile->addWidget(new DeveloperPortrait(developer));
    QVBoxLayout *bio=new QVBoxLayout;QHBoxLayout *made=new QHBoxLayout;made->addWidget(text(t("Made with care by","صُمّم بعناية بواسطة"),"mutedText",developer));made->addWidget(new Heartbeat(developer));made->addStretch();bio->addLayout(made);
    QLabel *author=text(t("Tarek Sadek","طارق صادق"),"developerName",developer);author->setStyleSheet(QStringLiteral("color:#00A89E;font-size:28px;font-weight:700;background:transparent;"));bio->addWidget(author);
    bio->addWidget(text(t("Interface design & development · Diagoo1","تصميم وتطوير الواجهة · Diagoo1"),"bodyText",developer));
    bio->addWidget(text(t("Creator of Dreams Software Collection","مطوّر مجموعة دريمز للبرامج"),"subText",developer));profile->addLayout(bio,1);box->addWidget(developer);
    QHBoxLayout *links=new QHBoxLayout;links->setSpacing(12);
    const QStringList titles=QStringList()<<t("GitHub projects","مشاريعي على GitHub")<<t("Support development","دعم التطوير")<<t("Contact me","تواصل معي");
    const QStringList urls=QStringList()<<QStringLiteral("https://github.com/Diagoo1")<<QStringLiteral("https://paypal.me/Diagoo1")<<QStringLiteral("mailto:tarek.sadek44@gmail.com");
    for(int i=0;i<titles.size();++i){ActionButton *button=new ActionButton(titles[i],body);button->setObjectName(QStringLiteral("outlineButton"));button->setMinimumHeight(48);button->setToolTip(urls[i]);const QUrl url(urls[i]);connect(button,&QPushButton::clicked,this,[url](){QDesktopServices::openUrl(url);});links->addWidget(button,1);}box->addLayout(links);
    QWidget *credits=new QWidget(body);credits->setObjectName(QStringLiteral("softSurface"));QVBoxLayout *creditBox=new QVBoxLayout(credits);creditBox->setContentsMargins(24,18,24,18);creditBox->setSpacing(9);
    creditBox->addWidget(text(t("Built on open source","مبني على برمجيات مفتوحة المصدر"),"sectionTitle",credits));
    creditBox->addWidget(text(t("Driver matching and installation: Snappy Driver Installer Origin contributors. Modern interface: Tarek Sadek. Distributed under GPL v3 or later.","مطابقة التعريفات وتثبيتها: مساهمو Snappy Driver Installer Origin. الواجهة الحديثة: طارق صادق. التوزيع وفق GPL الإصدار الثالث أو أحدث."),"bodyText",credits));
    creditBox->addWidget(text(t("Inspired by the developer card in Dreams Software Collection.","مستوحى من بطاقة المطوّر في مجموعة دريمز للبرامج."),"mutedText",credits));box->addWidget(credits);box->addStretch(1);m_scroll->setWidget(body);
}
