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

#include <QtCore/QFile>
#include <QtCore/QMetaType>
#include <QtCore/QTimer>
#include <QtCore/QUrl>

#include "vlc/vlc.h"

#include "debug.h"
#include "libvlc.h"
#include "seekstack.h"
#include "sinknode.h"

//Time in milliseconds before sending aboutToFinish() signal
//2 seconds
static const int ABOUT_TO_FINISH_TIME = 2000;

namespace Phonon
{
namespace VLC
{

MediaObject::MediaObject(QObject *p_parent)
    : QObject(p_parent)
    , m_videoWidget(0)
    , m_nextSource(MediaSource(QUrl()))
    , m_streamReader(0)
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
    m_player = libvlc_media_player_new(libvlc);
    if (!m_player) {
        qDebug() << "libvlc exception:" << libvlc_errmsg();
    }
    m_eventManager = 0;
    connectToPlayerVLCEvents();

    // Media
    m_media = 0;
    m_mediaEventManager = 0;

    // Media Discoverer
    m_mediaDiscoverer = 0;
    m_mediaDiscovererEventManager = 0;

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

    libvlc_media_player_stop(m_player); // ensure that we are stopped
    libvlc_media_player_release(m_player);
}

//void MediaObject::setVideoWidgetId(WId i_widget_id)
//{
//    i_video_widget_id = i_widget_id;
//}
//

void MediaObject::setVideoWidget(BaseWidget *widget)
{
    this->m_videoWidget = widget;
}

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

void MediaObject::loadMedia(const QByteArray &filename)
{
    DEBUG_BLOCK;

    // Initial state is loading, from which we quickly progress to stopped because
    // libvlc does not provide feedback on loading and the media does not get loaded
    // until we play it.
    // FIXME: libvlc should really allow for this as it can cause unexpected delay
    // even though the GUI might indicate that playback should start right away.
    emit stateChanged(Phonon::LoadingState);

    m_currentFile = filename;
    debug() << "loading encoded:" << m_currentFile;

    // We do not have a loading state generally speaking, usually the backend
    // is exepected to go to loading state and then at some point reach stopped,
    // at which point playback can be started.
    // See state enum documentation for more information.
    emit stateChanged(Phonon::StoppedState);
}

void MediaObject::loadMedia(const QString &filename)
{
    loadMedia(filename.toUtf8());
}

void MediaObject::resume()
{
    pause();
}

qint32 MediaObject::tickInterval() const
{
    return m_tickInterval;
}

void MediaObject::setTickInterval(qint32 tickInterval)
{
    m_tickInterval = tickInterval;
//    if (_tickInterval <= 0) {
//        _tickTimer->setInterval(50);
//    } else {
//        _tickTimer->setInterval(_tickInterval);
//    }
}

qint64 MediaObject::currentTime() const
{
    qint64 time = -1;

    switch (state()) {
    case Phonon::PausedState:
    case Phonon::BufferingState:
    case Phonon::PlayingState:
        time = libvlc_media_player_get_time(m_player);
        break;
    case Phonon::StoppedState:
    case Phonon::LoadingState:
        time = 0;
        break;
    case Phonon::ErrorState:
        time = -1;
        break;
    default:
        qCritical() << __FUNCTION__ << "Error: unsupported Phonon::State:" << state();
    }

    return time;
}

Phonon::State MediaObject::state() const
{
    return m_currentState;
}

Phonon::ErrorType MediaObject::errorType() const
{
    return Phonon::NormalError;
}

MediaSource MediaObject::source() const
{
    return m_mediaSource;
}

void MediaObject::setSource(const MediaSource &source)
{
    qDebug() << __FUNCTION__;

    // Reset previous streameraders
    if (m_streamReader) {
        m_streamReader->unlock();
        m_streamReader = 0;
    }

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
        QByteArray mrl;
        const QUrl &url = source.url();
        if (url.scheme() == QLatin1String("")) {
            mrl = QFile::encodeName("file://" + url.toString()).toPercentEncoding(":/\\?=&,@");
        } else if ((url.scheme() ==  QLatin1String("file://"))) {
            mrl = QFile::encodeName(url.toString()).toPercentEncoding(":/\\?=&,@");
        } else {
            mrl = url.toEncoded();
        }
        loadMedia(mrl);
    } // Keep these braces and the follwing break as-is, some compilers fall over the var decl above.
        break;
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
            /*
             * Replace "default" and "plughw" and "x-phonon" with "hw" for capture device names, because
             * VLC does not want to open them when using default instead of hw.
             * plughw also does not work.
             *
             * TODO investigate what happens
             */
            if (deviceName.startsWith("default")) {
                deviceName.replace(0, 7, "hw");
            }
            if (deviceName.startsWith("plughw")) {
                deviceName.replace(0, 6, "hw");
            }
            if (deviceName.startsWith("x-phonon")) {
                deviceName.replace(0, 8, "hw");
            }

