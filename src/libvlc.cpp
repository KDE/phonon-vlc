/*
    Copyright (C) 2011 vlc-phonon AUTHORS

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
#include <QtCore/QLibrary>
#include <QtCore/QSettings>
#include <QtCore/QString>
#include <QtCore/QStringBuilder>
#include <QtCore/QVarLengthArray>

#include <vlc/libvlc.h>

#include "debug.h"

LibVLC *LibVLC::self;

LibVLC::LibVLC() :
    m_vlcInstance(0)
{
}

LibVLC::~LibVLC()
{
    if (m_vlcInstance)
        libvlc_release(m_vlcInstance);
    vlcUnload();
    self = 0;
}

bool LibVLC::init()
{
    Q_ASSERT_X(!self, "LibVLC", "there should be only one LibVLC object");
    LibVLC::self = new LibVLC;

    QString path = self->vlcPath();
    if (!path.isEmpty()) {
        QList<QByteArray> args;

        QString pluginsPath = QLatin1Literal("--plugin-path=") %
                QDir::toNativeSeparators(QFileInfo(self->vlcPath()).dir().path());
#if defined(Q_OS_UNIX)
        pluginsPath.append("/vlc");
#elif defined(Q_OS_WIN)
        pluginsPath.append("\\plugins");
#endif
        args << QFile::encodeName(pluginsPath);

        // Ends up as something like $HOME/.config/Phonon/vlc.conf
        const QString configFileName = QSettings("Phonon", "vlc").fileName();
        if (QFile::exists(configFileName)) {
            args << QByteArray("--config=").append(QFile::encodeName(configFileName));
        }

        int debugLevel = 3 - (int) Debug::minimumDebugLevel();
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
        // Do not load xlib dependent modules as we cannot ensure proper init
        // order as expected by xlib thus leading to crashes.
        // KDE BUG: 240001
        args << "--no-xlib";
        // Do not preload services discovery modules, we don't use them.
        args << "--services-discovery=''";
        // Allow multiple starts (one gets to wonder whether that makes a difference.
        args << "--no-one-instance";

        // Build const char* array
        QVarLengthArray<const char*, 64> vlcArgs(args.size());
        for (int i = 0; i < args.size(); ++i) {
            vlcArgs[i] = args.at(i).constData();
        }

        // Create and initialize a libvlc instance (it should be done only once)
        self->m_vlcInstance = libvlc_new(vlcArgs.size(), vlcArgs.constData());
        if (!self->m_vlcInstance) {
            fatal() << "libVLC:" << LibVLC::errorMessage();
            return false;
        }
        return true;
    }
    return false;
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

QStringList LibVLC::findAllLibVlcPaths()
{
    QStringList paths;

#ifdef Q_OS_UNIX
    paths = QString::fromLatin1(qgetenv("LD_LIBRARY_PATH"))
            .split(QLatin1Char(':'), QString::SkipEmptyParts);
    paths << QLatin1String(PHONON_LIB_INSTALL_DIR) << QLatin1String("/usr/lib") << QLatin1String("/usr/local/lib");
    paths << QLatin1String("/usr/lib64") << QLatin1String("/usr/local/lib64");

#if defined(Q_WS_MAC)
    paths
            << QCoreApplication::applicationDirPath()
            << QCoreApplication::applicationDirPath() % QLatin1Literal("/../Frameworks")
            << QCoreApplication::applicationDirPath() % QLatin1Literal("/../PlugIns")
            << QCoreApplication::applicationDirPath() % QLatin1Literal("/lib");
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
            foundVlcs << path % QLatin1Char('/') % entry;
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
    if (vlcVersion.startsWith(QLatin1String("1.1")) && !vlcInstallDir.isEmpty()) {
        paths << vlcInstallDir % QLatin1Char('\\') % QLatin1Literal("libvlc.dll");
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
                foundVlcs << sp % QLatin1Char('\\') % entry;
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
            debug() << "Cannot resolve the symbol or load VLC library";
        }
        warning() << m_vlcLibrary->errorString();
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
