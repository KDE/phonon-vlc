/*
    Copyright (C) 2007-2008 Tanguy Krotoff <tkrotoff@gmail.com>
    Copyright (C) 2008 Lukas Durfina <lukas.durfina@gmail.com>
    Copyright (C) 2009 Fathi Boudra <fabo@kde.org>
    Copyright (C) 2009-2011 vlc-phonon AUTHORS
    Copyright (C) 2010-2011 Harald Sitter <sitter@kde.org>

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

#include "audiooutput.h"

#include <phonon/pulsesupport.h>

#include <vlc/vlc.h>

#include "backend.h"
#include "utils/debug.h"
#include "devicemanager.h"
#include "mediaobject.h"

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

void AudioOutput::connectPlayer(Player *mediaObject)
{
    DEBUG_BLOCK;
    debug() << mediaObject;
    SinkNode::connectPlayer(mediaObject);
    setOutputDeviceImplementation();
    if (!PulseSupport::getInstance()->isActive())
        applyVolume();
}

void AudioOutput::disconnectPlayer(Player *mediaObject)
{
    DEBUG_BLOCK;
    SinkNode::disconnectPlayer(mediaObject);
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

void AudioOutput::setOutputDeviceImplementation()
{
    Q_ASSERT(m_player);
    DEBUG_BLOCK;
    debug() << this;
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

} // namespace VLC
} // namespace Phonon
