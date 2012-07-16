/*
    Copyright (C) 2011-2012 Harald Sitter <sitter@kde.org>

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

#include <vlc/plugins/vlc_fourcc.h>

#include "utils/debug.h"
#include "mediaobject.h"

namespace Phonon {
namespace VLC {

VideoGraphicsObject::VideoGraphicsObject(QObject *parent) :
    QObject(parent),
    m_chosenFormat(VideoFrame::Format_Invalid)
{
    DEBUG_BLOCK;
    m_frame.format = VideoFrame::Format_Invalid;
}

VideoGraphicsObject::~VideoGraphicsObject()
{
    DEBUG_BLOCK;
    m_mutex.lock();
}

void VideoGraphicsObject::connectToMediaObject(MediaObject *mediaObject)
{
    DEBUG_BLOCK;
    SinkNode::connectToMediaObject(mediaObject);
    setCallbacks(m_player);
}

void VideoGraphicsObject::disconnectFromMediaObject(MediaObject *mediaObject)
{
    unsetCallbacks(m_player);
    SinkNode::disconnectFromMediaObject(mediaObject);
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

QList<VideoFrame::Format> VideoGraphicsObject::offering(QList<VideoFrame::Format> offers)
{
    // FIXME: impl
    return offers;
}

void VideoGraphicsObject::choose(VideoFrame::Format format)
{
    // FIXME: review
    m_chosenFormat = format;
}

void *VideoGraphicsObject::lockCallback(void **planes)
{
    lock();

    for (unsigned int i = 0; i < m_frame.planeCount; ++i) {
        planes[i] = reinterpret_cast<void *>(m_frame.plane[i].data());
    }

    return 0; // There is only one buffer, so no need to identify it.
}

void VideoGraphicsObject::unlockCallback(void *picture, void *const*planes)
{
    Q_UNUSED(picture);
    Q_UNUSED(planes);
    unlock();
    // To avoid thread polution do not call frameReady directly, but via the
    // event loop.
    QMetaObject::invokeMethod(this, "frameReady", Qt::QueuedConnection);
}

void VideoGraphicsObject::displayCallback(void *picture)
{
    Q_UNUSED(picture); // There is only one buffer.
}

unsigned int VideoGraphicsObject::formatCallback(char *chroma,
                                                 unsigned *width, unsigned *height,
                                                 unsigned *pitches, unsigned *lines)
{
    DEBUG_BLOCK;
    debug() << "Format:"
            << "chroma:" << chroma
            << "width:" << *width
            << "height:" << *height
            << "pitches:" << *pitches
            << "lines:" << *lines;

    if (m_chosenFormat == VideoFrame::Format_Invalid)
        emit needFormat();

    Q_ASSERT(m_chosenFormat != VideoFrame::Format_Invalid);

    const vlc_chroma_description_t *chromaDesc = 0;
    switch(m_chosenFormat) {
    case VideoFrame::Format_RGB32:
        m_frame.format = VideoFrame::Format_RGB32;
        qstrcpy(chroma, "RV32");
        chromaDesc = vlc_fourcc_GetChromaDescription(VLC_CODEC_RGB32);
        break;
    case VideoFrame::Format_YV12:
        m_frame.format = VideoFrame::Format_YV12;
        qstrcpy(chroma, "YV12");
        chromaDesc = vlc_fourcc_GetChromaDescription(VLC_CODEC_YV12);
        break;
    case VideoFrame::Format_I420:
        m_frame.format = VideoFrame::Format_I420;
        qstrcpy(chroma, "I420");
        chromaDesc = vlc_fourcc_GetChromaDescription(VLC_CODEC_I420);
        break;
    }

    Q_ASSERT(chromaDesc);

    m_frame.width = *width;
    m_frame.height = *height;
    m_frame.planeCount = chromaDesc->plane_count;

    debug() << chroma;
    const unsigned int bufferSize = setPitchAndLines(chromaDesc,
                                                     *width, *height,
                                                     pitches, lines,
                                                     (unsigned *) &m_frame.visiblePitch, (unsigned *)&m_frame.visibleLines);
    for (unsigned int i = 0; i < m_frame.planeCount; ++i) {
        m_frame.pitch[i] = pitches[i];
        m_frame.lines[i] = lines[i];
        m_frame.plane[i].resize(pitches[i] * lines[i]);
    }
    return bufferSize;
}

void VideoGraphicsObject::formatCleanUpCallback()
{
    DEBUG_BLOCK;
    // To avoid thread polution do not call reset directly but via the event loop.
    QMetaObject::invokeMethod(this, "reset", Qt::QueuedConnection);
}

} // namespace VLC
} // namespace Phonon
