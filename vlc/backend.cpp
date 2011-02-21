/*****************************************************************************
 * libVLC backend for the Phonon library                                     *
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

#include "backend.h"

#include <QtCore/QCoreApplication>
#include <QtCore/QDir>
#include <QtCore/QFileInfo>
#include <QtCore/QLatin1Literal>
#include <QtCore/QLibrary>
#include <QtCore/QSet>
#include <QtCore/QSettings>
#include <QtCore/QStringBuilder>
#include <QtCore/QtPlugin>
#include <QtCore/QVariant>

#ifdef PHONON_PULSESUPPORT
#include <phonon/pulsesupport.h>
#endif

#include <vlc/vlc.h>

#include "audiooutput.h"
#include "audiodataoutput.h"
#include "devicemanager.h"
#include "effect.h"
#include "effectmanager.h"
#include "libvlc.h"
#include "mediaobject.h"
#include "sinknode.h"
//#include "videodataoutput.h"
#include "videowidget.h"

#ifndef PHONON_VLC_NO_EXPERIMENTAL
#include "experimental/avcapture.h"
#endif // PHONON_VLC_NO_EXPERIMENTAL

Q_EXPORT_PLUGIN2(phonon_vlc, Phonon::VLC::Backend)

namespace Phonon
{
namespace VLC
{

/**
 * Constructs the backend. Sets the backend properties, fetches the debug level from the
 * environment, initializes libVLC, constructs the device and effect managers, initializes
 * PulseAudio support.
 *
 * \param parent A parent object for the backend (passed to the QObject constructor)
 */
Backend::Backend(QObject *parent, const QVariantList &)
    : QObject(parent)
    , m_deviceManager(NULL)
    , m_effectManager(NULL)
{
    /* Backend information properties */
    setProperty("identifier",     QLatin1String("phonon_vlc"));
    setProperty("backendName",    QLatin1String("VLC"));
    setProperty("backendComment", QLatin1String("VLC backend for Phonon"));
    setProperty("backendVersion", QLatin1String(PHONON_VLC_VERSION));
    setProperty("backendIcon",    QLatin1String("vlc"));
    setProperty("backendWebsite", QLatin1String("https://projects.kde.org/projects/kdesupport/phonon/phonon-vlc"));

    // Check if we should enable debug output
    int debugLevel = qgetenv("PHONON_VLC_DEBUG").toInt();
    if (debugLevel > 3) { // 3 is maximum
        debugLevel = 3;
    }
    m_debugLevel = (DebugLevel)debugLevel;

#ifdef __GNUC__
#warning maybe have init called separately to enable more error handling??
#endif
    /* Actual libVLC initialisation */
    LibVLC(m_debugLevel);
    logMessage(QString("Using VLC version %0").arg(libvlc_get_version()));

    m_deviceManager = new DeviceManager(this);
    m_effectManager = new EffectManager(this);

#ifdef PHONON_PULSESUPPORT
    // Initialise PulseAudio support
    PulseSupport *pulse = PulseSupport::getInstance();
    pulse->enable();
    connect(pulse, SIGNAL(objectDescriptionChanged(ObjectDescriptionType)), SIGNAL(objectDescriptionChanged(ObjectDescriptionType)));
#endif
}

Backend::~Backend()
{
//    vlcRelease();
#ifdef PHONON_PULSESUPPORT
    PulseSupport::shutdown();
#endif
}

/**
 * Creates a backend object of the desired class and with the desired parent. Extra arguments can be provided.
 *
 * \param c The class of object that is to be created
 * \param parent The object that will be the parent of the new object
 * \param args Optional arguments for the object creation
 * \return The desired object or NULL if the class is not implemented.
 */
