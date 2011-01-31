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

#include <QtCore/QUrl>
#include <QtCore/QMetaType>
#include <QtCore/QTimer>

#include "vlc/vlc.h"

#include "streamhooks.h"

#include "seekstack.h"
#include "sinknode.h"

extern libvlc_instance_t *vlc_instance;

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
    , m_videoWidget(0)
    , m_nextSource(MediaSource(QUrl()))
    , m_currentState(Phonon::StoppedState)
    , m_prefinishEmitted(false)
    , m_aboutToFinishEmitted(false)
    // By default, no tick() signal
    // FIXME: Not implemented yet
    , m_tickInterval(0)
    , m_transitionTime(0)
{
    qRegisterMetaType<QMultiMap<QString, QString> >("QMultiMap<QString, QString>");

    connect(this, SIGNAL(stateChanged(Phonon::State)),
            SLOT(stateChangedInternal(Phonon::State)));

    connect(this, SIGNAL(tickInternal(qint64)),
            SLOT(tickInternalSlot(qint64)));

    connect(this, SIGNAL(moveToNext()),
            SLOT(moveToNextSource()));

//    m_nextSource = MediaSource(QUrl());

    // Create an empty Media Player object
    p_vlc_media_player = libvlc_media_player_new(vlc_instance);
    if (!p_vlc_media_player) {
        qDebug() << "libvlc exception:" << libvlc_errmsg();
    }
    p_vlc_media_player_event_manager = 0;
    connectToPlayerVLCEvents();

    // Media
    p_vlc_media = 0;
    p_vlc_media_event_manager = 0;

    // Media Discoverer
    p_vlc_media_discoverer = 0;
    p_vlc_media_discoverer_event_manager = 0;

    // default to -1, so that streams won't break and to comply with the docs (-1 if unknown)
    m_totalTime = -1;
    m_hasVideo = false;
    m_seekable = false;
    m_seekpoint = 0;

    connect(this, SIGNAL(metaDataNeedsRefresh()), this, SLOT(updateMetaData()));
    connect(this, SIGNAL(durationChanged(qint64)), this, SLOT(updateDuration(qint64)));
}

MediaObject::~MediaObject()
{
    unloadMedia();

    libvlc_media_player_stop(p_vlc_media_player); // ensure that we are stopped
    libvlc_media_player_release(p_vlc_media_player);
}

/**
 * Remembers the widget id (window system identifier) that will be
 * later passed to libVLC to draw the video on it, if this media object
 * will have video.
 *
 * \param i_widget_id The widget id to be remembered for video
 * \see MediaObject::setVLCWidgetId()
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
 * \see MediaObject::setVLCWidgetId()
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
    m_streamReader = new StreamReader(m_mediaSource, this);

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


/**
 * Uninitializes the media
 */
void MediaObject::unloadMedia()
{
//    if( p_vlc_media_player ) {
//        libvlc_media_player_release(p_vlc_media_player);
//        p_vlc_media_player = 0;
//    }

    if (p_vlc_media) {
        libvlc_media_release(p_vlc_media);
        p_vlc_media = 0;
    }
}

/**
 * Sets p_current file to the desired filename. It will be loaded in playInternal().
 *
 * \see playInternal()
 */
void MediaObject::loadMediaInternal(const QString &filename)
{
    qDebug() << __FUNCTION__ << filename;

    m_currentFile = QUrl::toPercentEncoding(filename, ":/?=&,@");

    // Why is this needed???
    emit stateChanged(Phonon::StoppedState);
}

/**
 * Configures the VLC Media Player to draw the video on the desired widget. The actual function
 * call depends on the platform.
 *
 * \see setVideoWidgetId()
 */
void MediaObject::setVLCVideoWidget()
{
    // Nothing to do if there is no video widget
    if (!m_videoWidget)
        return;

    // Get our media player to use our window
#if defined(Q_OS_MAC)
    libvlc_media_player_set_nsobject(p_vlc_media_player, m_videoWidget->cocoaView());
#elif defined(Q_OS_UNIX)
    libvlc_media_player_set_xwindow(p_vlc_media_player, m_videoWidget->winId());
#elif defined(Q_OS_WIN)
    libvlc_media_player_set_hwnd(p_vlc_media_player, m_videoWidget->winId());
#endif
}

/**
 * This method actually calls the functions needed to begin playing the media.
 * If another media is already playing, it is discarded. The new media filename is set
 * with loadMediaInternal(). A new VLC Media is created and set into the VLC Media Player.
 * All the connected sink nodes are connected to the new media. It connects the media object
 * to the events for the VLC Media, updates the meta-data, sets up the video widget id, and
 * starts playing.
 *
 * \see loadMediaInternal()
 * \see connectToMediaVLCEvents()
 * \see updateMetaData()
 * \see setVLCWidgetId()
 */
