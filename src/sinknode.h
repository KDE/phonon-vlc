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

#ifndef PHONON_VLC_SINKNODE_H
#define PHONON_VLC_SINKNODE_H

namespace Phonon
{

namespace VLC
{

namespace Experimental
{
    class AvCapture;
}

class Media;
class MediaObject;
class MediaPlayer;

/** \brief The sink node is essentialy an output for a media object
 *
 * This class handles connections for the sink to a media object. It remembers
 * the media object and the libVLC media player associated with it.
 *
 * \see MediaObject
 */
class SinkNode
{
public:
    SinkNode();
    virtual ~SinkNode();

    /**
     * Associates the sink node to the provided media object. The m_mediaObject and m_vlcPlayer
     * attributes are set, and the sink is added to the media object's sinks.
     *
     * \param mediaObject A VLCMediaObject to connect to.
     *
     * \see disconnectFromMediaObject()
     */
    virtual void connectToMediaObject(MediaObject *mediaObject);

    /**
     * Removes this sink from the specified media object's sinks.
     *
     * \param mediaObject The media object to disconnect from
     *
     * \see connectToMediaObject()
     */
    virtual void disconnectFromMediaObject(MediaObject *mediaObject);

    /**
     * Does nothing. To be reimplemented in child classes.
     */
    virtual void addToMedia(Media *media);

#ifndef PHONON_VLC_NO_EXPERIMENTAL
    /**
     * Associates the sink node with the compatible media object owned by the specified AvCapture.
     * The sink node knows whether it is compatible with video media or audio media. Here, the
     * connection is attempted with both video media and audio media. One of them probably will not
     * work. This method can be reimplemented in child classes to disable connecting to one or both of them.
     *
     * \param avCapture An AvCapture to connect to
     *
     * \see connectToMediaObject()
     * \see disconnectFromAvCapture()
     */
    virtual void connectToAvCapture(Experimental::AvCapture *avCapture);

    /**
     * Removes this sink from any of the AvCapture's media objects. If connectToAvCapture() is
     * reimplemented in a child class, this method should also be reimplemented.
     *
     * \param avCapture An AvCapture to disconnect from
     *
     * \see connectToAvCapture()
     */
    virtual void disconnectFromAvCapture(Experimental::AvCapture *avCapture);
#endif // PHONON_VLC_NO_EXPERIMENTAL

protected:
    MediaObject *m_mediaObject;
    MediaPlayer *m_player;
};

}
} // Namespace Phonon::VLC

#endif // PHONON_VLC_SINKNODE_H
