/*
    Copyright (C) 2007-2008 Tanguy Krotoff <tkrotoff@gmail.com>
    Copyright (C) 2008 Lukas Durfina <lukas.durfina@gmail.com>
    Copyright (C) 2009 Fathi Boudra <fabo@kde.org>
    Copyright (C) 2009-2011 vlc-phonon AUTHORS <kde-multimedia@kde.org>
    Copyright (C) 2011-2012 Harald Sitter <sitter@kde.org>

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

#include <QtGui/QPainter>
#include <QtGui/QPaintEvent>

#include <vlc/vlc.h>

#include "utils/debug.h"
#include "mediaobject.h"
#include "media.h"

#include "video/videomemorystream.h"

namespace Phonon {
namespace VLC {

#define DEFAULT_QSIZE QSize(320, 240)

class SurfacePainter : public VideoMemoryStream
{
public:
    void handlePaint(QPaintEvent *event)
    {
        // Mind that locking here is still faster than making this lockfree by
        // dispatching QEvents.
        // Plus VLC can actually skip frames as necessary.
        QMutexLocker lock(&m_mutex);
        Q_UNUSED(event);
        QPainter painter(widget);
        // When using OpenGL for the QPaintEngine drawing the same QImage twice
        // does not actually result in a texture change for one reason or another.
        // So we simply create new iamges for every event. This is plenty cheap
        // as the QImage only points to the plane data (it can't even make it
        // properly shared as it does not know that the data belongs to a QBA).
        painter.drawImage(drawFrameRect(),
                          QImage(reinterpret_cast<const uchar *>(m_plane.constData()),
                                 m_frame.width(), m_frame.height(),
                                 m_frame.bytesPerLine(), m_frame.format()));
        event->accept();
    }

    VideoWidget *widget;

private:
    virtual void *lockCallback(void **planes)
    {
        m_mutex.lock();
        planes[0] = (void *) m_plane.data();
        return 0;
    }

    virtual void unlockCallback(void *picture,void *const *planes)
    {
        Q_UNUSED(picture);
        Q_UNUSED(planes);
        m_mutex.unlock();
    }

    virtual void displayCallback(void *picture)
    {
        Q_UNUSED(picture);
        if (widget)
            widget->update();
    }

    virtual unsigned formatCallback(char *chroma,
                                    unsigned *width, unsigned *height,
                                    unsigned *pitches,
                                    unsigned *lines)
    {
        qstrcpy(chroma, "RV32");
        unsigned bufferSize = setPitchAndLines(vlc_fourcc_GetChromaDescription(VLC_CODEC_RGB32),
                                               *width, *height,
                                               pitches, lines);
        m_plane.resize(bufferSize);
        m_frame = QImage(reinterpret_cast<const uchar *>(m_plane.constData()),
                         *width, *height, pitches[0], QImage::Format_RGB32);
        return bufferSize;
    }

    virtual void formatCleanUpCallback()
    {
        // Lazy delete the object to avoid callbacks from VLC after deletion.
        if (!widget)
            delete this;
    }

    QRect scaleToAspect(QRect srcRect, int w, int h) const
    {
        float width = srcRect.width();
        float height = srcRect.width() * (float(h) / float(w));
        if (height > srcRect.height()) {
            height = srcRect.height();
            width = srcRect.height() * (float(w) / float(h));
        }
        return QRect(0, 0, (int)width, (int)height);
    }

    QRect drawFrameRect() const
    {
        QRect widgetRect = widget->rect();
        QRect drawFrameRect;
        switch (widget->aspectRatio()) {
        case Phonon::VideoWidget::AspectRatioWidget:
            drawFrameRect = widgetRect;
            // No more calculations needed.
            return drawFrameRect;
        case Phonon::VideoWidget::AspectRatio4_3:
            drawFrameRect = scaleToAspect(widgetRect, 4, 3);
            break;
        case Phonon::VideoWidget::AspectRatio16_9:
            drawFrameRect = scaleToAspect(widgetRect, 16, 9);
            break;
        case Phonon::VideoWidget::AspectRatioAuto:
            drawFrameRect = QRect(0, 0, m_frame.width(), m_frame.height());
            break;
        }

        // Scale m_drawFrameRect to fill the widget
        // without breaking aspect:
        float widgetWidth = widgetRect.width();
        float widgetHeight = widgetRect.height();
        float frameWidth = widgetWidth;
        float frameHeight = drawFrameRect.height() * float(widgetWidth) / float(drawFrameRect.width());

        switch (widget->scaleMode()) {
        case Phonon::VideoWidget::ScaleAndCrop:
            if (frameHeight < widgetHeight) {
                frameWidth *= float(widgetHeight) / float(frameHeight);
                frameHeight = widgetHeight;
            }
            break;
        case Phonon::VideoWidget::FitInView:
            if (frameHeight > widgetHeight) {
                frameWidth *= float(widgetHeight) / float(frameHeight);
                frameHeight = widgetHeight;
            }
            break;
        }
        drawFrameRect.setSize(QSize(int(frameWidth), int(frameHeight)));
        drawFrameRect.moveTo(int((widgetWidth - frameWidth) / 2.0f),
                               int((widgetHeight - frameHeight) / 2.0f));
        return drawFrameRect;
    }

    QImage m_frame;
    // We need an idependent plane as QImage needs to be forced to use the right stride/pitch.
    QByteArray m_plane;
    QMutex m_mutex;
};

VideoWidget::VideoWidget(QWidget *parent) :
    BaseWidget(parent),
    SinkNode(),
    m_videoSize(DEFAULT_QSIZE),
    m_aspectRatio(Phonon::VideoWidget::AspectRatioAuto),
    m_scaleMode(Phonon::VideoWidget::FitInView),
    m_filterAdjustActivated(false),
    m_brightness(0.0),
    m_contrast(0.0),
    m_hue(0.0),
    m_saturation(0.0),
    m_surfacePainter(0)
{
    // We want background painting so Qt autofills with black.
    setAttribute(Qt::WA_NoSystemBackground, false);

    // Required for dvdnav
#ifdef __GNUC__
#warning dragonplayer munches on our mouse events, so clicking in a DVD menu does not work - vlc 1.2 where are thu?
#endif // __GNUC__
    setMouseTracking(true);

    // setBackgroundColor
    QPalette p = palette();
    p.setColor(backgroundRole(), Qt::black);
    setPalette(p);
    setAutoFillBackground(true);
}

VideoWidget::~VideoWidget()
{
    if (m_surfacePainter)
        m_surfacePainter->widget = 0; // Lazy delete
}

void VideoWidget::handleConnectToMediaObject(MediaObject *mediaObject)
{
    connect(mediaObject, SIGNAL(hasVideoChanged(bool)),
            SLOT(updateVideoSize(bool)));
    connect(mediaObject, SIGNAL(hasVideoChanged(bool)),
            SLOT(processPendingAdjusts(bool)));
    connect(mediaObject, SIGNAL(currentSourceChanged(MediaSource)),
            SLOT(clearPendingAdjusts()));

    clearPendingAdjusts();
}

void VideoWidget::handleDisconnectFromMediaObject(MediaObject *mediaObject)
{
    // Undo all connections or path creation->destruction->creation can cause
    // duplicated connections or getting singals from two different MediaObjects.
    disconnect(mediaObject, 0, this, 0);
}

void VideoWidget::handleAddToMedia(Media *media)
{
    media->addOption(":video");

    if (!m_surfacePainter) {
#if defined(Q_OS_MAC)
        m_player->setNsObject(cocoaView());
#elif defined(Q_OS_UNIX)
        m_player->setXWindow(winId());
#elif defined(Q_OS_WIN)
        m_player->setHwnd(winId());
#endif
    }
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
    // FIXME: find a way to implement aspectratiowidget, it is meant to scale
    // and stretch (i.e. scale to window without retaining aspect ratio).
    case Phonon::VideoWidget::AspectRatioAuto:
        m_player->setVideoAspectRatio(QByteArray());
        return;
    case Phonon::VideoWidget::AspectRatio4_3:
        m_player->setVideoAspectRatio("4:3");
        return;
    case Phonon::VideoWidget::AspectRatio16_9:
        m_player->setVideoAspectRatio("16:9");
        return;
    }
    warning() << "The aspect ratio" << aspect << "is not supported by Phonon VLC.";
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
    warning() << "The scale mode" << scale << "is not supported by Phonon VLC.";
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
    // Phonon operates on -1.0 to 1.0, so we need to consider 0 to 180 as
    // 0 to 1.0 and 180 to 360 as -1 to 0.0.
    //              360/0 (0)
    //                 ___
    //                /   \
    //    270 (-.25)  |   |  90 (.25)
    //                \___/
    //             180 (1/-1)
    // (-.25 is 360 minus 90 (vlcValue of .25).
    m_hue = hue;
    const int vlcValue = static_cast<int>(phononRangeToVlcRange(qAbs(hue), 180.0, false));
    int value = 0;
    if (hue >= 0)
        value = vlcValue;
    else
        value = 360.0 - vlcValue;
    m_player->setVideoAdjust(libvlc_adjust_Hue, value);
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

QSize VideoWidget::sizeHint() const
{
    return m_videoSize;
}

void VideoWidget::updateVideoSize(bool hasVideo)
{
    if (hasVideo) {
        m_videoSize = m_player->videoSize();
        updateGeometry();
        update();
    } else
        m_videoSize = DEFAULT_QSIZE;
}

void VideoWidget::setVisible(bool visible)
{
    if (window() && window()->testAttribute(Qt::WA_DontShowOnScreen) && !m_surfacePainter) {
        debug() << "SURFACE PAINTING";
        m_surfacePainter = new SurfacePainter;
        m_surfacePainter->widget = this;
        m_surfacePainter->setCallbacks(m_player);
    }
    QWidget::setVisible(visible);
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

void VideoWidget::paintEvent(QPaintEvent *event)
{
    Q_UNUSED(event);
    if (m_surfacePainter)
        m_surfacePainter->handlePaint(event);
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

QImage VideoWidget::snapshot() const
{
    DEBUG_BLOCK;
    if (m_player)
        return m_player->snapshot();
    else
        return QImage();
}

} // namespace VLC
} // namespace Phonon
