/*
    Copyright (C) 2007-2008 Tanguy Krotoff <tkrotoff@gmail.com>
    Copyright (C) 2008 Lukas Durfina <lukas.durfina@gmail.com>
    Copyright (C) 2009 Fathi Boudra <fabo@kde.org>
    Copyright (C) 2010 Ben Cooksley <sourtooth@gmail.com>
    Copyright (C) 2009-2011 vlc-phonon AUTHORS <kde-multimedia@kde.org>
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
#include <QtCore/QDir>

#include <vlc/libvlc_version.h>
#include <vlc/vlc.h>

#include "utils/debug.h"
#include "utils/libvlc.h"
#include "media.h"
#include "connector.h"
//#include "streamreader.h"

//Time in milliseconds before sending aboutToFinish() signal
//2 seconds
static const int ABOUT_TO_FINISH_TIME = 2000;

namespace Phonon {
namespace VLC {

Player::Player(QObject *parent)
    : QObject(parent)
    , m_nextSource(Source(QUrl()))
    , m_streamReader(0)
    , m_state(Phonon::StoppedState)
    , m_tickInterval(0)
    , m_transitionTime(0)
    , m_media(0)
    , m_player(0)
{
    qRegisterMetaType<QMultiMap<QString, QString> >("QMultiMap<QString, QString>");

    m_player = new MediaPlayer(this);
    if (!m_player->libvlc_media_player())
        error() << "libVLC:" << LibVLC::errorMessage();

    // Player signals.
    connect(m_player, SIGNAL(seekableChanged(bool)), this, SIGNAL(seekableChanged(bool)));
    connect(m_player, SIGNAL(timeChanged(qint64)), this, SLOT(onTimeChanged(qint64)));
    connect(m_player, SIGNAL(stateChanged(MediaPlayer::State)), this, SLOT(updateState(MediaPlayer::State)));
    connect(m_player, SIGNAL(hasVideoChanged(bool)), this, SLOT(onHasVideoChanged(bool)));
    connect(m_player, SIGNAL(bufferChanged(int)), this, SLOT(setBufferStatus(int)));

    // Internal Signals.
    connect(this, SIGNAL(moveToNext()), SLOT(moveToNextSource()));

    resetMembers();
}

Player::~Player()
{
    unloadMedia();
}

void Player::resetMembers()
{
    // default to -1, so that streams won't break and to comply with the docs (-1 if unknown)
    m_totalTime = -1;
    m_hasVideo = false;
    m_seekpoint = 0;

    m_prefinishEmitted = false;
    m_aboutToFinishEmitted = false;

    m_lastTick = 0;

    m_timesVideoChecked = 0;
}

void Player::play()
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
        setupMedia();
        if (m_player->play()){}
//            error() << "libVLC:" << LibVLC::errorMessage();
        break;
    }
}

void Player::pause()
{
    DEBUG_BLOCK;
    switch (m_state) {
    case PlayingState:
        m_player->pause();
        break;
    case PausedState:
        return;
    default:
        debug() << "doing paused play";
        setupMedia();
        m_player->pausedPlay();
        break;
    }
}

void Player::stop()
{
    DEBUG_BLOCK;
//    if (m_streamReader)
//        m_streamReader->unlock();
    m_nextSource = Source(QUrl());
    m_player->stop();
}

void Player::seek(qint64 milliseconds)
{
    DEBUG_BLOCK;

    switch (m_state) {
    case PlayingState:
    case PausedState:
        break;
    default:
        // Seeking while not being in a playingish state is cached for later.
        m_seekpoint = milliseconds;
        return;
    }

    debug() << "seeking" << milliseconds << "msec";

    m_player->setTime(milliseconds);

    const qint64 _time = time();
    const qint64 total = totalTime();

    // Reset last tick marker so we emit time even after seeking
    if (_time < m_lastTick)
        m_lastTick = _time;
    if (_time < total - m_prefinishMark)
        m_prefinishEmitted = false;
    if (_time < total - ABOUT_TO_FINISH_TIME)
        m_aboutToFinishEmitted = false;
}

void Player::onTimeChanged(qint64 time)
{
    const qint64 totalTime = m_totalTime;

    switch (m_state) {
    case PlayingState:
    case PausedState:
        emitTimeChange(time);
    default:
        break;
    }

    if (m_state == PlayingState) {
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

void Player::emitTimeChange(qint64 time)
{
    if (m_tickInterval == 0) // Make sure we do not ever emit ticks when deactivated.
        return;
    if (time + m_tickInterval >= m_lastTick) {
        m_lastTick = time;
        emit timeChanged(time);
    }
}

void Player::loadMedia(const QByteArray &mrl)
{
    DEBUG_BLOCK;
    changeState(Phonon::StoppedState);
    m_mrl = mrl;
    debug() << "loading encoded:" << m_mrl;
}

void Player::loadMedia(const QString &mrl)
{
    loadMedia(mrl.toUtf8());
}

qint32 Player::tickInterval() const
{
    return m_tickInterval;
}

/**
 * Supports runtime changes.
 * If the user goes to tick(0) we stop the timer, otherwise we fire it up.
 */
