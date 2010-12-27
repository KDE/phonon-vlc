#include "mediacontroller.h"

#include <QtGui/QFileDialog>
//#include <Phonon/MediaSource>
//#include <Phonon/MediaObject>
#include <phonon/mediasource.h>
#include <phonon/mediaobject.h>

using namespace Phonon;

MediaController::MediaController(QObject *parent) :
    QObject(parent)
{
    media = new MediaObject(this);
}

void MediaController::openFile()
{
    QString file = QFileDialog::getOpenFileName( NULL, "Open a new file to play", "" );
    if( !file.isEmpty() )
    {
        MediaSource s(file);
        playSource( s );
    }
}

void MediaController::openURL()
{

}

void MediaController::playSource( const MediaSource &s )
{
    media->setCurrentSource(s);
    media->play();
}
