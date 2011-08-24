/*
    Copyright (C) 2007-2008 Tanguy Krotoff <tkrotoff@gmail.com>
    Copyright (C) 2008 Lukas Durfina <lukas.durfina@gmail.com>
    Copyright (C) 2009 Fathi Boudra <fabo@kde.org>
    Copyright (C) 2010 Ben Cooksley <sourtooth@gmail.com>
    Copyright (C) 2009-2011 vlc-phonon AUTHORS
    Copyright (C) 2010-2011 Harald Sitter <sitter@kde.org>

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

#include "mediaobject.h"

#include <QtCore/QFile>
#include <QtCore/QMetaType>
#include <QtCore/QStringBuilder>
#include <QtCore/QTimer>
#include <QtCore/QUrl>

#include "vlc/vlc.h"

#include "backend.h"
#include "debug.h"
#include "libvlc.h"
#include "media.h"
#include "mediaplayer.h"
#include "sinknode.h"

//Time in milliseconds before sending aboutToFinish() signal
//2 seconds
static const int ABOUT_TO_FINISH_TIME = 2000;

namespace Phonon {
namespace VLC {

MediaObject::MediaObject(QObject *parent)
    : QObject(parent)
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
    , m_media(0)
{
    qRegisterMetaType<QMultiMap<QString, QString> >("QMultiMap<QString, QString>");

    m_player = new MediaPlayer(this);
    if (!m_player->libvlc_media_player())
        error() << "libVLC:" << LibVLC::errorMessage();

    // Player signals.
    connect(m_player, SIGNAL(seekableChanged(bool)), this, SIGNAL(seekableChanged(bool)));
    connect(m_player, SIGNAL(timeChanged(qint64)), this, SLOT(updateTime(qint64)));
    connect(m_player, SIGNAL(stateChanged(MediaPlayer::State)), this, SLOT(updateState(MediaPlayer::State)));

    // Internal Signals.
    connect(this, SIGNAL(stateChanged(Phonon::State)), SLOT(stateChangedInternal(Phonon::State)));
    connect(this, SIGNAL(tickInternal(qint64)), SLOT(tickInternalSlot(qint64)));
    connect(this, SIGNAL(moveToNext()), SLOT(moveToNextSource()));

    resetMembers();
}

MediaObject::~MediaObject()
{
    unloadMedia();
}

inline void MediaObject::resetMembers()
{
    // default to -1, so that streams won't break and to comply with the docs (-1 if unknown)
    m_totalTime = -1;
    m_hasVideo = false;
    m_seekpoint = 0;

    m_prefinishEmitted = false;
    m_aboutToFinishEmitted = false;

    m_timesVideoChecked = 0;

    resetMediaController();
}

void MediaObject::setVideoWidget(BaseWidget *widget)
{
    m_videoWidget = widget;
}

void MediaObject::play()
{
    DEBUG_BLOCK;

    switch (m_currentState) {
    case PlayingState:
        // Do not do anything if we are already playing (as per documentation).
        return;
    case PausedState:
        m_player->resume();
        break;
    default:
#warning if we got rid of playinternal, we coulde simply call play and it would resume/play
        playInternal();
        break;
    }

    emit playbackCommenced();
}

void MediaObject::pause()
{
    if (state() != Phonon::PausedState)
        m_player->pause();
}

void MediaObject::stop()
{
    DEBUG_BLOCK;
    if (m_streamReader)
        m_streamReader->unlock();
    m_nextSource = MediaSource(QUrl());
    m_player->stop();
}

void MediaObject::seek(qint64 milliseconds)
{
    seekInternal(milliseconds);

    qint64 currentTime = this->currentTime();
    qint64 totalTime = this->totalTime();

    if (currentTime < totalTime - m_prefinishMark)
        m_prefinishEmitted = false;
    if (currentTime < totalTime - ABOUT_TO_FINISH_TIME)
        m_aboutToFinishEmitted = false;
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
        time = m_player->time();
        break;
    case Phonon::StoppedState:
    case Phonon::LoadingState:
        time = 0;
        break;
    case Phonon::ErrorState:
        time = -1;
        break;
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
    DEBUG_BLOCK;

    // Reset previous streamereaders
    if (m_streamReader) {
        m_streamReader->unlock();
        delete m_streamReader;
        m_streamReader = 0;
    }

    // Reset previous isScreen flag
    m_isScreen = false;

    m_mediaSource = source;

    QByteArray driverName;
    QString deviceName;

    switch (source.type()) {
    case MediaSource::Invalid:
        error() << Q_FUNC_INFO << "MediaSource Type is Invalid:" << source.type();
        break;
    case MediaSource::Empty:
        error() << Q_FUNC_INFO << "MediaSource is empty.";
        break;
    case MediaSource::LocalFile:
    case MediaSource::Url: {
        debug() << "MediaSource::Mrl:" << source.mrl();
        loadMedia(source.mrl().toEncoded());
    } // Keep these braces and the following break as-is, some compilers fall over the var decl above.
        break;
    case MediaSource::Disc:
        switch (source.discType()) {
        case Phonon::NoDisc:
            error() << Q_FUNC_INFO << "the MediaSource::Disc doesn't specify which one (Phonon::NoDisc)";
            return;
        case Phonon::Cd:
            loadMedia(QLatin1Literal("cdda://") % m_mediaSource.deviceName());
            break;
        case Phonon::Dvd:
            loadMedia(QLatin1Literal("dvd://") % m_mediaSource.deviceName());
            break;
        case Phonon::Vcd:
            loadMedia(m_mediaSource.deviceName());
            break;
        default:
            error() << Q_FUNC_INFO << "unsupported MediaSource::Disc:" << source.discType();
            break;
        }
        break;
    case MediaSource::CaptureDevice:
        if (source.deviceAccessList().isEmpty()) {
            error() << Q_FUNC_INFO << "No device access list for this capture device";
            break;
        }

        // TODO try every device in the access list until it works, not just the first one
        driverName = source.deviceAccessList().first().first;
        deviceName = source.deviceAccessList().first().second;

        if (driverName == QByteArray("v4l2")) {
            loadMedia(QLatin1Literal("v4l2://") % deviceName);
        } else if (driverName == QByteArray("alsa")) {
            /*
             * Replace "default" and "plughw" and "x-phonon" with "hw" for capture device names, because
             * VLC does not want to open them when using default instead of hw.
             * plughw also does not work.
             *
             * TODO investigate what happens
             */
            if (deviceName.startsWith(QLatin1String("default"))) {
                deviceName.replace(0, 7, "hw");
            }
            if (deviceName.startsWith(QLatin1String("plughw"))) {
                deviceName.replace(0, 6, "hw");
            }
            if (deviceName.startsWith(QLatin1String("x-phonon"))) {
                deviceName.replace(0, 8, "hw");
            }

            loadMedia(QLatin1Literal("alsa://") % deviceName);
        } else if (driverName == "screen") {
            loadMedia(QLatin1Literal("screen://") % deviceName);

            // Set the isScreen flag needed to add extra options in playInternal
            m_isScreen = true;
        } else {
            error() << Q_FUNC_INFO << "Unsupported MediaSource::CaptureDevice:" << driverName;
            break;
        }
        break;
    case MediaSource::Stream:
        loadStream();
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
    debug() << m_currentState << "-->" << newState;

    if (newState == m_currentState) {
        // State not changed
        return;
    } else if (checkGaplessWaiting()) {
        // This is a no-op, warn that we are....
        debug() << Q_FUNC_INFO << "no-op gapless item awaiting in queue - " << m_nextSource.type() ;
        return;
    }

    // State changed
    Phonon::State previousState = m_currentState;
    m_currentState = newState;
    emit stateChanged(m_currentState, previousState);
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