void Player::setTickInterval(qint32 interval)
{
    m_tickInterval = interval;
}

qint64 Player::time() const
{
    qint64 time = -1;

    switch (state()) {
    case Phonon::PausedState:
    case Phonon::PlayingState:
        time = m_player->time();
        break;
    case Phonon::StoppedState:
        time = 0;
        break;
    }

    return time;
}

Phonon::State Player::state() const
{
    return m_state;
}

Phonon::ErrorType Player::errorType() const
{
    return Phonon::NormalError;
}

Source Player::source() const
{
    return m_mediaSource;
}

void Player::setSource(const Source &source)
{
    DEBUG_BLOCK;

    // Reset previous streamereaders
//    if (m_streamReader) {
//        m_streamReader->unlock();
//        delete m_streamReader;
//        m_streamReader = 0;
//        // For streamreaders we exchanage the player's seekability with the
//        // reader's so here we change it back.
//        // Note: the reader auto-disconnects due to destruction.
//        connect(m_player, SIGNAL(seekableChanged(bool)), this, SIGNAL(seekableChanged(bool)));
//    }

    // Reset previous isScreen flag
    m_isScreen = false;

    m_mediaSource = source;

    if (!source.url().isEmpty()) {
        /*
         * Source specified by URL
         */
        QByteArray url;
        debug() << "MediaSource::Url:" << source.url();
        if (source.url().scheme().isEmpty()) {
            url = "file:///";
            if (source.url().isRelative())
                url.append(QFile::encodeName(QDir::currentPath()) + '/');
        }
        url += source.url().toEncoded();
        loadMedia(url);
    } else if (source.deviceType() != Source::NoDevice) {
        /*
         * Source specified by device
         */
        switch (source.deviceType()) {
        case Source::AudioCd:
            loadMedia(QLatin1Literal("cdda://") % source.deviceName());
            break;
        case Source::VideoCd:
            loadMedia(QLatin1Literal("vcd://") % source.deviceName());
            break;
        case Source::Dvd:
            loadMedia(QLatin1Literal("dvd://") % source.deviceName());
            break;
        case Source::BluRay:
            loadMedia(QLatin1Literal("bluray://") % source.deviceName());
            break;
        case Source::AudioCapture:
        case Source::VideoCapture:
            // TODO (tweaking of Phonon::Source needed)
            error() << Q_FUNC_INFO << "Capture not yet implemented by Phonon-VLC";
            break;
        default:
            error() << Q_FUNC_INFO << "Source device type not yet implemented by Phonon-VLC";
        }
    } else if (source.stream()) {
        /*
         * Source specified by stream
         */
//        m_streamReader = new StreamReader(this);
//        // LibVLC refuses to emit seekability as it does a try-and-seek approach
//        // to work around this we exchange the player's seekability signal
//        // for the readers
//        // https://bugs.kde.org/show_bug.cgi?id=293012
//        connect(m_streamReader, SIGNAL(streamSeekableChanged(bool)), this, SIGNAL(seekableChanged(bool)));
//        disconnect(m_player, SIGNAL(seekableChanged(bool)), this, SIGNAL(seekableChanged(bool)));
//        // Only connect now to avoid seekability detection before we are connected.
//        m_streamReader->connectToSource(source);
//        loadMedia(QByteArray("imem://"));
    } else {
        error() << Q_FUNC_INFO << "Source empty or source type not yet implemented by Phonon-VLC";
    }

    debug() << "Sending currentSourceChanged";
    emit sourceChanged(m_mediaSource);
}

void Player::setNextSource(const Source &source)
{
    DEBUG_BLOCK;
    debug() << source.url();
    m_nextSource = source;
    // This function is not ever called by the consumer but only libphonon.
    // Furthermore libphonon only calls this function in its aboutToFinish slot,
    // iff sources are already in the queue. In case our aboutToFinish was too
    // late we may already be stopped when the slot gets activated.
    // Therefore we need to make sure that we move to the next source iff
    // this function is called when we are in stoppedstate.
    if (m_state == StoppedState)
        moveToNext();
}

