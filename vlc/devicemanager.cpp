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

/**
 * This class manages the list of currently active output devices.
 */

QT_BEGIN_NAMESPACE

namespace Phonon
{
namespace VLC
{

AudioDevice::AudioDevice(DeviceManager *manager, const QByteArray &deviceId, const QByteArray &hw_id)
{
    Q_UNUSED(manager)

    // Get an id
    static int counter = 0;
    id = counter++;
    // Get name from device
    if (vlcId == "default") {
        description = "Default audio device";
    } else {
        vlcId = deviceId;
        description = "";
    }
    hwId = hw_id;
}

DeviceManager::DeviceManager(Backend *parent)
    : QObject(parent)
    , m_backend(parent)
{
    updateDeviceList();
}

DeviceManager::~DeviceManager()
{
    m_audioDeviceList.clear();
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
    for (int i = 0 ; i < m_audioDeviceList.size() ; ++i) {
        if (m_audioDeviceList[i].vlcId == nameId) {
            return m_audioDeviceList[i].id;
        }
    }
    return -1;
}

/**
 * Get a human-readable description from a device id.
 */
QByteArray DeviceManager::deviceDescription(int i_id) const
{
    for (int i = 0 ; i < m_audioDeviceList.size() ; ++i) {
        if (m_audioDeviceList[i].id == i_id) {
            return m_audioDeviceList[i].description;
        }
    }
    return QByteArray();
}

/**
 * Update the current list of active devices.
 */
void DeviceManager::updateDeviceList()
{
    QList<QByteArray> list, list_hw;
    list.append("default");
    list_hw.append("");

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
            haspulse = true;
            break;
        }
        list.append(p_ao_list->psz_name);
        list_hw.append("");
        p_ao_list = p_ao_list->p_next;
    }
    libvlc_audio_output_list_release(p_start);


#ifdef PHONON_PULSESUPPORT
    if (haspulse) {
        return;
    }
    pulse->enable(false);
#endif

    for (int i = 0 ; i < list.size() ; ++i) {
        QByteArray nameId = list.at(i);
        QByteArray hwId = list_hw.at(i);
        if (deviceId(nameId) == -1) {
            // This is a new device, add it
            qDebug() << "add aout " << nameId.data();
            m_audioDeviceList.append(AudioDevice(this, nameId, hwId));
            emit deviceAdded(deviceId(nameId));
        }
    }
    if (list.size() < m_audioDeviceList.size()) {
        // A device was removed
        for (int i = m_audioDeviceList.size() - 1 ; i >= 0 ; --i) {
            QByteArray currId = m_audioDeviceList[i].vlcId;
            bool b_found = false;
            for (int k = list.size() - 1  ; k >= 0 ; --k) {
                if (currId == list[k]) {
                    b_found = true;
                    break;
                }
            }
            if (!b_found) {
                emit deviceRemoved(deviceId(currId));
                m_audioDeviceList.removeAt(i);
            }
        }
    }
}

/**
 * Return a list of hardware id.
 */
const QList<AudioDevice> DeviceManager::audioOutputDevices() const
{
    return m_audioDeviceList;
}

}
}

QT_END_NAMESPACE
