#pragma once
#include "vlive_def.h"
#include "video_profile.h"
#include "LiveEventInterface.h"
#include <string>
#include <list>
#include <Windows.h>
#include <functional>
#include "../../Interactive_live_SDK_win/vhall_webrtc_sdk/signalling/tool/VideoRenderReceiver.h"

#ifdef  VHALL_PAAS_SDK_EXPORT
#define VHALL_PAAS_SDK_EXPORT     __declspec(dllimport)
#else
#define VHALL_PAAS_SDK_EXPORT     __declspec(dllexport)
#endif

typedef std::function<void(const std::string&,const std::string&)> SetBroadCastEventCallback;

namespace vlive {


class WebRtcSDKInterface {
public:
    WebRtcSDKInterface() {};

    virtual void SetDebugLogAddr(const std::string& addr) = 0;

    virtual ~WebRtcSDKInterface() {};
    /*
    *   接口说明：初始化SDK 
    *   参数说明：obj，注册SDK接口回调事件处理对象。 logPath：日志存储路径,默认c:\\xxxxx\...\AppData\Roaming\VhallRtc目录下
    */
    virtual void InitSDK(WebRtcSDKEventInterface* obj, std::wstring logPath = std::wstring()) = 0;
    /*
    *  异步接口，监听OnWebRtcRoomConnetEventCallback
    */
    virtual int ConnetWebRtcRoom(const vlive::WebRtcRoomOption& option) = 0;
    virtual bool isConnetWebRtcRoom() = 0;
    /**
    *  接口说明：订阅流。  
    */
    virtual void SubScribeRemoteStream(const std::string &stream_id,int delayTimeOut = 0) = 0;
    /*
     *  接口说明：当接收到远端摄像头数据后，可以修改订阅的画面为大流或小流
    */
    virtual bool ChangeSubScribeUserSimulCast(const std::wstring& user_id, vlive::VHStreamType streamType, VHSimulCastType type) = 0;
    /*
    *   使能订阅流并开始订阅房间中的远端流
    */
    virtual void EnableSubScribeStream() = 0;
    /*
    *  禁止订阅流并取消订阅远端流
    */
    virtual void DisableSubScribeStream() = 0;
    /*清理本地订阅的流*/
    virtual void ClearSubScribeStream() = 0;
    /*
    *  接口说明：设置使能大小流开关。当使能大小流时，本地摄像头推流会推送两路流，订阅到的摄像头流订阅小流。当调用DisConnetWebRtcRoom之后会自动关闭
    *  参数说明：ture 打开， false 关闭
    */
    virtual void EnableSimulCast(bool enable) = 0;
    /*
    *   退出房间，同步接口
    */
    virtual int DisConnetWebRtcRoom() = 0;
    /*
    *   房间是否已连接
    */
    virtual bool IsWebRtcRoomConnected() = 0;
    /*
    *   获取摄像头列表
    **/
    virtual int GetCameraDevices(std::list<vhall::VideoDevProperty> &cameras) = 0;
    /*
    *  获取摄像头列表详情
    */                                   
    virtual int GetCameraDevDetails(std::list<CameraDetailsInfo> &cameraDetails) = 0;
    /*
    *   获取麦克风列表
    **/
    virtual int GetMicDevices(std::list<vhall::AudioDevProperty> &micDevList) = 0;
    /**
    *   获取扬声器列表
    **/
    virtual int GetPlayerDevices(std::list<vhall::AudioDevProperty> &playerDevList) = 0;
    /*
    *  是否支持此分辨率
    */
    virtual bool IsSupported(const std::string& devGuid, VideoProfileIndex index) = 0;
    /*
*  是否支持此分辨率
*/
    //virtual bool IsSupported(const std::string devId, VideoProfileIndex iProfileIndex) = 0;
    /**  是否存在音频或视频设备。
    *  返回值：只要存在一类设备 返回true, 如果音视频设备都没有则返回false
    **/
    virtual bool HasVideoOrAudioDev() = 0;
    virtual bool HasVideoDev() = 0;
    virtual bool HasAudioDev() = 0;
    /**
    * 摄像头画面预览，当预览结束之后需要停止预览，否则摄像头将被一直占用
    */
    virtual int PreviewCamera(void* wnd, const std::string& devGuid, VideoProfileIndex index,int micIndex = -1) = 0;
    /*
    *   切换预览麦克风
    */
    virtual void ChangePreViewMic(int micIndex) = 0;

