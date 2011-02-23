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

#ifndef PHONON_VLC_VIDEOWIDGET_H
#define PHONON_VLC_VIDEOWIDGET_H

#include <QtCore/QObject>
#include "sinknode.h"
#include <phonon/videowidgetinterface.h>

#ifndef PHONON_VLC_NO_EXPERIMENTAL
#include "experimental/avcapture.h"
#endif // PHONON_VLC_NO_EXPERIMENTAL

#include "overlaywidget.h"

#include <QtCore/QMutex>

namespace Phonon
{
namespace VLC
{

/** \brief Implements the Phonon VideoWidget MediaNode, responsible for displaying video
 *
 * Phonon video is displayed using this widget. It implements the VideoWidgetInterface.
 * It is connected to a media object that provides the video source. Methods to control
 * video settings such as brightness or contrast are provided.
 *
 * \see VLCVideoWidget
 *
 * \internal A VLCVideoWidget is owned by this widget. That widget is used for the actual
 * drawings of libVLC.
 */
class VideoWidget : public OverlayWidget, public SinkNode, public VideoWidgetInterface
{
    Q_OBJECT
    Q_INTERFACES(Phonon::VideoWidgetInterface)
public:
    VideoWidget(QWidget *p_parent);
    ~VideoWidget();

    void connectToMediaObject(MediaObject *mediaObject);

    #ifndef PHONON_VLC_NO_EXPERIMENTAL
    void connectToAvCapture(Experimental::AvCapture *avCapture);
    void disconnectFromAvCapture(Experimental::AvCapture *avCapture);
    #endif // PHONON_VLC_NO_EXPERIMENTAL

    Phonon::VideoWidget::AspectRatio aspectRatio() const;
    void setAspectRatio(Phonon::VideoWidget::AspectRatio aspect);

    Phonon::VideoWidget::ScaleMode scaleMode() const;
    void setScaleMode(Phonon::VideoWidget::ScaleMode scale);

    qreal brightness() const;
    void setBrightness(qreal brightness);

    qreal contrast() const;
    void setContrast(qreal contrast);

    qreal hue() const;
    void setHue(qreal hue);

    qreal saturation() const;
    void setSaturation(qreal saturation);

    void useCustomRender();

    static void *lock(void *data, void **bufRet);
    static void unlock(void *data, void *id, void *const *pixels);

    /**
     * \return The owned widget that is used for the actual draw.
     */
    QWidget *widget();

    void setVideoSize(const QSize &videoSize);
    void setAspectRatio(double aspectRatio);
    void setScaleAndCropMode(bool scaleAndCrop);

    QSize sizeHint() const;

    void setVisible(bool visible);

public slots:
    void setNextFrame(const QByteArray &array, int width, int height);

private slots:
    void videoWidgetSizeChanged(int width, int height);

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
     * Original size of the video, needed for sizeHint().
     */
    QSize m_videoSize;

    QMutex m_locker;
    QImage *m_img;

    Phonon::VideoWidget::AspectRatio aspect_ratio;

    Phonon::VideoWidget::ScaleMode scale_mode;

    bool  b_filter_adjust_activated;
    qreal f_brightness;
    qreal f_contrast;
    qreal f_hue;
    qreal f_saturation;
};

}
} // Namespace Phonon::VLC

#endif // PHONON_VLC_VIDEOWIDGET_H
