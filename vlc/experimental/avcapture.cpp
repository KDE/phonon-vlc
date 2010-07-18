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

#include "avcapture.h"

namespace Phonon
{
namespace Experimental
{
namespace VLC
{

AVCapture::AVCapture(QObject* parent)
    : QObject(parent)
{

}

AVCapture::~AVCapture()
{
    stop();
}

void AVCapture::start()
{
    m_audioMedia.play();
    m_videoMedia.play();
}

void AVCapture::stop()
{
    m_audioMedia.stop();
    m_videoMedia.stop();
}

AudioCaptureDevice AVCapture::audioCaptureDevice() const
{
    return m_audioCaptureDevice;
}

VideoCaptureDevice AVCapture::videoCaptureDevice() const
{
    return m_videoCaptureDevice;
}

void AVCapture::setAudioCaptureDevice(const Phonon::AudioCaptureDevice &device)
{
    MediaSource source(V4LAudio, device);
    m_audioMedia.setCurrentSource(source);
    m_audioCaptureDevice = device;
}

void AVCapture::setVideoCaptureDevice(const Phonon::VideoCaptureDevice &device)
{
    MediaSource source(V4LVideo, device);
    m_videoMedia.setCurrentSource(source);
    m_videoCaptureDevice = device;
}

} // Phonon::Experimental::VLC namespace
} // Phonon::Experimental namespace
} // Phonon namespace
