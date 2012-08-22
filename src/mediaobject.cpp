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

#include <QtCore/QStringBuilder>
#include <QtCore/QUrl>
#include <QDir>

#include <vlc/libvlc_version.h>
#include <vlc/vlc.h>

#include "utils/debug.h"
#include "utils/libvlc.h"
#include "media.h"
#include "sinknode.h"
#include "streamreader.h"

//Time in milliseconds before sending aboutToFinish() signal
//2 seconds
static const int ABOUT_TO_FINISH_TIME = 2000;

namespace Phonon {
namespace VLC {

MediaObject::MediaObject(QObject *parent)
    : QObject(parent)
    , m_nextSource(MediaSource(QUrl()))
    , m_streamReader(0)
    , m_state(Phonon::StoppedState)
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
    connect(m_player, SIGNAL(timeChanged(qint64)), this, SLOT(timeChanged(qint64)));
    connect(m_player, SIGNAL(stateChanged(MediaPlayer::State)), this, SLOT(updateState(MediaPlayer::State)));
    connect(m_player, SIGNAL(hasVideoChanged(bool)), this, SLOT(onHasVideoChanged(bool)));
    connect(m_player, SIGNAL(bufferChanged(int)), this, SLOT(setBufferStatus(int)));
    connect(m_player, SIGNAL(timeChanged(qint64)), this, SLOT(timeChanged(qint64)));

    // Internal Signals.
    connect(this, SIGNAL(moveToNext()), SLOT(moveToNextSource()));

    resetMembers();
}

MediaObject::~MediaObject()
{
    unloadMedia();
}

void MediaObject::resetMembers()
{
    // default to -1, so that streams won't break and to comply with the docs (-1 if unknown)
    m_totalTime = -1;
    m_hasVideo = false;
    m_seekpoint = 0;

    m_prefinishEmitted = false;
    m_aboutToFinishEmitted = false;

    m_lastTick = 0;

    m_timesVideoChecked = 0;

    m_buffering = false;
    m_stateAfterBuffering = ErrorState;

    resetMediaController();
}

void MediaObject::play()
{
    DEBUG_BLOCK;

    switch (m_state) {
    case PlayingState:
        // Do not do anything if we are already playing (as per documentation).
        return;
    case PausedState:
        m_player->resume();
        break;
    default:
#ifdef __GNUC__
#warning if we got rid of playinternal, we coulde simply call play and it would resume/play
#endif
        playInternal();
        break;
    }
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
    DEBUG_BLOCK;

    switch (m_state) {
    case PlayingState:
    case PausedState:
    case BufferingState:
        break;
    default:
        // Seeking while not being in a playingish state is cached for later.
        m_seekpoint = milliseconds;
        return;
    }

    debug() << "seeking" << milliseconds << "msec";

    m_player->setTime(milliseconds);

    const qint64 time = currentTime();
    const qint64 total = totalTime();

    if (time < total - m_prefinishMark)
        m_prefinishEmitted = false;
    if (time < total - ABOUT_TO_FINISH_TIME)
        m_aboutToFinishEmitted = false;
}

void MediaObject::timeChanged(qint64 time)
{
    const qint64 totalTime = m_totalTime;

    switch (m_state) {
    case PlayingState:
    case BufferingState:
    case PausedState:
        emitTick(time);
    default:
        break;
    }

    if (m_state == PlayingState || m_state == BufferingState) { // Buffering is concurrent
        if (time >= totalTime - m_prefinishMark) {
            if (!m_prefinishEmitted) {
                m_prefinishEmitted = true;
                emit prefinishMarkReached(totalTime - time);
            }
        }
        // Note that when the totalTime is <= 0 we cannot calculate any sane delta.
        if (totalTime > 0 && time >= totalTime - ABOUT_TO_FINISH_TIME)
            emitAboutToFinish();
    }
}

void MediaObject::emitTick(qint64 time)
{
    if (m_tickInterval == 0) // Make sure we do not ever emit ticks when deactivated.\]
        return;
    if (time + m_tickInterval >= m_lastTick) {
        m_lastTick = time;
        emit tick(time);
    }
}

void MediaObject::loadMedia(const QByteArray &mrl)
{
    DEBUG_BLOCK;

    // Initial state is loading, from which we quickly progress to stopped because
    // libvlc does not provide feedback on loading and the media does not get loaded
    // until we play it.
    // FIXME: libvlc should really allow for this as it can cause unexpected delay
    // even though the GUI might indicate that playback should start right away.
    changeState(Phonon::LoadingState);

    m_mrl = mrl;
    debug() << "loading encoded:" << m_mrl;

    // We do not have a loading state generally speaking, usually the backend
    // is exepected to go to loading state and then at some point reach stopped,
    // at which point playback can be started.
    // See state enum documentation for more information.
    changeState(Phonon::StoppedState);
}

void MediaObject::loadMedia(const QString &mrl)
{
    loadMedia(mrl.toUtf8());
}

qint32 MediaObject::tickInterval() const
{
    return m_tickInterval;
}

/**
 * Supports runtime changes.
 * If the user goes to tick(0) we stop the timer, otherwise we fire it up.
 */
void MediaObject::setTickInterval(qint32 interval)
{
    m_tickInterval = interval;
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
    return m_state;
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
        // For streamreaders we exchanage the player's seekability with the
        // reader's so here we change it back.
        // Note: the reader auto-disconnects due to destruction.
        connect(m_player, SIGNAL(seekableChanged(bool)), this, SIGNAL(seekableChanged(bool)));
    }

