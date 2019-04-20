#include "videothread.h"

VideoThread::VideoThread()
{

}

VideoThread::~VideoThread()
{

}

void VideoThread::run()
{
    char out[10000] = {0};
    while(!is_exit)
    {
        //若处于暂停状态，不处理
        if(!MFFmpeg::Get()->isPlay)
        {
            msleep(10);
            continue;
        }

        while(videos.size()>0)//确定list里有AVPacket包
        {
            //每次取出列表里的第一个包
            AVPacket pack = videos.front();

            //获得取出包的pts
            int pts = MFFmpeg::Get()->GetPts(&pack);

            if(pts > apts)
            {
                break;
            }

            //解码视频帧
            MFFmpeg::Get()->Decode(&pack);

            //清理该包
            av_packet_unref(&pack);

            //从列表删除处理好的视频帧
            videos.pop_front();
        }

        int free =
    }
}
