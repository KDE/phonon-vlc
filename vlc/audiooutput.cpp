/*****************************************************************************
 * VLC backend for the Phonon library                                        *
 * Copyright (C) 2007-2008 Tanguy Krotoff <tkrotoff@gmail.com>               *
 * Copyright (C) 2008 Lukas Durfina <lukas.durfina@gmail.com>                *
 * Copyright (C) 2009 Fathi Boudra <fabo@kde.org>                            *
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

/**
 * Creates an AudioOutput with the given backend object. The volume is set to 1.0
 *
 * \param p_back Parent backend
 * \param p_parent A parent object
 */
AudioOutput::AudioOutput(Backend *p_back, QObject * p_parent)
        : SinkNode(p_parent),
        f_volume(1.0),
        i_device(0),
        p_backend(p_back)
{
    p_media_object = 0;
}

AudioOutput::~AudioOutput()
{
}

#ifndef PHONON_VLC_NO_EXPERIMENTAL
/**
 * Connects the AudioOutput to an AvCapture. connectToMediaObject() is called
 * only for the video media of the AvCapture.
 *
 * \see AvCapture
 */
void AudioOutput::connectToAvCapture(Experimental::AvCapture *avCapture)
{
    connectToMediaObject(avCapture->audioMediaObject());
}

/**
 * Disconnect the AudioOutput from the audio media of the AvCapture.
 *
 * \see connectToAvCapture()
 */
void AudioOutput::disconnectFromAvCapture(Experimental::AvCapture *avCapture)
{
    disconnectFromMediaObject(avCapture->audioMediaObject());
}
#endif // PHONON_VLC_NO_EXPERIMENTAL

/**
 * \return The current volume for this audio output.
 */
qreal AudioOutput::volume() const
{
    return f_volume;
}

/**
 * Sets the volume of the audio output. See the Phonon::AudioOutputInterface::setVolume() documentation
 * for details.
 */
void AudioOutput::setVolume(qreal volume)
{
    if (p_vlc_player) {
        const int previous_volume = libvlc_audio_get_volume(p_vlc_player);
        f_volume = volume;
        libvlc_audio_set_volume(p_vlc_player, (int)(f_volume * 50));
        qDebug() << __FUNCTION__ << "Volume changed to - " << (int)(f_volume * 100) << " From " << previous_volume;
        emit volumeChanged(f_volume);
    }
}

/**
 * \return The index of the current audio output device from the list obtained from the backend object.
 */
int AudioOutput::outputDevice() const
{
    return i_device;
}

/**
 * Sets the current output device for this audio output. The validity of the device index
 * is verified before attempting to change the device.
 *
 * \param device The index of the device, obtained from the backend's audio device list
 * \return \c true if succeeded, or no change was made
 * \return \c false if failed
 */
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

    const QList<DeviceInfo> deviceList = p_backend->deviceManager()->audioOutputDevices();
    if (device >= 0 && device < deviceList.size()) {

        if (!p_vlc_player)
            return false;
        i_device = device;
        const QByteArray deviceName = deviceList.at(device).nameId;
        libvlc_audio_output_set(p_vlc_player, (char *) deviceList.at(device).nameId.data());
        qDebug() << "set aout " << deviceList.at(device).nameId.data();
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
/**
 * Does nothing.
 */
bool AudioOutput::setOutputDevice(const Phonon::AudioOutputDevice & device)
{
    return true;
}
#endif

/**
 * Sets the volume to f_volume.
 */
void AudioOutput::updateVolume()
{
    if (p_vlc_player) {
        const int previous_volume = libvlc_audio_get_volume(p_vlc_player);
        libvlc_audio_set_volume(p_vlc_player, (int)(f_volume * 50));
        qDebug() << __FUNCTION__ << "Volume changed to - " << (int)(f_volume * 50) << " From " << previous_volume;
    }
}

}
} // Namespace Phonon::VLC
