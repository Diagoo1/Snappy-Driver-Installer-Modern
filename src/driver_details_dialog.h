#ifndef SDIOMODERN_DRIVER_DETAILS_DIALOG_H
#define SDIOMODERN_DRIVER_DETAILS_DIALOG_H
#include "appearance.h"
#include "driver_catalog.h"
class DriverDetailsDialog : public RoundedDialog
{
    Q_OBJECT
public:
    DriverDetailsDialog(const DriverEntry &entry,bool demo,bool arabic,QWidget *parent=0);
};
#endif
