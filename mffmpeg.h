#ifndef MFFMPEG_H
#define MFFMPEG_H

#include <QtDebug>
#include<iostream>
#include <QMutex>

extern "C"
{
#include<libavformat/avformat.h>
#include<libswscale/swscale.h>
#include<libswresample/swresample.h>
}

class MFFmpeg
{
public:
    MFFmpeg();
    virtual ~MFFmpeg();

public:

    static MFFmpeg *Get()
    {
        static MFFmpeg ff;
        return &ff;
    }

    std::string GetError();//获取错误信息

    //打开解码器获取信息
    long long Open(const char *source);

    void Close();

    //读取视频的每一帧，返回每帧后需要清理空间
    AVPacket Read();

    //解码读取后的视频帧,返回它的pts
    int Decode(const AVPacket *pkt);

    //将解码后的YUV视频帧转化为RGB格式
    bool ToRGB(char *out,int outwidth,int outheight);

    //音频的重采样
    int ToPCM(char *out);

    //获得解码前视频帧的pts
    int GetPts(const AVPacket *pkt);

    //设置进度条的拖动位置后播放
    bool Seek(float pos);

    bool isPlay = false;//播放暂停

    long long totalMs = 0;//视频总时长
    int videoStream = 0;//视频流
    int audioStream = 1;//音频流
    int fps = 0;

    int sampleRate = 48000;//采样率
    int sampleSize = 16;//样本大小
    int channel = 2;//通道数

    int pts;//当前解码帧的时间用于同步


protected:
    int ret;
    char errorbuff[1024];//打开时发生的错误信息
    QMutex mutex;//互斥体
    AVFormatContext *AFCtx = nullptr;//解封装上下文
    AVFrame *yuv = nullptr;//解码后的视频帧数据
    AVFrame *pcm = nullptr;//解码后的音频数据
    SwsContext *video_Sctx = nullptr;//视频转码上下文
    SwrContext *audio_Sctx = nullptr;//音频转码上下文
};

#endif // MFFMPEG_H

