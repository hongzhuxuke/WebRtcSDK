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
    *   �Ƿ��Զ�������
    */
   virtual void EnableSubScribeStream();
   virtual void DisableSubScribeStream();
   virtual void SubScribeRemoteStream(const std::string &stream_id, int delayTimeOut = 0);

   //��/С ������
   virtual void EnableSimulCast(bool enable);
   virtual bool ChangeSubScribeUserSimulCast(const std::wstring& user_id, vlive::VHStreamType streamType, VHSimulCastType type);
   /*
*   ��ȡ�������Զ�����Ϣ
*/
   virtual std::string GetSubStreamUserData(const std::wstring& user);

   virtual bool IsWebRtcRoomConnected();
   virtual bool StartRenderLocalStream(vlive::VHStreamType streamType, void* wnd);
   virtual bool StartRenderLocalStreamByInterface(vlive::VHStreamType streamType, std::shared_ptr<vhall::VideoRenderReceiveInterface> receive);
   virtual bool IsStreamExit(std::string id);
   
   //��ȾԶ����
   virtual bool StopRenderRemoteStream(const std::wstring& user, const std::string streamId, vlive::VHStreamType streamType);
   virtual bool StartRenderRemoteStream(const std::wstring& user, vlive::VHStreamType streamType, void* wnd);
   virtual bool StartRenderRemoteStreamByInterface(const std::wstring& user, vlive::VHStreamType streamType, std::shared_ptr<vhall::VideoRenderReceiveInterface> receive);
   virtual bool IsRemoteStreamIsExist(const std::wstring& user, vlive::VHStreamType streamType);
   //��ʼ�ɼ�
   /*
   * doublePush : �Ƿ��ƴ�/С ��
   */
   virtual int StartLocalCapture(const std::string devId, VideoProfileIndex, bool doublePush = false);
   virtual int StartLocalAuxiliaryCapture(const std::string devId, VideoProfileIndex index);  //˫�ƻ�������
   virtual int StartLocalCapturePicture(const std::string filePath, VideoProfileIndex index, bool doublePush = false);
   virtual int StartLocalCapturePlayer(const int dev_index, const std::wstring dev_id, const int volume);
   //��ʼ����
   virtual int StartPushLocalStream();
   virtual int StartPushLocalAuxiliaryStream(std::string context = std::string());
   //ֹͣ�ɼ�
   virtual int StopLocalCapturePlayer();
   virtual void StopLocalCapture();
   virtual void StopLocalAuxiliaryCapture();

   //ֹͣ����
   virtual int StopPreviewCamera();
   virtual int StopPushLocalStream();
   virtual int StopPushLocalAuxiliaryStream();

   virtual void StopRecvAllRemoteStream();//ֹͣ����Զ����

   virtual int CheckPicEffectiveness(const std::string filePath);
   virtual int SetLocalCapturePlayerVolume(const int volume);
   virtual int PreviewCamera(void* wnd, const std::string& devGuid, VideoProfileIndex index, int micIndex = -1);  //������ʾ��
   virtual void ChangePreViewMic(int micIndex);
   virtual int GetMicVolumValue();

   //�����˲�������
   virtual bool IsEnableConfigMixStream();
   virtual void SetEnableConfigMixStream(bool enable);
   //������·����
   virtual int StartConfigBroadCast(LayoutMode layoutMode, int width, int height, int fps, int bitrate, bool showBorder, std::string boradColor = std::string("0x666666"), std::string backGroundColor = std::string("0x000000"), SetBroadCastEventCallback fun = nullptr);
   virtual int StartConfigBroadCast(LayoutMode layoutMode, BroadCastVideoProfileIndex profileIndex, bool showBorder, std::string boradColor = std::string("0x666666"), std::string backGroundColor = std::string("0x000000"), SetBroadCastEventCallback fun = nullptr);
   //�޸���·����
   virtual int SetConfigBroadCastLayOut(LayoutMode mode, SetBroadCastEventCallback fun = nullptr);
   //
   virtual std::string GetUserStreamID(const std::wstring user_id, VHStreamType streamType);
   virtual bool GetCaptureStreamType(const std::wstring& user_id, vlive::VHStreamType streamType, vlive::CaptureStreamAVType type);
   virtual int OperateRemoteUserVideo(std::wstring user_id, bool close = false);//�򿪡��ر�Զ���û�������Ƶ
   virtual int OperateRemoteUserAudio(std::wstring user_id, bool close = false);//�򿪡��ر�Զ���û���������
  //�ж��豸�Ƿ�֧��
   virtual bool IsSupported(const std::string& devGuid, VideoProfileIndex iProfileIndex);
   //virtual bool IsSupported(const std::string devId, VideoProfileIndex iProfileIndex)
   //���ô���
   virtual int SetMixLayoutMainView(std::string stream, SetBroadCastEventCallback fun = nullptr);
   //ֹͣ��·����
   virtual int StopBroadCast();

   virtual int StartDesktopCapture(int64_t index, VideoProfileIndex profileIndex);
   /*ֹͣ���湲��ɼ�*/
   virtual void StopDesktopCapture();
   /*��ʼ���湲��ɼ�����*/
   virtual int StartPushDesktopStream(std::string context);
   /*ֹͣ���湲��ɼ�����*/
   virtual int StopPushDesktopStream();
   /*��ǰ�ɼ������Ƿ���������*/
   virtual bool IsPushingStream(vlive::VHStreamType streamType);
   /*��ǰ�����Ƿ���ڸ������͵�Զ����*/
   virtual bool IsExitSubScribeStream(const vlive::VHStreamType& streamType);
   virtual bool IsExitSubScribeStream(const std::string& id, const vlive::VHStreamType& streamType);

   /**   ��ǰ���������Ƿ���ڸ������͵���Ƶ��*/
   virtual bool IsCapturingStream(vlive::VHStreamType streamType);
   virtual int GetPlayMeidaFileWidthAndHeight(std::string filePath, int& srcWidth, int &srcHeight);
   virtual int StartPlayMediaFile(std::string filePath, VideoProfileIndex profileIndex, long long seekPos = 0);
   virtual void ChangeMediaFileProfile(VideoProfileIndex profileIndex);
   virtual bool IsSupportMediaFormat(CaptureStreamAVType type);
   /*ֹͣ�岥�ļ�*/
   virtual void StopMediaFileCapture();
   /*��ʼ�岥�ļ�����*/
   virtual int StartPushMediaFileStream();
   /*ֹͣ�岥�ļ�����*/
   virtual void StopPushMediaFileStream();
   //�岥�ļ���ͣ����
   virtual void MediaFilePause();
   //�岥�ļ��ָ�����
   virtual void MediaFileResume();
   //�岥�ļ��������
   virtual void MediaFileSeek(const unsigned long long& seekTimeInMs);
   //�岥�ļ���ʱ��
   virtual const long long MediaFileGetMaxDuration();
   //�岥�ļ���ǰʱ��
   virtual const long long MediaFileGetCurrentDuration();
   //�岥�ļ��ĵ�ǰ״̬
   virtual int MediaGetPlayerState();
   /**��ȡ����ͷ�б�**/
   virtual int GetCameraDevices(std::list<vhall::VideoDevProperty> &cameras);
   /**��ȡ��˷��б�**/
   virtual int GetMicDevices(std::list<vhall::AudioDevProperty> &micDevList);
   /**��ȡ�������б�**/
   virtual int GetPlayerDevices(std::list<vhall::AudioDevProperty> &playerDevList);

   virtual int GetCameraDevDetails(std::list<CameraDetailsInfo> &cameraDetails);
   /*
   *  �Ƿ������Ƶ����Ƶ�豸��
   *  ����ֵ��ֻҪ����һ���豸 ����true, �������Ƶ�豸��û���򷵻�false
   **/
   virtual bool HasVideoOrAudioDev();
   virtual bool HasVideoDev();
   virtual bool HasAudioDev();
   /*
   *  ����ʹ�õ���˷�
   *  index: GetMicDevices��ȡ���б��� vhall::AudioDevProperty�е� devIndex
   */
   virtual int SetUsedMic(int micIndex, std::string micDevId, std::wstring desktopCaptureId);
   /**
   *  ��ȡ��ǰʹ�õ���˷��豸��Ϣ
   ***/
   virtual void GetCurrentMic(int &index, std::string& devId);
   /*
   *   ��ȡ��ǰ����ɼ��豸id
   */
   virtual std::wstring GetCurrentDesktopPlayCaptureId();
   /**
   *  ��ȡ��ǰʹ�õ������豸��Ϣ
   ***/
   virtual void GetCurrentCamera(std::string& devId);
   /*
   *  ����ʹ�õ�������
   *  index: GetPlayerDevices��ȡ���б��� vhall::AudioDevProperty�е� devIndex
   */
   virtual int SetUsedPlay(int index, std::string devid);
   /**
   *  ��ȡ��ǰʹ�õ������豸��Ϣ
   ***/
   virtual void GetCurrentPlayOut(int &index, std::string& devId);
   virtual int SetCurrentMicVol(int vol);
   virtual int SetCurrentPlayVol(int vol);
   virtual int GetCurrentMicVol();
   virtual int GetCurrentPlayVol();
   //�ر�����ͷ
   virtual bool CloseCamera();
   //������ͷ
   virtual bool OpenCamera();
   //�ر�����ͷ
   virtual bool IsCameraOpen();
   //�ر���˷�
   virtual bool CloseMic();
   //����˷�
   virtual bool OpenMic();
   virtual bool IsMicOpen();
   /**
   *   ��ȡ�������������ͣ��Ƿ��������Ƶ���ݡ�
   *   ����ֵ��0 ���ҳɹ�������ʧ��
   */
   virtual int GetSubScribeStreamFormat(const std::wstring& user, vlive::VHStreamType streamType, bool &hasVideo, bool &hasAudio);
   /**
   *   ��ȡ��ID
   *   ����ֵ���������id������id ���򷵻ؿ��ַ���
   */
   virtual std::string GetSubScribeStreamId(const std::wstring& user, vlive::VHStreamType streamType);
   /*
   *   ���ò岥�ļ�����
   */
   virtual int MediaFileVolumeChange(int vol);
   /*
   *   ��Ӳ�����Ƶ�ļ��ӿ�
   */
   virtual int PlayAudioFile(std::string fileName, int devIndex);
   /*
   *   ��ȡ�����ļ�����
   */
   virtual int GetPlayAudioFileVolum();
   /*
   *   ֹͣ������Ƶ�ļ��ӿ�
   */
   virtual int StopPlayAudioFile();
   void PlayFileCallback(int code);
   void ProcessTask();
   static DWORD WINAPI ThreadProcess(LPVOID p);
   /*
   *    ��ȡ���ڹ�������湲���б�
   **  vlive::VHStreamType      // 3:Screen,5:window
   */
   virtual std::vector<DesktopCaptureSource> GetDesktops(int streamType);
   /*����ѡ������Դ*/
   virtual int SelectSource(int64_t id);
   /*ֹͣ���Դ����ɼ�*/
   virtual void StopSoftwareCapture();
   /*ֹͣ�������ɼ�����*/
   virtual int StopPushSoftWareStream();
   /*��ʼ�������ɼ�����*/
   virtual int StartPushSoftWareStream();
   /**
   *  �򿪻�ر����ж��ĵ���Ƶ
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
  *  ��ȡ������Ƶ������.3���һ��
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
   std::atomic_bool mLocalStreamIsCapture = false;    //�Ƿ����ڲɼ�

   CThreadLock mLocalAuxiliaryStreamThreadLock;
   std::shared_ptr<vhall::VHStream> mLocalAuxiliaryStream = nullptr;
   HWND mLocalAuxiliaryStreamWnd;
   std::atomic_bool mLocalAuxiliaryStreamIsCapture = false;    //�Ƿ����ڲɼ�

   CThreadLock mPreviewStreamThreadLock;
   std::shared_ptr<vhall::VHStream> mPreviewLocalStream = nullptr; //����ͷԤ��

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
   vhall::StreamMsg mAuxiliaryStreamConfig;//˫������ͷ
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
   std::atomic_bool mbIsEnablePlayFileAndShare = true; //�жϵ�ǰ�û��Ƿ��ܹ����в岥�����湲���岥�����湲��ֻ�ܴ���һ�֣����ڻ���ġ�

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
   std::atomic_bool mbIsMixedAlready = false;          //�Ƿ�������û����ʹ���
   std::atomic_bool bAutoSubScribeStream = true;       //��¼������Ƿ��Զ��������� 
   std::atomic_bool mMuteAllAudio = false;             //�������ж��ĵ���
   std::atomic_bool mbStopPreviewCamera = true;           //�Ƿ��Ѿ���������ͷԤ��

   int iStreamMixed = 0;
   int iHandleStreamMixed = 0;
   bool mLastCaptureIsCamera = false;
};
