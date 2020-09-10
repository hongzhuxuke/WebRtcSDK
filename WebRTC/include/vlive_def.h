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

        int role = 0;//角色
        int classType = 0; //1小班课  2	大班课
        std::string userData;
        std::wstring strRoomToken;              //房间token
        std::wstring strLogUpLogUrl;            //日志上报URL
        std::wstring strUserId;                 //用户ID
        std::wstring strRoomId;                 //房间ID
        std::wstring strPassRoomId;                 //paasSdk房间ID
        std::wstring strThirdPushStreamUrl;     //混流端地址，用户混流端布局参数
        std::wstring strappid = L"0";           //应用id
        std::string strVid = "";    //子账号id
        std::string strVfid = "";   //父账号id
        std::wstring strBusinesstype = L"0";    //业务类型（平台类型列表）0: saas 1: pass
        std::wstring strAccountId = L"0";       //开发者ID
        int nWebRtcSdkReConnetTime = 10;      //房间连接超时重连次数，SDK底层业务没3秒触发一次，3000代表网络断开时重连超时时间为9000秒 */
    };

    enum VHStreamType {
		VHStreamType_AUDIO = 0,						//纯音频
		VHStreamType_VIDEO = 1,						//纯视频，没有音频
      VHStreamType_AVCapture = 2,                 //音视频数据,图片采集
      VHStreamType_Desktop = 3,                   //桌面共享数据
      VHStreamType_MediaFile = 4,                 //插播视频数据
		VHStreamType_SoftWare = 5,                  //软件共享数据
		VHStreamType_RegionShare = 6,    //区域共享数据

      VHStreamType_Auxiliary_CamerCapture = 7,    //双推摄像头
      VHStreamType_Stu_Desktop,                 //学员桌面共享
		VHStreamType_Preview_Video ,             //预览视频数据，图片采集
      VHStreamType_Count
    };

    enum VHDoubleStreamType
    {
       VHDoubleStreamType_Camera =1,   //摄像头
       VHDoubleStreamType_Desktop = 2, //桌面
    };

    enum CaptureStreamAVType {
        CaptureStreamAVType_Audio = 0,
        CaptureStreamAVType_Video
    };


    enum VhallLiveErrCode {
        VhallLive_OK = 0,
        VhallLive_ROOM_DISCONNECT,      //房间已经断开或网络异常
        VhallLive_NO_OBJ,               //调用的对象为空
        VhallLive_NO_DEVICE,            //没有音视频设备或采集失败
        VhallLive_NO_PERMISSION,        //无权限
        VhallLive_MUTEX_OPERATE,        //互斥操作，桌面共享和插播一个房间只能一个成员触发
        VhallLive_NOT_SUPPROT,          //不支持的分辨率
        VhallLive_PARAM_ERR,            //参数错误
        VhallLive_IS_PROCESSING,        //正在处理中
        VhallLive_SERVER_NOT_READY,     //服务器端未就绪
        VhallLive_ERROR,
    };

    enum MEDIA_FILE_PLAY_STATE
    {
        PLAY_STATE_NothingSpecial = 0,
        PLAY_STATE_Opening,               //正在打开文件
        PLAY_STATE_Buffering,             //正在加载文件
        PLAY_STATE_Playing,               //正在播放中
        PLAY_STATE_Paused,                //已暂停
        PLAY_STATE_Stopped,               //已停止
        PLAY_STATE_Ended,                 //播放完毕
        PLAY_STATE_Error                  //播放失败
    };


    enum VHRoomConnectEnum
    {
        VHRoomConnect_ROOM_CONNECTED = 0,       //互动房间连接成功
        VHRoomConnect_ROOM_ERROR,               //互动房间连接失败，连接重试超时之后上报次消息
        VHRoomConnect_ROOM_DISCONNECTED,        //网络异常时提示网络已经断开
        VHRoomConnect_ROOM_RECONNECTING,        //正在进行网络重连
        VHRoomConnect_RE_CONNECTED,             //网络重连已经恢复
        VHRoomConnect_ROOM_RECOVER,             //网络已经恢复
        VHRoomConnect_ROOM_NETWORKCHANGED,      //网络发生变化
        VHRoomConnect_ROOM_NETWORKRECOVER,      //网络已经恢复
        VHRoomConnect_ROOM_MIXED_STREAM_READY, //互动房间旁路布局使能事件，当接收到此消息之后SDK端才能进行旁路与大画面设置
    };

    enum VHCapture {
        //success
        VHCapture_OK = 0,               //采集成功
        VHCapture_ACCESS_DENIED,        //采集失败
        VHCapture_VIDEO_DENIED,         //视频采集失败
        VHCapture_AUDIO_DENIED,         //音频采集失败
        VHCapture_STREAM_SOURCE_LOST,   //摄像设备采集过程中采集失败 ,SDK将结束采集与推流
        VHCapture_PLAY_FILE_ERR,        //播放文件失败
    };

    enum VHSimulCastType {
        ///* 设置大小流 0小流 1大流 *
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