/*****************************************************************************
 * libVLC backend for the Phonon library                                     *
 *                                                                           *
 * Copyright (C) 2007-2008 Tanguy Krotoff <tkrotoff@gmail.com>               *
 * Copyright (C) 2008 Lukas Durfina <lukas.durfina@gmail.com>                *
 * Copyright (C) 2009 Fathi Boudra <fabo@kde.org>                            *
 * Copyright (C) 2009-2010 vlc-phonon AUTHORS                                *
 * Copyright (C) 2010 Ben Cooksley <sourtooth@gmail.com>                     *
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

#include "vlcmediaobject.h"

#include "videowidget.h"
#include "sinknode.h"

#include "vlcloader.h"

#include <QtCore/QUrl>
#include <QtCore/QTimer>
#include <QtCore/QtDebug>

#include "streamhooks.h"

namespace Phonon
{
namespace VLC
{

/**
 * Creates a libVLC Media Player and connects to it's events. Media and Media Discoverer are
 * not created here. Members are initialized and metaDataNeedsRefresh(), durationChanged() signals
 * are connected to their corresponding slots.
 *
 * \param parent A parent for the QObject
 */
VLCMediaObject::VLCMediaObject(QObject *parent)
    : MediaObject(parent), VLCMediaController()
{
    // Create an empty Media Player object
    p_vlc_media_player = libvlc_media_player_new(vlc_instance);
    if (!p_vlc_media_player) {
        qDebug() << "libvlc exception:" << libvlc_errmsg();
    }
    p_vlc_media_player_event_manager = 0;
    connectToPlayerVLCEvents();

    // Media
    p_vlc_media = 0;
    p_vlc_media_event_manager = 0;

    // Media Discoverer
    p_vlc_media_discoverer = 0;
    p_vlc_media_discoverer_event_manager = 0;

    // default to -1, so that streams won't break and to comply with the docs (-1 if unknown)
    m_totalTime = -1;
    m_hasVideo = false;
    m_seekable = false;
    m_seekpoint = 0;

    connect(this, SIGNAL(metaDataNeedsRefresh()), this, SLOT(updateMetaData()));
    connect(this, SIGNAL(durationChanged(qint64)), this, SLOT(updateDuration(qint64)));
}

/**
 * Uninitializes the media, stops the VLC Media Player, before destroying the media object.
 */
VLCMediaObject::~VLCMediaObject()
{
    unloadMedia();

    libvlc_media_player_stop(p_vlc_media_player); // ensure that we are stopped
    libvlc_media_player_release(p_vlc_media_player);
}

/**
 * Uninitializes the media
 */
void VLCMediaObject::unloadMedia()
{
//    if( p_vlc_media_player ) {
//        libvlc_media_player_release(p_vlc_media_player);
//        p_vlc_media_player = 0;
//    }

    if (p_vlc_media) {
        libvlc_media_release(p_vlc_media);
        p_vlc_media = 0;
    }
}

/**
 * Sets p_current file to the desired filename. It will be loaded in playInternal().
 *
 * \see playInternal()
 */
void VLCMediaObject::loadMediaInternal(const QString &filename)
{
    qDebug() << __FUNCTION__ << filename;

    m_currentFile = QUrl::toPercentEncoding(filename, ":/?=&,");

    // Why is this needed???
    emit stateChanged(Phonon::StoppedState);
}

/**
 * Configures the VLC Media Player to draw the video on the desired widget. The actual function
 * call depends on the platform.
 *
 * \see setVideoWidgetId()
 */
void VLCMediaObject::setVLCVideoWidget()
{
    // Nothing to do if there is no video widget
    if (!m_videoWidget)
        return;

    // Get our media player to use our window
#if defined(Q_OS_MAC)
    libvlc_media_player_set_nsobject(p_vlc_media_player, p_video_widget->cocoaView());
#elif defined(Q_OS_UNIX)
    libvlc_media_player_set_xwindow(p_vlc_media_player, m_videoWidget->winId());
#elif defined(Q_OS_WIN)
    libvlc_media_player_set_hwnd(p_vlc_media_player, p_video_widget->winId());
#endif
}

/**
 * This method actually calls the functions needed to begin playing the media.
 * If another media is already playing, it is discarded. The new media filename is set
 * with loadMediaInternal(). A new VLC Media is created and set into the VLC Media Player.
 * All the connected sink nodes are connected to the new media. It connects the media object
 * to the events for the VLC Media, updates the meta-data, sets up the video widget id, and
 * starts playing.
 *
 * \see loadMediaInternal()
 * \see connectToMediaVLCEvents()
 * \see updateMetaData()
 * \see setVLCWidgetId()
 */