inline void MediaObject::unloadMedia()
{
    if (m_media) {
        m_media->disconnect(this);
        m_media->deleteLater();
        m_media = 0;
    }
}

void MediaObject::setVLCVideoWidget()
{
    // Nothing to do if there is no video widget
    if (!m_videoWidget)
        return;

    // Get our media player to use our window
#warning
#if defined(Q_OS_MAC)
    libvlc_media_player_set_nsobject(m_player->libvlc_media_player(), m_videoWidget->cocoaView());
#elif defined(Q_OS_UNIX)
    libvlc_media_player_set_xwindow(m_player->libvlc_media_player(), m_videoWidget->winId());
#elif defined(Q_OS_WIN)
    libvlc_media_player_set_hwnd(m_player->libvlc_media_player(), m_videoWidget->winId());
#endif
}

void MediaObject::playInternal()
{
    DEBUG_BLOCK;
    unloadMedia();

    m_totalTime = -1;

    // Create a media with the given MRL
    m_media = new Media(m_currentFile, this);
    if (!m_media) {
        error() << "libVLC:" << LibVLC::errorMessage();
    }

    if (m_streamReader) { // Set callbacks for stream reading using imem
        m_streamReader->lock(); // Make sure we can lock in read().

        m_media->addOption(QLatin1String("imem-cat=4"));
        m_media->addOption(QLatin1String("imem-data="), INTPTR_PTR(m_streamReader));
        m_media->addOption(QLatin1String("imem-get="), INTPTR_FUNC(StreamReader::readCallback));
        m_media->addOption(QLatin1String("imem-release="), INTPTR_FUNC(StreamReader::readDoneCallback));
        m_media->addOption(QLatin1String("imem-seek="), INTPTR_FUNC(StreamReader::seekCallback));

        // if stream has known size, we may pass it
        // imem module will use it and pass it to demux
        if (m_streamReader->streamSize() > 0) {
            m_media->addOption(QString("imem-size=%1").arg(m_streamReader->streamSize()));
        }
    }

    if (m_isScreen) {
        m_media->addOption(QLatin1String("screen-fps=24.0"));
        m_media->addOption(QLatin1String("screen-caching=300"));
    }

    if (source().discType() == Cd && m_currentTitle > 0) {
        debug() << "setting CDDA track";
        m_media->addOption(QLatin1String("cdda-track="), QVariant(m_currentTitle));
    }

    foreach (SinkNode *sink, m_sinks) {
        sink->addToMedia(m_media);
    }

    // Set the media that will be used by the media player
    m_player->setMedia(m_media);

    // connectToMediaVLCEvents() at the end since it needs to be done for each new libvlc_media_t instance
    connectToMediaVLCEvents();

    // Update available audio channels/subtitles/angles/chapters/etc...
    // i.e everything from MediaController
    // There is no audio channel/subtitle/angle/chapter events inside libvlc
    // so let's send our own events...
    // This will reset the GUI
    resetMediaController();

    // Set up the widget id for libVLC if there is a video widget available
    setVLCVideoWidget();

    // Play
    if (m_player->play()) {
        error() << "libVLC:" << LibVLC::errorMessage();
    }

    if (m_seekpoint != 0) {  // Workaround that seeking needs to work before the file is being played...
        seekInternal(m_seekpoint);
        m_seekpoint = 0;
    }
}