    /*
    *   获取预览时麦克风音量。
    *   返回值：音量值。
    */
    virtual int GetMicVolumValue() = 0;
    /*
    * 停止摄像头预览
    */
    virtual int StopPreviewCamera() = 0;
    /*
    *  设置使用的麦克风
    *  index: GetMicDevices获取的列表中 vhall::AudioDevProperty中的 devIndex  0,0,  slta 2
    */
    virtual int SetUsedMic(int micIndex,std::string micDevId, std::wstring desktopCaptureId) = 0;
    /**
    *  获取当前使用的麦克风设备信息
    ***/
    virtual void GetCurrentMic(int &index, std::string& devId) = 0;
    /*
    *   获取当前桌面采集设备id
    */
    virtual std::wstring GetCurrentDesktopPlayCaptureId() = 0;
    /**
    *  获取当前使用的摄像设备信息
    ***/
    virtual void GetCurrentCamera(std::string& devId) = 0;
    /*
    *  设置使用的扬声器
    *  index: GetPlayerDevices获取的列表中 vhall::AudioDevProperty中的 devIndex
    */
    virtual int SetUsedPlay(int index, std::string devid) = 0;
    /**
    *  获取当前使用的摄像设备信息
    ***/
    virtual void GetCurrentPlayOut(int &index, std::string& devId) = 0;
    /*  开始摄像头\麦克风采集
    *   参数：
    *       devId: 设备id 
    *       VideoProfileIndex: 分辨率
    *       doublePush: 是否开启双流
    *   回调检测 OnOpenCaptureCallback
    */
    virtual int StartLocalCapture(const std::string devId,VideoProfileIndex index, bool doublePush = false) = 0;
    /*
    *   停止摄像头\麦克风采集
    */
    virtual void StopLocalCapture() = 0;
    /*
    *   开启辅助双推摄像头采集
    */
    virtual int StartLocalAuxiliaryCapture(const std::string devId, VideoProfileIndex index) = 0;
    /*
    *  停止辅助双推摄像头采集
    */
    virtual void StopLocalAuxiliaryCapture() = 0;
    /*
    *   判断图片有效性
    */
    virtual int CheckPicEffectiveness(const std::string filePath) = 0;
    /*
    *   开始采集图片画面
    *   参数：
    *   回调检测 OnOpenCaptureCallback
    **/
    virtual int StartLocalCapturePicture(const std::string filePath,VideoProfileIndex index, bool doublePush = false) = 0;
    /**
    *   开始桌面采集与采集音量控制
    *   参数：
    *       dev_index:设备索引 
    *       dev_id;设备id
    *       float:采集音量 0-100
    **/
    virtual int StartLocalCapturePlayer(const int dev_index,const std::wstring dev_id,const int volume) = 0;
    /*
    *  设置桌面音频采集音量
    **/
    virtual int SetLocalCapturePlayerVolume(const int volume) = 0;
    /*
    *   停止桌面音频采集
    */
    virtual int StopLocalCapturePlayer() = 0;

    /*
    *   开始摄像头数据推流  
    *   回调检测：OnStartPushLocalStream
    */
    virtual int StartPushLocalStream() = 0;
    /*
    *   开始辅助摄像头数据推流
    *   context ： 推流添加的自定义字段
    *   回调检测：OnStartPushLocalAuxiliaryStream
    */
    virtual int StartPushLocalAuxiliaryStream(std::string context = std::string()) = 0;
    /*
    *   停止辅助摄像头数据推流
    *   回调检测：OnStopPushLocalAuxiliaryStream
    */
    virtual int StopPushLocalAuxiliaryStream() = 0;
    /**
    *   当前采集类型是否正在推流
    */
    virtual bool IsPushingStream(vlive::VHStreamType streamType) = 0;
    /*当前房间是否存在给定类型的远端流*/
    virtual bool IsExitSubScribeStream(const vlive::VHStreamType& streamType) = 0;
    virtual bool IsExitSubScribeStream(const std::string& id, const vlive::VHStreamType& streamType) = 0;
    /**
    *   当前互动房间是否存在桌面共享视频流
    */
    //virtual bool IsUserPushingDesktopStream() = 0;
    /**
    *   当前互动房间是否存在插播视频流
    */
    //virtual bool IsUserPushingMediaFileStream() = 0;
    /*
    *   停止摄像头数据推流 回调检测：OnStopPushLocalStream
    */ 
    virtual int StopPushLocalStream() = 0;
    /*
    *   停止接收所有远端数据流
    */
    virtual void StopRecvAllRemoteStream() = 0;
    /*
    *   获取订阅流自定义消息
    */
    virtual std::string GetSubStreamUserData(const std::wstring& user) = 0;
    /**
    *   开始渲染"本地"摄像设备、桌面共享、文件插播媒体数据流.
    */
    virtual bool StartRenderLocalStream(vlive::VHStreamType streamType, void* wnd) = 0;
    virtual bool StartRenderLocalStreamByInterface(vlive::VHStreamType streamType, std::shared_ptr<vhall::VideoRenderReceiveInterface> receive) = 0;
    virtual bool IsStreamExit(std::string id) = 0;