void VLCMediaObject::playInternal()
{
    if (p_vlc_media) {  // We are changing media, discard the old one
        libvlc_media_release(p_vlc_media);
        p_vlc_media = 0;
    }


    m_totalTime = -1;

    // Create a media with the given MRL
    p_vlc_media = libvlc_media_new_location(vlc_instance, m_currentFile);
    if (!p_vlc_media) {
        qDebug() << "libvlc exception:" << libvlc_errmsg();
    }

    if (m_currentFile == "imem://") {
#ifdef _MSC_VER
            char formatstr[] = "0x%p";
#else
            char formatstr[] = "%p";
#endif

            char rptr[64];
            snprintf(rptr, sizeof(rptr), formatstr, streamReadCallback);

            char rdptr[64];
            snprintf(rdptr, sizeof(rdptr), formatstr, streamReadDoneCallback);

            char sptr[64];
            snprintf(sptr, sizeof(sptr), formatstr, streamSeekCallback);

            char srptr[64];
            snprintf(srptr, sizeof(srptr), formatstr, m_streamReader);


            setOption("imem-cat=4");
            setOption(QString("imem-data=%1").arg(srptr));
            setOption(QString("imem-get=%1").arg(rptr));
            setOption(QString("imem-release=%1").arg(rdptr));
            setOption(QString("imem-seek=%1").arg(sptr));

            // if stream has known size, we may pass it
            // imem module will use it and pass it to demux
            if( m_streamReader->streamSize() > 0 )
                setOption(QString("imem-size=%1").arg( m_streamReader->streamSize() ));

    }


    foreach(SinkNode * sink, m_sinks) {
        sink->addToMedia(p_vlc_media);
    }

    // Set the media that will be used by the media player
    libvlc_media_player_set_media(p_vlc_media_player, p_vlc_media);

    // connectToMediaVLCEvents() at the end since it needs to be done for each new libvlc_media_t instance
    connectToMediaVLCEvents();

    // Get meta data (artist, title, etc...)
    updateMetaData();

    // Update available audio channels/subtitles/angles/chapters/etc...
    // i.e everything from MediaController
    // There is no audio channel/subtitle/angle/chapter events inside libvlc
    // so let's send our own events...
    // This will reset the GUI
    clearMediaController();

    // Set up the widget id for libVLC if there is a video widget available
    setVLCVideoWidget();

    // Play
    if (libvlc_media_player_play(p_vlc_media_player)) {
        qDebug() << "libvlc exception:" << libvlc_errmsg();
    }

    if (m_seekpoint != 0) {  // Workaround that seeking needs to work before the file is being played...
        seekInternal(m_seekpoint);
        m_seekpoint = 0;
    }

    emit stateChanged(Phonon::PlayingState);
}

/**
 * Pauses the playback for the media player.
 */
void VLCMediaObject::pause()
{
    libvlc_media_t *media = libvlc_media_player_get_media(p_vlc_media_player);

    if (state() == Phonon::PausedState) {
#ifdef __GNUC__
#warning HACK!!!! -> after loading we are in pause, even though no media is loaded
#endif
        if (media == 0) {
            // Nothing playing yet -> play
            playInternal();
        } else {
            // Resume
            libvlc_media_player_set_pause(p_vlc_media_player, 0);
            emit stateChanged(Phonon::PlayingState);
        }
    } else {
        // Pause
        libvlc_media_player_set_pause(p_vlc_media_player, 1);
        emit stateChanged(Phonon::PausedState);
    }

}

/**
 * Sets the next media source to an empty one and stops playback.
 */
void VLCMediaObject::stop()
{
    m_nextSource = MediaSource(QUrl());
    libvlc_media_player_stop(p_vlc_media_player);
//    unloadMedia();
    emit stateChanged(Phonon::StoppedState);
}

/**
 * Seeks to the required position. If the state is not playing, the seek position is remembered.
 */
void VLCMediaObject::seekInternal(qint64 milliseconds)
{
    if (state() != Phonon::PlayingState) {  // Is we aren't playing, seeking is invalid...
        m_seekpoint = milliseconds;
    }

    qDebug() << __FUNCTION__ << milliseconds;
    libvlc_media_player_set_time(p_vlc_media_player, milliseconds);
}

/**
 * \return An error message with the last libVLC error.
 */
QString VLCMediaObject::errorString() const
{
    return libvlc_errmsg();
}

bool VLCMediaObject::hasVideo() const
{
    return m_hasVideo;
}

bool VLCMediaObject::isSeekable() const
{
    return m_seekable;
}

