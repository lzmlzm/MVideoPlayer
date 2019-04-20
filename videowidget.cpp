#include "videowidget.h"

VideoWidget::VideoWidget(QWidget *parent):QOpenGLWidget (parent)
{
    startTimer(20);//设置定时器
    //启动解码
    //TODO
}

VideoWidget::~VideoWidget()
{

}

void VideoWidget::paintEvent(QPaintEvent *event)
{
    static QImage *image = nullptr;
    static int w = 0;
    static int h = 0;

    if(w != width() || h != height())
    {
        //确保图像数据为空
        if(image)
        {
            delete image->bits();//前面的数据
            delete image;
            image = nullptr;
        }
    }

    if(image == nullptr)
    {
        uchar *buf = new uchar[width()*height()*4];//存放解码后的视频空间
        image= new QImage(buf, width(), height(), QImage::Format_ARGB32);
    }

    //将解码后的视频帧转化为RGB
    MFFmpeg::Get()->ToRGB((char *)image->bits(),width(),height());

    QPainter painter;
    painter.begin(this);//清屏
    //绘制解码后的视频
    painter.drawImage(QPoint(0,0),*image);
    painter.end();


}

void VideoWidget::timerEvent(QTimerEvent *event)
{
    this->update();
}
