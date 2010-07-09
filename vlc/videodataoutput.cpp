/*  This file is part of the KDE project
    Copyright (C) 2006 Matthias Kretz <kretz@kde.org>
    Copyright (C) 2009 Martin Sandsmark <sandsmark@samfundet.no>

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
#include "mediaobject.h"
#include <QtCore/QVector>
#include <QtCore/QMap>
#include <QtCore/QThread>
#include <phonon/audiooutput.h>
#include <phonon/experimental/abstractvideodataoutput.h>
#include <phonon/experimental/videoframe.h>

namespace Phonon
{
namespace VLC
{
VideoDataOutput::VideoDataOutput(Backend *backend, QObject *parent)
    : SinkNode(parent)
{
    Q_UNUSED(backend)
}

VideoDataOutput::~VideoDataOutput()
{
}

void VideoDataOutput::addToMedia( libvlc_media_t * media )
{
    libvlc_media_add_option_flag( media, ":vout=vmem", libvlc_media_option_trusted );

    char param[64];
    // Add lock callback
    void * lock_call = reinterpret_cast<void*>( &VideoDataOutput::lock );
    sprintf( param, ":vmem-lock=%"PRId64, (long)(intptr_t)lock_call );
    libvlc_media_add_option_flag( media, param, libvlc_media_option_trusted );

    // Add unlock callback
    void * unlock_call = reinterpret_cast<void*>( &VideoDataOutput::unlock );
    sprintf( param, ":vmem-unlock=%"PRId64, (long)(intptr_t)unlock_call );
    libvlc_media_add_option_flag( media, param, libvlc_media_option_trusted );

    // Add pointer to ourselves...
    sprintf( param, ":vmem-data=%"PRId64, (long)(intptr_t)this );
    libvlc_media_add_option_flag( media, param, libvlc_media_option_trusted );
}

void VideoDataOutput::lock(VideoDataOutput *cw, void **bufRet)
{
    cw->m_locker.lock();

    const int width = libvlc_video_get_width(cw->p_vlc_player);
    const int height = libvlc_video_get_height(cw->p_vlc_player);

    cw->m_buffer = new char[width * height];
    *bufRet = cw->m_buffer;
}

void VideoDataOutput::unlock(VideoDataOutput *cw)
{
    // Might be a good idea to cache these, but this should be insignificant overhead compared to the image conversion
    const int width = libvlc_video_get_width(cw->p_vlc_player);
    const int height = libvlc_video_get_height(cw->p_vlc_player);
    const char *aspect = libvlc_video_get_aspect_ratio(cw->p_vlc_player);

    const Experimental::VideoFrame2 frame = {
        width,
        height,
        *aspect,
        Experimental::VideoFrame2::Format_RGB32,
        QByteArray::fromRawData(cw->m_buffer, width*height), // data0
        0, //data1
        0  //data2
    };
    delete aspect;
    cw->m_locker.unlock();

    //TODO: check which thread?
    emit cw->frameReadySignal(frame);
}

Experimental::AbstractVideoDataOutput *VideoDataOutput::frontendObject() const
{
    return m_frontend;
}

void VideoDataOutput::setFrontendObject(Experimental::AbstractVideoDataOutput *frontend)
{
    m_frontend = frontend;
}

}} //namespace Phonon::VLC

#include "moc_videodataoutput.cpp"
// vim: sw=4 ts=4

