#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QtGui/QMainWindow>

class MediaController;

class MainWindow : public QMainWindow
{
    Q_OBJECT
public:
    explicit MainWindow(QWidget *parent = 0);

signals:

public slots:

protected:
    virtual void contextMenuEvent( QContextMenuEvent *e );

private:
    MediaController *mediaControl;
};

#endif // MAINWINDOW_H
