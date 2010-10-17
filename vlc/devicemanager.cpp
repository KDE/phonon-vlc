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

#include "devicemanager.h"
#include "devicescan.h"
#include "backend.h"
//#include "videowidget.h"
//#include "widgetrenderer.h"
#include "vlcloader.h"

#ifdef PHONON_PULSESUPPORT
#  include <phonon/pulsesupport.h>
#endif

QT_BEGIN_NAMESPACE

namespace Phonon
{
namespace VLC
{

/**
 * Constructs a device info object and sets it's device identifiers.
 */
DeviceInfo::DeviceInfo( const QByteArray& name, const QString& description)
{
    // Get an id
    static int counter = 0;
    id = counter++;

    // Get name and description for the device
    this->name = name;
    this->description = description;
    capabilities = None;
}

/**
 * Constructs a device manager and immediately updates the device lists.
 */
DeviceManager::DeviceManager(Backend *parent)
    : QObject(parent)
    , m_backend(parent)
{
    updateDeviceList();
}

/**
 * Clears all the device lists before destroying.
 */
DeviceManager::~DeviceManager()
{
    m_audioCaptureDeviceList.clear();
    m_videoCaptureDeviceList.clear();
    m_audioOutputDeviceList.clear();
}

/**
 * \return True if the device can be opened.
 */
bool DeviceManager::canOpenDevice() const
{
    return true;
}

/**
 * Searches for the device in the device lists, by comparing it's name
 *
 * \param name Name identifier for the device to search for
 * \return A positive device id or -1 if device does not exist.
 */
int DeviceManager::deviceId(const QByteArray &name) const
{
    for (int i = 0 ; i < m_audioCaptureDeviceList.size() ; ++i) {
        if (m_audioCaptureDeviceList[i].name == name)
            return m_audioCaptureDeviceList[i].id;
    }

    for (int i = 0 ; i < m_videoCaptureDeviceList.size() ; ++i) {
        if (m_videoCaptureDeviceList[i].name == name)
            return m_videoCaptureDeviceList[i].id;
    }

    for (int i = 0 ; i < m_audioOutputDeviceList.size() ; ++i) {
        if (m_audioOutputDeviceList[i].name == name)
            return m_audioOutputDeviceList[i].id;
    }

    return -1;
}

/**
 * Get a human-readable description from a device id.
 * \param i_id Device identifier
 */
QString DeviceManager::deviceDescription(int i_id) const
{
    for (int i = 0 ; i < m_audioCaptureDeviceList.size() ; ++i) {
        if (m_audioCaptureDeviceList[i].id == i_id)
            return m_audioCaptureDeviceList[i].description;
    }

    for (int i = 0 ; i < m_videoCaptureDeviceList.size() ; ++i) {
        if (m_videoCaptureDeviceList[i].id == i_id)
            return m_videoCaptureDeviceList[i].description;
    }

    for (int i = 0 ; i < m_audioOutputDeviceList.size() ; ++i) {
        if (m_audioOutputDeviceList[i].id == i_id)
            return m_audioOutputDeviceList[i].description;
    }

    return QByteArray();
}

/** \internal
 * Compares the list with the devices available at the moment with the last list. If
 * a new device is seen, a signal is emitted. If a device dissapeared, another signal
 * is emitted. The devices are only from one category (example audio output devices).
 *
 * \param newDevices The list for the devices currently available
 * \param deviceList The old device list
 *
 * \see deviceAdded
 * \see deviceRemoved
 */
void DeviceManager::updateDeviceSublist(const QList<DeviceInfo> &newDevices, QList<DeviceInfo> &deviceList)
{
    // New and old device counts
    int ndc = newDevices.count();
    int odc = deviceList.count();

    for (int i = 0 ; i < ndc ; ++i) {
        if (deviceId(newDevices[i].name) == -1) {
            // This is a new device, add it
            deviceList.append(newDevices[i]);
            emit deviceAdded(deviceId(newDevices[i].name));
            qDebug() << "added device " << newDevices[i].name.data() << "id" << deviceId(newDevices[i].name);
        }
    }

    if (ndc < odc) {
        // A device was removed
        for (int i = odc - 1; i >= 0 ; --i) {
            QByteArray currId = deviceList[i].name;
            bool b_found = false;
            for (int k = ndc - 1; k >= 0 ; --k) {
                if (currId == newDevices[k].name) {
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

/**
 * Update the current list of active devices. It probes for audio output devices,
 * audio capture devices, video capture devices. The methods depend on the
 * device types.
 */
void DeviceManager::updateDeviceList()
{
    // Lists for capture devices
    QList<DeviceInfo> devices, vcs, acs;
    int i;

    // Get the list of available capture devices
    scanDevices(devices);

    // See the device capabilities and sort them accordingly
    for (i = 0; i < devices.count(); ++ i) {
        if (devices[i].capabilities & DeviceInfo::VideoCapture)
            vcs << devices[i];
        if (devices[i].capabilities & DeviceInfo::AudioCapture)
            acs << devices[i];
    }

    devices.clear();

    // Update the capture device lists
    updateDeviceSublist(vcs, m_videoCaptureDeviceList);
    updateDeviceSublist(acs, m_audioCaptureDeviceList);

    // Lists for audio output devices
    QList<DeviceInfo> aos;
    aos.append(DeviceInfo("default"));
    aos.last().capabilities = DeviceInfo::AudioOutput;

    if(!vlc_instance)
        return;

    // Get the list of available audio outputs
    libvlc_audio_output_t *p_ao_list = libvlc_audio_output_list_get(vlc_instance);
    if (!p_ao_list) {
        qDebug() << "libvlc exception:" << libvlc_errmsg();
    }
    libvlc_audio_output_t *p_start = p_ao_list;

    bool checkpulse = false;
#ifdef PHONON_PULSESUPPORT
    PulseSupport *pulse = PulseSupport::getInstance();
    checkpulse = pulse->isActive();
#endif
    bool haspulse = false;
    while (p_ao_list) {
        if (checkpulse && 0 == strcmp(p_ao_list->psz_name, "pulse")) {
#ifndef PHONON_VLC_NO_EXPERIMENTAL
            aos.last().accessList.append(DeviceAccess("pulse", "default"));
#endif // PHONON_VLC_NO_EXPERIMENTAL
            haspulse = true;
            break;
        }

        aos.append(DeviceInfo(p_ao_list->psz_name));
#ifndef PHONON_VLC_NO_EXPERIMENTAL
        aos.last().accessList.append(DeviceAccess("?", p_ao_list->psz_name));
#endif // PHONON_VLC_NO_EXPERIMENTAL
        aos.last().capabilities = DeviceInfo::AudioOutput;

        p_ao_list = p_ao_list->p_next;
    }
    libvlc_audio_output_list_release(p_start);


#ifdef PHONON_PULSESUPPORT
    if (haspulse) {
        return;
    }
    pulse->enable(false);
#endif

    updateDeviceSublist(aos, m_audioOutputDeviceList);
}

/**
* Return a list of name identifiers for audio capture devices.
*/
const QList<DeviceInfo> DeviceManager::audioCaptureDevices() const
{
    return m_audioCaptureDeviceList;
}

/**
* Return a list of name identifiers for video capture devices.
*/
const QList<DeviceInfo> DeviceManager::videoCaptureDevices() const
{
    return m_videoCaptureDeviceList;
}

/**
 * Return a list of name identifiers for audio output devices.
 */
const QList<DeviceInfo> DeviceManager::audioOutputDevices() const
{
    return m_audioOutputDeviceList;
}

}
}

QT_END_NAMESPACE
