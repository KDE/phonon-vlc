/*
    Copyright (C) 2007-2008 Tanguy Krotoff <tkrotoff@gmail.com>
    Copyright (C) 2008 Lukas Durfina <lukas.durfina@gmail.com>
    Copyright (C) 2009 Fathi Boudra <fabo@kde.org>
    Copyright (C) 2009-2011 vlc-phonon AUTHORS <kde-multimedia@kde.org>
    Copyright (C) 2011 Harald Sitter <sitter@kde.org>

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

#include "effectmanager.h"

#include <vlc/vlc.h>
#include <vlc/libvlc_version.h>

#include "equalizereffect.h"

#include "utils/debug.h"
#include "utils/libvlc.h"

namespace Phonon {
namespace VLC {

EffectInfo::EffectInfo(const QString &name, const QString &description,
                       const QString &author, int filter, Type type)
    : m_name(name)
    , m_description(description)
    , m_author(author)
    , m_filter(filter)
    , m_type(type)
{}

EffectManager::EffectManager(QObject *parent)
    : QObject(parent)
{
    if (!pvlc_libvlc)
        return;

    updateEffects();
}

EffectManager::~EffectManager()
{
    m_audioEffectList.clear();
    m_videoEffectList.clear();

    // EffectsList holds the same pointers as audio and video, so qDeleteAll on
    // this container would cause a double freeing.
    m_effectList.clear();
}

const QList<EffectInfo> EffectManager::audioEffects() const
{
    return m_audioEffectList;
}

const QList<EffectInfo> EffectManager::videoEffects() const
{
    return m_videoEffectList;
}

const QList<EffectInfo> EffectManager::effects() const
{
    return m_effectList;
}

QObject *EffectManager::createEffect(int id, QObject *parent)
{
    Q_UNUSED(id);
    return new EqualizerEffect(parent);
}

void EffectManager::updateEffects()
{
    DEBUG_BLOCK;

    m_effectList.clear();
    m_audioEffectList.clear();
    m_videoEffectList.clear();

    // Generic effect activation etc is entirely kaput and equalizer has specific
    // API anyway, so we simply manually insert it \o/

    const QString eqName = QString("equalizer-%1bands").arg(QString::number(libvlc_audio_equalizer_get_band_count()));
    m_audioEffectList.append(EffectInfo(
                                 eqName,
                                 QString(""),
                                 QString(""),
                                 0,
                                 EffectInfo::AudioEffect));

//    int moduleCount = -1;
//    VLC_FOREACH_MODULE(module, libvlc_audio_filter_list_get(libvlc)) {
//        m_audioEffectList.append(new EffectInfo(module->psz_longname,
//                                                module->psz_help,
//                                                QString(),
//                                                ++moduleCount,
//                                                EffectInfo::AudioEffect));
//    }

//    moduleCount = -1;
//    VLC_FOREACH_MODULE(module, libvlc_video_filter_list_get(libvlc)) {
//        m_videoEffectList.append(new EffectInfo(module->psz_longname,
//                                                module->psz_help,
//                                                QString(),
//                                                ++moduleCount,
//                                                EffectInfo::VideoEffect));
//    }

    m_effectList.append(m_audioEffectList);
    m_effectList.append(m_videoEffectList);
}

} // namespace VLC
} // namespace Phonon
