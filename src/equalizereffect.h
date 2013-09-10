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

#ifndef PHONON_VLC_EQUALIZEREFFECT_H
#define PHONON_VLC_EQUALIZEREFFECT_H

#include <QtCore/QObject>

#include <phonon/effectinterface.h>
#include <phonon/effectparameter.h>

#include <vlc/vlc.h>

#include "sinknode.h"

namespace Phonon {
namespace VLC {

class EqualizerEffect : public QObject, public SinkNode, public EffectInterface
{
    Q_OBJECT
    Q_INTERFACES(Phonon::EffectInterface)
public:
    explicit EqualizerEffect(QObject *parent = 0);
    ~EqualizerEffect();

    QList<EffectParameter> parameters() const;
    QVariant parameterValue(const EffectParameter &parameter) const;
    void setParameterValue(const EffectParameter &parameter, const QVariant &newValue);

    void handleConnectToMediaObject(MediaObject *mediaObject);

private:
    libvlc_equalizer_t *m_equalizer;
    QList <EffectParameter> m_bands;
};

} // namespace VLC
} // namespace Phonon

#endif // PHONON_VLC_EQUALIZEREFFECT_H
