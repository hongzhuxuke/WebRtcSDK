 #pragma once

#include "WebRtcSDKInterface.h"
#include "signalling/vh_connection.h"
#include "signalling/vh_data_message.h"
#include "signalling/vh_events.h"
#include "signalling/vh_room.h"
#include "signalling/vh_tools.h"
#include "signalling/vh_stream.h" 
#include "signalling/tool/AudioPlayDeviceModule.h"
#include "signalling/tool/AudioSendFrame.h"
#include "signalling/tool/VideoRenderReceiver.h"
#include "signalling/BaseStack/statsInfo.h"
#include <atomic>
#include <string>
#include <list>
#include "../thread_lock.h"
#include "CTaskEvent.h"
#include "ffmpeg_demuxer.h"
#include <cstdlib>
#include <string.h>
#include <string>
#include "SubScribeStreamManager.h"
#include "CAudioVolume.h"


#define VhallWebRTC_VERSION "1.0.0.0"
#define SDK_SUCCESS "success"
#define SDK_FAILED  "failed"

using namespace vlive;
class WebRtcSDK : public WebRtcSDKInterface, public vhall::AudioSendFrame
{
public:
   WebRtcSDK();
   virtual ~WebRtcSDK();

   virtual void SendAudioData(const void * audio_data, int bits_per_sample, int sample_rate, size_t number_of_channels, size_t number_frames_perChannel);
   virtual void InitSDK(WebRtcSDKEventInterface* obj, std::wstring logPath = std::wstring());
   virtual void SetDebugLogAddr(const std::string& addr);
   virtual int ConnetWebRtcRoom(const vlive::WebRtcRoomOption& option);
   virtual int DisConnetWebRtcRoom();
   virtual bool isConnetWebRtcRoom();
   virtual void ClearSubScribeStream();
   static int ParamPushType(std::string& context);
   /*
    *   是否自动订阅流
    */
   virtual void EnableSubScribeStream();
   virtual void DisableSubScribeStream();
   virtual void SubScribeRemoteStream(const std::string &stream_id, int delayTimeOut = 0);

   //大/小 流控制
   virtual void EnableSimulCast(bool enable);
   virtual bool ChangeSubScribeUserSimulCast(const std::wstring& user_id, vlive::VHStreamType streamType, VHSimulCastType type);
   /*
*   获取订阅流自定义消息
*/
   virtual std::string GetSubStreamUserData(const std::wstring& user);

   virtual bool IsWebRtcRoomConnected();
   virtual bool StartRenderLocalStream(vlive::VHStreamType streamType, void* wnd);
   virtual bool StartRenderLocalStreamByInterface(vlive::VHStreamType streamType, std::shared_ptr<vhall::VideoRenderReceiveInterface> receive);
   virtual bool IsStreamExit(std::string id);
   
   //渲染远端流
   virtual bool StopRenderRemoteStream(const std::wstring& user, const std::string streamId, vlive::VHStreamType streamType);
   virtual bool StartRenderRemoteStream(const std::wstring& user, vlive::VHStreamType streamType, void* wnd);
   virtual bool StartRenderRemoteStreamByInterface(const std::wstring& user, vlive::VHStreamType streamType, std::shared_ptr<vhall::VideoRenderReceiveInterface> receive);
   virtual bool IsRemoteStreamIsExist(const std::wstring& user, vlive::VHStreamType streamType);
   //开始采集
   /*
   * doublePush : 是否推大/小 流
   */
   virtual int StartLocalCapture(const std::string devId, VideoProfileIndex, bool doublePush = false);
   virtual int StartLocalAuxiliaryCapture(const std::string devId, VideoProfileIndex index);  //双推画面推流
   virtual int StartLocalCapturePicture(const std::string filePath, VideoProfileIndex index, bool doublePush = false);
   virtual int StartLocalCapturePlayer(const int dev_index, const std::wstring dev_id, const int volume);
   //开始推流
   virtual int StartPushLocalStream();
   virtual int StartPushLocalAuxiliaryStream(std::string context = std::string());
   //停止采集
   virtual int StopLocalCapturePlayer();
   virtual void StopLocalCapture();
   virtual void StopLocalAuxiliaryCapture();

