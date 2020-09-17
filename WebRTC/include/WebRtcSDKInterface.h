#pragma once
#include "vlive_def.h"
#include "video_profile.h"
#include "LiveEventInterface.h"
#include "VideoRenderReceiver.h"
#include <string>
#include <list>
#include <Windows.h>
#include <functional>

#ifdef  VHALL_PAAS_SDK_EXPORT
#define VHALL_PAAS_SDK_EXPORT     __declspec(dllimport)
#else
#define VHALL_PAAS_SDK_EXPORT     __declspec(dllexport)
#endif

typedef std::function<void(const std::string&,const std::string&,int errorCode)> SetBroadCastEventCallback;

namespace vlive {


class WebRtcSDKInterface {
public:
    WebRtcSDKInterface() {};

    virtual void SetDebugLogAddr(const std::string& addr) = 0;

    virtual ~WebRtcSDKInterface() {};
    /*
    *   �ӿ�˵������ʼ��SDK 
    *   ����˵����obj��ע��SDK�ӿڻص��¼�������� logPath����־�洢·��,Ĭ��c:\\xxxxx\...\AppData\Roaming\VhallRtcĿ¼��
    */
    virtual void InitSDK(VRtcEngineEventDelegate* obj, std::wstring logPath = std::wstring()) = 0;
    /*
    *  �첽�ӿڣ�����OnRtcRoomNetStateCallback
    */
    virtual int ConnetWebRtcRoom(const vlive::WebRtcRoomOption& option) = 0;
    /*
    *   �˳����䣬ͬ���ӿ�
    */
    virtual int DisConnetWebRtcRoom() = 0;
    /**
    *  �ӿ�˵������������  
    */
    virtual void SubScribeRemoteStream(const std::string &stream_id,int delayTimeOut = 0) = 0;
    /*
     *  �ӿ�˵���������յ�Զ������ͷ���ݺ󣬿����޸Ķ��ĵĻ���Ϊ������С��
    */
    virtual bool ChangeSubScribeUserSimulCast(const std::wstring& user_id, vlive::VHStreamType streamType, VHSimulCastType type) = 0;
    /*
    *  �ӿ�˵��������ʹ�ܴ�С�����ء���ʹ�ܴ�С��ʱ����������ͷ������������·�������ĵ�������ͷ������С����������DisConnetWebRtcRoom֮����Զ��ر�
    *  ����˵����ture �򿪣� false �ر�
    */
    virtual void EnableSimulCast(bool enable) = 0;
    /*
    *   �����Ƿ�������
    */
    virtual bool IsWebRtcRoomConnected() = 0;
    /*
    *   ��ȡ����ͷ�б�
    **/
    virtual int GetCameraDevices(std::list<vhall::VideoDevProperty> &cameras) = 0;
    /*
    *   ��ȡ��˷��б�
    **/
    virtual int GetMicDevices(std::list<vhall::AudioDevProperty> &micDevList) = 0;
    /**
    *   ��ȡ�������б�
    **/
    virtual int GetPlayerDevices(std::list<vhall::AudioDevProperty> &playerDevList) = 0;
    /*
    *  �Ƿ������Ƶ����Ƶ�豸��
    *  ����ֵ��ֻҪ����һ���豸 ����true, �������Ƶ�豸��û���򷵻�false
    **/
    virtual bool HasVideoOrAudioDev() = 0;
    /*
    *  �Ƿ������Ƶ�豸��
    *  ����ֵ��ֻҪ����һ���豸 ����true, �������Ƶ�豸��û���򷵻�false
    **/
    virtual bool HasVideoDev() = 0;
    /*
    *  �Ƿ������Ƶ�豸��
    *  ����ֵ��ֻҪ����һ���豸 ����true, �������Ƶ�豸��û���򷵻�false
    **/
    virtual bool HasAudioDev() = 0;
    /**
    * ����ͷ����Ԥ������Ԥ������֮����ҪֹͣԤ������������ͷ����һֱռ��
    */
    virtual int StartPreviewCamera(void* wnd, const std::string& devGuid, VideoProfileIndex index,int micIndex = -1) = 0;
    /**
    * ����ͷ����Ԥ������Ԥ������֮����ҪֹͣԤ������������ͷ����һֱռ��. �˷����ɼ����ݻ�ͨ��recv��ʽ�ص�����������
    */
    virtual int StartPreviewCamera(std::shared_ptr<vhall::VideoRenderReceiveInterface> recv, const std::string& devGuid, VideoProfileIndex index, int micIndex = -1) = 0;
    /*
    *   ��ȡԤ��ʱ��˷�������
    *   ����ֵ������ֵ��
    */
    virtual int GetPrevieMicVolumValue() = 0;
    /*
    * ֹͣ����ͷԤ��
    */
    virtual int StopPreviewCamera() = 0;
    /*
    *  ����ʹ�õ���˷�
    *  index: GetMicDevices��ȡ���б��� vhall::AudioDevProperty�е� devIndex  0,0,  slta 2
    */
    virtual int SetUsedMic(int micIndex,std::string micDevId, std::wstring desktopCaptureId) = 0;
    /**
    *  ��ȡ��ǰʹ�õ���˷��豸��Ϣ
    ***/
    virtual void GetCurrentMic(int &index, std::string& devId) = 0;
    /*
    *   ��ȡ��ǰ����ɼ��豸id
    */
    virtual std::wstring GetCurrentDesktopPlayCaptureId() = 0;
    /**
    *  ��ȡ��ǰʹ�õ������豸��Ϣ
    ***/
    virtual void GetCurrentCamera(std::string& devId) = 0;
    /*
    *  ����ʹ�õ�������
    *  index: GetPlayerDevices��ȡ���б��� vhall::AudioDevProperty�е� devIndex
    */
    virtual int SetUsedPlay(int index, std::string devid) = 0;
    /**
    *  ��ȡ��ǰʹ�õ������豸��Ϣ
    ***/
    virtual void GetCurrentPlayOut(int &index, std::string& devId) = 0;
    /*  ��ʼ����ͷ\��˷�ɼ�
    *   ������
    *       devId: �豸id 
    *       VideoProfileIndex: �ֱ���
    *       doublePush: �Ƿ���˫��
    *   �ص���� OnOpenCaptureCallback
    */
    virtual int StartLocalCapture(const std::string devId,VideoProfileIndex index, bool doublePush = false) = 0;
    /*
    *   ֹͣ����ͷ\��˷�ɼ�
    */
    virtual void StopLocalCapture() = 0;
    /*
    *   ��������˫������ͷ�ɼ�
    */
    virtual int StartLocalAuxiliaryCapture(const std::string devId, VideoProfileIndex index) = 0;
    /*
    *  ֹͣ����˫������ͷ�ɼ�
    */
    virtual void StopLocalAuxiliaryCapture() = 0;
    /*
    *   �ж�ͼƬ��Ч��
    */
    virtual int CheckPicEffectiveness(const std::string filePath) = 0;
    /*
    *   ��ʼ�ɼ�ͼƬ����
    *   ������
    *   �ص���� OnOpenCaptureCallback
    **/
    virtual int StartLocalCapturePicture(const std::string filePath,VideoProfileIndex index, bool doublePush = false) = 0;
    /**
    *   ��ʼ����ɼ���ɼ���������
    *   ������
    *       dev_index:�豸���� 
    *       dev_id;�豸id
    *       float:�ɼ����� 0-100
    **/
    virtual int StartLocalCapturePlayer(const std::wstring dev_id,const int volume) = 0;
    /*
    *  ����������Ƶ�ɼ�����
    **/
    virtual int SetLocalCapturePlayerVolume(const int volume) = 0;
    /*
    *   ֹͣ������Ƶ�ɼ�
    */
    virtual int StopLocalCapturePlayer() = 0;
    /*
    *   ��ʼ����ͷ��������  
    *   �ص���⣺OnStartPushLocalStream
    */
    virtual int StartPushLocalStream() = 0;
    /*
    *   ��ʼ��������ͷ��������
    *   context �� ������ӵ��Զ����ֶ�
    *   �ص���⣺OnStartPushLocalAuxiliaryStream
    */
    virtual int StartPushLocalAuxiliaryStream(std::string context = std::string()) = 0;
    /*
    *   ֹͣ��������ͷ��������
    *   �ص���⣺OnStopPushLocalAuxiliaryStream
    */
    virtual int StopPushLocalAuxiliaryStream() = 0;
    /**
    *   ��ǰ�ɼ������Ƿ���������
    */
    virtual bool IsPushingStream(vlive::VHStreamType streamType) = 0;
    /**
    *   ��ǰ���������Ƿ�������湲����Ƶ��
    */
    //virtual bool IsUserPushingDesktopStream() = 0;
    /**
    *   ��ǰ���������Ƿ���ڲ岥��Ƶ��
    */
    //virtual bool IsUserPushingMediaFileStream() = 0;
    /*
    *   ֹͣ����ͷ�������� �ص���⣺OnStopPushLocalStream
    */ 
    virtual int StopPushLocalStream() = 0;
    /*
    *   ֹͣ��������Զ��������
    */
    virtual void StopRecvAllRemoteStream() = 0;
    /**
    *   ��ʼ��Ⱦ"����"�����豸�����湲���ļ��岥ý��������.
    */
    virtual bool StartRenderLocalStream(vlive::VHStreamType streamType, void* wnd) = 0;
    /**
    *   ��ʼ��Ⱦ"����"�����豸�����湲���ļ��岥ý��������.
    */
    virtual bool StartRenderLocalStream(vlive::VHStreamType streamType, std::shared_ptr<vhall::VideoRenderReceiveInterface> recv) = 0;
    /**
    *   ��ȡ��ID
    *   ����ֵ���������id������id ���򷵻ؿ��ַ���
    */
    virtual std::string GetSubScribeStreamId(const std::wstring& user, vlive::VHStreamType streamType) = 0;
    /**
    *   ��ȡ�������������ͣ��Ƿ��������Ƶ���ݡ�
    *   ����ֵ��0 ���ҳɹ�������ʧ��
    */
    virtual int GetSubScribeStreamFormat(const std::wstring& user, vlive::VHStreamType streamType,bool &hasVideo,bool &hasAudio) = 0;
    /**
    *   ��ʼ��Ⱦ"Զ��"�����豸�����湲���ļ��岥ý��������.
    */
    virtual bool StartRenderRemoteStream(const std::wstring& user, vlive::VHStreamType streamType, void* wnd) = 0;

