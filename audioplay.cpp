#include "audioplay.h"

class MAudioPlay :public AudioPlay
{
public:
    QAudioOutput *output = nullptr;
    QIODevice *io = nullptr;
    QMutex mutex;
    void Stop()
    {
        mutex.lock();
        if(output)
        {
            output->stop();
            delete output;
            output = nullptr;
            io = nullptr;
        }

        mutex.unlock();
    }


    bool Start()
    {
        Stop();
        mutex.lock();
        //QAudioOutput *out;//播放音频
        QAudioFormat fmt;//设置音频输出格式

        fmt.setSampleRate(sampleRate);
        fmt.setSampleSize(sampleSize);
        fmt.setChannelCount(channel);
        fmt.setCodec("audio/pcm");
        fmt.setByteOrder(QAudioFormat::LittleEndian);
        fmt.setSampleType(QAudioFormat::UnSignedInt);

        output = new QAudioOutput(fmt);
        io = output->start();
        mutex.unlock();
        return true;

    }

    void Play(bool isplay)
    {
        mutex.lock();
        if(!output)
        {
            mutex.unlock();
            return;
        }

        if(isplay)
        {
            output->resume();//恢复播放
        }
        else {
            output->suspend();//暂停播放
        }
        mutex.unlock();
    }
    int GetFree()
    {
        mutex.lock();
        if (!output)
            {
                mutex.unlock();
                return 0;
            }
        int free = output->bytesFree();//剩余的空间

        mutex.unlock();

        return free;
    }

    bool Write(const char *data,int datasize)
    {
        mutex.lock();
        if (io)
            io->write(data, datasize);//将获取的音频写入到缓冲区中
        mutex.unlock();
        return true;
    }

};

AudioPlay::AudioPlay()
{

}

AudioPlay::~AudioPlay()
{

}

AudioPlay *AudioPlay::Get()
{
    static MAudioPlay ap;
    return &ap;

}
