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

#include "sinknode.h"

#include "utils/debug.h"
#include "mediaobject.h"
#include "mediaplayer.h"

namespace Phonon
{
namespace VLC
{

SinkNode::SinkNode()
    : m_mediaObject(0)
    , m_player(0)
{
}

SinkNode::~SinkNode()
{
}

void SinkNode::connectToMediaObject(MediaObject *mediaObject)
{
    if (m_mediaObject) {
        error() << Q_FUNC_INFO << "m_mediaObject already connected";
    }

    m_mediaObject = mediaObject;
    m_player = mediaObject->m_player;
    m_mediaObject->addSink(this);
}

void SinkNode::disconnectFromMediaObject(MediaObject *mediaObject)
{
    if (m_mediaObject != mediaObject) {
        error() << Q_FUNC_INFO << "SinkNode was not connected to mediaObject";
    }

    if (m_mediaObject) {
        m_mediaObject->removeSink(this);
    }

    m_player = 0;
}

void SinkNode::addToMedia(Media *media)
{
    Q_UNUSED(media);
}

}
} // Namespace Phonon::VLC
