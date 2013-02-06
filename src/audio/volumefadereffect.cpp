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

#include "volumefadereffect.h"
#include "common.h"

#ifndef QT_NO_PHONON_VOLUMEFADEREFFECT
namespace Phonon
{
namespace VLC
{
VolumeFaderEffect::VolumeFaderEffect(Backend *backend, QObject *parent)
    : Effect(backend, parent, AudioSource | AudioSink)
    , m_fadeCurve(Phonon::VolumeFaderEffect::Fade3Decibel)
    , m_fadeTimer(0)
    , m_fadeDuration(0)
    , m_fadeFromVolume(0)
    , m_fadeToVolume(0)
{
    init();
}

VolumeFaderEffect::~VolumeFaderEffect()
{
    if (m_fadeTimer) {
        killTimer(m_fadeTimer);
    }
}

float VolumeFaderEffect::volume() const
{
    //TODO: Implement me
}

void VolumeFaderEffect::setVolume(float volume)
{
    //TODO: Implement me
}

Phonon::VolumeFaderEffect::FadeCurve VolumeFaderEffect::fadeCurve() const
{
    return m_fadeCurve;
}

void VolumeFaderEffect::setFadeCurve(Phonon::VolumeFaderEffect::FadeCurve fadeCurve)
{
    m_fadeCurve = fadeCurve;
}

void VolumeFaderEffect::fadeTo(float targetVolume, int fadeTime)
{
    m_fadeToVolume = targetVolume;
    m_fadeDuration = fadeTime;
    m_fadeFromVolume = volume();
    m_fadeStartTime.start();

    if (m_fadeTimer) {
        killTimer(m_fadeTimer);
    }
    m_fadeTimer = startTimer(30);
}

void VolumeFaderEffect::updateFade()
{
    double currVal = 0.0;
    float step = float(m_fadeStartTime.elapsed()) / float(m_fadeDuration);
    if (step > 1) {
        step = 1;
        if (m_fadeTimer) {
            killTimer(m_fadeTimer);
            m_fadeTimer = 0;
        }
    }
    // This is a very loose and interpretation of the API
    // But in fact when fading between arbitrary values, the decibel values make no sense
    // Note : seems like we will change the API to re-use names from QTimeline for this
    switch (fadeCurve()) {
    case Phonon::VolumeFaderEffect::Fade3Decibel: // Slow in the beginning
        currVal = step * step;
        break;
    case Phonon::VolumeFaderEffect::Fade6Decibel: // Linear fade
        currVal = step;
        break;
    case Phonon::VolumeFaderEffect::Fade9Decibel: // Fast in the beginning / Linear
        currVal = step * 0.5 + (1.0 - (1.0 - step) * (1.0 - step)) * 0.5;
        break;
    case Phonon::VolumeFaderEffect::Fade12Decibel: // Fast in the beginning
        currVal = 1.0 - (1.0 - step) * (1.0 - step);
        break;
    default:
        break;
    }
    const double volume = (1.0 - currVal) * m_fadeFromVolume + currVal * m_fadeToVolume;
    setVolume(volume);
}

bool VolumeFaderEffect::event(QEvent *event)
{
    switch (event->type()) {
    case QEvent::Timer: {
        QTimerEvent *timerEvent = static_cast<QTimerEvent *>(event);
        if (timerEvent->timerId() == m_fadeTimer) {
            updateFade();
        }
        break;
    }
    default:
        break;
    }
    return QObject::event(event);
}

}
} //namespace Phonon::VLC
#endif //QT_NO_PHONON_VOLUMEFADEREFFECT

#include "moc_volumefadereffect.cpp"
