#ifndef H_WEBRTRCSDK_EVENT_INTERFACE_H
#define H_WEBRTRCSDK_EVENT_INTERFACE_H

#include "vlive_def.h"

/*
**===================================
**
**   ����SDK�¼������ص��ӿڣ����нӿڻص�����SDKҵ���̡߳�
**   ����ص�����appҵ���¼��������ڻص��н��С�
**
**===================================
*/

class VRtcEngineEventDelegate {
public:
    /*
    *   RTC���������¼����������ӳɹ���������״̬�ϱ���
    */
    virtual void OnRtcRoomNetStateCallback(const vlive::RtcRoomNetState) = 0;
    /*
    *    ������·�����¼��ص�
    */
    virtual void OnConfigBroadCast(const int layout,const int profile, const std::string& result, const std::string& msg) = 0;
    /*
    *    ֹͣ��·�����¼��ص�
    */
    virtual void OnStopConfigBroadCast(const std::string& result, const std::string& msg) = 0;
    /*
    *    ������·�¼��ص�
    */
    virtual void OnChangeConfigBroadCastLayout(const int layout,const std::string& result, const std::string& msg) = 0;
    /*
    *    ���ô��沼��
    */
    virtual void OnSetMainView(const std::string main_view_stream_id, int stream_type, const std::string& result, const std::string& msg) = 0;
    /*
    *   ���ش򿪲ɼ��豸,��������ͷ�����湲����ʼ���岥��Ƶ�Ȼص�,
    */
    virtual void OnOpenCaptureCallback(vlive::VHStreamType streamType, vlive::VHCapture code, bool hasVideo, bool hasAudio) = 0;
    /*
    *   �����ɹ��ص�
    */
    virtual void OnPushStreamSuc(vlive::VHStreamType streamType, std::string& streamid, bool hasVideo, bool hasAudio, std::string& attributes) = 0;
    /*
    *   ����ʧ�ܻص�
    */
    virtual void OnPushStreamError(vlive::VHStreamType streamType, const int codeErr = 0, const std::string& msg = std::string()) = 0;
    /*
    *   �������������ɹ�֮������������ID�����ı�֪ͨ
    **/
    virtual void OnRePublishStreamIDChanged(vlive::VHStreamType streamType, const std::wstring& user_id, const std::string& old_streamid , const std::string& new_streamid) = 0;
    /*
    *   ֹͣ�����ص�
    *   code: 0�ɹ��� ����ʧ��
    */
    virtual void OnStopPushStreamCallback(vlive::VHStreamType streamType,int code, const std::string& msg) = 0;
    /**
    *   ���յ�Զ������
    */
    virtual void OnRemoteStreamAdd(const std::string& user_id, const std::string& stream_id, vlive::VHStreamType streamType) = 0;
    /**
    *   ������ʧ�ܡ�
    */
    virtual void OnSubScribeStreamErr(const std::string& stream_id, const std::string& msg,int errorCode, const std::string& join_id = std::string()) = 0;
    /*
    *    ���յ�Զ�˵�ý����  hasVideo ���Ƿ������Ƶ
    *    �����յ�Զ��ý���������岥��Ƶ���������湲��������������Ѿ��򿪲岥�����湲��ʱ�Զ�ֹͣ��
    */
    virtual void OnReciveRemoteUserLiveStream(const std::wstring& user, const std::string& streamid, const vlive::VHStreamType type, bool hasVideo, bool hasAudio, const std::string user_data) = 0;
    /*
    *   Զ�˵�ý�����˳���
    */
    virtual void OnRemoteUserLiveStreamRemoved(const std::wstring& user, const std::string& streamid, const vlive::VHStreamType type) = 0;
    /*
    *   ���Ĵ�С���ص�
    */
    virtual void OnChangeSubScribeUserSimulCast(const std::wstring& user_id, vlive::VHSimulCastType type, int code, std::string msg) = 0;

};

#endif // !H_WEBRTRCSDK_EVENT_INTERFACE_H
