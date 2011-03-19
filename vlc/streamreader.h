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

#ifndef PHONON_IODEVICEREADER_H
#define PHONON_IODEVICEREADER_H

#include <phonon/mediasource.h>
#include <phonon/streaminterface.h>

#include <stdint.h>

#include <QtCore/QMutex>
#include <QtCore/QWaitCondition>


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
    StreamReader(const Phonon::MediaSource &source, MediaObject *parent);
    ~StreamReader();

    void lock();
    void unlock();

    static int readCallback(void *data, const char *cookie,
                            int64_t *dts, int64_t *pts, unsigned *flags,
                            size_t *bufferSize, void **buffer);

    static int readDoneCallback(void *data, const char *cookie,
                                size_t bufferSize, void *buffer);

    static int seekCallback(void *data, const uint64_t pos);

    quint64 currentBufferSize() const;
    void writeData(const QByteArray &data);
    quint64 currentPos() const;
    void setCurrentPos(qint64 pos);

    /**
     * Requests data from this stream. The stream requests data from the
     * Phonon::MediaSource's abstract media stream with the needData() signal.
     * If the requested data is available, it is copied into the buffer.
     *
     * \param pos Position in the stream
     * \param length Length of the data requested
     * \param buffer A buffer to put the data
     */
    bool read(quint64 offset, int *length, char *buffer);

    void endOfData();
    void setStreamSize(qint64 newSize);
    qint64 streamSize() const;
    void setStreamSeekable(bool s);
    bool streamSeekable() const;

protected:
    QByteArray m_buffer;
    quint64 m_pos;
    quint64 m_size;
    bool m_eos;
    bool m_seekable;
    bool m_unlocked;
    QMutex m_mutex;
    QWaitCondition m_waitingForData;
    MediaObject *m_mediaObject;
};

}
}

#endif //QT_NO_PHONON_ABSTRACTMEDIASTREAM

QT_END_NAMESPACE

#endif
