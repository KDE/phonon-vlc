#ifndef VLCMACWIDGET_H
#define VLCMACWIDGET_H

#include <QtGui>

#import <qmaccocoaviewcontainer_mac.h>

class VlcMacWidget : public QMacCocoaViewContainer
{
public:
    VlcMacWidget(QWidget *parent = 0);
};


#endif // VLCMACWIDGET_H