            loadMedia("alsa://" + deviceName);
        } else {
            qCritical() << __FUNCTION__ << "Error: unsupported MediaSource::CaptureDevice:" << driverName;
            break;
        }

        break;
    case MediaSource::Stream:
        loadStream();
        break;
    default:
        qCritical() << __FUNCTION__ << "Error: Unsupported MediaSource Type:" << source.type();
        break;
    }

    emit currentSourceChanged(m_mediaSource);
}

void MediaObject::loadStream()
{
    m_streamReader = new StreamReader(m_mediaSource, this);

    loadMedia(QByteArray("imem://"));
}

void MediaObject::setNextSource(const MediaSource &source)
{
    DEBUG_BLOCK;
    debug() << source.url();
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

void MediaObject::stateChangedInternal(Phonon::State newState)
{
    DEBUG_BLOCK;
    debug() << phononStateToString(m_currentState)
            << "-->"
            << phononStateToString(newState);

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

QString MediaObject::phononStateToString(Phonon::State state)
{
    QString string;
    switch (state) {
    case Phonon::ErrorState:
        string = QLatin1String("ErrorState");
        break;
    case Phonon::LoadingState:
        string = QLatin1String("LoadingState");
        break;
    case Phonon::StoppedState:
        string = QLatin1String("StoppedState");
        break;
    case Phonon::PlayingState:
        string = QLatin1String("PlayingState");
        break;
    case Phonon::BufferingState:
        string = QLatin1String("BufferingState");
        break;
    case Phonon::PausedState:
        string = QLatin1String("PausedState");
        break;
    }
    return string;
}

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

void MediaObject::unloadMedia()
{
//    if( m_player ) {
//        libvlc_media_player_release(m_player);
//        m_player = 0;
//    }

    if (m_media) {
        libvlc_media_release(m_media);
        m_media = 0;
    }
}

void MediaObject::setVLCVideoWidget()
{
    // Nothing to do if there is no video widget
    if (!m_videoWidget)
        return;

    // Get our media player to use our window
#if defined(Q_OS_MAC)
    libvlc_media_player_set_nsobject(m_player, m_videoWidget->cocoaView());
#elif defined(Q_OS_UNIX)
    libvlc_media_player_set_xwindow(m_player, m_videoWidget->winId());
#elif defined(Q_OS_WIN)
    libvlc_media_player_set_hwnd(m_player, m_videoWidget->winId());
#endif
}

void MediaObject::playInternal()
{
    DEBUG_BLOCK;
    if (m_media) {  // We are changing media, discard the old one
        libvlc_media_release(m_media);
        m_media = 0;
    }

    m_totalTime = -1;

    // Create a media with the given MRL
    m_media = libvlc_media_new_location(libvlc, m_currentFile);
    if (!m_media) {
        qDebug() << "libvlc exception:" << libvlc_errmsg();
    }

    if (m_streamReader) { // Set callbacks for stream reading using imem
        m_streamReader->lock(); // Make sure we can lock in read().

        addOption(QString("imem-cat=4"));
        addOption(QString("imem-data="), INTPTR_PTR(m_streamReader));
        addOption(QString("imem-get="), INTPTR_FUNC(StreamReader::readCallback));
        addOption(QString("imem-release="), INTPTR_FUNC(StreamReader::readDoneCallback));
        addOption(QString("imem-seek="), INTPTR_FUNC(StreamReader::seekCallback));

        // if stream has known size, we may pass it
        // imem module will use it and pass it to demux
        if (m_streamReader->streamSize() > 0) {
            QString(QString("imem-size=%1").arg(m_streamReader->streamSize()));
        }
    }

    foreach (SinkNode * sink, m_sinks) {
        sink->addToMedia(m_media);
    }

    // Set the media that will be used by the media player
    libvlc_media_player_set_media(m_player, m_media);

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
    if (libvlc_media_player_play(m_player)) {
        qDebug() << "libvlc exception:" << libvlc_errmsg();
    }

    if (m_seekpoint != 0) {  // Workaround that seeking needs to work before the file is being played...
        seekInternal(m_seekpoint);
        m_seekpoint = 0;
    }

    emit stateChanged(Phonon::PlayingState);
}

void MediaObject::pause()
{
    libvlc_media_t *media = libvlc_media_player_get_media(m_player);

    if (state() == Phonon::PausedState) {
#ifdef __GNUC__
#warning HACK!!!! -> after loading we are in pause, even though no media is loaded
#endif
        if (media == 0) {
            // Nothing playing yet -> play
            playInternal();
        } else {
            // Resume
            libvlc_media_player_set_pause(m_player, 0);
            emit stateChanged(Phonon::PlayingState);
        }
    } else {
        // Pause
        libvlc_media_player_set_pause(m_player, 1);
        emit stateChanged(Phonon::PausedState);
    }

}

void MediaObject::stop()
{
    DEBUG_BLOCK;
    if (m_streamReader)
        m_streamReader->unlock();
    m_nextSource = MediaSource(QUrl());
    libvlc_media_player_stop(m_player);
//    unloadMedia();
    emit stateChanged(Phonon::StoppedState);
}

void MediaObject::seekInternal(qint64 milliseconds)
{
    if (state() != Phonon::PlayingState) {  // Is we aren't playing, seeking is invalid...
        m_seekpoint = milliseconds;
    }

    qDebug() << __FUNCTION__ << milliseconds;
    libvlc_media_player_set_time(m_player, milliseconds);
}

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

void MediaObject::connectToPlayerVLCEvents()
{
    // Get the event manager from which the media player send event
    m_eventManager = libvlc_media_player_event_manager(m_player);
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
        libvlc_event_attach(m_eventManager, eventsMediaPlayer[i],
                            eventCallback, this);
    }
}

void MediaObject::connectToMediaVLCEvents()
{
    // Get event manager from media descriptor object
    m_mediaEventManager = libvlc_media_event_manager(m_media);
    libvlc_event_type_t eventsMedia[] = {
        libvlc_MediaMetaChanged,
        //libvlc_MediaSubItemAdded, // Could this be used for Audio Channels / Subtitles / Chapter info??
        libvlc_MediaDurationChanged,
        //libvlc_MediaFreed, // Not needed is this?
        //libvlc_MediaStateChanged, // We don't use this? Could we??
    };
    int i_nbEvents = sizeof(eventsMedia) / sizeof(*eventsMedia);
    for (int i = 0; i < i_nbEvents; i++) {
        libvlc_event_attach(m_mediaEventManager, eventsMedia[i], eventCallback, this);
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

void MediaObject::eventCallback(const libvlc_event_t *event, void *data)
{
    static int i_first_time_media_player_time_changed = 0;
    static bool b_media_player_title_changed = false;

    MediaObject *const that = static_cast<MediaObject *>(data);

//    qDebug() << (int)p_vlc_mediaObject << "event:" << libvlc_event_type_name(p_event->type);

    // Media player events
    if (event->type == libvlc_MediaPlayerSeekableChanged) {
        const bool b_seekable = libvlc_media_player_is_seekable(that->m_player);
        if (b_seekable != that->m_seekable) {
            that->m_seekable = b_seekable;
            emit that->seekableChanged(that->m_seekable);
        }
    }
    if (event->type == libvlc_MediaPlayerTimeChanged) {

        ++i_first_time_media_player_time_changed;

#ifdef __GNUC__
#warning FIXME 4.5 - This is ugly. It should be solved by some events in libvlc
#endif
        if (!that->m_hasVideo && i_first_time_media_player_time_changed < 15) {
            debug() << "Looking for Video";
            // Update metadata
            that->updateMetaData();

            // Get current video width and height
            uint width = 0;
            uint height = 0;
            libvlc_video_get_size(that->m_player, 0, &width, &height);
            emit that->videoWidgetSizeChanged(width, height);

            // Does this media player have a video output
            const bool b_has_video = libvlc_media_player_has_vout(that->m_player);
            if (b_has_video != that->m_hasVideo) {
                that->m_hasVideo = b_has_video;
                emit that->hasVideoChanged(that->m_hasVideo);
            }

            if (b_has_video) {
                qDebug() << Q_FUNC_INFO << "hasVideo!";

                // Give info about audio tracks
                that->refreshAudioChannels();
                // Give info about subtitle tracks
                that->refreshSubtitles();

                // Get movie chapter count
                // It is not a title/chapter media if there is no chapter
                if (libvlc_media_player_get_chapter_count(
                            that->m_player) > 0) {
                    // Give info about title
                    // only first time, no when title changed
                    if (!b_media_player_title_changed) {
                        libvlc_track_description_t *p_info = libvlc_video_get_title_description(
                                    that->m_player);
                        while (p_info) {
                            that->titleAdded(p_info->i_id, p_info->psz_name);
                            p_info = p_info->p_next;
                        }
                        libvlc_track_description_release(p_info);
                    }

                    // Give info about chapters for actual title 0
                    if (b_media_player_title_changed) {
                        that->refreshChapters(libvlc_media_player_get_title(
                                                               that->m_player));
                    } else {
                        that->refreshChapters(0);
                    }
                }
                if (b_media_player_title_changed) {
                    b_media_player_title_changed = false;
                }
            }

            // Bugfix with Qt mediaplayer example
            // Now we are in playing state
            emit that->stateChanged(Phonon::PlayingState);
        }

        emit that->tickInternal(that->currentTime());
    }

    if (event->type == libvlc_MediaPlayerBuffering) {
        emit that->stateChanged(Phonon::BufferingState);
    }

    if (event->type == libvlc_MediaPlayerPlaying) {
        if (that->state() != Phonon::BufferingState) {
            // Bugfix with Qt mediaplayer example
            emit that->stateChanged(Phonon::PlayingState);
        }
    }

    if (event->type == libvlc_MediaPlayerPaused) {
        emit that->stateChanged(Phonon::PausedState);
    }

    if (event->type == libvlc_MediaPlayerEndReached && !that->checkGaplessWaiting()) {
        i_first_time_media_player_time_changed = 0;
        that->resetMediaController();
        that->emitAboutToFinish();
        emit that->finished();
        emit that->stateChanged(Phonon::StoppedState);
    } else if (event->type == libvlc_MediaPlayerEndReached) {
        emit that->moveToNext();
    }

    if (event->type == libvlc_MediaPlayerEncounteredError && !that->checkGaplessWaiting()) {
        i_first_time_media_player_time_changed = 0;
        that->resetMediaController();
        emit that->finished();
        emit that->stateChanged(Phonon::ErrorState);
    } else if (event->type == libvlc_MediaPlayerEncounteredError) {
        emit that->moveToNext();
    }

    if (event->type == libvlc_MediaPlayerStopped && !that->checkGaplessWaiting()) {
        i_first_time_media_player_time_changed = 0;
        that->resetMediaController();
        emit that->stateChanged(Phonon::StoppedState);
    }

    if (event->type == libvlc_MediaPlayerTitleChanged) {
        i_first_time_media_player_time_changed = 0;
        b_media_player_title_changed = true;
    }

    // Media events

    if (event->type == libvlc_MediaDurationChanged) {
        emit that->durationChanged(event->u.media_duration_changed.new_duration);
    }

    if (event->type == libvlc_MediaMetaChanged) {
        emit that->metaDataNeedsRefresh();
    }
}

void MediaObject::updateMetaData()
{
    QMultiMap<QString, QString> metaDataMap;

    const char *artist = libvlc_media_get_meta(m_media, libvlc_meta_Artist);
    const char *title = libvlc_media_get_meta(m_media, libvlc_meta_Title);
    const char *nowplaying = libvlc_media_get_meta(m_media, libvlc_meta_NowPlaying);

    // Streams sometimes have the artist and title munged in nowplaying.
    // With ALBUM = Title and TITLE = NowPlaying it will still show up nicely in Amarok.
    if (qstrlen(artist) == 0 && qstrlen(nowplaying) > 0) {
        metaDataMap.insert(QLatin1String("ALBUM"),
                        QString::fromUtf8(title));
        metaDataMap.insert(QLatin1String("TITLE"),
                        QString::fromUtf8(nowplaying));
    } else {
        metaDataMap.insert(QLatin1String("ALBUM"),
                        QString::fromUtf8(libvlc_media_get_meta(m_media, libvlc_meta_Album)));
        metaDataMap.insert(QLatin1String("TITLE"),
                        QString::fromUtf8(title));
    }

    metaDataMap.insert(QLatin1String("ARTIST"),
                       QString::fromUtf8(artist));
    metaDataMap.insert(QLatin1String("DATE"),
                       QString::fromUtf8(libvlc_media_get_meta(m_media, libvlc_meta_Date)));
    metaDataMap.insert(QLatin1String("GENRE"),
                       QString::fromUtf8(libvlc_media_get_meta(m_media, libvlc_meta_Genre)));
    metaDataMap.insert(QLatin1String("TRACKNUMBER"),
                       QString::fromUtf8(libvlc_media_get_meta(m_media, libvlc_meta_TrackNumber)));
    metaDataMap.insert(QLatin1String("DESCRIPTION"),
                       QString::fromUtf8(libvlc_media_get_meta(m_media, libvlc_meta_Description)));
    metaDataMap.insert(QLatin1String("COPYRIGHT"),
                       QString::fromUtf8(libvlc_media_get_meta(m_media, libvlc_meta_Copyright)));
    metaDataMap.insert(QLatin1String("URL"),
                       QString::fromUtf8(libvlc_media_get_meta(m_media, libvlc_meta_URL)));
    metaDataMap.insert(QLatin1String("ENCODEDBY"),
                       QString::fromUtf8(libvlc_media_get_meta(m_media, libvlc_meta_EncodedBy)));

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

void MediaObject::addSink(SinkNode *node)
{
    if (m_sinks.contains(node)) {
        // This shouldn't happen....
        return;
    }
    m_sinks.append(node);
}

void MediaObject::removeSink(SinkNode *node)
{
    if( node != NULL )
        m_sinks.removeAll(node);
}

void MediaObject::addOption(const QString &option)
{
    addOption(m_media, option);
}

void MediaObject::addOption(libvlc_media_t *media, const QString &option)
{
    Q_ASSERT(media);
    qDebug() << Q_FUNC_INFO << option;
    libvlc_media_add_option_flag(media, qPrintable(option), libvlc_media_option_trusted);
}

void MediaObject::addOption(const QString &option, intptr_t functionPtr)
{
    addOption(m_media, option, functionPtr);
}

void MediaObject::addOption(libvlc_media_t *media, const QString &option,
                            intptr_t functionPtr)
{
    Q_ASSERT(media);
    QString optionWithPtr = option;
    optionWithPtr.append(QString::number(static_cast<qint64>(functionPtr)));
    qDebug() << Q_FUNC_INFO << optionWithPtr;
    libvlc_media_add_option_flag(media, qPrintable(optionWithPtr), libvlc_media_option_trusted);
}

}
} // Namespace Phonon::VLC
