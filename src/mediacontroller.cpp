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

#include <phonon/GlobalDescriptionContainer>

#include "utils/debug.h"
#include "utils/libvlc.h"
#include "mediaplayer.h"

namespace Phonon {
namespace VLC {

#ifdef __GNUC__
#warning titles and chapters not covered by globaldescriptioncontainer!!
#endif

MediaController::MediaController()
    : m_player(0)
{
    GlobalSubtitles::instance()->register_(this);
    GlobalAudioChannels::instance()->register_(this);
    resetMembers();
}

MediaController::~MediaController()
{
    GlobalSubtitles::instance()->unregister_(this);
    GlobalAudioChannels::instance()->unregister_(this);
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
        return false;
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
    }

    warning() << "Interface" << iface << "is not supported by Phonon VLC :(";
    return false;
}

QVariant MediaController::interfaceCall(Interface iface, int i_command, const QList<QVariant> & arguments)
{
    DEBUG_BLOCK;
    switch (iface) {
    case AddonInterface::ChapterInterface:
        switch (static_cast<AddonInterface::ChapterCommand>(i_command)) {
        case AddonInterface::availableChapters:
            return availableChapters();
        case AddonInterface::chapter:
            return currentChapter();
        case AddonInterface::setChapter:
            if (arguments.isEmpty() || !arguments.first().canConvert(QVariant::Int)) {
                error() << Q_FUNC_INFO << "arguments invalid";
                return false;
            }
            setCurrentChapter(arguments.first().toInt());
            return true;
        default:
            error() << Q_FUNC_INFO << "unsupported AddonInterface::ChapterInterface command:" << i_command;
        }
        break;
    case AddonInterface::TitleInterface:
        switch (static_cast<AddonInterface::TitleCommand>(i_command)) {
        case AddonInterface::availableTitles:
            return availableTitles();
        case AddonInterface::title:
            return currentTitle();
        case AddonInterface::setTitle:
            if (arguments.isEmpty() || !arguments.first().canConvert(QVariant::Int)) {
                error() << Q_FUNC_INFO << "arguments invalid";
                return false;
            }
            setCurrentTitle(arguments.first().toInt());
            return true;
        case AddonInterface::autoplayTitles:
            return autoplayTitles();
        case AddonInterface::setAutoplayTitles:
            if (arguments.isEmpty() || !arguments.first().canConvert(QVariant::Bool)) {
                error() << Q_FUNC_INFO << " arguments invalid";
                return false;
            }
            setAutoplayTitles(arguments.first().toBool());
            return true;
        default:
            error() << Q_FUNC_INFO << "unsupported AddonInterface::TitleInterface command:" << i_command;
        }
        break;
    case AddonInterface::AngleInterface:
        warning() << "AddonInterface::AngleInterface not supported!";
        break;
    case AddonInterface::SubtitleInterface:
        switch (static_cast<AddonInterface::SubtitleCommand>(i_command)) {
        case AddonInterface::availableSubtitles:
            return QVariant::fromValue(availableSubtitles());
        case AddonInterface::currentSubtitle:
            return QVariant::fromValue(currentSubtitle());
        case AddonInterface::setCurrentSubtitle:
            if (arguments.isEmpty() || !arguments.first().canConvert<SubtitleDescription>()) {
                error() << Q_FUNC_INFO << "arguments invalid";
                return false;
            }
            setCurrentSubtitle(arguments.first().value<SubtitleDescription>());
            return true;
        default:
            error() << Q_FUNC_INFO << "unsupported AddonInterface::SubtitleInterface command:" << i_command;
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
                error() << Q_FUNC_INFO << "arguments invalid";
                return false;
            }
            setCurrentAudioChannel(arguments.first().value<AudioChannelDescription>());
            return true;
        default:
            error() << Q_FUNC_INFO << "unsupported AddonInterface::AudioChannelInterface command:" << i_command;
        }
        break;
    default:
        error() << Q_FUNC_INFO << "unsupported AddonInterface::Interface:" << iface;
    }

    return QVariant();
}

void MediaController::resetMediaController()
{
    resetMembers();
    emit availableAudioChannelsChanged();
    emit availableSubtitlesChanged();
    emit availableTitlesChanged(0);
    emit availableChaptersChanged(0);
}

void MediaController::resetMembers()
{
    m_currentAudioChannel = Phonon::AudioChannelDescription();
    GlobalAudioChannels::self->clearListFor(this);

    m_currentSubtitle = Phonon::SubtitleDescription();
    GlobalSubtitles::instance()->clearListFor(this);

    m_currentChapter = 0;
    m_availableChapters = 0;

    m_currentTitle = 0;
    m_availableTitles = 0;

    m_autoPlayTitles = false;
}

