/*

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

#include "mediaobject.h"
#include "streamreader.h"

#include <phonon/streaminterface.h>

QT_BEGIN_NAMESPACE
#ifndef QT_NO_PHONON_ABSTRACTMEDIASTREAM
namespace Phonon
{
namespace VLC
{
/**
 * Requests data from this stream. The stream requests data from the
 * Phonon::MediaSource's abstract media stream with the needData() signal.
 * If the requested data is available, it is copied into the buffer.
 *
 * \param pos Position in the stream
 * \param length Length of the data requested
 * \param buffer A buffer to put the data
 */
bool StreamReader::read(quint64 pos, int *length, char *buffer)
{
    QMutexLocker lock(&m_mutex);
    bool ret = true;

    if (currentPos() != pos) {
        if (!streamSeekable()) {
            return false;
        }
        setCurrentPos(pos);
    }

    if (m_buffer.capacity() < *length) {
        m_buffer.reserve(*length);
    }

    while (currentBufferSize() < *length) {
        int oldSize = currentBufferSize();
        needData();
        m_waitingForData.wait(&m_mutex);

        if (oldSize == currentBufferSize()) {
            // We didn't get any more data
            *length = oldSize;
            ret = false;
        }
    }

    qMemCopy(buffer, m_buffer.data(), *length);

    // trim the buffer by the amount read
    m_buffer = m_buffer.mid(*length);
    m_pos += *length;

    return ret;
}

void StreamReader::writeData(const QByteArray &data)
{
    QMutexLocker lock(&m_mutex);
    m_buffer += data;

    m_waitingForData.wakeAll();

    if (m_mediaObject->state() != Phonon::BufferingState
        && m_mediaObject->state() != Phonon::LoadingState)
        enoughData();
}

void StreamReader::setCurrentPos(qint64 pos)
{
    QMutexLocker lock(&m_mutex);
    m_pos = pos;
    m_buffer.clear(); // Not optimal, but meh

    // Do not touch m_size here, it reflects the size of the stream not the size of the buffer,
    // and generally seeking does not change the size!

    seekStream(pos);
}

}// namespace VLC
}// namespace Phonon
#endif //QT_NO_PHONON_ABSTRACTMEDIASTREAM

QT_END_NAMESPACE
