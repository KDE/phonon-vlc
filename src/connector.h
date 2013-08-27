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

#ifndef PHONON_VLC_CONNECTOR_H
#define PHONON_VLC_CONNECTOR_H

namespace Phonon {
namespace VLC {

class Media;
class Player;
class MediaPlayer;

class Connector
{
public:
    Connector();
    virtual ~Connector();

    void connectPlayer(Player *player);
    void disconnectPlayer(Player *player);
    void addToMedia(Media *media);

protected:
    /**
     * Handling function for derived classes.
     * \note This handle is executed *after* the global handle.
     *       Meaning the SinkNode base will be done handling the connect.
     * \see connectPlayer
     */
    virtual void handleConnectPlayer(Player *player);

    /**
     * Handling function for derived classes.
     * \note This handle is executed *before* the global handle.
     *       Meaning the SinkNode base will continue handling the disconnect.
     * \see disconnectPlayer
     */
    virtual void handleDisconnectPlayer(Player *player);

    /**
     * Handling function for derived classes.
     * \note This handle is executed *after* the global handle.
     *       Meaning the SinkNode base will be done handling the connect.
     * \see addToMedia
     */
    virtual void handleAddToMedia(Media *media);

    /** Available while connected to a MediaObject (until disconnected) */
    Player *m_player;

    /** Available while connected to a MediaObject (until disconnected) */
    MediaPlayer *m_vlcPlayer;
};

} // namespace VLC
} // namespace Phonon

#endif // PHONON_VLC_CONNECTOR_H