// ----------------------------- Audio Channel ------------------------------ //
void MediaController::setCurrentAudioChannel(const Phonon::AudioChannelDescription &audioChannel)
{
    const int localIndex = GlobalAudioChannels::instance()->localIdFor(this, audioChannel.index());
    if (m_player->setAudioTrack(localIndex))
        error() << "libVLC:" << LibVLC::errorMessage();
    else
        m_currentAudioChannel = audioChannel;
}

QList<Phonon::AudioChannelDescription> MediaController::availableAudioChannels() const
{
    return GlobalAudioChannels::instance()->listFor(this);
}

Phonon::AudioChannelDescription MediaController::currentAudioChannel() const
{
    return m_currentAudioChannel;
}

void MediaController::refreshAudioChannels()
{
    GlobalAudioChannels::instance()->clearListFor(this);

    int idCount = 0;
    VLC_TRACK_FOREACH(it, m_player->audioTrackDescription()) {
        // LibVLC's internal ID is broken, so we simply count up as internally
        // the setter will simply go by position in list anyway.
        GlobalAudioChannels::instance()->add(this, idCount, it->psz_name, "");
        ++idCount;
    }

    emit availableAudioChannelsChanged();
}

// -------------------------------- Subtitle -------------------------------- //
void MediaController::setCurrentSubtitle(const Phonon::SubtitleDescription &subtitle)
{
    QString type = subtitle.property("type").toString();

    if (type == "file") {
        QString filename = subtitle.property("name").toString();
        if (!filename.isEmpty()) {
            if (!m_player->setSubtitle(filename))
                error() << "libVLC:" << LibVLC::errorMessage();
            else
                m_currentSubtitle = subtitle;

            // There is no subtitle event inside libvlc so let's send our own event...
            GlobalSubtitles::instance()->add(this, m_currentSubtitle);
            emit availableSubtitlesChanged();
        }
    } else {
        const int localIndex = GlobalSubtitles::instance()->localIdFor(this, subtitle.index());
        if (m_player->setSubtitle(localIndex))
            error() << "libVLC:" << LibVLC::errorMessage();
        else
            m_currentSubtitle = subtitle;
    }
}

QList<Phonon::SubtitleDescription> MediaController::availableSubtitles() const
{
    return GlobalSubtitles::instance()->listFor(this);
}

Phonon::SubtitleDescription MediaController::currentSubtitle() const
{
    return m_currentSubtitle;
}

void MediaController::refreshSubtitles()
{
    DEBUG_BLOCK;
    GlobalSubtitles::instance()->clearListFor(this);

    int idCount = 0;
    VLC_TRACK_FOREACH(it, m_player->videoSubtitleDescription()) {
        // LibVLC's internal ID is broken, so we simply count up as internally
        // the setter will simply go by position in list anyway.
        GlobalSubtitles::instance()->add(this, idCount, it->psz_name, "");
        ++idCount;
    }

    emit availableSubtitlesChanged();
}

// --------------------------------- Title ---------------------------------- //
void MediaController::setCurrentTitle(int title)
{
    DEBUG_BLOCK;
    m_currentTitle = title;

    switch (source().discType()) {
    case Cd:
#ifdef __GNUC__
#warning use media subitem to set track of audiocd
#endif
        // Leave for MediaObject to handle.
        break;
    case Dvd:
    case Vcd:
         //    libvlc_media_player_set_title(m_player, title.index(), vlc_exception);
        m_player->setTitle(title);
        break;
    default:
        warning() << "Current media source is not a CD, DVD or VCD!";
    }
}

int MediaController::availableTitles() const
{
    return m_availableTitles;
}

int MediaController::currentTitle() const
{
    return m_currentTitle;
}

void MediaController::setAutoplayTitles(bool autoplay)
{
    m_autoPlayTitles = autoplay;
}

bool MediaController::autoplayTitles() const
{
    return m_autoPlayTitles;
}

void MediaController::refreshTitles()
{
    m_availableTitles = 0;

    VLC_TRACK_FOREACH(it, m_player->videoTitleDescription()) {
        ++m_availableTitles;
        emit availableTitlesChanged(m_availableTitles);
    }
}

// -------------------------------- Chapter --------------------------------- //
void MediaController::setCurrentChapter(int chapter)
{
    m_currentChapter = chapter;
    m_player->setChapter(chapter);
}

int MediaController::availableChapters() const
{
    return m_availableChapters;
}

int MediaController::currentChapter() const
{
    return m_currentChapter;
}

// We need to rebuild available chapters when title is changed
void MediaController::refreshChapters(int title)
{
    m_availableChapters = 0;

    // Get the description of available chapters for specific title
    VLC_TRACK_FOREACH(it, m_player->videoChapterDescription(title)) {
        ++m_availableChapters;
        emit availableChaptersChanged(m_availableChapters);
    }
}

// --------------------------------- Angle ---------------------------------- //
//                          NOT SUPPORTED IN LIBVLC                           //

} // namespace VLC
} // namespace Phonon