QObject *Backend::createObject(BackendInterface::Class c, QObject *parent, const QList<QVariant> &args)
{
#ifndef PHONON_VLC_NO_EXPERIMENTAL
    Phonon::Experimental::BackendInterface::Class cex = static_cast<Phonon::Experimental::BackendInterface::Class>(c);

    switch (cex) {
    case Phonon::Experimental::BackendInterface::AvCaptureClass:
        logMessage("createObject() : AvCapture created - WARNING: Experimental!");
        return new Experimental::AvCapture(parent);
    default:
        if ((quint32) cex >= (quint32) Phonon::Experimental::BackendInterface::VideoDataOutputClass)
            logMessage("createObject() - experimental : Backend object not available");
    }
#endif // PHONON_VLC_NO_EXPERIMENTAL

    switch (c) {
    case MediaObjectClass:
        return new MediaObject(parent);
    case VolumeFaderEffectClass:
        logMessage("createObject() : VolumeFaderEffect not implemented");
        break;
    case AudioOutputClass: {
        AudioOutput *ao = new AudioOutput(this, parent);
        m_audioOutputs.append(ao);
        return ao;
    }
    case AudioDataOutputClass:
        return new AudioDataOutput(this, parent);
        logMessage("createObject() : AudioDataOutput created - WARNING: POSSIBLY INCORRECTLY IMPLEMENTED");
        break;
    case VisualizationClass:
        logMessage("createObject() : Visualization not implemented");
        break;
    case VideoDataOutputClass:
//        return new Phonon::VLC::VideoDataOutput(this, parent);
        logMessage("createObject() : VideoDataOutput not implemented");
        break;
    case EffectClass:
        return new Effect(m_effectManager, args[0].toInt(), parent);
    case VideoWidgetClass:
        return new VideoWidget(qobject_cast<QWidget *>(parent));
    default:
        logMessage("createObject() : Backend object not available");
    }
    return 0;
}

bool Backend::supportsVideo() const
{
    return true;
}

bool Backend::supportsOSD() const
{
    return true;
}

bool Backend::supportsFourcc(quint32 fourcc) const
{
    return true;
}

bool Backend::supportsSubtitles() const
{
    return true;
}

QStringList Backend::availableMimeTypes() const
{
    if (m_supportedMimeTypes.isEmpty()) {
        const_cast<Backend *>(this)->m_supportedMimeTypes
                << QLatin1String("application/ogg")
                << QLatin1String("application/vnd.rn-realmedia")
                << QLatin1String("application/x-annodex")
                << QLatin1String("application/x-flash-video")
                << QLatin1String("application/x-quicktimeplayer")
                << QLatin1String("audio/168sv")
                << QLatin1String("audio/8svx")
                << QLatin1String("audio/aiff")
                << QLatin1String("audio/basic")
                << QLatin1String("audio/mp3")
                << QLatin1String("audio/mp4")
                << QLatin1String("audio/mpeg")
                << QLatin1String("audio/mpeg2")
                << QLatin1String("audio/mpeg3")
                << QLatin1String("audio/vnd.rn-realaudio")
                << QLatin1String("audio/vnd.rn-realmedia")
                << QLatin1String("audio/wav")
                << QLatin1String("audio/webm")
                << QLatin1String("audio/x-16sv")
                << QLatin1String("audio/x-8svx")
                << QLatin1String("audio/x-aiff")
                << QLatin1String("audio/x-basic")
                << QLatin1String("audio/x-m4a")
                << QLatin1String("audio/x-mp3")
                << QLatin1String("audio/x-mpeg")
                << QLatin1String("audio/x-mpeg2")
                << QLatin1String("audio/x-mpeg3")
                << QLatin1String("audio/x-mpegurl")
                << QLatin1String("audio/x-ms-wma")
                << QLatin1String("audio/x-ogg")
                << QLatin1String("audio/x-pn-aiff")
                << QLatin1String("audio/x-pn-au")
                << QLatin1String("audio/x-pn-realaudio-plugin")
                << QLatin1String("audio/x-pn-wav")
                << QLatin1String("audio/x-pn-windows-acm")
                << QLatin1String("audio/x-real-audio")
                << QLatin1String("audio/x-realaudio")
                << QLatin1String("audio/x-speex+ogg")
                << QLatin1String("audio/x-vorbis+ogg")
                << QLatin1String("audio/x-wav")
                << QLatin1String("image/ilbm")
                << QLatin1String("image/png")
                << QLatin1String("image/x-ilbm")
                << QLatin1String("image/x-png")
                << QLatin1String("video/anim")
                << QLatin1String("video/avi")
                << QLatin1String("video/mkv")
                << QLatin1String("video/mng")
                << QLatin1String("video/mp4")
                << QLatin1String("video/mpeg")
                << QLatin1String("video/mpg")
                << QLatin1String("video/msvideo")
                << QLatin1String("video/quicktime")
                << QLatin1String("video/webm")
                << QLatin1String("video/x-anim")
                << QLatin1String("video/x-flic")
                << QLatin1String("video/x-mng")
                << QLatin1String("video/x-mpeg")
                << QLatin1String("video/x-ms-asf")
                << QLatin1String("video/x-ms-wmv")
                << QLatin1String("video/x-msvideo")
                << QLatin1String("video/x-quicktime")
                << QLatin1String("audio/x-flac")
                << QLatin1String("audio/x-ape");
    }
    return m_supportedMimeTypes;
}

