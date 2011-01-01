/*****************************************************************************
 * libVLC backend for the Phonon library                                     *
 *                                                                           *
 * Copyright (C) 2007-2008 Tanguy Krotoff <tkrotoff@gmail.com>               *
 * Copyright (C) 2008 Lukas Durfina <lukas.durfina@gmail.com>                *
 * Copyright (C) 2009 Fathi Boudra <fabo@kde.org>                            *
 * Copyright (C) 2010 Ben Cooksley <sourtooth@gmail.com>                     *
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

#include "mediaobject.h"
#include "streamhooks.h"

#include "seekstack.h"

#include <QtCore/QUrl>
#include <QtCore/QMetaType>
#include <QtCore/QTimer>

//Time in milliseconds before sending aboutToFinish() signal
//2 seconds
static const int ABOUT_TO_FINISH_TIME = 2000;

namespace Phonon
{
namespace VLC
{

/**
 * Initializes the members, connects the private slots to their corresponding signals,
 * sets the next media source to an empty media source.
 *
 * \param p_parent A parent for the QObject
 */
MediaObject::MediaObject(QObject *p_parent)
    : QObject(p_parent)
{
    m_currentState = Phonon::StoppedState;
    m_videoWidget = NULL;
    m_prefinishEmitted = false;
    m_aboutToFinishEmitted = false;
    m_transitionTime = 0;

    // By default, no tick() signal
    // FIXME: Not implemented yet
    m_tickInterval = 0;

    qRegisterMetaType<QMultiMap<QString, QString> >("QMultiMap<QString, QString>");

    connect(this, SIGNAL(stateChanged(Phonon::State)),
            SLOT(stateChangedInternal(Phonon::State)));

    connect(this, SIGNAL(tickInternal(qint64)),
            SLOT(tickInternalSlot(qint64)));

    connect(this, SIGNAL(moveToNext()),
            SLOT(moveToNextSource()));

    m_nextSource = MediaSource(QUrl());
    m_videoWidget = NULL;
}

MediaObject::~MediaObject()
{
}

/**
 * Remembers the widget id (window system identifier) that will be
 * later passed to libVLC to draw the video on it, if this media object
 * will have video.
 *
 * \param i_widget_id The widget id to be remembered for video
 * \see VLCMediaObject::setVLCWidgetId()
 */
//void MediaObject::setVideoWidgetId(WId i_widget_id)
//{
//    i_video_widget_id = i_widget_id;
//}
//
/**
 * Remembers the widget id (window system identifier) that will be
 * later passed to libVLC to draw the video on it, if this media object
 * will have video.
 * note : I prefer to have a full access to the widget
 * \param widget the widget to pass to vlc
 * \see VLCMediaObject::setVLCWidgetId()
 */

void MediaObject::setVideoWidget(BaseWidget *widget)
{
    this->m_videoWidget = widget;
}

/**
 * If the current state is paused, it resumes playing. Else, the playback
 * is commenced. The corresponding playbackCommenced() signal is emitted.
 */
void MediaObject::play()
{
    qDebug() << __FUNCTION__;

    if (m_currentState == Phonon::PausedState) {
        resume();
    } else {
        m_prefinishEmitted = false;
        m_aboutToFinishEmitted = false;
        // Play the file
        playInternal();
    }

    emit playbackCommenced();
}

/**
 * Pushes a seek command to the SeekStack for this media object. The SeekStack then
 * calls seekInternal() when it's popped.
 */
void MediaObject::seek(qint64 milliseconds)
{
    static SeekStack *p_stack = new SeekStack(this);

    p_stack->pushSeek(milliseconds);

    qint64 currentTime = this->currentTime();
    qint64 totalTime = this->totalTime();

    if (currentTime < totalTime - m_prefinishMark) {
        m_prefinishEmitted = false;
    }
    if (currentTime < totalTime - ABOUT_TO_FINISH_TIME) {
        m_aboutToFinishEmitted = false;
    }
}

/**
 * Checks when the tick(), prefinishMarkReached(), aboutToFinish() signals need to
 * be emitted and emits them if necessary.
 *
 * \param currentTime The current play time for the media, in miliseconds.
 */
void MediaObject::tickInternalSlot(qint64 currentTime)
{
    qint64 totalTime = this->totalTime();

    if (m_tickInterval > 0) {
        // If _tickInternal == 0 means tick() signal is disabled
        // Default is _tickInternal = 0
        emit tick(currentTime);
    }

    if (m_currentState == Phonon::PlayingState) {
        if (currentTime >= totalTime - m_prefinishMark) {
            if (!m_prefinishEmitted) {
                m_prefinishEmitted = true;
                emit prefinishMarkReached(totalTime - currentTime);
            }
        }
        if (totalTime > -1 && currentTime >= totalTime - ABOUT_TO_FINISH_TIME) {
            emitAboutToFinish();
        }
    }
}