   //停止推流
   virtual int StopPreviewCamera();
   virtual int StopPushLocalStream();
   virtual int StopPushLocalAuxiliaryStream();

   virtual void StopRecvAllRemoteStream();//停止接受远端流

   virtual int CheckPicEffectiveness(const std::string filePath);
   virtual int SetLocalCapturePlayerVolume(const int volume);
   virtual int PreviewCamera(void* wnd, const std::string& devGuid, VideoProfileIndex index, int micIndex = -1);  //回显显示器
   virtual void ChangePreViewMic(int micIndex);
   virtual int GetMicVolumValue();

   //混流端布局设置
   virtual bool IsEnableConfigMixStream();
   virtual void SetEnableConfigMixStream(bool enable);
   //开启旁路混流
   virtual int StartConfigBroadCast(LayoutMode layoutMode, int width, int height, int fps, int bitrate, bool showBorder, std::string boradColor = std::string("0x666666"), std::string backGroundColor = std::string("0x000000"), SetBroadCastEventCallback fun = nullptr);
   virtual int StartConfigBroadCast(LayoutMode layoutMode, BroadCastVideoProfileIndex profileIndex, bool showBorder, std::string boradColor = std::string("0x666666"), std::string backGroundColor = std::string("0x000000"), SetBroadCastEventCallback fun = nullptr);
   //修改旁路布局
   virtual int SetConfigBroadCastLayOut(LayoutMode mode, SetBroadCastEventCallback fun = nullptr);
   //
   virtual std::string GetUserStreamID(const std::wstring user_id, VHStreamType streamType);
   virtual bool GetCaptureStreamType(const std::wstring& user_id, vlive::VHStreamType streamType, vlive::CaptureStreamAVType type);
   virtual int OperateRemoteUserVideo(std::wstring user_id, bool close = false);//打开、关闭远端用户本地视频
   virtual int OperateRemoteUserAudio(std::wstring user_id, bool close = false);//打开、关闭远端用户本地声音
  //判断设备是否支持
   virtual bool IsSupported(const std::string& devGuid, VideoProfileIndex iProfileIndex);
   //virtual bool IsSupported(const std::string devId, VideoProfileIndex iProfileIndex)
   //设置大流
   virtual int SetMixLayoutMainView(std::string stream, SetBroadCastEventCallback fun = nullptr);
   //停止旁路混流
   virtual int StopBroadCast();

   virtual int StartDesktopCapture(int64_t index, VideoProfileIndex profileIndex);
   /*停止桌面共享采集*/
   virtual void StopDesktopCapture();
   /*开始桌面共享采集推流*/
   virtual int StartPushDesktopStream(std::string context);
   /*停止桌面共享采集推流*/
   virtual int StopPushDesktopStream();
   /*当前采集类型是否正在推流*/
   virtual bool IsPushingStream(vlive::VHStreamType streamType);
   /*当前房间是否存在给定类型的远端流*/
   virtual bool IsExitSubScribeStream(const vlive::VHStreamType& streamType);
   virtual bool IsExitSubScribeStream(const std::string& id, const vlive::VHStreamType& streamType);

   /**   当前互动房间是否存在给定类型的视频流*/
   virtual bool IsCapturingStream(vlive::VHStreamType streamType);
   virtual int GetPlayMeidaFileWidthAndHeight(std::string filePath, int& srcWidth, int &srcHeight);
   virtual int StartPlayMediaFile(std::string filePath, VideoProfileIndex profileIndex, long long seekPos = 0);
   virtual void ChangeMediaFileProfile(VideoProfileIndex profileIndex);
   virtual bool IsSupportMediaFormat(CaptureStreamAVType type);
   /*停止插播文件*/
   virtual void StopMediaFileCapture();
   /*开始插播文件推流*/
   virtual int StartPushMediaFileStream();
   /*停止插播文件推流*/
   virtual void StopPushMediaFileStream();
   //插播文件暂停处理
   virtual void MediaFilePause();
   //插播文件恢复处理
   virtual void MediaFileResume();
   //插播文件快进处理
   virtual void MediaFileSeek(const unsigned long long& seekTimeInMs);
   //插播文件总时长
   virtual const long long MediaFileGetMaxDuration();
   //插播文件当前时长
   virtual const long long MediaFileGetCurrentDuration();
   //插播文件的当前状态
   virtual int MediaGetPlayerState();
   /**获取摄像头列表**/
   virtual int GetCameraDevices(std::list<vhall::VideoDevProperty> &cameras);
   /**获取麦克风列表**/
   virtual int GetMicDevices(std::list<vhall::AudioDevProperty> &micDevList);
   /**获取扬声器列表**/
   virtual int GetPlayerDevices(std::list<vhall::AudioDevProperty> &playerDevList);

