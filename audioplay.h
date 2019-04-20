#ifndef AUDIOPLAY_H
#define AUDIOPLAY_H

#include <QAudioOutput>
#include<QMutex>

class AudioPlay
{
public:
    AudioPlay();
    virtual ~AudioPlay();

public:
    static AudioPlay *Get();

    virtual bool Start() = 0;//开始
    virtual void Play(bool isplay) = 0;//是否播放
    virtual bool Write(const char *data,int datasize) = 0;//写入音频
    virtual void Stop() = 0;//停止
    virtual int GetFree() = 0;//获取剩余空间

    int sampleRate = 48000;//采样率
    int sampleSize = 16;//样本大小
    int channel = 2;//通道数
};

#endif // AUDIOPLAY_H
