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

#include "sinknode.h"

#include <phonon/audiooutputinterface.h>

namespace Phonon
{
namespace VLC {
class Backend;

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
class AudioOutput : public SinkNode, public AudioOutputInterface
{
    Q_OBJECT
    Q_INTERFACES(Phonon::AudioOutputInterface)

public:

    AudioOutput(Backend *p_back, QObject * p_parent);
    ~AudioOutput();

    #ifdef PHONON_VLC_EXPERIMENTAL
    void connectToAvCapture(Experimental::AvCapture *avCapture);
    void disconnectFromAvCapture(Experimental::AvCapture *avCapture);
    #endif // PHONON_VLC_EXPERIMENTAL

    qreal volume() const;
    void setVolume(qreal volume);

    int outputDevice() const;
    bool setOutputDevice(int);
#if (PHONON_VERSION >= PHONON_VERSION_CHECK(4, 2, 0))
    bool setOutputDevice(const AudioOutputDevice & device);
#endif

signals:

    void volumeChanged(qreal volume);
    void audioDeviceFailed();

private slots:
    void updateVolume();

private:

    qreal f_volume;
    int i_device;
    Backend *p_backend;

};

}
} // Namespace Phonon::VLC

#endif // PHONON_VLC_AUDIOOUTPUT_H
