/*****************************************************************************
 * libVLC backend for the Phonon library                                     *
 *                                                                           *
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

#ifndef Phonon_VLC_DEVICEMANAGER_H
#define Phonon_VLC_DEVICEMANAGER_H

#include <QtCore/QObject>

#include <Phonon/ObjectDescription>

QT_BEGIN_NAMESPACE

namespace Phonon
{
namespace VLC
{

class Backend;

/** \brief Container for information about devices supported by libVLC
 *
 * It includes a (hopefully unique) device identifier, a name identifier, a
 * description, a hardware identifier (may be a platform dependent device name),
 * and other relevant info.
 */
class DeviceInfo
{
public:
    enum Capability {
        None            = 0x0000,
        AudioOutput     = 0x0001,
        AudioCapture    = 0x0002,
        VideoCapture    = 0x0004
    };
public:
    /**
     * Constructs a device info object and sets it's device identifiers.
     */
    DeviceInfo(const QByteArray& name, const QString& description = "");

    int id;
    QByteArray name;
    QString description;
#ifndef PHONON_VLC_NO_EXPERIMENTAL
    DeviceAccessList accessList;
#endif // PHONON_VLC_NO_EXPERIMENTAL
    quint16 capabilities;
};

/** \brief Keeps track of audio/video devices that libVLC supports
 *
 * This class maintains a device list for each of the following device categories:
 * \li audio output devices
 * \li audio capture devices
 * \li video capture devices
 *
 * Methods are provided to retrieve information about these devices.
 *
 * \see EffectManager
 */
class DeviceManager : public QObject
{
    Q_OBJECT

public:
    /**
     * Constructs a device manager and immediately updates the device lists.
     */
    DeviceManager(Backend *parent);

    /**
     * Clears all the device lists before destroying.
     */
    virtual ~DeviceManager();

    /**
    * \return a list of name identifiers for audio capture devices.
    */
    const QList<DeviceInfo> audioCaptureDevices() const;

    /**
    * \return a list of name identifiers for video capture devices.
    */
    const QList<DeviceInfo> videoCaptureDevices() const;

    /**
     * \return a list of name identifiers for audio output devices.
     */
    const QList<DeviceInfo> audioOutputDevices() const;

    /**
     * Searches for the device in the device lists, by comparing it's name
     *
     * \param name Name identifier for the device to search for
     * \return A positive device id or -1 if device does not exist.
     */
    int deviceId(const QByteArray &vlcId) const;

    /**
     * Get a human-readable description from a device id.
     * \param i_id Device identifier
     */
    QString deviceDescription( int i_id) const;

signals:
    void deviceAdded(int);
    void deviceRemoved(int);

public slots:
    /**
     * Update the current list of active devices. It probes for audio output devices,
     * audio capture devices, video capture devices. The methods depend on the
     * device types.
     */
    void updateDeviceList();

private:
    /**
     * \return True if the device can be opened.
     */
    bool canOpenDevice() const;

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
    void updateDeviceSublist(const QList<DeviceInfo> &newDevices, QList<Phonon::VLC::DeviceInfo> &deviceList);

private:
    Backend *m_backend;
    QList<DeviceInfo> m_audioCaptureDeviceList;
    QList<DeviceInfo> m_videoCaptureDeviceList;
    QList<DeviceInfo> m_audioOutputDeviceList;
};
}
} // namespace Phonon::VLC

QT_END_NAMESPACE

#endif // Phonon_VLC_DEVICEMANAGER_H
