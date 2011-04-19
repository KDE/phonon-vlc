/*****************************************************************************
 * libVLC backend for the Phonon library                                     *
 *                                                                           *
 * Copyright (C) 2007-2008 Tanguy Krotoff <tkrotoff@gmail.com>               *
 * Copyright (C) 2008 Lukas Durfina <lukas.durfina@gmail.com>                *
 * Copyright (C) 2009 Fathi Boudra <fabo@kde.org>                            *
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

#ifndef PHONON_VLC_MEDIACONTROLLER_H
#define PHONON_VLC_MEDIACONTROLLER_H

#include <phonon/addoninterface.h>
#include <phonon/objectdescription.h>

#include "debug.h"

struct libvlc_media_player_t;

namespace Phonon
{
namespace VLC
{

/**
 * \brief Interface for AddonInterface.
 *
 * Provides a bridge between Phonon's AddonInterface and VLCMediaController.
 *
 * This class cannot inherit from QObject has MediaObject already inherit from QObject.
 * This is a Qt limitation: there is no possibility to inherit virtual Qobject :/
 * See http://doc.trolltech.com/qq/qq15-academic.html
 * Phonon implementation got the same problem.
 *
 * \see VLCMediaController
 * \see VLCMediaObject
 * \see MediaObject
 */
class MediaController : public AddonInterface
{
public:

    MediaController();
    virtual ~MediaController();

    bool hasInterface(Interface iface) const;

    QVariant interfaceCall(Interface iface, int i_command, const QList<QVariant> & arguments = QList<QVariant>());

    // MediaController signals
    virtual void availableSubtitlesChanged() = 0;
    virtual void availableAudioChannelsChanged() = 0;

    virtual void availableChaptersChanged(int) = 0;
    virtual void availableTitlesChanged(int) = 0;

    virtual void availableAnglesChanged(int i_available_angles) = 0;
    virtual void angleChanged(int i_angle_number) = 0;
    virtual void chapterChanged(int i_chapter_number) = 0;


    void audioChannelAdded(int id, const QString &lang);
    void subtitleAdded(int id, const QString &lang, const QString &type);
    void titleAdded(int id, const QString &name);
    void chapterAdded(int titleId, const QString &name);

protected:
    // AudioChannel
    void setCurrentAudioChannel(const Phonon::AudioChannelDescription &audioChannel);
    QList<Phonon::AudioChannelDescription> availableAudioChannels() const;
    Phonon::AudioChannelDescription currentAudioChannel() const;
    void refreshAudioChannels();

    // Subtitle
    void setCurrentSubtitle(const Phonon::SubtitleDescription &subtitle);
    QList<Phonon::SubtitleDescription> availableSubtitles() const;
    Phonon::SubtitleDescription currentSubtitle() const;
    void refreshSubtitles();

    // Angle
    void setCurrentAngle(int angleNumber);
    int availableAngles() const;
    int currentAngle() const;

    // Chapter
//    void setCurrentChapter( const Phonon::ChapterDescription & chapter );
//    QList<Phonon::ChapterDescription> availableChapters() const;
//    Phonon::ChapterDescription currentChapter() const;
    void setCurrentChapter(int chapterNumber);
    int availableChapters() const;
    int currentChapter() const;
    void refreshChapters(int title);

    // Title
//    void setCurrentTitle( const Phonon::TitleDescription & title );
//    QList<Phonon::TitleDescription> availableTitles() const;
//    Phonon::TitleDescription currentTitle() const;
    void setCurrentTitle(int titleNumber);
    int availableTitles() const;
    int currentTitle() const;

    void setAutoplayTitles(bool autoplay);
    bool autoplayTitles() const;

    /**
     * Clear all member variables and emit appropriate signals.
     * This is used each time we restart the video.
     *
     * \see resetMembers
     */
    void resetMediaController();

    /**
     * Reset all member variables.
     *
     * \see resetMediaController
     */
    void resetMembers();

    Phonon::AudioChannelDescription m_currentAudioChannel;
    QList<Phonon::AudioChannelDescription> m_availableAudioChannels;

    Phonon::SubtitleDescription m_currentSubtitle;

//    Phonon::ChapterDescription current_chapter;
//    QList<Phonon::ChapterDescription> available_chapters;
    int m_currentChapter;
    int m_availableChapters;

//    Phonon::TitleDescription current_title;
//    QList<Phonon::TitleDescription> available_titles;
    int m_currentTitle;
    int m_availableTitles;

    int m_currentAngle;
    int m_availableAngles;

    bool m_autoPlayTitles;

    // MediaPlayer
    libvlc_media_player_t *m_player;
};

template <typename D>
class GlobalDescriptionContainer
{
    typedef int global_id_t;
    typedef int local_id_t;

