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

#include "connector.h"

#include "player.h"
#include "utils/debug.h"
#include "utils/mediaplayer.h"

namespace Phonon {
namespace VLC {

Connector::Connector()
    : m_player(0)
    , m_vlcPlayer(0)
{
}

Connector::~Connector()
{
    if (m_player)
        disconnectPlayer(m_player);
}

void Connector::connectPlayer(Player *player)
{
    if (m_player)
        pCritical() << Q_FUNC_INFO << "Already connected to a Player.";

    m_player = player;
    m_vlcPlayer = player->m_player;
    m_player->attach(this);

    // ---> Global handling goes here! ***Above*** the derivee handle! <--- //

    handleConnectPlayer(player);
}

void Connector::disconnectPlayer(Player *player)
{
    handleDisconnectPlayer(player);

    // ---> Global handling goes here! ***Below*** the derivee handle! <--- //

    if (m_player != player)
        pCritical() << Q_FUNC_INFO << "Was not connected to the Player it is being disconnected from.";

    if (m_player)
        m_player->remove(this);

    m_player = 0;
    m_vlcPlayer = 0;
}

void Connector::addToMedia(Media *media)
{
    // ---> Global handling goes here! ***Above*** the derivee handle! <--- //

    handleAddToMedia(media);
}

void Connector::handleConnectPlayer(Player *player)
{
    Q_UNUSED(player);
}

void Connector::handleDisconnectPlayer(Player *player)
{
    Q_UNUSED(player);
}

void Connector::handleAddToMedia(Media *media)
{
    Q_UNUSED(media);
}

} // namespace VLC
} // namespace Phonon
