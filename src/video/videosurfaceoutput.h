#ifndef PHONON_VLC_VIDEOSURFACEOUTPUT_H
#define PHONON_VLC_VIDEOSURFACEOUTPUT_H

#include <QObject>
#include <QMutex>
#include <QSet>

#include <phonon/videosurfaceinterface.h>

#include "connector.h"
#include "videomemorystream.h"

namespace Phonon {
namespace VLC {

class VideoSurfaceOutput : public QObject,
                           public VideoSurfaceOutputInterface,
                           public Connector,
                           public VideoMemoryStream
{
    Q_OBJECT
    Q_INTERFACES(Phonon::VideoSurfaceOutputInterface)
public:
    explicit VideoSurfaceOutput(QObject *parent = 0);
    ~VideoSurfaceOutput();

    /////////////////////////////////////////////// Connector
    ///
    virtual void handleConnectPlayer(Player *player) Q_DECL_FINAL;
    virtual void handleDisconnectPlayer(Player *player) Q_DECL_FINAL;
    virtual void handleAddToMedia(Media *media) Q_DECL_FINAL;
    
    /////////////////////////////////////////////// Interface
    ///
    virtual bool tryLock() Q_DECL_OVERRIDE;
    virtual void lock() Q_DECL_OVERRIDE;
    virtual void unlock() Q_DECL_OVERRIDE;
    virtual const VideoFrame *frame() const Q_DECL_OVERRIDE;

    /////////////////////////////////////////////// VMS
    ///
    virtual void *lockCallback(void **planes) Q_DECL_OVERRIDE;
    virtual void unlockCallback(void *picture,void *const *planes) Q_DECL_OVERRIDE;
    virtual void displayCallback(void *picture) Q_DECL_OVERRIDE;
    virtual unsigned formatCallback(char *chroma,
                                    unsigned *width, unsigned *height,
                                    unsigned *pitches,
                                    unsigned *lines) Q_DECL_OVERRIDE;
    virtual void formatCleanUpCallback() Q_DECL_OVERRIDE;

signals:
    virtual void frameReady() Q_DECL_OVERRIDE;

private:
    VideoFrame *createFrame();

    QMutex m_mutex;

    QSet<VideoFrame *> m_frameSet;
    VideoFrame *m_presentFrame;
    VideoFrame m_frame;
};

} // namespace VLC
} // namespace Phonon

#endif // PHONON_VLC_VIDEOSURFACEOUTPUT_H