    /**
    *   获取流ID
    *   返回值：如果有流id返回流id 否则返回空字符串
    */
    virtual std::string GetSubScribeStreamId(const std::wstring& user, vlive::VHStreamType streamType) = 0;
    /**
    *   获取订阅流中流类型，是否包括音视频数据。
    *   返回值：0 查找成功，其它失败
    */
    virtual int GetSubScribeStreamFormat(const std::wstring& user, vlive::VHStreamType streamType,bool &hasVideo,bool &hasAudio) = 0;
    /**
    *   开始渲染"远端"摄像设备、桌面共享、文件插播媒体数据流.
    */
    virtual bool StartRenderRemoteStream(const std::wstring& user, vlive::VHStreamType streamType, void* wnd) = 0;
    virtual bool StartRenderRemoteStreamByInterface(const std::wstring& user, vlive::VHStreamType streamType, std::shared_ptr<vhall::VideoRenderReceiveInterface> receive) = 0;
    virtual bool IsRemoteStreamIsExist(const std::wstring& user, vlive::VHStreamType streamType) = 0;

    /*
    *   停止渲染远端流
    */
    virtual bool StopRenderRemoteStream(const std::wstring& user, const std::string streamId, vlive::VHStreamType streamType) = 0;
    /**
    *   获取流ID
    */
    virtual std::string GetUserStreamID(const std::wstring user_id, VHStreamType streamType) = 0;
    /**
    *  判断当前是否可以进行混流与设置大屏
    */
    virtual bool IsEnableConfigMixStream() = 0;

    /*
    *   设置API混流接口调用使能
    */
    virtual void SetEnableConfigMixStream(bool enable) = 0;
    /**
    *   开启房间旁路混流
    *   layoutMode: 如果layoutMode设置为均分模式，需要传递参数gridSize。
    **/
    virtual int StartConfigBroadCast(LayoutMode layoutMode, int width, int height, int fps, int bitrate, bool showBorder,std::string boradColor = std::string("0x666666"), std::string backGroundColor =  std::string("0x000000"), SetBroadCastEventCallback fun = nullptr) = 0;
    /**
    *   开启房间旁路混流
    *   layoutMode: 如果layoutMode设置为均分模式，需要传递参数gridSize。
    **/
    virtual int StartConfigBroadCast(LayoutMode layoutMode, BroadCastVideoProfileIndex profileIndex, bool showBorder, std::string boradColor = std::string("0x666666"), std::string backGroundColor  = std::string("0x000000"), SetBroadCastEventCallback fun = nullptr) = 0;
    /*
    *   停止互动旁路推流
    */
    virtual int StopBroadCast() = 0;
    /*
    *   设置旁路混流布局
    **/
    virtual int SetConfigBroadCastLayOut(LayoutMode mode, SetBroadCastEventCallback fun = nullptr) = 0;
    /*
    *   设置混流端大画面流 回调检测：OnSetMixLayoutMainView
    */
    virtual int SetMixLayoutMainView(std::string stream, SetBroadCastEventCallback fun = nullptr) = 0;
    /*
    *   开始桌面共享采集 ,如果桌面采集分辨率过大，内部推流时会进行裁剪，默认宽高为 1280 * 720，可根据接口SetDesktopCutSize设置推流分辨率。
    *   回调检测：OnStartDesktopCaptureSuc OnStartDesktopCaptureErr
    */
    virtual int StartDesktopCapture(int64_t index, VideoProfileIndex profileIndex) = 0;
    /*
    *   停止桌面共享采集
    */
    virtual void StopDesktopCapture() = 0;
    /*
    *   开始桌面共享采集推流 回调检测：OnStartPushDesktopStream
    */
    virtual int StartPushDesktopStream(std::string context) = 0;
    /*
    *   停止桌面共享采集推流 回调检测：OnStopPushDesktopStream
    */
    virtual int StopPushDesktopStream() = 0;
    //
    virtual bool GetCaptureStreamType(const std::wstring& user_id, vlive::VHStreamType streamType, vlive::CaptureStreamAVType type) = 0;
    /**
    *  获取插播文件原始大小
    *  
    */
    virtual int GetPlayMeidaFileWidthAndHeight(std::string filePath, int& srcWidth, int &srcHeight) = 0;
    /*
    *   开始插播文件。
    */
    virtual int StartPlayMediaFile(std::string filePath, VideoProfileIndex profileIndex = RTC_VIDEO_PROFILE_720P_16x9_H, long long seekPos = 0) = 0;
    /**
    *   切换插播推流分辨率
    */
    virtual void ChangeMediaFileProfile(VideoProfileIndex profileIndex) = 0;
    /*
    *   查看是否保护查询媒体类型
    */
    virtual bool IsSupportMediaFormat(CaptureStreamAVType type) = 0;
    /*
    *   停止插播文件
    */
    virtual void StopMediaFileCapture() = 0;
    /*
    *   开始插播文件推流  回调检测：OnStartPushMediaFileStream
    */
    virtual int StartPushMediaFileStream() = 0;
    /*
    *   停止插播文件推流 回调检测：OnStopPushMediaFileStream
    */
    virtual void StopPushMediaFileStream() = 0;
    /*
    *   插播文件暂停处理
    */
    virtual void MediaFilePause() = 0;
    /*
    *   插播文件恢复处理
    */
    virtual void MediaFileResume() = 0;
    /*
    *   播文件快进处理
    */
    virtual void MediaFileSeek(const unsigned long long& seekTimeInMs) = 0;
    /*
    *   设置插播文件音量
    */
    virtual int MediaFileVolumeChange(int vol) = 0;
    /*  
    *   插播文件总时长
    */
    virtual const long long MediaFileGetMaxDuration() = 0;
    /*
    *   插播文件当前时长
    */
    virtual const long long MediaFileGetCurrentDuration() = 0;
    /*
    *   插播文件的当前状态 
    *   返回值：播放状态  MEDIA_FILE_PLAY_STATE
    */

