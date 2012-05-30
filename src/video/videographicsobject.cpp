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

#include "videographicsobject.h"

#include <QtCore/qmath.h>

#include <vlc/libvlc.h>
#include <vlc/libvlc_media.h>
#include <vlc/libvlc_media_player.h>

#include <vlc/plugins/vlc_fourcc.h>

#include "utils/debug.h"
#include "mediaobject.h"

namespace Phonon {
namespace VLC {

#define P_THAT1point1 VideoGraphicsObject1point1 *that = static_cast<VideoGraphicsObject1point1 *>(opaque)
#define P_THAT VideoGraphicsObject *that = static_cast<VideoGraphicsObject *>(opaque)
#define P_THAT2 VideoGraphicsObject *that = static_cast<VideoGraphicsObject *>(*opaque)

VideoGraphicsObject1point1::VideoGraphicsObject1point1(QObject *parent) :
    QObject(parent),
    m_chosenFormat(VideoFrame::Format_Invalid)
{
    DEBUG_BLOCK;
    m_frame.format = VideoFrame::Format_Invalid;
}

VideoGraphicsObject1point1::~VideoGraphicsObject1point1()
{
    DEBUG_BLOCK;
    if (m_mediaObject)
        m_mediaObject->stop();
}

void VideoGraphicsObject1point1::connectToMediaObject(MediaObject *mediaObject)
{
    DEBUG_BLOCK;
    SinkNode::connectToMediaObject(mediaObject);

    m_frame.width = 640;
    m_frame.height = 360;

#warning todo
    libvlc_video_set_format(*m_player, "RV32", m_frame.width, m_frame.height, m_frame.width * 4);
    libvlc_video_set_callbacks(*m_player, lock_cb, unlock_cb, display_cb, this);

    m_frame.format = VideoFrame::Format_RGB32;
    m_frame.planeCount = 1;
    // RGB32 is packed with a pixel size of 4.
    m_frame.plane[0].resize(m_frame.width * m_frame.height * 4);
}

void VideoGraphicsObject1point1::lock()
{
    m_mutex.lock();
}

bool VideoGraphicsObject1point1::tryLock()
{
    return m_mutex.tryLock();
}

void VideoGraphicsObject1point1::unlock()
{
    m_mutex.unlock();
}

void *VideoGraphicsObject1point1::lock_cb(void *opaque, void **planes)
{
    P_THAT1point1;
    that->lock();

    for (int i = 0; i < that->m_frame.planeCount; ++i) {
        planes[i] = reinterpret_cast<void *>(that->m_frame.plane[i].data());
    }

    return 0; // There is only one buffer, so no need to identify it.
}

void VideoGraphicsObject1point1::unlock_cb(void *opaque, void *picture,
                                    void *const*planes)
{
    P_THAT1point1;
    that->unlock();
    // To avoid thread polution do not call frameReady directly, but via the
    // event loop.
    QMetaObject::invokeMethod(that, "frameReady", Qt::QueuedConnection);
}

void VideoGraphicsObject1point1::display_cb(void *opaque, void *picture)
{
    Q_UNUSED(opaque);
    Q_UNUSED(picture); // There is only one buffer.
}

QList<VideoFrame::Format> VideoGraphicsObject1point1::offering(QList<VideoFrame::Format> offers)
{
    // FIXME: impl
    return offers;
}

void VideoGraphicsObject1point1::choose(VideoFrame::Format format)
{
    // FIXME: review
    m_chosenFormat = format;
}

#if (LIBVLC_VERSION_INT >= LIBVLC_VERSION(2, 0, 0, 0))
VideoGraphicsObject::VideoGraphicsObject(QObject *parent) :
    VideoGraphicsObject1point1(parent)
{}

VideoGraphicsObject::~VideoGraphicsObject()
{
}

void VideoGraphicsObject::connectToMediaObject(MediaObject *mediaObject)
{
    DEBUG_BLOCK;
    SinkNode::connectToMediaObject(mediaObject);
    libvlc_video_set_callbacks(*m_player, lock_cb, unlock_cb, display_cb, this);
    libvlc_video_set_format_callbacks(*m_player, format_cb, cleanup_cb);
}

void VideoGraphicsObject::disconnectFromMediaObject(MediaObject *mediaObject)
{
    SinkNode::disconnectFromMediaObject(mediaObject);
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

    if (that->m_chosenFormat == VideoFrame::Format_Invalid)
        emit that->needFormat();

    Q_ASSERT(that->m_chosenFormat != VideoFrame::Format_Invalid);

    const vlc_chroma_description_t *chromaDesc = 0;
    switch(that->m_chosenFormat) {
    case VideoFrame::Format_RGB32:
        that->m_frame.format = VideoFrame::Format_RGB32;
        qstrcpy(chroma, "RV32");
        chromaDesc = vlc_fourcc_GetChromaDescription(VLC_CODEC_RGB32);
        break;
    case VideoFrame::Format_YV12:
        that->m_frame.format = VideoFrame::Format_YV12;
        qstrcpy(chroma, "YV12");
        chromaDesc = vlc_fourcc_GetChromaDescription(VLC_CODEC_YV12);
        break;
    case VideoFrame::Format_I420:
        that->m_frame.format = VideoFrame::Format_I420;
        qstrcpy(chroma, "I420");
        chromaDesc = vlc_fourcc_GetChromaDescription(VLC_CODEC_I420);
        break;
    }

    Q_ASSERT(chromaDesc);

    that->m_frame.width = *width;
    that->m_frame.height = *height;
    that->m_frame.planeCount = chromaDesc->plane_count;

    unsigned int bufferSize = 0;
    for (unsigned int i = 0; i < that->m_frame.planeCount; ++i) {
        pitches[i] = *width * chromaDesc->p[i].w.num / chromaDesc->p[i].w.den * chromaDesc->pixel_size;
        lines[i] = *height * chromaDesc->p[i].h.num / chromaDesc->p[i].h.den;
        that->m_frame.plane[i].resize(pitches[i] * lines[i]);

        // Add to overall buffer size.
        bufferSize += that->m_frame.plane[i].size();
    }

    debug() << chroma;
    return bufferSize;
}

void VideoGraphicsObject::cleanup_cb(void *opaque)
{
    P_THAT;
    // To avoid thread polution do not call reset directly but via the event loop.
    QMetaObject::invokeMethod(that, "reset", Qt::QueuedConnection);
}
#endif // >= VLC 2

} // namespace VLC
} // namespace Phonon
