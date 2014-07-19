/*
    Copyright (C) 2011 Harald Sitter <sitter@kde.org>

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

#ifndef PHONON_VLC_MEDIAPLAYER_H
#define PHONON_VLC_MEDIAPLAYER_H

#include <QtCore/QObject>
#include <QtCore/QSize>

#include <vlc/libvlc_version.h>
#include <vlc/vlc.h>

class QImage;
class QString;

namespace Phonon {
namespace VLC {

class Media;

class MediaPlayer : public QObject
{
    Q_OBJECT
public:
    enum State {
        NoState = 0,
        OpeningState,
        BufferingState,
        PlayingState,
        PausedState,
        StoppedState,
        EndedState,
        ErrorState
    };

    explicit MediaPlayer(QObject *parent = 0);
    ~MediaPlayer();

    inline libvlc_media_player_t *libvlc_media_player() const { return m_player; }
    inline operator libvlc_media_player_t *() const { return m_player; }

    void setMedia(Media *media);

    void setVideoCallbacks();
    void setVideoFormatCallbacks();

    void setNsObject(void *drawable) { libvlc_media_player_set_nsobject(m_player, drawable); }
    void setXWindow(quint32 drawable) { libvlc_media_player_set_xwindow(m_player, drawable); }
    void setHwnd(void *drawable) { libvlc_media_player_set_hwnd(m_player, drawable); }

    // Playback
    bool play();
    void pause();
    void pausedPlay();
    void resume();
    void togglePause();
    Q_INVOKABLE void stop();

    qint64 length() const;
    qint64 time() const;
    void setTime(qint64 newTime);

    bool isSeekable() const;

    // Video
    QSize videoSize() const
    {
        unsigned int width;
        unsigned int height;
        libvlc_video_get_size(m_player, 0, &width, &height);
        return QSize(width, height);
    }

    bool hasVideoOutput() const;

    /// Set new video aspect ratio.
    /// \param aspect new video aspect-ratio or empty to reset to default
    void setVideoAspectRatio(const QByteArray &aspect)
    { libvlc_video_set_aspect_ratio(m_player, aspect.isEmpty() ? 0 : aspect.data()); }

    void setVideoAdjust(libvlc_video_adjust_option_t adjust, int value)
    { libvlc_video_set_adjust_int(m_player, adjust, value); }

    void setVideoAdjust(libvlc_video_adjust_option_t adjust, float value)
    { libvlc_video_set_adjust_float(m_player, adjust, value); }

    int subtitle() const
    { return libvlc_video_get_spu(m_player); }

    libvlc_track_description_t *videoSubtitleDescription() const
    { return libvlc_video_get_spu_description(m_player); }

    bool setSubtitle(int subtitle);
    bool setSubtitle(const QString &file);

    int title() const
    { return libvlc_media_player_get_title(m_player); }

    int titleCount() const
    { return libvlc_media_player_get_title_count(m_player); }

    libvlc_track_description_t *titleDescription() const
    { return libvlc_video_get_title_description(m_player); }

    void setTitle(int title);

    int videoChapterCount() const
    { return libvlc_media_player_get_chapter_count(m_player); }

    libvlc_track_description_t *videoChapterDescription(int title) const
    { return libvlc_video_get_chapter_description(m_player, title); }

    void setChapter(int chapter);

    /** Reentrant, through libvlc */
    QImage snapshot() const;

    // Audio
    /// Get current audio volume.
    /// \return the software volume in percents (0 = mute, 100 = nominal / 0dB)
    int audioVolume() const { return m_volume; }

    /// Set new audio volume.
    /// \param volume new volume
    void setAudioVolume(int volume);

    /// Set the fade percentage, between 0 (muted) and 1.0 (no fade)
    void setAudioFade(qreal fade);

    /// \param name name of the output to set
    /// \returns \c true when setting was successful, \c false otherwise
    bool setAudioOutput(const QByteArray &name)
    { return libvlc_audio_output_set(m_player, name.data()) == 0; }

    /**
     * Set audio output device by name.
     * \param outputName the aout name (pulse, alsa, oss, etc.)
     * \param deviceName the output name (aout dependent)
     */
    void setAudioOutputDevice(const QByteArray &outputName, const QByteArray &deviceName)
    { libvlc_audio_output_device_set(m_player, outputName.data(), deviceName.data()); }

    int audioTrack() const
    { return libvlc_audio_get_track(m_player); }

    libvlc_track_description_t * audioTrackDescription() const
    { return libvlc_audio_get_track_description(m_player); }

    bool setAudioTrack(int track);

    void setCdTrack(int track);

#if (LIBVLC_VERSION_INT >= LIBVLC_VERSION(2, 2, 0, 0))
    void setEqualizer(libvlc_equalizer_t *equalizer);
#endif

signals:
    void lengthChanged(qint64 length);
    void seekableChanged(bool seekable);
    void stateChanged(MediaPlayer::State state);
    void timeChanged(qint64 time);
    void bufferChanged(int percent);

    /** Emitted when the vout availability has changed */
    void hasVideoChanged(bool hasVideo);

private:
    static void event_cb(const libvlc_event_t *event, void *opaque);
    void setVolumeInternal();

    Media *m_media;

    libvlc_media_player_t *m_player;

    bool m_doingPausedPlay;
    int m_volume;
    qreal m_fadeAmount;
};

QDebug operator<<(QDebug dbg, const MediaPlayer::State &s);

} // namespace VLC
} // namespace Phonon

#endif // PHONON_VLC_MEDIAPLAYER_H
