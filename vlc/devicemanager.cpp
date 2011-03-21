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

#ifdef PHONON_PULSESUPPORT
#  include <phonon/pulsesupport.h>
#endif

#include <vlc/vlc.h>

#include "backend.h"
#include "libvlc.h"

QT_BEGIN_NAMESPACE

namespace Phonon
{
namespace VLC
{

DeviceInfo::DeviceInfo( const QByteArray& name, const QString& description, bool isAdvanced)
{
    // Get an id
    static int counter = 0;
    id = counter++;

    // Get name and description for the device
    this->name = name;
    this->description = description;
    this->isAdvanced = isAdvanced;
    capabilities = None;
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

void DeviceManager::updateDeviceList()
{
    // Lists for capture devices
    QList<DeviceInfo> devices, vcs, acs;
    int i;

    // Setup a list of available capture devices
    // TODO

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

    if (!LibVLC::self)
        return;

    // Get the list of available audio outputs
    libvlc_audio_output_t *p_ao_list = libvlc_audio_output_list_get(libvlc);
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
        if (checkpulse && strcmp(p_ao_list->psz_name, "pulse") == 0) {
            aos.last().isAdvanced = false;
            aos.last().accessList.append(DeviceAccess("pulse", "default"));
            haspulse = true;
            break;
        }

        aos.append(DeviceInfo(p_ao_list->psz_name, p_ao_list->psz_description, true));
        aos.last().accessList.append(DeviceAccess(p_ao_list->psz_name, QString()));
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
