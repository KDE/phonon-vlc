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
#include "utils/debug.h"
#include "devicemanager.h"
#include "mediaobject.h"

namespace Phonon {
namespace VLC {

#ifdef __GNUC__
#warning implement 4.2 interface, AudioOutputInterface42
#endif

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
    setOutputDeviceImplementation();
    connect(m_mediaObject, SIGNAL(playbackCommenced()), this, SLOT(updateVolume()));
}

void AudioOutput::disconnectFromMediaObject(MediaObject *mediaObject)
{
    SinkNode::disconnectFromMediaObject(mediaObject);
    if (m_mediaObject) {
        disconnect(m_mediaObject, SIGNAL(playbackCommenced()), this, SLOT(updateVolume()));
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
        updateVolume();
        emit volumeChanged(m_volume);
    }
}

int AudioOutput::outputDevice() const
{
    return m_deviceIndex;
}

bool AudioOutput::setOutputDevice(int deviceIndex)
{
    const DeviceInfo *device = Backend::self->deviceManager()->device(deviceIndex);
    if (!device) {
        error() << "Unable to find any output device with index" << deviceIndex;
        return false;
    }
    if (device->accessList().isEmpty()) {
        error() << "This output device cannot be used, it has no information for accessing it";
        return false;
    }
    if (m_deviceIndex != deviceIndex) {
        m_deviceIndex = deviceIndex;
        if (m_player) {
            setOutputDeviceImplementation();
        }
    }
    return true;
}

void AudioOutput::setOutputDeviceImplementation()
{
    Q_ASSERT(m_player);
#ifdef PHONON_PULSESUPPORT
    if (PulseSupport::getInstance()->isActive()) {
        m_player->setAudioOutput("pulse");
        debug() << "Setting aout to pulse";
        return;
    }
#endif
    const DeviceInfo *device = Backend::self->deviceManager()->device(m_deviceIndex);
    if (!device || device->accessList().isEmpty())
        return;

    // ### we're not trying the whole access list (could mean same device on different soundsystems)
    const DeviceAccess &firstDeviceAccess = device->accessList().first();

    QByteArray soundSystem = firstDeviceAccess.first;
    debug() << "Setting output soundsystem to" << soundSystem;
    m_player->setAudioOutput(soundSystem);

    QByteArray deviceName = firstDeviceAccess.second.toLatin1();
    // print the name as possibly messed up by toLatin1() to see conversion problems
    debug() << "Setting output device to" << deviceName << '(' << device->name() << ')';
    m_player->setAudioOutputDevice(soundSystem, deviceName);
}

void AudioOutput::updateVolume()
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