   virtual int GetCameraDevDetails(std::list<CameraDetailsInfo> &cameraDetails);
   /*
   *  是否存在音频或视频设备。
   *  返回值：只要存在一类设备 返回true, 如果音视频设备都没有则返回false
   **/
   virtual bool HasVideoOrAudioDev();
   virtual bool HasVideoDev();
   virtual bool HasAudioDev();
   /*
   *  设置使用的麦克风
   *  index: GetMicDevices获取的列表中 vhall::AudioDevProperty中的 devIndex
   */
   virtual int SetUsedMic(int micIndex, std::string micDevId, std::wstring desktopCaptureId);
   /**
   *  获取当前使用的麦克风设备信息
   ***/
   virtual void GetCurrentMic(int &index, std::string& devId);
   /*
   *   获取当前桌面采集设备id
   */
   virtual std::wstring GetCurrentDesktopPlayCaptureId();
   /**
   *  获取当前使用的摄像设备信息
   ***/
   virtual void GetCurrentCamera(std::string& devId);
   /*
   *  设置使用的扬声器
   *  index: GetPlayerDevices获取的列表中 vhall::AudioDevProperty中的 devIndex
   */
   virtual int SetUsedPlay(int index, std::string devid);
   /**
   *  获取当前使用的摄像设备信息
   ***/
   virtual void GetCurrentPlayOut(int &index, std::string& devId);
   virtual int SetCurrentMicVol(int vol);
   virtual int SetCurrentPlayVol(int vol);
   virtual int GetCurrentMicVol();
   virtual int GetCurrentPlayVol();
   //关闭摄像头
   virtual bool CloseCamera();
   //打开摄像头
   virtual bool OpenCamera();
   //关闭摄像头
   virtual bool IsCameraOpen();
   //关闭麦克风
   virtual bool CloseMic();
   //打开麦克风
   virtual bool OpenMic();
   virtual bool IsMicOpen();
   /**
   *   获取订阅流中流类型，是否包括音视频数据。
   *   返回值：0 查找成功，其它失败
   */
   virtual int GetSubScribeStreamFormat(const std::wstring& user, vlive::VHStreamType streamType, bool &hasVideo, bool &hasAudio);
   /**
   *   获取流ID
   *   返回值：如果有流id返回流id 否则返回空字符串
   */
   virtual std::string GetSubScribeStreamId(const std::wstring& user, vlive::VHStreamType streamType);
   /*
   *   设置插播文件音量
   */
   virtual int MediaFileVolumeChange(int vol);
   /*
   *   添加播放音频文件接口
   */
   virtual int PlayAudioFile(std::string fileName, int devIndex);
   /*
   *   获取播放文件音量
   */
   virtual int GetPlayAudioFileVolum();
   /*
   *   停止播放音频文件接口
   */
   virtual int StopPlayAudioFile();
   void PlayFileCallback(int code);
   void ProcessTask();
   static DWORD WINAPI ThreadProcess(LPVOID p);
   /*
   *    获取窗口共享和桌面共享列表
   **  vlive::VHStreamType      // 3:Screen,5:window
   */
   virtual std::vector<DesktopCaptureSource> GetDesktops(int streamType);
   /*设置选择的软件源*/
   virtual int SelectSource(int64_t id);
   /*停止软件源共享采集*/
   virtual void StopSoftwareCapture();
   /*停止软件共享采集推流*/
   virtual int StopPushSoftWareStream();
   /*开始软件共享采集推流*/
   virtual int StartPushSoftWareStream();
   /**
   *  打开或关闭所有订阅的音频
   */
   virtual int MuteAllSubScribeAudio(bool mute);
   virtual bool GetMuteAllAudio();
   VHStreamType  CalcStreamType(const bool& bAudio, const bool& bVedio);
   
