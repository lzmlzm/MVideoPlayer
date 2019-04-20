#ifndef VIDEOTHREAD_H
#define VIDEOTHREAD_H

#include<QThread>
#include<list>
#include"mffmpeg.h"
#include"audioplay.h"
using namespace std;
class VideoThread:public QThread
{
public:
    VideoThread();
    virtual ~VideoThread();

    static VideoThread *Get()
    {
        static VideoThread vt;
        return &vt;
    }

    void run();

public:
    static list<AVPacket> videos;//存放解码前的视频帧
    bool is_exit = false;//线程未退出
    int apts = -1;//音频的pts

};

#endif // VIDEOTHREAD_H
