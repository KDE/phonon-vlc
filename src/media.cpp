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

#include "media.h"

#include <QtCore/QDebug>

#include <vlc/vlc.h>

#include "utils/debug.h"
#include "utils/libvlc.h"
#include "utils/vstring.h"

namespace Phonon {
namespace VLC {

Media::Media(const QByteArray &mrl, QObject *parent) :
    QObject(parent),
    m_media(libvlc_media_new_location(pvlc_libvlc, mrl.constData())),
    m_mrl(mrl)
{
    Q_ASSERT(m_media);

    libvlc_event_manager_t *manager = libvlc_media_event_manager(m_media);
    libvlc_event_type_t events[] = {
        libvlc_MediaMetaChanged,
        libvlc_MediaSubItemAdded,
        libvlc_MediaDurationChanged,
        libvlc_MediaParsedChanged,
        libvlc_MediaFreed,
        libvlc_MediaStateChanged
    };
    const int eventCount = sizeof(events) / sizeof(*events);
    for (int i = 0; i < eventCount; ++i) {
        libvlc_event_attach(manager, events[i], event_cb, this);
    }
}

Media::~Media()
{
    if (m_media) {
        libvlc_media_release(m_media);
        m_media = 0;
    }
}

void Media::addOption(const QString &option)
{
    libvlc_media_add_option_flag(m_media,
                                 option.toUtf8().data(),
                                 libvlc_media_option_trusted);
}

QString Media::meta(libvlc_meta_t meta)
{
    return VString(libvlc_media_get_meta(m_media, meta)).toQString();
}

void Media::event_cb(const libvlc_event_t *event, void *opaque)
{
    Media *that = reinterpret_cast<Media *>(opaque);
    Q_ASSERT(that);

    switch (event->type) {
    case libvlc_MediaDurationChanged:
        QMetaObject::invokeMethod(
                    that, "durationChanged",
                    Qt::QueuedConnection,
                    Q_ARG(qint64, event->u.media_duration_changed.new_duration));
        break;
    case libvlc_MediaMetaChanged:
        QMetaObject::invokeMethod(
                    that, "metaDataChanged",
                    Qt::QueuedConnection);
        break;
    case libvlc_MediaSubItemAdded:
    case libvlc_MediaParsedChanged:
    case libvlc_MediaFreed:
    case libvlc_MediaStateChanged:
        break;
    }
}

void Media::setCdTrack(int track)
{
    debug() << "setting CDDA track" << track;
    addOption(QLatin1String(":cdda-track="), QVariant(track));
}

} // namespace VLC
} // namespace Phonon
