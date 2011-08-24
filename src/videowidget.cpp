/*
    Copyright (C) 2007-2008 Tanguy Krotoff <tkrotoff@gmail.com>
    Copyright (C) 2008 Lukas Durfina <lukas.durfina@gmail.com>
    Copyright (C) 2009 Fathi Boudra <fabo@kde.org>
    Copyright (C) 2009-2011 vlc-phonon AUTHORS

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

#include "videowidget.h"

#include <QtCore/QtDebug>
#include <QtGui/QApplication>
#include <QtGui/QDesktopWidget>
#include <QtGui/QPainter>
#include <QtGui/QResizeEvent>

#include <vlc/vlc.h>

#include "debug.h"

#ifndef PHONON_VLC_NO_EXPERIMENTAL
#include "experimental/avcapture.h"
#endif // PHONON_VLC_NO_EXPERIMENTAL
#include "mediaobject.h"

namespace Phonon
{
namespace VLC
{

VideoWidget::VideoWidget(QWidget *parent) :
    OverlayWidget(parent),
    SinkNode(),
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
}

void VideoWidget::connectToMediaObject(MediaObject *mediaObject)
{
    SinkNode::connectToMediaObject(mediaObject);

    connect(mediaObject, SIGNAL(videoWidgetSizeChanged(int, int)),
            SLOT(videoWidgetSizeChanged(int, int)));
    connect(mediaObject, SIGNAL(hasVideoChanged(bool)),
            SLOT(processPendingAdjusts(bool)));
    connect(mediaObject, SIGNAL(currentSourceChanged(MediaSource)),
            SLOT(clearPendingAdjusts()));

    mediaObject->setVideoWidget(this);

    clearPendingAdjusts();
}

void VideoWidget::disconnectFromMediaObject(MediaObject *mediaObject)
{
    SinkNode::disconnectFromMediaObject(mediaObject);
    // Undo all connections or path creation->destruction->creation can cause
    // duplicated connections or getting singals from two different MediaObjects.
    disconnect(mediaObject, 0, this, 0);
}

Phonon::VideoWidget::AspectRatio VideoWidget::aspectRatio() const
{
    return m_aspectRatio;
}

void VideoWidget::setAspectRatio(Phonon::VideoWidget::AspectRatio aspect)
{
    DEBUG_BLOCK;
    if (!m_player)
        return;

    m_aspectRatio = aspect;

    switch (m_aspectRatio) {
    // FIXME: find a way to implement aspectratiowidget, or rather, find out what
    // that is supposed to achieve to begin with.
    case Phonon::VideoWidget::AspectRatioWidget:
    case Phonon::VideoWidget::AspectRatioAuto:
        m_player->setVideoAspectRatio(QByteArray());
        break;
    case Phonon::VideoWidget::AspectRatio4_3:
        m_player->setVideoAspectRatio("4:3");
        break;
    case Phonon::VideoWidget::AspectRatio16_9:
        m_player->setVideoAspectRatio("16:9");
        break;
    }
}

Phonon::VideoWidget::ScaleMode VideoWidget::scaleMode() const
{
    return m_scaleMode;
}

void VideoWidget::setScaleMode(Phonon::VideoWidget::ScaleMode scale)
{
#ifdef __GNUC__
#warning OMG WTF
#endif
    m_scaleMode = scale;
    switch (m_scaleMode) {
    }
    warning() << Q_FUNC_INFO << "unknow Phonon::VideoWidget::ScaleMode:" << m_scaleMode;
}

qreal VideoWidget::brightness() const
{
    return m_brightness;
}

void VideoWidget::setBrightness(qreal brightness)
{
    DEBUG_BLOCK;
    if (!m_player) {
        return;
    }
    if (!enableFilterAdjust()) {
        // Add to pending adjusts
        m_pendingAdjusts.insert(QByteArray("setBrightness"), brightness);
        return;
    }

    // VLC operates within a 0.0 to 2.0 range for brightness.
    m_brightness = brightness;
    m_player->setVideoAdjust(libvlc_adjust_Brightness,
                             phononRangeToVlcRange(m_brightness, 2.0));
}

qreal VideoWidget::contrast() const
{
    return m_contrast;
}

void VideoWidget::setContrast(qreal contrast)
{
    DEBUG_BLOCK;
    if (!m_player) {
        return;
    }
    if (!enableFilterAdjust()) {
        // Add to pending adjusts
        m_pendingAdjusts.insert(QByteArray("setContrast"), contrast);
        return;
    }

    // VLC operates within a 0.0 to 2.0 range for contrast.
    m_contrast = contrast;
    m_player->setVideoAdjust(libvlc_adjust_Contrast, phononRangeToVlcRange(m_contrast, 2.0));
}

qreal VideoWidget::hue() const
{
    return m_hue;
}

void VideoWidget::setHue(qreal hue)
{
    DEBUG_BLOCK;
    if (!m_player) {
        return;
    }
    if (!enableFilterAdjust()) {
        // Add to pending adjusts
        m_pendingAdjusts.insert(QByteArray("setHue"), hue);
        return;
    }

    // VLC operates within a 0 to 360 range for hue.
    m_hue = hue;
    m_player->setVideoAdjust(libvlc_adjust_Hue,
                             static_cast<int>(phononRangeToVlcRange(m_hue, 360.0, false)));
}

qreal VideoWidget::saturation() const
{
    return m_saturation;
}

void VideoWidget::setSaturation(qreal saturation)
{
    DEBUG_BLOCK;
    if (!m_player) {
        return;
    }
    if (!enableFilterAdjust()) {
        // Add to pending adjusts
        m_pendingAdjusts.insert(QByteArray("setSaturation"), saturation);
        return;
    }

    // VLC operates within a 0.0 to 3.0 range for saturation.
    m_saturation = saturation;
    m_player->setVideoAdjust(libvlc_adjust_Saturation,
                              phononRangeToVlcRange(m_saturation, 3.0));
}

QWidget *VideoWidget::widget()
{
    return this;
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

void VideoWidget::videoWidgetSizeChanged(int width, int height)
{
    debug() << Q_FUNC_INFO << "video width" << width << "height:" << height;

    // It resizes dynamically the widget and the main window
    // Note: I didn't find another way

    QSize videoSize(width, height);
    videoSize.boundedTo(QApplication::desktop()->availableGeometry().size());

    hide();
    setVideoSize(videoSize);
    show();
}

void VideoWidget::processPendingAdjusts(bool videoAvailable)
{
    if (!videoAvailable || !m_mediaObject || !m_mediaObject->hasVideo()) {
        return;
    }

    QHashIterator<QByteArray, qreal> it(m_pendingAdjusts);
    while (it.hasNext()) {
        it.next();
        QMetaObject::invokeMethod(this, it.key().constData(), Q_ARG(qreal, it.value()));
    }
    m_pendingAdjusts.clear();
}

void VideoWidget::clearPendingAdjusts()
{
    m_pendingAdjusts.clear();
}

bool VideoWidget::enableFilterAdjust(bool adjust)
{
    DEBUG_BLOCK;
    // Need to check for MO here, because we can get called before a VOut is actually
    // around in which case we just ignore this.
    if (!m_mediaObject || !m_mediaObject->hasVideo()) {
        debug() << "no mo or no video!!!";
        return false;
    }
    if ((!m_filterAdjustActivated && adjust) ||
            (m_filterAdjustActivated && !adjust)) {
        debug() << "adjust: " << adjust;
        m_player->setVideoAdjust(libvlc_adjust_Enable, static_cast<int>(adjust));
        m_filterAdjustActivated = adjust;
    }
    return true;
}

float VideoWidget::phononRangeToVlcRange(qreal phononValue, float upperBoundary,
                                         bool shift)
{
    // VLC operates on different ranges than Phonon. Phonon always uses a range of
    // -1:1 with 0 as the default value.
    // It is therefore necessary to convert between the two schemes using sophisticated magic.
    // First the incoming range is locked between -1..1, then depending on shift
    // either normalized to 0..2 or 0..1 and finally a new value is calculated
    // depending on the upperBoundary and the normalized range.
    float value = static_cast<float>(phononValue);
    float range = 2.0; // The default normalized range will be 0..2 = 2

    // Ensure valid range
    if (value < -1.0)
        value = -1.0;
    else if (value > 1.0)
        value = 1.0;

    if (shift)
        value += 1.0; // Shift into 0..2 range
    else {
        // Chop negative value; normalize to 0..1 = range 1
        if (value < 0.0)
            value = 0.0;
        range = 1.0;
    }

    return (value * (upperBoundary/range));
}

}
} // Namespace Phonon::VLC
