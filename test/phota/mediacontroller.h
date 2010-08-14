#ifndef MEDIACONTROLLER_H
#define MEDIACONTROLLER_H

#include <QObject>
#include <phonon/MediaObject>
#include <phonon/MediaSource>

using namespace Phonon;



class MediaController : public QObject
{
    Q_OBJECT
public:
    explicit MediaController(QObject *parent = 0);

    void openFile();
    void openURL();
signals:

public slots:

private:
    MediaObject *media;

    void playSource( const MediaSource &);
};

#endif // MEDIACONTROLLER_H
