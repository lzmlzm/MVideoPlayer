#ifndef VIDEOWIDGET_H
#define VIDEOWIDGET_H

#include <QWidget>
#include <QPainter>
#include <QOpenGLWidget>
#include "mffmpeg.h"

class VideoWidget: public QOpenGLWidget
{
public:
    VideoWidget(QWidget *parent = nullptr);
    virtual ~VideoWidget();

    void paintEvent(QPaintEvent *event);//窗口重绘
    void timerEvent(QTimerEvent *event);//定时器

};

#endif // VIDEOWIDGET_H
