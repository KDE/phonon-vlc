#ifndef VIDEOEFFECTS_H
#define VIDEOEFFECTS_H

#include <QDialog>

namespace Ui {
    class VideoEffects;
}
namespace Phonon { class VideoWidget; }

class VideoEffects : public QDialog
{
    Q_OBJECT

public:
    explicit VideoEffects(Phonon::VideoWidget *, QWidget *parent = 0);
    ~VideoEffects();

private:
    Ui::VideoEffects *ui;
    Phonon::VideoWidget *videoWidget;

private slots:
    void on_saturationSlider_valueChanged(int value);
    void on_hueSlider_valueChanged(int value);
    void on_contrastSlider_valueChanged(int value);
    void on_brightnessSlider_valueChanged(int value);
};

#endif // VIDEOEFFECTS_H