qint32 Player::prefinishMark() const
{
    return m_prefinishMark;
}

void Player::setPrefinishMark(qint32 msecToEnd)
{
    m_prefinishMark = msecToEnd;
    if (time() < totalTime() - m_prefinishMark) {
        // Not about to finish
        m_prefinishEmitted = false;
    }
}

qint32 Player::transitionTime() const
{
    return m_transitionTime;
}

void Player::setTransitionTime(qint32 time)
{
    m_transitionTime = time;
}

void Player::emitAboutToFinish()
{
    if (!m_aboutToFinishEmitted) {
        // Track is about to finish
        m_aboutToFinishEmitted = true;
        emit aboutToFinish();
    }
}

// State changes are force queued by libphonon.
void Player::changeState(Phonon::State newState)
{
    DEBUG_BLOCK;

    // State not changed
    if (newState == m_state)
        return;

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

void Player::moveToNextSource()
{
    DEBUG_BLOCK;

    setSource(m_nextSource);
    play();
    m_nextSource = Source(QUrl());
}

inline bool Player::hasNextTrack()
{
    return false;
#warning not backed
//    return m_nextSource.type() != Source::Invalid && m_nextSource.type() != Source::Empty;
}

inline void Player::unloadMedia()
{
    if (m_media) {
        m_media->disconnect(this);
        m_media->deleteLater();
        m_media = 0;
    }
}

void Player::setupMedia()
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

//    if (source().discType() == Cd && m_currentTitle > 0) {
//        debug() << "setting CDDA track";
//        m_media->addOption(QLatin1String("cdda-track="), QVariant(m_currentTitle));
//    }

//    if (m_streamReader)
//        // StreamReader is no sink but a source, for this we have no concept right now
//        // also we do not need one since the reader is the only source we have.
//        // Consequently we need to manually tell the StreamReader to attach to the Media.
//        m_streamReader->addToMedia(m_media);

    foreach (Connector *sink, m_attachments) {
        sink->addToMedia(m_media);
    }

    // Connect to Media signals. Disconnection is done at unloading.
    connect(m_media, SIGNAL(durationChanged(qint64)),
            this, SLOT(updateDuration(qint64)));
    connect(m_media, SIGNAL(metaDataChanged()),
            this, SLOT(updateMetaData()));

    // Play
    m_player->setMedia(m_media);
}

QString Player::errorString() const
{
    return libvlc_errmsg();
}

bool Player::hasVideo() const
{
    return m_player->hasVideoOutput();
}

bool Player::isSeekable() const
{
//    if (m_streamReader)
//        return m_streamReader->streamSeekable();
    return m_player->isSeekable();
}

void Player::updateDuration(qint64 newDuration)
{
    // This here cache is needed because we need to provide -1 as totalTime()
    // for as long as we do not get a proper update through this slot.
    // VLC reports -1 with no media but 0 if it does not know the duration, so
    // apps that assume 0 = unknown get screwed if they query too early.
    // http://bugs.tomahawk-player.org/browse/TWK-1029
    m_totalTime = newDuration;
    emit totalTimeChanged(m_totalTime);
}