/**
 * Connect libvlc_callback() to all VLC media player events.
 *
 * \see libvlc_callback()
 * \see connectToMediaVLCEvents()
 */
void VLCMediaObject::connectToPlayerVLCEvents()
{
    // Get the event manager from which the media player send event
    p_vlc_media_player_event_manager = libvlc_media_player_event_manager(p_vlc_media_player);
    libvlc_event_type_t eventsMediaPlayer[] = {
        libvlc_MediaPlayerPlaying,
        libvlc_MediaPlayerPaused,
        libvlc_MediaPlayerEndReached,
        libvlc_MediaPlayerStopped,
        libvlc_MediaPlayerEncounteredError,
        libvlc_MediaPlayerTimeChanged,
        libvlc_MediaPlayerTitleChanged,
        //libvlc_MediaPlayerPositionChanged, // What does this event do???
        libvlc_MediaPlayerSeekableChanged,
        //libvlc_MediaPlayerPausableChanged, // Phonon has no use for this
    };
    int i_nbEvents = sizeof(eventsMediaPlayer) / sizeof(*eventsMediaPlayer);
    for (int i = 0; i < i_nbEvents; i++) {
        libvlc_event_attach(p_vlc_media_player_event_manager, eventsMediaPlayer[i],
                            libvlc_callback, this);
    }
}

/**
 * Connect libvlc_callback() to all VLC media events.
 *
 * \see libvlc_callback()
 * \see connectToPlayerVLCEvents()
 */
void VLCMediaObject::connectToMediaVLCEvents()
{
    // Get event manager from media descriptor object
    p_vlc_media_event_manager = libvlc_media_event_manager(p_vlc_media);
    libvlc_event_type_t eventsMedia[] = {
        libvlc_MediaMetaChanged,
        //libvlc_MediaSubItemAdded, // Could this be used for Audio Channels / Subtitles / Chapter info??
        libvlc_MediaDurationChanged,
        //libvlc_MediaFreed, // Not needed is this?
        //libvlc_MediaStateChanged, // We don't use this? Could we??
    };
    int i_nbEvents = sizeof(eventsMedia) / sizeof(*eventsMedia);
    for (int i = 0; i < i_nbEvents; i++) {
        libvlc_event_attach(p_vlc_media_event_manager, eventsMedia[i], libvlc_callback, this);
    }

    // Get event manager from media service discoverer object
    // FIXME why libvlc_media_discoverer_event_manager() does not take a libvlc_exception_t ?
//    p_vlc_media_discoverer_event_manager = libvlc_media_discoverer_event_manager(p_vlc_media_discoverer);
//    libvlc_event_type_t eventsMediaDiscoverer[] = {
//        libvlc_MediaDiscovererStarted,
//        libvlc_MediaDiscovererEnded
//    };
//    nbEvents = sizeof(eventsMediaDiscoverer) / sizeof(*eventsMediaDiscoverer);
//    for (int i = 0; i < nbEvents; i++) {
//        libvlc_event_attach(p_vlc_media_discoverer_event_manager, eventsMediaDiscoverer[i], libvlc_callback, this, vlc_exception);
//    }
}

/**
 * Libvlc callback.
 *
 * Receive all vlc events.
 *
 * Most of the events trigger the emission of one or more signals from the media object.
 *
 * \warning This code will be owned by the libVLC thread.
 *
 * \see connectToMediaVLCEvents()
 * \see connectToPlayerVLCEvents()
 * \see libvlc_event_attach()
 */
