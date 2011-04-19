/*
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

#ifndef PHONON_VLC_GLOBALDESCRIPTIONCONTAINER_H
#define PHONON_VLC_GLOBALDESCRIPTIONCONTAINER_H

#include <QtCore/QMap>

#include <phonon/objectdescription.h>

namespace Phonon
{
namespace VLC
{

class MediaController;

template <typename D>
class GlobalDescriptionContainer
{
public:
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

    /**
     * Registers a new MediaController with the container.
     * This essentially creates a new empty ID map.
     *
     * \param mediaController The MediaController
     */
    void register_(MediaController *mediaController)
    {
        Q_ASSERT(mediaController);
        Q_ASSERT(m_localIds.find(mediaController) == m_localIds.end());
        m_localIds[mediaController] = LocalIdMap();
    }

    /**
     * Unregisters a MediaController from the container.
     * This essentially clears the ID map and removes all traces of the
     * MediaController.
     *
     * \param mediaController The MediaController
     */
    void unregister_(MediaController *mediaController)
    {
        // TODO: remove all descriptions that are *only* associated with this MC
        Q_ASSERT(mediaController);
        Q_ASSERT(m_localIds.find(mediaController) != m_localIds.end());
        m_localIds[mediaController].clear();
        m_localIds.remove(mediaController);
    }

    /**
     * Clear the internal mapping of global to local id for a given MediaController.
     *
     * \param mediaController The MediaController
     */
    void clearListFor(MediaController *mediaController)
    {
        Q_ASSERT(mediaController);
        Q_ASSERT(m_localIds.find(mediaController) != m_localIds.end());
        m_localIds[mediaController].clear();
    }

    /**
     * Adds a new description object for a specific MediaController.
     * A description object *must* have a global unique id, which is ensured
     * by using this function, which will either reuse an existing equal
     * ObjectDescription or use the next free unique ID.
     * Using the provided index the unique ID is then mapped to the one of the
     * specific MediaController.
     *
     * \param mediaController The MediaController
     * \param index local ID (i.e. within the MediaController)
     * \param name Name of the description
     * \param type Type of the description (e.g. file)
     */
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

    /**
     * Overload function.
     * The index of the provided descriptor *must* be unique within the
     * context of the container.
     *
     * \param mediaController The MediaController
     * \param descriptor the DescriptionObject with unique index
     */
    void add(MediaController *mediaController,
             D descriptor)
    {
        Q_ASSERT(mediaController);
        Q_ASSERT(m_localIds.find(mediaController) != m_localIds.end());
        Q_ASSERT(m_globalDescriptors.find(descriptor.index()) == m_globalDescriptors.end());

        m_globalDescriptors.insert(descriptor.index(), descriptor);
        m_localIds[mediaController].insert(descriptor.index(), descriptor.index());
    }

    /**list of ObjectDescriptions for a given MediaController, the
     * descriptions are limied by the scope of the type (obviously), so you only
     * get subtitle descriptions from a subtitle container.
     * \param mediaController The MediaController
     *
     * \returns the list of ObjectDescriptions for a given MediaController, the
     * descriptions are limied by the scope of the type (obviously), so you only
     * get subtitle descriptions from a subtitle container.
     */
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

    /**
     * \param mediaController The MediaController
     * \param key the global ID (i.e. index of an ObjectDescription)
     *
     * \returns the local ID associated with the description object
     */
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

    /**
     * \returns next free unique index to be used as global ID for an ObjectDescription
     */
    global_id_t nextFreeIndex()
    {
        return ++m_peak;
    }

    GlobalDescriptorMap m_globalDescriptors;
    QMap<const MediaController *, LocalIdMap> m_localIds;

    global_id_t m_peak;
};

template <typename D>
GlobalDescriptionContainer<D> *GlobalDescriptionContainer<D>::self = 0;

typedef GlobalDescriptionContainer<SubtitleDescription> GlobalSubtitles;

} // Namespace VLC
} // Namespace Phonon


#endif // PHONON_VLC_GLOBALDESCRIPTIONCONTAINER_H
