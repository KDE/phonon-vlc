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

#ifndef PHONON_VLC_SEEKSTACK_H
#define PHONON_VLC_SEEKSTACK_H

#include "mediaobject.h"

#include <QtCore/QObject>
#include <QtCore/QStack>

class QTimer;

namespace Phonon
{
namespace VLC {

/** \brief Special stack for seek commands
 *
 * It is a queue of seek commands. The stack is popped at a constant interval.
 * After one seek command is popped, the others are discarded.
 */
class SeekStack : public QObject
{
    Q_OBJECT

public:

    SeekStack(MediaObject *mediaObject);
    ~SeekStack();

    void pushSeek(qint64 milliseconds);

signals:

private slots:

    void popSeek();

    void reconnectTickSignal();

private:

    /**
     * The parent media object for the seek stack
     */
    MediaObject *p_media_object;

    /**
     * Timer for popping the stack
     */
    QTimer *p_timer;

    /**
     * The stack containing seek timings. Only the top timing is relevant.
     */
    QStack<qint64> stack;
};

}
} // Namespace Phonon::VLC

#endif // PHONON_VLC_SEEKSTACK_H
