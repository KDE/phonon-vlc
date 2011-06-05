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
    m_connectedMO = NULL;
}

AvCapture::~AvCapture()
{
    stop();
}

State AvCapture::state() const
{
    if (m_connectedMO)
        return m_connectedMO->state();

    return ErrorState;
}

void AvCapture::start()
{
    m_audioMedia.play();
    m_videoMedia.play();
}

void AvCapture::pause()
{
    m_audioMedia.pause();
    m_videoMedia.pause();
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
    m_audioMedia.setSource(device);
    m_audioCaptureDevice = device;
    setupStateChangedSignal();
}

void AvCapture::setVideoCaptureDevice(const Phonon::VideoCaptureDevice &device)
{
    m_videoMedia.setSource(device);
    m_videoCaptureDevice = device;
    setupStateChangedSignal();
}

void AvCapture::setupStateChangedSignal()
{
    // Disconnect the old media object state change signal
    if (m_connectedMO)
        disconnect(m_connectedMO, SIGNAL(stateChanged(Phonon::State,Phonon::State)), this, SIGNAL(stateChanged(Phonon::State,Phonon::State)));

    // Determine the media object from which the state change signal will be connected
    m_connectedMO = NULL;
    if (m_videoCaptureDevice.isValid()) {
        m_connectedMO = &m_videoMedia;
    } else {
        if (m_audioCaptureDevice.isValid()) {
            m_connectedMO = &m_audioMedia;
        }
    }

    // Connect the state change signal
    if (m_connectedMO)
        connect(m_connectedMO, SIGNAL(stateChanged(Phonon::State,Phonon::State)), this, SIGNAL(stateChanged(Phonon::State,Phonon::State)));
}

} // Experimental namespace
} // VLC namespace
} // Phonon namespace

#endif // PHONON_VLC_NO_EXPERIMENTAL
