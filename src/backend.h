/*
    Copyright (C) 2007-2008 Tanguy Krotoff <tkrotoff@gmail.com>
    Copyright (C) 2008 Lukas Durfina <lukas.durfina@gmail.com>
    Copyright (C) 2009 Fathi Boudra <fabo@kde.org>
    Copyright (C) 2009-2011 vlc-phonon AUTHORS <kde-multimedia@kde.org>
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

#ifndef Phonon_VLC_BACKEND_H
#define Phonon_VLC_BACKEND_H

#include <QtCore/QStringList>

#include <phonon/objectdescription.h>
#include <phonon/backendinterface.h>

class LibVLC;

namespace Phonon
{
namespace VLC
{
class DeviceManager;
class EffectManager;

/** \brief Backend class for Phonon-VLC.
 *
 * This class provides the special objects created by the backend and information about
 * various things that the backend supports. An object of this class is the root for
 * the backend plugin.
 *
 * Phonon will request the backend to create objects of various classes, like MediaObject,
 * AudioOutput, VideoWidget, Effect. There are also methods to handle the connections between
 * these objects.
 *
 * This class also provides information about the devices and effects that the backend supports.
 * These are audio output devices, audio capture devices, video capture devices, effects.
 */

class Backend : public QObject, public BackendInterface
{
    Q_OBJECT
    Q_PLUGIN_METADATA(IID "org.kde.phonon.vlc" FILE "phonon-vlc.json")
    Q_INTERFACES(Phonon::BackendInterface)

public:
    /**
     * Instance. Since there is no backend instance without actual Backend object
     * this class behaves likes a singleton.
     */
    static Backend *self;

    /**
     * Constructs the backend. Sets the backend properties, fetches the debug level from the
     * environment, initializes libVLC, constructs the device and effect managers, initializes
     * PulseAudio support.
     *
     * \param parent A parent object for the backend (passed to the QObject constructor)
     */
    explicit Backend(QObject *parent = 0, const QVariantList & = QVariantList());
    virtual ~Backend();

    /// \return The device manager that is associated with this backend object
    DeviceManager *deviceManager() const;

    /// \return The effect manager that is associated with this backend object.
    EffectManager *effectManager() const;

    /**
     * Creates a backend object of the desired class and with the desired parent. Extra arguments can be provided.
     *
     * \param c The class of object that is to be created
     * \param parent The object that will be the parent of the new object
     * \param args Optional arguments for the object creation
     * \return The desired object or NULL if the class is not implemented.
     */
    QObject *createObject(BackendInterface::Class, QObject *parent, const QList<QVariant> &args);

    QList<AudioOutputDevice> audioOutputDevices() const Q_DECL_OVERRIDE Q_DECL_FINAL;
    QList<AudioCaptureDevice> audioCaptureDevices() const Q_DECL_OVERRIDE Q_DECL_FINAL;
    QList<VideoCaptureDevice> videoCaptureDevices() const Q_DECL_OVERRIDE Q_DECL_FINAL;

Q_SIGNALS:
    void audioOutputDevicesChanged();
    void audioCaptureDevicesChanged();
    void videoCaptureDevicesChanged();

private:
    DeviceManager *m_deviceManager;
    EffectManager *m_effectManager;
};

} // namespace VLC
} // namespace Phonon

#endif // Phonon_VLC_BACKEND_H
