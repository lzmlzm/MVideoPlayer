#include "mffmpeg.h"



static double r2d(AVRational r)
{
    return r.den == 0 ? 0 : static_cast<double>(r.num) / static_cast<double>(r.den);
}
MFFmpeg::MFFmpeg()
{
    av_register_all();
}

MFFmpeg::~MFFmpeg()
{

}

std::string MFFmpeg::GetError()
{
    mutex.lock();
    std::string re = this->errorbuff;
    mutex.unlock();
    return re;
}

long long MFFmpeg::Open(const char *source)
{

    Close();//关闭上次播放
    mutex.lock();//加锁

    //打开解封装器
    ret = avformat_open_input(&AFCtx, source, nullptr, nullptr);
    if(ret != 0)
    {
        mutex.unlock();
        av_strerror(ret,errorbuff,sizeof (errorbuff));
        printf("open %s failed : %s\n",source,errorbuff);
        return 0;
    }

    //获取视频总时长
    totalMs = AFCtx->duration / (AV_TIME_BASE);

    //打印视频的详细信息
    av_dump_format(AFCtx, 0, source, 0);

    //解码器
    for (unsigned int i = 0; i < AFCtx->nb_streams; i++)
    {
        AVCodecContext *AC_Ctx = AFCtx->streams[i]->codec;//解码器上下文

        //查找视频流
        if(AC_Ctx->codec_type == AVMEDIA_TYPE_VIDEO)
        {
            videoStream = i;//第i帧
            fps = r2d(AFCtx->streams[i]->avg_frame_rate);

            //查找解码器
            AVCodec *codec = avcodec_find_decoder(AC_Ctx->codec_id);
            if(!codec)
            {
                mutex.unlock();
                printf("can not find video codec\n");
                return 0;
            }

            //打开解码器
            int err = avcodec_open2(AC_Ctx, codec,nullptr);
            if(err != 0)
            {
                mutex.unlock();
                char buf[1024] = {0};
                av_strerror(err, buf, sizeof (buf));
                printf("open codec failed: %s\n",buf);
                return 0;
            }
            printf("open codec success!\n");
        }
        //查找音频流
        else if(AC_Ctx->codec_type == AVMEDIA_TYPE_AUDIO)
        {
            audioStream = i;//第i帧

            //查找对应的音频解码器
            AVCodec *codec = avcodec_find_decoder(AC_Ctx->codec_id);

            if(avcodec_open2(AC_Ctx,codec,nullptr) < 0)
            {
                mutex.unlock();
                return 0;
            }
            this->sampleRate = AC_Ctx->sample_rate;//采样率
            this->channel = AC_Ctx->channels;//采样通道

            //选择样本大小
            switch(AC_Ctx->sample_fmt)
            {

            case AV_SAMPLE_FMT_S16:
                this->sampleSize = 16;
                break;
            case AV_SAMPLE_FMT_S32:
                this->sampleSize = 32;
            default:
                break;
            }
            printf("audio sample_rate :%d sample_size:%d channels:%d",
                   this->sampleRate,this->sampleSize,this->channel);

        }
    }//打开解码器全过程

    printf("file totalSec is %d - %d\n",totalMs / 60,totalMs % 60);
    mutex.unlock();


    return totalMs;



}

void MFFmpeg::Close()
{
    mutex.lock();//需要上锁，以防多线程中这里在close，另一个线程中在读取
    if(AFCtx)
        avformat_close_input(&AFCtx);



    mutex.unlock();

}

AVPacket MFFmpeg::Read()
{
    AVPacket pkt;
    memset(&pkt, 0, sizeof (AVPacket));
    mutex.lock();
    if(!AFCtx)
    {
        mutex.unlock();
        return pkt;
    }

    //利用解封装上下文读取视频帧 存入 pkt
    int err = av_read_frame(AFCtx,&pkt);
    if(err != 0)
    {
        av_strerror(err,errorbuff,sizeof (errorbuff));
    }

    mutex.unlock();
    return pkt;
}

int MFFmpeg::Decode(const AVPacket *pkt)
{
    mutex.lock();

    //打开视频失败
    if(!AFCtx)
        {
        mutex.unlock();
        return NULL;
    }

    //申请解码对象空间
    if(yuv == nullptr)
    {
        yuv = av_frame_alloc();
    }
    if(pcm == nullptr)
    {
        pcm = av_frame_alloc();
    }

    AVFrame *frame = yuv;//frame为解码后的视频流

    if(pkt->stream_index == audioStream)//若为音频
    {
        frame = pcm;//frame为解码后的音频流
    }

    //发送之前读取的pkt
    int ret = avcodec_send_packet(AFCtx->streams[pkt->stream_index]->codec,pkt);
    if(ret != 0)
    {
        mutex.unlock();
        return NULL;
    }

    //解码pkt后存入yuv
    ret = avcodec_receive_frame(AFCtx->streams[pkt->stream_index]->codec,frame);
    if(ret != 0)
    {
        mutex.unlock();
        return NULL;
    }

    qDebug()<< "pts="<<frame->pts;
    mutex.unlock();

    //当前解码的显示时间
    int p = frame->pts*r2d(AFCtx->streams[pkt->stream_index]->time_base);
    if(pkt->stream_index == audioStream)
        this->pts = p;

    return p;
}

