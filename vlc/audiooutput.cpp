/*****************************************************************************
 * VLC backend for the Phonon library                                        *
 * Copyright (C) 2007-2008 Tanguy Krotoff <tkrotoff@gmail.com>               *
 * Copyright (C) 2008 Lukas Durfina <lukas.durfina@gmail.com>                *
 * Copyright (C) 2009 Fathi Boudra <fabo@kde.org>                            *
 *                                                                           *
 * This program is free software; you can redistribute it and/or             *
 * modify it under the terms of the GNU Lesser General Public                *
 * License as published by the Free Software Foundation; either              *
 * version 3 of the License, or (at your option) any later version.          *
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

#include "audiooutput.h"
#include "devicemanager.h"
#include "backend.h"

#include "mediaobject.h"
#include "vlcmediaobject.h"

#include "vlcloader.h"

#ifdef PHONON_PULSESUPPORT
#  include <phonon/pulsesupport.h>
#endif

namespace Phonon
{
namespace VLC {

AudioOutput::AudioOutput(Backend *p_back, QObject * p_parent)
        : SinkNode(p_parent),
        f_volume(0.5),
        i_device(0),
        p_backend(p_back)
{
    p_media_object = 0;
}

AudioOutput::~AudioOutput()
{
}

qreal AudioOutput::volume() const
{
    return f_volume;
}

void AudioOutput::setVolume(qreal volume)
{
    if (p_vlc_player) {
        const int previous_volume = libvlc_audio_get_volume(p_vlc_player);
        libvlc_audio_set_volume(p_vlc_player, (int)(f_volume * 100));
        f_volume = volume;
        qDebug() << __FUNCTION__ << "Volume changed to - " << (int)(f_volume * 100) << " From " << previous_volume;
        emit volumeChanged(f_volume);
    }
}

int AudioOutput::outputDevice() const
{
    return i_device;
}

bool AudioOutput::setOutputDevice(int device)
{
    if (i_device == device)
        return true;

#ifdef PHONON_PULSESUPPORT
    if (PulseSupport::getInstance()->isActive()) {
        i_device = device;
        libvlc_audio_output_set(p_vlc_player, "pulse");
        qDebug() << "set aout " << "pulse";
        return true;
    }
#endif

    const QList<AudioDevice> deviceList = p_backend->deviceManager()->audioOutputDevices();
    if (device >= 0 && device < deviceList.size()) {

        if (!p_vlc_player)
            return false;
        i_device = device;
        const QByteArray deviceName = deviceList.at(device).vlcId;
        libvlc_audio_output_set(p_vlc_player, (char *) deviceList.at(device).vlcId.data());
        qDebug() << "set aout " << deviceList.at(device).vlcId.data();
//         if (deviceName == DEFAULT_ID) {
//             libvlc_audio_device_set(p_vlc_instance, DEFAULT, vlc_exception);
//             vlcExceptionRaised();
//         } else if (deviceName.startsWith(ALSA_ID)) {
//             qDebug() << "setting ALSA " << deviceList.at(device).hwId.data();
//             libvlc_audio_device_set(p_vlc_instance, ALSA, vlc_exception);
//             vlcExceptionRaised();
//             libvlc_audio_alsa_device_set(p_vlc_instance,
//                                          deviceList.at(device).hwId,
//                                          vlc_exception);
//             vlcExceptionRaised();
    }

    return true;
}

#if (PHONON_VERSION >= PHONON_VERSION_CHECK(4, 2, 0))
bool AudioOutput::setOutputDevice(const Phonon::AudioOutputDevice & device)
{
    return true;
}
#endif

void AudioOutput::updateVolume()
{
    if (p_vlc_player) {
        const int previous_volume = libvlc_audio_get_volume(p_vlc_player);
        libvlc_audio_set_volume(p_vlc_player, (int)(f_volume * 100));
        qDebug() << __FUNCTION__ << "Volume changed to - " << (int)(f_volume * 100) << " From " << previous_volume;
    }
}

}
} // Namespace Phonon::VLC