    /**
    *   ��ʼ��Ⱦ"Զ��"�����豸�����湲���ļ��岥ý��������.
    */
    virtual bool StartRenderRemoteStream(const std::wstring& user, vlive::VHStreamType streamType, std::shared_ptr<vhall::VideoRenderReceiveInterface> recv) = 0;
    /*
    *   ֹͣ��ȾԶ����
    */
    virtual bool StopRenderRemoteStream(const std::wstring& user, const std::string streamId, vlive::VHStreamType streamType) = 0;
    /**
    *   ��ȡ��ID
    */
    virtual std::string GetUserStreamID(const std::wstring user_id, VHStreamType streamType) = 0;
    /**
    *  �жϵ�ǰ�Ƿ���Խ��л��������ô���
    */
    virtual bool IsEnableConfigMixStream() = 0;
    /*
    *  �Ƿ�֧�ִ˷ֱ���
    */
    virtual bool IsSupported(const std::string devId, VideoProfileIndex iProfileIndex) = 0;
    /*
    *   ����API�����ӿڵ���ʹ��
    */
    virtual void SetEnableConfigMixStream(bool enable) = 0;
    /**
    *   ����������·����
    *   layoutMode: ���layoutMode����Ϊ����ģʽ����Ҫ���ݲ���gridSize��
    **/
    virtual int StartConfigBroadCast(LayoutMode layoutMode, int width, int height, int fps, int bitrate, bool showBorder,std::string boradColor = std::string("0x666666"), std::string backGroundColor =  std::string("0x000000"), SetBroadCastEventCallback fun = nullptr) = 0;
    /**
    *   ����������·����
    *   layoutMode: ���layoutMode����Ϊ����ģʽ����Ҫ���ݲ���gridSize��
    **/
    virtual int StartConfigBroadCast(LayoutMode layoutMode, BroadCastVideoProfileIndex profileIndex, bool showBorder, std::string boradColor = std::string("0x666666"), std::string backGroundColor  = std::string("0x000000"), SetBroadCastEventCallback fun = nullptr) = 0;
    /*
    *   ֹͣ������·����
    */
    virtual int StopBroadCast(SetBroadCastEventCallback fun = nullptr) = 0;
    /*
    *   ������·��������
    **/
    virtual int SetConfigBroadCastLayOut(LayoutMode mode, SetBroadCastEventCallback fun = nullptr) = 0;
    /*
    *   ���û����˴����� �ص���⣺OnSetMixLayoutMainView
    */
    virtual int SetMixLayoutMainView(std::string stream, SetBroadCastEventCallback fun = nullptr) = 0;
    /*
    *   ��ʼ���湲��ɼ� ,�������ɼ��ֱ��ʹ����ڲ�����ʱ����вü���Ĭ�Ͽ��Ϊ 1280 * 720���ɸ��ݽӿ�SetDesktopCutSize���������ֱ��ʡ�
    *   �ص���⣺OnStartDesktopCaptureSuc OnStartDesktopCaptureErr
    */
    virtual int StartDesktopCapture(int64_t index, VideoProfileIndex profileIndex) = 0;
    /*
    *   ֹͣ���湲��ɼ�
    */
    virtual void StopDesktopCapture() = 0;
    /**
    *  �������湲����.��Ҫ�������ɼ�����ʱ������������ɫ������ʱ������ͨ���˽ӿ�����������ȣ���߹������������ȵ�Ч����
    *  �˽ӿڵ��ã���Ҫ�����湲��ɼ��ɹ�֮����е��á�������[OnOpenCaptureCallback]�ص��¼���
    *  ���ر�ʱ�������湲���������ʱ�رա�
    */
    virtual int SetDesktopEdgeEnhance(bool enable) = 0;
    /**
    *  �ӿ�˵��������Ƿ�֧�����չ���
    **/
    virtual bool IsSupprotBeauty() = 0;
    /*
      *   �ӿ�˵������������ͷ���ռ���Level����Ϊ0-5������level=0�Ǳ�ʾ�رգ�������Ҫ��level����Ϊ1-5����ֵ��
      *   �˽ӿڵ���������ͷ������ر�ǰ���þ����ԡ����ı��ֵʱ���������ʵʱ��Ч��
      *   ��Ҫ�ر�ע����ǣ�����豸��һ�δ򿪣���ʹ��[PreviewCamera]�ӿ�ֱ�Ӵ򿪲���ʾ���ԡ����ǵ�
      *   ���豸�Ѿ���ռ�ã�����ʹ��[bool StartRenderLocalStream(vlive::VHStreamType streamType, std::shared_ptr<vhall::VideoRenderReceiveInterface> recv); ]
      *   �ӿڽ��б������Զ�����Ⱦ����Ԥ��û�л���ʱ��ͨ���豸ID����ƥ�䣬�Ƿ���յ����豸���ݣ�������յ���ͨ����vhall::VideoRenderReceiveInterface�����ص�����
      *   �����Զ�����Ⱦ��
      *   �ص������� OnStopPushStreamCallback
      */
    virtual int SetCameraBeautyLevel(int level) = 0;
    virtual int SetPreviewCameraBeautyLevel(int level) = 0;


