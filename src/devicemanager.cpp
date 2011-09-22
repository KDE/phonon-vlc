/*
    Copyright (C) 2007-2008 Tanguy Krotoff <tkrotoff@gmail.com>
    Copyright (C) 2008 Lukas Durfina <lukas.durfina@gmail.com>
    Copyright (C) 2009 Fathi Boudra <fabo@kde.org>
    Copyright (C) 2009-2010 vlc-phonon AUTHORS
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

#ifdef PHONON_PULSESUPPORT
#  include <phonon/pulsesupport.h>
#endif

#include <vlc/vlc.h>

#include "backend.h"
#include "debug.h"
#include "libvlc.h"

QT_BEGIN_NAMESPACE

namespace Phonon
{
namespace VLC
{

/*
 * Device Info
 */

DeviceInfo::DeviceInfo(const QByteArray &name,
                       const QString &description,
                       bool isAdvanced)
{
    // Get an id
    static int counter = 0;
    m_id = counter++;

    // Get name and description for the device
    m_name = name;
    m_description = description;
    m_isAdvanced = isAdvanced;
    m_capabilities = None;
}

int DeviceInfo::id() const
{
    return m_id;
}

const QByteArray& DeviceInfo::name() const
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
#ifdef __GNUC__
#warning capture code needs rewrite, was removed due to brokeneness
#endif
    m_deviceLists << &m_audioOutputDeviceList
                  << &m_audioCaptureDeviceList
                  << &m_videoCaptureDeviceList;
    updateDeviceList();
}

DeviceManager::~DeviceManager()
{
    foreach (const QList<DeviceInfo> *list, m_deviceLists) {
        const_cast<QList<DeviceInfo> *>(list)->clear();
    }
}

bool DeviceManager::canOpenDevice() const
{
    return true;
}

int DeviceManager::deviceId(const QByteArray &name) const
{
    foreach (const QList<DeviceInfo> *list, m_deviceLists) {
        foreach (const DeviceInfo &device, *list) {
            if (device.name() == name)
                return device.id();
        }
    }

    return -1;
}

QString DeviceManager::deviceDescription(int id) const
{
    foreach (const QList<DeviceInfo> *list, m_deviceLists) {
        foreach (const DeviceInfo &device, *list) {
            if (device.id() == id)
                return QString(device.description());
        }
    }

    return QString();
}

void DeviceManager::updateDeviceSublist(const QList<DeviceInfo> &newDevices, QList<DeviceInfo> &deviceList)
{
    // New and old device counts
    int newDeviceCount = newDevices.count();
    int oldDeviceCount = deviceList.count();

    for (int i = 0; i < newDeviceCount; ++i) {
        int id = deviceId(newDevices[i].name());
        if (id == -1) {
            // This is a new device, add it
            deviceList.append(newDevices[i]);
            id = deviceId(newDevices[i].name());
            emit deviceAdded(id);

            debug() << "Added backend device" << newDevices[i].name() << "with id" << id;
        }
    }

    if (newDeviceCount < oldDeviceCount) {
        // A device was removed
        for (int i = oldDeviceCount - 1; i >= 0; --i) {
            QByteArray currId = deviceList[i].name();
            bool b_found = false;
            for (int k = newDeviceCount - 1; k >= 0; --k) {
                if (currId == newDevices[k].name()) {
                    b_found = true;
                    break;
                }
            }
            if (!b_found) {
                emit deviceRemoved(deviceId(currId));
                deviceList.removeAt(i);
            }
        }
    }
}

static QList<QByteArray> vlcAudioOutBackends()
{
    QList<QByteArray> ret;

    libvlc_audio_output_t *firstAudioOut = libvlc_audio_output_list_get(libvlc);
    if (!firstAudioOut) {
        error() << "libVLC:" << LibVLC::errorMessage();
        return ret;
    }
    for (libvlc_audio_output_t *audioOut = firstAudioOut; audioOut; audioOut = audioOut->p_next) {
        ret.append(QByteArray(audioOut->psz_name));
    }
    libvlc_audio_output_list_release(firstAudioOut);

    return ret;
}

void DeviceManager::updateDeviceList()
{
    // Lists for audio output devices
    QList<DeviceInfo> audioOutputDeviceList;

    audioOutputDeviceList.append(DeviceInfo("default"));
    DeviceInfo &defaultAudioOutputDevice = audioOutputDeviceList.first();
    defaultAudioOutputDevice.setCapabilities(DeviceInfo::AudioOutput);

    if (!LibVLC::self || !libvlc)
        return;

    QList<QByteArray> audioOutBackends = vlcAudioOutBackends();

#ifdef PHONON_PULSESUPPORT
    PulseSupport *pulse = PulseSupport::getInstance();
    if (pulse && pulse->isActive()) {
        if (audioOutBackends.contains("pulse")) {
            defaultAudioOutputDevice.setAdvanced(false);
            defaultAudioOutputDevice.addAccess(DeviceAccess("pulse", "default"));
            return;
        } else {
            pulse->enable(false);
        }
    }
#endif

    QList<QByteArray> knownSoundSystems;
    knownSoundSystems << "alsa" << "oss";
    foreach (const QByteArray &soundSystem, knownSoundSystems) {
        if (audioOutBackends.contains(soundSystem)) {
            const int deviceCount = libvlc_audio_output_device_count(libvlc, soundSystem);

            for (int i = 0; i < deviceCount; i++) {
                const char *idName = libvlc_audio_output_device_id(libvlc, soundSystem, i);
                const char *longName = libvlc_audio_output_device_longname(libvlc, soundSystem, i);

                DeviceInfo device(longName, QByteArray() /* no description, sorry */, false);
                device.addAccess(DeviceAccess(soundSystem, idName));
                device.setCapabilities(DeviceInfo::AudioOutput);
                audioOutputDeviceList.append(device);
            }
            break;
        }
    }

    updateDeviceSublist(audioOutputDeviceList, m_audioOutputDeviceList);
}

const QList<DeviceInfo> DeviceManager::audioCaptureDevices() const
{
    return m_audioCaptureDeviceList;
}

const QList<DeviceInfo> DeviceManager::videoCaptureDevices() const
{
    return m_videoCaptureDeviceList;
}

const QList<DeviceInfo> DeviceManager::audioOutputDevices() const
{
    return m_audioOutputDeviceList;
}

}
}

QT_END_NAMESPACE
