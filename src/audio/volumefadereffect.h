/*  This file is part of the KDE project.

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

#ifndef Phonon_VLC_VOLUMEFADEREFFECT_H
#define Phonon_VLC_VOLUMEFADEREFFECT_H

#include "medianode.h"
#include "effect.h"

#include <phonon/effectinterface.h>
#include <phonon/effectparameter.h>
#include <phonon/volumefaderinterface.h>


QT_BEGIN_NAMESPACE
#ifndef QT_NO_PHONON_VOLUMEFADEREFFECT
namespace Phonon
{
namespace VLC
{
/** \brief Implements the volume fader effect
 *
 * The volume is updated with the updateFade() method.
 *
 * \todo Implement volume(), setVolume()
 */
class VolumeFaderEffect : public Effect, public VolumeFaderInterface
{
    Q_OBJECT
    Q_INTERFACES(Phonon::VolumeFaderInterface)

public:
    explicit VolumeFaderEffect(Backend *backend, QObject *parent = 0);
    ~VolumeFaderEffect();

    bool event(QEvent *);
    void updateFade();

    // VolumeFaderInterface:
    float volume() const;
    void setVolume(float volume);
    Phonon::VolumeFaderEffect::FadeCurve fadeCurve() const;
    void setFadeCurve(Phonon::VolumeFaderEffect::FadeCurve fadeCurve);
    void fadeTo(float volume, int fadeTime);

    Phonon::VolumeFaderEffect::FadeCurve m_fadeCurve;
    int m_fadeTimer;
    int m_fadeDuration;
    float m_fadeFromVolume;
    float m_fadeToVolume;
    QTime m_fadeStartTime;
};
}
} //namespace Phonon::Gstreamer
#endif //QT_NO_PHONON_VOLUMEFADEREFFECT
QT_END_NAMESPACE

#endif // Phonon_VLC_VOLUMEFADEREFFECT_H