   virtual std::string GetAuxiliaryId();
   virtual std::string GetLocalAuxiliaryId();
   virtual bool IsPlayFileHasVideo();
   virtual void GetStreams(std::list<std::string>& streams);
   virtual std::string LocalStreamId();
   /**
  *  获取推流视频丢包率.3秒读一次
  */
   virtual double GetPushDesktopVideoLostRate();
private:
   void ExitThread();
   bool RemoveSubScribeStreamObj(const std::wstring &userId, std::string stream_id);
   std::string GetSubScribeStreamID(const std::wstring &userId);
   void HandleRePublished(int type, std::wstring& user_id, std::string& new_streamId);
   int PrepareMediaFileCapture(VideoProfileIndex profileIndex, long long seekPos);

   static void OnPlayMediaFileCb(int, void *);
   void CheckAssistFunction(int type, bool streamRemove);
   void PushFrontEvent(CTaskEvent task);
   void PostTaskEvent(CTaskEvent task);
   void RelaseStream(std::shared_ptr<vhall::VHStream>&);


   int HandleStartLocalAuxiliaryCapture(const std::string devId, VideoProfileIndex index);
   int HandleStartLocalCapture(const std::string devId, VideoProfileIndex index, bool doublePush);
   int HandleStartLocalCapturePicture(const std::string filePath, VideoProfileIndex index, bool doublePush);
   int HandlePreviewCamera(void* wnd, const std::string& devGuid, VideoProfileIndex index, int micIndex /*= -1*/);
   void HandleRoomConnected(CTaskEvent &task);
   void HandleRoomStreamSubScribe(CTaskEvent &task);
   void HandleRoomStreamSubScribeError(CTaskEvent &task);
   void HandleRoomError(CTaskEvent& event);
   void HandleOnStreamMixed(CTaskEvent& event);
   void HandleRoomReConnecting(CTaskEvent& event);
   void HandleRoomReConnected(CTaskEvent& event);
   void HandleRoomRecover(CTaskEvent& event);
   void HandleRoomNetworkChanged(CTaskEvent& event);
   void HandleStreamRePublished(CTaskEvent& event);
   void HandleStreamFailed(CTaskEvent& event);
   void HandleStreamRemoved(CTaskEvent& event);
   void HandleStreamSubSuccess(CTaskEvent& event);
   void HandlePrivew(CTaskEvent &event);
   void HandleCaptureLocalStream(CTaskEvent &event);
   void HandleCaptureAuxiliaryLocalStrea(CTaskEvent &event);
   void HandleCaptureMediaFile(CTaskEvent &event);
   void HandleCaptureSoftwareStream(CTaskEvent &event);
   void HandleCaptureDesktopStream(CTaskEvent &event);
   void HandleAddStream(CTaskEvent &event);
   void HandleRecvStreamLost(CTaskEvent &event);
   void HandleSetMicDev(CTaskEvent& event);
   void HandleSetPlayerDev(CTaskEvent& event);
   void ClearAllLocalStream();
   
private:

   CThreadLock mRoomThreadLock;
   std::shared_ptr<vhall::VHRoom> mLiveRoom = nullptr;

   CAudioVolume mPreviewAudioVolume;
   CAudioVolume mPlayFileAudioVolume;

   CThreadLock mLocalStreamThreadLock;
   std::shared_ptr<vhall::VHStream> mLocalStream = nullptr;
   HWND mLocalStreamWnd;
   std::shared_ptr<vhall::VideoRenderReceiveInterface> mLocalStreamReceive = nullptr;
   bool mbReceive = false;
   std::atomic_bool mLocalStreamIsCapture = false;    //是否正在采集

   CThreadLock mLocalAuxiliaryStreamThreadLock;
   std::shared_ptr<vhall::VHStream> mLocalAuxiliaryStream = nullptr;
   HWND mLocalAuxiliaryStreamWnd;
   std::atomic_bool mLocalAuxiliaryStreamIsCapture = false;    //是否正在采集

