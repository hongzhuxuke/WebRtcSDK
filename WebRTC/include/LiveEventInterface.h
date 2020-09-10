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

class WebRtcSDKEventInterface {
public:
    /*
    *   ���������¼�
    */
    virtual void OnWebRtcRoomConnetEventCallback(const vlive::VHRoomConnectEnum,const std::string& joinId = "") = 0;
    /*
    *   ���ش򿪲ɼ��豸,��������ͷ�����湲����ʼ�岥��Ƶ�ص�,
    */
    virtual void OnOpenCaptureCallback(vlive::VHStreamType streamType, vlive::VHCapture code, bool hasVideo, bool hasAudio) = 0;
    /*
    *   �����ɹ��ص�
    */
    virtual void OnPushStreamSuc(vlive::VHStreamType streamType, std::string& streamid, bool hasVideo, bool hasAudio, std::string& attributes) = 0;
    /*
    *   ����ʧ�ܻص�
    */
    virtual void OnPushStreamError(std::string streamId, vlive::VHStreamType streamType, const int codeErr = 0, const std::string& msg = std::string()) = 0;
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
    virtual void OnSubScribeStreamErr(const std::string& user_id, const std::string& msg,int errorCode) = 0;
    /*
    *    ���յ�Զ�˵�ý����  hasVideo ���Ƿ������Ƶ
    *    �����յ�Զ��ý���������岥��Ƶ���������湲��������������Ѿ��򿪲岥�����湲��ʱ�Զ�ֹͣ��
    */
    virtual void OnReciveRemoteUserLiveStream(const std::wstring& user, const std::string& streamid, const vlive::VHStreamType type, bool hasVideo, bool hasAudio) = 0;
    /*
    *   Զ�˵�ý�����˳���
    */
    virtual void OnRemoveRemoteUserLiveStream(const std::wstring& user, const std::string& stream, const vlive::VHStreamType type) = 0;

    /*
    *  
    */
    virtual void OnStreamMixed(const std::string& user, const std::string& stream, int type) = 0;
    virtual void OnRemoteUserLiveStreamRemoved(const std::wstring& user, const std::string& streamid, const vlive::VHStreamType type) = 0;
    /*
    *   ���Ĵ�С���ص�
    */
    virtual void OnChangeSubScribeUserSimulCast(const std::wstring& user_id, vlive::VHSimulCastType type,int code, std::string msg) = 0;
};

#endif // !H_WEBRTRCSDK_EVENT_INTERFACE_H
