#include "videographicsobject.h"

#include <QtCore/qmath.h>

#include <vlc/libvlc.h>
#include <vlc/libvlc_media.h>
#include <vlc/libvlc_media_player.h>

#include "debug.h"

namespace Phonon {
namespace VLC {

#define P_THAT VideoGraphicsObject *that = static_cast<VideoGraphicsObject *>(opaque)
#define P_THAT2 VideoGraphicsObject *that = static_cast<VideoGraphicsObject *>(*opaque)

VideoGraphicsObject::VideoGraphicsObject(QObject *parent) :
    QObject(parent),
    m_videoGraphicsObject(0)
{
    m_frame.format = VideoFrame::Format_Invalid;
}

void VideoGraphicsObject::addToMedia(libvlc_media_t *media)
{
    DEBUG_BLOCK;
    libvlc_video_set_callbacks(m_player, lock_cb, unlock_cb, display_cb, this);
    libvlc_video_set_format_callbacks(m_player, format_cb, cleanup_cb);
}

void VideoGraphicsObject::lock()
{
    m_mutex.lock();
}

bool VideoGraphicsObject::tryLock()
{
    return m_mutex.tryLock();
}

void VideoGraphicsObject::unlock()
{
    m_mutex.unlock();
}

void *VideoGraphicsObject::lock_cb(void *opaque, void **planes)
{
    DEBUG_BLOCK;
    P_THAT;
    that->lock();

    for (int i = 0; i < that->m_frame.planeCount; ++i) {
        planes[i] = static_cast<void *>(that->m_frame.plane[i].data());
    }

    return 0; // There is only one buffer, so no need to identify it.
}

void VideoGraphicsObject::unlock_cb(void *opaque, void *picture,
                                    void *const*planes)
{
    DEBUG_BLOCK;
    P_THAT;
    that->unlock();
//    QMetaObject::invokeMethod(that, "frameReady");
    emit that->frameReady();
}

void VideoGraphicsObject::display_cb(void *opaque, void *picture)
{
    DEBUG_BLOCK;
    Q_UNUSED(picture); // There is only one buffer.
    P_THAT;
//    emit that->frameReady();
}

unsigned int VideoGraphicsObject::format_cb(void **opaque, char *chroma,
                                            unsigned int *width,
                                            unsigned int *height,
                                            unsigned int *pitches,
                                            unsigned int *lines)
{
    DEBUG_BLOCK;

    debug() << "Format:"
            << "chroma:" << chroma
            << "width:" << *width
            << "height:" << *height
            << "pitches:" << *pitches
            << "lines:" << *lines;

    P_THAT2;

    that->m_frame.width = *width;
    that->m_frame.height = *height;

    const QByteArray paintEnv = qgetenv("PHONON_COLOR");
    if (paintEnv == "rgb")
        qstrcpy(chroma, "RV32");
    else if (paintEnv == "yuv")
        qstrcpy(chroma, "YV12");

    if (qstrcmp(chroma, "RV32") == 0) {
        that->m_frame.format = VideoFrame::Format_RGB32;
        that->m_frame.planeCount = 1;
        // RGB32 is packed with a pixel size of 4.
        that->m_frame.plane[0].resize(that->m_frame.width * that->m_frame.height * 4);
    } else if (qstrcmp(chroma, "YV12") != 0) {
        // Ensure we use a valid format, so choose YV12 as last resort fallback.
        qstrcpy(chroma, "YV12");
    } else if (qstrcmp(chroma, "YV12") == 0) {
        that->m_frame.format = VideoFrame::Format_YV12;
        that->m_frame.planeCount = 3; // Y, U, V
        unsigned int halfWidth = that->m_frame.width/2;
        that->m_frame.plane[0].resize(that->m_frame.width);
        that->m_frame.plane[1].resize(halfWidth);
        that->m_frame.plane[2].resize(halfWidth);
    }

    unsigned int bufferSize = 0;
    for (int i = 0; i < that->m_frame.planeCount; ++i) {
        bufferSize += that->m_frame.plane[i].size();
    }

    debug() << chroma;

    return bufferSize;
}

void VideoGraphicsObject::cleanup_cb(void *opaque)
{
    DEBUG_BLOCK;
    P_THAT;
    QMetaObject::invokeMethod(that, "reset");
}

} // namespace VLC
} // namespace Phonon