    /*
    *   ��ʼ���湲��ɼ����� �ص���⣺OnStartPushDesktopStream
    */
    virtual int StartPushDesktopStream() = 0;
    /*
    *   ֹͣ���湲��ɼ����� �ص���⣺OnStopPushDesktopStream
    */
    virtual int StopPushDesktopStream() = 0;
    //
    virtual bool GetCaptureStreamType(const std::wstring& user_id, vlive::VHStreamType streamType, vlive::CaptureStreamAVType type) = 0;
    /**
    *  ��ȡ�岥�ļ�ԭʼ��С
    */
    virtual int GetPlayMeidaFileWidthAndHeight(std::string filePath, int& srcWidth, int &srcHeight) = 0;
    /*
    *   ��ʼ�岥�ļ���
    */
    virtual int InitMediaFile() = 0;
    /*
    *   ��ʼ�岥�ļ�����  �ص���⣺OnStartPushMediaFileStream
    */
    virtual int StartPushMediaFileStream() = 0;
    /*
    *   ֹͣ�岥�ļ����� �ص���⣺OnStopPushMediaFileStream
    */
    virtual void StopPushMediaFileStream() = 0;
    /**
    **  ���š������ɹ�����Ե��ô˽ӿڽ����ļ�����
    */
    virtual bool PlayFile(std::string file, VideoProfileIndex profileIndex) = 0;
    /*
    *  �岥�ļ��Ƿ������Ƶ���档
    */
    virtual bool IsPlayFileHasVideo() = 0;
    /**
    *   �л��岥�����ֱ���
    */
    virtual void ChangeMediaFileProfile(VideoProfileIndex profileIndex) = 0;
    /*
    *   �鿴�Ƿ񱣻���ѯý������
    */
    virtual bool IsSupportMediaFormat(CaptureStreamAVType type) = 0;
    /*
    *   ֹͣ�岥�ļ�
    */
    virtual void StopMediaFileCapture() = 0;