void MediaObject::playInternal()
{
    if (p_vlc_media) {  // We are changing media, discard the old one
        libvlc_media_release(p_vlc_media);
        p_vlc_media = 0;
    }


    m_totalTime = -1;

    // Create a media with the given MRL
    p_vlc_media = libvlc_media_new_location(vlc_instance, m_currentFile);
    if (!p_vlc_media) {
        qDebug() << "libvlc exception:" << libvlc_errmsg();
    }

    if (m_currentFile == "imem://") { // Set callbacks for stream reading using imem
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
            snprintf(srptr, sizeof(srptr), formatstr, m_streamReader);


            setOption("imem-cat=4");
            setOption(QString("imem-data=%1").arg(srptr));
            setOption(QString("imem-get=%1").arg(rptr));
            setOption(QString("imem-release=%1").arg(rdptr));
            setOption(QString("imem-seek=%1").arg(sptr));

            // if stream has known size, we may pass it
            // imem module will use it and pass it to demux
            if( m_streamReader->streamSize() > 0 )
                setOption(QString("imem-size=%1").arg( m_streamReader->streamSize() ));

    }


    foreach(SinkNode * sink, m_sinks) {
        sink->addToMedia(p_vlc_media);
    }

    // Set the media that will be used by the media player
    libvlc_media_player_set_media(p_vlc_media_player, p_vlc_media);

    // connectToMediaVLCEvents() at the end since it needs to be done for each new libvlc_media_t instance
    connectToMediaVLCEvents();

    // Get meta data (artist, title, etc...)
    updateMetaData();

    // Update available audio channels/subtitles/angles/chapters/etc...
    // i.e everything from MediaController
    // There is no audio channel/subtitle/angle/chapter events inside libvlc
    // so let's send our own events...
    // This will reset the GUI
    resetMediaController();

    // Set up the widget id for libVLC if there is a video widget available
    setVLCVideoWidget();

    // Play
    if (libvlc_media_player_play(p_vlc_media_player)) {
        qDebug() << "libvlc exception:" << libvlc_errmsg();
    }

    if (m_seekpoint != 0) {  // Workaround that seeking needs to work before the file is being played...
        seekInternal(m_seekpoint);
        m_seekpoint = 0;
    }

    emit stateChanged(Phonon::PlayingState);
}

/**
 * Pauses the playback for the media player.
 */
void MediaObject::pause()
{
    libvlc_media_t *media = libvlc_media_player_get_media(p_vlc_media_player);

    if (state() == Phonon::PausedState) {
#ifdef __GNUC__
#warning HACK!!!! -> after loading we are in pause, even though no media is loaded
#endif
        if (media == 0) {
            // Nothing playing yet -> play
            playInternal();
        } else {
            // Resume
            libvlc_media_player_set_pause(p_vlc_media_player, 0);
            emit stateChanged(Phonon::PlayingState);
        }
    } else {
        // Pause
        libvlc_media_player_set_pause(p_vlc_media_player, 1);
        emit stateChanged(Phonon::PausedState);
    }

}

/**
 * Sets the next media source to an empty one and stops playback.
 */
void MediaObject::stop()
{
    m_nextSource = MediaSource(QUrl());
    libvlc_media_player_stop(p_vlc_media_player);
//    unloadMedia();
    emit stateChanged(Phonon::StoppedState);
}

/**
 * Seeks to the required position. If the state is not playing, the seek position is remembered.
 */
void MediaObject::seekInternal(qint64 milliseconds)
{
    if (state() != Phonon::PlayingState) {  // Is we aren't playing, seeking is invalid...
        m_seekpoint = milliseconds;
    }

    qDebug() << __FUNCTION__ << milliseconds;
    libvlc_media_player_set_time(p_vlc_media_player, milliseconds);
}

/**
 * \return An error message with the last libVLC error.
 */
QString MediaObject::errorString() const
{
    return libvlc_errmsg();
}

bool MediaObject::hasVideo() const
{
    return m_hasVideo;
}

bool MediaObject::isSeekable() const
{
    return m_seekable;
}

/**
 * Connect libvlc_callback() to all VLC media player events.
 *
 * \see libvlc_callback()
 * \see connectToMediaVLCEvents()
 */