    typedef QMap<global_id_t, D> GlobalDescriptorMap;
    typedef QMapIterator<global_id_t, D> GlobalDescriptorMapIterator;

    typedef QMap<global_id_t, local_id_t> LocalIdMap;
    typedef QMapIterator<global_id_t, local_id_t> LocaIdMapIterator;

public:
    static GlobalDescriptionContainer *self;

    static GlobalDescriptionContainer *instance()
    {
        if (!self)
            self = new GlobalDescriptionContainer;
        return self;
    }

    virtual ~GlobalDescriptionContainer() {}

    QList<int> globalIndexes()
    {
        QList<int> list;
        GlobalDescriptorMapIterator it(m_globalDescriptors);
       while (it.hasNext()) {
           it.next();
           list << it.key();
        }
        return list;
    }

    SubtitleDescription fromIndex(global_id_t key)
    {
        return m_globalDescriptors.value(key, SubtitleDescription());
    }

    // ----------- MediaController Specific ----------- //

    void register_(MediaController *mediaController)
    {
        Q_ASSERT(mediaController);
        Q_ASSERT(m_localIds.find(mediaController) == m_localIds.end());
        m_localIds[mediaController] = LocalIdMap();
    }

    void unregister_(MediaController *mediaController)
    {
        Q_ASSERT(mediaController);
        Q_ASSERT(m_localIds.find(mediaController) != m_localIds.end());
        m_localIds[mediaController] = LocalIdMap();
    }

    /**
     * Clear the internal mapping of global to local id
     */
    void clearListFor(MediaController *mediaController)
    {
        Q_ASSERT(mediaController);
        Q_ASSERT(m_localIds.find(mediaController) != m_localIds.end());
        m_localIds[mediaController].clear();
    }

    void add(MediaController *mediaController,
             local_id_t index, const QString &name, const QString &type)
    {
        DEBUG_BLOCK;
        Q_ASSERT(mediaController);
        Q_ASSERT(m_localIds.find(mediaController) != m_localIds.end());

        QHash<QByteArray, QVariant> properties;
        properties.insert("name", name);
        properties.insert("description", "");
        properties.insert("type", type);

        // Empty lists will start at 0.
        global_id_t id = 0;
        {
            // Find id, either a descriptor with name and type is already present
            // or get the next available index.
            GlobalDescriptorMapIterator it(m_globalDescriptors);
            while (it.hasNext()) {
                it.next();
#ifdef __GNUC__
#warning make properties accessible
#endif
                if (it.value().property("name") == name &&
                        it.value().property("type") == type) {
                    id = it.value().index();
                } else {
                    id = nextFreeIndex();
                }
            }
        }

        debug() << "add: ";
        debug() << "id: " << id;
        debug() << "  local: " << index;
        debug() << "  name: " << name;
        debug() << "  type: " << type;

        D descriptor = D(id, properties);

        m_globalDescriptors.insert(id, descriptor);
        m_localIds[mediaController].insert(id, index);
    }

    void add(MediaController *mediaController,
             D descriptor)
    {
        Q_ASSERT(mediaController);
        Q_ASSERT(m_localIds.find(mediaController) != m_localIds.end());
        Q_ASSERT(m_globalDescriptors.find(descriptor.index()) == m_globalDescriptors.end());

        m_globalDescriptors.insert(descriptor.index(), descriptor);
        m_localIds[mediaController].insert(descriptor.index(), descriptor.index());
    }

    QList<D> listFor(const MediaController *mediaController) const
    {
        Q_ASSERT(mediaController);
        Q_ASSERT(m_localIds.find(mediaController) != m_localIds.end());

        QList<D> list;
        LocaIdMapIterator it(m_localIds.value(mediaController));
        while (it.hasNext()) {
            it.next();
            Q_ASSERT(m_globalDescriptors.find(it.key()) != m_globalDescriptors.end());
            list << m_globalDescriptors[it.key()];
        }
        return list;
    }

    int localIdFor(MediaController * mediaController, global_id_t key) const
    {
        Q_ASSERT(mediaController);
        Q_ASSERT(m_localIds.find(mediaController) != m_localIds.end());
#ifdef __GNUC__
#warning localid fail not handled
#endif
        return m_localIds[mediaController].value(key, 0);
    }

protected:
    GlobalDescriptionContainer() : m_peak(0) {}

    global_id_t nextFreeIndex()
    {
        return ++m_peak;
    }

//    virtual void purifier() = 0;

    GlobalDescriptorMap m_globalDescriptors;
    QMap<const MediaController *, LocalIdMap> m_localIds;

    global_id_t m_peak;
};

typedef GlobalDescriptionContainer<SubtitleDescription> GlobalSubtitles;

}
} // Namespace Phonon::VLC

#endif // PHONON_VLC_MEDIACONTROLLER_H
