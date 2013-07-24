/*
    Copyright (C) 2007-2008 Tanguy Krotoff <tkrotoff@gmail.com>
    Copyright (C) 2008 Lukas Durfina <lukas.durfina@gmail.com>
    Copyright (C) 2009 Fathi Boudra <fabo@kde.org>
    Copyright (C) 2009-2011 vlc-phonon AUTHORS
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

#ifndef Phonon_VLC_EFFECTMANAGER_H
#define Phonon_VLC_EFFECTMANAGER_H

#include <QtCore/QObject>

namespace Phonon {
namespace VLC {

class Backend;
class EffectManager;

/// Holds information about an effect
class EffectInfo
{
public:

    enum Type {AudioEffect, VideoEffect};

    EffectInfo(const QString &name,
               const QString &description,
               const QString &author,
               int filter,
               Type type);

    QString name() const {
        return m_name;
    }

    QString description() const {
        return m_description;
    }

    QString author() const {
        return m_author;
    }

    int filter() const {
        return m_filter;
    }

    Type type() const {
        return m_type;
    }

private:
    const QString m_name;
    const QString m_description;
    const QString m_author;
    const int m_filter;
    const Type m_type;
};

/** \brief Manages a list of effects.
 *
 * \see EffectInfo
 */
class EffectManager : public QObject
{
    Q_OBJECT
public:
    /**
     * Creates a new effect manager. It creates the lists of effects.
     *
     * \param backend A parent backend object for the effect manager
     *
     * \warning Currently it doesn't add any effects, everything is disabled.
     * \see EffectInfo
     */
    explicit EffectManager(QObject *parent = 0);

    /// Deletes all the effects from the lists and destroys the effect manager.
    ~EffectManager();

    /// Returns a list of available audio effects
    const QList<EffectInfo *> audioEffects() const;

    /// Returns a list of available video effects
    const QList<EffectInfo *> videoEffects() const;

    /// Returns a list of available effects
    const QList<EffectInfo *> effects() const;

private:
    /// Generates the aggegated list of effects from both video and audio
    void updateEffects();

    QList<EffectInfo *> m_effectList;
    QList<EffectInfo *> m_audioEffectList;
    QList<EffectInfo *> m_videoEffectList;
    bool m_equalizerEnabled;
};

} // namespace VLC
} // namespace Phonon

#endif // Phonon_VLC_EFFECTMANAGER_H
