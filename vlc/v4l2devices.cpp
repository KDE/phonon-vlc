/*****************************************************************************
* libVLC backend for the Phonon library                                     *
*                                                                           *
* Copyright (C) 2010 Casian Andrei <skeletk13@gmail.com>                    *
* Copyright (C) 2010 vlc-phonon AUTHORS                                     *
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

#ifdef HAVE_LIBV4L2

#include "v4l2devices.h"

#include <linux/videodev2.h>
#include <libv4l2.h>
#include <fcntl.h>

#include <QtCore/QDebug>
#include <QtCore/QString>
#include <QtCore/QStringList>
#include <QtCore/QDir>

QT_BEGIN_NAMESPACE

namespace Phonon {
namespace VLC {
namespace V4L2Support {


typedef enum {
    IO_METHOD_AUTO,
    IO_METHOD_READ,
    IO_METHOD_MMAP,
    IO_METHOD_USERPTR,
} io_method;

struct demux_sys_t {
    char *psz_device;  /* Main device from MRL */
    int  i_fd;

    char *psz_requested_chroma;

    /* Video */
    io_method io;

    int i_cache;

    struct v4l2_capability dev_cap;

    int i_input;
    struct v4l2_input *p_inputs;
    int i_selected_input;

    int i_standard;
    struct v4l2_standard *p_standards;
    v4l2_std_id i_selected_standard_id;

    int i_audio;
    /* V4L2 devices cannot have more than 32 audio inputs */
    struct v4l2_audio p_audios[32];
    int i_selected_audio_input;

    int i_tuner;
    struct v4l2_tuner *p_tuners;

    int i_codec;
    struct v4l2_fmtdesc *p_codecs;

    struct buffer_t *p_buffers;
    unsigned int i_nbuffers;

    int i_width;
    int i_height;
    unsigned int i_aspect;
    float f_fps;            /* <= 0.0 mean to grab at full rate */
    int64_t i_video_pts;    /* only used when f_fps > 0 */
    int i_fourcc;
    uint32_t i_block_flags;

    /* es_out_id_t *p_es; */
    void *p_es;

    /* Tuner */
    int i_cur_tuner;
    int i_frequency;
    int i_audio_mode;

    /* Controls */
    char *psz_set_ctrls;

    /* */
    int (*pf_open)(const char *, int, ...);
    int (*pf_close)(int);
    int (*pf_dup)(int);
    int (*pf_ioctl)(int, unsigned long int, ...);
    ssize_t (*pf_read)(int, void *, size_t );
    void *(*pf_mmap)(void *, size_t, int, int, int, off_t);
    int (*pf_munmap)(void *, size_t);
    bool b_libv4l2;
};


/*
Tries to open a v4l device specified by devicePath, and queries it's capabilities.
Depending on the device capabilities, it is added to the capture device lists
*/
static bool probeDevice(QByteArray devicePath, QList<DeviceInfo> & devices)
{
    int i_index;
    int i_standard;
    int i_fd;

    struct demux_sys_t v4lDeviceInfo;
    memset(&v4lDeviceInfo, 0, sizeof(v4lDeviceInfo));

    // Open device
    if ((i_fd = v4l2_open(devicePath.constData(), O_RDWR)) < 0) {
        qCritical() << Q_FUNC_INFO << "Cannot open video device" << devicePath;
        if (i_fd >= 0)
            v4l2_close(i_fd);
        return false;
    }

    int libv4l2_fd;
    libv4l2_fd = v4l2_fd_open(i_fd, V4L2_ENABLE_ENUM_FMT_EMULATION);
    if (libv4l2_fd != -1)
        i_fd = libv4l2_fd;

    // Get device capabilites
    if (v4l2_ioctl(i_fd, VIDIOC_QUERYCAP, &v4lDeviceInfo.dev_cap) < 0) {
        qCritical() << Q_FUNC_INFO << "Cannot get video capabilities for" << devicePath;
        if (i_fd >= 0)
            v4l2_close(i_fd);
        return false;
    }

    DeviceInfo di("", devicePath);
    bool ok = false;

    // Create a device description if the device has either video or audio capabilities, or both
    if (v4lDeviceInfo.dev_cap.capabilities & (V4L2_CAP_VIDEO_CAPTURE | V4L2_CAP_AUDIO)) {
        di.nameId = QByteArray((const char*) v4lDeviceInfo.dev_cap.card).trimmed();
        di.description = "Video For Linux 2 Video Device, using driver ";
        di.description.append(QByteArray((const char*) v4lDeviceInfo.dev_cap.driver).trimmed());
        di.deviceClass = DeviceInfo::V4L2;
        ok = true;
    }

    // Probe video inputs
    if (v4lDeviceInfo.dev_cap.capabilities & V4L2_CAP_VIDEO_CAPTURE) {
        qDebug() << "found video capture device" << devicePath;
        di.capabilities |= DeviceInfo::VideoCapture;
    }

    // Probe audio inputs
    if (v4lDeviceInfo.dev_cap.capabilities & V4L2_CAP_AUDIO) {
        qDebug() << "found audio capture device" << devicePath;
        di.capabilities |= DeviceInfo::AudioCapture;
    }

    if (ok)
        devices << di;

    if (i_fd >= 0)
        v4l2_close(i_fd);
    return true;
}

bool scanDevices(QList<DeviceInfo> & devices)
{
    QDir deviceDir("/dev");
    if (!deviceDir.isReadable()) {
        qCritical() << Q_FUNC_INFO << "Unable to read /dev";
        return false;
    }

    // Search trough /dev/videoX for valid V4L devices
    QStringList nameFilters;
    nameFilters << "video*";
    deviceDir.setFilter(QDir::System);
    deviceDir.setNameFilters(nameFilters);

    QStringList devicePaths = deviceDir.entryList();
    foreach (QString dn, devicePaths) {
        dn = deviceDir.filePath(dn);

        // Get information about the device from V4L, and append it to the capture device lists
        probeDevice(dn.toLatin1(), devices);
    }

    return true;
}

}
}
} // namespace Phonon::VLC::V4L2Support

QT_END_NAMESPACE

#endif // HAVE_LIBV4L2
