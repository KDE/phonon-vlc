/*  This file is part of the KDE project
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

namespace Phonon
{
namespace VLC
{

/**
 * Creates an audio data output. The sample rate is set to 44100 Hz.
 * The available audio channels are registered. These are:
 * \li Left \li Right \li Center \li LeftSurround \li RightSurround \li Subwoofer
 */
AudioDataOutput::AudioDataOutput(Backend *backend, QObject *parent)
    : QObject(parent)
{
    Q_UNUSED(backend)

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
/**
 * Connect this AudioDataOutput only to the audio media part of the AvCapture.
 *
 * \see AvCapture
 */
void AudioDataOutput::connectToAvCapture(Experimental::AvCapture *avCapture)
{
    connectToMediaObject(avCapture->audioMediaObject());
}

/**
 * Disconnect the AudioDataOutput from the video media of the AvCapture.
 *
 * \see connectToAvCapture()
 */
void AudioDataOutput::disconnectFromAvCapture(Experimental::AvCapture *avCapture)
{
    disconnectFromMediaObject(avCapture->audioMediaObject());
}
#endif // PHONON_VLC_NO_EXPERIMENTAL

/**
 * \return The currently used number of samples passed through the signal.
 */
int AudioDataOutput::dataSize() const
{
    return m_dataSize;
}

/**
 * \return The current sample rate in Hz.
 */
int AudioDataOutput::sampleRate() const
{
    return m_sampleRate;
}

/**
 * Sets the number of samples to be passed in one signal emission.
 */
void AudioDataOutput::setDataSize(int size)
{
    m_dataSize = size;
}

/**
 * Adds special options to the libVLC Media Object to adapt it to give audio data
 * directly. There are two callbacks used: lock(), unlock().
 *
 * \see lock()
 * \see unlock()
 * \see SinkNode::connectToMediaObject()
 */
void AudioDataOutput::addToMedia(libvlc_media_t *media)
{
    // WARNING: DO NOT CHANGE ANYTHING HERE FOR CODE CLEANING PURPOSES!
    // WARNING: REQUIRED FOR COMPATIBILITY WITH LIBVLC!
    char param[64];

    // Output to stream renderer
    libvlc_media_add_option_flag(media, ":sout=#duplicate{dst=display,select=audio,dst='transcode{}'}:smem", libvlc_media_option_trusted);
    libvlc_media_add_option_flag(media, ":sout-transcode-acodec=f32l", libvlc_media_option_trusted);
    libvlc_media_add_option_flag(media, ":sout-smem-time-sync", libvlc_media_option_trusted);

    // Add audio lock callback
    void *lock_call = reinterpret_cast<void *>(&AudioDataOutput::lock);
    sprintf(param, ":sout-smem-audio-prerender-callback=%"PRId64, (qint64)(intptr_t)lock_call);
    libvlc_media_add_option_flag(media, param, libvlc_media_option_trusted);

    // Add audio unlock callback
    void *unlock_call = reinterpret_cast<void *>(&AudioDataOutput::unlock);
    sprintf(param, ":sout-smem-audio-postrender-callback=%"PRId64, (qint64)(intptr_t)unlock_call);
    libvlc_media_add_option_flag(media, param, libvlc_media_option_trusted);

    // Add pointer to ourselves...
    sprintf(param, ":sout-smem-audio-data=%"PRId64, (qint64)(intptr_t)this);
    libvlc_media_add_option_flag(media, param, libvlc_media_option_trusted);
}

/**
 * This is a VLC prerender callback. The m_locker mutex is locked, and a new buffer is prepared
 * for the incoming audio data.
 *
 * \param cw The AudioDataOutput for this callback
 * \param pcm_buffer The new data buffer
 * \param size Size for the incoming data
 *
 * \see unlock()
 */
void AudioDataOutput::lock(AudioDataOutput *cw, quint8 **pcm_buffer , quint32 size)
{
    cw->m_locker.lock();

    cw->m_buffer = new uchar[size];
    *pcm_buffer = cw->m_buffer;
}

/**
 * This is a VLC postrender callback. Interprets the data received in m_buffer,
 * separating the samples and channels. Finally, the buffer is freed and m_locker
 * is unlocked. Now the audio data output is ready for sending data.
 *
 * \param cw The AudioDataOutput for this callback
 *
 * \see lock()
 * \see sendData()
 */
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

/**
 * Looks at the channel samples generated in lock() and creates the QMap required for
 * the dataReady() signal. Then the signal is emitted. This repeats as long as there is
 * data remaining.
 *
 * \see lock()
 */
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

