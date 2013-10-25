/*
    Copyright (C) 2009-2010 vlc-phonon AUTHORS <kde-multimedia@kde.org>

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

#ifndef Phonon_VLC_DEVICEMANAGER_H
#define Phonon_VLC_DEVICEMANAGER_H

#include <QtCore/QObject>

#include <phonon/ObjectDescription>

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
    explicit DeviceInfo(const QString &name, bool isAdvanced = true);

    int id() const;
    const QString& name() const;
    const QString& description() const;
    bool isAdvanced() const;
    void setAdvanced(bool advanced);
    const DeviceAccessList& accessList() const;
    void addAccess(const DeviceAccess &access);
    quint16 capabilities() const;
    void setCapabilities(quint16 cap);

private:
    int m_id;
    QString m_name;
    QString m_description;
    bool m_isAdvanced;
    DeviceAccessList m_accessList;
    quint16 m_capabilities;
};

/** \brief Keeps track of audio/video devices that libVLC supports
 *
 * This class maintains a device list. Types of devices:
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
     * Constructs a device manager and immediately updates the devices.
     */
    explicit DeviceManager(Backend *parent);

    /**
     * Clears all the devices before destroying.
     */
    virtual ~DeviceManager();

    /**
     * \param type Only devices with a capability of this type are returned
     * The following are supported:
     * \li AudioOutputDeviceType
     * \li AudioCaptureDeviceType
     * \li VideoCaptureDeviceType
     *
     * \return A list of device identifiers that have capabilities that
     * match the desired type
     *
     * \note The capture devices are temporarily not implemented / removed
     */
    QList<int> deviceIds(ObjectDescriptionType type);

    /**
     * \param id The identifier for the device
     * \return Object description properties for a device
     */
    QHash<QByteArray, QVariant> deviceProperties(int id);

    QList<AudioOutputDevice> audioOutputDevies();

    /**
     * \param id The identifier for the device
     * \return Pointer to DeviceInfo, or NULL if the id is invalid
     */
    const DeviceInfo *device(int id) const;

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
    static bool listContainsDevice(const QList<DeviceInfo> &list, int id);

private:
    Backend *m_backend;
    QList<DeviceInfo> m_devices;
};
}
} // namespace Phonon::VLC

#endif // Phonon_VLC_DEVICEMANAGER_H
