/*****************************************************************************
 * libVLC backend for the Phonon library                                     *
 *                                                                           *
 * Copyright (C) 2007-2008 Tanguy Krotoff <tkrotoff@gmail.com>               *
 * Copyright (C) 2008 Lukas Durfina <lukas.durfina@gmail.com>                *
 * Copyright (C) 2009 Fathi Boudra <fabo@kde.org>                            *
 * Copyright (C) 2010 Ben Cooksley <sourtooth@gmail.com>                     *
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

#ifndef PHONON_VLC_VLCMEDIAOBJECT_H
#define PHONON_VLC_VLCMEDIAOBJECT_H

#include "vlcmediacontroller.h"

#include "mediaobject.h"

#include <phonon/mediaobjectinterface.h>
#include <phonon/addoninterface.h>

#include <QtCore/QObject>
#include <QtCore/QString>
#include <QtCore/QMultiMap>
#include <QtCore/QList>

namespace Phonon
{
namespace VLC
{

class SinkNode;

/** \brief VLCMediaObject uses libvlc in order to send commands and receive events from VLC.
 *
 * This is the "brain" of the VLC backend.
 *
 * Encapsulates VLC specific code.
 * Take care of libvlc events via libvlc_callback()
 *
 * Inherits the functionality of VLCMediaController, including an owned media player. The
 * signals from MediaController are also here.
 *
 * Objects from libVLC used by this class are MediaPlayer (inherited from VLCMediaController),
 * Media, MediaDiscoverer, and an event manager for each of them.
 *
 * The protected methods that are not implemented in MediaObject are implemented here,
 * using libVLC. There are also public methods from Phonon::MediaObject implemented here.
 *
 * This class captures libVLC events and processes them.
 *
 * The methods addSink() and removeSink() manage sinks connected to the media object.
 *
 * \see MediaObject
 * \see VLCMediaController
 */
class VLCMediaObject : public MediaObject, public VLCMediaController
{
    Q_OBJECT
    Q_INTERFACES(Phonon::MediaObjectInterface  Phonon::AddonInterface)
    friend class SinkNode;

public:

    VLCMediaObject(QObject *parent);
    ~VLCMediaObject();

    void pause();
    void stop();

    bool hasVideo() const;
    bool isSeekable() const;

    qint64 totalTime() const;

    QString errorString() const;

    void addSink(SinkNode *node);
    void removeSink(SinkNode *node);

signals:

    // MediaController signals
    void availableSubtitlesChanged();
    void availableAudioChannelsChanged();

//    void availableChaptersChanged();
//    void availableTitlesChanged();
    void availableChaptersChanged(int);
    void availableTitlesChanged(int);

    void availableAnglesChanged(int availableAngles);
    void angleChanged(int angleNumber);
    void chapterChanged(int chapterNumber);
    void titleChanged(int titleNumber);
    void metaDataNeedsRefresh();
    void durationChanged(qint64 newDuration);

    /**
     * New widget size computed by VLC.
     *
     * It should be applied to the widget that contains the VLC video.
     */
    void videoWidgetSizeChanged(int i_width, int i_height);

protected:

    void loadMediaInternal(const QString &filename);
    void playInternal();
    void seekInternal(qint64 milliseconds);
    void setOption(QString opt);

    qint64 currentTimeInternal() const;

private slots:
    void updateMetaData();
    void updateDuration(qint64 newDuration);

private:

    void connectToMediaVLCEvents();
    void connectToPlayerVLCEvents();

    static void libvlc_callback(const libvlc_event_t *p_event, void *p_user_data);

    void unloadMedia();

    void setVLCVideoWidget();

    // MediaPlayer
//    libvlc_media_player_t * p_vlc_media_player;
    libvlc_event_manager_t *p_vlc_media_player_event_manager;

    // Media
    libvlc_media_t *p_vlc_media;
    libvlc_event_manager_t *p_vlc_media_event_manager;

    // MediaDiscoverer
    libvlc_media_discoverer_t *p_vlc_media_discoverer;
    libvlc_event_manager_t *p_vlc_media_discoverer_event_manager;

    qint64 i_total_time;
    QByteArray p_current_file;
    QMultiMap<QString, QString> p_vlc_meta_data;
    QList<SinkNode *> m_sinks;

    bool b_has_video;

    bool b_seekable;
    qint64 p_seek_point;
};

}
} // Namespace Phonon::VLC

#endif
