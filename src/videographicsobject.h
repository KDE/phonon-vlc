#ifndef PHONON_VLC_VIDEOGRAPHICSOBJECT_H
#define PHONON_VLC_VIDEOGRAPHICSOBJECT_H

#include <QtCore/QObject>
#include <QtCore/QMutex>

#include <phonon/videoframe.h>
#include <phonon/videographicsobject.h>

#include "sinknode.h"

struct libvlc_media_t;

namespace Phonon {
namespace VLC {

class VideoGraphicsObject : public QObject,
                            public VideoGraphicsObjectInterface,
                            public SinkNode
{
    Q_OBJECT
    Q_INTERFACES(Phonon::VideoGraphicsObjectInterface)
public:
    VideoGraphicsObject(QObject *parent = 0);

//    void connectToMediaObject(MediaObject *mediaObject);
//    void disconnectFromMediaObject(MediaObject *mediaObject);
    void addToMedia(libvlc_media_t *media);

    Phonon::VideoGraphicsObject *videoGraphicsObject() { return m_videoGraphicsObject; }
    void setVideoGraphicsObject(Phonon::VideoGraphicsObject *object) { m_videoGraphicsObject = object; }

    void lock();
    bool tryLock();
    void unlock();

    const VideoFrame *frame() const { return &m_frame; }

    static void *lock_cb(void *opaque, void **planes);
    static void unlock_cb(void *opaque, void *picture, void *const *planes);
    static void display_cb(void *opaque, void *picture);

    static unsigned int format_cb(void **opaque, char *chroma,
                                  unsigned int *width, unsigned int *height,
                                  unsigned int *pitches,
                                  unsigned int *lines);
    static void cleanup_cb(void *opaque);

signals:
    void frameReady();
    void reset();

private:
    QMutex m_mutex;

    Phonon::VideoGraphicsObject *m_videoGraphicsObject;
    Phonon::VideoFrame m_frame;
};

} // namespace VLC
} // namespace Phonon

#endif // PHONON_VLC_VIDEOGRAPHICSOBJECT_H