    virtual int MediaGetPlayerState() = 0;
    /*
    *   当前采集类型是否正在进行本地数据源采集
    */
    virtual bool IsCapturingStream(vlive::VHStreamType streamType) = 0;
    /*
    *   设置当前使用的麦克风音量
    **/
    virtual int SetCurrentMicVol(int vol) = 0;
    /**
    *   获取当前使用的麦克风音量
    **/
    virtual int GetCurrentMicVol() = 0;
    /**
    *   设置当前使用的扬声器音量
    **/
    virtual int SetCurrentPlayVol(int vol) = 0;
    /**
    *   获取当前使用的扬声器音量
    **/
    virtual int GetCurrentPlayVol() = 0;
    /*
    *   关闭本地摄像头
    */
    virtual bool CloseCamera() = 0;
    /*
    *   打开本地摄像头
    */
    virtual bool OpenCamera() = 0;
	/*
    *   关闭本地摄像头
    */
	virtual bool IsCameraOpen() = 0;
    /*
    *   关闭本地麦克风
    */
    virtual bool CloseMic() = 0;
    /*
    *   打开本地麦克风
    */
    virtual bool OpenMic() = 0;
	/*
    *   本地麦克风是否打开
    */
	virtual bool IsMicOpen() = 0;
    /*
    *   添加播放音频文件接口
    */
    virtual int PlayAudioFile(std::string fileName, int devIndex) = 0;
    /*
    *   获取播放文件音量
    */
    virtual int GetPlayAudioFileVolum() = 0;
    /*
    *   停止播放音频文件接口
    */
    virtual int StopPlayAudioFile() = 0;
    /*
    *    获取窗口共享和桌面共享列表
    **  vlive::VHStreamType      // 3:Screen,5:window
    */
    virtual std::vector<DesktopCaptureSource> GetDesktops(int streamType) = 0;
	/**
    *   设置选择的软件源
    */
	virtual int SelectSource(int64_t id) = 0;
	/**
    *   停止软件源共享采集
    */
	virtual void StopSoftwareCapture() = 0;
	/**
    *   停止软件共享采集推流
    */
	virtual int StopPushSoftWareStream() = 0;
	/**
    *   开始软件共享采集推流
    */
	virtual int StartPushSoftWareStream() = 0;
   virtual std::string GetAuxiliaryId() = 0;
   virtual std::string GetLocalAuxiliaryId() = 0;
    /**
    * 打开、关闭远端用户本地视频
    */
    virtual int OperateRemoteUserVideo(std::wstring user_id, bool close = false) = 0;
    /*
    * 打开、关闭远端用户本地声音
    */
    virtual int OperateRemoteUserAudio(std::wstring user_id, bool close = false) = 0;
    /**
    *  打开或关闭所有订阅的音频
    */
    virtual int MuteAllSubScribeAudio(bool mute) = 0;
	virtual bool GetMuteAllAudio() = 0;
	virtual VHStreamType  CalcStreamType(const bool& bAudio, const bool& bVedio) = 0;
   virtual bool IsPlayFileHasVideo() = 0;
   virtual void GetStreams(std::list<std::string>& streams) = 0;
   virtual std::string LocalStreamId() = 0;
   /**
*  获取推流视频丢包率.3秒读一次
*/
   virtual double GetPushDesktopVideoLostRate() = 0;
};


VHALL_PAAS_SDK_EXPORT WebRtcSDKInterface* GetWebRtcSDKInstance();
VHALL_PAAS_SDK_EXPORT  void DestroyWebRtcSDKInstance();

}
