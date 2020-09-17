#pragma once
#include "signalling/vh_stream.h"
#include "signalling/vh_room.h"
#include "thread_lock.h"
#include <map>
#include "vlive_def.h"

class VRtcEngineEventDelegate;
class SubScribeStreamManager
{
public:
    SubScribeStreamManager();
    ~SubScribeStreamManager();

    void RegisterRoomObj(std::shared_ptr<vhall::VHRoom>, VRtcEngineEventDelegate*);

    void InsertSubScribeStreamObj(const std::wstring &subStreamIndex, std::shared_ptr<vhall::VHStream>);
    bool RemoveSubScribeStreamObj(const std::wstring &subStreamIndex, std::string stream_id);
    void ClearSubScribeStream();
    void StopSubScribeStream();
    int GetSubScribeStreamFormat(const std::wstring& user, vlive::VHStreamType streamType, bool &hasVideo, bool &hasAudio);
    std::string GetSubScribeStreamId(const std::wstring& user, vlive::VHStreamType streamType);
    bool IsSubScribeDesktopStream();
    bool IsSubScribeMediaFileStream();
    bool IsSubScribedStreamByUserId(const std::wstring& subStreamIndex);
    bool IsSubScribeSoftwareStream();
    bool HandleRecvStreamLost(const std::wstring& user);
    bool IsHasMediaType(const std::wstring& subStreamIndex, vlive::CaptureStreamAVType type);
    int OperateRemoteUserVideo(std::wstring user_id, bool close/* = false*/);
    int OperateRemoteUserAudio(std::wstring user_id, bool close = false);

    bool StartRenderStream(const std::wstring& subStreamIndex, void* wnd);
    bool StartRenderStream(const std::wstring& subStreamIndex, std::shared_ptr<vhall::VideoRenderReceiveInterface> recv);
    bool StopRenderRemoteStream(const std::wstring& subStreamIndex);
    bool ChangeSubScribeUserSimulCast(const std::wstring& user_id,const std::wstring& subStreamIndex, vlive::VHSimulCastType type);

    static std::string WString2String(const std::wstring& ws);
    static std::wstring String2WString(const std::string& s);
    void MuteAllSubScribeAudio(bool mute);

    int GetAudioLevel(std::wstring user_id);
    double GetVideoLostRate(std::wstring user_id);
private:
    CThreadLock mSubScribeStreamMutex;
    std::map<std::wstring, std::shared_ptr<vhall::VHStream>> mSubScribeStreamMap;
    std::map<std::wstring, std::shared_ptr<vhall::ReceiveVideoSsrc>> mVideoSsrcMap;
    std::shared_ptr<vhall::VHRoom> mLiveRoom = nullptr;
    VRtcEngineEventDelegate *mWebRtcSDKEventReciver = nullptr;
    std::atomic_bool mMuteAllAudio = false;             //静音所有订阅的流
};