#ifdef  __cplusplus
extern "C" {
#endif
#include <libavutil/imgutils.h>
#include <libavutil/timestamp.h>
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libavutil/avutil.h>
#include <libswscale/swscale.h>
#ifdef  __cplusplus
}
#endif
#pragma warning(disable : 4996)
#include "pictureDecoder.h"
//#include "common/vhall_log.h"
#define MEDIA_TIME_BASE           1000

//#pragma warning(disable : 4996)

namespace vhall {
  PictureDecoder::PictureDecoder() {
    Init();
    /*av_register_all();
    avcodec_register_all();
    avformat_network_init();*/
  }

  PictureDecoder::~PictureDecoder() {
    DeInit();
  }

  int PictureDecoder::Decode(const char* picture, std::shared_ptr<unsigned char[]>& DstData, int &width, int& height) {
    int result = 0;
    int decodedSize = 0;

    result = Parse(picture);
    if (0 != result) {
      return result;
    }
    AVPacket pkt;
    av_init_packet(&pkt);
    result = ReadPacket(&pkt);
    if (0 != result) {
      return result;
    }
    result = InitCodec(mVideoParam);
    if (0 != result) {
      return result;
    }
    result = DecodeVideo(&pkt);
    if (result < 0) {
      av_packet_unref(&pkt);
      return result;
    }
    const AVFrame* frame = nullptr;
    frame = GetDecodedData();
    if (nullptr == frame) {
      return 10;
    }
    av_packet_unref(&pkt);
    if (NULL != frame) {
      if (NULL == mDecodedData) {
        decodedSize = av_image_get_buffer_size(AV_PIX_FMT_YUV420P,
          mVideoParam->width,
          mVideoParam->height,
          1);
        mDecodedData.reset(new (std::nothrow) unsigned char[decodedSize]);
        if (NULL == mDecodedData) {
          //LOGE("NULL == mDecodedData");
          return 11;
        }
      }
      if (mDecodedData) {
        av_image_copy_to_buffer(mDecodedData.get(),
          decodedSize,
          frame->data,
          frame->linesize,
          AV_PIX_FMT_YUV420P,
          mVideoParam->width,
          mVideoParam->height,
          1);
      }
      DstData = mDecodedData;
      width = mVideoParam->width;
      height = mVideoParam->height;
    }
    return 0;
  }

  void PictureDecoder::Init() {
    hasVideo = false;
    mVideoStreamIndex = 0;
    mIfmtCtx = nullptr;
    mImgConvertCtx = nullptr;
    mFrame = nullptr;
    mFrameOut = nullptr;
    mVideoParam = nullptr;
    mVideoCodecCtx = nullptr;
    mVideoCodec = nullptr;
  }

  void PictureDecoder::DeInit() {
    // TODO£º release
    if (mIfmtCtx) {
        avformat_close_input(&mIfmtCtx);
        mIfmtCtx = NULL;
    }
    if (mVideoCodecCtx) {
      avcodec_free_context(&mVideoCodecCtx);
      mVideoCodecCtx = nullptr;
    }
    if (mFrameOut) {
      av_frame_free(&mFrameOut);
      mFrameOut = nullptr;
    }
    if (mFrame) {
      av_frame_free(&mFrame);
      mFrame = nullptr;
    }
    if (mImgConvertCtx) {
      sws_freeContext(mImgConvertCtx);
      mImgConvertCtx = nullptr;
    }
  }

  int PictureDecoder::Parse(const char* picture) {
    int ret = 0;
    ret = avformat_open_input(&mIfmtCtx, picture, NULL, NULL);
    if (ret < 0) {
      char buf[1024] = {0};
      av_strerror(ret, buf, 1024);
      //LOGE("Cannot open input file\n");
      return 1;
    }
    if ((ret = avformat_find_stream_info(mIfmtCtx, NULL)) < 0) {
      //LOGE("Cannot find stream information\n");
      return 2;
    }
    for (unsigned int i = 0; i < mIfmtCtx->nb_streams; i++) {
      AVStream *stream;
      AVCodecParameters *codecpar;
      stream = mIfmtCtx->streams[i];
      codecpar = stream->codecpar;
      /* analyze Audio/Video foramt/Codec info */
      if (codecpar->codec_type == AVMEDIA_TYPE_VIDEO) {
        mVideoParam = std::make_shared<VideoPar>();
        if (NULL == mVideoParam) {
          //LOGE("new mVideoParam fail!\n");
          return 3;
        }
        hasVideo = true;
        mVideoStreamIndex = i;
        mVideoParam->codecId = codecpar->codec_id;
        mVideoParam->framesPerSecond = av_q2d(stream->avg_frame_rate);
        mVideoParam->bitRate = codecpar->bit_rate;
        mVideoParam->format = codecpar->format;
        mVideoParam->height = codecpar->height;
        mVideoParam->width = codecpar->width;
        if (codecpar->extradata_size > 0) {
          mVideoParam->avc_extra_data.reset(new char[codecpar->extradata_size + AV_INPUT_BUFFER_PADDING_SIZE]);
          if (NULL != mVideoParam->avc_extra_data) {
            memcpy(mVideoParam->avc_extra_data.get(), codecpar->extradata, codecpar->extradata_size);
            mVideoParam->avc_extra_size = codecpar->extradata_size;
          }
        }
      }
      
    }
    av_dump_format(mIfmtCtx, 0, picture, 0);
    return ret;
  }

