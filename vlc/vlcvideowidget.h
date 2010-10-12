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

#ifndef PHONON_VLC_VLCVIDEOWIDGET_H
#define PHONON_VLC_VLCVIDEOWIDGET_H

#include "overlaywidget.h"

#include <QtGui/QWidget>

class QResizeEvent;

namespace Phonon
{
namespace VLC
{

class VideoWidget;

/**
 * @short Widget where to show VLC video.
 *
 * It's window system identifier is used by a VLC media object to provide libVLC
 * the place where to draw the video.
 *
 * @ref VLCMediaObject::setVLCWidgetId()
 * @see VideoWidget
 * @see VLCMediaObject
 */
class VLCVideoWidget : public OverlayWidget
{
    Q_OBJECT
public:

    VLCVideoWidget(QWidget *parent, VideoWidget *videoWidget);
    ~VLCVideoWidget();

    void setVideoSize(const QSize &videoSize);
    void setAspectRatio(double aspectRatio);
    void setScaleAndCropMode(bool scaleAndCrop);

    QSize sizeHint() const;

    void setVisible(bool visible);

public slots:

    void setNextFrame(const QByteArray &array, int width, int height);

protected:

    virtual void paintEvent(QPaintEvent *event);
    virtual void resizeEvent(QResizeEvent *event);

private:
    /**
     * Whether custom rendering is used.
     */
    bool m_customRender;

    /**
     * Next drawable frame (if any).
     */
    mutable QImage m_frame;

    /**
     * VideoWidget containing this VLCVideoWidget.
     */
    VideoWidget *m_videoWidget;

    /**
     * Original size of the video, needed for sizeHint().
     */
    QSize m_videoSize;
};

}
} // Namespace Phonon::VLC_MPlayer

#endif // PHONON_VLC_MPLAYER_VLCVIDEOWIDGET_H
