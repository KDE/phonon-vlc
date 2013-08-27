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

#include "sinknode.h"

#include "utils/debug.h"
#include "mediaobject.h"
#include "mediaplayer.h"

namespace Phonon {
namespace VLC {

SinkNode::SinkNode()
    : m_player(0)
    , m_vlcPlayer(0)
{
}

SinkNode::~SinkNode()
{
    if (m_player) {
        disconnectPlayer(m_player);
    }
}

void SinkNode::connectPlayer(Player *player)
{
    if (m_player) {
        error() << Q_FUNC_INFO << "m_mediaObject already connected";
    }

    m_player = player;
    m_vlcPlayer = player->m_player;
    m_player->addSink(this);

    // ---> Global handling goes here! Above the derivee handle! <--- //

    handleConnectPlayer(player);
}

void SinkNode::disconnectPlayer(Player *player)
{
    handleDisconnectPlayer(player);

    // ---> Global handling goes here! Below the derivee handle! <--- //

    if (m_player != player) {
        error() << Q_FUNC_INFO << "SinkNode was not connected to mediaObject";
    }

    if (m_player) {
        m_player->removeSink(this);
    }

    m_player = 0;
    m_vlcPlayer = 0;
}

void SinkNode::addToMedia(Media *media)
{
    // ---> Global handling goes here! Above the derivee handle! <--- //

    handleAddToMedia(media);
}

} // namespace VLC
} // namespace Phonon
