/*
    Copyright (C) 2007-2008 Tanguy Krotoff <tkrotoff@gmail.com>
    Copyright (C) 2008 Lukas Durfina <lukas.durfina@gmail.com>
    Copyright (C) 2009 Fathi Boudra <fabo@kde.org>
    Copyright (C) 2009-2011 vlc-phonon AUTHORS <kde-multimedia@kde.org>
    Copyright (C) 2011-2018 Harald Sitter <sitter@kde.org>

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

#include "mediacontroller.h"

#include <phonon/GlobalDescriptionContainer>

#include <QTimer>

#include "utils/debug.h"
#include "utils/libvlc.h"
#include "mediaplayer.h"

namespace Phonon {
namespace VLC {

#ifdef __GNUC__
#warning titles and chapters not covered by globaldescriptioncontainer!!
#endif

MediaController::MediaController()
    : m_subtitleAutodetect(true)
    , m_subtitleEncoding("UTF-8")
    , m_subtitleFontChanged(false)
    , m_player(0)
    , m_refreshTimer(new QTimer(dynamic_cast<QObject *>(this)))
    , m_attemptingAutoplay(false)
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
        case AddonInterface::setCurrentSubtitleFile:
            if (arguments.isEmpty() || !arguments.first().canConvert<QUrl>()) {
                error() << Q_FUNC_INFO << " arguments invalid";
                return false;
            }
            setCurrentSubtitleFile(arguments.first().value<QUrl>());
        case AddonInterface::subtitleAutodetect:
            return QVariant::fromValue(subtitleAutodetect());
        case AddonInterface::setSubtitleAutodetect:
            if (arguments.isEmpty() || !arguments.first().canConvert<bool>()) {
                error() << Q_FUNC_INFO << " arguments invalid";
                return false;
            }
            setSubtitleAutodetect(arguments.first().value<bool>());
            return true;
        case AddonInterface::subtitleEncoding:
            return subtitleEncoding();
        case AddonInterface::setSubtitleEncoding:
            if (arguments.isEmpty() || !arguments.first().canConvert<QString>()) {
                error() << Q_FUNC_INFO << " arguments invalid";
                return false;
            }
            setSubtitleEncoding(arguments.first().value<QString>());
            return true;
        case AddonInterface::subtitleFont:
            return subtitleFont();
        case AddonInterface::setSubtitleFont:
            if (arguments.isEmpty() || !arguments.first().canConvert<QFont>()) {
                error() << Q_FUNC_INFO << " arguments invalid";
                return false;
            }
            setSubtitleFont(arguments.first().value<QFont>());
            return true;
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
        }
        break;
    }

    error() << Q_FUNC_INFO << "unsupported AddonInterface::Interface:" << iface;

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

    m_currentTitle = 1;
    m_availableTitles = 0;

    m_attemptingAutoplay = false;
}

// ----------------------------- Audio Channel ------------------------------ //
void MediaController::setCurrentAudioChannel(const Phonon::AudioChannelDescription &audioChannel)
{
    const int localIndex = GlobalAudioChannels::instance()->localIdFor(this, audioChannel.index());
    if (!m_player->setAudioTrack(localIndex))
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

    const int currentChannelId = m_player->audioTrack();

    int idCount = 0;
    VLC_FOREACH_TRACK(it, m_player->audioTrackDescription()) {
#if (LIBVLC_VERSION_INT >= LIBVLC_VERSION(3, 0, 0, 0))
        idCount= it->i_id;
#else
        // LibVLC's internal ID is broken, so we simply count up as internally
        // the setter will simply go by position in list anyway.
#endif
        GlobalAudioChannels::instance()->add(this, idCount, QString::fromUtf8(it->psz_name), "");
        if (idCount == currentChannelId) {
#ifdef __GNUC__
#warning GlobalDescriptionContainer does not allow reverse resolution from local to descriptor!
#endif
            const QList<AudioChannelDescription> list = GlobalAudioChannels::instance()->listFor(this);
            foreach (const AudioChannelDescription &descriptor, list) {
                if (descriptor.name() == QString::fromUtf8(it->psz_name)) {
                    m_currentAudioChannel = descriptor;
                }
            }
        }
        ++idCount;
    }

    emit availableAudioChannelsChanged();
}

// -------------------------------- Subtitle -------------------------------- //
void MediaController::setCurrentSubtitle(const Phonon::SubtitleDescription &subtitle)
{
    DEBUG_BLOCK;
    QString type = subtitle.property("type").toString();

    debug() << subtitle;

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
        debug () << "localid" << localIndex;
        if (!m_player->setSubtitle(localIndex))
            error() << "libVLC:" << LibVLC::errorMessage();
        else
            m_currentSubtitle = subtitle;
    }
}

void MediaController::setCurrentSubtitleFile(const QUrl &url)
{
    const QString file = url.toLocalFile();
    if (!m_player->setSubtitle(file))
        error() << "libVLC failed to set subtitle file:" << LibVLC::errorMessage();
    // Unfortunately the addition of SPUs does not trigger an event in the
    // VLC mediaplayer, yet the actual addition to the descriptor is async.
    // So for the time being our best shot at getting an up-to-date list of SPUs
    // is shooting in the dark and hoping we hit something.
    // Refresha after 1, 2 and 5 seconds. If we have no updated list after 5
    // seconds we are out of luck.
    // https://trac.videolan.org/vlc/ticket/9796
    QObject *mediaObject = dynamic_cast<QObject *>(this); // MediaObject : QObject, MediaController
    m_refreshTimer->singleShot(1 * 1000, mediaObject, SLOT(refreshDescriptors()));
    m_refreshTimer->singleShot(2 * 1000, mediaObject, SLOT(refreshDescriptors()));
    m_refreshTimer->singleShot(5 * 1000, mediaObject, SLOT(refreshDescriptors()));
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

    const int currentSubtitleId = m_player->subtitle();

    VLC_FOREACH_TRACK(it, m_player->videoSubtitleDescription()) {
        debug() << "found subtitle" << it->psz_name << "[" << it->i_id << "]";
        GlobalSubtitles::instance()->add(this, it->i_id, QString::fromUtf8(it->psz_name), "");
        if (it->i_id == currentSubtitleId) {
#ifdef __GNUC__
#warning GlobalDescriptionContainer does not allow reverse resolution from local to descriptor!
#endif
            const QList<SubtitleDescription> list = GlobalSubtitles::instance()->listFor(this);
            foreach (const SubtitleDescription &descriptor, list) {
                if (descriptor.name() == QString::fromUtf8(it->psz_name)) {
                    m_currentSubtitle = descriptor;
                }
            }
        }
    }

    emit availableSubtitlesChanged();
}

bool MediaController::subtitleAutodetect() const
{
    return m_subtitleAutodetect;
}

void MediaController::setSubtitleAutodetect(bool enabled)
{
    m_subtitleAutodetect = enabled;
}

QString MediaController::subtitleEncoding() const
{
    return m_subtitleEncoding;
}

void MediaController::setSubtitleEncoding(const QString &encoding)
{
    m_subtitleEncoding = encoding;
}

QFont MediaController::subtitleFont() const
{
    return m_subtitleFont;
}

void MediaController::setSubtitleFont(const QFont &font)
{
    m_subtitleFontChanged = true;
    m_subtitleFont = font;
}

// --------------------------------- Title ---------------------------------- //
void MediaController::setCurrentTitle(int title)
{
    DEBUG_BLOCK;
    m_currentTitle = title;

    switch (source().discType()) {
    case Cd:
        m_player->setCdTrack(title);
        return;
    case Dvd:
    case Vcd:
    case BluRay:
        m_player->setTitle(title);
        return;
    case NoDisc:
        warning() << "Current media source is not a CD, DVD or VCD!";
        return;
    }

    warning() << "MediaSource does not support setting of tile in this version of Phonon VLC!"
              << "Type is" << source().discType();
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

#if (LIBVLC_VERSION_INT >= LIBVLC_VERSION(3, 0, 0, 0))
    SharedTitleDescriptions list = m_player->titleDescription();
    for (unsigned int i = 0; i < list->size(); ++i) {
        ++m_availableTitles;
        emit availableTitlesChanged(m_availableTitles);
    }
#else
    VLC_FOREACH_TRACK(it, m_player->titleDescription()) {
        ++m_availableTitles;
        emit availableTitlesChanged(m_availableTitles);
    }
#endif
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
#if (LIBVLC_VERSION_INT >= LIBVLC_VERSION(3, 0, 0, 0))
    SharedChapterDescriptions list = m_player->videoChapterDescription(title);
    for (unsigned int i = 0; i < list->size(); ++i) {
        ++m_availableChapters;
        emit availableChaptersChanged(m_availableChapters);
    }
#else
    VLC_FOREACH_TRACK(it, m_player->videoChapterDescription(title)) {
        ++m_availableChapters;
        emit availableChaptersChanged(m_availableChapters);
    }
#endif
}

// --------------------------------- Angle ---------------------------------- //
//                          NOT SUPPORTED IN LIBVLC                           //

} // namespace VLC
} // namespace Phonon
