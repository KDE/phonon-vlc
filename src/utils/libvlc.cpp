/*
    Copyright (C) 2011-2012 vlc-phonon AUTHORS

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
#include <QtCore/QSettings>
#include <QtCore/QString>
#include <QtCore/QStringBuilder>
#include <QtCore/QVarLengthArray>

#include <vlc/libvlc.h>
#include <vlc/libvlc_version.h>

#include "debug.h"

LibVLC *LibVLC::self;

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

    int debugLevel = qgetenv("PHONON_SUBSYSTEM_DEBUG").toInt();
    if (debugLevel > 0) {
        args << QByteArray("--verbose=").append(QString::number(debugLevel));
        args << QByteArray("--extraintf=logger");
#ifdef Q_WS_WIN
        QDir logFilePath(QString(qgetenv("APPDATA")).append("/vlc"));
#else
        QDir logFilePath(QDir::homePath().append("/.vlc"));
#endif //Q_WS_WIN
        logFilePath.mkdir("log");
        const QString logFile = logFilePath.path()
                .append("/log/vlc-log-")
                .append(QString::number(qApp->applicationPid()))
                .append(".txt");
        args << QByteArray("--logfile=").append(QFile::encodeName(QDir::toNativeSeparators(logFile)));
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
        fatal() << "libVLC: could not initialize";
        return false;
    }
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
