/*
    Copyright (C) 2007-2008 Tanguy Krotoff <tkrotoff@gmail.com>
    Copyright (C) 2008 Lukas Durfina <lukas.durfina@gmail.com>
    Copyright (C) 2009 Fathi Boudra <fabo@kde.org>
    Copyright (C) 2009-2011 vlc-phonon AUTHORS
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

#include "backend.h"

#include <QtCore/QLatin1Literal>
#include <QtCore/QtPlugin>
#include <QtCore/QVariant>
#include <QtGui/QMessageBox>

#include <phonon/GlobalDescriptionContainer>
#ifdef PHONON_PULSESUPPORT
#include <phonon/pulsesupport.h>
#endif

#include <vlc/libvlc_version.h>

#include "audio/audiooutput.h"
#include "audio/audiodataoutput.h"
#include "devicemanager.h"
#include "effect.h"
#include "effectmanager.h"
#include "mediaobject.h"
#include "sinknode.h"
#include "utils/debug.h"
#include "utils/libvlc.h"
//#include "video/videodataoutput.h"
#include "video/videographicsobject.h"
#include "video/videowidget.h"

Q_EXPORT_PLUGIN2(phonon_vlc, Phonon::VLC::Backend)

namespace Phonon
{
namespace VLC
{

Backend *Backend::self;

Backend::Backend(QObject *parent, const QVariantList &)
    : QObject(parent)
    , m_deviceManager(0)
    , m_effectManager(0)
{
    self = this;

    // Backend information properties
    setProperty("identifier",     QLatin1String("phonon_vlc"));
    setProperty("backendName",    QLatin1String("VLC"));
    setProperty("backendComment", QLatin1String("VLC backend for Phonon"));
    setProperty("backendVersion", QLatin1String(PHONON_VLC_VERSION));
    setProperty("backendIcon",    QLatin1String("vlc"));
    setProperty("backendWebsite", QLatin1String("https://projects.kde.org/projects/kdesupport/phonon/phonon-vlc"));

    // Check if we should enable debug output
    int debugLevel = qgetenv("PHONON_VLC_DEBUG").toInt();
    if (debugLevel > 3) // 3 is maximum
        debugLevel = 3;
    Debug::setMinimumDebugLevel((Debug::DebugLevel)((int) Debug::DEBUG_NONE - 1 - debugLevel));

    // Actual libVLC initialisation
    if (LibVLC::init()) {
        debug() << "Using VLC version %0" << libvlc_get_version();
    } else {
#ifdef __GNUC__
#warning TODO - this error message is about as useful as a cooling unit in the arctic
#warning TODO - supposedly Phonon VLC should not make kabooom if libvlc fails to init, probably impossible though
#endif
        QMessageBox msg;
        msg.setIcon(QMessageBox::Critical);
        msg.setWindowTitle(tr("LibVLC Failed to Initialize"));
        msg.setText(tr("Phonon's VLC backend failed to start."
                       "\n\n"
                       "This usually means a problem with your VLC installation,"
                       " please report a bug with your distributor."));
        msg.setDetailedText(LibVLC::errorMessage());
        msg.exec();
        fatal() << "Phonon::VLC::vlcInit: Failed to initialize VLC";
    }

    m_deviceManager = new DeviceManager(this);
    m_effectManager = new EffectManager(this);

#ifdef PHONON_PULSESUPPORT
    // Initialise PulseAudio support
    PulseSupport *pulse = PulseSupport::getInstance();
    pulse->enable();
    connect(pulse, SIGNAL(objectDescriptionChanged(ObjectDescriptionType)),
            SIGNAL(objectDescriptionChanged(ObjectDescriptionType)));
#endif
}

Backend::~Backend()
{
    if (LibVLC::self)
        delete LibVLC::self;
    if (GlobalAudioChannels::self)
        delete GlobalAudioChannels::self;
    if (GlobalSubtitles::self)
        delete GlobalSubtitles::self;
#ifdef PHONON_PULSESUPPORT
    PulseSupport::shutdown();
#endif
}

QObject *Backend::createObject(BackendInterface::Class c, QObject *parent, const QList<QVariant> &args)
{
    if (!LibVLC::self || !libvlc)
        return 0;

    switch (c) {
    case MediaObjectClass:
        return new MediaObject(parent);
    case AudioOutputClass:
        return new AudioOutput(parent);
#ifdef __GNUC__
#warning VLC 2.0 broke sout/transcode/smem -> ADO
#endif
#if (LIBVLC_VERSION_INT < LIBVLC_VERSION(2, 0, 0, 0))
    case AudioDataOutputClass:
        return new AudioDataOutput(parent);
#endif
    case EffectClass:
        return new Effect(m_effectManager, args[0].toInt(), parent);
    case VideoGraphicsObjectClass:
        return createVideoGraphicsObject(parent);
    case VideoWidgetClass:
        return new VideoWidget(qobject_cast<QWidget *>(parent));
    }

    warning() << "Backend class" << c << "is not supported by Phonon VLC :(";
    return 0;
}

QStringList Backend::availableMimeTypes() const
{
    if (m_supportedMimeTypes.isEmpty()) {
        const_cast<Backend *>(this)->m_supportedMimeTypes
                << QLatin1String("application/ogg")
                << QLatin1String("application/x-ogg")
                << QLatin1String("application/vnd.rn-realmedia")
                << QLatin1String("application/x-annodex")
                << QLatin1String("application/x-flash-video")
                << QLatin1String("application/x-quicktimeplayer")
                << QLatin1String("application/x-extension-mp4")
                << QLatin1String("audio/168sv")
                << QLatin1String("audio/3gpp")
                << QLatin1String("audio/3gpp2")
                << QLatin1String("audio/8svx")
                << QLatin1String("audio/aiff")
                << QLatin1String("audio/amr")
                << QLatin1String("audio/amr-wb")
                << QLatin1String("audio/basic")
                << QLatin1String("audio/mp3")
                << QLatin1String("audio/mp4")
                << QLatin1String("audio/midi")
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
                << QLatin1String("audio/x-it")
                << QLatin1String("audio/x-m4a")
                << QLatin1String("audio/x-matroska")
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
                << QLatin1String("audio/x-s3m")
                << QLatin1String("audio/x-speex+ogg")
                << QLatin1String("audio/x-vorbis+ogg")
                << QLatin1String("audio/x-wav")
                << QLatin1String("audio/x-xm")
                << QLatin1String("image/ilbm")
                << QLatin1String("image/png")
                << QLatin1String("image/x-ilbm")
                << QLatin1String("image/x-png")
                << QLatin1String("video/3gpp")
                << QLatin1String("video/3gpp2")
                << QLatin1String("video/anim")
                << QLatin1String("video/avi")
                << QLatin1String("video/divx")
                << QLatin1String("video/flv")
                << QLatin1String("video/mkv")
                << QLatin1String("video/mng")
                << QLatin1String("video/mp4")
                << QLatin1String("video/mpeg")
                << QLatin1String("video/mpeg-system")
                << QLatin1String("video/mpg")
                << QLatin1String("video/msvideo")
                << QLatin1String("video/ogg")
                << QLatin1String("video/quicktime")
                << QLatin1String("video/webm")
                << QLatin1String("video/x-anim")
                << QLatin1String("video/x-flic")
                << QLatin1String("video/x-flv")
                << QLatin1String("video/x-matroska")
                << QLatin1String("video/x-mng")
                << QLatin1String("video/x-m4v")
                << QLatin1String("video/x-mpeg")
                << QLatin1String("video/x-mpeg-system")
                << QLatin1String("video/x-ms-asf")
                << QLatin1String("video/x-ms-wma")
                << QLatin1String("video/x-ms-wmv")
                << QLatin1String("video/x-ms-wvx")
                << QLatin1String("video/x-msvideo")
                << QLatin1String("video/x-quicktime")
                << QLatin1String("audio/x-flac")
                << QLatin1String("audio/x-ape");
    }
    return m_supportedMimeTypes;
}

QList<int> Backend::objectDescriptionIndexes(ObjectDescriptionType type) const
{
    QList<int> list;

    switch (type) {
    case Phonon::AudioChannelType: {
        list << GlobalAudioChannels::instance()->globalIndexes();
    }
    break;
    case Phonon::AudioOutputDeviceType:
    case Phonon::AudioCaptureDeviceType:
    case Phonon::VideoCaptureDeviceType: {
        return deviceManager()->deviceIds(type);
    }
    break;
    case Phonon::EffectType: {
        QList<EffectInfo *> effectList = effectManager()->effects();
        for (int eff = 0; eff < effectList.size(); ++eff) {
            list.append(eff);
        }
    }
    break;
    case Phonon::SubtitleType: {
        list << GlobalSubtitles::instance()->globalIndexes();
    }
    break;
    }

    return list;
}

QHash<QByteArray, QVariant> Backend::objectDescriptionProperties(ObjectDescriptionType type, int index) const
{
    QHash<QByteArray, QVariant> ret;

    switch (type) {
    case Phonon::AudioChannelType: {
        const AudioChannelDescription description = GlobalAudioChannels::instance()->fromIndex(index);
        ret.insert("name", description.name());
        ret.insert("description", description.description());
    }
    break;
    case Phonon::AudioOutputDeviceType:
    case Phonon::AudioCaptureDeviceType:
    case Phonon::VideoCaptureDeviceType: {
        // Index should be unique, even for different categories
        return deviceManager()->deviceProperties(index);
    }
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
    case Phonon::SubtitleType: {
        const SubtitleDescription description = GlobalSubtitles::instance()->fromIndex(index);
        ret.insert("name", description.name());
        ret.insert("description", description.description());
        ret.insert("type", description.property("type"));
    }
    break;
    }

    return ret;
}

bool Backend::startConnectionChange(QSet<QObject *> objects)
{
    //FIXME
    foreach(QObject * object, objects) {
        debug() << "Object:" << object->metaObject()->className();
    }

    // There is nothing we can do but hope the connection changes will not take too long
    // so that buffers would underrun
    // But we should be pretty safe the way xine works by not doing anything here.
    return true;
}

bool Backend::connectNodes(QObject *source, QObject *sink)
{
    debug() << "Backend connected" << source->metaObject()->className() << "to" << sink->metaObject()->className();

    SinkNode *sinkNode = dynamic_cast<SinkNode *>(sink);
    if (sinkNode) {
        MediaObject *mediaObject = qobject_cast<MediaObject *>(source);
        if (mediaObject) {
            // Connect the SinkNode to a MediaObject
            sinkNode->connectToMediaObject(mediaObject);
            return true;
        }

        /*
        Effect *effect = qobject_cast<Effect *>(source);
        if (effect) {
            // FIXME connect the effect
            return true;
        }
        */
    }

    warning() << "Linking" << source->metaObject()->className() << "to" << sink->metaObject()->className() << "failed";

    return false;
}

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

bool Backend::endConnectionChange(QSet<QObject *> objects)
{
    foreach(QObject *object, objects) {
        debug() << "Object:" << object->metaObject()->className();
    }
    return true;
}

DeviceManager *Backend::deviceManager() const
{
    return m_deviceManager;
}

EffectManager *Backend::effectManager() const
{
    return m_effectManager;
}

} // namespace VLC
} // namespace Phonon
