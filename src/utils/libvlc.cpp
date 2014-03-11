/*
    Copyright (C) 2011-2012 vlc-phonon AUTHORS <kde-multimedia@kde.org>

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

#include "libvlc.h"

#include <QtCore/QByteArray>
#include <QtCore/QCoreApplication>
#include <QtCore/QDebug>
#include <QtCore/QDir>
#include <QtCore/QLoggingCategory>
#include <QtCore/QSettings>
#include <QtCore/QString>
#include <QtCore/QStringBuilder>
#include <QtCore/QVarLengthArray>

#include <vlc/libvlc.h>
#include <vlc/libvlc_version.h>

#include "debug.h"

LibVLC *LibVLC::self;

// The subsystem debugging (libvlc) is using a separate category to allow
// filtering of the subsystem and the backend independently.
Q_DECLARE_LOGGING_CATEGORY(LIBVLC_DEBUG_CATEGORY)
Q_LOGGING_CATEGORY(LIBVLC_DEBUG_CATEGORY, "phonon.subsystem")

LibVLC::LibVLC()
    : m_vlcInstance(0)
{
}

LibVLC::~LibVLC()
{
    if (m_vlcInstance)
        libvlc_release(m_vlcInstance);
    self = 0;
}

bool LibVLC::init()
{
    Q_ASSERT_X(!self, "LibVLC", "there should be only one LibVLC object");
    LibVLC::self = new LibVLC;

    QList<QByteArray> args;

    // Ends up as something like $HOME/.config/Phonon/vlc.conf
    const QString configFileName = QSettings("Phonon", "vlc").fileName();
    if (QFile::exists(configFileName)) {
        args << QByteArray("--config=").append(QFile::encodeName(configFileName));
        args << "--no-ignore-config";
    }

    args << "--no-media-library";
    args << "--no-osd";
    args << "--no-stats";
    args << "--no-video-title-show";
    args << "--album-art=0";
    // By default VLC will put a picture-in-picture when making a snapshot.
    // This is unexpected behaviour for us, so we force it off.
    args << "--no-snapshot-preview";
    // Do not load xlib dependent modules as we cannot ensure proper init
    // order as expected by xlib thus leading to crashes.
    // KDE BUG: 240001
    args << "--no-xlib";
    // Do not preload services discovery modules, we don't use them.
    args << "--services-discovery=''";
    // Allow multiple starts (one gets to wonder whether that makes a difference).
#if (LIBVLC_VERSION_INT > LIBVLC_VERSION(2, 1, 0, 0) && defined(Q_OS_MAC)) || defined( Q_OS_WIN) || !defined(PHONON_NO_DBUS)
    args << "--no-one-instance";
#endif
    args << "--no-audio";
    args << "--no-video";

    // Build const char* array
    QVarLengthArray<const char *, 64> vlcArgs(args.size());
    for (int i = 0; i < args.size(); ++i) {
        vlcArgs[i] = args.at(i).constData();
    }

    // Create and initialize a libvlc instance (it should be done only once)
    self->m_vlcInstance = libvlc_new(vlcArgs.size(), vlcArgs.constData());
    if (!self->m_vlcInstance) {
        pFatal("libVLC: could not initialize");
        return false;
    }
#warning subsystem debug forced off
    QLoggingCategory::setFilterRules(QStringLiteral("phonon.subsystem=false"));
    libvlc_log_set(self->m_vlcInstance, &LibVLC::log, self);
    return true;
}

const char *LibVLC::errorMessage()
{
    return libvlc_errmsg();
}

#if defined(Q_OS_UNIX)
bool LibVLC::libGreaterThan(const QString &lhs, const QString &rhs)
{
    const QStringList lhsparts = lhs.split(QLatin1Char('.'));
    const QStringList rhsparts = rhs.split(QLatin1Char('.'));
    Q_ASSERT(lhsparts.count() > 1 && rhsparts.count() > 1);

    for (int i = 1; i < rhsparts.count(); ++i) {
        if (lhsparts.count() <= i) {
            // left hand side is shorter, so it's less than rhs
            return false;
        }

        bool ok = false;
        int b = 0;
        int a = lhsparts.at(i).toInt(&ok);
        if (ok) {
            b = rhsparts.at(i).toInt(&ok);
        }
        if (ok) {
            // both toInt succeeded
            if (a == b) {
                continue;
            }
            return a > b;
        } else {
            // compare as strings;
            if (lhsparts.at(i) == rhsparts.at(i)) {
                continue;
            }
            return lhsparts.at(i) > rhsparts.at(i);
        }
    }

    // they compared strictly equally so far
    // lhs cannot be less than rhs
    return true;
}
#endif

void LibVLC::log(void */*data*/, int _level, const vlc_log_t */*ctx*/,
                 const char *fmt, va_list args)
{
    // QDebug and friends do not have explicit va_lists handling, so pass the
    // information through QString and get a const char * out of it.
    QString qstr;
    qstr.vsprintf(fmt, args);
    const QByteArray &data = qstr.toUtf8();
    const char *str = data.constData();

    const libvlc_log_level level = (libvlc_log_level)_level;
    switch (level) {
    case LIBVLC_DEBUG:
        qCDebug(LIBVLC_DEBUG_CATEGORY) << str;
        break;
    case LIBVLC_NOTICE:
    case LIBVLC_WARNING:
        qCWarning(LIBVLC_DEBUG_CATEGORY) << str;
        break;
    case LIBVLC_ERROR:
        qCCritical(LIBVLC_DEBUG_CATEGORY) << str;
        break;
    }
}
