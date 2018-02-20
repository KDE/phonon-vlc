/*
    Copyright (C) 2011-2018 Harald Sitter <sitter@kde.org>

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

#include "mediaplayer.h"

#include <QtCore/QDebug>
#include <QtCore/QDir>
#include <QtCore/QMetaType>
#include <QtCore/QString>
#include <QtCore/QTemporaryFile>
#include <QtGui/QImage>

#include <vlc/libvlc_version.h>

#include "utils/libvlc.h"
#include "media.h"

// Callbacks come from a VLC thread. In some cases Qt fails to detect this and
// tries to invoke directly (i.e. from same thread). This can lead to thread
// pollution throughout Phonon, which is very much not desired.
#define P_EMIT_HAS_VIDEO(hasVideo) \
    QMetaObject::invokeMethod(\
        that, "hasVideoChanged", \
        Qt::QueuedConnection, \
        Q_ARG(bool, hasVideo))

#define P_EMIT_STATE(__state) \
    QMetaObject::invokeMethod(\
        that, "stateChanged", \
        Qt::QueuedConnection, \
        Q_ARG(MediaPlayer::State, __state))

namespace Phonon {
namespace VLC {

MediaPlayer::MediaPlayer(QObject *parent)
    : QObject(parent)
    , m_media(0)
    , m_player(libvlc_media_player_new(pvlc_libvlc))
    , m_doingPausedPlay(false)
    , m_volume(75)
    , m_fadeAmount(1.0f)
{
    Q_ASSERT(m_player);

    qRegisterMetaType<MediaPlayer::State>("MediaPlayer::State");

    libvlc_event_manager_t *manager = libvlc_media_player_event_manager(m_player);
    libvlc_event_type_t events[] = {
        libvlc_MediaPlayerMediaChanged,
        libvlc_MediaPlayerNothingSpecial,
        libvlc_MediaPlayerOpening,
        libvlc_MediaPlayerBuffering,
        libvlc_MediaPlayerPlaying,
        libvlc_MediaPlayerPaused,
        libvlc_MediaPlayerStopped,
        libvlc_MediaPlayerForward,
        libvlc_MediaPlayerBackward,
        libvlc_MediaPlayerEndReached,
        libvlc_MediaPlayerEncounteredError,
        libvlc_MediaPlayerTimeChanged,
        libvlc_MediaPlayerPositionChanged,
        libvlc_MediaPlayerSeekableChanged,
        libvlc_MediaPlayerPausableChanged,
        libvlc_MediaPlayerTitleChanged,
        libvlc_MediaPlayerSnapshotTaken,
        libvlc_MediaPlayerLengthChanged,
        libvlc_MediaPlayerVout
    #if (LIBVLC_VERSION_INT >= LIBVLC_VERSION(2, 2, 2, 0))
        , libvlc_MediaPlayerCorked,
        libvlc_MediaPlayerUncorked,
        libvlc_MediaPlayerMuted,
        libvlc_MediaPlayerUnmuted,
        libvlc_MediaPlayerAudioVolume
    #endif
    };
    const int eventCount = sizeof(events) / sizeof(*events);
    for (int i = 0; i < eventCount; ++i) {
        libvlc_event_attach(manager, events[i], event_cb, this);
    }

    // Deactivate video title overlay (i.e. name of the video displaying
    // at start. Since 2.1 that is handled via the API which in general is more
    // reliable than setting it via libvlc_new (or so I have been told....)
    libvlc_media_player_set_video_title_display(m_player, libvlc_position_disable, 0);
}

MediaPlayer::~MediaPlayer()
{
    libvlc_media_player_release(m_player);
}

void MediaPlayer::setMedia(Media *media)
{
    m_media = media;
    libvlc_media_player_set_media(m_player, *m_media);
}

bool MediaPlayer::play()
{
    m_doingPausedPlay = false;
    return libvlc_media_player_play(m_player) == 0;
}

void MediaPlayer::pause()
{
    m_doingPausedPlay = false;
    libvlc_media_player_set_pause(m_player, 1);
}

void MediaPlayer::pausedPlay()
{
    m_doingPausedPlay = true;
    libvlc_media_player_play(m_player);
}

void MediaPlayer::resume()
{
    m_doingPausedPlay = false;
    libvlc_media_player_set_pause(m_player, 0);
}

void MediaPlayer::togglePause()
{
    libvlc_media_player_pause(m_player);
}

void MediaPlayer::stop()
{
    m_doingPausedPlay = false;
    libvlc_media_player_stop(m_player);
}

qint64 MediaPlayer::length() const
{
    return libvlc_media_player_get_length(m_player);
}

qint64 MediaPlayer::time() const
{
    return libvlc_media_player_get_time(m_player);
}

void MediaPlayer::setTime(qint64 newTime)
{
    libvlc_media_player_set_time(m_player, newTime);
}

bool MediaPlayer::isSeekable() const
{
    return libvlc_media_player_is_seekable(m_player);
}

bool MediaPlayer::hasVideoOutput() const
{
    return libvlc_media_player_has_vout(m_player) > 0;
}

bool MediaPlayer::setSubtitle(int subtitle)
{
    return libvlc_video_set_spu(m_player, subtitle) == 0;
}

bool MediaPlayer::setSubtitle(const QString &file)
{
#if (LIBVLC_VERSION_INT >= LIBVLC_VERSION(3, 0, 0, 0))
    return libvlc_media_player_add_slave(m_player,
                                         libvlc_media_slave_type_subtitle,
                                         file.toUtf8().data(),
                                         true) == 0;
#else
    return libvlc_video_set_subtitle_file(m_player, file.toUtf8().data()) == 1;
#endif
}

void MediaPlayer::setTitle(int title)
{
    libvlc_media_player_set_title(m_player, title);
}

void MediaPlayer::setChapter(int chapter)
{
    libvlc_media_player_set_chapter(m_player, chapter);
}

QImage MediaPlayer::snapshot() const
{
    QTemporaryFile tempFile(QDir::tempPath() % QDir::separator() % QLatin1Literal("phonon-vlc-snapshot"));
    tempFile.open();

    // This function is sync.
    if (libvlc_video_take_snapshot(m_player, 0, tempFile.fileName().toLocal8Bit().data(), 0, 0) != 0)
        return QImage();

    return QImage(tempFile.fileName());
}

bool MediaPlayer::setAudioTrack(int track)
{
    return libvlc_audio_set_track(m_player, track) == 0;
}

void MediaPlayer::event_cb(const libvlc_event_t *event, void *opaque)
{
    MediaPlayer *that = reinterpret_cast<MediaPlayer *>(opaque);
    Q_ASSERT(that);

    // Do not forget to register for the events you want to handle here!
    switch (event->type) {
    case libvlc_MediaPlayerTimeChanged:
        QMetaObject::invokeMethod(
                    that, "timeChanged",
                    Qt::QueuedConnection,
                    Q_ARG(qint64, event->u.media_player_time_changed.new_time));
        break;
    case libvlc_MediaPlayerSeekableChanged:
        QMetaObject::invokeMethod(
                    that, "seekableChanged",
                    Qt::QueuedConnection,
                    Q_ARG(bool, event->u.media_player_seekable_changed.new_seekable));
        break;
    case libvlc_MediaPlayerLengthChanged:
        QMetaObject::invokeMethod(
                    that, "lengthChanged",
                    Qt::QueuedConnection,
                    Q_ARG(qint64, event->u.media_player_length_changed.new_length));
        break;
    case libvlc_MediaPlayerNothingSpecial:
        P_EMIT_STATE(NoState);
        break;
    case libvlc_MediaPlayerOpening:
        P_EMIT_STATE(OpeningState);
        break;
    case libvlc_MediaPlayerBuffering:
        QMetaObject::invokeMethod(
                    that, "bufferChanged",
                    Qt::QueuedConnection,
                    Q_ARG(int, event->u.media_player_buffering.new_cache));
        break;
    case libvlc_MediaPlayerPlaying:
        // Intercept state change and apply pausing once playing.
        if (that->m_doingPausedPlay) {
            that->m_doingPausedPlay = false;
            // VLC internally will call stop if a player can not be paused, this
            // can lead to deadlocks as stop is partially blocking, to avoid this
            // we explicitly do a queued stop whenever a player can not be paused.
            // In those situations pausedplay capability can not be provided, so
            // applications wanting to do pausedplay will need to handle it anyway
            // as faking a paused state when there is none would be a very code
            // intense workaround asking for weird abstraction leakage.
            // See kde bug 337604.
            if (libvlc_media_player_can_pause(that->m_player)) {
                that->pause();
            } else {
                QMetaObject::invokeMethod(that, "stop", Qt::QueuedConnection);
            }
        } else
            P_EMIT_STATE(PlayingState);
        break;
    case libvlc_MediaPlayerPaused:
        P_EMIT_STATE(PausedState);
        break;
    case libvlc_MediaPlayerStopped:
        P_EMIT_STATE(StoppedState);
        break;
    case libvlc_MediaPlayerEndReached:
        P_EMIT_STATE(EndedState);
        break;
    case libvlc_MediaPlayerEncounteredError:
        P_EMIT_STATE(ErrorState);
        break;
    case libvlc_MediaPlayerVout:
        if (event->u.media_player_vout.new_count > 0) {
            P_EMIT_HAS_VIDEO(true);
        } else {
            P_EMIT_HAS_VIDEO(false);
        }
        break;
    case libvlc_MediaPlayerMediaChanged:
        break;
#if (LIBVLC_VERSION_INT >= LIBVLC_VERSION(2, 2, 2, 0))
    case libvlc_MediaPlayerCorked:
        that->pause();
        break;
    case libvlc_MediaPlayerUncorked:
        that->play();
        break;
    case libvlc_MediaPlayerMuted:
        QMetaObject::invokeMethod(
                    that, "mutedChanged",
                    Qt::QueuedConnection,
                    Q_ARG(bool, true));
        break;
    case libvlc_MediaPlayerUnmuted:
        QMetaObject::invokeMethod(
                    that, "mutedChanged",
                    Qt::QueuedConnection,
                    Q_ARG(bool, false));
        break;
    case libvlc_MediaPlayerAudioVolume:
        QMetaObject::invokeMethod(
                    that, "volumeChanged",
                    Qt::QueuedConnection,
                    Q_ARG(float, event->u.media_player_audio_volume.volume));
        break;
#endif
    case libvlc_MediaPlayerForward:
    case libvlc_MediaPlayerBackward:
    case libvlc_MediaPlayerPositionChanged:
    case libvlc_MediaPlayerPausableChanged:
    case libvlc_MediaPlayerTitleChanged:
    case libvlc_MediaPlayerSnapshotTaken: // Snapshot call is sync, so this is useless.
    default:
        break;
        QString msg = QString("Unknown event: ") + QString(libvlc_event_type_name(event->type));
        Q_ASSERT_X(false, "event_cb", qPrintable(msg));
        break;
    }
}

QDebug operator<<(QDebug dbg, const MediaPlayer::State &s)
{
    QString name;
    switch (s) {
    case MediaPlayer::NoState:
        name = QLatin1String("MediaPlayer::NoState");
        break;
    case MediaPlayer::OpeningState:
        name = QLatin1String("MediaPlayer::OpeningState");
        break;
    case MediaPlayer::BufferingState:
        name = QLatin1String("MediaPlayer::BufferingState");
        break;
    case MediaPlayer::PlayingState:
        name = QLatin1String("MediaPlayer::PlayingState");
        break;
    case MediaPlayer::PausedState:
        name = QLatin1String("MediaPlayer::PausedState");
        break;
    case MediaPlayer::StoppedState:
        name = QLatin1String("MediaPlayer::StoppedState");
        break;
    case MediaPlayer::EndedState:
        name = QLatin1String("MediaPlayer::EndedState");
        break;
    case MediaPlayer::ErrorState:
        name = QLatin1String("MediaPlayer::ErrorState");
        break;
    }
    dbg.nospace() << "State(" << qPrintable(name) << ")";
    return dbg.space();
}

void MediaPlayer::setAudioFade(qreal fade)
{
    m_fadeAmount = fade;
    setVolumeInternal();
}

void MediaPlayer::setAudioVolume(int volume)
{
    m_volume = volume;
    setVolumeInternal();
}

bool MediaPlayer::mute() const
{
    return libvlc_audio_get_mute(m_player);
}

void MediaPlayer::setMute(bool mute)
{
    libvlc_audio_set_mute(m_player, mute);
}

void MediaPlayer::setVolumeInternal()
{
    libvlc_audio_set_volume(m_player, m_volume * m_fadeAmount);
}

void MediaPlayer::setCdTrack(int track)
{
    if (!m_media)
        return;
    libvlc_media_player_stop(m_player);
    m_media->setCdTrack(track);
    libvlc_media_player_set_media(m_player, *m_media);
    libvlc_media_player_play(m_player);
}

#if (LIBVLC_VERSION_INT >= LIBVLC_VERSION(2, 2, 0, 0))
void MediaPlayer::setEqualizer(libvlc_equalizer_t *equalizer)
{
    libvlc_media_player_set_equalizer(m_player, equalizer);
}
#endif

} // namespace VLC
} // namespace Phonon
