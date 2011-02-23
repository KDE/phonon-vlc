/*****************************************************************************
 * libVLC backend for the Phonon library                                     *
 *                                                                           *
 * Copyright (C) 2007-2008 Tanguy Krotoff <tkrotoff@gmail.com>               *
 * Copyright (C) 2008 Lukas Durfina <lukas.durfina@gmail.com>                *
 * Copyright (C) 2009 Fathi Boudra <fabo@kde.org>                            *
 * Copyright (C) 2009-2011 vlc-phonon AUTHORS                                *
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

#include <QtCore/QtDebug>
#include <QtGui/QApplication>
#include <QtGui/QDesktopWidget>
#include <QtGui/QPainter>
#include <QtGui/QResizeEvent>
#include <QtGui/QWidget>

#include <vlc/vlc.h>

#include "mediaobject.h"

namespace Phonon
{
namespace VLC
{

/**
 * Constructs a new VideoWidget with the given parent. The video settings members
 * are set to their default values.
 */
VideoWidget::VideoWidget(QWidget *parent) :
    OverlayWidget(parent),
    SinkNode(),
    m_customRender(false),
    m_img(0),
    m_aspectRatio(Phonon::VideoWidget::AspectRatioAuto),
    m_scaleMode(Phonon::VideoWidget::FitInView),
    m_filterAdjustActivated(false),
    m_brightness(0.0),
    m_contrast(0.0),
    m_hue(0.0),
    m_saturation(0.0)
{
    setBackgroundColor(Qt::black);
}

VideoWidget::~VideoWidget()
{
    if (m_img) {
        delete m_img;
    }
}

void VideoWidget::connectToMediaObject(MediaObject *mediaObject)
{
    SinkNode::connectToMediaObject(mediaObject);

    connect(mediaObject, SIGNAL(videoWidgetSizeChanged(int, int)),
            SLOT(videoWidgetSizeChanged(int, int)));

    //  mediaObject->setVideoWidgetId(p_video_widget->winId());
    mediaObject->setVideoWidget(this);
}

#ifndef PHONON_VLC_NO_EXPERIMENTAL
void VideoWidget::connectToAvCapture(Experimental::AvCapture *avCapture)
{
    connectToMediaObject(avCapture->videoMediaObject());
}

void VideoWidget::disconnectFromAvCapture(Experimental::AvCapture *avCapture)
{
    disconnectFromMediaObject(avCapture->videoMediaObject());
}
#endif // PHONON_VLC_NO_EXPERIMENTAL

Phonon::VideoWidget::AspectRatio VideoWidget::aspectRatio() const
{
    return m_aspectRatio;
}

void VideoWidget::setAspectRatio(Phonon::VideoWidget::AspectRatio aspect)
{
    // finish if no player
    if (!m_player) {
        return;
    }

    m_aspectRatio = aspect;

    switch (m_aspectRatio) {
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
        qCritical() << __FUNCTION__ << "error: unsupported AspectRatio:" << (int) m_aspectRatio;
    }
}

Phonon::VideoWidget::ScaleMode VideoWidget::scaleMode() const
{
    return m_scaleMode;
}

void VideoWidget::setScaleMode(Phonon::VideoWidget::ScaleMode scale)
{
    m_scaleMode = scale;
    switch (m_scaleMode) {
    case Phonon::VideoWidget::FitInView: // The video will be fitted to fill the view keeping aspect ratio
        break;
    case Phonon::VideoWidget::ScaleAndCrop: // The video is scaled
        break;
    default:
        qWarning() << __FUNCTION__ << "unknow Phonon::VideoWidget::ScaleMode:" << m_scaleMode;
    }
}

qreal VideoWidget::brightness() const
{
    return m_brightness;
}

void VideoWidget::setBrightness(qreal brightness)
{
    m_brightness = brightness;

    // vlc takes brightness in range 0.0 - 2.0
    if (m_player) {
        if (!m_filterAdjustActivated) {
//            p_libvlc_video_filter_add(p_vlc_current_media_player, ADJUST, vlc_exception);
//            vlcExceptionRaised();
            m_filterAdjustActivated = true;
        }
//        p_libvlc_video_set_brightness(p_vlc_current_media_player, f_brightness + 1.0, vlc_exception);
//        vlcExceptionRaised();
    }
}


qreal VideoWidget::contrast() const
{
    return m_contrast;
}

void VideoWidget::setContrast(qreal contrast)
{
#ifdef __GNUC__
#warning OMG WTF
#endif
    m_contrast = contrast;

    // vlc takes contrast in range 0.0 - 2.0
    float f_contrast = contrast;
    if (m_player) {
        if (!m_filterAdjustActivated) {
//            p_libvlc_video_filter_add(p_vlc_current_media_player, ADJUST, vlc_exception);
//            vlcExceptionRaised();
            m_filterAdjustActivated = true;
        }
//        p_libvlc_video_set_contrast(p_vlc_current_media_player, f_contrast + 1.0, vlc_exception);
//        vlcExceptionRaised();
    }
}

