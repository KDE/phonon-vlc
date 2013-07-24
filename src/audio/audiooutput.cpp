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

#include <phonon/pulsesupport.h>

#include <vlc/vlc.h>

#include "backend.h"
#include "utils/debug.h"
#include "devicemanager.h"
#include "mediaobject.h"
#include "media.h"

namespace Phonon {
namespace VLC {

AudioOutput::AudioOutput(QObject *parent)
    : QObject(parent),
      m_volume(1.0)
{
}

AudioOutput::~AudioOutput()
{
}

void AudioOutput::handleConnectToMediaObject(MediaObject *mediaObject)
{
    setOutputDeviceImplementation();
    if (!PulseSupport::getInstance()->isActive())
        applyVolume();
}

void AudioOutput::handleAddToMedia(Media *media)
{
    media->addOption(":audio");
    PulseSupport *pulse = PulseSupport::getInstance();
    if (pulse && pulse->isActive()) {
        pulse->setupStreamEnvironment(m_streamUuid);
    }
}

qreal AudioOutput::volume() const
{
    return m_volume;
}

void AudioOutput::setVolume(qreal volume)
{
    if (m_player) {
        debug() << "async setting of volume to" << volume;
        m_volume = volume;
        applyVolume();
        emit volumeChanged(m_volume);
    }
}

int AudioOutput::outputDevice() const
{
    return m_device.index();
}

bool AudioOutput::setOutputDevice(int deviceIndex)
{
    const AudioOutputDevice device = AudioOutputDevice::fromIndex(deviceIndex);
    if (!device.isValid()) {
        error() << Q_FUNC_INFO << "Unable to find the output device with index" << deviceIndex;
        return false;
    }
    return setOutputDevice(device);
}

#if (PHONON_VERSION >= PHONON_VERSION_CHECK(4, 2, 0))
bool AudioOutput::setOutputDevice(const AudioOutputDevice &newDevice)
{
    debug() << Q_FUNC_INFO;

    if (!newDevice.isValid()) {
        error() << "Invalid audio output device";
        return false;
    }

    if (newDevice == m_device)
        return true;

    m_device = newDevice;
    if (m_player) {
        setOutputDeviceImplementation();
    }

    return true;
}
#endif

#if (PHONON_VERSION >= PHONON_VERSION_CHECK(4, 6, 50))
void AudioOutput::setStreamUuid(QString uuid)
{
    qDebug() << Q_FUNC_INFO;
    qDebug() << uuid;
    m_streamUuid = uuid;
}
#endif

void AudioOutput::setOutputDeviceImplementation()
{
    Q_ASSERT(m_player);
    if (PulseSupport::getInstance()->isActive()) {
        m_player->setAudioOutput("pulse");
        debug() << "Setting aout to pulse";
        return;
    }

    const QVariant dalProperty = m_device.property("deviceAccessList");
    if (!dalProperty.isValid()) {
        error() << "Device" << m_device.property("name") << "has no access list";
        return;
    }
    const DeviceAccessList deviceAccessList = dalProperty.value<DeviceAccessList>();
    if (deviceAccessList.isEmpty()) {
        error() << "Device" << m_device.property("name") << "has an empty access list";
        return;
    }

    // ### we're not trying the whole access list (could mean same device on different soundsystems)
    const DeviceAccess &firstDeviceAccess = deviceAccessList.first();

    QByteArray soundSystem = firstDeviceAccess.first;
    debug() << "Setting output soundsystem to" << soundSystem;
    m_player->setAudioOutput(soundSystem);

    QByteArray deviceName = firstDeviceAccess.second.toLatin1();
    if (!deviceName.isEmpty()) {
        // print the name as possibly messed up by toLatin1() to see conversion problems
        debug() << "Setting output device to" << deviceName << '(' << m_device.property("name") << ')';
        m_player->setAudioOutputDevice(soundSystem, deviceName);
    }
}

void AudioOutput::applyVolume()
{
    if (m_player) {
        const int preVolume = m_player->audioVolume();
        const int newVolume = m_volume * 100;
        m_player->setAudioVolume(newVolume);

        debug() << "Volume changed from" << preVolume << "to" << newVolume;
    }
}

}
} // Namespace Phonon::VLC
