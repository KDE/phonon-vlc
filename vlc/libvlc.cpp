/*****************************************************************************
 * libVLC backend for the Phonon library                                     *
 *                                                                           *
 * Copyright (C) 2011 vlc-phonon AUTHORS                                     *
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

#include "libvlc.h"

#include <QtCore/QByteArray>
#include <QtCore/QCoreApplication>
#include <QtCore/QDebug>
#include <QtCore/QDir>
#include <QtCore/QLibrary>
#include <QtCore/QString>

#include <vlc/libvlc.h>

LibVLC *LibVLC::self;

LibVLC::LibVLC(int debugLevel) :
    m_vlcInstance(0)
{
    init(debugLevel);
}

LibVLC::~LibVLC()
{
    libvlc_release(m_vlcInstance);
    vlcUnload();
}

void LibVLC::init(int debugLevel)
{
    Q_ASSERT_X(!self, "LibVLC", "there should be only one LibVLC object");
    LibVLC::self = this;

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
        if (debugLevel > 0) {
            verboseLevl = QString("--verbose=").append(QString::number(debugLevel)).toLatin1();
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
#ifndef Q_OS_MAC
            "--no-one-instance",
#endif
            "--no-osd",
            "--no-stats",
            "--no-video-title-show",
            "--album-art=0"
        };

        // Create and initialize a libvlc instance (it should be done only once)
        m_vlcInstance = libvlc_new(sizeof(vlcArgs) / sizeof(*vlcArgs), vlcArgs);
        if (!m_vlcInstance) {
            qCritical() << "libvlc exception:" << libvlc_errmsg();
        }
    }
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

QStringList LibVLC::findAllLibVlcPaths()
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
            << QCoreApplication::applicationDirPath() + QLatin1String("/lib");
#endif

    QStringList foundVlcs;
    foreach (const QString & path, paths) {
        QDir dir = QDir(path);
        QStringList entryList = dir.entryList(QStringList() << QLatin1String("libvlc.*"), QDir::Files);

        qSort(entryList.begin(), entryList.end(), LibVLC::libGreaterThan);
        foreach(const QString & entry, entryList) {
            if (entry.contains(".debug")) {
                continue;
            }
            foundVlcs << path + QLatin1Char('/') + entry;
        }
    }

    return foundVlcs;
#elif defined(Q_OS_WIN)
    // Current application path has highest priority.
    // Application could be delivered with own copy of qt, phonon, LibVLC, vlc and vlc plugins.
    QString appDirVlc =
            QDir::toNativeSeparators(QCoreApplication::applicationDirPath()
                                     % QLatin1Char('\\')
                                     % QLatin1Literal("libvlc.dll"));
    if (QFile::exists(appDirVlc)) {
        paths << appDirVlc;
    }

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

QString LibVLC::vlcPath()
{
    static QString path;
    if (!path.isEmpty()) {
        return path;
    }

    m_vlcLibrary = new QLibrary();
    QStringList paths = LibVLC::findAllLibVlcPaths();
    foreach(path, paths) {
        m_vlcLibrary->setFileName(path);

        if (!m_vlcLibrary->resolve("libvlc_exception_init")) { //"libvlc_exception_init" not contained in 1.1+
            return path;
        } else {
            qDebug("Cannot resolve the symbol or load VLC library");
        }
        qWarning() << m_vlcLibrary->errorString();
    }

    vlcUnload();

    return QString();
}

void LibVLC::vlcUnload()
{
    m_vlcLibrary->unload();
    delete m_vlcLibrary;
    m_vlcLibrary = 0;
}
