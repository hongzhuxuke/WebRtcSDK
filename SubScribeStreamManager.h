#pragma once
#include "signalling/vh_stream.h"
#include "signalling/vh_room.h"
#include "thread_lock.h"
#include <map>
#include "vlive_def.h"

class WebRtcSDKEventInterface;
class SubScribeStreamManager
{
public:
    SubScribeStreamManager();
    ~SubScribeStreamManager();

    void RegisterRoomObj(std::shared_ptr<vhall::VHRoom>, WebRtcSDKEventInterface*);

    void InsertSubScribeStreamObj(const std::wstring &subStreamIndex, std::shared_ptr<vhall::VHStream>);
    bool RemoveSubScribeStreamObj(const std::wstring &subStreamIndex, std::string stream_id);
    void ClearSubScribeStream();
    void StopSubScribeStream();
    int GetSubScribeStreamFormat(const std::wstring& user, vlive::VHStreamType streamType, bool &hasVideo, bool &hasAudio);
    std::string GetSubScribeStreamId(const std::wstring& user, vlive::VHStreamType streamType);
    std::string GetAuxiliaryId();

    bool IsSubScribeDesktopStream();
    bool IsSubScribeMediaFileStream();

    std::string GetUserData(const std::wstring& user_id);
    //扬声器都是针对远端用户设置的。
    int ChangeUserPlayDev(int index, int vol);
    int SetCurrentPlayVol(int vol);
    bool IsSubScribedStreamByUserId(const std::wstring& subStreamIndex);
    bool IsSubScribeSoftwareStream();
    bool HandleRecvStreamLost(const std::wstring& user);
    bool IsHasMediaType(const std::wstring& subStreamIndex, vlive::CaptureStreamAVType type);

    int OperateRemoteUserVideo(std::wstring user_id, bool close/* = false*/);
    int OperateRemoteUserAudio(std::wstring user_id, bool close = false);

    bool StartRenderStream(const std::wstring& subStreamIndex, void* wnd);
    bool StartRenderRemoteStreamByInterface(const std::wstring& subStreamIndex, std::shared_ptr<vhall::VideoRenderReceiveInterface> receive);
    bool IsRemoteStreamIsExist(const std::wstring & user);
    bool StopRenderRemoteStream(const std::wstring& subStreamIndex);
    bool ChangeSubScribeUserSimulCast(const std::wstring& subStreamIndex, vlive::VHSimulCastType type);

    void SetPlayIndexAndVolume(const std::wstring& index,int PlayerIndex, int PlayVol);
    static std::string WString2String(const std::wstring& ws);
    static std::wstring String2WString(const std::string& s);
    void MuteAllSubScribeAudio(bool mute);
    bool IsExitStream(const vlive::VHStreamType& streamType);
    bool IsCurIdExitStream(const std::string& id, const vlive::VHStreamType& streamType);
    virtual void GetStreams(std::list<std::string>& streams);
private:
    CThreadLock mSubScribeStreamMutex;
    std::map<std::wstring, std::shared_ptr<vhall::VHStream>> mSubScribeStreamMap;

    std::shared_ptr<vhall::VHRoom> mLiveRoom = nullptr;
    WebRtcSDKEventInterface *mWebRtcSDKEventReciver = nullptr;
    std::atomic_bool mMuteAllAudio = false;             //静音所有订阅的流
};