/*****************************************************************************
 * VLC backend for the Phonon library                                        *
 * Copyright (C) 2007-2008 Tanguy Krotoff <tkrotoff@gmail.com>               *
 * Copyright (C) 2008 Lukas Durfina <lukas.durfina@gmail.com>                *
 * Copyright (C) 2009 Fathi Boudra <fabo@kde.org>                            *
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

#include "audiooutput.h"

#ifdef PHONON_PULSESUPPORT
#include <phonon/pulsesupport.h>
#endif

#include <vlc/vlc.h>

#include "backend.h"
#include "debug.h"
#include "devicemanager.h"
#include "mediaobject.h"

#ifndef PHONON_VLC_NO_EXPERIMENTAL
#include <experimental/avcapture.h>
#endif // PHONON_VLC_NO_EXPERIMENTAL

namespace Phonon
{
namespace VLC
{

AudioOutput::AudioOutput(QObject *parent)
    : QObject(parent),
      m_volume(1.0),
      m_deviceIndex(0)
{
}

AudioOutput::~AudioOutput()
{
}

void AudioOutput::connectToMediaObject(MediaObject *mediaObject)
{
    SinkNode::connectToMediaObject(mediaObject);
    setAudioOutputDeviceImplementation();
    connect(m_mediaObject, SIGNAL(playbackCommenced()), this, SLOT(updateVolume()));
}

void AudioOutput::disconnectFromMediaObject(MediaObject *mediaObject)
{
    SinkNode::disconnectFromMediaObject(mediaObject);
    if (m_mediaObject) {
        disconnect(m_mediaObject, SIGNAL(playbackCommenced()), this, SLOT(updateVolume()));
    }
}

#ifndef PHONON_VLC_NO_EXPERIMENTAL
void AudioOutput::connectToAvCapture(Experimental::AvCapture *avCapture)
{
    connectToMediaObject(avCapture->audioMediaObject());
}

void AudioOutput::disconnectFromAvCapture(Experimental::AvCapture *avCapture)
{
    disconnectFromMediaObject(avCapture->audioMediaObject());
}
#endif // PHONON_VLC_NO_EXPERIMENTAL

qreal AudioOutput::volume() const
{
    return m_volume;
}

void AudioOutput::setVolume(qreal volume)
{
    if (m_player) {
        const int previous_volume = libvlc_audio_get_volume(m_player);
        m_volume = volume;
        libvlc_audio_set_volume(m_player, (int)(m_volume * 50));
        debug() << "Volume changed from" << previous_volume << "to" << (int)(m_volume * 100);
        emit volumeChanged(m_volume);
    }
}

int AudioOutput::outputDevice() const
{
    return m_deviceIndex;
}

bool AudioOutput::setOutputDevice(int deviceIndex)
{
    const QList<DeviceInfo> deviceList = Backend::self->deviceManager()->audioOutputDevices();
    if (deviceIndex < 0 || deviceIndex >= deviceList.size()) {
        return false;
    }
    if (m_deviceIndex != deviceIndex) {
        m_deviceIndex = deviceIndex;
        if (m_player) {
            setAudioOutputDeviceImplementation();
        }
    }
    return true;
}

void AudioOutput::setAudioOutputDeviceImplementation()
{
    Q_ASSERT(m_player);
#ifdef PHONON_PULSESUPPORT
    if (PulseSupport::getInstance()->isActive()) {
        libvlc_audio_output_set(m_player, "pulse");
        debug() << "Setting aout to pulse";
        return;
    }
#endif
    const QList<DeviceInfo> deviceList = Backend::self->deviceManager()->audioOutputDevices();

    Q_ASSERT(m_deviceIndex >= 0 && m_deviceIndex < deviceList.size());
    const DeviceInfo &device = deviceList.at(m_deviceIndex);

    // ### we're not trying the whole access list (could mean same device on different soundsystems)
    QByteArray soundSystem = device.accessList.first().first;
    debug() << "Setting output soundsystem to" << soundSystem;
    libvlc_audio_output_set(m_player, soundSystem);

    QByteArray deviceName = device.accessList.first().second.toLatin1();
    // print the name as possibly messed up by toLatin1() to see conversion problems
    debug() << "Setting output device to" << deviceName << '(' << device.name << ')';
    libvlc_audio_output_device_set(m_player, soundSystem, deviceName);
}

#if (PHONON_VERSION >= PHONON_VERSION_CHECK(4, 2, 0))
bool AudioOutput::setOutputDevice(const Phonon::AudioOutputDevice &device)
{
    return true;
}
#endif

void AudioOutput::updateVolume()
{
    if (m_player) {
        const int previous_volume = libvlc_audio_get_volume(m_player);
        libvlc_audio_set_volume(m_player, (int)(m_volume * 50));
        debug() << "Volume changed from" << previous_volume << "to" << (int)(m_volume * 100);
    }
}

}
} // Namespace Phonon::VLC
