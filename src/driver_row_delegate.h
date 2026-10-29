#ifndef SDIOMODERN_DRIVER_ROW_DELEGATE_H
#define SDIOMODERN_DRIVER_ROW_DELEGATE_H

#include <QStyledItemDelegate>

// More generous two-line rows and native vector star ratings (demo only).
// Item data comes from DriverCatalog; this delegate never decides which driver is safe.
class DriverRowDelegate : public QStyledItemDelegate
{
public:
    explicit DriverRowDelegate(QObject *parent=0);
    void paint(QPainter *painter,const QStyleOptionViewItem &option,
               const QModelIndex &index) const Q_DECL_OVERRIDE;
};

#endif
