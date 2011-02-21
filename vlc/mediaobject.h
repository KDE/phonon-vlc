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

#ifndef PHONON_VLC_MEDIAOBJECT_H
#define PHONON_VLC_MEDIAOBJECT_H

#include <QtCore/QObject>
#include <phonon/mediaobjectinterface.h>
#include <phonon/addoninterface.h>
#include "mediacontroller.h"

#include <QtGui/QWidget>

#include "streamreader.h"

// for BaseWidget
#include "overlaywidget.h"

struct libvlc_event_t;
struct libvlc_event_manager_t;
struct libvlc_media_t;
struct libvlc_media_discoverer_t;

namespace Phonon
{
namespace VLC
{

class SeekStack;
class SinkNode;

/** \brief Implementation for the most important class in Phonon
 *
 * The MediaObject class is the workhorse for Phonon. It handles what is needed
 * to play media. It has a media source object used to configure what media to
 * play.
 *
 * It provides the essential methods setSource(), play(), seek() and additional
 * methods to configure the next media source, the transition between sources,
 * transition times, ticks, other.
 *
 * There are numerous signals that provide information about the state of the media
 * and of the playing process. The aboutToFinish() and finished() signals are used
 * to see when the current media is finished.
 *
 * This class does not contain methods directly involved with libVLC. This part is
 * handled by the VLCMediaObject class. There are protected methods and slots
 * inherited by that class, like playInternal(), seekInternal().
 * These methods have no implementation here.
 *
 * For documentation regarding the methods implemented for MediaObjectInterface, see
 * the Phonon documentation.
 *
 * \see Phonon::MediaObjectInterface
 * \see VLCMediaObject
 */
class MediaObject : public QObject, public MediaObjectInterface, public MediaController
{
    Q_OBJECT
    Q_INTERFACES(Phonon::MediaObjectInterface Phonon::AddonInterface)
    friend class SinkNode;
    friend class SeekStack;

public:
    MediaObject(QObject *p_parent);
    virtual ~MediaObject();

    void pause();
    void stop();

    bool hasVideo() const;
    bool isSeekable() const;

    qint64 totalTime() const;

    QString errorString() const;

    void addSink(SinkNode *node);
    void removeSink(SinkNode *node);

    /**
     * Widget Id where VLC will show the videos.
     */
//    void setVideoWidgetId(WId i_widget_id);

    void setVideoWidget(BaseWidget *widget);

    void play();
    void seek(qint64 milliseconds);

    qint32 tickInterval() const;
    void setTickInterval(qint32 tickInterval);

    qint64 currentTime() const;
    Phonon::State state() const;
    Phonon::ErrorType errorType() const;
    MediaSource source() const;
    void setSource(const MediaSource &source);
    void setNextSource(const MediaSource &source);

    qint32 prefinishMark() const;
    void setPrefinishMark(qint32 msecToEnd);

    qint32 transitionTime() const;
    void setTransitionTime(qint32);

    void emitAboutToFinish();

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

    void aboutToFinish();
    void bufferStatus(int i_percent_filled);
    void currentSourceChanged(const MediaSource &newSource);
    void finished();
    void hasVideoChanged(bool b_has_video);
    void metaDataChanged(const QMultiMap<QString, QString> & metaData);
    void prefinishMarkReached(qint32 msecToEnd);
    void seekableChanged(bool b_is_seekable);
    void stateChanged(Phonon::State newState, Phonon::State oldState);
    void tick(qint64 time);
    void totalTimeChanged(qint64 newTotalTime);

    // Signal from VLCMediaObject
    void stateChanged(Phonon::State newState);
    void moveToNext();
    void playbackCommenced();

    void tickInternal(qint64 time);

protected:
    void playInternal();
    void seekInternal(qint64 milliseconds);
    void setOption(QString opt);

    qint64 currentTimeInternal() const;

    bool checkGaplessWaiting();

    BaseWidget *m_videoWidget;
    MediaSource m_nextSource;

    MediaSource m_mediaSource;
    StreamReader *m_streamReader;
    Phonon::State m_currentState;

private slots:
    void updateMetaData();
    void updateDuration(qint64 newDuration);

    void stateChangedInternal(Phonon::State newState);

    void tickInternalSlot(qint64 time);

    void moveToNextSource();

private:
    void connectToMediaVLCEvents();
    void connectToPlayerVLCEvents();

    static void libvlc_callback(const libvlc_event_t *p_event, void *p_user_data);

    void loadMedia(const QString &filename);
    void unloadMedia();
    void loadStream();

    void setVLCVideoWidget();

    void resume();

    QString PhononStateToString(Phonon::State newState);

    qint32 m_prefinishMark;
    bool m_prefinishEmitted;

    bool m_aboutToFinishEmitted;

    qint32 m_tickInterval;
    qint32 m_transitionTime;

    // MediaPlayer
//    libvlc_media_player_t * p_vlc_media_player;
    libvlc_event_manager_t *m_eventManager;

    // Media
    libvlc_media_t *p_vlc_media;
    libvlc_event_manager_t *p_vlc_media_event_manager;

    // MediaDiscoverer
    libvlc_media_discoverer_t *p_vlc_media_discoverer;
    libvlc_event_manager_t *p_vlc_media_discoverer_event_manager;

    qint64 m_totalTime;
    QByteArray m_currentFile;
    QMultiMap<QString, QString> m_vlcMetaData;
    QList<SinkNode *> m_sinks;

    bool m_hasVideo;

    bool m_seekable;
    qint64 m_seekpoint;
};

}
} // Namespace Phonon::VLC

#endif // PHONON_VLC_MEDIAOBJECT_H