/**
 * Returns a list of indexes for the desired object types. It specifies a list of objects
 * of a particular category that the backend knows about. These indexes can be used with
 * objectDescriptionProperties() to get the properties of a particular object.
 *
 * \param type The type of objects for the list
 */
QList<int> Backend::objectDescriptionIndexes(ObjectDescriptionType type) const
{
    QList<int> list;
    QList<DeviceInfo> deviceList;
    int dev;

    switch (type) {
    case Phonon::AudioOutputDeviceType: {
        deviceList = deviceManager()->audioOutputDevices();
        for (dev = 0 ; dev < deviceList.size() ; ++dev)
            list.append(deviceList[dev].id);
    }
    break;
    case Phonon::AudioCaptureDeviceType: {
        deviceList = deviceManager()->audioCaptureDevices();
        for (dev = 0 ; dev < deviceList.size() ; ++dev)
            list.append(deviceList[dev].id);
    }
    break;
#ifndef PHONON_VLC_NO_EXPERIMENTAL
    case Phonon::VideoCaptureDeviceType: {
        deviceList = deviceManager()->videoCaptureDevices();
        for (dev = 0 ; dev < deviceList.size() ; ++dev)
            list.append(deviceList[dev].id);
    }
    break;
#endif // PHONON_VLC_NO_EXPERIMENTAL
    case Phonon::EffectType: {
        QList<EffectInfo *> effectList = effectManager()->effects();
        for (int eff = 0; eff < effectList.size(); ++eff) {
            list.append(eff);
        }
    }
    break;
    default:
        break;
    }

    return list;
}

/**
 * Returns a list of properties for a particular object of the desired category.
 *
 * \param type The type of object for the index
 * \param index The index for the object of the desired type
 * \return The property list. If the object is inexistent, an empty list is returned.
 */
