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

#include <phonon/audiooutputinterface.h>

#include <QtCore/QObject>

QT_BEGIN_NAMESPACE

namespace Phonon
{
namespace VLC {

class Backend;
class DeviceManager;
class AbstractRenderer;
class VideoWidget;

class DeviceInfo
{
public:
    enum Class {
        Unknown,
        Pulse,
        V4L2
    };

    enum Capability {
        None            = 0x0000,
        AudioOutput     = 0x0001,
        AudioCapture    = 0x0002,
        VideoCapture    = 0x0004
    };
public:
    DeviceInfo(const QByteArray& deviceId, const QByteArray& hwId = "");
    const QByteArray deviceClassString() const;

    int id;
    QByteArray nameId;
    QByteArray description;
    QByteArray hwId;
    Class deviceClass;
    quint16 capabilities;
};

class DeviceManager : public QObject
{
    Q_OBJECT

public:
    DeviceManager(Backend *parent);
    virtual ~DeviceManager();
    const QList<DeviceInfo> audioCaptureDevices() const;
    const QList<DeviceInfo> videoCaptureDevices() const;
    const QList<DeviceInfo> audioOutputDevices() const;
    int deviceId(const QByteArray &vlcId) const;
    QByteArray deviceDescription(int id) const;

signals:
    void deviceAdded(int);
    void deviceRemoved(int);

public slots:
    void updateDeviceList();

private:
    bool canOpenDevice() const;
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
