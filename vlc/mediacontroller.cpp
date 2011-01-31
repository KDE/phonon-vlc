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

#include "mediacontroller.h"

#include <vlc/vlc.h>

namespace Phonon
{
namespace VLC
{

MediaController::MediaController()
    : p_vlc_media_player(0)
{
    clearMediaController();
}

MediaController::~MediaController()
{
}

bool MediaController::hasInterface(Interface iface) const
{
    switch (iface) {
    case AddonInterface::NavigationInterface:
        return true;
        break;
    case AddonInterface::ChapterInterface:
        return true;
        break;
    case AddonInterface::AngleInterface:
        return true;
        break;
    case AddonInterface::TitleInterface:
        return true;
        break;
    case AddonInterface::SubtitleInterface:
        return true;
        break;
    case AddonInterface::AudioChannelInterface:
        return true;
        break;
    default:
        qCritical() << __FUNCTION__
                    << "Error: unsupported AddonInterface::Interface"
                    << iface;
    }

    return false;
}

QVariant MediaController::interfaceCall(Interface iface, int i_command, const QList<QVariant> & arguments)
{
    switch (iface) {
    case AddonInterface::ChapterInterface:
        switch (static_cast<AddonInterface::ChapterCommand>(i_command)) {
//        case AddonInterface::availableChapters:
//            return QVariant::fromValue(availableChapters());
        case AddonInterface::availableChapters:
            return availableChapters();
//        case AddonInterface::currentChapter:
//            return QVariant::fromValue(currentChapter());
        case AddonInterface::chapter:
            return currentChapter();
//        case AddonInterface::setCurrentChapter:
//            if( arguments.isEmpty() || !arguments.first().canConvert<ChapterDescription>()) {
//                    qCritical() << __FUNCTION__ << "Error: arguments invalid";
//                    return false;
//                }
//            setCurrentChapter(arguments.first().value<ChapterDescription>());
//            return true;
        case AddonInterface::setChapter:
            if (arguments.isEmpty() || !arguments.first().canConvert(QVariant::Int)) {
                qCritical() << __FUNCTION__ << "Error: arguments invalid";
                return false;
            }
            setCurrentChapter(arguments.first().toInt());
            return true;
        default:
            qCritical() << __FUNCTION__
                        << "Error: unsupported AddonInterface::ChapterInterface command:"
                        << i_command;
        }
        break;
    case AddonInterface::TitleInterface:
        switch (static_cast<AddonInterface::TitleCommand>(i_command)) {
//        case AddonInterface::availableTitles:
//            return QVariant::fromValue(availableTitles());
        case AddonInterface::availableTitles:
            return availableTitles();
//        case AddonInterface::currentTitle:
//            return QVariant::fromValue(currentTitle());
        case AddonInterface::title:
            return currentTitle();
//        case AddonInterface::setCurrentTitle:
//            if( arguments.isEmpty() || !arguments.first().canConvert<TitleDescription>()) {
//                    qCritical() << __FUNCTION__ << "Error: arguments invalid";
//                    return false;
//            }
//            setCurrentTitle(arguments.first().value<TitleDescription>());
//            return true;
        case AddonInterface::setTitle:
            if (arguments.isEmpty() || !arguments.first().canConvert(QVariant::Int)) {
                qCritical() << __FUNCTION__ << "Error: arguments invalid";
                return false;
            }
            setCurrentTitle(arguments.first().toInt());
            return true;
        case AddonInterface::autoplayTitles:
            return autoplayTitles();
        case AddonInterface::setAutoplayTitles:
            if (arguments.isEmpty() || !arguments.first().canConvert(QVariant::Bool)) {
                qCritical() << __FUNCTION__ << "Error: arguments invalid";
                return false;
            }
            setAutoplayTitles(arguments.first().toBool());
            return true;
        default:
            qCritical() << __FUNCTION__
                        << "Error: unsupported AddonInterface::TitleInterface command:"
                        << i_command;
        }
        break;
    case AddonInterface::AngleInterface:
        switch (static_cast<AddonInterface::AngleCommand>(i_command)) {
        case AddonInterface::availableAngles:
        case AddonInterface::angle:
        case AddonInterface::setAngle:
            break;
        default:
            qCritical() << __FUNCTION__
                        << "Error: unsupported AddonInterface::AngleInterface command:"
                        << i_command;
        }
        break;
    case AddonInterface::SubtitleInterface:
        switch (static_cast<AddonInterface::SubtitleCommand>(i_command)) {
        case AddonInterface::availableSubtitles:
            return QVariant::fromValue(availableSubtitles());
        case AddonInterface::currentSubtitle:
            return QVariant::fromValue(currentSubtitle());
        case AddonInterface::setCurrentSubtitle:
            if (arguments.isEmpty() || !arguments.first().canConvert<SubtitleDescription>()) {
                qCritical() << __FUNCTION__ << "Error: arguments invalid";
                return false;
            }
            setCurrentSubtitle(arguments.first().value<SubtitleDescription>());
            return true;
        default:
            qCritical() << __FUNCTION__
                        << "Error: unsupported AddonInterface::SubtitleInterface command:"
                        << i_command;
        }
        break;
    case AddonInterface::AudioChannelInterface:
        switch (static_cast<AddonInterface::AudioChannelCommand>(i_command)) {
        case AddonInterface::availableAudioChannels:
            return QVariant::fromValue(availableAudioChannels());
        case AddonInterface::currentAudioChannel:
            return QVariant::fromValue(currentAudioChannel());
        case AddonInterface::setCurrentAudioChannel:
            if (arguments.isEmpty() || !arguments.first().canConvert<AudioChannelDescription>()) {
                qCritical() << __FUNCTION__ << "Error: arguments invalid";
                return false;
            }
            setCurrentAudioChannel(arguments.first().value<AudioChannelDescription>());
            return true;
        default:
            qCritical() << __FUNCTION__
                        << "Error: unsupported AddonInterface::AudioChannelInterface command:"
                        << i_command;
        }
        break;
    default:
        qCritical() << __FUNCTION__
                    << "Error: unsupported AddonInterface::Interface:"
                    << iface;
    }

    return QVariant();
}





void MediaController::clearMediaController()
{
    current_audio_channel = Phonon::AudioChannelDescription();
    available_audio_channels.clear();

    current_subtitle = Phonon::SubtitleDescription();
    available_subtitles.clear();

    i_current_angle = 0;
    i_available_angles = 0;

//    current_chapter = Phonon::ChapterDescription();
//    available_chapters.clear();
    current_chapter = 0;
    available_chapters = 0;

//    current_title = Phonon::TitleDescription();
//    available_titles.clear();
    current_title = 0;
    available_titles = 0;

    b_autoplay_titles = false;

    emit availableAudioChannelsChanged();
    emit availableSubtitlesChanged();
    emit availableTitlesChanged(0);
    emit availableChaptersChanged(0);
}

// Add audio channel -> in libvlc it is track, it means audio in another language
void MediaController::audioChannelAdded(int id, const QString &lang)
{
    QHash<QByteArray, QVariant> properties;
    properties.insert("name", lang);
    properties.insert("description", "");

    available_audio_channels << Phonon::AudioChannelDescription(id, properties);
    emit availableAudioChannelsChanged();
}

// Add subtitle
void MediaController::subtitleAdded(int id, const QString &lang, const QString &type)
{
    QHash<QByteArray, QVariant> properties;
    properties.insert("name", lang);
    properties.insert("description", "");
    properties.insert("type", type);

    available_subtitles << Phonon::SubtitleDescription(id, properties);
    emit availableSubtitlesChanged();
}

// Add title
void MediaController::titleAdded(int id, const QString &name)
{
//    QHash<QByteArray, QVariant> properties;
//    properties.insert("name", name);
//    properties.insert("description", "");

//    available_titles << Phonon::TitleDescription(id, properties);
    available_titles++;
    emit availableTitlesChanged(available_titles);
}

// Add chapter
void MediaController::chapterAdded(int titleId, const QString &name)
{
//    QHash<QByteArray, QVariant> properties;
//    properties.insert("name", name);
//    properties.insert("description", "");

//    available_chapters << Phonon::ChapterDescription(titleId, properties);
    available_chapters++;
    emit availableChaptersChanged(available_chapters);
}

// Audio channel

void MediaController::setCurrentAudioChannel(const Phonon::AudioChannelDescription &audioChannel)
{
    current_audio_channel = audioChannel;
    if (libvlc_audio_set_track(p_vlc_media_player, audioChannel.index())) {
        qDebug() << "libvlc exception:" << libvlc_errmsg();
    }
}

QList<Phonon::AudioChannelDescription> MediaController::availableAudioChannels() const
{
    return available_audio_channels;
}

Phonon::AudioChannelDescription MediaController::currentAudioChannel() const
{
    return current_audio_channel;
}

void MediaController::refreshAudioChannels()
{
    current_audio_channel = Phonon::AudioChannelDescription();
    available_audio_channels.clear();

    libvlc_track_description_t *p_info = libvlc_audio_get_track_description(p_vlc_media_player);
    while (p_info) {
        audioChannelAdded(p_info->i_id, p_info->psz_name);
        p_info = p_info->p_next;
    }
    libvlc_track_description_release(p_info);
}

// Subtitle

void MediaController::setCurrentSubtitle(const Phonon::SubtitleDescription &subtitle)
{
    current_subtitle = subtitle;
//    int id = current_subtitle.index();
    QString type = current_subtitle.property("type").toString();

    if (type == "file") {
        QString filename = current_subtitle.property("name").toString();
        if (!filename.isEmpty()) {
            if (!libvlc_video_set_subtitle_file(p_vlc_media_player,
                                                filename.toAscii().data())) {
                qDebug() << "libvlc exception:" << libvlc_errmsg();
            }

            // There is no subtitle event inside libvlc so let's send our own event...
            available_subtitles << current_subtitle;
            emit availableSubtitlesChanged();
        }
    } else {
        if (libvlc_video_set_spu(p_vlc_media_player, subtitle.index())) {
            qDebug() << "libvlc exception:" << libvlc_errmsg();
        }
    }
}

QList<Phonon::SubtitleDescription> MediaController::availableSubtitles() const
{
    return available_subtitles;
}

Phonon::SubtitleDescription MediaController::currentSubtitle() const
{
    return current_subtitle;
}

void MediaController::refreshSubtitles()
{
    current_subtitle = Phonon::SubtitleDescription();
    available_subtitles.clear();

    libvlc_track_description_t *p_info = libvlc_video_get_spu_description(
            p_vlc_media_player);
    while (p_info) {
        subtitleAdded(p_info->i_id, p_info->psz_name, "");
        p_info = p_info->p_next;
    }
    libvlc_track_description_release(p_info);
}

// Title

//void MediaController::setCurrentTitle( const Phonon::TitleDescription & title )
void MediaController::setCurrentTitle(int title)
{
    current_title = title;

//    libvlc_media_player_set_title(p_vlc_media_player, title.index(), vlc_exception);
    libvlc_media_player_set_title(p_vlc_media_player, title);
}

//QList<Phonon::TitleDescription> MediaController::availableTitles() const
int MediaController::availableTitles() const
{
    return available_titles;
}

//Phonon::TitleDescription MediaController::currentTitle() const
int MediaController::currentTitle() const
{
    return current_title;
}

void MediaController::setAutoplayTitles(bool autoplay)
{
    b_autoplay_titles = autoplay;
}

bool MediaController::autoplayTitles() const
{
    return b_autoplay_titles;
}

// Chapter

//void MediaController::setCurrentChapter(const Phonon::ChapterDescription &chapter)
void MediaController::setCurrentChapter(int chapter)
{
    current_chapter = chapter;
//    libvlc_media_player_set_chapter(p_vlc_media_player, chapter.index(), vlc_exception);
    libvlc_media_player_set_chapter(p_vlc_media_player, chapter);
}

//QList<Phonon::ChapterDescription> MediaController::availableChapters() const
int MediaController::availableChapters() const
{
    return available_chapters;
}

//Phonon::ChapterDescription MediaController::currentChapter() const
int MediaController::currentChapter() const
{
    return current_chapter;
}

// We need to rebuild available chapters when title is changed
void MediaController::refreshChapters(int i_title)
{
//    current_chapter = Phonon::ChapterDescription();
//    available_chapters.clear();
    current_chapter = 0;
    available_chapters = 0;

    // Get the description of available chapters for specific title
    libvlc_track_description_t *p_info = libvlc_video_get_chapter_description(
            p_vlc_media_player, i_title);
    while (p_info) {
        chapterAdded(p_info->i_id, p_info->psz_name);
        p_info = p_info->p_next;
    }
    libvlc_track_description_release(p_info);
}

// Angle

void MediaController::setCurrentAngle(int angleNumber)
{
    i_current_angle = angleNumber;
}

int MediaController::availableAngles() const
{
    return i_available_angles;
}

int MediaController::currentAngle() const
{
    return i_current_angle;
}

}
} // Namespace Phonon::VLC
