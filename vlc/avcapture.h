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

#ifndef PHONON_VLC_AVCAPTURE_H
#define PHONON_VLC_AVCAPTURE_H

#include "mediaobject.h"

#include <phonon/avcaptureinterface.h>

namespace Phonon
{
namespace VLC
{

class AvCapture : public QObject, public Phonon::AvCaptureInterface
{
    Q_OBJECT
    Q_INTERFACES(Phonon::AvCaptureInterface)

    public:
        AvCapture(QObject *parent);
        ~AvCapture();

        State state() const;
        void start();
        void pause();
        void stop();

        AudioCaptureDevice audioCaptureDevice() const;
        VideoCaptureDevice videoCaptureDevice() const;
        void setAudioCaptureDevice(const AudioCaptureDevice &device);
        void setVideoCaptureDevice(const VideoCaptureDevice &device);

        MediaObject* audioMediaObject();
        MediaObject* videoMediaObject();

    signals:
        void stateChanged(State newState, State oldState);

    private:
        void setupStateChangedSignal();

    private:
        AudioCaptureDevice m_audioCaptureDevice;
        VideoCaptureDevice m_videoCaptureDevice;
        MediaObject m_audioMedia;
        MediaObject m_videoMedia;
        MediaObject *m_connectedMO;
};

} // VLC namespace
} // Phonon namespace

#endif // PHONON_VLC_AVCAPTURE_H