void MediaObject::connectToPlayerVLCEvents()
{
    // Get the event manager from which the media player send event
    p_vlc_media_player_event_manager = libvlc_media_player_event_manager(p_vlc_media_player);
    libvlc_event_type_t eventsMediaPlayer[] = {
        libvlc_MediaPlayerPlaying,
        libvlc_MediaPlayerPaused,
        libvlc_MediaPlayerEndReached,
        libvlc_MediaPlayerStopped,
        libvlc_MediaPlayerEncounteredError,
        libvlc_MediaPlayerTimeChanged,
        libvlc_MediaPlayerTitleChanged,
        //libvlc_MediaPlayerPositionChanged, // What does this event do???
        libvlc_MediaPlayerSeekableChanged,
        //libvlc_MediaPlayerPausableChanged, // Phonon has no use for this
    };
    int i_nbEvents = sizeof(eventsMediaPlayer) / sizeof(*eventsMediaPlayer);
    for (int i = 0; i < i_nbEvents; i++) {
        libvlc_event_attach(p_vlc_media_player_event_manager, eventsMediaPlayer[i],
                            libvlc_callback, this);
    }
}

/**
 * Connect libvlc_callback() to all VLC media events.
 *
 * \see libvlc_callback()
 * \see connectToPlayerVLCEvents()
 */
void MediaObject::connectToMediaVLCEvents()
{
    // Get event manager from media descriptor object
    p_vlc_media_event_manager = libvlc_media_event_manager(p_vlc_media);
    libvlc_event_type_t eventsMedia[] = {
        libvlc_MediaMetaChanged,
        //libvlc_MediaSubItemAdded, // Could this be used for Audio Channels / Subtitles / Chapter info??
        libvlc_MediaDurationChanged,
        //libvlc_MediaFreed, // Not needed is this?
        //libvlc_MediaStateChanged, // We don't use this? Could we??
    };
    int i_nbEvents = sizeof(eventsMedia) / sizeof(*eventsMedia);
    for (int i = 0; i < i_nbEvents; i++) {
        libvlc_event_attach(p_vlc_media_event_manager, eventsMedia[i], libvlc_callback, this);
    }

    // Get event manager from media service discoverer object
    // FIXME why libvlc_media_discoverer_event_manager() does not take a libvlc_exception_t ?
//    p_vlc_media_discoverer_event_manager = libvlc_media_discoverer_event_manager(p_vlc_media_discoverer);
//    libvlc_event_type_t eventsMediaDiscoverer[] = {
//        libvlc_MediaDiscovererStarted,
//        libvlc_MediaDiscovererEnded
//    };
//    nbEvents = sizeof(eventsMediaDiscoverer) / sizeof(*eventsMediaDiscoverer);
//    for (int i = 0; i < nbEvents; i++) {
//        libvlc_event_attach(p_vlc_media_discoverer_event_manager, eventsMediaDiscoverer[i], libvlc_callback, this, vlc_exception);
//    }
}

/**
 * Libvlc callback.
 *
 * Receive all vlc events.
 *
 * Most of the events trigger the emission of one or more signals from the media object.
 *
 * \warning This code will be owned by the libVLC thread.
 *
 * \see connectToMediaVLCEvents()
 * \see connectToPlayerVLCEvents()
 * \see libvlc_event_attach()
 */