void VLCMediaObject::libvlc_callback(const libvlc_event_t *p_event, void *p_user_data)
{
    static int i_first_time_media_player_time_changed = 0;
    static bool b_media_player_title_changed = false;

    VLCMediaObject *const p_vlc_mediaObject = (VLCMediaObject *) p_user_data;

//    qDebug() << (int)p_vlc_mediaObject << "event:" << libvlc_event_type_name(p_event->type);

    // Media player events
    if (p_event->type == libvlc_MediaPlayerSeekableChanged) {
        const bool b_seekable = libvlc_media_player_is_seekable(p_vlc_mediaObject->p_vlc_media_player);
        if (b_seekable != p_vlc_mediaObject->m_seekable) {
            p_vlc_mediaObject->m_seekable = b_seekable;
            emit p_vlc_mediaObject->seekableChanged(p_vlc_mediaObject->m_seekable);
        }
    }
    if (p_event->type == libvlc_MediaPlayerTimeChanged) {

        i_first_time_media_player_time_changed++;

        // FIXME It is ugly. It should be solved by some events in libvlc
        if (i_first_time_media_player_time_changed == 15) {
            // Update metadata
            p_vlc_mediaObject->updateMetaData();

            // Get current video width and height
            uint width = 0;
            uint height = 0;
            libvlc_video_get_size(p_vlc_mediaObject->p_vlc_media_player, 0, &width, &height);
            emit p_vlc_mediaObject->videoWidgetSizeChanged(width, height);

            // Does this media player have a video output
            const bool b_has_video = libvlc_media_player_has_vout(p_vlc_mediaObject->p_vlc_media_player);
            if (b_has_video != p_vlc_mediaObject->m_hasVideo) {
                p_vlc_mediaObject->m_hasVideo = b_has_video;
                emit p_vlc_mediaObject->hasVideoChanged(p_vlc_mediaObject->m_hasVideo);
            }

            if (b_has_video) {
                // Give info about audio tracks
                p_vlc_mediaObject->refreshAudioChannels();
                // Give info about subtitle tracks
                p_vlc_mediaObject->refreshSubtitles();

                // Get movie chapter count
                // It is not a title/chapter media if there is no chapter
                if (libvlc_media_player_get_chapter_count(
                            p_vlc_mediaObject->p_vlc_media_player) > 0) {
                    // Give info about title
                    // only first time, no when title changed
                    if (!b_media_player_title_changed) {
                        libvlc_track_description_t *p_info = libvlc_video_get_title_description(
                                p_vlc_mediaObject->p_vlc_media_player);
                        while (p_info) {
                            p_vlc_mediaObject->titleAdded(p_info->i_id, p_info->psz_name);
                            p_info = p_info->p_next;
                        }
                        libvlc_track_description_release(p_info);
                    }

                    // Give info about chapters for actual title 0
                    if (b_media_player_title_changed)
                        p_vlc_mediaObject->refreshChapters(libvlc_media_player_get_title(
                                                               p_vlc_mediaObject->p_vlc_media_player));
                    else {
                        p_vlc_mediaObject->refreshChapters(0);
                    }
                }
                if (b_media_player_title_changed) {
                    b_media_player_title_changed = false;
                }
            }

            // Bugfix with Qt mediaplayer example
            // Now we are in playing state
            emit p_vlc_mediaObject->stateChanged(Phonon::PlayingState);
        }

        emit p_vlc_mediaObject->tickInternal(p_vlc_mediaObject->currentTime());
    }

    if (p_event->type == libvlc_MediaPlayerPlaying) {
        if (p_vlc_mediaObject->state() != Phonon::BufferingState) {
            // Bugfix with Qt mediaplayer example
            emit p_vlc_mediaObject->stateChanged(Phonon::PlayingState);
        }
    }

    if (p_event->type == libvlc_MediaPlayerPaused) {
        emit p_vlc_mediaObject->stateChanged(Phonon::PausedState);
    }

    if (p_event->type == libvlc_MediaPlayerEndReached && !p_vlc_mediaObject->checkGaplessWaiting()) {
        i_first_time_media_player_time_changed = 0;
        p_vlc_mediaObject->clearMediaController();
        p_vlc_mediaObject->emitAboutToFinish();
        emit p_vlc_mediaObject->finished();
        emit p_vlc_mediaObject->stateChanged(Phonon::StoppedState);
    } else if (p_event->type == libvlc_MediaPlayerEndReached) {
        emit p_vlc_mediaObject->moveToNext();
    }

    if (p_event->type == libvlc_MediaPlayerEncounteredError && !p_vlc_mediaObject->checkGaplessWaiting()) {
        i_first_time_media_player_time_changed = 0;
        p_vlc_mediaObject->clearMediaController();
        emit p_vlc_mediaObject->finished();
        emit p_vlc_mediaObject->stateChanged(Phonon::ErrorState);
    } else if (p_event->type == libvlc_MediaPlayerEncounteredError) {
        emit p_vlc_mediaObject->moveToNext();
    }

    if (p_event->type == libvlc_MediaPlayerStopped && !p_vlc_mediaObject->checkGaplessWaiting()) {
        i_first_time_media_player_time_changed = 0;
        p_vlc_mediaObject->clearMediaController();
        emit p_vlc_mediaObject->stateChanged(Phonon::StoppedState);
    }

    if (p_event->type == libvlc_MediaPlayerTitleChanged) {
        i_first_time_media_player_time_changed = 0;
        b_media_player_title_changed = true;
    }

    // Media events

    if (p_event->type == libvlc_MediaDurationChanged) {
        emit p_vlc_mediaObject->durationChanged(p_event->u.media_duration_changed.new_duration);
    }

    if (p_event->type == libvlc_MediaMetaChanged) {
        emit p_vlc_mediaObject->metaDataNeedsRefresh();
    }
}