bool MFFmpeg::ToRGB(char *out, int outwidth, int outheight)
{
    mutex.lock();
    if(!AFCtx || !yuv)
    {
        mutex.unlock();
        return false;
    }

    AVCodecContext *videoCtx = AFCtx->streams[this->videoStream]->codec;
    video_Sctx = sws_getCachedContext(video_Sctx,videoCtx->width,//初始化视频转码上下文
                                      videoCtx->height,
                                      videoCtx->pix_fmt,//输入像素格式
                                      outwidth,outheight,
                                      AV_PIX_FMT_BGRA,//输出像素格式
                                      SWS_BICUBIC,//转码算法
                                      nullptr,nullptr,nullptr);
    if(!video_Sctx)
    {
        mutex.unlock();
        printf("sws_getCachedContext failed!\n");
        return false;
    }

    uint8_t *data[AV_NUM_DATA_POINTERS] = {0};
    data[0] = (uint8_t*)out;//第一位输出RGB

    int linesize[AV_NUM_DATA_POINTERS] = {0};
    linesize[0] = outwidth *4;//一行的宽度。32位4个字节

    //转码
    int h = sws_scale(
                video_Sctx,yuv->data, //当前处理区域的每个通道数据指针
                yuv->linesize,//每个通道行字节数
                0,videoCtx->height,//原视频帧的高度
                data,//输出的每个通道数据指针
                linesize//输出的每个通道行字节数

                );

    if(h > 0)
    {
        printf("(%d",h);
    }
    mutex.unlock();
    return true;
}

int MFFmpeg::ToPCM(char *out)
{
    mutex.lock();
        if (!AFCtx || !pcm || !out)//文件未打开，解码器未打开，无数据
        {
            mutex.unlock();
            return 0;
        }
        AVCodecContext *ctx = AFCtx->streams[audioStream]->codec;//音频解码器上下文

        //初始化解码音频上下文
        if (audio_Sctx == nullptr)
        {
            audio_Sctx = swr_alloc();
            swr_alloc_set_opts(audio_Sctx,ctx->channel_layout,
                AV_SAMPLE_FMT_S16,
                  ctx->sample_rate,
                  ctx->channels,
                  ctx->sample_fmt,
                  ctx->sample_rate,
                  0,nullptr
                  );
            swr_init(audio_Sctx);
        }

        uint8_t  *data[1];
        data[0] = (uint8_t *)out;

        //转换覆盖进nb_samples里
        int len = swr_convert(audio_Sctx, data, 10000,
            (const uint8_t **)pcm->data,
            pcm->nb_samples
            );
        if (len <= 0)
        {
            mutex.unlock();
            return 0;
        }
        int outsize = av_samples_get_buffer_size(nullptr, ctx->channels,
            pcm->nb_samples,
            AV_SAMPLE_FMT_S16,
            0);

        mutex.unlock();
        return outsize;
}

int MFFmpeg::GetPts(const AVPacket *pkt)
{
   mutex.lock();
   if(!AFCtx)
   {
       mutex.unlock();
       return -1;
   }

   int pts = pkt->pts*r2d(AFCtx->streams[pkt->stream_index]->time_base);
   mutex.unlock();
   return pts;
}

bool MFFmpeg::Seek(float pos)
{
    mutex.lock();
    if (!AFCtx)//未打开视频
        {
            mutex.unlock();
            return false;
        }

    int64_t stamp = 0;
    stamp = pos * AFCtx->streams[videoStream]->duration;//当前它实际的位置
    pts = stamp * r2d(AFCtx->streams[videoStream]->time_base);//获得滑动条滑动后的时间戳

    int re = av_seek_frame(AFCtx, videoStream, stamp,
        AVSEEK_FLAG_BACKWARD | AVSEEK_FLAG_FRAME);//将视频移至到当前位置

    avcodec_flush_buffers(AFCtx->streams[videoStream]->codec);//刷新缓冲,清理掉

    mutex.unlock();
    if (re > 0)
        return true;
    return false;
}