void MediaObject::libvlc_callback(const libvlc_event_t *p_event, void *p_user_data)
{
    static int i_first_time_media_player_time_changed = 0;
    static bool b_media_player_title_changed = false;

    MediaObject *const p_vlc_mediaObject = (MediaObject *) p_user_data;

//    qDebug() << (int)p_vlc_mediaObject << "event:" << libvlc_event_type_name(p_event->type);

    // Media player events
    if (p_event->type == libvlc_MediaPlayerSeekableChanged) {
        const bool b_seekable = libvlc_media_player_is_seekable(p_vlc_mediaObject->p_vlc_media_player);
        if (b_seekable != p_vlc_mediaObject->m_seekable) {
            p_vlc_mediaObject->m_seekable = b_seekable;
            emit p_vlc_mediaObject->seekableChanged(p_vlc_mediaObject->m_seekable);
        }
    }
    if (p_event->type == libvlc_MediaPlayerTimeChanged) {

        i_first_time_media_player_time_changed++;

        // FIXME It is ugly. It should be solved by some events in libvlc
        if (i_first_time_media_player_time_changed == 15) {
            // Update metadata
            p_vlc_mediaObject->updateMetaData();

            // Get current video width and height
            uint width = 0;
            uint height = 0;
            libvlc_video_get_size(p_vlc_mediaObject->p_vlc_media_player, 0, &width, &height);
            emit p_vlc_mediaObject->videoWidgetSizeChanged(width, height);

            // Does this media player have a video output
            const bool b_has_video = libvlc_media_player_has_vout(p_vlc_mediaObject->p_vlc_media_player);
            if (b_has_video != p_vlc_mediaObject->m_hasVideo) {
                p_vlc_mediaObject->m_hasVideo = b_has_video;
                emit p_vlc_mediaObject->hasVideoChanged(p_vlc_mediaObject->m_hasVideo);
            }

            if (b_has_video) {
                qDebug() << Q_FUNC_INFO << "hasVideo!";

                // Give info about audio tracks
                p_vlc_mediaObject->refreshAudioChannels();
                // Give info about subtitle tracks
                p_vlc_mediaObject->refreshSubtitles();

                // Get movie chapter count
                // It is not a title/chapter media if there is no chapter
                if (libvlc_media_player_get_chapter_count(
                            p_vlc_mediaObject->p_vlc_media_player) > 0) {
                    // Give info about title
                    // only first time, no when title changed
                    if (!b_media_player_title_changed) {
                        libvlc_track_description_t *p_info = libvlc_video_get_title_description(
                                p_vlc_mediaObject->p_vlc_media_player);
                        while (p_info) {
                            p_vlc_mediaObject->titleAdded(p_info->i_id, p_info->psz_name);
                            p_info = p_info->p_next;
                        }
                        libvlc_track_description_release(p_info);
                    }

                    // Give info about chapters for actual title 0
                    if (b_media_player_title_changed)
                        p_vlc_mediaObject->refreshChapters(libvlc_media_player_get_title(
                                                               p_vlc_mediaObject->p_vlc_media_player));
                    else {
                        p_vlc_mediaObject->refreshChapters(0);
                    }
                }
                if (b_media_player_title_changed) {
                    b_media_player_title_changed = false;
                }
            }

            // Bugfix with Qt mediaplayer example
            // Now we are in playing state
            emit p_vlc_mediaObject->stateChanged(Phonon::PlayingState);
        }

        emit p_vlc_mediaObject->tickInternal(p_vlc_mediaObject->currentTime());
    }

    if (p_event->type == libvlc_MediaPlayerBuffering) {
        emit p_vlc_mediaObject->stateChanged(Phonon::BufferingState);
    }

    if (p_event->type == libvlc_MediaPlayerPlaying) {
        if (p_vlc_mediaObject->state() != Phonon::BufferingState) {
            // Bugfix with Qt mediaplayer example
            emit p_vlc_mediaObject->stateChanged(Phonon::PlayingState);
        }
    }

    if (p_event->type == libvlc_MediaPlayerPaused) {
        emit p_vlc_mediaObject->stateChanged(Phonon::PausedState);
    }

    if (p_event->type == libvlc_MediaPlayerEndReached && !p_vlc_mediaObject->checkGaplessWaiting()) {
        i_first_time_media_player_time_changed = 0;
        p_vlc_mediaObject->resetMediaController();
        p_vlc_mediaObject->emitAboutToFinish();
        emit p_vlc_mediaObject->finished();
        emit p_vlc_mediaObject->stateChanged(Phonon::StoppedState);
    } else if (p_event->type == libvlc_MediaPlayerEndReached) {
        emit p_vlc_mediaObject->moveToNext();
    }

    if (p_event->type == libvlc_MediaPlayerEncounteredError && !p_vlc_mediaObject->checkGaplessWaiting()) {
        i_first_time_media_player_time_changed = 0;
        p_vlc_mediaObject->resetMediaController();
        emit p_vlc_mediaObject->finished();
        emit p_vlc_mediaObject->stateChanged(Phonon::ErrorState);
    } else if (p_event->type == libvlc_MediaPlayerEncounteredError) {
        emit p_vlc_mediaObject->moveToNext();
    }

    if (p_event->type == libvlc_MediaPlayerStopped && !p_vlc_mediaObject->checkGaplessWaiting()) {
        i_first_time_media_player_time_changed = 0;
        p_vlc_mediaObject->resetMediaController();
        emit p_vlc_mediaObject->stateChanged(Phonon::StoppedState);
    }

    if (p_event->type == libvlc_MediaPlayerTitleChanged) {
        i_first_time_media_player_time_changed = 0;
        b_media_player_title_changed = true;
    }

    // Media events

    if (p_event->type == libvlc_MediaDurationChanged) {
        emit p_vlc_mediaObject->durationChanged(p_event->u.media_duration_changed.new_duration);
    }

    if (p_event->type == libvlc_MediaMetaChanged) {
        emit p_vlc_mediaObject->metaDataNeedsRefresh();
    }
}