void MediaObject::seekInternal(qint64 milliseconds)
{
    DEBUG_BLOCK;
    if (state() != Phonon::PlayingState) {  // Is we aren't playing, seeking is invalid...
        m_seekpoint = milliseconds;
    }

    debug() << Q_FUNC_INFO << milliseconds;
    m_player->setTime(milliseconds);
}

QString MediaObject::errorString() const
{
    return libvlc_errmsg();
}

bool MediaObject::hasVideo() const
{
    return m_player->hasVideoOutput();
}

bool MediaObject::isSeekable() const
{
    return m_player->isSeekable();
}

void MediaObject::connectToMediaVLCEvents()
{
    // ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
    connect(m_media, SIGNAL(durationChanged(qint64)),
            this, SLOT(updateDuration(qint64)));
    connect(m_media, SIGNAL(metaDataChanged()),
            this, SLOT(updateMetaData()));
}

void MediaObject::updateDuration(qint64 newDuration)
{
#warning duration signal can just be forwarded, we have no gain from caching this
    m_totalTime = newDuration;
    emit totalTimeChanged(m_totalTime);
}

void MediaObject::updateMetaData()
{
    QMultiMap<QString, QString> metaDataMap;

    const QString artist = m_media->meta(libvlc_meta_Artist);
    const QString title = m_media->meta(libvlc_meta_Title);
    const QString nowPlaying = m_media->meta(libvlc_meta_NowPlaying);

    // Streams sometimes have the artist and title munged in nowplaying.
    // With ALBUM = Title and TITLE = NowPlaying it will still show up nicely in Amarok.
    if (artist.isEmpty() && !nowPlaying.isEmpty()) {
        metaDataMap.insert(QLatin1String("ALBUM"), title);
        metaDataMap.insert(QLatin1String("TITLE"), nowPlaying);
    } else {
        metaDataMap.insert(QLatin1String("ALBUM"), m_media->meta(libvlc_meta_Album));
        metaDataMap.insert(QLatin1String("TITLE"), title);
    }

    metaDataMap.insert(QLatin1String("ARTIST"), artist);
    metaDataMap.insert(QLatin1String("DATE"), m_media->meta(libvlc_meta_Date));
    metaDataMap.insert(QLatin1String("GENRE"), m_media->meta(libvlc_meta_Genre));
    metaDataMap.insert(QLatin1String("TRACKNUMBER"), m_media->meta(libvlc_meta_TrackNumber));
    metaDataMap.insert(QLatin1String("DESCRIPTION"), m_media->meta(libvlc_meta_Description));
    metaDataMap.insert(QLatin1String("COPYRIGHT"), m_media->meta(libvlc_meta_Copyright));
    metaDataMap.insert(QLatin1String("URL"), m_media->meta(libvlc_meta_URL));
    metaDataMap.insert(QLatin1String("ENCODEDBY"), m_media->meta(libvlc_meta_EncodedBy));

    if (metaDataMap == m_vlcMetaData) {
        // No need to issue any change, the data is the same
        return;
    }
    m_vlcMetaData = metaDataMap;

    emit metaDataChanged(metaDataMap);
}

