/*
    Copyright (C) 2013 Harald Sitter <sitter@kde.org>

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

#include "equalizereffect.h"

#include "mediaplayer.h"
#include "utils/debug.h"

namespace Phonon {
namespace VLC {

EqualizerEffect::EqualizerEffect(QObject *parent)
    : QObject(parent)
    , SinkNode()
    , EffectInterface()
    , m_equalizer(libvlc_audio_equalizer_new())
{
    // Amarok decided to make up rules because phonon didn't manage to
    // pre-amp string needs to be pre-amp
    // bands need to be xxHz
    // That way they can be consistently mapped to localized/formatted strings.

    EffectParameter preamp(-1, "pre-amp", {} /* hint */, 0.0f, -20.0f, 20.0f);
    m_bands.append(preamp);

    const unsigned int bandCount = libvlc_audio_equalizer_get_band_count();
    for (unsigned int i = 0; i < bandCount; ++i) {
        const float frequency = libvlc_audio_equalizer_get_band_frequency(i);
        const QString name = QString("%1Hz").arg(QString::number(frequency));
        EffectParameter parameter(i, name, {} /* hint */, 0.0f, -20.0f, 20.0f);
        m_bands.append(parameter);
    }
}

EqualizerEffect::~EqualizerEffect()
{
    libvlc_audio_equalizer_release(m_equalizer);
}

QList<EffectParameter> EqualizerEffect::parameters() const
{
    return m_bands;
}

QVariant EqualizerEffect::parameterValue(const EffectParameter &parameter) const
{
    return libvlc_audio_equalizer_get_amp_at_index(m_equalizer, parameter.id());
}

void EqualizerEffect::setParameterValue(const EffectParameter &parameter,
                                        const QVariant &newValue)
{
    if (parameter.id() == -1)
        libvlc_audio_equalizer_set_preamp(m_equalizer, newValue.toFloat());
    else
        libvlc_audio_equalizer_set_amp_at_index(m_equalizer, newValue.toFloat(), parameter.id());
}

void EqualizerEffect::handleConnectToMediaObject(MediaObject *mediaObject)
{
    Q_UNUSED(mediaObject);
    m_player->setEqualizer(m_equalizer);
}

} // namespace VLC
} // namespace Phonon
