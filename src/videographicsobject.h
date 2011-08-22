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

#ifndef PHONON_VLC_VIDEOGRAPHICSOBJECT_H
#define PHONON_VLC_VIDEOGRAPHICSOBJECT_H

#include <QtCore/QObject>
#include <QtCore/QMutex>

#include <phonon/videoframe.h>
#include <phonon/videographicsobject.h>
#include <phonon/videographicsobjectinterface.h>

#include <vlc/libvlc_version.h>

#include "sinknode.h"

struct libvlc_media_t;

namespace Phonon {
namespace VLC {

class VideoGraphicsObject1point1 : public QObject,
                                   public VideoGraphicsObjectInterface,
                                   public SinkNode
{
    Q_OBJECT
    Q_INTERFACES(Phonon::VideoGraphicsObjectInterface)
public:
    VideoGraphicsObject1point1(QObject *parent = 0);
    virtual ~VideoGraphicsObject1point1();
    virtual void connectToMediaObject(MediaObject *mediaObject);

    Phonon::VideoGraphicsObject *videoGraphicsObject() { return m_videoGraphicsObject; }
    void setVideoGraphicsObject(Phonon::VideoGraphicsObject *object) { m_videoGraphicsObject = object; }

    void lock();
    bool tryLock();
    void unlock();

    const VideoFrame *frame() const { return &m_frame; }

    static void *lock_cb(void *opaque, void **planes);
    static void unlock_cb(void *opaque, void *picture, void *const *planes);
    static void display_cb(void *opaque, void *picture);

signals:
    void frameReady();
    void reset();

protected:
    QMutex m_mutex;

    Phonon::VideoGraphicsObject *m_videoGraphicsObject;
    Phonon::VideoFrame m_frame;
};

#if defined(P_LIBVLC12)
class VideoGraphicsObject : public VideoGraphicsObject1point1
{
    Q_OBJECT
public:
    VideoGraphicsObject(QObject *parent = 0);
    virtual ~VideoGraphicsObject();
    virtual void connectToMediaObject(MediaObject *mediaObject);
    virtual void disconnectFromMediaObject(MediaObject *mediaObject);

    static unsigned int format_cb(void **opaque, char *chroma,
                                  unsigned int *width, unsigned int *height,
                                  unsigned int *pitches,
                                  unsigned int *lines);
    static void cleanup_cb(void *opaque);
};
#endif // P_LIBVLC12

static VideoGraphicsObject1point1 *createVideoGraphicsObject(QObject *parent = 0)
{
#if defined(P_LIBVLC12)
    return new VideoGraphicsObject(parent);
#else
    return new VideoGraphicsObject1point1(parent);
#endif // P_LIBVLC12
}

} // namespace VLC
} // namespace Phonon

#endif // PHONON_VLC_VIDEOGRAPHICSOBJECT_H
