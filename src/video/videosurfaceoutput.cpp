#include "videosurfaceoutput.h"

#include "mediaobject.h"
#include "utils/debug.h"
#include "media.h"

#include <stdio.h>

//#define MULTIFRAME

namespace Phonon {
namespace VLC {

VideoSurfaceOutput::VideoSurfaceOutput(QObject *parent) :
    QObject(parent)
  , m_presentFrame(new VideoFrame)
{
    DEBUG_BLOCK;
}

VideoSurfaceOutput::~VideoSurfaceOutput()
{
    DEBUG_BLOCK;
}

void VideoSurfaceOutput::handleConnectPlayer(Player *player)
{
    DEBUG_BLOCK;
    setCallbacks(m_vlcPlayer);
}

void VideoSurfaceOutput::handleDisconnectPlayer(Player *player)
{
    DEBUG_BLOCK;
    m_player->stop();
    unsetCallbacks(m_vlcPlayer);
}

void VideoSurfaceOutput::handleAddToMedia(Media *media)
{
    DEBUG_BLOCK;
    media->addOption(":video");
}

bool VideoSurfaceOutput::tryLock()
{
    return m_mutex.tryLock(500);
}

void VideoSurfaceOutput::lock()
{
    m_mutex.lock();
}

void VideoSurfaceOutput::unlock()
{
    m_mutex.unlock();
}

const VideoFrame *VideoSurfaceOutput::frame() const
{
#ifdef MULTIFRAME
    return m_presentFrame;
#else
    return &m_frame;
#endif
}

void *VideoSurfaceOutput::lockCallback(void **planes)
{
    DEBUG_BLOCK;
    debug() << this;

#ifdef MULTIFRAME
    VideoFrame *frame = createFrame();
#else
    lock();
    VideoFrame *frame = &m_frame;
#endif

    debug() << frame->planeCount;
    for (unsigned int i = 0; i < frame->planeCount; ++i) {
        planes[i] = reinterpret_cast<void *>(frame->plane[i].data());
    }
    debug() << planes[0];

    return frame; // There is only one buffer, so no need to identify it.
}

void VideoSurfaceOutput::unlockCallback(void *picture, void * const *planes)
{
    Q_UNUSED(picture);
    Q_UNUSED(planes);
    DEBUG_BLOCK;
    debug() << this;
    VideoFrame *frame = (VideoFrame *)picture;

//     m_frame.format = VideoFrame::Format_RGB32;
//    frame->format = VideoFrame::Format_I420;
    frame->format = VideoFrame::Format_YV12;

#ifdef MULTIFRAME
    lock();
    Q_ASSERT(m_frameSet.contains(frame));
    qDebug() << m_frameSet.size();
    m_frameSet.remove(frame);
    if (m_presentFrame)
        delete m_presentFrame;
    m_presentFrame = frame;
#endif
    unlock();

    // To avoid thread polution do not call frameReady directly, but via the
    // event loop.
    QMetaObject::invokeMethod(this, "frameReady", Qt::QueuedConnection);
}

void VideoSurfaceOutput::displayCallback(void *picture)
{
    Q_UNUSED(picture); // There is only one buffer.
#warning there is only one buffer :(
}

unsigned VideoSurfaceOutput::formatCallback(char *chroma, unsigned *width, unsigned *height, unsigned *pitches, unsigned *lines)
{
#warning forcing YUV
    DEBUG_BLOCK;
        debug() << "Format:"
                << "chroma:" << chroma
                << "width:" << *width
                << "height:" << *height
                << "pitches:" << *pitches
                << "lines:" << *lines;

        const vlc_chroma_description_t *chromaDesc = 0;

//         qstrcpy(chroma, "RV32");
//         chromaDesc = vlc_fourcc_GetChromaDescription(VLC_CODEC_RGB32);

        qstrcpy(chroma, "YV12");
        chromaDesc = vlc_fourcc_GetChromaDescription(VLC_CODEC_YV12);

//       qstrcpy(chroma, "I420");
//       chromaDesc = vlc_fourcc_GetChromaDescription(VLC_CODEC_I420);

        Q_ASSERT(chromaDesc);

        debug() << m_frame.format;
        m_frame.width = *width;
        m_frame.height = *height;
        m_frame.planeCount = chromaDesc->plane_count;

        debug() << chroma;
        const unsigned int bufferSize = setPitchAndLines(chromaDesc,
                                                         *width, *height,
                                                         pitches, lines,
                                                         (unsigned *) &m_frame.visiblePitch,
                                                         (unsigned *) &m_frame.visibleLines);
        for (unsigned int i = 0; i < m_frame.planeCount; ++i) {
            m_frame.pitch[i] = pitches[i];
            m_frame.lines[i] = lines[i];
            m_frame.plane[i].resize(pitches[i] * lines[i]);
        }
        return bufferSize;
}

void VideoSurfaceOutput::formatCleanUpCallback()
{
    DEBUG_BLOCK;
#warning reset not present in the api
}

VideoFrame *VideoSurfaceOutput::createFrame()
{
    VideoFrame *frame = new VideoFrame;
    frame->format = m_frame.format;
    frame->width = m_frame.width;
    frame->height = m_frame.height;
    frame->planeCount = m_frame.planeCount;
    for (unsigned int i = 0; i < frame->planeCount; ++i) {
        frame->pitch[i] = m_frame.pitch[i];
        frame->lines[i] = m_frame.lines[i];
        frame->plane[i].resize(m_frame.plane[i].size());
        frame->plane[i].fill('a');
        qDebug() << "plane" << i << "pitch/lines" << frame->pitch[i] << "/" << frame->lines[i] << "size" << frame->plane[i].size();
     }
    lock();
    m_frameSet.insert(frame);
    unlock();
    return frame;
}

} // namespace VLC
} // namespace Phonon
