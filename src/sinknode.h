/*
    Copyright (C) 2013 Harald Sitter <sitter@kde.org>

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

#ifndef PHONON_VLC_SINKNODE_H
#define PHONON_VLC_SINKNODE_H

#include <QPointer>

namespace Phonon {
namespace VLC {

class Media;
class Player;
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

    void connectPlayer(Player *player);
    void disconnectPlayer(Player *player);

    /**
     * Does nothing. To be reimplemented in child classes.
     */
    void addToMedia(Media *media);

protected:
    /**
     * Handling function for derived classes.
     * \note This handle is executed *after* the global handle.
     *       Meaning the SinkNode base will be done handling the connect.
     * \see connectPlayer
     */
    virtual void handleConnectPlayer(Player *player) { Q_UNUSED(player); }

    /**
     * Handling function for derived classes.
     * \note This handle is executed *before* the global handle.
     *       Meaning the SinkNode base will continue handling the disconnect.
     * \see disconnectPlayer
     */
    virtual void handleDisconnectPlayer(Player *player) { Q_UNUSED(player); }

    /**
     * Handling function for derived classes.
     * \note This handle is executed *after* the global handle.
     *       Meaning the SinkNode base will be done handling the connect.
     * \see addToMedia
     */
    virtual void handleAddToMedia(Media *media) { Q_UNUSED(media); }

    /** Available while connected to a MediaObject (until disconnected) */
    Player *m_player;

    /** Available while connected to a MediaObject (until disconnected) */
    MediaPlayer *m_vlcPlayer;
};

} // namespace VLC
} // namespace Phonon

#endif // PHONON_VLC_SINKNODE_H
