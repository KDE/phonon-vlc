/*
    Copyright (C) 2007-2008 Tanguy Krotoff <tkrotoff@gmail.com>
    Copyright (C) 2008 Lukas Durfina <lukas.durfina@gmail.com>
    Copyright (C) 2009 Fathi Boudra <fabo@kde.org>
    Copyright (C) 2009-2010 vlc-phonon AUTHORS <kde-multimedia@kde.org>
    Copyright (C) 2011 Harald Sitter <sitter@kde.org>

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

#include "devicemanager.h"

#include <vlc/vlc.h>

#include "backend.h"
#include "utils/debug.h"
#include "utils/libvlc.h"
#include "utils/vstring.h"

namespace Phonon
{
namespace VLC
{

/*
 * Device Info
 */

DeviceInfo::DeviceInfo(const QString &name, bool isAdvanced)
{
    // Get an id
    static int counter = 0;
    m_id = counter++;

    // Get name and description for the device
    m_name = name;
    m_isAdvanced = isAdvanced;
    m_capabilities = None;

    // A default device should never be advanced
    if (name.startsWith(QLatin1String("default"), Qt::CaseInsensitive))
        m_isAdvanced = false;
}

int DeviceInfo::id() const
{
    return m_id;
}

const QString& DeviceInfo::name() const
{
    return m_name;
}

const QString& DeviceInfo::description() const
{
    return m_description;
}

bool DeviceInfo::isAdvanced() const
{
    return m_isAdvanced;
}

void DeviceInfo::setAdvanced(bool advanced)
{
    m_isAdvanced = advanced;
}

DeviceInfo::SoundSystem DeviceInfo::soundSystem() const
{
    return m_soundSystem;
}

QByteArray DeviceInfo::deviceName() const
{
    return m_deviceName;
}

void DeviceInfo::setAccessInfo(SoundSystem soundSystem, const QByteArray &deviceName)
{
    m_soundSystem = soundSystem;
    m_deviceName = deviceName;
}

quint16 DeviceInfo::capabilities() const
{
    return m_capabilities;
}

void DeviceInfo::setCapabilities(quint16 cap)
{
    m_capabilities = cap;
}


/*
 * Device Manager
 */

DeviceManager::DeviceManager(Backend *parent)
    : QObject(parent)
    , m_backend(parent)
{
    DEBUG_BLOCK;
    Q_ASSERT(parent);

    m_knownSoundSystems.insert(DeviceInfo::Pulse, QByteArray("pulse"));
    m_knownSoundSystems.insert(DeviceInfo::Alsa, QByteArray("alsa"));
    m_knownSoundSystems.insert(DeviceInfo::Oss, QByteArray("oss"));
    m_knownSoundSystems.insert(DeviceInfo::Jack, QByteArray("jack"));
    m_knownSoundSystems.insert(DeviceInfo::Aout_DirectX, QByteArray("aout_directx"));
    m_knownSoundSystems.insert(DeviceInfo::DirectSound, QByteArray("directsound"));
    m_knownSoundSystems.insert(DeviceInfo::Auhal, QByteArray("auhal"));

    updateDeviceList();
}

DeviceManager::~DeviceManager()
{
}

const DeviceInfo *DeviceManager::device(int id) const
{
    for (int i = 0; i < m_devices.size(); i ++) {
        if (m_devices[i].id() == id)
            return &m_devices[i];
    }

    return NULL;
}

static QList<QByteArray> vlcAudioOutBackends()
{
    QList<QByteArray> ret;

    VLC_FOREACH_LIST(audio_output, aout) {
        QByteArray name(aout->psz_name);
        if (!ret.contains(name))
            ret.append(name);
    }

    return ret;
}

void DeviceManager::updateDeviceList()
{
    DEBUG_BLOCK;
    QList<DeviceInfo> newDeviceList;

    if (!LibVLC::self || !libvlc)
        return;

    QList<QByteArray> audioOutBackends = vlcAudioOutBackends();

    // If we have pulse, screw the rest.
    if (audioOutBackends.contains("pulse")) {
        DeviceInfo defaultAudioOutputDevice(tr("Default"), false);
        defaultAudioOutputDevice.setCapabilities(DeviceInfo::AudioOutput);
        defaultAudioOutputDevice.setAccessInfo(DeviceInfo::Pulse, "default");
        newDeviceList.append(defaultAudioOutputDevice);
        return;
    }

    // NOTE: if listing was not intercepted by the PA code above we also need
    //       to try injecting the pulse aout as otherwise the user would have to
    //       use the fake PA device in ALSA to output through PA (kind of silly).

    foreach (const QByteArray &soundSystemName, m_knownSoundSystems.values()) {
        if (!audioOutBackends.contains(soundSystemName)) {
            debug() << "Sound system" << soundSystemName << "not supported by libvlc";
            continue;
        }

        const int deviceCount = libvlc_audio_output_device_count(libvlc, soundSystemName);

        for (int i = 0; i < deviceCount; i++) {
            VString idName(libvlc_audio_output_device_id(libvlc, soundSystemName, i));
            VString longName(libvlc_audio_output_device_longname(libvlc, soundSystemName, i));

            debug() << "found device" << soundSystemName << idName << longName;

            DeviceInfo device(longName, true);
            device.setAccessInfo(soundSystemFromName(soundSystemName), idName.const_data());
            device.setCapabilities(DeviceInfo::AudioOutput);
            newDeviceList.append(device);
        }

        // libVLC gives no devices for some sound systems, like OSS
        if (deviceCount == 0) {
            debug() << "manually injecting sound system" << soundSystemName;
            // NOTE: Do not mark manually injected devices as advanced.
            //       libphonon filters advanced devices from the default
            //       selection which on systems such as OSX or Windows can
            //       lead to an empty device list as the injected device is
            //       the only available one.
            DeviceInfo device(QString::fromUtf8(soundSystemName), false);
            device.setAccessInfo(soundSystemFromName(soundSystemName), "");
            device.setCapabilities(DeviceInfo::AudioOutput);
            newDeviceList.append(device);
        }
    }

    /*
     * Compares the list with the devices available at the moment with the last list. If
     * a new device is seen, a signal is emitted. If a device disappeared, another signal
     * is emitted.
     */

    // Search for added devices
    for (int i = 0; i < newDeviceList.count(); ++i) {
        int id = newDeviceList[i].id();
        if (!listContainsDevice(m_devices, id)) {
            // This is a new device, add it
            m_devices.append(newDeviceList[i]);
            emit deviceAdded(id);

            debug() << "Added backend device" << newDeviceList[i].name();
        }
    }

    // Search for removed devices
    for (int i = m_devices.count() - 1; i >= 0; --i) {
        int id = m_devices[i].id();
        if (!listContainsDevice(newDeviceList, id)) {
            emit deviceRemoved(id);
            m_devices.removeAt(i);
        }
    }
}

bool DeviceManager::listContainsDevice(const QList<DeviceInfo> &list, int id)
{
    foreach (const DeviceInfo &d, list) {
        if (d.id() == id)
            return true;
    }
    return false;
}

DeviceInfo::SoundSystem DeviceManager::soundSystemFromName(const QByteArray &name)
{
    foreach (DeviceInfo::SoundSystem soundSystem, m_knownSoundSystems.keys()) {
        if (m_knownSoundSystems.value(soundSystem) == name)
            return soundSystem;
    }

    return DeviceInfo::UnknownSoundSystem;
}

}
}
