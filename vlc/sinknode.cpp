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

#include "mediaobject.h"

#ifndef PHONON_VLC_NO_EXPERIMENTAL
#include "experimental/avcapture.h"
#endif // PHONON_VLC_NO_EXPERIMENTAL

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
        qCritical() << __FUNCTION__ << "m_mediaObject already connected";
    }

    m_mediaObject = mediaObject;
    m_player = mediaObject->m_player;
    m_mediaObject->addSink(this);
}

void SinkNode::disconnectFromMediaObject(MediaObject *mediaObject)
{
    if (m_mediaObject != mediaObject) {
        qCritical() << __FUNCTION__ << "SinkNode was not connected to mediaObject";
    }

    if (m_mediaObject) {
        m_mediaObject->removeSink(this);
    }
}

void SinkNode::addToMedia(libvlc_media_t *media)
{
    Q_UNUSED(media);
}

#ifndef PHONON_VLC_NO_EXPERIMENTAL
void SinkNode::connectToAvCapture(Experimental::AvCapture *avCapture)
{
    connectToMediaObject(avCapture->audioMediaObject());
    connectToMediaObject(avCapture->videoMediaObject());
}

void SinkNode::disconnectFromAvCapture(Experimental::AvCapture *avCapture)
{
    disconnectFromMediaObject(avCapture->audioMediaObject());
    disconnectFromMediaObject(avCapture->videoMediaObject());
}
#endif // PHONON_VLC_NO_EXPERIMENTAL

}
} // Namespace Phonon::VLC