void MediaObject::updateState(MediaPlayer::State state)
{
    DEBUG_BLOCK;
    debug() << state;
    switch (state) {
    case MediaPlayer::NoState:
        emit stateChanged(LoadingState);
        break;
    case MediaPlayer::OpeningState:
        emit stateChanged(LoadingState);
        break;
    case MediaPlayer::BufferingState:
        emit stateChanged(BufferingState);
        break;
    case MediaPlayer::PlayingState:
        emit stateChanged(PlayingState);
        break;
    case MediaPlayer::PausedState:
        emit stateChanged(PausedState);
        break;
    case MediaPlayer::StoppedState:
        resetMembers();
        emit stateChanged(StoppedState);
        break;
    case MediaPlayer::EndedState:
        resetMembers();
        emitAboutToFinish();
        emit finished();
        emit stateChanged(StoppedState);
        break;
    case MediaPlayer::ErrorState:
        resetMembers();
        emitAboutToFinish();
        emit finished();
        emit stateChanged(ErrorState);
        break;
    }
}

void MediaObject::updateTime(qint64 time)
{
    DEBUG_BLOCK;
    debug() << time;

#ifdef __GNUC__
#warning FIXME - This is ugly. It should be solved by some events in libvlc
#endif
    // Check 10 times for a video, then give up.
    if (!m_hasVideo && ++m_timesVideoChecked < 11) {
        debug() << "Looking for Video";

//        // Does this media player have a video output
        const bool hasVideo = m_player->hasVideoOutput();
        if (m_hasVideo != hasVideo) {
            m_hasVideo = hasVideo;
            emit hasVideoChanged(m_hasVideo);
        }

        if (hasVideo) {
            debug() << "HASVIDEO";
#warning
//            debug() << Q_FUNC_INFO << "hasVideo!";

//            // Give info about audio tracks
//            refreshAudioChannels();
//            // Give info about subtitle tracks
//            refreshSubtitles();

//            // Get movie chapter count
//            // It is not a title/chapter media if there is no chapter
//            if (libvlc_media_player_get_chapter_count(
//                        m_player) > 0) {
//                // Give info about title
//                // only first time, no when title changed
//                if (!b_media_player_title_changed) {
//                    libvlc_track_description_t *p_info = libvlc_video_get_title_description(
//                                m_player);
//                    while (p_info) {
//                        titleAdded(p_info->i_id, p_info->psz_name);
//                        p_info = p_info->p_next;
//                    }
//                    libvlc_track_description_release(p_info);
//                }

//                // Give info about chapters for actual title 0
//                if (b_media_player_title_changed) {
//                    refreshChapters(libvlc_media_player_get_title(
//                                                           m_player));
//                } else {
//                    refreshChapters(0);
//                }
//            }
//            if (b_media_player_title_changed) {
//                b_media_player_title_changed = false;
//            }
        }
    }

    emit tickInternal(time);
}

qint64 MediaObject::totalTime() const
{
    return m_totalTime;
}

void MediaObject::addSink(SinkNode *node)
{
    Q_ASSERT(!m_sinks.contains(node));
    m_sinks.append(node);
}

void MediaObject::removeSink(SinkNode *node)
{
    Q_ASSERT(node);
    m_sinks.removeAll(node);
}

} // namespace VLC
} // namespace Phonon
