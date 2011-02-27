/*
    Copyright (C) 2007-2008 Tanguy Krotoff <tkrotoff@gmail.com>
    Copyright (C) 2008 Lukas Durfina <lukas.durfina@gmail.com>
    Copyright (C) 2009 Fathi Boudra <fabo@kde.org>
    Copyright (C) 2009-2011 vlc-phonon AUTHORS

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

#include "streamreader.h"

#include <phonon/streaminterface.h>

#include <QtCore/QMutexLocker>

#include "mediaobject.h"

QT_BEGIN_NAMESPACE
#ifndef QT_NO_PHONON_ABSTRACTMEDIASTREAM
namespace Phonon
{
namespace VLC
{

#define BLOCKSIZE 32768

StreamReader::StreamReader(const Phonon::MediaSource &source, MediaObject *parent)
    : m_pos(0)
    , m_size(0)
    , m_eos(false)
    , m_gotDataOnce(false)
    , m_seekable(false)
    , m_mediaObject(parent)
{
    connectToSource(source);
}

StreamReader::~StreamReader()
{
}

int StreamReader::readCallback(void *data, const char *cookie,
                               int64_t *dts, int64_t *pts, unsigned *flags,
                               size_t *bufferSize, void **buffer)
{
    Q_UNUSED(cookie);
    Q_UNUSED(dts);
    Q_UNUSED(pts);
    Q_UNUSED(flags);

    StreamReader *that = static_cast<StreamReader *>(data);
    size_t length = BLOCKSIZE;

    *buffer = new char[length];

    int size = length;
    bool ret = that->read(that->currentPos(), &size, static_cast<char*>(*buffer));

    *bufferSize = static_cast<size_t>(size);

    return ret ? 0 : -1;
}

int StreamReader::readDoneCallback(void *data, const char *cookie,
                                   size_t bufferSize, void *buffer)
{
    Q_UNUSED(data);
    Q_UNUSED(cookie);
    Q_UNUSED(bufferSize);
    delete static_cast<char *>(buffer);
    return 0;
}

int StreamReader::seekCallback(void *data, const uint64_t pos)
{
    StreamReader *that = static_cast<StreamReader *>(data);
    if (static_cast<int64_t>(pos) > that->streamSize()) {
        // attempt to seek past the end of our data.
        return -1;
    }

    that->setCurrentPos(pos);
    // this should return a true/false, but it doesn't, so assume success.

    return 0;
}

quint64 StreamReader::currentBufferSize() const
{
    return m_buffer.size();
}

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

    while (currentBufferSize() < static_cast<unsigned int>(*length) || !m_gotDataOnce) {
        quint64 oldSize = currentBufferSize();
        needData();

        m_waitingForData.wait(&m_mutex);

        if (oldSize == currentBufferSize() && m_gotDataOnce) {
            if (m_eos) {
                return false;
            }
            // We didn't get any more data
            *length = static_cast<int>(oldSize);
            // If we have some data to return, why tell to reader that we failed?
            // Remember that length argument is more like maxSize not requiredSize
            ret = *length > 0;
        } else if (!m_gotDataOnce && currentBufferSize() >= static_cast<unsigned int>(*length)) {
            m_gotDataOnce = true;
        }
    }

    if (m_mediaObject->state() != Phonon::BufferingState &&
        m_mediaObject->state() != Phonon::LoadingState) {
        enoughData();
    }

    qMemCopy(buffer, m_buffer.data(), *length);
    m_pos += *length;
    // trim the buffer by the amount read
    m_buffer = m_buffer.mid(*length);
    return ret;
}

void StreamReader::endOfData()
{
    m_eos = true;
    m_waitingForData.wakeAll();
}

void StreamReader::writeData(const QByteArray &data)
{
    QMutexLocker lock(&m_mutex);
    m_buffer.append(data);
    m_waitingForData.wakeAll();
}

quint64 StreamReader::currentPos() const
{
    return m_pos;
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

void StreamReader::setStreamSize(qint64 newSize)
{
    m_size = newSize;
}

qint64 StreamReader::streamSize() const
{
    return m_size;
}

void StreamReader::setStreamSeekable(bool s)
{
    m_seekable = s;
}

bool StreamReader::streamSeekable() const
{
    return m_seekable;
}

}// namespace VLC
}// namespace Phonon
#endif //QT_NO_PHONON_ABSTRACTMEDIASTREAM

QT_END_NAMESPACE