QHash<QByteArray, QVariant> Backend::objectDescriptionProperties(ObjectDescriptionType type, int index) const
{
    QHash<QByteArray, QVariant> ret;
    QList<DeviceInfo> deviceList;

    switch (type) {
    case Phonon::AudioOutputDeviceType: {
        deviceList = deviceManager()->audioOutputDevices();
        if (index >= 0 && index < deviceList.size()) {
            ret.insert("name", deviceList[index].name);
            ret.insert("description", deviceList[index].description);
            ret.insert("icon", QLatin1String("audio-card"));
        }
    }
    break;
    case Phonon::AudioCaptureDeviceType: {
        deviceList = deviceManager()->audioCaptureDevices();
        if (index >= 0 && index < deviceList.size()) {
            ret.insert("name", deviceList[index].name);
            ret.insert("description", deviceList[index].description);
            ret.insert("icon", QLatin1String("audio-input-microphone"));
#ifndef PHONON_VLC_NO_EXPERIMENTAL
            ret.insert("deviceAccessList", QVariant::fromValue<Phonon::DeviceAccessList>(deviceList[index].accessList));
#endif // PHONON_VLC_NO_EXPERIMENTAL
            if (deviceList[index].capabilities & DeviceInfo::VideoCapture)
                ret.insert("hasvideo", true);
        }
    }
    break;
#ifndef PHONON_VLC_NO_EXPERIMENTAL
    case Phonon::VideoCaptureDeviceType: {
        deviceList = deviceManager()->videoCaptureDevices();
        if (index >= 0 && index < deviceList.size()) {
            ret.insert("name", deviceList[index].name);
            ret.insert("description", deviceList[index].description);
            ret.insert("icon", QLatin1String("camera-web"));
            ret.insert("deviceAccessList", QVariant::fromValue<Phonon::DeviceAccessList>(deviceList[index].accessList));
            if (deviceList[index].capabilities & DeviceInfo::AudioCapture)
                ret.insert("hasaudio", true);
        }
    }
#endif // PHONON_VLC_NO_EXPERIMENTAL
    break;
    case Phonon::EffectType: {
        QList<EffectInfo *> effectList = effectManager()->effects();
        if (index >= 0 && index <= effectList.size()) {
            const EffectInfo *effect = effectList[ index ];
            ret.insert("name", effect->name());
            ret.insert("description", effect->description());
            ret.insert("author", effect->author());
        } else {
            Q_ASSERT(1); // Since we use list position as ID, this should not happen
        }
    }
    break;
    default:
        break;
    }

    return ret;
}

/**
 * Called when a connection between nodes is about to be changed
 *
 * \param objects A set of objects that will be involved in the change
 */
bool Backend::startConnectionChange(QSet<QObject *> objects)
{
    //FIXME
    foreach(QObject * object, objects) {
        logMessage(QString("Object: %0").arg(object->metaObject()->className()));
    }

    // There is nothing we can do but hope the connection changes will not take too long
    // so that buffers would underrun
    // But we should be pretty safe the way xine works by not doing anything here.
    return true;
}

/**
 * Connects two media nodes. The sink is informed that it should connect itself to the source.
 *
 * \param source The source media node for the connection
 * \param sink The sink media node for the connection
 * \return True if the connection was successful
 */
bool Backend::connectNodes(QObject *source, QObject *sink)
{
    logMessage(QString("Backend connected %0 to %1")
               .arg(source->metaObject()->className())
               .arg(sink->metaObject()->className()));

    // Example:
    // source = Phonon::VLC_MPlayer::MediaObject
    // sink = Phonon::VLC_MPlayer::VideoWidget

    // Example:
    // source = Phonon::VLC_MPlayer::MediaObject
    // sink = Phonon::VLC_MPlayer::AudioOutput

    // Example:
    // source = Phonon::VLC_MPlayer::MediaObject
    // sink = Phonon::VLC_MPlayer::Effect

    // Example:
    // source = Phonon::VLC_MPlayer::Effect
    // sink = Phonon::VLC_MPlayer::AudioOutput

    // Example:
    // source = Phonon::Experimental::AvCapture
    // sink = Phonon::VLC::VideoWidget

    SinkNode *sinkNode = dynamic_cast<SinkNode *>(sink);
    if (sinkNode) {
        MediaObject *mediaObject = qobject_cast<MediaObject *>(source);
        if (mediaObject) {
            // Connect the SinkNode to a MediaObject
            sinkNode->connectToMediaObject(mediaObject);
            return true;
        }

#ifndef PHONON_VLC_NO_EXPERIMENTAL
        Experimental::AvCapture *avCapture = qobject_cast<Experimental::AvCapture *>(source);
        if (avCapture) {
            // Connect the SinkNode to a AvCapture
            sinkNode->connectToAvCapture(avCapture);
            return true;
        }
#endif // PHONON_VLC_NO_EXPERIMENTAL

        /*
        Effect *effect = qobject_cast<Effect *>(source);
        if (effect) {
            // FIXME connect the effect
            return true;
        }
        */
    }

    logMessage(QString("Linking %0 to %1 failed")
               .arg(source->metaObject()->className())
               .arg(sink->metaObject()->className()),
               Warning);

    return false;
}

