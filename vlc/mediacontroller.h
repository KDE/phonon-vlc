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

#ifndef PHONON_VLC_MEDIACONTROLLER_H
#define PHONON_VLC_MEDIACONTROLLER_H

#include <phonon/addoninterface.h>
#include <phonon/objectdescription.h>

struct libvlc_media_player_t;

namespace Phonon
{
namespace VLC
{

/**
 * \brief Interface for AddonInterface.
 *
 * Provides a bridge between Phonon's AddonInterface and VLCMediaController.
 *
 * This class cannot inherit from QObject has MediaObject already inherit from QObject.
 * This is a Qt limitation: there is no possibility to inherit virtual Qobject :/
 * See http://doc.trolltech.com/qq/qq15-academic.html
 * Phonon implementation got the same problem.
 *
 * \see VLCMediaController
 * \see VLCMediaObject
 * \see MediaObject
 */
class MediaController : public AddonInterface
{
public:

    MediaController();
    virtual ~MediaController();

    bool hasInterface(Interface iface) const;

    QVariant interfaceCall(Interface iface, int i_command, const QList<QVariant> & arguments = QList<QVariant>());

    // MediaController signals
    virtual void availableSubtitlesChanged() = 0;
    virtual void availableAudioChannelsChanged() = 0;

    virtual void availableChaptersChanged(int) = 0;
    virtual void availableTitlesChanged(int) = 0;

    virtual void availableAnglesChanged(int i_available_angles) = 0;
    virtual void angleChanged(int i_angle_number) = 0;
    virtual void chapterChanged(int i_chapter_number) = 0;


    void audioChannelAdded(int id, const QString &lang);
    void subtitleAdded(int id, const QString &lang, const QString &type);
    void titleAdded(int id, const QString &name);
    void chapterAdded(int titleId, const QString &name);


protected:

    // AudioChannel
    void setCurrentAudioChannel(const Phonon::AudioChannelDescription &audioChannel);
    QList<Phonon::AudioChannelDescription> availableAudioChannels() const;
    Phonon::AudioChannelDescription currentAudioChannel() const;
    void refreshAudioChannels();

    // Subtitle
    void setCurrentSubtitle(const Phonon::SubtitleDescription &subtitle);
    QList<Phonon::SubtitleDescription> availableSubtitles() const;
    Phonon::SubtitleDescription currentSubtitle() const;
    void refreshSubtitles();

    // Angle
    void setCurrentAngle(int angleNumber);
    int availableAngles() const;
    int currentAngle() const;

    // Chapter
//    void setCurrentChapter( const Phonon::ChapterDescription & chapter );
//    QList<Phonon::ChapterDescription> availableChapters() const;
//    Phonon::ChapterDescription currentChapter() const;
    void setCurrentChapter(int chapterNumber);
    int availableChapters() const;
    int currentChapter() const;
    void refreshChapters(int i_title);

    // Title
//    void setCurrentTitle( const Phonon::TitleDescription & title );
//    QList<Phonon::TitleDescription> availableTitles() const;
//    Phonon::TitleDescription currentTitle() const;
    void setCurrentTitle(int titleNumber);
    int availableTitles() const;
    int currentTitle() const;

    void setAutoplayTitles(bool autoplay);
    bool autoplayTitles() const;

    /**
     * Clear all (i.e availableSubtitles, availableChapters...).
     *
     * This is used each time we restart the video.
     */
    virtual void clearMediaController();

    Phonon::AudioChannelDescription current_audio_channel;
    QList<Phonon::AudioChannelDescription> available_audio_channels;

    Phonon::SubtitleDescription current_subtitle;
    QList<Phonon::SubtitleDescription> available_subtitles;

//    Phonon::ChapterDescription current_chapter;
//    QList<Phonon::ChapterDescription> available_chapters;
    int current_chapter;
    int available_chapters;

//    Phonon::TitleDescription current_title;
//    QList<Phonon::TitleDescription> available_titles;
    int current_title;
    int available_titles;

    int i_current_angle;
    int i_available_angles;

    bool b_autoplay_titles;

    // MediaPlayer
    libvlc_media_player_t *p_vlc_media_player;


private:
};

}
} // Namespace Phonon::VLC

#endif // PHONON_VLC_MEDIACONTROLLER_H
