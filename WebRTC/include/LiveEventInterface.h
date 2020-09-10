#ifndef H_WEBRTRCSDK_EVENT_INTERFACE_H
#define H_WEBRTRCSDK_EVENT_INTERFACE_H

#include "vlive_def.h"

/*
**===================================
**
**   互动SDK事件监听回调接口，所有接口回调处于SDK业务线程。
**   如果回调处理app业务事件不建议在回调中进行。
**
**===================================
*/

class WebRtcSDKEventInterface {
public:
    /*
    *   房间链接事件
    */
    virtual void OnWebRtcRoomConnetEventCallback(const vlive::VHRoomConnectEnum,const std::string& joinId = "") = 0;
    /*
    *   本地打开采集设备,包括摄像头、桌面共享、开始插播视频回调,
    */
    virtual void OnOpenCaptureCallback(vlive::VHStreamType streamType, vlive::VHCapture code, bool hasVideo, bool hasAudio) = 0;
    /*
    *   推流成功回调
    */
    virtual void OnPushStreamSuc(vlive::VHStreamType streamType, std::string& streamid, bool hasVideo, bool hasAudio, std::string& attributes) = 0;
    /*
    *   推流失败回调
    */
    virtual void OnPushStreamError(std::string streamId, vlive::VHStreamType streamType, const int codeErr = 0, const std::string& msg = std::string()) = 0;
    /*
    *   本地网络重连成功之后重推流，流ID发生改变通知
    **/
    virtual void OnRePublishStreamIDChanged(vlive::VHStreamType streamType, const std::wstring& user_id, const std::string& old_streamid , const std::string& new_streamid) = 0;
    /*
    *   停止推流回调
    *   code: 0成功， 其他失败
    */
    virtual void OnStopPushStreamCallback(vlive::VHStreamType streamType,int code, const std::string& msg) = 0;
    /**
    *   接收到远端推流
    */
    virtual void OnRemoteStreamAdd(const std::string& user_id, const std::string& stream_id, vlive::VHStreamType streamType) = 0;
    /**
    *   订阅流失败。
    */
    virtual void OnSubScribeStreamErr(const std::string& user_id, const std::string& msg,int errorCode) = 0;
    /*
    *    接收到远端的媒体流  hasVideo ：是否包含视频
    *    当接收到远端媒体流包括插播视频流或者桌面共享流，如果本地已经打开插播或桌面共享时自动停止。
    */
    virtual void OnReciveRemoteUserLiveStream(const std::wstring& user, const std::string& streamid, const vlive::VHStreamType type, bool hasVideo, bool hasAudio) = 0;
    /*
    *   远端的媒体流退出了
    */
    virtual void OnRemoveRemoteUserLiveStream(const std::wstring& user, const std::string& stream, const vlive::VHStreamType type) = 0;

    /*
    *  
    */
    virtual void OnStreamMixed(const std::string& user, const std::string& stream, int type) = 0;
    virtual void OnRemoteUserLiveStreamRemoved(const std::wstring& user, const std::string& streamid, const vlive::VHStreamType type) = 0;
    /*
    *   订阅大小流回调
    */
    virtual void OnChangeSubScribeUserSimulCast(const std::wstring& user_id, vlive::VHSimulCastType type,int code, std::string msg) = 0;
};

#endif // !H_WEBRTRCSDK_EVENT_INTERFACE_H
