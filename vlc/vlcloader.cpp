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

#include "vlcloader.h"

#include <QtCore/QDebug>
#include <QtCore/QDir>
#include <QtCore/QFile>
#include <QtCore/QLibrary>
#include <QtCore/QSettings>
#include <QtCore/QString>
#include <QtCore/QStringList>
#include <QtCore/QCoreApplication>

// Global variables
libvlc_instance_t *vlc_instance = 0;

namespace Phonon
{
namespace VLC
{

bool vlcInit(int debugLevl)
{
    // Global variables
    vlc_instance = 0;

    QString path = vlcPath();
    if (!path.isEmpty()) {
        QString pluginsPath = QString("--plugin-path=") + QDir::toNativeSeparators(QFileInfo(vlcPath()).dir().path());

#if defined(Q_OS_UNIX)
        pluginsPath.append("/vlc");
#elif defined(Q_OS_WIN)
        pluginsPath.append("\\plugins");
#endif
        QByteArray p = QFile::encodeName(path);
        QByteArray pp = QFile::encodeName(pluginsPath);

        QByteArray log;
        QByteArray logFile;
        QByteArray verboseLevl;
        if (debugLevl > 0) {
            verboseLevl = QString("--verbose=").append(QString::number(debugLevl)).toLatin1();
            log = "--extraintf=logger";
            logFile = "--logfile=";
#ifdef Q_WS_WIN
            QDir logFilePath(QString(qgetenv("APPDATA")).append("/vlc"));
#else
            QDir logFilePath(QDir::homePath().append("/.vlc"));
#endif //Q_WS_WIN
            logFilePath.mkdir("log");
            logFile.append(QFile::encodeName(QDir::toNativeSeparators(logFilePath.path().append("/log/vlc-log-").append(QString::number(qApp->applicationPid())).append(".txt"))));
        }
        // VLC command line options. See vlc --full-help
        const char *vlcArgs[] = {
            p.constData(),
            pp.constData(),
            log.constData(),
            logFile.constData(),
            verboseLevl.constData(),
            "--reset-plugins-cache",
            "--no-media-library",
#ifdef Q_WS_X11
            "--no-xlib",
#endif
#ifndef Q_OS_MAC
            "--no-one-instance",
#endif
            "--no-osd",
            "--no-stats",
            "--no-video-title-show",
            "--album-art=0"
        };

        // Create and initialize a libvlc instance (it should be done only once)
        vlc_instance = libvlc_new(sizeof(vlcArgs) / sizeof(*vlcArgs), vlcArgs);
        if (!vlc_instance) {
            qDebug() << "libvlc exception:" << libvlc_errmsg();
        }

        return true;
    }

    return false;
}

void vlcRelease()
{
    libvlc_release(vlc_instance);
    vlcUnload();
}

#if defined(Q_OS_UNIX)
static bool libGreaterThan(const QString &lhs, const QString &rhs)
{
    const QStringList lhsparts = lhs.split(QLatin1Char('.'));
    const QStringList rhsparts = rhs.split(QLatin1Char('.'));
    Q_ASSERT(lhsparts.count() > 1 && rhsparts.count() > 1);

    for (int i = 1; i < rhsparts.count(); ++i) {
        if (lhsparts.count() <= i)
            // left hand side is shorter, so it's less than rhs
        {
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

static QStringList findAllLibVlc()
{
    QStringList paths;

#ifdef Q_OS_UNIX
    paths = QString::fromLatin1(qgetenv("LD_LIBRARY_PATH"))
            .split(QLatin1Char(':'), QString::SkipEmptyParts);
    paths << QLatin1String(PHONON_LIB_INSTALL_DIR) << QLatin1String("/usr/lib") << QLatin1String("/usr/local/lib");

#if defined(Q_WS_MAC)
    paths
            << QCoreApplication::applicationDirPath()
            << QCoreApplication::applicationDirPath() + QLatin1String("/../Frameworks")
            << QCoreApplication::applicationDirPath() + QLatin1String("/../PlugIns")
            << QCoreApplication::applicationDirPath() + QLatin1String("/lib")

            ;
#endif

    QStringList foundVlcs;
    foreach(const QString & path, paths) {
        QDir dir = QDir(path);
        QStringList entryList = dir.entryList(QStringList() << QLatin1String("libvlc.*"), QDir::Files);

        qSort(entryList.begin(), entryList.end(), libGreaterThan);
        foreach(const QString & entry, entryList) {
            if (entry.contains(".debug")) {
                continue;
            }
            foundVlcs << path + QLatin1Char('/') + entry;
        }
    }

    return foundVlcs;
#elif defined(Q_OS_WIN)
    // Read VLC version and installation directory from Windows registry
    QSettings settings(QSettings::SystemScope, "VideoLAN", "VLC");
    QString vlcVersion = settings.value("Version").toString();
    QString vlcInstallDir = settings.value("InstallDir").toString();
    if (vlcVersion.startsWith("1.1") && !vlcInstallDir.isEmpty()) {
        paths << vlcInstallDir + QLatin1Char('\\') + "libvlc.dll";
        return paths;
    } else {
        //If nothing is found in the registry try %PATH%
        QStringList searchPaths = QString::fromLatin1(qgetenv("PATH"))
                                  .split(QLatin1Char(';'), QString::SkipEmptyParts);
        //search also in the application dir
        searchPaths.append(QCoreApplication::applicationDirPath());

        QStringList foundVlcs;
        foreach(const QString & sp, searchPaths) {
            QDir dir = QDir(sp);
            QStringList entryList = dir.entryList(QStringList() << QLatin1String("libvlc.dll"), QDir::Files);
            foreach(const QString & entry, entryList){
                foundVlcs << sp + QLatin1Char('\\') + entry;
            }
        }
        paths << foundVlcs;
        return paths;
    }
#endif
}

static QLibrary *vlcLibrary = 0;

QString vlcPath()
{
    static QString path;
    if (!path.isEmpty()) {
        return path;
    }

    vlcLibrary = new QLibrary();
    QStringList paths = findAllLibVlc();
    foreach(path, paths) {
        vlcLibrary->setFileName(path);

        if (!vlcLibrary->resolve("libvlc_exception_init")) { //"libvlc_exception_init" not contained in 1.1+
            return path;
        } else {
            qDebug("Cannot resolve the symbol or load VLC library");
        }
        qWarning() << vlcLibrary->errorString();
    }

    vlcUnload();

    return QString();
}

void vlcUnload()
{
    vlcLibrary->unload();
    delete vlcLibrary;
    vlcLibrary = 0;
}

}
} // Namespace Phonon::VLC
