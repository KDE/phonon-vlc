#include "videoeffects.h"
#include "ui_videoeffects.h"
#include <assert.h>

#include <Phonon/VideoWidget>

VideoEffects::VideoEffects(Phonon::VideoWidget *_w, QWidget *parent) :
    QDialog(parent),
    ui(new Ui::VideoEffects),
    videoWidget(_w)
{
    ui->setupUi(this);
    assert(_w);
}

VideoEffects::~VideoEffects()
{
    delete ui;
}

void VideoEffects::on_brightnessSlider_valueChanged(int value)
{
    videoWidget->setBrightness(qreal(value)/100);
}

void VideoEffects::on_contrastSlider_valueChanged(int value)
{
    videoWidget->setContrast(qreal(value)/100);
}

void VideoEffects::on_hueSlider_valueChanged(int value)
{
    videoWidget->setHue(qreal(value)/100);
}

void VideoEffects::on_saturationSlider_valueChanged(int value)
{
    videoWidget->setSaturation(qreal(value)/100);
}
