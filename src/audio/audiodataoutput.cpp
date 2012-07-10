/*
    Copyright (C) 2006 Matthias Kretz <kretz@kde.org>
    Copyright (C) 2009 Martin Sandsmark <sandsmark@samfundet.no>
    Copyright (C) 2010 Ben Cooksley <sourtooth@gmail.com>
    Copyright (C) 2011 Harald Sitter <sitter@kde.org>

    This library is free software; you can redistribute it and/or
    modify it under the terms of the GNU Lesser General Public
    License as published by the Free Software Foundation; either
    version 2.1 of the License, or (at your option) version 3, or any
    later version accepted by the membership of KDE e.V. (or its
    successor approved by the membership of KDE e.V.), Nokia Corporation
    (or its successors, if any) and the KDE Free Qt Foundation, which shall
    act as a proxy defined in Section 6 of version 3 of the license.

    This library is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
    Lesser General Public License for more details.

    You should have received a copy of the GNU Lesser General Public
    License along with this library.  If not, see <http://www.gnu.org/licenses/>.
*/

#include "audiodataoutput.h"

#include "media.h"
#include "utils/debug.h"

namespace Phonon {
namespace VLC {

AudioDataOutput::AudioDataOutput(QObject *parent)
    : QObject(parent)
    , m_sampleRate(44100)
{
    m_sampleRate = 44100;
    connect(this, SIGNAL(sampleReadDone()), this, SLOT(sendData()));

    // Register channels
    m_channels.append(Phonon::AudioDataOutput::LeftChannel);
    m_channels.append(Phonon::AudioDataOutput::RightChannel);
    m_channels.append(Phonon::AudioDataOutput::CenterChannel);
    m_channels.append(Phonon::AudioDataOutput::LeftSurroundChannel);
    m_channels.append(Phonon::AudioDataOutput::RightSurroundChannel);
    m_channels.append(Phonon::AudioDataOutput::SubwooferChannel);
}

AudioDataOutput::~AudioDataOutput()
{
}

int AudioDataOutput::dataSize() const
{
    return m_dataSize;
}

int AudioDataOutput::sampleRate() const
{
    return m_sampleRate;
}

void AudioDataOutput::setDataSize(int size)
{
    m_dataSize = size;
}

void AudioDataOutput::addToMedia(Media *media)
{
    Q_UNUSED(media);
    setCallbacks(m_player);
}

void AudioDataOutput::lock(AudioDataOutput *cw, quint8 **pcm_buffer , quint32 size)
{
    cw->m_locker.lock();
    *pcm_buffer = new quint8[size];
}

void AudioDataOutput::unlock(AudioDataOutput *cw, quint8 *pcm_buffer,
                             quint32 channelCount, quint32 rate,
                             quint32 sampleCount, quint32 bits_per_sample,
                             quint32 size, qint64 pts)
{
    Q_UNUSED(size);
    Q_UNUSED(pts);

    // (bytesPerChannelPerSample * channels * read_samples) + (bytesPerChannelPerSample * read_channels)
    int bytesPerChannelPerSample = bits_per_sample / 8;
    cw->m_sampleRate = rate;
    cw->m_channelCount = channelCount;

    for (quint32 readSamples = 0; readSamples < sampleCount; ++readSamples) {
        // Prepare a sample buffer, and initialise it
        quint16 sampleBuffer[6];
        for (int initialised = 0; initialised < 6; ++initialised) {
            sampleBuffer[initialised] = 0;
        }

        int bufferPosition = (bytesPerChannelPerSample * channelCount * readSamples);

        for (quint32 readChannels = 0; readChannels < channelCount; ++readChannels) {
            quint32 complet = 0;
            for (int readBytes = 0; readBytes < bytesPerChannelPerSample; ++readBytes) {
                // Read from the pcm_buffer into the per channel internal buffer

                quint32 complet_temp = 0;
                complet_temp = pcm_buffer[bufferPosition];
                complet_temp <<=  (8 * readBytes);

                complet += complet_temp;
                ++bufferPosition;
            }

            sampleBuffer[readChannels] = complet;
        }

        if (channelCount == 1) {
            cw->m_channelSamples[1].append(qint16(sampleBuffer[0]));
        }

        for (quint32 readChannels = 0; readChannels < channelCount; ++readChannels) {
            cw->m_channelSamples[readChannels].append(qint16(sampleBuffer[readChannels]));
        }
        // Finished reading one sample
    }

    delete pcm_buffer;

    cw->m_locker.unlock();
    emit cw->sampleReadDone();
}

void AudioDataOutput::sendData()
{
    m_locker.lock();

    int chan_count = m_channelCount;
    if (m_channelCount == 1) {
        chan_count = 2;
    }

    while (m_channelSamples[0].count() > m_dataSize) {
        QMap<Phonon::AudioDataOutput::Channel, QVector<qint16> > m_data;
        for (int position = 0; position < chan_count; position++) {
            Phonon::AudioDataOutput::Channel chan = m_channels.value(position);
            QVector<qint16> data = m_channelSamples[position].mid(0, m_dataSize);
            m_channelSamples[position].remove(0, data.count());
            m_data.insert(chan, data);
        }
        emit dataReady(m_data);
    }
    m_locker.unlock();
}

void AudioDataOutput::playCallback(const void *samples, unsigned sampleCount, int64_t pts)
{
    DEBUG_BLOCK;
    QMutexLocker lock(&m_locker);

    // (bytesPerChannelPerSample * channels * read_samples) + (bytesPerChannelPerSample * read_channels)
//    int bytesPerChannelPerSample = bits_per_sample / 8;
    int bytesPerChannelPerSample = 16/8;

    for (quint32 readSamples = 0; readSamples < sampleCount; ++readSamples) {
        // Prepare a sample buffer, and initialise it
        quint16 sampleBuffer[6];
        for (int initialised = 0; initialised < 6; ++initialised) {
            sampleBuffer[initialised] = 0;
        }

        int bufferPosition = (bytesPerChannelPerSample * m_channelCount * readSamples);

        for (quint32 readChannels = 0; readChannels < m_channelCount; ++readChannels) {
            quint32 complet = 0;
            for (int readBytes = 0; readBytes < bytesPerChannelPerSample; ++readBytes) {
                // Read from the pcm_buffer into the per channel internal buffer

                quint32 complet_temp = 0;
                complet_temp = ((quint8 *) samples)[bufferPosition];
                complet_temp <<=  (8 * readBytes);

                complet += complet_temp;
                ++bufferPosition;
            }

            sampleBuffer[readChannels] = complet;
        }

        if (m_channelCount == 1) {
            m_channelSamples[1].append(qint16(sampleBuffer[0]));
        }

        for (quint32 readChannels = 0; readChannels < m_channelCount; ++readChannels) {
            m_channelSamples[readChannels].append(qint16(sampleBuffer[readChannels]));
        }
        // Finished reading one sample
    }

    emit sampleReadDone();
}

bool AudioDataOutput::setupCallback(char *format, unsigned *rate, unsigned *channels)
{
    qstrcpy(format, "S16N"); // As of 2.0 S16N is the only format amem supports
    *rate = m_sampleRate; // Force 44100 Hz as it is not clear what VLC will propose here...
    if (*channels > 6)
        *channels = 6; // Phonon only supports 6 channels
    m_channelCount = *channels;
    return true;
}

} // namespace VLC
} // namespace Phonon

#include "moc_audiodataoutput.cpp"
