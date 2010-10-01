/*  libVLC backend for the Phonon library
    Copyright (C) 2006 Matthias Kretz <kretz@kde.org>
    Copyright (C) 2009 Martin Sandsmark <sandsmark@samfundet.no>
    Copyright (C) 2010 Harald Sitter <apachelogger@ubuntu.com>

    This library is free software; you can redistribute it and/or
    modify it under the terms of the GNU Lesser General Public
    License as published by the Free Software Foundation; either
    version 2.1 of the License, or (at your option) version 3, or any
    later version accepted by the membership of KDE e.V. (or its
    successor approved by the membership of KDE e.V.), Nokia Corporation
    (or its successors, if any) and the KDE Free Qt Foundation, which shall
    act as a proxy defined in Section 6 of version 3 of the license.

    This library is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
    Lesser General Public License for more details.

    You should have received a copy of the GNU Lesser General Public
    License along with this library.  If not, see <http://www.gnu.org/licenses/>.
*/

#include "videodataoutput.h"

#include <phonon/medianode.h>
#include <phonon/audiooutput.h>
#include <phonon/experimental/abstractvideodataoutput.h>
#include <phonon/experimental/videoframe2.h>

#include "mediaobject.h"

namespace Phonon
{
namespace VLC
{
VideoDataOutput::VideoDataOutput(QObject *parent) :
        SinkNode(parent),
        m_frontend(0),
        m_img(0)
{
}

VideoDataOutput::~VideoDataOutput()
{
}

void VideoDataOutput::connectToMediaObject(PrivateMediaObject *mediaObject)
{
    SinkNode::connectToMediaObject(mediaObject);

// FIXME: second call to libvlc_video_set_format does not work, and thus break
//        playback due to size mismatch between QImage and VLC.
//    connect(mediaObject, SIGNAL(videoWidgetSizeChanged(int, int)),
//            SLOT(videoSizeChanged(int, int)));
}

void VideoDataOutput::addToMedia(libvlc_media_t *media)
{
    const int width = 300;
    const int height = width;
    videoSizeChanged(width, height);
    libvlc_video_set_callbacks(p_vlc_player, lock, unlock, 0, this);
}

void VideoDataOutput::videoSizeChanged(int width, int height)
{
    QSize size (width, height);
    if (m_img) {
        if (size == m_img->size()) {
            return; // False alarm.
        }
        delete m_img;
    }
    qDebug() << "changing video size to:" << size;
    m_img = new QImage(size, QImage::Format_RGB32);
    libvlc_video_set_format(p_vlc_player, "RV32", width, height, width * 4);
}

void *VideoDataOutput::lock(void *data, void **bufRet)
{
    Q_ASSERT(data);
    VideoDataOutput *cw = (VideoDataOutput *)data;
    cw->m_mutex.lock();

//    const int width = libvlc_video_get_width(cw->p_vlc_player);
//    const int height = libvlc_video_get_height(cw->p_vlc_player);

//    cw->m_buffer = new char[width * height];
//    *bufRet = cw->m_buffer;

    *bufRet = cw->m_img->bits();

    return NULL; // Picture identifier, not needed here. - NULL because we are called by Mr. C.
}

void VideoDataOutput::unlock(void *data, void *id, void *const *pixels)
{
    Q_ASSERT(data);
    Q_UNUSED(id);
    Q_UNUSED(pixels);

    VideoDataOutput *cw = (VideoDataOutput *)data;

    // Might be a good idea to cache these, but this should be insignificant overhead compared to the image conversion
//    const char *aspect = libvlc_video_get_aspect_ratio(cw->p_vlc_player);

    const Experimental::VideoFrame2 frame = {
        cw->m_img->width(),
        cw->m_img->height(),
        0,
        // FIXME: VideoFrame2 does not (yet) support RGB32, so we use 888 but really
        //        hope the frontend implementation will convert to RGB32 QImage.
        Experimental::VideoFrame2::Format_RGB888,
        QByteArray::fromRawData((const char *)cw->m_img->bits(),
                                 cw->m_img->byteCount()), // data0
        0, //data1
        0  //data2
    };

    //TODO: check which thread?
    if (cw->m_frontend) {
        cw->m_frontend->frameReady(frame);
    }
    cw->m_mutex.unlock();
}

Experimental::AbstractVideoDataOutput *VideoDataOutput::frontendObject() const
{
    return m_frontend;
}

void VideoDataOutput::setFrontendObject(Experimental::AbstractVideoDataOutput *frontend)
{
    m_frontend = frontend;
}

}
} //namespace Phonon::VLC

#include "moc_videodataoutput.cpp"
// vim: sw=4 ts=4

