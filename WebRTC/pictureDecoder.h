#pragma once
#ifdef  __cplusplus
extern "C" {
#endif
#include <libavutil/avutil.h>
#ifdef  __cplusplus
}
#endif
#include <iostream>
#include <string>
#include <list>
#include <vector>
#include <mutex>

struct AVPacket;
struct AVFrame;
struct AVCodecContext;
struct AVCodec;
struct AVFormatContext;
struct SwsContext;

namespace vhall {
  class PictureDecoder {
  public:
    PictureDecoder();
    virtual ~PictureDecoder();
    int Decode(const char* picture, std::shared_ptr<unsigned char[]>& DstData, int &width, int& height);
    typedef struct {
      int codecId = 0;
      int format = 0;
      int bitRate = 0;
      int width = 0;
      int height = 0;
      int framesPerSecond = 0;
      int avc_extra_size = 0;
      std::shared_ptr<char[]> avc_extra_data = nullptr;
    } VideoPar;
  private:
    void Init();
    void DeInit();
    int Parse(const char* picture);
    int ReadPacket(AVPacket* packet, enum AVMediaType mType = AVMEDIA_TYPE_VIDEO);
    int InitCodec(std::shared_ptr<VideoPar>& videoParam);
    int DecodeVideo(AVPacket* pkt);
    const AVFrame* GetDecodedData(); // outPut fmt: YUV420p
  private:
    int                       hasVideo;
    int                       mVideoStreamIndex;
    AVFormatContext*          mIfmtCtx;
    std::mutex                mReadMtx;

    struct SwsContext* mImgConvertCtx;
    AVFrame*           mFrame;
    AVFrame*           mFrameOut;
    std::shared_ptr<VideoPar>           mVideoParam;
    AVCodecContext*    mVideoCodecCtx;
    AVCodec*           mVideoCodec;
    std::shared_ptr<unsigned char[]>      mDecodedData;
  };
}