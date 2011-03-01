/*
    Copyright (C) 2006 Matthias Kretz <kretz@kde.org>
    Copyright (C) 2009 Martin Sandsmark <sandsmark@samfundet.no>
    Copyright (C) 2010 Ben Cooksley <sourtooth@gmail.com>

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

#include <QtCore/QVector>
#include <QtCore/QMap>

#include <phonon/medianode.h>
#include <phonon/audiooutput.h>

#include <vlc/vlc.h>

#include "mediaobject.h"

#ifndef PHONON_VLC_NO_EXPERIMENTAL
#include <experimental/avcapture.h>
#endif // PHONON_VLC_NO_EXPERIMENTAL

namespace Phonon
{
namespace VLC
{

AudioDataOutput::AudioDataOutput(QObject *parent)
    : QObject(parent)
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

#ifndef PHONON_VLC_NO_EXPERIMENTAL
void AudioDataOutput::connectToAvCapture(Experimental::AvCapture *avCapture)
{
    connectToMediaObject(avCapture->audioMediaObject());
}

void AudioDataOutput::disconnectFromAvCapture(Experimental::AvCapture *avCapture)
{
    disconnectFromMediaObject(avCapture->audioMediaObject());
}
#endif // PHONON_VLC_NO_EXPERIMENTAL

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

void AudioDataOutput::addToMedia(libvlc_media_t *media)
{
    MediaObject::addOption(media, ":sout=#duplicate{dst=display,select=audio,dst='transcode{}'}:smem");
    MediaObject::addOption(media, ":sout-transcode-acodec=f32l");
    MediaObject::addOption(media, ":sout-smem-time-sync");

    // Add audio lock callback
    MediaObject::addOption(media, ":sout-smem-audio-prerender-callback=", INTPTR_FUNC(AudioDataOutput::lock));
    // Add audio unlock callback
    MediaObject::addOption(media, ":sout-smem-audio-postrender-callback=", INTPTR_FUNC(AudioDataOutput::unlock));
    // Add pointer to ourselves...
    MediaObject::addOption(media, ":sout-smem-audio-data=", INTPTR_PTR(this));
}

void AudioDataOutput::lock(AudioDataOutput *cw, quint8 **pcm_buffer , quint32 size)
{
    cw->m_locker.lock();

    cw->m_buffer = new uchar[size];
    *pcm_buffer = cw->m_buffer;
}

void AudioDataOutput::unlock(AudioDataOutput *cw, quint8 *pcm_buffer,
                             quint32 channels, quint32 rate,
                             quint32 nb_samples, quint32 bits_per_sample,
                             quint32 size, qint64 pts)
{
    Q_UNUSED(size);
    Q_UNUSED(pts);
    Q_UNUSED(pcm_buffer);

    // (bytesPerChannelPerSample * channels * read_samples) + (bytesPerChannelPerSample * read_channels)
    int bytesPerChannelPerSample = bits_per_sample / 8;
    cw->m_sampleRate = rate;
    cw->m_channel_count = channels;

    for (quint32 read_samples = 0; nb_samples > read_samples; read_samples++) {
        // Prepare a sample buffer, and initialise it
        quint16 sample_buffer[6];
        for (int initialised = 0; initialised < 6; initialised++) {
            sample_buffer[initialised] = 0;
        }

        int buffer_pos = (bytesPerChannelPerSample * channels * read_samples);

        for (quint32 channels_read = 0; channels_read < channels; channels_read++) {
            for (int bytes_read = 0; bytes_read < bytesPerChannelPerSample; bytes_read++) {
                // Read from the pcm_buffer into the per channel internal buffer
                sample_buffer[channels_read] += cw->m_buffer[buffer_pos];
                buffer_pos++;
            }
        }

        if (channels == 1) {
            cw->m_channel_samples[1].append(sample_buffer[0]);
        }

        for (quint32 channels_read = 0; channels > channels_read; channels_read++) {
            cw->m_channel_samples[channels_read].append(sample_buffer[channels_read]);
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

    int chan_count = m_channel_count;
    if (m_channel_count == 1) {
        chan_count = 2;
    }

    while (m_channel_samples[0].count() > m_dataSize) {
        QMap<Phonon::AudioDataOutput::Channel, QVector<qint16> > m_data;
        for (int position = 0; position < chan_count; position++) {
            Phonon::AudioDataOutput::Channel chan = m_channels.value(position);
            QVector<qint16> data = m_channel_samples[position].mid(0, m_dataSize);
            m_channel_samples[position].remove(0, data.count());
            m_data.insert(chan, data);
        }
        emit dataReady(m_data);
    }
    m_locker.unlock();
}

}
} //namespace Phonon::VLC

#include "moc_audiodataoutput.cpp"
// vim: sw=4 ts=4

