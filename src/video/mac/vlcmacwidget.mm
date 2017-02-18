/*
    Copyright (C) 2010 Benoit Calvez <benoit@litchis.org>

    This library is free software; you can redistribute it and/or
    modify it under the terms of the GNU Lesser General Public
    License as published by the Free Software Foundation; either
    version 2.1 of the License, or (at your option) any later version.

    This library is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
    Lesser General Public License for more details.

    You should have received a copy of the GNU Lesser General Public
    License along with this library.  If not, see <http://www.gnu.org/licenses/>.
*/

#include "vlcmacwidget.h"
#import <Cocoa/Cocoa.h>
#import "nsvideoview.h"

#include <QtCore/QtDebug>

VlcMacWidget::VlcMacWidget(QWidget *parent) : QMacCocoaViewContainer(0, parent)
{
    // Many Cocoa objects create temporary autorelease objects,
    // so create a pool to catch them.
    @autoreleasepool {
        VideoView *videoView = [[VideoView alloc] init];

        this->setCocoaView(videoView);

        // Release our reference, since our super class takes ownership and we
        // don't need it anymore (except with Qt 5.8.0 because of a regression).
        if (strcmp(qVersion(), "5.8.0")) {
            [videoView release];
        }
    }
}
