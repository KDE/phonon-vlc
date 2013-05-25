/*
    Copyright (C) 2007-2008 Tanguy Krotoff <tkrotoff@gmail.com>
    Copyright (C) 2008 Lukas Durfina <lukas.durfina@gmail.com>
    Copyright (C) 2009 Fathi Boudra <fabo@kde.org>
    Copyright (C) 2009-2011 vlc-phonon AUTHORS
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

#ifndef PHONON_VLC_VIDEOWIDGET_H
#define PHONON_VLC_VIDEOWIDGET_H

#include <QWidget>

#include <phonon/videowidgetinterface.h>

#ifdef Q_OS_MAC
#include "video/mac/vlcmacwidget.h"
typedef VlcMacWidget BaseWidget;
#else
typedef QWidget BaseWidget;
#endif

#include "sinknode.h"

namespace Phonon {
namespace VLC {

class SurfacePainter;

/** \brief Implements the Phonon VideoWidget MediaNode, responsible for displaying video
 *
 * Phonon video is displayed using this widget. It implements the VideoWidgetInterface.
 * It is connected to a media object that provides the video source. Methods to control
 * video settings such as brightness or contrast are provided.
 */
class VideoWidget : public BaseWidget, public SinkNode, public VideoWidgetInterface44
{
    Q_OBJECT
    Q_INTERFACES(Phonon::VideoWidgetInterface44)
public:
    /**
     * Constructs a new VideoWidget with the given parent. The video settings members
     * are set to their default values.
     */
    VideoWidget(QWidget *parent);

    /**
     * Death to the VideoWidget!
     */
    ~VideoWidget();

    /**
     * Connects the VideoWidget to a media object by setting the video widget
     * window system identifier of the media object to that of the owned private
     * video widget. It also connects the signal from the mediaObject regarding
     * a resize of the video.
     *
     * If the mediaObject was connected to another VideoWidget, the connection is
     * lost.
     *
     * \see MediaObject
     * \param mediaObject What media object to connect to
     */
    void connectToMediaObject(MediaObject *mediaObject);

    /// \reimp
    void disconnectFromMediaObject(MediaObject *mediaObject);

    /// \reimp
    void addToMedia(Media *media);

    /**
     * \return The aspect ratio previously set for the video widget
     */
    Phonon::VideoWidget::AspectRatio aspectRatio() const;

    /**
     * Set the aspect ratio of the video.
     * VLC accepted formats are x:y (4:3, 16:9, etc...) expressing the global image aspect.
     */
    void setAspectRatio(Phonon::VideoWidget::AspectRatio aspect);

    /**
     * \return The scale mode previously set for the video widget
     */
    Phonon::VideoWidget::ScaleMode scaleMode() const;

    /**
     * Set how the video is scaled, keeping the aspect ratio into account when the video is resized.
     *
     * The ScaleMode enumeration describes how to treat aspect ratio during resizing of video.
     * \li Phonon::VideoWidget::FitInView - the video will be fitted to fill the view keeping aspect ratio
     * \li Phonon::VideoWidget::ScaleAndCrop - the video is scaled
     */
    void setScaleMode(Phonon::VideoWidget::ScaleMode scale);

    /**
     * \return The brightness previously set for the video widget
     */
    qreal brightness() const;

    /**
     * Set the brightness of the video
     */
    Q_INVOKABLE void setBrightness(qreal brightness);

    /**
     * \return The contrast previously set for the video widget
     */
    qreal contrast() const;

    /**
     * Set the contrast of the video
     */
    Q_INVOKABLE void setContrast(qreal contrast);

    /**
     * \return The hue previously set for the video widget
     */
    qreal hue() const;

    /**
     * Set the hue of the video
     */
    Q_INVOKABLE void setHue(qreal hue);

    /**
     * \return The saturation previously set for the video widget
     */
    qreal saturation() const;

    /**
     * Set the saturation of the video
     */
    Q_INVOKABLE void setSaturation(qreal saturation);

    /**
     * \return The owned widget that is used for the actual draw.
     */
    QWidget *widget();

    /// \reimp
    QSize sizeHint() const;

    void setVisible(bool visible);

private slots:
    /// Updates the sizeHint to match the native size of the video.
    /// \param hasVideo \c true when there is a video, \c false otherwise
    void updateVideoSize(bool hasVideo);

    /**
     * Sets all pending video adjusts (hue, brightness etc.) that the application
     * wanted to set before the vidoe became available.
     *
     * \param videoAvailable whether or not video is available at the time of calling
     */
    void processPendingAdjusts(bool videoAvailable);

    /**
     * Clears all pending video adjusts (hue, brightness etc.).
     */
    void clearPendingAdjusts();

protected:
    /// \reimp
    void paintEvent(QPaintEvent *event);

private:
    /**
     * Sets whether filter adjust is active or not.
     *
     * \param adjust true if adjust is supposed to be activated, false if not
     *
     * \returns whether the adjust request was accepted, if not the callee should
     *          add the request to m_pendingAdjusts for later processing once a video
     *          became available. Adjusts get accepted always except when
     *          MediaObject::hasVideo() is false, so it is not related to the
     *          actual execution of the request.
     */
    bool enableFilterAdjust(bool adjust = true);

    /**
     * Converts a Phonon range to a VLC value range.
     *
     * A Phonon range is always a qreal between -1.0 and 1.0, a VLC range however
     * can be any between 0 and 360. This functon maps the Phonon value to an
     * appropriate value within a specified target range.
     *
     * \param phononValue the incoming Phonon specific value, should be -1.0:1.0
     *                    should it however not be within that range will it be
     *                    manually locked (i.e. exceeding values become either -1.0 or 1.0)
     * \param upperBoundary the upper boundary for the target range. The lower
     *                      boundary is currently always assumed to be 0
     * \param shift whether or not to shift the Phonon range to positive values
     *        before mapping to VLC values (useful when our 0 must be a VLC 0).
     *        Please note that if you do not shift the range will be reduced to
     *        0:1, phononValue < 0 will be set to 0.
     *
     * \returns float usable to VLC
     */
    static float phononRangeToVlcRange(qreal phononValue, float upperBoundary,
                                       bool shift = true);

    /**
     * \return The snapshot of the current video frame.
     */
    QImage snapshot() const;

    /**
     * Pending video adjusts the application tried to set before we actually
     * had a video to set them on.
     */
    QHash<QByteArray, qreal> m_pendingAdjusts;

    /**
     * Original size of the video, needed for sizeHint().
     */
    QSize m_videoSize;

    Phonon::VideoWidget::AspectRatio m_aspectRatio;
    Phonon::VideoWidget::ScaleMode m_scaleMode;

    bool  m_filterAdjustActivated;
    qreal m_brightness;
    qreal m_contrast;
    qreal m_hue;
    qreal m_saturation;

    SurfacePainter *m_surfacePainter;
};

} // namespace VLC
} // namespace Phonon

#endif // PHONON_VLC_VIDEOWIDGET_H
