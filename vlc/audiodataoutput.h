/*****************************************************************************
 * libVLC backend for the Phonon library                                     *
 *                                                                           *
 * Copyright (C) 2006 Matthias Kretz <kretz@kde.org>
 * Copyright (C) 2009 Martin Sandsmark <sandsmark@samfundet.no>
 * Copyright (C) 2010 Ben Cooksley <sourtooth@gmail.com>
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

#ifndef Phonon_VLC_AUDIODATAOUTPUT_H
#define Phonon_VLC_AUDIODATAOUTPUT_H

#include <QtCore/QObject>
#include <phonon/audiodataoutputinterface.h>
#include "sinknode.h"

#include <QtCore/QMutex>

#include <phonon/audiodataoutput.h>

#include "backend.h"

//Allow PRId64 to be defined:
#define __STDC_FORMAT_MACROS
#include <inttypes.h>

QT_BEGIN_HEADER
QT_BEGIN_NAMESPACE

namespace Phonon
{
namespace VLC
{
/** \brief Implementation for AudioDataOutput using libVLC
 *
 * This class makes the capture of raw audio data possible. It sets special options
 * for the libVLC Media Object when connecting to it, and then captures libVLC events
 * to get the audio data and send it further with the dataReady() signal.
 *
 * As a sink node, it can be connected to media objects.
 *
 * The frontend Phonon::AudioDataOutput object is unused.
 *
 * See the Phonon documentation for details.
 *
 * \see AudioOutput
 * \see SinkNode
 *
 * \author Martin Sandsmark <sandsmark@samfundet.no>
 */
class AudioDataOutput : public QObject, public SinkNode, public AudioDataOutputInterface
{
    Q_OBJECT
    Q_INTERFACES(Phonon::AudioDataOutputInterface)

public:
    AudioDataOutput(QObject *parent);
    ~AudioDataOutput();

#ifndef PHONON_VLC_NO_EXPERIMENTAL
    void connectToAvCapture(Experimental::AvCapture *avCapture);
    void disconnectFromAvCapture(Experimental::AvCapture *avCapture);
#endif//PHONON_VLC_NO_EXPERIMENTAL

public Q_SLOTS:
    int dataSize() const;
    int sampleRate() const;
    void setDataSize(int size);
    void addToMedia( libvlc_media_t * media );

public:
    Phonon::AudioDataOutput *frontendObject() const {
        return m_frontend;
    }
    void setFrontendObject(Phonon::AudioDataOutput *frontend) {
        m_frontend = frontend;
    }

signals:
    void dataReady(const QMap<Phonon::AudioDataOutput::Channel, QVector<qint16> > &data);
    void dataReady(const QMap<Phonon::AudioDataOutput::Channel, QVector<float> > &data);
    void endOfMedia(int remainingSamples);
    void sampleReadDone();

private Q_SLOTS:
    void sendData();

private:
    static void lock(AudioDataOutput *cw, quint8 **pcm_buffer , quint32 size);
    static void unlock(AudioDataOutput *cw, quint8 *pcm_buffer,
                       quint32 channels, quint32 rate,
                       quint32 nb_samples, quint32 bits_per_sample,
                       quint32 size, qint64 pts);

    int m_dataSize;
    int m_sampleRate;
    Phonon::AudioDataOutput *m_frontend;

    QMutex m_locker;
    int m_channel_count;
    unsigned char *m_buffer;
    QVector<qint16> m_channel_samples[6];
    QList<Phonon::AudioDataOutput::Channel> m_channels;
};
}
} //namespace Phonon::VLC

QT_END_NAMESPACE
QT_END_HEADER

// vim: sw=4 ts=4 tw=80
#endif // Phonon_VLC_AUDIODATAOUTPUT_H
