/*
    Copyright (C) 2007-2008 Tanguy Krotoff <tkrotoff@gmail.com>
    Copyright (C) 2008 Lukas Durfina <lukas.durfina@gmail.com>
    Copyright (C) 2009 Fathi Boudra <fabo@kde.org>
    Copyright (C) 2013-2015 Harald Sitter <sitter@kde.org>

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
#include "media.h"

namespace Phonon {
namespace VLC {

AudioOutput::AudioOutput(QObject *parent)
    : QObject(parent)
    , m_volume(1.0)
    , m_muted(false)
    , m_category(Phonon::NoCategory)
{
}

AudioOutput::~AudioOutput()
{
}

#if (LIBVLC_VERSION_INT >= LIBVLC_VERSION(3, 0, 0, 0))
static libvlc_media_player_role categoryToRole(Category category)
{
    switch(category) {
    case NoCategory:
        return libvlc_role_None;
    case NotificationCategory:
        return libvlc_role_Notification;
    case MusicCategory:
        return libvlc_role_Music;
    case VideoCategory:
        return libvlc_role_Video;
    case CommunicationCategory:
        return libvlc_role_Communication;
    case GameCategory:
        return libvlc_role_Game;
    case AccessibilityCategory:
        return libvlc_role_Accessibility;
    }
    return libvlc_role_None;
}
#endif

void AudioOutput::handleConnectToMediaObject(MediaObject *mediaObject)
{
    Q_UNUSED(mediaObject);
    setOutputDeviceImplementation();
    if (!PulseSupport::getInstance()->isActive()) {
        connect(m_player, SIGNAL(mutedChanged(bool)),
                this, SLOT(onMutedChanged(bool)));
        connect(m_player, SIGNAL(volumeChanged(float)),
                this, SLOT(onVolumeChanged(float)));
        applyVolume();
    }
#if (LIBVLC_VERSION_INT >= LIBVLC_VERSION(3, 0, 0, 0))
    libvlc_media_player_set_role(*m_player, categoryToRole(m_category));
#endif
}

void AudioOutput::handleAddToMedia(Media *media)
{
    media->addOption(":audio");
#if (PHONON_VERSION >= PHONON_VERSION_CHECK(4, 6, 50))
    PulseSupport *pulse = PulseSupport::getInstance();
    if (pulse && pulse->isActive()) {
        pulse->setupStreamEnvironment(m_streamUuid);
    }
#endif
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

#if (LIBVLC_VERSION_INT < LIBVLC_VERSION(2, 2, 2, 0))
        emit volumeChanged(m_volume);
#endif
    }
}

#if (PHONON_VERSION >= PHONON_VERSION_CHECK(4, 8, 50))
void AudioOutput::setMuted(bool mute)
{
    if (mute == m_player->mute()) {
        // Make sure we actually have propagated the mutness into the frontend.
        onMutedChanged(mute);
        return;
    }
    m_player->setMute(mute);
}
#endif

void AudioOutput::setCategory(Category category)
{
    m_category = category;
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
    DEBUG_BLOCK;
    debug() << uuid;
    m_streamUuid = uuid;
}
#endif

void AudioOutput::setOutputDeviceImplementation()
{
    Q_ASSERT(m_player);
#if (LIBVLC_VERSION_INT >= LIBVLC_VERSION(2, 2, 0, 0))
    // VLC 2.2 has the PulseSupport overrides always disabled because of
    // incompatibility. Also see backend.cpp for more detals.
    // To get access to the correct activity state we need to temporarily
    // enable pulse and then disable it again. This is necessary because isActive
    // is in fact isActive&isEnabled.............
    PulseSupport::getInstance()->enable(true);
    const bool pulseActive = PulseSupport::getInstance()->isActive();
    PulseSupport::getInstance()->enable(false);
#else
    const bool pulseActive = PulseSupport::getInstance()->isActive();
#endif
    if (pulseActive) {
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

#if (LIBVLC_VERSION_INT < LIBVLC_VERSION(2, 2, 2, 0))
        onMutedChanged(m_volume == 0.0);
        onVolumeChanged(newVolume);
#endif

        debug() << "Volume changed from" << preVolume << "to" << newVolume;
    }
}

void AudioOutput::onMutedChanged(bool mute)
{
    m_muted = mute;
    emit mutedChanged(mute);
#if (PHONON_VERSION < PHONON_VERSION_CHECK(4, 8, 51))
    // Previously we had no interface signal to communicate mutness, so instead
    // emit volume.
    mute ? emit volumeChanged(0.0) : emit volumeChanged(volume());
#endif
}

void AudioOutput::onVolumeChanged(float volume)
{
    m_volume = volume;
    emit volumeChanged(volume);
}

}
} // Namespace Phonon::VLC
