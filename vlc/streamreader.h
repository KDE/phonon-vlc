/*  This file is part of the KDE project.

Copyright (C) 2009 Nokia Corporation and/or its subsidiary(-ies).

This library is free software: you can redistribute it and/or modify
it under the terms of the GNU Lesser General Public License as published by
the Free Software Foundation, either version 2.1 or 3 of the License.

This library is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU Lesser General Public License for more details.

You should have received a copy of the GNU Lesser General Public License
along with this library.  If not, see <http://www.gnu.org/licenses/>.
*/

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


#ifndef PHONON_IODEVICEREADER_H
#define PHONON_IODEVICEREADER_H

#include <phonon/mediasource.h>
#include <phonon/streaminterface.h>

#include <QMutex>
#include <QMutexLocker>
#include <QWaitCondition>

QT_BEGIN_NAMESPACE

#ifndef QT_NO_PHONON_ABSTRACTMEDIASTREAM

namespace Phonon
{
class MediaSource;
namespace VLC
{

class MediaObject;

/** \brief Class for supporting custom data streams to the backend
 *
 * This class receives data from a Phonon MediaSource that is a stream.
 * When data is requested, it fetches it from the media source and passes it further.
 * MediaObject uses this class to pass stream data to libVLC.
 *
 * It implements Phonon::StreamInterface, necessary for the connection with an
 * Phonon::AbstractMediaStream owned by the Phonon::MediaSource. See the Phonon
 * documentation for details.
 *
 * There are callbacks implemented in streamhooks.cpp, for libVLC.
 */
class StreamReader : public Phonon::StreamInterface
{
public:

    StreamReader(const Phonon::MediaSource &source, MediaObject *parent)
        : m_pos(0)
        , m_size(0)
        , m_seekable(false)
        , m_mediaObject(parent) {
        connectToSource(source);
    }

    quint64 currentBufferSize() const {
        return m_buffer.size();
    }

    void writeData(const QByteArray &data);

    void setCurrentPos(qint64 pos);

    quint64 currentPos() const {
        return m_pos;
    }

    bool read(quint64 offset, int *length, char *buffer);

    void endOfData() {
        m_waitingForData.wakeAll();
    }

    void setStreamSize(qint64 newSize) {
        m_size = newSize;
    }

    qint64 streamSize() const {
        return m_size;
    }

    void setStreamSeekable(bool s) {
        m_seekable = s;
    }

    bool streamSeekable() const {
        return m_seekable;
    }

protected:
    QByteArray m_buffer;
    quint64 m_pos;
    quint64 m_size;
    bool m_seekable;
    QMutex m_mutex;
    QWaitCondition m_waitingForData;
    MediaObject *m_mediaObject;
};

}
}

#endif //QT_NO_PHONON_ABSTRACTMEDIASTREAM

QT_END_NAMESPACE

#endif
