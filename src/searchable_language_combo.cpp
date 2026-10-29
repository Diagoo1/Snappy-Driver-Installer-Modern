#include "searchable_language_combo.h"
#include "appearance.h"
#include "ui_language.h"
#include <QPropertyAnimation>
#include <QFrame>
#include <QLineEdit>
#include <QListWidget>
#include <QListWidgetItem>
#include <QVBoxLayout>
#include <QApplication>
#include <QDesktopWidget>
#include <QScrollBar>
#include <QAbstractItemView>

SearchableLanguageCombo::SearchableLanguageCombo(QWidget *parent)
    : QComboBox(parent),m_popup(new RoundedPanel(this)),
      m_filter(new QLineEdit(m_popup)),m_list(new QListWidget(m_popup))
{
    m_popup->setWindowFlags(Qt::Popup|Qt::FramelessWindowHint|Qt::NoDropShadowWindowHint);
    m_popup->setObjectName(QStringLiteral("languagePopup"));
    m_popup->setProperty("darkMode",palette().color(QPalette::Base).lightness()<130);
    m_popup->setMinimumWidth(290);
    m_popup->setFocusPolicy(Qt::StrongFocus);
    QVBoxLayout *layout=new QVBoxLayout(m_popup);
    layout->setContentsMargins(10,10,10,10);layout->setSpacing(8);
    m_filter->setObjectName(QStringLiteral("languageSearch"));
    m_filter->setPlaceholderText(UiLanguage::text(QStringLiteral("Search..."),QString::fromUtf8("ابحث...")));
    m_filter->setClearButtonEnabled(true);
    m_list->setObjectName(QStringLiteral("languageChoices"));
    m_list->setUniformItemSizes(true);
    m_list->setVerticalScrollMode(QAbstractItemView::ScrollPerPixel);
    m_list->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    m_list->setSelectionMode(QAbstractItemView::SingleSelection);
    m_list->setFocusPolicy(Qt::NoFocus);
    layout->addWidget(m_filter);
    layout->addWidget(m_list,1);
    connect(m_filter,&QLineEdit::textChanged,this,[this](const QString &text){filterItems(text);});
    connect(m_list,&QListWidget::itemClicked,this,[this](QListWidgetItem *item){choose(item);});
    connect(m_list,&QListWidget::itemActivated,this,[this](QListWidgetItem *item){choose(item);});
    connect(m_filter,&QLineEdit::returnPressed,this,[this](){
        for(int i=0;i<m_list->count();++i)if(!m_list->item(i)->isHidden()){
            choose(m_list->item(i));return;
        }
    });
}
void SearchableLanguageCombo::filterItems(const QString &query)
{
    for(int i=0;i<m_list->count();++i){
        QListWidgetItem *item=m_list->item(i);
        const int index=item->data(Qt::UserRole).toInt();
        item->setHidden(!query.trimmed().isEmpty() &&
            !itemText(index).contains(query,Qt::CaseInsensitive) &&
            !itemData(index).toString().contains(query,Qt::CaseInsensitive));
    }
}
void SearchableLanguageCombo::choose(QListWidgetItem *item)
{
    if(!item)return;
    const int index=item->data(Qt::UserRole).toInt();
    if(index>=0 && index<count())setCurrentIndex(index);
    hidePopup();
}
void SearchableLanguageCombo::showPopup()
{
    m_popup->setProperty("darkMode",palette().color(QPalette::Base).lightness()<130);
    m_filter->setPlaceholderText(property("languageSelector").toBool()?
        UiLanguage::text(QStringLiteral("Search languages..."),QString::fromUtf8("ابحث عن لغة...")):
        objectName()==QStringLiteral("candidatePicker")?
        UiLanguage::text(QStringLiteral("Search versions..."),QString::fromUtf8("ابحث عن إصدار...")):
        UiLanguage::text(QStringLiteral("Search..."),QString::fromUtf8("ابحث...")));
    m_popup->setLayoutDirection(layoutDirection());
    m_filter->setAlignment(Qt::AlignAbsolute|(UiLanguage::isRtl()?Qt::AlignRight:Qt::AlignLeft));
    m_list->clear();
    for(int i=0;i<count();++i){
        QListWidgetItem *entry=new QListWidgetItem(itemText(i),m_list);
        entry->setData(Qt::UserRole,i);
        entry->setSizeHint(QSize(0,44));
        if(i==currentIndex())m_list->setCurrentItem(entry);
    }
    m_filter->clear();m_filter->setVisible(count()>8);
    int w=qMax(width(),330);
    const QRect screen=QApplication::desktop()->availableGeometry(this);
    w=qMin(w,screen.width()-24);
    const int h=qMin(420,qMin(screen.height()-24,qMax(150,count()*44+70)));
    QPoint pos=mapToGlobal(QPoint(0,height()+6));
    if(pos.y()+h>screen.bottom()-4)pos=mapToGlobal(QPoint(0,-h-6));
    if(pos.y()<screen.top()+4)pos.setY(screen.top()+4);
    if(pos.x()+w>screen.right()-4)pos.setX(screen.right()-w-4);
    if(pos.x()<screen.left()+4)pos.setX(screen.left()+4);
    m_popup->setGeometry(QRect(pos,QSize(w,h)));
    m_popup->show();m_popup->raise();m_filter->setFocus(Qt::PopupFocusReason);
    if(!qApp->property("reduceMotion").toBool()){
        QPropertyAnimation *fade=new QPropertyAnimation(m_popup,"windowOpacity",m_popup);
        fade->setDuration(160);fade->setStartValue(.0);fade->setEndValue(1.0);fade->start(QAbstractAnimation::DeleteWhenStopped);
    }
}
void SearchableLanguageCombo::hidePopup()
{
    m_popup->hide();
}
