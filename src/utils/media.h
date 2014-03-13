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

#ifndef PHONON_VLC_MEDIA_H
#define PHONON_VLC_MEDIA_H

#include <QtCore/QObject>
#include <QtCore/QStringBuilder>
#include <QtCore/QVariant>

#include <vlc/libvlc.h>
#include <vlc/libvlc_media.h>

#define INTPTR_PTR(x) reinterpret_cast<intptr_t>(x)
#define INTPTR_FUNC(x) reinterpret_cast<intptr_t>(&x)

namespace Phonon {
namespace VLC {

class Media : public QObject
{
    Q_OBJECT
public:
    explicit Media(const QByteArray &mrl, QObject *parent = 0);
    ~Media();

    inline libvlc_media_t *libvlc_media() const { return m_media; }
    inline operator libvlc_media_t *() const { return m_media; }

    inline void addOption(const QString &option, const QVariant &argument)
    {
        addOption(option % argument.toString());
    }

    inline void addOption(const QString &option, intptr_t functionPtr)
    {
        QString optionWithPtr = option;
        optionWithPtr.append(QString::number(static_cast<qint64>(functionPtr)));
        addOption(optionWithPtr);
    }

    void addOption(const QString &option);

    QString meta(libvlc_meta_t meta);

    void setCdTrack(int track);

signals:
    void durationChanged(qint64 duration);
    void metaDataChanged();

private:
    static void event_cb(const libvlc_event_t *event, void *opaque);

    libvlc_media_t *m_media;
    libvlc_state_t m_state;
    QByteArray m_mrl;
};

} // namespace VLC
} // namespace Phonon

#endif // PHONON_VLC_MEDIA_H
