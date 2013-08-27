/*
    Copyright (C) 2007-2008 Tanguy Krotoff <tkrotoff@gmail.com>
    Copyright (C) 2008 Lukas Durfina <lukas.durfina@gmail.com>
    Copyright (C) 2009 Fathi Boudra <fabo@kde.org>
    Copyright (C) 2009-2011 vlc-phonon AUTHORS
    Copyright (C) 2010-2011 Harald Sitter <sitter@kde.org>

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

#ifndef PHONON_VLC_AUDIOOUTPUT_H
#define PHONON_VLC_AUDIOOUTPUT_H

#include <QtCore/QObject>

#include <phonon/audiooutputinterface.h>

#include "sinknode.h"

namespace Phonon {
namespace VLC {

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
    explicit AudioOutput(QObject *parent);
    ~AudioOutput();

    /** \reimp */
    void handleConnectPlayer(Player *mediaObject);

    /** \reimp */
    void handleAddToMedia(Media *media);

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
     * Sets the current output device for this audio output.
     *
     * \param device The device to set; it should be valid and contain an usable deviceAccessList property
     * \return \c true if succeeded, or no change was made
     * \return \c false if failed
     */
    bool setOutputDevice(const AudioOutputDevice &newDevice);
#if (PHONON_VERSION >= PHONON_VERSION_CHECK(4, 6, 50))
    void setStreamUuid(QString uuid);
#endif

signals:
    void volumeChanged(qreal volume);
    void audioDeviceFailed();

private slots:
    /**
     * Sets the volume to m_volume.
     */
    void applyVolume();

private:
    /**
     * We can only really set the output device once we have a libvlc_media_player, which comes
     * from our SinkNode.
     */
    void setOutputDeviceImplementation();

    qreal m_volume;
    AudioOutputDevice m_device;
    QString m_streamUuid;
};

} // namespace VLC
} // namespace Phonon

#endif // PHONON_VLC_AUDIOOUTPUT_H
