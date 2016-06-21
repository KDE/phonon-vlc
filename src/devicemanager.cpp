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

#include <phonon/pulsesupport.h>

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

const DeviceAccessList& DeviceInfo::accessList() const
{
    return m_accessList;
}

void DeviceInfo::addAccess(const DeviceAccess& access)
{
    if (m_accessList.isEmpty())
        m_description = access.first + ": " + access.second;
    m_accessList.append(access);
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
    Q_ASSERT(parent);
    updateDeviceList();
}

DeviceManager::~DeviceManager()
{
}

QList<int> DeviceManager::deviceIds(ObjectDescriptionType type)
{
    DeviceInfo::Capability capability = DeviceInfo::None;
    switch (type) {
    case Phonon::AudioOutputDeviceType:
        capability = DeviceInfo::AudioOutput;
        break;
    case Phonon::AudioCaptureDeviceType:
        capability = DeviceInfo::AudioCapture;
        break;
    case Phonon::VideoCaptureDeviceType:
        capability = DeviceInfo::VideoCapture;
        break;
    default: ;
    }

    QList<int> ids;
    foreach (const DeviceInfo &device, m_devices) {
        if (device.capabilities() & capability)
            ids.append(device.id());
    }

    return ids;
}

QHash<QByteArray, QVariant> DeviceManager::deviceProperties(int id)
{
    QHash<QByteArray, QVariant> properties;

    foreach (const DeviceInfo &device, m_devices) {
        if (device.id() == id) {
            properties.insert("name", device.name());
            properties.insert("description", device.description());
            properties.insert("isAdvanced", device.isAdvanced());
            properties.insert("deviceAccessList", QVariant::fromValue<Phonon::DeviceAccessList>(device.accessList()));
            properties.insert("discovererIcon", "vlc");

            if (device.capabilities() & DeviceInfo::AudioOutput) {
                properties.insert("icon", QLatin1String("audio-card"));
            }

            if (device.capabilities() & DeviceInfo::AudioCapture) {
                properties.insert("hasaudio", true);
                properties.insert("icon", QLatin1String("audio-input-microphone"));
            }

            if (device.capabilities() & DeviceInfo::VideoCapture) {
                properties.insert("hasvideo", true);
                properties.insert("icon", QLatin1String("camera-web"));
            }
            break;
        }
    }

    return properties;
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
    QList<DeviceInfo> newDeviceList;

    if (!LibVLC::self || !pvlc_libvlc)
        return;

    QList<QByteArray> audioOutBackends = vlcAudioOutBackends();

    PulseSupport *pulse = PulseSupport::getInstance();
#if (PHONON_VERSION < PHONON_VERSION_CHECK(4, 8, 51))
    if (pulse && pulse->isActive()) {
#else
    if (pulse && pulse->isUsable()) {
#endif
        if (audioOutBackends.contains("pulse")) {
            DeviceInfo defaultAudioOutputDevice(tr("Default"), false);
            defaultAudioOutputDevice.setCapabilities(DeviceInfo::AudioOutput);
            defaultAudioOutputDevice.addAccess(DeviceAccess("pulse", "default"));
            newDeviceList.append(defaultAudioOutputDevice);
#if (PHONON_VERSION >= PHONON_VERSION_CHECK(4, 8, 51))
            pulse->request(true);
#endif
            return;
        } else {
            pulse->enable(false);
        }
    }

    QList<QByteArray> knownSoundSystems;
    // Whitelist - Order has no particular impact.
    // NOTE: if listing was not intercepted by the PA code above we also need
    //       to try injecting the pulse aout as otherwise the user would have to
    //       use the fake PA device in ALSA to output through PA (kind of silly).
    knownSoundSystems << QByteArray("pulse")
                      << QByteArray("alsa")
                      << QByteArray("oss")
                      << QByteArray("jack")
                      << QByteArray("aout_directx") // Windows up to VLC 2.0
                      << QByteArray("directsound") // Windows from VLC 2.1 upwards
                      << QByteArray("auhal"); // Mac
    foreach (const QByteArray &soundSystem, knownSoundSystems) {
        if (!audioOutBackends.contains(soundSystem)) {
            debug() << "Sound system" << soundSystem << "not supported by libvlc";
            continue;
        }

        // FIXME: there is a rather ungodly amount of code duplication going
        //        on here.
#if (LIBVLC_VERSION_INT >= LIBVLC_VERSION(2, 2, 0, 0))
        bool hasDevices = false;
        VLC_FOREACH(audio_output_device,
                    device,
                    libvlc_audio_output_device_list_get(pvlc_libvlc, soundSystem),
                    libvlc_audio_output_device_list_release) {
            QString idName = QString::fromUtf8(device->psz_device);
            QString longName = QString::fromUtf8(device->psz_description);

            debug() << "found device" << soundSystem << idName << longName;

            DeviceInfo info(longName, true);
            info.addAccess(DeviceAccess(soundSystem, idName));
            info.setCapabilities(DeviceInfo::AudioOutput);
            newDeviceList.append(info);

            hasDevices = true;
        }

        if (!hasDevices) {
            debug() << "manually injecting sound system" << soundSystem;
            DeviceInfo info(QString::fromUtf8(soundSystem), false);
            info.addAccess(DeviceAccess(soundSystem, ""));
            info.setCapabilities(DeviceInfo::AudioOutput);
            newDeviceList.append(info);
        }
#else
        const int deviceCount = libvlc_audio_output_device_count(pvlc_libvlc, soundSystem);

        for (int i = 0; i < deviceCount; i++) {
            VString idName(libvlc_audio_output_device_id(libvlc, soundSystem, i));
            VString longName(libvlc_audio_output_device_longname(pvlc_libvlc, soundSystem, i));

            debug() << "found device" << soundSystem << idName << longName;

            DeviceInfo info(longName, true);
            info.addAccess(DeviceAccess(soundSystem, idName));
            info.setCapabilities(DeviceInfo::AudioOutput);
            newDeviceList.append(info);
        }

        // libVLC gives no devices for some sound systems, like OSS
        if (deviceCount == 0) {
            debug() << "manually injecting sound system" << soundSystem;
            // NOTE: Do not mark manually injected devices as advanced.
            //       libphonon filters advanced devices from the default
            //       selection which on systems such as OSX or Windows can
            //       lead to an empty device list as the injected device is
            //       the only available one.
            DeviceInfo info(QString::fromUtf8(soundSystem), false);
            info.addAccess(DeviceAccess(soundSystem, ""));
            info.setCapabilities(DeviceInfo::AudioOutput);
            newDeviceList.append(info);
        }
#endif
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

}
}
