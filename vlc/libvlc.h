/*****************************************************************************
 * libVLC backend for the Phonon library                                     *
 *                                                                           *
 * Copyright (C) 2011 vlc-phonon AUTHORS                                     *
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

#ifndef LIBVLC_H
#define LIBVLC_H

#include <QtCore/QtGlobal>

#include <QtCore/QStringList>

class QLibrary;

struct libvlc_instance_t;

#define libvlc LibVLC::self->vlc()

class LibVLC
{
public:
    LibVLC(int debugLevl = 0);
    ~LibVLC();

    static LibVLC *self;

    libvlc_instance_t *vlc()
    {
        return m_vlcInstance;
    }

private:
    Q_DISABLE_COPY(LibVLC)

    /**
     * Initialize and launch VLC library.
     *
     * instance and global variables are initialized.
     *
     * @return VLC initialization result
     */
    void init(int debugLevl);

    /**
     * Get VLC path.
     *
     * @return the VLC path
     */
    QString vlcPath();

    /**
     * Unload VLC library.
     */
    void vlcUnload();

#if defined(Q_OS_UNIX)
     static bool libGreaterThan(const QString &lhs, const QString &rhs);
#endif // defined(Q_OS_UNIX)

     static QStringList findAllLibVlcPaths();

     QLibrary *m_vlcLibrary;
    libvlc_instance_t *m_vlcInstance;
};

#endif // LIBVLC_H