    // Reset previous isScreen flag
    m_isScreen = false;

    m_mediaSource = source;

    QByteArray url;
    switch (source.type()) {
    case MediaSource::Invalid:
        error() << Q_FUNC_INFO << "MediaSource Type is Invalid:" << source.type();
        break;
    case MediaSource::Empty:
        error() << Q_FUNC_INFO << "MediaSource is empty.";
        break;
    case MediaSource::LocalFile:
    case MediaSource::Url:
        debug() << "MediaSource::Url:" << source.url();
        if (source.url().scheme().isEmpty()) {
            url = "file:///";
            if (source.url().isRelative())
                url.append(QFile::encodeName(QDir::currentPath()) + '/');
        }
        url += source.url().toEncoded();
        loadMedia(url);
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
#if (PHONON_VERSION >= PHONON_VERSION_CHECK(4, 7, 0))
        case Phonon::BluRay:
            loadMedia(QLatin1Literal("bluray://") % m_mediaSource.deviceName());
#endif
        }
        break;
    case MediaSource::CaptureDevice: {
        QByteArray driverName;
        QString deviceName;

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
    }
    case MediaSource::Stream:
        m_streamReader = new StreamReader(this);
        // LibVLC refuses to emit seekability as it does a try-and-seek approach
        // to work around this we exchange the player's seekability signal
        // for the readers
        // https://bugs.kde.org/show_bug.cgi?id=293012
        connect(m_streamReader, SIGNAL(streamSeekableChanged(bool)), this, SIGNAL(seekableChanged(bool)));
        disconnect(m_player, SIGNAL(seekableChanged(bool)), this, SIGNAL(seekableChanged(bool)));
        // Only connect now to avoid seekability detection before we are connected.
        m_streamReader->connectToSource(source);
        loadMedia(QByteArray("imem://"));
        break;
    }

    debug() << "Sending currentSourceChanged";
    emit currentSourceChanged(m_mediaSource);
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

// State changes are force queued by libphonon.
void MediaObject::changeState(Phonon::State newState)
{
    DEBUG_BLOCK;
    if (newState == m_state) {
        // State not changed
        return;
    } else if (checkGaplessWaiting()) {
        // This is a no-op, warn that we are....
        debug() << Q_FUNC_INFO << "no-op gapless item awaiting in queue - " << m_nextSource.type() ;
        return;
    }

    debug() << m_state << "-->" << newState;

#ifdef __GNUC__
#warning do we actually need m_seekpoint? if a consumer seeks before playing state that is their problem?!
#endif
    // Workaround that seeking needs to work before the file is being played...
    // We store seeks and apply them when going to seek (or discard them on reset).
    if (newState == PlayingState) {
        if (m_seekpoint != 0) {
            seek(m_seekpoint);
            m_seekpoint = 0;
        }
    }

    // State changed
    Phonon::State previousState = m_state;
    m_state = newState;
    emit stateChanged(m_state, previousState);
}

void MediaObject::moveToNextSource()
{
    DEBUG_BLOCK;
    if (m_nextSource.type() == MediaSource::Invalid) {
        // No item is scheduled to be next...
        return;
    }

    setSource(m_nextSource);
    play();
    m_nextSource = MediaSource(QUrl());
}