   CThreadLock mPreviewStreamThreadLock;
   std::shared_ptr<vhall::VHStream> mPreviewLocalStream = nullptr; //摄像头预览

   CThreadLock mMediaStreamThreadLock;
   std::shared_ptr<vhall::VHStream> mMediaStream = nullptr;
   std::atomic_bool mMediaStreamIsCapture = false;

   CThreadLock mDesktopStreamThreadLock;
   std::shared_ptr<vhall::VHStream> mDesktopStream = nullptr;
   std::shared_ptr<vhall::SendVideoSsrc> mDesktopVideoSsrc = nullptr;
   std::atomic_bool mDesktopStreamIsCapture = false;

   CThreadLock mSoftwareStreamThreadLock;
   std::shared_ptr<vhall::VHStream> mSoftwareStream = nullptr;
   std::atomic_bool mSoftWareStreamIsCapture = false;

   std::string mLocalStreamId;
   std::string mLocalAuxiliaryStreamId;
   std::string mDesktopStreamId;
   std::string mMediaFileStreamId;
   std::string mSoftwareStreamId;
   //std::string mCurPlayFileName;

   std::string mCurPlayFile;

   std::atomic_bool mbIsWebRtcRoomConnected = false;
   vlive::WebRtcRoomOption mWebRtcRoomOption;
   WebRtcSDKEventInterface *mWebRtcSDKEventReciver = nullptr;

   static SubScribeStreamManager mSubScribeStreamManager;

   std::shared_ptr<vhall::VHTools> mLocalDevTools = nullptr;
   std::shared_ptr<vhall::EventDispatcher> mAudioDeviceDispatcher = nullptr;

   vhall::StreamMsg mCameraStreamConfig;
   vhall::StreamMsg mAuxiliaryStreamConfig;//双推摄像头
   vhall::StreamMsg previewStreamConfig;
   vhall::StreamMsg desktopStreamConfig;
   vhall::StreamMsg mediaStreamConfig;
   vhall::StreamMsg mSoftSourceStreamConfig;

   std::atomic_int mCurrentCameraIndex = 0;
   std::string mCurrentCameraGuid;
   std::atomic_int mCurrentMicIndex = 0;
   std::string mCurrentMicDevid;
   std::atomic_int mCurrentPlayerIndex = 0;
   std::string mCurrentPlayerDevid;
   std::wstring mDesktopCaptureId;

   std::atomic_bool mEnableSimulCast;
   std::atomic_int mCurMicVol = 100;
   std::atomic_int mCurPlayVol = 100;
   std::atomic_bool mbIsOpenMic = true;
   std::atomic_bool mbIsOpenCamera = true;
   std::atomic_bool mbIsOpenPlayer = true;
   std::atomic_bool mbIsEnablePlayFileAndShare = true; //判断当前用户是否能够进行插播和桌面共享，插播和桌面共享只能处理一种，属于互斥的。

   CThreadLock mBroadCastLayoutMutex;
   SetBroadCastEventCallback mBroadCastLayoutCallback = nullptr;

   CThreadLock mainViewMutex;
   SetBroadCastEventCallback mMainViewCallback = nullptr;

   HWND previewWnd = nullptr;

   HANDLE mHThread = nullptr;
   std::thread* mProcessThread = nullptr;
   DWORD  threadId = 0;

   std::atomic_bool mbIsInit = false;
   static HANDLE gTaskEvent;
   static HANDLE gThreadExitEvent;
   static std::atomic_bool bThreadRun;
   static CThreadLock taskListMutex;
   static std::list<CTaskEvent> mtaskEventList;

   std::atomic_bool mSettingCurrentLayoutMode;
   std::atomic_bool mbIsMixedAlready = false;          //是否可以配置混流和大屏
   std::atomic_bool bAutoSubScribeStream = true;       //登录房间后是否自动订阅流。 
   std::atomic_bool mMuteAllAudio = false;             //静音所有订阅的流
   std::atomic_bool mbStopPreviewCamera = true;           //是否已经开启摄像头预览

   int iStreamMixed = 0;
   int iHandleStreamMixed = 0;
   bool mLastCaptureIsCamera = false;
};