  int PictureDecoder::ReadPacket(AVPacket * packet, AVMediaType mType) {
    int ret = 0;
    if (NULL == packet) {
      return 4;
    }
    av_init_packet(packet);
    std::unique_lock<std::mutex> lock(mReadMtx);
    if ((ret = av_read_frame(mIfmtCtx, packet)) < 0) {
      //LOGE("av_read_frame fail\n");
      return 5;
    }
    lock.unlock();
    mType = mIfmtCtx->streams[packet->stream_index]->codecpar->codec_type;
    AVRational dstRational = { 1, MEDIA_TIME_BASE };
    packet->pts = av_rescale_q_rnd(packet->pts, mIfmtCtx->streams[packet->stream_index]->time_base, dstRational, AV_ROUND_NEAR_INF);
    //LOGI("Demuxer gave frame of stream_index %u\n", packet->stream_index);
    return 0;
  }

  int PictureDecoder::InitCodec(std::shared_ptr<VideoPar>& videoParam) {
    int ret = 0;
    mVideoCodec = avcodec_find_decoder((AVCodecID)videoParam->codecId);
    if (NULL == mVideoCodec) {
      av_log(NULL, AV_LOG_ERROR, "Failed to find decoder for mVideoCodec\n");
      return 6;
    }
    mVideoCodecCtx = avcodec_alloc_context3(mVideoCodec);
    if (!mVideoCodecCtx) {
      av_log(NULL, AV_LOG_ERROR, "Failed to allocate the decoder context for VideoDecode\n");
      return 7;
    }
    mVideoCodecCtx->bit_rate = videoParam->bitRate;
    mVideoCodecCtx->width = videoParam->width;
    mVideoCodecCtx->height = videoParam->height;
    if (videoParam->avc_extra_data && videoParam->avc_extra_size > 0) {
      mVideoCodecCtx->extradata = (uint8_t*)av_mallocz(videoParam->avc_extra_size + AV_INPUT_BUFFER_PADDING_SIZE);
      if (mVideoCodecCtx->extradata) {
        memcpy(mVideoCodecCtx->extradata, videoParam->avc_extra_data.get(), videoParam->avc_extra_size);
        mVideoCodecCtx->extradata_size = videoParam->avc_extra_size;
      }
      //LOGD("avc_extra_size:%d", videoParam->avc_extra_size);
      //LOGD("width:%d", videoParam->width);
      //LOGD("height:%d", videoParam->height);
    }
    ret = avcodec_open2(mVideoCodecCtx, mVideoCodec, NULL);
    if (ret < 0) {
      //LOGE("Failed to open decoder for VideoDecode: AV_LOG_ERROR");
      return 8;
    }
    //LOGD("VideoDecoder Init done");
    return 0;
  }

  int PictureDecoder::DecodeVideo(AVPacket * pkt) {
    int ret = 0;
    ret = avcodec_send_packet(mVideoCodecCtx, pkt);
    if (ret < 0) {
      //LOGE("Error sending a packet for decoding\n");
      return 9;
    }
    return ret;
  }

  const AVFrame * PictureDecoder::GetDecodedData() {
    int ret = 0;
    if (NULL == mFrame) {
      mFrame = av_frame_alloc();
    }
    if (NULL == mFrame) {
      //LOGE("av_frame_alloc failed");
      return NULL;
    }
    ret = avcodec_receive_frame(mVideoCodecCtx, mFrame);
    if (ret == AVERROR(EAGAIN) || ret == AVERROR_EOF) {
      //LOGE("avcodec_receive_frame failed");
      char buf[1024] = { 0 };
      av_strerror(ret, buf, 1024);
      return NULL;
    }
    else if (ret < 0) {
      //LOGE("Fatal Error during decoding\n");
      return NULL;
    }

    if (NULL == mImgConvertCtx) {
      if (NULL == mFrameOut) {
        mFrameOut = av_frame_alloc();
      }
      if (NULL == mFrameOut) {
        return NULL;
      }
      mFrameOut->format = AV_PIX_FMT_YUV420P;
      mFrameOut->width = mVideoCodecCtx->width;
      mFrameOut->height = mVideoCodecCtx->height;
      ret = av_frame_get_buffer(mFrameOut, 1);
      if (ret < 0) {
        //LOGE("[MediaVideoDecode] Could not allocate the video frame data\n");
        return NULL;
      }
      mImgConvertCtx = sws_getContext(mVideoParam->width,
        mVideoParam->height,
        mVideoCodecCtx->pix_fmt,
        mVideoCodecCtx->width,
        mVideoCodecCtx->height,
        AV_PIX_FMT_YUV420P,
        SWS_BICUBIC, NULL, NULL, NULL);

      // update video param
      mVideoParam->format = mVideoCodecCtx->pix_fmt;
      mVideoParam->height = mVideoCodecCtx->height;
      mVideoParam->width = mVideoCodecCtx->width;
      mVideoParam->framesPerSecond = mVideoCodecCtx->framerate.num / mVideoCodecCtx->framerate.den;
    }
    if (mImgConvertCtx && mFrameOut)
    {
      ret = sws_scale(mImgConvertCtx,
        (const unsigned char* const *)mFrame->data,
        mFrame->linesize, 0,
        mVideoCodecCtx->height,
        mFrameOut->data,
        mFrameOut->linesize);
      if (ret < 0) {
        //LOGE("sws_scale failed");
        return NULL;
      }
    }
    return mFrameOut;
  }
}