inline bool MediaObject::checkGaplessWaiting()
{
    DEBUG_BLOCK;
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

void MediaObject::playInternal()
{
    DEBUG_BLOCK;

    unloadMedia();
    resetMembers();

    // Create a media with the given MRL
    m_media = new Media(m_mrl, this);
    if (!m_media)
        error() << "libVLC:" << LibVLC::errorMessage();

    if (m_isScreen) {
        m_media->addOption(QLatin1String("screen-fps=24.0"));
        m_media->addOption(QLatin1String("screen-caching=300"));
    }

    if (source().discType() == Cd && m_currentTitle > 0) {
        debug() << "setting CDDA track";
        m_media->addOption(QLatin1String("cdda-track="), QVariant(m_currentTitle));
    }

    if (m_streamReader)
        // StreamReader is no sink but a source, for this we have no concept right now
        // also we do not need one since the reader is the only source we have.
        // Consequently we need to manually tell the StreamReader to attach to the Media.
        m_streamReader->addToMedia(m_media);

    foreach (SinkNode *sink, m_sinks) {
        sink->addToMedia(m_media);
    }

    // Connect to Media signals. Disconnection is done at unloading.
    connect(m_media, SIGNAL(durationChanged(qint64)),
            this, SLOT(updateDuration(qint64)));
    connect(m_media, SIGNAL(metaDataChanged()),
            this, SLOT(updateMetaData()));

    // Update available audio channels/subtitles/angles/chapters/etc...
    // i.e everything from MediaController
    // There is no audio channel/subtitle/angle/chapter events inside libvlc
    // so let's send our own events...
    // This will reset the GUI
    resetMediaController();

    // Play
    m_player->setMedia(m_media);
    if (m_player->play())
        error() << "libVLC:" << LibVLC::errorMessage();
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
    if (m_streamReader)
        return m_streamReader->streamSeekable();
    return m_player->isSeekable();
}

void MediaObject::updateDuration(qint64 newDuration)
{
    // This here cache is needed because we need to provide -1 as totalTime()
    // for as long as we do not get a proper update through this slot.
    // VLC reports -1 with no media but 0 if it does not know the duration, so
    // apps that assume 0 = unkown get screwed if they query too early.
    // http://bugs.tomahawk-player.org/browse/TWK-1029
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
        changeState(LoadingState);
        break;
    case MediaPlayer::OpeningState:
        changeState(LoadingState);
        break;
    case MediaPlayer::BufferingState:
        changeState(BufferingState);
        break;
    case MediaPlayer::PlayingState:
        changeState(PlayingState);
        break;
    case MediaPlayer::PausedState:
        changeState(PausedState);
        break;
    case MediaPlayer::StoppedState:
        changeState(StoppedState);
        break;
    case MediaPlayer::EndedState:
        emitAboutToFinish();
        emit finished();
        changeState(StoppedState);
        break;
    case MediaPlayer::ErrorState:
        debug() << errorString();
        emitAboutToFinish();
        emit finished();
        changeState(ErrorState);
        break;
    }

    if (m_buffering) {
        switch (state) {
        case MediaPlayer::BufferingState:
            break;
        case MediaPlayer::PlayingState:
            debug() << "Restoring buffering state after state change to Playing";
            changeState(BufferingState);
            m_stateAfterBuffering = PlayingState;
            break;
        case MediaPlayer::PausedState:
            debug() << "Restoring buffering state after state change to Paused";
            changeState(BufferingState);
            m_stateAfterBuffering = PausedState;
            break;
        default:
            debug() << "Buffering aborted!";
            m_buffering = false;
            break;
        }
    }

    return;
}

void MediaObject::onHasVideoChanged(bool hasVideo)
{
    DEBUG_BLOCK;
    if (m_hasVideo != hasVideo) {
        m_hasVideo = hasVideo;
        emit hasVideoChanged(m_hasVideo);
    } else
        // We can simply return if we are have the appropriate caching already.
        // Otherwise we'd do pointless rescans of mediacontroller stuff.
        // MC and MO are force-reset on media changes anyway.
        return;

    if (hasVideo) {
        refreshAudioChannels();
        refreshSubtitles();

        // Get movie chapter count
        // It is not a title/chapter media if there is no chapter
        if (m_player->videoChapterCount() > 0) {
            refreshTitles();
            refreshChapters(m_player->title());
        }
    }
}

void MediaObject::setBufferStatus(int percent)
{
    // VLC does not have a buffering state (surprise!) but instead only sends the
    // event (surprise!). Hence we need to simulate the state change.
    // Problem with BufferingState is that it is actually concurent to Playing or Paused
    // meaning that while you are buffering you can also pause, thus triggering
    // a state change to paused. To handle this we let updateState change the
    // state accordingly (as we need to allow the UI to update itself, and
    // immediately after that we change back to buffering again.
    // This loop can only be interrupted by a state change to !Playing & !Paused
    // or by reaching a 100 % buffer caching (i.e. full cache).

    m_buffering = true;
    if (m_state != BufferingState) {
        m_stateAfterBuffering = m_state;
        changeState(BufferingState);
    }

    emit bufferStatus(percent);

    // Transit to actual state only after emission so the signal is still
    // delivered while in BufferingState.
    if (percent >= 100) { // http://trac.videolan.org/vlc/ticket/5277
        m_buffering = false;
        changeState(m_stateAfterBuffering);
    }
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
