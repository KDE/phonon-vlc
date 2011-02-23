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

#ifndef PHONON_VLC_VIDEOWIDGET_H
#define PHONON_VLC_VIDEOWIDGET_H

#include "overlaywidget.h"
#include "sinknode.h"
#include <phonon/videowidgetinterface.h>

#include <QtCore/QMutex>

namespace Phonon
{
namespace VLC
{

namespace Experimental
{
class AvCapture;
}

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
     * \see VLCMediaObject
     * \param mediaObject What media object to connect to
     */
    void connectToMediaObject(MediaObject *mediaObject);

#ifndef PHONON_VLC_NO_EXPERIMENTAL
    /**
     * Connects the VideoWidget to an AvCapture. connectToMediaObject() is called
     * only for the video media of the AvCapture.
     *
     * \see AvCapture
     */
    void connectToAvCapture(Experimental::AvCapture *avCapture);

    /**
     * Disconnect the VideoWidget from the video media of the AvCapture.
     *
     * \see connectToAvCapture()
     */
    void disconnectFromAvCapture(Experimental::AvCapture *avCapture);
#endif // PHONON_VLC_NO_EXPERIMENTAL

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
    void setBrightness(qreal brightness);

    /**
     * \return The contrast previously set for the video widget
     */
    qreal contrast() const;

    /**
     * Set the contrast of the video
     */
    void setContrast(qreal contrast);

    /**
     * \return The hue previously set for the video widget
     */
    qreal hue() const;

    /**
     * Set the hue of the video
     */
    void setHue(qreal hue);

    /**
     * \return The saturation previously set for the video widget
     */
    qreal saturation() const;

    /**
     * Set the saturation of the video
     */
    void setSaturation(qreal saturation);

    void useCustomRender();

    /**
    * Call back function for libVLC.
    *
    * This function is public so that the compiler does not fall over.
    *
    * This function gets called by libVLC to lock the context, i.e. this
    * interface.
    *
    * \param data pointer to 'this'
    * \param buffer when this function returns this pointer should contain the
    *        address of a buffer to use for libVLC
    *
    * \return picture identifier - NOT USED -> ALWAYS NULL
    *
    * \see unlock()
    */
    static void *lock(void *data, void **bufRet);

    /**
     * Call back function for libVLC.
     *
     * This function is public so that the compiler does not fall over.
     *
     * \param data pointer to 'this'
     * \param id TODO: dont know that off the top of my head
     * \param pixels the pixel buffer of the current frame
     *
     * \see lock()
     */
    static void unlock(void *data, void *id, void *const *pixels);

    /**
     * \return The owned widget that is used for the actual draw.
     */
    QWidget *widget();

    /**
     * Sets an approximate video size to provide a size hint. It will be set
     * to the original size of the video.
     */
    void setVideoSize(const QSize &videoSize);

    /* Overloading QWidget */
    QSize sizeHint() const;

    void setVisible(bool visible);

public slots:
    void setNextFrame(const QByteArray &array, int width, int height);

private slots:
    /**
     * Handles the change in the size of the video
     */
    void videoWidgetSizeChanged(int width, int height);

protected:
    /* Overloading QWidget */
    virtual void paintEvent(QPaintEvent *event);

    /* Overloading QWidget */
    virtual void resizeEvent(QResizeEvent *event);

private:
    /**
     * Sets whether filter adjust is active or not.
     *
     * \param adjust true if adjust is supposed to be activated, false if not
     */
    void enableFilterAdjust(bool adjust = true);

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
    static float phononRangeToVlcRange(qreal phononValue, float upperBoundary = 2.0,
                                       bool shift = true);

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

    Phonon::VideoWidget::AspectRatio m_aspectRatio;

    Phonon::VideoWidget::ScaleMode m_scaleMode;

    bool  m_filterAdjustActivated;
    qreal m_brightness;
    qreal m_contrast;
    qreal m_hue;
    qreal m_saturation;
};

}
} // Namespace Phonon::VLC

#endif // PHONON_VLC_VIDEOWIDGET_H