/**
 * Retrieve meta data of a file (i.e ARTIST, TITLE, ALBUM, etc...).
 */
void VLCMediaObject::updateMetaData()
{
    QMultiMap<QString, QString> metaDataMap;

    const char *artist = libvlc_media_get_meta(p_vlc_media, libvlc_meta_Artist);
    const char *title = libvlc_media_get_meta(p_vlc_media, libvlc_meta_Title);
    const char *nowplaying = libvlc_media_get_meta(p_vlc_media, libvlc_meta_NowPlaying);

    // Streams sometimes have the artist and title munged in nowplaying.
    // With ALBUM = Title and TITLE = NowPlaying it will still show up nicely in Amarok.
    if (qstrlen(artist) == 0 && qstrlen(nowplaying) > 0) {
        metaDataMap.insert(QLatin1String("ALBUM"),
                        QString::fromUtf8(title));
        metaDataMap.insert(QLatin1String("TITLE"),
                        QString::fromUtf8(nowplaying));
    } else {
        metaDataMap.insert(QLatin1String("ALBUM"),
                        QString::fromUtf8(libvlc_media_get_meta(p_vlc_media, libvlc_meta_Album)));
        metaDataMap.insert(QLatin1String("TITLE"),
                        QString::fromUtf8(title));
    }

    metaDataMap.insert(QLatin1String("ARTIST"),
                       QString::fromUtf8(artist));
    metaDataMap.insert(QLatin1String("DATE"),
                       QString::fromUtf8(libvlc_media_get_meta(p_vlc_media, libvlc_meta_Date)));
    metaDataMap.insert(QLatin1String("GENRE"),
                       QString::fromUtf8(libvlc_media_get_meta(p_vlc_media, libvlc_meta_Genre)));
    metaDataMap.insert(QLatin1String("TRACKNUMBER"),
                       QString::fromUtf8(libvlc_media_get_meta(p_vlc_media, libvlc_meta_TrackNumber)));
    metaDataMap.insert(QLatin1String("DESCRIPTION"),
                       QString::fromUtf8(libvlc_media_get_meta(p_vlc_media, libvlc_meta_Description)));
    metaDataMap.insert(QLatin1String("COPYRIGHT"),
                       QString::fromUtf8(libvlc_media_get_meta(p_vlc_media, libvlc_meta_Copyright)));
    metaDataMap.insert(QLatin1String("URL"),
                       QString::fromUtf8(libvlc_media_get_meta(p_vlc_media, libvlc_meta_URL)));
    metaDataMap.insert(QLatin1String("ENCODEDBY"),
                       QString::fromUtf8(libvlc_media_get_meta(p_vlc_media, libvlc_meta_EncodedBy)));

    if (metaDataMap == m_vlcMetaData) {
        // No need to issue any change, the data is the same
        return;
    }

    m_vlcMetaData = metaDataMap;

    emit metaDataChanged(metaDataMap);
}

qint64 VLCMediaObject::totalTime() const
{
    return m_totalTime;
}

qint64 VLCMediaObject::currentTimeInternal() const
{
    return libvlc_media_player_get_time(p_vlc_media_player);
}

/**
 * Update media duration time
 */
void VLCMediaObject::updateDuration(qint64 newDuration)
{
    // If its within 5ms of the current total time, don't bother....
    if (newDuration - 5 > m_totalTime || newDuration + 5 < m_totalTime) {
        qDebug() << __FUNCTION__ << "Length changing from " << m_totalTime
                 << " to " << newDuration;
        m_totalTime = newDuration;
        emit totalTimeChanged(m_totalTime);
    }
}

/**
 * Adds a sink for this media object. During playInternal(), all the sinks
 * will have their addToMedia() called.
 *
 * \see playInternal()
 * \see SinkNode::addToMedia()
 */
void VLCMediaObject::addSink(SinkNode *node)
{
    if (m_sinks.contains(node)) {
        // This shouldn't happen....
        return;
    }
    m_sinks.append(node);
}

/**
 * Removes a sink from this media object.
 */
void VLCMediaObject::removeSink(SinkNode *node)
{
    if( node != NULL )
        m_sinks.removeAll(node);
}

/**
 * Adds an option to the libVLC media.
 *
 * \param opt What option to add
 */
void VLCMediaObject::setOption(QString opt)
{
    Q_ASSERT(p_vlc_media);
    qDebug() << Q_FUNC_INFO << opt;
    libvlc_media_add_option_flag(p_vlc_media, opt.toLocal8Bit(), libvlc_media_option_trusted);
}

}
} // Namespace Phonon::VLC
