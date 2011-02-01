/*****************************************************************************
* libVLC backend for the Phonon library                                     *
*                                                                           *
* Copyright (C) 2007-2008 Tanguy Krotoff <tkrotoff@gmail.com>               *
* Copyright (C) 2008 Lukas Durfina <lukas.durfina@gmail.com>                *
* Copyright (C) 2009 Fathi Boudra <fabo@kde.org>                            *
* Copyright (C) 2009-2010 vlc-phonon AUTHORS                                *
*                                                                           *
* This program is free software; you can redistribute it and/or             *
* modify it under the terms of the GNU Lesser General Public                *
* License as published by the Free Software Foundation; either              *
* version 2.1 of the License, or (at your option) any later version.        *
*                                                                           *
* This program is distributed in the hope that it will be useful,           *
* but WITHOUT ANY WARRANTY; without even the implied warranty of            *
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU         *
* Lesser General Public License for more details.                           *
*                                                                           *
* You should have received a copy of the GNU Lesser General Public          *
* License along with this package; if not, write to the Free Software       *
* Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301 USA *
*****************************************************************************/

#ifndef PHONON_VLC_NO_EXPERIMENTAL
#include "avcapture.h"

namespace Phonon
{
namespace VLC
{
namespace Experimental
{

AvCapture::AvCapture(QObject* parent)
    : QObject(parent),
    m_audioMedia(parent),
    m_videoMedia(parent)
{

}

AvCapture::~AvCapture()
{
    stop();
}

void AvCapture::start()
{
    m_audioMedia.play();
    m_videoMedia.play();
}

void AvCapture::stop()
{
    m_audioMedia.stop();
    m_videoMedia.stop();
}

MediaObject* AvCapture::audioMediaObject()
{
    return &m_audioMedia;
}

MediaObject* AvCapture::videoMediaObject()
{
    return &m_videoMedia;
}

AudioCaptureDevice AvCapture::audioCaptureDevice() const
{
    return m_audioCaptureDevice;
}

VideoCaptureDevice AvCapture::videoCaptureDevice() const
{
    return m_videoCaptureDevice;
}

void AvCapture::setAudioCaptureDevice(const Phonon::AudioCaptureDevice &device)
{
    MediaSource source(device);
    m_audioMedia.setSource(source);
    m_audioCaptureDevice = device;
}

void AvCapture::setVideoCaptureDevice(const Phonon::VideoCaptureDevice &device)
{
    MediaSource source(device);
    m_videoMedia.setSource(source);
    m_videoCaptureDevice = device;
}

} // Experimental namespace
} // VLC namespace
} // Phonon namespace

#endif // PHONON_VLC_NO_EXPERIMENTAL