/**
 * Disconnects two previously connected media nodes. It disconnects the sink node from the source node.
 *
 * \param source The source node for the disconnection
 * \param sink The sink node for the disconnection
 * \return True if the disconnection was successful
 */
bool Backend::disconnectNodes(QObject *source, QObject *sink)
{
    SinkNode *sinkNode = dynamic_cast<SinkNode *>(sink);
    if (sinkNode) {
        MediaObject *const mediaObject = qobject_cast<MediaObject *>(source);
        if (mediaObject) {
            // Disconnect the SinkNode from a MediaObject
            sinkNode->disconnectFromMediaObject(mediaObject);
            return true;
        }

#ifndef PHONON_VLC_NO_EXPERIMENTAL
        Experimental::AvCapture *avCapture = qobject_cast<Experimental::AvCapture *>(source);
        if (avCapture) {
            // Disconnect the SinkNode from a AvCapture
            sinkNode->disconnectFromAvCapture(avCapture);
            return true;
        }
#endif // PHONON_VLC_NO_EXPERIMENTAL

        /*
        Effect *effect = qobject_cast<Effect *>(source);
        if (effect) {
            // FIXME disconnect the effect
            return true;
        }
        */
    }

    return false;
}

/**
 * Called after a connection between nodes has been changed
 *
 * \param objects Nodes involved in the disconnection
 */
bool Backend::endConnectionChange(QSet<QObject *> objects)
{
    foreach(QObject * object, objects) {
        logMessage(QString("Object: %0").arg(object->metaObject()->className()));
    }

    return true;
}

/**
 * \return The device manager that is associated with this backend object
 */
DeviceManager *Backend::deviceManager() const
{
    return m_deviceManager;
}

/**
 * \return The effect manager that is associated with this backend object.
 */
EffectManager *Backend::effectManager() const
{
    return m_effectManager;
}

/**
 * Return a debuglevel that is determined by the
 * PHONON_VLC_DEBUG environment variable.
 *
 * \li Warning - important warnings
 * \li Info    - general info
 * \li Debug   - gives extra info
 *
 * \return The debug level.
 */
Backend::DebugLevel Backend::debugLevel() const
{
    return m_debugLevel;
}

/**
 * Print a conditional debug message based on the current debug level
 * If obj is provided, classname and objectname will be printed as well
 *
 * \param message Debug message
 * \param priority Priority of the debug message, see debugLevel()
 * \param obj Who gives the message, or some object relevant for the message
 */
void Backend::logMessage(const QString &message, int priority, QObject *obj) const
{
    if (debugLevel() > 0) {
        QString output;
        if (obj) {
            // Strip away namespace from className
            QString className(obj->metaObject()->className());
            const int nameLength = className.length() - className.lastIndexOf(':') - 1;
            className = className.right(nameLength);
            output.sprintf("%s %s (%s %p)", message.toLatin1().constData(),
                           obj->objectName().toLatin1().constData(),
                           className.toLatin1().constData(), obj);
        } else {
            output = message;
        }
        if (priority <= (int)debugLevel()) {
            qDebug() << QString("PVLC(%1): %2").arg(priority).arg(output);
        }
    }
}

}
} // Namespace Phonon::VLC
