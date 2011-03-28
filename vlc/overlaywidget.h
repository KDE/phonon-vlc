/*****************************************************************************
 * libVLC backend for the Phonon library                                     *
 *                                                                           *
 * Copyright (C) 2007-2008 Tanguy Krotoff <tkrotoff@gmail.com>               *
 * Copyright (C) 2008 Lukas Durfina <lukas.durfina@gmail.com>                *
 * Copyright (C) 2009 Fathi Boudra <fabo@kde.org>                            *
 * Copyright (C) 2009-2010 vlc-phonon AUTHORS                                *
 *                                                                           *
 * This program is free software; you can redistribute it and/or             *
 * modify it under the terms of the GNU Lesser General Public                *
 * License as published by the Free Software Foundation; either              *
 * version 2.1 of the License, or (at your option) any later version.        *
 *                                                                           *
 * This program is distributed in the hope that it will be useful,           *
 * but WITHOUT ANY WARRANTY; without even the implied warranty of            *
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU         *
 * Lesser General Public License for more details.                           *
 *                                                                           *
 * You should have received a copy of the GNU Lesser General Public          *
 * License along with this package; if not, write to the Free Software       *
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301 USA *
 *****************************************************************************/

#ifndef PHONON_VLC_OVERLAYWIDGET_H
#define PHONON_VLC_OVERLAYWIDGET_H

#include <QtGui/QWidget>

#ifdef Q_OS_MAC
#include "vlcmacwidget.h"
typedef VlcMacWidget BaseWidget;
#else
typedef QWidget BaseWidget;
#endif

namespace Phonon
{
namespace VLC
{

/**
 * @short Utility class: special widget for playing videos.
 *
 * Does not handle paintEvent()
 */
class OverlayWidget : public BaseWidget
{
    Q_OBJECT

public:
    /**
     * Constructor.
     *
     * @param parent parent widget
     */
    OverlayWidget(QWidget *parent);

    /**
     * Sets the background color.
     *
     * @param color the color to set
     */
    void setBackgroundColor(const QColor &color);

protected:
    /*
     * Overloaded from BaseWidget (QWidget)
     */
    void paintEvent(QPaintEvent *event);
};

}
} // Namespace Phonon::VLC

#endif // PHONON_VLC_OVERLAYWIDGET_H
