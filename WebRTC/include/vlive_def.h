#ifndef H_VLIVE_DEF_H
#define H_VLIVE_DEF_H

#include <string>
#include <video_profile.h>
#include <vhall_dev_format.h>
#include <atomic>

namespace vlive {
    class WebRtcRoomOption {
    public:
        //http://wiki.vhallops.com/pages/viewpage.action?pageId=2491279

        int role = 0;//��ɫ
        int classType = 0; //1С���  2	����
        std::string userData;
        std::wstring strRoomToken;              //����token
        std::wstring strLogUpLogUrl;            //��־�ϱ�URL
        std::wstring strUserId;                 //�û�ID
        std::wstring strRoomId;                 //����ID
        std::wstring strPassRoomId;                 //paasSdk����ID
        std::wstring strThirdPushStreamUrl;     //�����˵�ַ���û������˲��ֲ���
        std::wstring strappid = L"0";           //Ӧ��id
        std::string strVid = "";    //���˺�id
        std::string strVfid = "";   //���˺�id
        std::wstring strBusinesstype = L"0";    //ҵ�����ͣ�ƽ̨�����б�0: saas 1: pass
        std::wstring strAccountId = L"0";       //������ID
        int nWebRtcSdkReConnetTime = 10;      //�������ӳ�ʱ����������SDK�ײ�ҵ��û3�봥��һ�Σ�3000��������Ͽ�ʱ������ʱʱ��Ϊ9000�� */
    };

    enum VHStreamType {
		VHStreamType_AUDIO = 0,						//����Ƶ
		VHStreamType_VIDEO = 1,						//����Ƶ��û����Ƶ
      VHStreamType_AVCapture = 2,                 //����Ƶ����,ͼƬ�ɼ�
      VHStreamType_Desktop = 3,                   //���湲������
      VHStreamType_MediaFile = 4,                 //�岥��Ƶ����
		VHStreamType_SoftWare = 5,                  //�����������
		VHStreamType_RegionShare = 6,    //����������

      VHStreamType_Auxiliary_CamerCapture = 7,    //˫������ͷ
      VHStreamType_Stu_Desktop,                 //ѧԱ���湲��
		VHStreamType_Preview_Video ,             //Ԥ����Ƶ���ݣ�ͼƬ�ɼ�
      VHStreamType_Count
    };

    enum VHDoubleStreamType
    {
       VHDoubleStreamType_Camera =1,   //����ͷ
       VHDoubleStreamType_Desktop = 2, //����
    };

    enum CaptureStreamAVType {
        CaptureStreamAVType_Audio = 0,
        CaptureStreamAVType_Video
    };


    enum VhallLiveErrCode {
        VhallLive_OK = 0,
        VhallLive_ROOM_DISCONNECT,      //�����Ѿ��Ͽ��������쳣
        VhallLive_NO_OBJ,               //���õĶ���Ϊ��
        VhallLive_NO_DEVICE,            //û������Ƶ�豸��ɼ�ʧ��
        VhallLive_NO_PERMISSION,        //��Ȩ��
        VhallLive_MUTEX_OPERATE,        //������������湲��Ͳ岥һ������ֻ��һ����Ա����
        VhallLive_NOT_SUPPROT,          //��֧�ֵķֱ���
        VhallLive_PARAM_ERR,            //��������
        VhallLive_IS_PROCESSING,        //���ڴ�����
        VhallLive_SERVER_NOT_READY,     //��������δ����
        VhallLive_ERROR,
    };

    enum MEDIA_FILE_PLAY_STATE
    {
        PLAY_STATE_NothingSpecial = 0,
        PLAY_STATE_Opening,               //���ڴ��ļ�
        PLAY_STATE_Buffering,             //���ڼ����ļ�
        PLAY_STATE_Playing,               //���ڲ�����
        PLAY_STATE_Paused,                //����ͣ
        PLAY_STATE_Stopped,               //��ֹͣ
        PLAY_STATE_Ended,                 //�������
        PLAY_STATE_Error                  //����ʧ��
    };


    enum VHRoomConnectEnum
    {
        VHRoomConnect_ROOM_CONNECTED = 0,       //�����������ӳɹ�
        VHRoomConnect_ROOM_ERROR,               //������������ʧ�ܣ��������Գ�ʱ֮���ϱ�����Ϣ
        VHRoomConnect_ROOM_DISCONNECTED,        //�����쳣ʱ��ʾ�����Ѿ��Ͽ�
        VHRoomConnect_ROOM_RECONNECTING,        //���ڽ�����������
        VHRoomConnect_RE_CONNECTED,             //���������Ѿ��ָ�
        VHRoomConnect_ROOM_RECOVER,             //�����Ѿ��ָ�
        VHRoomConnect_ROOM_NETWORKCHANGED,      //���緢���仯
        VHRoomConnect_ROOM_NETWORKRECOVER,      //�����Ѿ��ָ�
        VHRoomConnect_ROOM_MIXED_STREAM_READY, //����������·����ʹ���¼��������յ�����Ϣ֮��SDK�˲��ܽ�����·���������
    };

    enum VHCapture {
        //success
        VHCapture_OK = 0,               //�ɼ��ɹ�
        VHCapture_ACCESS_DENIED,        //�ɼ�ʧ��
        VHCapture_VIDEO_DENIED,         //��Ƶ�ɼ�ʧ��
        VHCapture_AUDIO_DENIED,         //��Ƶ�ɼ�ʧ��
        VHCapture_STREAM_SOURCE_LOST,   //�����豸�ɼ������вɼ�ʧ�� ,SDK�������ɼ�������
        VHCapture_PLAY_FILE_ERR,        //�����ļ�ʧ��
    };

    enum VHSimulCastType {
        ///* ���ô�С�� 0С�� 1���� *
        VHSimulCastType_SmallStream = 0,
        VHSimulCastType_BigStream,
    };

    class CameraDetailsInfo {
    public:
        uint32_t mIndex = 0;
        std::string mDevName = "";
        std::string mDevId = "";
        std::list<VideoProfile> profileList;
    };
}


#endif // !H_VLIVE_DEF_H