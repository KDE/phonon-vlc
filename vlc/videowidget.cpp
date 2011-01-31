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

#include "videowidget.h"

#include <QtGui/QWidget>
#include <QtGui/QApplication>
#include <QtGui/QDesktopWidget>
#include <QtCore/QtDebug>

#include "mediaobject.h"

#include "vlcloader.h"

namespace Phonon
{
namespace VLC
{

/**
 * Constructs a new VideoWidget with the given parent. The video settings members
 * are set to their default values.
 */
VideoWidget::VideoWidget(QWidget *p_parent) :
    SinkNode(p_parent),
    m_img(0)
{
    p_video_widget = new Widget(p_parent, this);

    aspect_ratio = Phonon::VideoWidget::AspectRatioAuto;
    scale_mode = Phonon::VideoWidget::FitInView;

    b_filter_adjust_activated = false;
    f_brightness = 0.0;
    f_contrast = 0.0;
    f_hue = 0.0;
    f_saturation = 0.0;
}

VideoWidget::~VideoWidget()
{
    if (m_img) {
        delete m_img;
    }
}

/**
 * Connects the VideoWidget to a media object by setting the video widget
 * window system identifier of the media object to that of the owned private
 * video widget. It also connects the signal from the mediaObject regarding
 * a resize of the video.
 *
 * If the mediaObject was connected to another VideoWidget, the connection is
 * lost.
 *
 * \see VLCMediaObject
 * \param mediaObject What media object to connect to
 */
void VideoWidget::connectToMediaObject(MediaObject *mediaObject)
{
    SinkNode::connectToMediaObject(mediaObject);

    connect(mediaObject, SIGNAL(videoWidgetSizeChanged(int, int)),
            SLOT(videoWidgetSizeChanged(int, int)));

    //  mediaObject->setVideoWidgetId(p_video_widget->winId());
    mediaObject->setVideoWidget(p_video_widget);
}

#ifndef PHONON_VLC_NO_EXPERIMENTAL
/**
 * Connects the VideoWidget to an AvCapture. connectToMediaObject() is called
 * only for the video media of the AvCapture.
 *
 * \see AvCapture
 */
void VideoWidget::connectToAvCapture(Experimental::AvCapture *avCapture)
{
    connectToMediaObject(avCapture->videoMediaObject());
}

/**
 * Disconnect the VideoWidget from the video media of the AvCapture.
 *
 * \see connectToAvCapture()
 */
void VideoWidget::disconnectFromAvCapture(Experimental::AvCapture *avCapture)
{
    disconnectFromMediaObject(avCapture->videoMediaObject());
}
#endif // PHONON_VLC_NO_EXPERIMENTAL

/**
 * \return The aspect ratio previously set for the video widget
 */
Phonon::VideoWidget::AspectRatio VideoWidget::aspectRatio() const
{
    return aspect_ratio;
}

/**
 * Set the aspect ratio of the video.
 * VLC accepted formats are x:y (4:3, 16:9, etc...) expressing the global image aspect.
 */
void VideoWidget::setAspectRatio(Phonon::VideoWidget::AspectRatio aspect)
{
    // finish if no player
    if (!m_vlcPlayer) {
        return;
    }

    aspect_ratio = aspect;

    switch (aspect_ratio) {
    case Phonon::VideoWidget::AspectRatioAuto: // Let the decoder find the aspect ratio automatically from the media file (this is the default)
//        p_libvlc_video_set_aspect_ratio(p_vlc_current_media_player, "", vlc_exception);
        break;
    case Phonon::VideoWidget::AspectRatioWidget: // Fit the video into the widget making the aspect ratio depend solely on the size of the widget
        // This way the aspect ratio is freely resizeable by the user
//        p_libvlc_video_set_aspect_ratio(p_vlc_current_media_player, "", vlc_exception);
        break;
    case Phonon::VideoWidget::AspectRatio4_3:
//        p_libvlc_video_set_aspect_ratio(p_vlc_current_media_player, "4:3", vlc_exception);
        break;
    case Phonon::VideoWidget::AspectRatio16_9:
//        p_libvlc_video_set_aspect_ratio(p_vlc_current_media_player, "16:9", vlc_exception);
        break;
    default:
        qCritical() << __FUNCTION__ << "error: unsupported AspectRatio:" << (int) aspect_ratio;
    }
}

/**
 * \return The scale mode previously set for the video widget
 */
Phonon::VideoWidget::ScaleMode VideoWidget::scaleMode() const
{
    return scale_mode;
}

/**
 * Set how the video is scaled, keeping the aspect ratio into account when the video is resized.
 *
 * The ScaleMode enumeration describes how to treat aspect ratio during resizing of video.
 * \li Phonon::VideoWidget::FitInView - the video will be fitted to fill the view keeping aspect ratio
 * \li Phonon::VideoWidget::ScaleAndCrop - the video is scaled
 */
void VideoWidget::setScaleMode(Phonon::VideoWidget::ScaleMode scale)
{
    scale_mode = scale;
    switch (scale_mode) {
    case Phonon::VideoWidget::FitInView: // The video will be fitted to fill the view keeping aspect ratio
        break;
    case Phonon::VideoWidget::ScaleAndCrop: // The video is scaled
        break;
    default:
        qWarning() << __FUNCTION__ << "unknow Phonon::VideoWidget::ScaleMode:" << scale_mode;
    }
}

/**
 * \return The brightness previously set for the video widget
 */
qreal VideoWidget::brightness() const
{
    return f_brightness;
}

/**
 * Set the brightness of the video
 */
void VideoWidget::setBrightness(qreal brightness)
{
    f_brightness = brightness;

    // vlc takes brightness in range 0.0 - 2.0
    if (m_vlcPlayer) {
        if (!b_filter_adjust_activated) {
//            p_libvlc_video_filter_add(p_vlc_current_media_player, ADJUST, vlc_exception);
//            vlcExceptionRaised();
            b_filter_adjust_activated = true;
        }
//        p_libvlc_video_set_brightness(p_vlc_current_media_player, f_brightness + 1.0, vlc_exception);
//        vlcExceptionRaised();
    }
}

/**
 * \return The contrast previously set for the video widget
 */
qreal VideoWidget::contrast() const
{
    return f_contrast;
}

/**
 * Set the contrast of the video
 */
void VideoWidget::setContrast(qreal contrast)
{
    f_contrast = contrast;

    // vlc takes contrast in range 0.0 - 2.0
    float f_contrast = contrast;
    if (m_vlcPlayer) {
        if (!b_filter_adjust_activated) {
//            p_libvlc_video_filter_add(p_vlc_current_media_player, ADJUST, vlc_exception);
//            vlcExceptionRaised();
            b_filter_adjust_activated = true;
        }
//        p_libvlc_video_set_contrast(p_vlc_current_media_player, f_contrast + 1.0, vlc_exception);
//        vlcExceptionRaised();
    }
}

/**
 * \return The hue previously set for the video widget
 */
qreal VideoWidget::hue() const
{
    return f_hue;
}

/**
 * Set the hue of the video
 */
void VideoWidget::setHue(qreal hue)
{
    f_hue = hue;

    // vlc takes hue in range 0 - 360 in integer
    int i_hue = (f_hue + 1.0) * 180;
    if (m_vlcPlayer) {
        if (!b_filter_adjust_activated) {
//            p_libvlc_video_filter_add(p_vlc_current_media_player, ADJUST, vlc_exception);
//            vlcExceptionRaised();
            b_filter_adjust_activated = true;
        }
//        p_libvlc_video_set_hue(p_vlc_current_media_player, i_hue, vlc_exception);
//        vlcExceptionRaised();
    }

}

/**
 * \return The saturation previously set for the video widget
 */
qreal VideoWidget::saturation() const
{
    return f_saturation;
}

/**
 * Set the saturation of the video
 */
void VideoWidget::setSaturation(qreal saturation)
{
    f_saturation = saturation;

    // vlc takes brightness in range 0.0 - 3.0
    if (m_vlcPlayer) {
        if (!b_filter_adjust_activated) {
//            p_libvlc_video_filter_add(p_vlc_current_media_player, ADJUST, vlc_exception);
//            vlcExceptionRaised();
            b_filter_adjust_activated = true;
        }
//        p_libvlc_video_set_saturation(p_vlc_current_media_player, (f_saturation + 1.0) * 1.5, vlc_exception);
//        vlcExceptionRaised();
    }
}

/**
 * \return The owned widget that is used for the actual draw.
 */
Widget *VideoWidget::widget()
{
    return p_video_widget;
}

/**
 * Handles the change in the size of the video
 */
void VideoWidget::videoWidgetSizeChanged(int width, int height)
{
    qDebug() << __FUNCTION__ << "video width" << width << "height:" << height;

    // It resizes dynamically the widget and the main window
    // Note: I didn't find another way

    QSize videoSize(width, height);
    videoSize.boundedTo(QApplication::desktop()->availableGeometry().size());

    p_video_widget->hide();
    p_video_widget->setVideoSize(videoSize);
#ifdef Q_OS_WIN
    QWidget *p_parent = qobject_cast<QWidget *>(this->parent());
    QSize previousSize = p_parent->minimumSize();
    p_parent->setMinimumSize(videoSize);
#endif
    p_video_widget->show();
#ifdef Q_OS_WIN
    p_parent->setMinimumSize(previousSize);
#endif

    if (m_img) {
        delete m_img;
    }
    m_img = new QImage(videoSize, QImage::Format_RGB32);
    libvlc_video_set_format(m_vlcPlayer, "RV32", width, height, width * 4);
}

void VideoWidget::useCustomRender()
{
    QSize size = p_video_widget->sizeHint();
    int width = size.width();
    int height = size.height();

    if (m_img) {
        delete m_img;
    }
    m_img = new QImage(size, QImage::Format_RGB32);
    libvlc_video_set_format(m_vlcPlayer, "RV32", width, height, width * 4);
    libvlc_video_set_callbacks(m_vlcPlayer, lock, unlock, 0, this);
}

void *VideoWidget::lock(void *data, void **bufRet)
{
    VideoWidget *cw = (VideoWidget *)data;
    cw->m_locker.lock();
    *bufRet = cw->m_img->bits();
    return NULL; // Picture identifier, not needed here.
}

void VideoWidget::unlock(void *data, void *id, void *const *pixels)
{
    Q_UNUSED(id);
    Q_UNUSED(pixels);

    VideoWidget *cw = (VideoWidget *)data;

    // Might be a good idea to cache these, but this should be insignificant overhead compared to the image conversion
//    const char *aspect = libvlc_video_get_aspect_ratio(cw->m_vlcPlayer);
//    delete aspect;

    cw->p_video_widget->setNextFrame(QByteArray::fromRawData((const char *)cw->m_img->bits(),
                                     cw->m_img->byteCount()),
                                     cw->m_img->width(), cw->m_img->height());
    cw->m_locker.unlock();
}

}
} // Namespace Phonon::VLC
