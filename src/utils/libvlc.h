/*
    Copyright (C) 2011 vlc-phonon AUTHORS <kde-multimedia@kde.org>

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

#ifndef LIBVLC_H
#define LIBVLC_H

#include <QtCore/QtGlobal>
#include <QtCore/QStringList>

#include <vlc/libvlc_version.h>

struct libvlc_instance_t;

/**
 * Convenience macro accessing the vlc_instance_t via LibVLC::self.
 * Please note that init() must have been called whenever using this, as no
 * checking of self is conducted (i.e. can be null).
 */
#define libvlc LibVLC::self->vlc()

/**
 * Foreach loop macro for VLC descriptions.
 *
 * For this macro to work the type name must be of the form:
 * \verbatim libvlc_FOO_t \endverbatim
 * *
 * \param type the type identifier of VLC (without libvlc and _t)
 * \param variable the variable name you want to use
 * \param getter the getter from which to get the iterator
 * \param releaser, function name to release the list
 */
#define VLC_FOREACH(type, variable, getter, releaser) \
    for (libvlc_##type##_t *__libvlc_first_element = getter, *variable = __libvlc_first_element; \
        variable; \
        variable = variable->p_next, !variable ? releaser(__libvlc_first_element) : (void)0)

// This foreach expects only a type and variable because getter and releaser are generic.
// Also the type is in short form i.e. libvlc_foo_t would be foo.
#define VLC_FOREACH_LIST(type, variable) VLC_FOREACH(type, variable, libvlc_##type##_list_get(libvlc), libvlc_##type##_list_release)

// These foreach expect no type because the type is generic, they do however
// expect a getter to allow usage with our wrapper classes and since the getter
// will most likely not be generic.
// For instance libvlc_audio_get_track_description returns a generic
// libvlc_track_description_t pointer. So the specific audio_track function
// relates to the generic track description type.
#define VLC_FOREACH_TRACK(variable, getter) VLC_FOREACH(track_description, variable, getter, libvlc_track_description_list_release)
#define VLC_FOREACH_MODULE(variable, getter) VLC_FOREACH(module_description, variable, getter, libvlc_module_description_list_release)

/**
 * \brief Singleton class containing a libvlc instance.
 *
 * This class is a convenience class implementing the singleton pattern to hold
 * an instance of libvlc. This instance is necessary to call various libvlc
 * functions (such as creating a new mediaplayer instance).
 *
 * To initialize the object call init(), this will create the actualy LibVLC
 * instance and then try to initialize the libvlc instance itself.
 * init() returns false in case the libvlc instance could not be created.
 *
 * For convenience reasons there is also a libvlc macro which gets the LibVLC
 * instance and then the libvlc_instance_t form that. Note that this macro
 * does not check whether LibVLC actually got initialized, so it should only
 * be used when you can be absolutely sure that init() was already called
 *
 * \code
 * LibVLC::init(0); // init LibVLC
 * if (!LibVLC::self) {
 *     exit(1); // error if self is null
 * }
 * libvlc_media_player_new(libvlc); // use libvlc macro
 * \endcode
 *
 * \author Harald Sitter <sitter@kde->org>
 */
class LibVLC
{
public:
    /**
     * The singleton itself. Beware that this returns 0 unless init was called.
     *
     * \returns LibVLC instance or 0 if there is none.
     *
     * \see init
     */
    static LibVLC *self;

    /**
     * \returns the contained libvlc instance.
     */
    libvlc_instance_t *vlc()
    {
        return m_vlcInstance;
    }

    /**
     * Construct singleton and initialize and launch the VLC library.
     *
     * \return VLC initialization result
     */
    static bool init();

    /**
     * \returns the most recent error message of libvlc
     */
    static const char *errorMessage();

    /**
     * Destruct the LibVLC singleton and release the contained libvlc instance.
     */
    ~LibVLC();

private:
    Q_DISABLE_COPY(LibVLC)

    /**
     * Private default constructor, to create LibVLC call init instead.
     *
     * \see init
     */
    LibVLC();

    libvlc_instance_t *m_vlcInstance;
};

#endif // LIBVLC_H
