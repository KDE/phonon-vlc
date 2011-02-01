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

namespace Phonon
{
namespace VLC
{

SinkNode::SinkNode(QObject *p_parent)
    : QObject(p_parent)
    , m_mediaObject(0)
    , m_player(0)
{
}

SinkNode::~SinkNode()
{
}

/**
 * Associates the sink node to the provided media object. The m_mediaObject and m_vlcPlayer
 * attributes are set, and the sink is added to the media object's sinks.
 *
 * \param mediaObject A VLCMediaObject to connect to.
 *
 * \see disconnectFromMediaObject()
 */
void SinkNode::connectToMediaObject(MediaObject *mediaObject)
{
    if (m_mediaObject) {
        qCritical() << __FUNCTION__ << "m_mediaObject already connected";
    }

    m_mediaObject = mediaObject;
    m_player = mediaObject->m_player;
    connect(m_mediaObject, SIGNAL(playbackCommenced()), this, SLOT(updateVolume()));
    m_mediaObject->addSink(this);
}

/**
 * Removes this sink from the specified media object's sinks.
 *
 * \param mediaObject The media object to disconnect from
 *
 * \see connectToMediaObject()
 */
void SinkNode::disconnectFromMediaObject(MediaObject *mediaObject)
{
    if (m_mediaObject != mediaObject) {
        qCritical() << __FUNCTION__ << "SinkNode was not connected to mediaObject";
    }

    if( m_mediaObject ){
        m_mediaObject->removeSink(this);
        disconnect(m_mediaObject, SIGNAL(playbackCommenced()), this, SLOT(updateVolume()));
    }
}

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
void SinkNode::connectToAvCapture(Experimental::AvCapture *avCapture)
{
    connectToMediaObject(avCapture->audioMediaObject());
    connectToMediaObject(avCapture->videoMediaObject());
}

/**
 * Removes this sink from any of the AvCapture's media objects. If connectToAvCapture() is
 * reimplemented in a child class, this method should also be reimplemented.
 *
 * \param avCapture An AvCapture to disconnect from
 *
 * \see connectToAvCapture()
 */
void SinkNode::disconnectFromAvCapture(Experimental::AvCapture *avCapture)
{
    disconnectFromMediaObject(avCapture->audioMediaObject());
    disconnectFromMediaObject(avCapture->videoMediaObject());
}
#endif // PHONON_VLC_NO_EXPERIMENTAL

/**
 * Does nothing. To be reimplemented in child classes.
 */
void SinkNode::updateVolume()
{
}

/**
 * Does nothing. To be reimplemented in child classes.
 */
void SinkNode::addToMedia(libvlc_media_t *media)
{
}

}
} // Namespace Phonon::VLC_MPlayer