/**
 * Retrieve meta data of a file (i.e ARTIST, TITLE, ALBUM, etc...).
 */
void MediaObject::updateMetaData()
{
    QMultiMap<QString, QString> metaDataMap;

    const char *artist = libvlc_media_get_meta(p_vlc_media, libvlc_meta_Artist);
    const char *title = libvlc_media_get_meta(p_vlc_media, libvlc_meta_Title);
    const char *nowplaying = libvlc_media_get_meta(p_vlc_media, libvlc_meta_NowPlaying);

    // Streams sometimes have the artist and title munged in nowplaying.
    // With ALBUM = Title and TITLE = NowPlaying it will still show up nicely in Amarok.
    if (qstrlen(artist) == 0 && qstrlen(nowplaying) > 0) {
        metaDataMap.insert(QLatin1String("ALBUM"),
                        QString::fromUtf8(title));
        metaDataMap.insert(QLatin1String("TITLE"),
                        QString::fromUtf8(nowplaying));
    } else {
        metaDataMap.insert(QLatin1String("ALBUM"),
                        QString::fromUtf8(libvlc_media_get_meta(p_vlc_media, libvlc_meta_Album)));
        metaDataMap.insert(QLatin1String("TITLE"),
                        QString::fromUtf8(title));
    }

    metaDataMap.insert(QLatin1String("ARTIST"),
                       QString::fromUtf8(artist));
    metaDataMap.insert(QLatin1String("DATE"),
                       QString::fromUtf8(libvlc_media_get_meta(p_vlc_media, libvlc_meta_Date)));
    metaDataMap.insert(QLatin1String("GENRE"),
                       QString::fromUtf8(libvlc_media_get_meta(p_vlc_media, libvlc_meta_Genre)));
    metaDataMap.insert(QLatin1String("TRACKNUMBER"),
                       QString::fromUtf8(libvlc_media_get_meta(p_vlc_media, libvlc_meta_TrackNumber)));
    metaDataMap.insert(QLatin1String("DESCRIPTION"),
                       QString::fromUtf8(libvlc_media_get_meta(p_vlc_media, libvlc_meta_Description)));
    metaDataMap.insert(QLatin1String("COPYRIGHT"),
                       QString::fromUtf8(libvlc_media_get_meta(p_vlc_media, libvlc_meta_Copyright)));
    metaDataMap.insert(QLatin1String("URL"),
                       QString::fromUtf8(libvlc_media_get_meta(p_vlc_media, libvlc_meta_URL)));
    metaDataMap.insert(QLatin1String("ENCODEDBY"),
                       QString::fromUtf8(libvlc_media_get_meta(p_vlc_media, libvlc_meta_EncodedBy)));

    if (metaDataMap == m_vlcMetaData) {
        // No need to issue any change, the data is the same
        return;
    }

    m_vlcMetaData = metaDataMap;

    emit metaDataChanged(metaDataMap);
}

qint64 MediaObject::totalTime() const
{
    return m_totalTime;
}

qint64 MediaObject::currentTimeInternal() const
{
    return libvlc_media_player_get_time(p_vlc_media_player);
}

/**
 * Update media duration time
 */
void MediaObject::updateDuration(qint64 newDuration)
{
    // If its within 5ms of the current total time, don't bother....
    if (newDuration - 5 > m_totalTime || newDuration + 5 < m_totalTime) {
        qDebug() << __FUNCTION__ << "Length changing from " << m_totalTime
                 << " to " << newDuration;
        m_totalTime = newDuration;
        emit totalTimeChanged(m_totalTime);
    }
}

/**
 * Adds a sink for this media object. During playInternal(), all the sinks
 * will have their addToMedia() called.
 *
 * \see playInternal()
 * \see SinkNode::addToMedia()
 */
void MediaObject::addSink(SinkNode *node)
{
    if (m_sinks.contains(node)) {
        // This shouldn't happen....
        return;
    }
    m_sinks.append(node);
}

/**
 * Removes a sink from this media object.
 */
void MediaObject::removeSink(SinkNode *node)
{
    if( node != NULL )
        m_sinks.removeAll(node);
}

/**
 * Adds an option to the libVLC media.
 *
 * \param opt What option to add
 */
void MediaObject::setOption(QString opt)
{
    Q_ASSERT(p_vlc_media);
    qDebug() << Q_FUNC_INFO << opt;
    libvlc_media_add_option_flag(p_vlc_media, opt.toLocal8Bit(), libvlc_media_option_trusted);
}


}
} // Namespace Phonon::VLC