/**
 * Changes the current state to buffering and calls loadMediaInternal()
 * \param filename A MRL from the media source
 */
void MediaObject::loadMedia(const QString &filename)
{
    // Default MediaObject state is Phonon::BufferingState
    emit stateChanged(Phonon::BufferingState);

    // Load the media
    loadMediaInternal(filename);
}

void MediaObject::resume()
{
    pause();
}

/**
 * \return The interval between successive tick() signals. If set to 0, the emission
 * of these signals is disabled.
 */
qint32 MediaObject::tickInterval() const
{
    return m_tickInterval;
}

/**
 * Sets the interval between successive tick() signals. If set to 0, it disables the
 * emission of these signals.
 */
void MediaObject::setTickInterval(qint32 tickInterval)
{
    m_tickInterval = tickInterval;
//    if (_tickInterval <= 0) {
//        _tickTimer->setInterval(50);
//    } else {
//        _tickTimer->setInterval(_tickInterval);
//    }
}

/**
 * \return The current time of the media, depending on the current state.
 * If the current state is stopped or loading, 0 is returned.
 * If the current state is error or unknown, -1 is returned.
 */
qint64 MediaObject::currentTime() const
{
    qint64 time = -1;
    Phonon::State st = state();

    switch (st) {
    case Phonon::PausedState:
        time = currentTimeInternal();
        break;
    case Phonon::BufferingState:
        time = currentTimeInternal();
        break;
    case Phonon::PlayingState:
        time = currentTimeInternal();
        break;
    case Phonon::StoppedState:
        time = 0;
        break;
    case Phonon::LoadingState:
        time = 0;
        break;
    case Phonon::ErrorState:
        time = -1;
        break;
    default:
        qCritical() << __FUNCTION__ << "Error: unsupported Phonon::State:" << st;
    }

    return time;
}

/**
 * \return The current state for this media object.
 */
Phonon::State MediaObject::state() const
{
    return m_currentState;
}

/**
 * All errors are categorized as normal errors.
 */
Phonon::ErrorType MediaObject::errorType() const
{
    return Phonon::NormalError;
}

/**
 * \return The current media source for this media object.
 */
MediaSource MediaObject::source() const
{
    return m_mediaSource;
}

/**
 * Sets the current media source for this media object. Depending on the source type,
 * the media object loads the specified media. The MRL is passed to loadMedia(), if the media
 * is not a stream. If it is a stream, loadStream() is used. The currentSourceChanged() signal
 * is emitted.
 *
 * Supported media source types:
 * \li local files
 * \li URL
 * \li discs (CD, DVD, VCD)
 * \li capture devices (V4L)
 * \li streams
 *
 * \param source The media source that will become the current source.
 *
 * \see loadMedia()
 * \see loadStream()
 */
void MediaObject::setSource(const MediaSource &source)
{
    qDebug() << __FUNCTION__;

    m_mediaSource = source;

    QByteArray driverName;
    QString deviceName;

    switch (source.type()) {
    case MediaSource::Invalid:
        qCritical() << __FUNCTION__ << "Error: MediaSource Type is Invalid:" << source.type();
        break;
    case MediaSource::Empty:
        qCritical() << __FUNCTION__ << "Error: MediaSource is empty.";
        break;
    case MediaSource::LocalFile:
    case MediaSource::Url: {
        qCritical() << __FUNCTION__ << "yeap, 'tis a local file or url" << source.url().scheme();
        const QString &mrl = (source.url().scheme() == QLatin1String("") ?
                              QLatin1String("file://") + source.url().toString() :
                              source.url().toString());
        loadMedia(mrl);
    }
    break;
    /*    case MediaSource::Url:
            loadMedia(mediaSource.url().toEncoded());
            break;*/
    case MediaSource::Disc:
        switch (source.discType()) {
        case Phonon::NoDisc:
            qCritical() << __FUNCTION__
                        << "Error: the MediaSource::Disc doesn't specify which one (Phonon::NoDisc)";
            return;
        case Phonon::Cd:
            qDebug() << m_mediaSource.deviceName();
            loadMedia("cdda://" + m_mediaSource.deviceName());
            break;
        case Phonon::Dvd:
            loadMedia("dvd://" + m_mediaSource.deviceName());
            break;
        case Phonon::Vcd:
            loadMedia(m_mediaSource.deviceName());
            break;
        default:
            qCritical() << __FUNCTION__ << "Error: unsupported MediaSource::Disc:" << source.discType();
            break;
        }
        break;
#ifndef PHONON_VLC_NO_EXPERIMENTAL
    case MediaSource::CaptureDevice:
        if (source.deviceAccessList().isEmpty()) {
            qCritical() << __FUNCTION__ << "No device access list for this capture device";
            break;
        }

        // TODO try every device in the access list until it works, not just the first one
        driverName = source.deviceAccessList().first().first;
        deviceName = source.deviceAccessList().first().second;

        if (driverName == "v4l2") {
            loadMedia("v4l2://" + deviceName);
        } else if (driverName == "alsa") {
            loadMedia("alsa://" + deviceName);
        } else {
            qCritical() << __FUNCTION__ << "Error: unsupported MediaSource::CaptureDevice:" << driverName;
            break;
        }

        break;
#endif // PHONON_VLC_NO_EXPERIMENTAL
    case MediaSource::Stream:
        loadStream();
        break;
    default:
        qCritical() << __FUNCTION__ << "Error: Unsupported MediaSource Type:" << source.type();
        break;
    }

    emit currentSourceChanged(m_mediaSource);
}

