#include "mainwindow.h"
#include "mediacontroller.h"

#include <phonon/VideoWidget>

#include <QtGui/QFileDialog>
#include <phonon/MediaSource>
#include <phonon/MediaObject>

using namespace Phonon;

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent)
{
    VideoWidget *videoWidget = new Phonon::VideoWidget();
    setCentralWidget (videoWidget);
    mediaControl = new MediaController( this );
}

void MainWindow::contextMenuEvent( QContextMenuEvent *e )
{
    mediaControl->openFile();
}


