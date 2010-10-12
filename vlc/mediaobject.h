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

#include <phonon/mediaobjectinterface.h>

#include <QtCore/QObject>
#include <QtGui/QWidget>

#include "streamreader.h"

// for BaseWidget
#include "overlaywidget.h"


namespace Phonon
{
namespace VLC
{

class SeekStack;

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
 * inherited by that class, like loadMediaInternal(), playInternal(), seekInternal().
 * These methods have no implementation here.
 *
 * For documentation regarding the methods implemented for MediaObjectInterface, see
 * the Phonon documentation.
 *
 * \see Phonon::MediaObjectInterface
 * \see VLCMediaObject
 */
class MediaObject : public QObject, public MediaObjectInterface
{
    Q_OBJECT
    friend class SeekStack;

public:

    MediaObject(QObject *p_parent);
    virtual ~MediaObject();

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

    virtual void loadMediaInternal(const QString &filename) = 0;
    virtual void playInternal() = 0;
    virtual void seekInternal(qint64 milliseconds) = 0;

    virtual qint64 currentTimeInternal() const = 0;
    virtual void setOption(QString opt) = 0;

    bool checkGaplessWaiting();

    BaseWidget *p_video_widget;
    MediaSource p_next_source;

private slots:

    void stateChangedInternal(Phonon::State newState);

    void tickInternalSlot(qint64 time);

    void moveToNextSource();

private:

    void loadMedia(const QString &filename);
    void loadStream();

    void resume();

    QString PhononStateToString(Phonon::State newState);

    MediaSource mediaSource;
    StreamReader *streamReader;
    Phonon::State currentState;

    qint32 i_prefinish_mark;
    bool b_prefinish_mark_reached_emitted;

    bool b_about_to_finish_emitted;

    qint32 i_tick_interval;
    qint32 i_transition_time;
};

}
} // Namespace Phonon::VLC

#endif // PHONON_VLC_MEDIAOBJECT_H
