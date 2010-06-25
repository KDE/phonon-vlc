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
#include "backend.h"
//#include "videowidget.h"
//#include "widgetrenderer.h"
#include "vlcloader.h"

#ifdef PHONON_PULSESUPPORT
#  include <phonon/pulsesupport.h>
#endif

#ifdef HAVE_LIBV4L2
#  include <v4l2devices.h>
#endif

/**
 * This class manages the list of currently active output devices.
 */

QT_BEGIN_NAMESPACE

namespace Phonon
{
namespace VLC {

Device::Device(const QByteArray &deviceId, const QByteArray &hwId)
{
    // Get an id
    static int counter = 0;
    id = counter++;

    // Get name and description for the device
    nameId = deviceId;
    this->hwId = hwId;
    description = deviceId == "default" ? "Default device" : "";
    v4l = false;
}

DeviceManager::DeviceManager(Backend *parent)
        : QObject(parent)
        , m_backend(parent)
{
    updateDeviceList();
}

DeviceManager::~DeviceManager()
{
    m_audioCaptureDeviceList.clear();
    m_videoCaptureDeviceList.clear();
    m_audioOutputDeviceList.clear();
}

bool DeviceManager::canOpenDevice() const
{
    return true;
}

/**
 * Return a positive device id or -1 if device does not exist.
 */
int DeviceManager::deviceId(const QByteArray &nameId) const
{
    for (int i = 0 ; i < m_audioCaptureDeviceList.size() ; ++i) {
        if (m_audioCaptureDeviceList[i].nameId == nameId)
            return m_audioCaptureDeviceList[i].id;
    }

    for (int i = 0 ; i < m_videoCaptureDeviceList.size() ; ++i) {
        if (m_videoCaptureDeviceList[i].nameId == nameId)
            return m_videoCaptureDeviceList[i].id;
    }

    for (int i = 0 ; i < m_audioOutputDeviceList.size() ; ++i) {
        if (m_audioOutputDeviceList[i].nameId == nameId)
            return m_audioOutputDeviceList[i].id;
    }

    return -1;
}

/**
 * Get a human-readable description from a device id.
 */
QByteArray DeviceManager::deviceDescription(int i_id) const
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

void DeviceManager::updateDeviceSublist(const QList<Device> &newDevices, QList<Device> &deviceList)
{
    // New and old device counts
    int ndc = newDevices.count();
    int odc = deviceList.count();

    for (int i = 0 ; i < ndc ; ++i) {
        if (deviceId(newDevices[i].nameId) == -1) {
            // This is a new device, add it
            deviceList.append(newDevices[i]);
            emit deviceAdded(deviceId(newDevices[i].nameId));
            qDebug() << "added device " << newDevices[i].nameId.data() << "id" << deviceId(newDevices[i].nameId);
        }
    }

    if (ndc < odc) {
        // A device was removed
        for (int i = odc - 1; i >= 0 ; --i) {
            QByteArray currId = deviceList[i].nameId;
            bool b_found = false;
            for (int k = ndc - 1; k >= 0 ; --k) {
                if (currId == newDevices[k].nameId) {
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
 * Update the current list of active devices.
 */
void DeviceManager::updateDeviceList()
{
#ifdef HAVE_LIBV4L2
    // Lists for capture devices
    QList<Device> vcs, acs;
    int i;

    qDebug() << Q_FUNC_INFO << "Probing for v4l devices";

    // Get the list of available v4l2 capture devices
    V4L2Support::scanDevices(vcs, acs);

    updateDeviceSublist(vcs, m_videoCaptureDeviceList);
    updateDeviceSublist(acs, m_audioCaptureDeviceList);
#endif

    // Lists for audio output devices
    QList<Device> aos;
    aos.append(Device("default"));

    // Get the list of available audio outputs
    libvlc_audio_output_t *p_ao_list = libvlc_audio_output_list_get(vlc_instance);
    if(!p_ao_list)
        qDebug() << "libvlc exception:" << libvlc_errmsg();
    libvlc_audio_output_t *p_start = p_ao_list;

    bool checkpulse = false;
#ifdef PHONON_PULSESUPPORT
    PulseSupport *pulse = PulseSupport::getInstance();
    checkpulse = pulse->isActive();
#endif
    bool haspulse = false;
    while (p_ao_list) {
        if (checkpulse && 0 == strcmp(p_ao_list->psz_name, "pulse")) {
            haspulse = true;
            break;
        }
        aos.append(Device(p_ao_list->psz_name));
        p_ao_list = p_ao_list->p_next;
    }
    libvlc_audio_output_list_release(p_start);


#ifdef PHONON_PULSESUPPORT
    if (haspulse)
        return;
    pulse->enable(false);
#endif

    updateDeviceSublist(aos, m_audioOutputDeviceList);
}

/**
* Return a list of name identifiers for audio capture devices.
*/
const QList<Device> DeviceManager::audioCaptureDevices() const
{
    return m_audioCaptureDeviceList;
}

/**
* Return a list of name identifiers for video capture devices.
*/
const QList<Device> DeviceManager::videoCaptureDevices() const
{
    return m_videoCaptureDeviceList;
}

/**
 * Return a list of name identifiers for audio output devices.
 */
const QList<Device> DeviceManager::audioOutputDevices() const
{
    return m_audioOutputDeviceList;
}

}
}

QT_END_NAMESPACE
