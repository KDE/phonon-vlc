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

class QString;

struct libvlc_event_t;
struct libvlc_media_player_t;

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

    MediaPlayer(QObject *parent = 0);
    ~MediaPlayer();

    inline libvlc_media_player_t *libvlc_media_player() const { return m_player; }

    void setMedia(Media *media);

    // Playback
    bool play();
    void pause();
    void resume();
    void togglePause();
    void stop();

    qint64 time() const;
    void setTime(qint64 newTime);

    // Video
    bool hasVideoOutput() const;

    bool setSubtitle(int subtitle);
    bool setSubtitle(const QString &file);

    void setTitle(int title);

    void setChapter(int chapter);

    // Audio
    bool setAudioTrack(int track);

signals:
    void lengthChanged(qint64 length);
    void seekableChanged(bool seekable);
    void stateChanged(MediaPlayer::State state);
    void timeChanged(qint64 time);

private:
    static void event_cb(const libvlc_event_t *event, void *opaque);

    Media *m_media;

    libvlc_media_player_t *m_player;
};

QDebug operator<<(QDebug dbg, const MediaPlayer::State &s);

} // namespace VLC
} // namespace Phonon

#endif // PHONON_VLC_MEDIAPLAYER_H
