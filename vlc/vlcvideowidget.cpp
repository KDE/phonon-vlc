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

#include "vlcvideowidget.h"

#include <QtGui/QPainter>
#include <QtGui/QResizeEvent>
#include <QtCore/QDebug>

#include "videowidget.h"

namespace Phonon
{
namespace VLC
{

VLCVideoWidget::VLCVideoWidget(QWidget *parent, VideoWidget *videoWidget) :
    WidgetNoPaintEvent(parent),
    m_videoWidget(videoWidget)
{
    // Set background color
    setBackgroundColor(Qt::black);
}

VLCVideoWidget::~VLCVideoWidget()
{
}

void VLCVideoWidget::resizeEvent(QResizeEvent *event)
{
    qDebug() << "resizeEvent" << event->size();
}

void VLCVideoWidget::setAspectRatio(double aspectRatio)
{
}

void VLCVideoWidget::setScaleAndCropMode(bool scaleAndCrop)
{
}

/**
 * Sets an approximate video size to provide a size hint. It will be set
 * to the original size of the video.
 */
void VLCVideoWidget::setVideoSize(const QSize &size)
{
    m_videoSize = size;
    updateGeometry();
    update();
}

QSize VLCVideoWidget::sizeHint() const
{
    if (!m_videoSize.isEmpty()) {
        return m_videoSize;
    }
    return QSize(640, 480);
}

void VLCVideoWidget::setVisible(bool visible)
{
    if (window() && window()->testAttribute(Qt::WA_DontShowOnScreen)) {
        qDebug() << "Widget rendering forced";
        m_videoWidget->useCustomRender();
        m_customRender = true;
    }
    QWidget::setVisible(visible);
}

void VLCVideoWidget::setNextFrame(const QByteArray &array, int width, int height)
{
    // TODO: Should preloading ever become available ... what do to here?

    m_frame = QImage();
    {
        m_frame = QImage((uchar *)array.constData(), width, height, QImage::Format_RGB32);
    }
    update();
}

void VLCVideoWidget::paintEvent(QPaintEvent *event)
{
    if (m_customRender) {
        QPainter painter(this);
        // TODO: more sensible rect calculation.
        painter.drawImage(rect(), m_frame);
    } else {
        QWidget::paintEvent(event);
    }
}

}
} // Namespace Phonon::VLC