void Player::updateMetaData()
{
    QMultiMap<MetaData, QString> metaDataMap;

    qDebug() << "VLC MetaData:";
    qDebug() << "    libvlc_meta_Title ->" << m_media->meta(libvlc_meta_Title);
    qDebug() << "    libvlc_meta_Artist ->" << m_media->meta(libvlc_meta_Artist);
    qDebug() << "    libvlc_meta_Genre ->" << m_media->meta(libvlc_meta_Genre);
    qDebug() << "    libvlc_meta_Copyright ->" << m_media->meta(libvlc_meta_Copyright);
    qDebug() << "    libvlc_meta_Album ->" << m_media->meta(libvlc_meta_Album);
    qDebug() << "    libvlc_meta_TrackNumber ->" << m_media->meta(libvlc_meta_TrackNumber);
    qDebug() << "    libvlc_meta_Description ->" << m_media->meta(libvlc_meta_Description);
    qDebug() << "    libvlc_meta_Rating ->" << m_media->meta(libvlc_meta_Rating);
    qDebug() << "    libvlc_meta_Date ->" << m_media->meta(libvlc_meta_Date);
    qDebug() << "    libvlc_meta_Setting ->" << m_media->meta(libvlc_meta_Setting);
    qDebug() << "    libvlc_meta_URL ->" << m_media->meta(libvlc_meta_URL);
    qDebug() << "    libvlc_meta_Language ->" << m_media->meta(libvlc_meta_Language);
    qDebug() << "    libvlc_meta_NowPlaying ->" << m_media->meta(libvlc_meta_NowPlaying);
    qDebug() << "    libvlc_meta_Publisher ->" << m_media->meta(libvlc_meta_Publisher);
    qDebug() << "    libvlc_meta_EncodedBy ->" << m_media->meta(libvlc_meta_EncodedBy);
    qDebug() << "    libvlc_meta_ArtworkURL ->" << m_media->meta(libvlc_meta_ArtworkURL);
    qDebug() << "    libvlc_meta_TrackID ->" << m_media->meta(libvlc_meta_TrackID);

    const QString artist = m_media->meta(libvlc_meta_Artist);
    const QString title = m_media->meta(libvlc_meta_Title);
    const QString nowPlaying = m_media->meta(libvlc_meta_NowPlaying);

    // Streams sometimes have the artist and title munged in nowplaying.
    // With ALBUM = Title and TITLE = NowPlaying it will still show up nicely in Amarok.
    if (artist.isEmpty() && !nowPlaying.isEmpty()) {
        metaDataMap.insert(AlbumMetaData, title);
        metaDataMap.insert(TitleMetaData, nowPlaying);
    } else {
        metaDataMap.insert(AlbumMetaData, m_media->meta(libvlc_meta_Album));
        metaDataMap.insert(TitleMetaData, title);
    }

    metaDataMap.insert(ArtistMetaData, artist);
    metaDataMap.insert(DateMetaData, m_media->meta(libvlc_meta_Date));
    metaDataMap.insert(GenreMetaData, m_media->meta(libvlc_meta_Genre));
    metaDataMap.insert(TrackNumberMetaData, m_media->meta(libvlc_meta_TrackNumber));
    metaDataMap.insert(DescriptionMetaData, m_media->meta(libvlc_meta_Description));
#warning some metadata missing
//    metaDataMap.insert(MetaData::, m_media->meta(libvlc_meta_Copyright));
//    metaDataMap.insert(QLatin1String("URL"), m_media->meta(libvlc_meta_URL));
//    metaDataMap.insert(QLatin1String("ENCODEDBY"), m_media->meta(libvlc_meta_EncodedBy));

    if (metaDataMap == m_vlcMetaData) {
        // No need to issue any change, the data is the same
        return;
    }
    m_vlcMetaData = metaDataMap;

    emit metaDataChanged(metaDataMap);
}

void Player::updateState(MediaPlayer::State state)
{
    DEBUG_BLOCK;
    debug() << state;

    switch (state) {
    case MediaPlayer::PlayingState:
        changeState(PlayingState);
        break;
    case MediaPlayer::PausedState:
        changeState(PausedState);
        break;
    case MediaPlayer::OpeningState:
    case MediaPlayer::NoState:
    case MediaPlayer::StoppedState:
        changeState(StoppedState);
        break;
    case MediaPlayer::EndedState:
        if (hasNextTrack())
            moveToNextSource();
        else {
            emitAboutToFinish();
            emit finished();
            changeState(StoppedState);
        }
        break;
//    case MediaPlayer::ErrorState:
//        debug() << errorString();
//        emitAboutToFinish();
//        emit finished();
//        changeState(ErrorState);
//        break;
    }

    return;
}

void Player::onHasVideoChanged(bool hasVideo)
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
}

void Player::setBufferStatus(int percent)
{
#warning vlc has no buffering end event, so this at least needs frontend documentation that 100=end, or special handling
// http://trac.videolan.org/vlc/ticket/5277
    emit bufferStatus(percent);
}

void Player::addOutput(QObject *output)
{
    DEBUG_BLOCK;
    debug() << output;
    Connector *connector = dynamic_cast<Connector *>(output);
    if (connector)
        connector->connectPlayer(this);
    else
        warning() << "Output does not seem to be a Connector.";
}

qint64 Player::totalTime() const
{
    return m_totalTime;
}

void Player::attach(Connector *connector)
{
    Q_ASSERT(!m_attachments.contains(connector));
    m_attachments.append(connector);
}

void Player::remove(Connector *connector)
{
    Q_ASSERT(connector);
    m_attachments.removeAll(connector);
}

} // namespace VLC
} // namespace Phonon
