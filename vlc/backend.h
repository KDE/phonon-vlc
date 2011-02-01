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

#ifndef Phonon_VLC_BACKEND_H
#define Phonon_VLC_BACKEND_H

#include "devicemanager.h"
#include "audiooutput.h"

#include <phonon/objectdescription.h>
#include <phonon/backendinterface.h>

#ifndef PHONON_VLC_NO_EXPERIMENTAL
#include "phonon/experimental/backendinterface.h"
#endif // PHONON_VLC_NO_EXPERIMENTAL

#include <QtCore/QList>
#include <QtCore/QPointer>
#include <QtCore/QStringList>

struct libvlc_instance_t;

/**
 * VLC library instance global variable.
 */
extern libvlc_instance_t *vlc_instance;

namespace Phonon
{
namespace VLC
{
class AudioOutput;
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
    Q_INTERFACES(Phonon::BackendInterface)

public:

    enum DebugLevel {NoDebug, Warning, Info, Debug};
    explicit Backend(QObject *parent = 0, const QVariantList & = QVariantList());
    virtual ~Backend();

    DeviceManager *deviceManager() const;
    EffectManager *effectManager() const;

    QObject *createObject(BackendInterface::Class, QObject *parent, const QList<QVariant> &args);

    bool supportsVideo() const;
    bool supportsOSD() const;
    bool supportsFourcc(quint32 fourcc) const;
    bool supportsSubtitles() const;
    QStringList availableMimeTypes() const;

    QList<int> objectDescriptionIndexes(ObjectDescriptionType type) const;
    QHash<QByteArray, QVariant> objectDescriptionProperties(ObjectDescriptionType type, int index) const;

    bool startConnectionChange(QSet<QObject *>);
    bool connectNodes(QObject *, QObject *);
    bool disconnectNodes(QObject *, QObject *);
    bool endConnectionChange(QSet<QObject *>);

    DebugLevel debugLevel() const;

    void logMessage(const QString &message, int priority = 2, QObject *obj = 0) const;

Q_SIGNALS:
    void objectDescriptionChanged(ObjectDescriptionType);

private:
    /**
     * Get VLC path.
     *
     * @return the VLC path
     */
    QString vlcPath();

    /**
     * Unload VLC library.
     */
    void vlcUnload();

    /**
     * Initialize and launch VLC library.
     *
     * instance and global variables are initialized.
     *
     * @return VLC initialization result
     */
    bool vlcInit(int debugLevl = 0);

    /**
     * Stop VLC library.
     */
    void vlcRelease();

    mutable QStringList m_supportedMimeTypes;
    QList<QPointer<AudioOutput> > m_audioOutputs;

    DeviceManager *m_deviceManager;
    EffectManager *m_effectManager;
    DebugLevel m_debugLevel;
};

}
} // namespace Phonon::VLC

#endif // Phonon_VLC_BACKEND_H
