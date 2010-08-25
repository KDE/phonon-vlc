#include "mainwindow.h"
#include "mediacontroller.h"
#include "videoeffects.h"

#include <phonon/VideoWidget>

#include <QtGui/QFileDialog>
#include <phonon/MediaSource>
#include <phonon/MediaObject>
#include <QtGui/QMenu>
#include <QtGui/QContextMenuEvent>

using namespace Phonon;

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent)
{
    videoWidget = new Phonon::VideoWidget();
    setCentralWidget (videoWidget);
    mediaControl = new MediaController( this );
    resize( 600, 400 );
}

void MainWindow::contextMenuEvent( QContextMenuEvent *e )
{
    QMenu menu("Control Menu", this );
    menu.addAction( "Open a file", mediaControl, SLOT( openFile() ) );
    menu.addAction( "Open a URL", mediaControl, SLOT( openURL()) );
    menu.addSeparator();
    menu.addAction( "Video Effects", this, SLOT(effectsDialog()) );
    menu.exec(e->globalPos() );
}

void MainWindow::effectsDialog()
{
    VideoEffects *ef = new VideoEffects(videoWidget, this );
    ef->show();
}


