#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QtGui/QMainWindow>

namespace Phonon { class VideoWidget; }
class MediaController;

class MainWindow : public QMainWindow
{
    Q_OBJECT
public:
    explicit MainWindow(QWidget *parent = 0);

signals:

public slots:

private slots:
    void effectsDialog();

protected:
    virtual void contextMenuEvent( QContextMenuEvent *e );

private:
    MediaController *mediaControl;
    Phonon::VideoWidget *videoWidget;
};

#endif // MAINWINDOW_H