qreal VideoWidget::hue() const
{
    return m_hue;
}

void VideoWidget::setHue(qreal hue)
{
    m_hue = hue;

    // vlc takes hue in range 0 - 360 in integer
    int i_hue = (m_hue + 1.0) * 180;
    if (m_player) {
        if (!m_filterAdjustActivated) {
//            p_libvlc_video_filter_add(p_vlc_current_media_player, ADJUST, vlc_exception);
//            vlcExceptionRaised();
            m_filterAdjustActivated = true;
        }
//        p_libvlc_video_set_hue(p_vlc_current_media_player, i_hue, vlc_exception);
//        vlcExceptionRaised();
    }

}

qreal VideoWidget::saturation() const
{
    return m_saturation;
}

void VideoWidget::setSaturation(qreal saturation)
{
    m_saturation = saturation;

    // vlc takes brightness in range 0.0 - 3.0
    if (m_player) {
        if (!m_filterAdjustActivated) {
//            p_libvlc_video_filter_add(p_vlc_current_media_player, ADJUST, vlc_exception);
//            vlcExceptionRaised();
            m_filterAdjustActivated = true;
        }
//        p_libvlc_video_set_saturation(p_vlc_current_media_player, (f_saturation + 1.0) * 1.5, vlc_exception);
//        vlcExceptionRaised();
    }
}


void VideoWidget::useCustomRender()
{
    m_customRender = true;
    QSize size = sizeHint();
    int width = size.width();
    int height = size.height();

    if (m_img) {
        delete m_img;
    }
    m_img = new QImage(size, QImage::Format_RGB32);
    libvlc_video_set_format(m_player, "RV32", width, height, width * 4);
    libvlc_video_set_callbacks(m_player, lock, unlock, 0, this);
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

    cw->setNextFrame(QByteArray::fromRawData((const char *)cw->m_img->bits(),
                                     cw->m_img->byteCount()),
                                     cw->m_img->width(), cw->m_img->height());
    cw->m_locker.unlock();
}


QWidget *VideoWidget::widget()
{
    return this;
}


void VideoWidget::resizeEvent(QResizeEvent *event)
{
    qDebug() << "resizeEvent" << event->size();
}

void VideoWidget::setAspectRatio(double aspectRatio)
{
#ifdef __GNUC__
#warning OMG WTF
#endif
}

void VideoWidget::setScaleAndCropMode(bool scaleAndCrop)
{
#ifdef __GNUC__
#warning OMG WTF
#endif
}

void VideoWidget::setVideoSize(const QSize &size)
{
    m_videoSize = size;
    updateGeometry();
    update();
}

QSize VideoWidget::sizeHint() const
{
    if (!m_videoSize.isEmpty()) {
        return m_videoSize;
    }
    return QSize(640, 480);
}

void VideoWidget::setVisible(bool visible)
{
    if (window() && window()->testAttribute(Qt::WA_DontShowOnScreen)) {
        qDebug() << "Widget rendering forced";
        useCustomRender();
    }
    QWidget::setVisible(visible);
}

void VideoWidget::setNextFrame(const QByteArray &array, int width, int height)
{
    // TODO: Should preloading ever become available ... what do to here?

    m_frame = QImage();
    {
        m_frame = QImage((uchar *)array.constData(), width, height, QImage::Format_RGB32);
    }
    update();
}

void VideoWidget::videoWidgetSizeChanged(int width, int height)
{
    qDebug() << __FUNCTION__ << "video width" << width << "height:" << height;

    // It resizes dynamically the widget and the main window
    // Note: I didn't find another way

    QSize videoSize(width, height);
    videoSize.boundedTo(QApplication::desktop()->availableGeometry().size());

    hide();
    setVideoSize(videoSize);
#ifdef Q_OS_WIN
    QWidget *p_parent = qobject_cast<QWidget *>(this->parent());
    QSize previousSize = p_parent->minimumSize();
    p_parent->setMinimumSize(videoSize);
#endif
    show();
#ifdef Q_OS_WIN
    setMinimumSize(previousSize);
#endif

    if (m_img) {
        delete m_img;
    }
    m_img = new QImage(videoSize, QImage::Format_RGB32);
    libvlc_video_set_format(m_player, "RV32", width, height, width * 4);
}

void VideoWidget::paintEvent(QPaintEvent *event)
{
    if (m_customRender) {
        QPainter painter(this);
        // TODO: more sensible rect calculation.
        painter.drawImage(rect(), m_frame);
    } else {
        OverlayWidget::paintEvent(event);
    }
}


}
} // Namespace Phonon::VLC