/**
 * Loads a stream specified by the current media source. It creates a stream reader
 * for the media source. Then, loadMedia() is called. The stream callbacks are set up
 * using special options. These callbacks are implemented in streamhooks.cpp, and
 * are basically part of StreamReader.
 *
 * \see StreamReader
 */
void MediaObject::loadStream()
{
    m_streamReader = new StreamReader(m_mediaSource);

    loadMedia("imem://");
}

/**
 * Sets the media source that will replace the current one, after the playback for it finishes.
 */
void MediaObject::setNextSource(const MediaSource &source)
{
    qDebug() << __FUNCTION__;
    m_nextSource = source;
}

qint32 MediaObject::prefinishMark() const
{
    return m_prefinishMark;
}

void MediaObject::setPrefinishMark(qint32 msecToEnd)
{
    m_prefinishMark = msecToEnd;
    if (currentTime() < totalTime() - m_prefinishMark) {
        // Not about to finish
        m_prefinishEmitted = false;
    }
}

qint32 MediaObject::transitionTime() const
{
    return m_transitionTime;
}

void MediaObject::setTransitionTime(qint32 time)
{
    m_transitionTime = time;
}

void MediaObject::emitAboutToFinish()
{
    if (!m_aboutToFinishEmitted) {
        // Track is about to finish
        m_aboutToFinishEmitted = true;
        emit aboutToFinish();
    }
}

/**
 * If the new state is different from the current state, the current state is
 * changed and the corresponding signal is emitted.
 */
void MediaObject::stateChangedInternal(Phonon::State newState)
{
    qDebug() << __FUNCTION__ << "newState:" << PhononStateToString(newState)
             << "previousState:" << PhononStateToString(m_currentState) ;

    if (newState == m_currentState) {
        // State not changed
        return;
    } else if (checkGaplessWaiting()) {
        // This is a no-op, warn that we are....
        qDebug() << __FUNCTION__ << "no-op gapless item awaiting in queue - "
                 << m_nextSource.type() ;
        return;
    }

    // State changed
    Phonon::State previousState = m_currentState;
    m_currentState = newState;
    emit stateChanged(m_currentState, previousState);
}

/**
 * \return A string representation of a Phonon state.
 */
QString MediaObject::PhononStateToString(Phonon::State newState)
{
    QString stream;
    switch (newState) {
    case Phonon::ErrorState:
        stream += "ErrorState";
        break;
    case Phonon::LoadingState:
        stream += "LoadingState";
        break;
    case Phonon::StoppedState:
        stream += "StoppedState";
        break;
    case Phonon::PlayingState:
        stream += "PlayingState";
        break;
    case Phonon::BufferingState:
        stream += "BufferingState";
        break;
    case Phonon::PausedState:
        stream += "PausedState";
        break;
    }
    return stream;
}

/**
 * If the next media source is valid, the current source is replaced and playback is commenced.
 * The next source is set to an empty source.
 *
 * \see setNextSource()
 */
void MediaObject::moveToNextSource()
{
    if (m_nextSource.type() == MediaSource::Invalid) {
        // No item is scheduled to be next...
        return;
    }

    setSource(m_nextSource);
    play();
    m_nextSource = MediaSource(QUrl());
}

bool MediaObject::checkGaplessWaiting()
{
    return m_nextSource.type() != MediaSource::Invalid && m_nextSource.type() != MediaSource::Empty;
}


}
} // Namespace Phonon::VLC