    /*
    *   �岥�ļ���ͣ����
    */
    virtual void MediaFilePause() = 0;
    /*
    *   �岥�ļ��ָ�����
    */
    virtual void MediaFileResume() = 0;
    /*
    *   ���ļ��������
    */
    virtual void MediaFileSeek(const unsigned long long& seekTimeInMs) = 0;
    /*
    *   ���ò岥�ļ�����
    */
    virtual int MediaFileVolumeChange(int vol) = 0;
    /*  
    *   �岥�ļ���ʱ��
    */
    virtual const long long MediaFileGetMaxDuration() = 0;
    /*
    *   �岥�ļ���ǰʱ��
    */
    virtual const long long MediaFileGetCurrentDuration() = 0;
    /*
    *   �岥�ļ��ĵ�ǰ״̬ 
    *   ����ֵ������״̬  MEDIA_FILE_PLAY_STATE
    */

    virtual int MediaGetPlayerState() = 0;
    /*
    *   ��ǰ�ɼ������Ƿ����ڽ��б�������Դ�ɼ�
    */
    virtual bool IsCapturingStream(vlive::VHStreamType streamType) = 0;
    /*
    *   ���õ�ǰʹ�õ���˷�����
    **/
    virtual int SetCurrentMicVol(int vol) = 0;
    /**
    *   ��ȡ��ǰʹ�õ���˷�����
    **/
    virtual int GetCurrentMicVol() = 0;
    /**
    *   ���õ�ǰʹ�õ�����������
    **/
    virtual int SetCurrentPlayVol(int vol) = 0;
    /**
    *   ��ȡ��ǰʹ�õ�����������
    **/
    virtual int GetCurrentPlayVol() = 0;
    /*
    *   �رձ�������ͷ
    */
    virtual bool CloseCamera() = 0;
    /*
    *   �򿪱�������ͷ
    */
    virtual bool OpenCamera() = 0;
	/*
    *   �رձ�������ͷ
    */
	virtual bool IsCameraOpen() = 0;
    /*
    *   �رձ�����˷�
    */
    virtual bool CloseMic() = 0;
    /*
    *   �򿪱�����˷�
    */
    virtual bool OpenMic() = 0;
	/*
    *   ������˷��Ƿ��
    */
	virtual bool IsMicOpen() = 0;
    /*
    *   ��Ӳ�����Ƶ�ļ��ӿ�
    */
    virtual int PlayAudioFile(std::string fileName, int devIndex) = 0;
    /*
    *   ��ȡ�����ļ�����
    */
    virtual int GetPlayAudioFileVolum() = 0;
    /*
    *   ֹͣ������Ƶ�ļ��ӿ�
    */
    virtual int StopPlayAudioFile() = 0;
    /*
    *    ��ȡ���ڹ�������湲���б�
    **  vlive::VHStreamType      // 3:Screen,5:window
    */
    virtual std::vector<DesktopCaptureSource> GetDesktops(int streamType) = 0;
	/**
    *   ����ѡ������Դ
    */
	virtual int SelectSource(int64_t id) = 0;
	/**
    *   ֹͣ���Դ����ɼ�
    */
	virtual void StopSoftwareCapture() = 0;
	/**
    *   ֹͣ�������ɼ�����
    */
	virtual int StopPushSoftWareStream() = 0;
	/**
    *   ��ʼ�������ɼ�����
    */
	virtual int StartPushSoftWareStream() = 0;
    /**
    * �򿪡��ر�Զ���û�������Ƶ
    */
    virtual int OperateRemoteUserVideo(std::wstring user_id, bool close = false) = 0;
    /*
    * �򿪡��ر�Զ���û���������
    */
    virtual int OperateRemoteUserAudio(std::wstring user_id, bool close = false) = 0;
    /**
    *  �򿪻�ر����ж��ĵ���Ƶ
    */
    virtual int MuteAllSubScribeAudio(bool mute) = 0;
    /**
    *  ����״̬
    **/
	 virtual bool GetMuteAllAudio() = 0;

    /**
    *  ��ȡ��/������Ƶ�����ȼ�
    */
    virtual int GetAudioLevel(std::wstring user_id) = 0;

    /**
    *  ��ȡ��/������Ƶ������
    */
    virtual double GetVideoLostRate(std::wstring user_id) = 0;
    /**
    *  ��ȡ������Ƶ������.3���һ��
    */
    virtual double GetPushDesktopVideoLostRate() = 0;
};


VHALL_PAAS_SDK_EXPORT WebRtcSDKInterface* GetWebRtcSDKInstance();
VHALL_PAAS_SDK_EXPORT  void DestroyWebRtcSDKInstance();

}
