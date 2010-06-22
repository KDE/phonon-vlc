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
namespace VLC {

/**
 * Initializes the members, connects the private slots to their corresponding signals,
 * sets the next media source to an empty media source.
 *
 * \param p_parent A parent for the QObject
 */
MediaObject::MediaObject(QObject *p_parent)
        : QObject(p_parent)
{
    currentState = Phonon::StoppedState;
    i_video_widget_id = 0;
    b_prefinish_mark_reached_emitted = false;
    b_about_to_finish_emitted = false;
    i_transition_time = 0;

    // By default, no tick() signal
    // FIXME: Not implemented yet
    i_tick_interval = 0;

    qRegisterMetaType<QMultiMap<QString, QString> >("QMultiMap<QString, QString>");

    connect(this, SIGNAL(stateChanged(Phonon::State)),
            SLOT(stateChangedInternal(Phonon::State)));

    connect(this, SIGNAL(tickInternal(qint64)),
            SLOT(tickInternalSlot(qint64)));

    connect(this, SIGNAL(moveToNext()),
            SLOT(moveToNextSource()));

    p_next_source = MediaSource(QUrl());
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
void MediaObject::setVideoWidgetId(WId i_widget_id)
{
    i_video_widget_id = i_widget_id;
}

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
    this->p_video_widget = widget;
}

/**
 * If the current state is paused, it resumes playing. Else, the playback
 * is commenced. The corresponding playbackCommenced() signal is emitted.
 */
void MediaObject::play()
{
    qDebug() << __FUNCTION__;

    if (currentState == Phonon::PausedState) {
        resume();
    } else {
        b_prefinish_mark_reached_emitted = false;
        b_about_to_finish_emitted = false;
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

    if (currentTime < totalTime - i_prefinish_mark) {
        b_prefinish_mark_reached_emitted = false;
    }
    if (currentTime < totalTime - ABOUT_TO_FINISH_TIME) {
        b_about_to_finish_emitted = false;
    }
}

/**
 * Checks when the tick(), prefinishMarkReached(), aboutToFinish() signals need to
 * be emitted and emits them if neccessary.
 *
 * \param currentTime The current play time for the media, in miliseconds.
 */
void MediaObject::tickInternalSlot(qint64 currentTime)
{
    qint64 totalTime = this->totalTime();

    if (i_tick_interval > 0) {
        // If _tickInternal == 0 means tick() signal is disabled
        // Default is _tickInternal = 0
        emit tick(currentTime);
    }

    if (currentState == Phonon::PlayingState) {
        if (currentTime >= totalTime - i_prefinish_mark) {
            if (!b_prefinish_mark_reached_emitted) {
                b_prefinish_mark_reached_emitted = true;
                emit prefinishMarkReached(totalTime - currentTime);
            }
        }
        if (currentTime >= totalTime - ABOUT_TO_FINISH_TIME) {
            if (!b_about_to_finish_emitted) {
                // Track is about to finish
                b_about_to_finish_emitted = true;
                emit aboutToFinish();
            }
        }
    }
}

/**
 * Changes the current state to buffering and calls loadMediaInternal()
 * \param filename A MRL from the media source
 */
void MediaObject::loadMedia(const QString & filename)
{
    // Default MediaObject state is Phonon::BufferingState
    emit stateChanged( Phonon::BufferingState );

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
    return i_tick_interval;
}

/**
 * Sets the interval between successive tick() signals. If set to 0, it disables the
 * emission of these signals.
 */
void MediaObject::setTickInterval(qint32 tickInterval)
{
    i_tick_interval = tickInterval;
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
    return currentState;
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
    return mediaSource;
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
void MediaObject::setSource(const MediaSource & source)
{
    qDebug() << __FUNCTION__;

    mediaSource = source;

    switch (source.type()) {
    case MediaSource::Invalid:
        qCritical() << __FUNCTION__ << "Error: MediaSource Type is Invalid:" << source.type();
        break;
    case MediaSource::Empty:
        qCritical() << __FUNCTION__ << "Error: MediaSource is empty.";
        break;
    case MediaSource::LocalFile:
    case MediaSource::Url:
        {
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
            qDebug() << mediaSource.deviceName();
            loadMedia(mediaSource.deviceName());
            break;
        case Phonon::Dvd:
            loadMedia("dvd://" + mediaSource.deviceName());
            break;
        case Phonon::Vcd:
            loadMedia(mediaSource.deviceName());
            break;
        default:
            qCritical() << __FUNCTION__ << "Error: unsupported MediaSource::Disc:" << source.discType();
            break;
        }
        break;
    case MediaSource::CaptureDeviceSource:
        switch (source.captureDeviceType()) {
        case Phonon::V4LVideo:
            loadMedia("v4l2://" + mediaSource.deviceName());
            break;
        case Phonon::V4LAudio:
            loadMedia("v4l2://" + mediaSource.deviceName());
            break;
        default:
            qCritical() << __FUNCTION__ << "Error: unsupported MediaSource::CaptureDevice:" << source.captureDeviceType();
            break;
        }
        break;
    case MediaSource::Stream:
        if (!source.url().isEmpty())
            loadStream();
        break;
    default:
        qCritical() << __FUNCTION__ << "Error: Unsupported MediaSource Type:" << source.type();
        break;
    }

    emit currentSourceChanged(mediaSource);
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
    streamReader = new StreamReader(mediaSource);

#ifdef _MSC_VER
    char formatstr[] = "0x%p";
#else
    char formatstr[] = "%p";
#endif

    char rptr[64];
    snprintf(rptr, sizeof(rptr), formatstr, streamReadCallback);

    char rdptr[64];
    snprintf(rdptr, sizeof(rdptr), formatstr, streamReadDoneCallback);

    char sptr[64];
    snprintf(sptr, sizeof(sptr), formatstr, streamSeekCallback);

    char srptr[64];
    snprintf(srptr, sizeof(srptr), formatstr, streamReader);

    loadMedia( "imem/ffmpeg://" );

    setOption("imem-cat=4");
    setOption(QString("imem-data=%1").arg(srptr));
    setOption(QString("imem-get=%1").arg(rptr));
    setOption(QString("imem-release=%1").arg(rdptr));
    setOption(QString("imem-seek=%1").arg(sptr));
}

/**
 * Sets the media source that will replace the current one, after the playback for it finishes.
 */
void MediaObject::setNextSource(const MediaSource & source)
{
    qDebug() << __FUNCTION__;
    p_next_source = source;
}

qint32 MediaObject::prefinishMark() const
{
    return i_prefinish_mark;
}

void MediaObject::setPrefinishMark(qint32 msecToEnd)
{
    i_prefinish_mark = msecToEnd;
    if (currentTime() < totalTime() - i_prefinish_mark) {
        // Not about to finish
        b_prefinish_mark_reached_emitted = false;
    }
}

qint32 MediaObject::transitionTime() const
{
    return i_transition_time;
}

void MediaObject::setTransitionTime(qint32 time)
{
    i_transition_time = time;
}

/**
 * If the new state is different from the current state, the current state is
 * changed and the corresponding signal is emitted.
 */
void MediaObject::stateChangedInternal(Phonon::State newState)
{
    qDebug() << __FUNCTION__ << "newState:" << PhononStateToString( newState )
    << "previousState:" << PhononStateToString( currentState ) ;

    if (newState == currentState) {
        // State not changed
        return;
    } else if ( checkGaplessWaiting() ) {
        // This is a no-op, warn that we are....
        qDebug() << __FUNCTION__ << "no-op gapless item awaiting in queue - "
                 << p_next_source.type() ;
        return;
    }

    // State changed
    Phonon::State previousState = currentState;
    currentState = newState;
    emit stateChanged(currentState, previousState);
}

/**
 * \return A string representation of a Phonon state.
 */
QString MediaObject::PhononStateToString( Phonon::State newState )
{
    QString stream;
    switch(newState)
    {
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
    if( p_next_source.type() == MediaSource::Invalid ) {
        // No item is scheduled to be next...
        return;
    }

    setSource( p_next_source );
    play();
    p_next_source = MediaSource(QUrl());
}

bool MediaObject::checkGaplessWaiting()
{
    return p_next_source.type() != MediaSource::Invalid && p_next_source.type() != MediaSource::Empty;
}


}
} // Namespace Phonon::VLC
