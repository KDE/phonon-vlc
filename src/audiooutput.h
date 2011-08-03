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

#ifndef PHONON_VLC_AUDIOOUTPUT_H
#define PHONON_VLC_AUDIOOUTPUT_H

#include <QtCore/QObject>
#include <phonon/audiooutputinterface.h>
#include "sinknode.h"

namespace Phonon
{
namespace VLC
{

/** \brief AudioOutput implementation for Phonon-VLC
 *
 * This class is a SinkNode that implements the AudioOutputInterface from Phonon. It
 * supports setting the volume and the audio output device.
 *
 * There are signals for the change of the volume or for when an audio device failed.
 *
 * See the Phonon::AudioOutputInterface documentation for details.
 *
 * \see AudioDataOutput
 */
class AudioOutput : public QObject, public SinkNode, public AudioOutputInterface
{
    Q_OBJECT
    Q_INTERFACES(Phonon::AudioOutputInterface)

public:
    /**
     * Creates an AudioOutput with the given backend object. The volume is set to 1.0
     *
     * \param p_back Parent backend
     * \param p_parent A parent object
     */
    AudioOutput(QObject *parent);
    ~AudioOutput();

    /* Overload */
    virtual void connectToMediaObject(MediaObject *mediaObject);

    /* Overload */
    virtual void disconnectFromMediaObject(MediaObject *mediaObject);

#ifndef PHONON_VLC_NO_EXPERIMENTAL
    /**
     * Connects the AudioOutput to an AvCapture. connectToMediaObject() is called
     * only for the video media of the AvCapture.
     *
     * \see AvCapture
     */
    void connectToAvCapture(Experimental::AvCapture *avCapture);

    /**
     * Disconnect the AudioOutput from the audio media of the AvCapture.
     *
     * \see connectToAvCapture()
     */
    void disconnectFromAvCapture(Experimental::AvCapture *avCapture);
#endif // PHONON_VLC_NO_EXPERIMENTAL

    /**
     * \return The current volume for this audio output.
     */
    qreal volume() const;

    /**
     * Sets the volume of the audio output. See the Phonon::AudioOutputInterface::setVolume() documentation
     * for details.
     */
    void setVolume(qreal volume);

    /**
     * \return The index of the current audio output device from the list obtained from the backend object.
     */
    int outputDevice() const;

    /**
     * Sets the current output device for this audio output. The validity of the device index
     * is verified before attempting to change the device.
     *
     * \param device The index of the device, obtained from the backend's audio device list
     * \return \c true if succeeded, or no change was made
     * \return \c false if failed
     */
    bool setOutputDevice(int);

#if (PHONON_VERSION >= PHONON_VERSION_CHECK(4, 2, 0))
    /**
     * Does nothing.
     */
    bool setOutputDevice(const AudioOutputDevice &device);
#endif

signals:
    void volumeChanged(qreal volume);
    void audioDeviceFailed();

private slots:
    /**
     * Sets the volume to m_volume.
     */
    void updateVolume();

private:
    /**
     * We can only really set the output device once we have a libvlc_media_player, which comes
     * from our SinkNode.
     */
    void setAudioOutputDeviceImplementation();

    qreal m_volume;
    int m_deviceIndex;
};

}
} // Namespace Phonon::VLC

#endif // PHONON_VLC_AUDIOOUTPUT_H
