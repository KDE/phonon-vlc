/*
    Copyright (C) 2007-2008 Tanguy Krotoff <tkrotoff@gmail.com>
    Copyright (C) 2008 Lukas Durfina <lukas.durfina@gmail.com>
    Copyright (C) 2009 Fathi Boudra <fabo@kde.org>
    Copyright (C) 2009-2011 vlc-phonon AUTHORS
    Copyright (C) 2010-2011 Harald Sitter <sitter@kde.org>

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

#include "audiooutput.h"

#include <vlc/vlc.h>

#include "backend.h"
#include "player.h"
#include "utils/debug.h"
#include "utils/media.h"

namespace Phonon {
namespace VLC {

AudioOutput::AudioOutput(QObject *parent)
    : QObject(parent),
      m_volume(1.0)
{
}

AudioOutput::~AudioOutput()
{
}

void AudioOutput::handleConnectPlayer(Player *mediaObject)
{
    DEBUG_BLOCK;
    setOutputDeviceImplementation();
    applyVolume();
}

void AudioOutput::handleAddToMedia(Media *media)
{
    media->addOption(":audio");
}

qreal AudioOutput::volume() const
{
    return m_volume;
}

void AudioOutput::setVolume(qreal volume)
{
    if (m_vlcPlayer) {
        pDebug() << "async setting of volume to" << volume;
        m_volume = volume;
        applyVolume();
        emit volumeChanged(m_volume);
    }
}

void AudioOutput::setOutputDeviceImplementation()
{
    Q_ASSERT(m_vlcPlayer);
    DEBUG_BLOCK;
    pDebug() << this;

#warning forcing pulse ... because I can.
    m_vlcPlayer->setAudioOutput("pulse");
    return;
}

void AudioOutput::applyVolume()
{
    if (m_vlcPlayer) {
        const int preVolume = m_vlcPlayer->audioVolume();
        const int newVolume = m_volume * 100;
        m_vlcPlayer->setAudioVolume(newVolume);

        pDebug() << "Volume changed from" << preVolume << "to" << newVolume;
    }
}

} // namespace VLC
} // namespace Phonon
