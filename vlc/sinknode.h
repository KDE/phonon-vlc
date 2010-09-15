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

#include <QtCore/QObject>
#include <QtCore/QString>

#include "vlcloader.h"
#include "vlcmediaobject.h"

#ifndef PHONON_VLC_NO_EXPERIMENTAL
#include "experimental/avcapture.h"
#endif // PHONON_VLC_NO_EXPERIMENTAL

namespace Phonon
{
namespace VLC
{

class VLCMediaObject;
typedef VLCMediaObject PrivateMediaObject;

/** \brief The sink node is essentialy an output for a media object
 *
 * This class handles connections for the sink to a media object. It remembers
 * the media object and the libVLC media player associated with it.
 *
 * \see VLCMediaObject
 */
class SinkNode : public QObject
{
    Q_OBJECT

public:

    SinkNode(QObject *p_parent);
    virtual ~SinkNode();

    virtual void connectToMediaObject(PrivateMediaObject *mediaObject);

    virtual void disconnectFromMediaObject(PrivateMediaObject *mediaObject);

    #ifndef PHONON_VLC_NO_EXPERIMENTAL
    virtual void connectToAvCapture(Experimental::AvCapture *avCapture);

    virtual void disconnectFromAvCapture(Experimental::AvCapture *avCapture);
    #endif // PHONON_VLC_NO_EXPERIMENTAL

    virtual void addToMedia( libvlc_media_t * media );

protected:

    PrivateMediaObject *p_media_object;
    libvlc_media_player_t *p_vlc_player;

private slots:
    virtual void updateVolume();
};

}
} // Namespace Phonon::VLC

#endif // PHONON_VLC_SINKNODE_H
