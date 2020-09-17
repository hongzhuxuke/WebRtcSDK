#include "SubScribeStreamManager.h"
#include "./WebRTC/vhall_sdk_log.h"
#include "WebRtcSDKInterface.h"
#include "signalling\BaseStack\statsInfo.h"
using namespace webrtc_sdk;
SubScribeStreamManager::SubScribeStreamManager()
{
}


SubScribeStreamManager::~SubScribeStreamManager()
{
}

void SubScribeStreamManager::RegisterRoomObj(std::shared_ptr<vhall::VHRoom> obj, VRtcEngineEventDelegate* callback) {
    mLiveRoom = obj;
    mWebRtcSDKEventReciver = callback;
}

void SubScribeStreamManager::InsertSubScribeStreamObj(const std::wstring &subStreamIndex, std::shared_ptr<vhall::VHStream> stream) {
    LOGD_SDK("userId:%ws", subStreamIndex.c_str());
    CThreadLockHandle lockHandle(&mSubScribeStreamMutex);
    LOGD_SDK("mSubScribeStreamMutex");

    std::map<std::wstring, std::shared_ptr<vhall::VHStream>>::iterator iter = mSubScribeStreamMap.find(subStreamIndex);
    if (iter == mSubScribeStreamMap.end()) {
        LOGD_SDK("insert userId:%ws stream:%s", subStreamIndex.c_str(), stream->GetID());
        mSubScribeStreamMap.insert(make_pair(subStreamIndex, stream));
        std::map<std::wstring, std::shared_ptr<vhall::ReceiveVideoSsrc>>::iterator iterSsrc = mVideoSsrcMap.find(subStreamIndex);
        if (iterSsrc == mVideoSsrcMap.end()) {
           std::shared_ptr<vhall::ReceiveVideoSsrc> desktopVideoSsrc = std::make_shared<vhall::ReceiveVideoSsrc>();
           mVideoSsrcMap.insert(make_pair(subStreamIndex, desktopVideoSsrc));
        }
    }
    else {
        LOGD_SDK("replace userId:%ws stream:%s", subStreamIndex.c_str(), stream->GetID());
        std::map<std::wstring, std::shared_ptr<vhall::ReceiveVideoSsrc>>::iterator iterSsrc = mVideoSsrcMap.find(subStreamIndex);
        if (iterSsrc != mVideoSsrcMap.end()) {
           iterSsrc->second.reset();
           mVideoSsrcMap.erase(iterSsrc);
        }
        mSubScribeStreamMap.erase(iter);
        mSubScribeStreamMap.insert(make_pair(subStreamIndex, stream));
        std::shared_ptr<vhall::ReceiveVideoSsrc> desktopVideoSsrc = std::make_shared<vhall::ReceiveVideoSsrc>();
        mVideoSsrcMap.insert(make_pair(subStreamIndex, desktopVideoSsrc));
    }
    LOGD_SDK("%s end", __FUNCTION__);
}

std::string SubScribeStreamManager::GetSubScribeStreamId(const std::wstring& user, vlive::VHStreamType streamType) {
    LOGD_SDK("enter");
    CThreadLockHandle lockHandle(&mSubScribeStreamMutex);
    LOGD_SDK("mSubScribeStreamMutex");
    std::map<std::wstring, std::shared_ptr<vhall::VHStream>>::iterator iter = mSubScribeStreamMap.begin();
    while (iter != mSubScribeStreamMap.end()) {
        std::string mediaType = std::to_string(streamType);
        std::wstring subStreamIndex = user + SubScribeStreamManager::String2WString(mediaType);
        std::wstring streamIndex = iter->first;
        size_t pos = streamIndex.find(subStreamIndex);
        if (std::wstring::npos != pos) {
            return iter->second->mStreamId;
            break;
        }
        iter++;
    }
    LOGD_SDK("%s end", __FUNCTION__);
    return std::string();
}

bool SubScribeStreamManager::RemoveSubScribeStreamObj(const std::wstring &subStreamIndex, std::string stream_id) {
    LOGD_SDK("enter");
    CThreadLockHandle lockHandle(&mSubScribeStreamMutex);
    LOGD_SDK("mSubScribeStreamMutex");
    std::map<std::wstring, std::shared_ptr<vhall::VHStream>>::iterator iter = mSubScribeStreamMap.find(subStreamIndex);
    if (iter != mSubScribeStreamMap.end()) {
        LOGD_SDK("find user:%ws stream_id:%s", subStreamIndex.c_str(), stream_id.c_str());
        if (std::string(iter->second->GetID()) == stream_id) {
            mSubScribeStreamMap.erase(iter);
            std::map<std::wstring, std::shared_ptr<vhall::ReceiveVideoSsrc>>::iterator iterSsrc = mVideoSsrcMap.find(subStreamIndex);
            if (iterSsrc != mVideoSsrcMap.end()) {
               iterSsrc->second.reset();
               mVideoSsrcMap.erase(iterSsrc);
            }
            LOGD_SDK("find and delete stream");
            return true;
        }
    }
    else {
        LOGD_SDK("not find user:%ws stream_id:%s", subStreamIndex.c_str(), stream_id.c_str());
    }
    return false;
}

void SubScribeStreamManager::ClearSubScribeStream() {
    LOGD_SDK("mSubScribeStreamMutex");
    CThreadLockHandle lockHandle(&mSubScribeStreamMutex);
    LOGD_SDK("mSubScribeStreamMap.clear");
    mSubScribeStreamMap.clear();
}

void SubScribeStreamManager::StopSubScribeStream() {
    LOGD_SDK("enter");
    CThreadLockHandle lockHandle(&mSubScribeStreamMutex);
    LOGD_SDK("mSubScribeStreamMutex");
    std::map<std::wstring, std::shared_ptr<vhall::VHStream>>::iterator iter = mSubScribeStreamMap.begin();
    while (iter != mSubScribeStreamMap.end()) {
        if (iter->second) {
            std::string user = iter->second->GetUserId();
            std::string stream = iter->second->GetID();
            LOGD_SDK("UnSubscribe user:%s stream:%s", user.c_str(), stream.c_str());
            mLiveRoom->UnSubscribe(iter->second, [&](const vhall::RespMsg& resp) {
                LOGD_SDK("resp mResult:%s", resp.mResult.c_str());
            }, 3000);
            std::map<std::wstring, std::shared_ptr<vhall::ReceiveVideoSsrc>>::iterator iterSsrc = mVideoSsrcMap.find(iter->first);
            if (iterSsrc != mVideoSsrcMap.end()) {
               iterSsrc->second.reset();
               mVideoSsrcMap.erase(iterSsrc);
            }
        }
        iter = mSubScribeStreamMap.erase(iter);
    }
    LOGD_SDK("%s end", __FUNCTION__);
}

int SubScribeStreamManager::GetSubScribeStreamFormat(const std::wstring& user, vlive::VHStreamType streamType, bool &hasVideo, bool &hasAudio) {
    LOGD_SDK("enter");
    CThreadLockHandle lockHandle(&mSubScribeStreamMutex);
    LOGD_SDK("mSubScribeStreamMutex");
    std::map<std::wstring, std::shared_ptr<vhall::VHStream>>::iterator iter = mSubScribeStreamMap.begin();
    while (iter != mSubScribeStreamMap.end()) {
        std::string mediaType = std::to_string(streamType);
        std::wstring subStreamIndex = user + SubScribeStreamManager::String2WString(mediaType);
        std::wstring streamIndex = iter->first;
        size_t pos = streamIndex.find(subStreamIndex);
        if (std::wstring::npos != pos) {
            hasVideo = iter->second->HasAudio();
            hasAudio = iter->second->HasVideo();
            LOGD_SDK("%s end", __FUNCTION__);
            return 0;
        }
        iter++;
    }
    LOGD_SDK("%s end", __FUNCTION__);
    return -1;
}

bool SubScribeStreamManager::IsSubScribeDesktopStream() {
    LOGD_SDK("enter");
    CThreadLockHandle lockHandle(&mSubScribeStreamMutex);
    LOGD_SDK("%s start", __FUNCTION__);
    std::map<std::wstring, std::shared_ptr<vhall::VHStream>>::iterator iter = mSubScribeStreamMap.begin();
    while (iter != mSubScribeStreamMap.end()) {
        if (iter->second) {
            if (iter->second->mStreamType == vlive::VHStreamType_Desktop) {
                LOGD_SDK("find stream:%s user:%s", iter->second->GetID(), iter->second->GetUserId());
                return true;
            }
        }
        iter++;
    }
    LOGD_SDK("%s end", __FUNCTION__);
    return false;
}

bool SubScribeStreamManager::IsSubScribeSoftwareStream()
{
    LOGD_SDK("enter");
    CThreadLockHandle lockHandle(&mSubScribeStreamMutex);
    LOGD_SDK("%s start", __FUNCTION__);
    std::map<std::wstring, std::shared_ptr<vhall::VHStream>>::iterator iter = mSubScribeStreamMap.begin();
    while (iter != mSubScribeStreamMap.end()) {
        if (iter->second) {
            if (iter->second->mStreamType == vlive::VHStreamType_SoftWare) {
                LOGD_SDK("find stream:%s user:%s", iter->second->GetID(), iter->second->GetUserId());
                return true;
            }
        }
        iter++;
    }
    LOGD_SDK("%s end", __FUNCTION__);
    return false;
}

bool SubScribeStreamManager::IsSubScribeMediaFileStream() {
    LOGD_SDK("enter");
    CThreadLockHandle lockHandle(&mSubScribeStreamMutex);
    LOGD_SDK("%s start", __FUNCTION__);
    std::map<std::wstring, std::shared_ptr<vhall::VHStream>>::iterator iter = mSubScribeStreamMap.begin();
    while (iter != mSubScribeStreamMap.end()) {
        if (iter->second) {
            if (iter->second->mStreamType == vlive::VHStreamType_MediaFile) {
                LOGD_SDK("find stream:%s user:%s", iter->second->GetID(), iter->second->GetUserId());
                return true;
            }
        }
        iter++;
    }
    LOGD_SDK("%s end", __FUNCTION__);
    return false;
}

bool SubScribeStreamManager::IsSubScribedStreamByUserId(const std::wstring& subStreamIndex) {
    LOGD_SDK("enter");
    CThreadLockHandle lockHandle(&mSubScribeStreamMutex);
    LOGD_SDK("user_id:%ws", subStreamIndex.c_str());
    std::map<std::wstring, std::shared_ptr<vhall::VHStream>>::iterator iter = mSubScribeStreamMap.find(subStreamIndex);
    if (iter != mSubScribeStreamMap.end()) {
        LOGD_SDK("find remote user");
        return true;
    }
    LOGD_SDK("can not find remote user");
    return false;
}

bool SubScribeStreamManager::IsHasMediaType(const std::wstring& subStreamIndex, vlive::CaptureStreamAVType type) {
    LOGD_SDK("mSubScribeStreamMutex");
    CThreadLockHandle lockHandle(&mSubScribeStreamMutex);
    std::map<std::wstring, std::shared_ptr<vhall::VHStream>>::iterator iter = mSubScribeStreamMap.find(subStreamIndex);
    if (iter != mSubScribeStreamMap.end()) {
        if (type == vlive::CaptureStreamAVType_Audio) {
            LOGD_SDK("iter->second->HasAudio():%d", iter->second->HasAudio());
            return iter->second->HasAudio();
        }
        else {
            LOGD_SDK("iter->second->HasVideo():%d", iter->second->HasVideo());
            return iter->second->HasVideo();
        }
    }
    LOGD_SDK("can not find remote user");
    return false;
}



bool SubScribeStreamManager::StartRenderStream(const std::wstring& subStreamIndex, std::shared_ptr<vhall::VideoRenderReceiveInterface> recv) {
   LOGD_SDK("enter");
   CThreadLockHandle lockHandle(&mSubScribeStreamMutex);
   LOGD_SDK("StartRenderStream subStreamIndex:%s", subStreamIndex.c_str());
   std::map<std::wstring, std::shared_ptr<vhall::VHStream>>::iterator iter = mSubScribeStreamMap.find(subStreamIndex);
   if (iter != mSubScribeStreamMap.end()) {
      if (iter->second != nullptr) {
         std::string user = iter->second->GetUserId();
         std::string stream = iter->second->GetID();
         LOGD_SDK("StartRenderStream user:%s stream:%s", iter->second->GetUserId(), stream.c_str());
         iter->second->stop();
         iter->second->play(recv);
         LOGD_SDK("play remote end");
         return true;
      }
      else
      {
         LOGD_SDK("mSubScribeStreamMap.size: %d subStreamIndex %ws ", mSubScribeStreamMap.size(), subStreamIndex);
      }
   }
   LOGD_SDK("can not find user,can not play");
   return false;
}

bool SubScribeStreamManager::StartRenderStream(const std::wstring& subStreamIndex, void* wnd) {
    LOGD_SDK("enter");
    CThreadLockHandle lockHandle(&mSubScribeStreamMutex);
    LOGD_SDK("StartRenderStream subStreamIndex:%s", subStreamIndex.c_str());
    std::map<std::wstring, std::shared_ptr<vhall::VHStream>>::iterator iter = mSubScribeStreamMap.find(subStreamIndex);
    if (iter != mSubScribeStreamMap.end()) {
        if (iter->second != nullptr) {
            std::string user = iter->second->GetUserId();
            std::string stream = iter->second->GetID();
            LOGD_SDK("StartRenderStream user:%s stream:%s", iter->second->GetUserId(), stream.c_str());
            iter->second->stop();
            HWND playWnd = (HWND)(wnd);
            iter->second->play(playWnd);
            LOGD_SDK("play remote end");
            return true;
        }
		else
		{
			LOGD_SDK("mSubScribeStreamMap.size: %d subStreamIndex %ws ", mSubScribeStreamMap.size(), subStreamIndex);
		}
    }
    LOGD_SDK("can not find user,can not play");
    return false;
}

bool SubScribeStreamManager::StopRenderRemoteStream(const std::wstring& subStreamIndex) {
    LOGD_SDK("enter");
    CThreadLockHandle lockHandle(&mSubScribeStreamMutex);
    LOGD_SDK("StopRenderRemoteStream subStreamIndex:%s", subStreamIndex.c_str());
    std::map<std::wstring, std::shared_ptr<vhall::VHStream>>::iterator iter = mSubScribeStreamMap.find(subStreamIndex);
    if (iter != mSubScribeStreamMap.end()) {
        if (iter->second != nullptr) {
            std::string user = iter->second->GetUserId();
            std::string stream = iter->second->GetID();
            LOGD_SDK("StopRenderRemoteStream user:%s stream:%s", iter->second->GetUserId(), stream.c_str());
            iter->second->stop();
            LOGD_SDK("play remote end");
            return true;
        }
    }
    LOGD_SDK("can not find user,can not play");
    return false;
}


int SubScribeStreamManager::OperateRemoteUserVideo(std::wstring user_id, bool close/* = false*/) {
    LOGD_SDK("user_id:%ws", user_id.c_str());
    CThreadLockHandle lockHandle(&mSubScribeStreamMutex);
    LOGD_SDK("mSubScribeStreamMutex");
    std::map<std::wstring, std::shared_ptr<vhall::VHStream>>::iterator iter = mSubScribeStreamMap.find(user_id);
    if (iter != mSubScribeStreamMap.end()) {
        if (close) {
            iter->second->MuteVideo(true);
        }
        else {
            iter->second->MuteVideo(false);
        }
        LOGD_SDK("VhallLive_OK");
        return vlive::VhallLive_OK;
    }
    LOGD_SDK("VhallLive_NO_OBJ");
    return vlive::VhallLive_NO_OBJ;
}

int SubScribeStreamManager::OperateRemoteUserAudio(std::wstring user_id, bool close) {
    LOGD_SDK("user_id:%s", user_id.c_str());
    CThreadLockHandle lockHandle(&mSubScribeStreamMutex);
    LOGD_SDK("mSubScribeStreamMutex");
    std::map<std::wstring, std::shared_ptr<vhall::VHStream>>::iterator iter = mSubScribeStreamMap.find(user_id);
    if (iter != mSubScribeStreamMap.end()) {
        if (close) {
            iter->second->MuteAudio(true);
        }
        else {
            iter->second->MuteAudio(false);
        }
        LOGD_SDK("VhallLive_OK");
        return vlive::VhallLive_OK;
    }
    LOGD_SDK("VhallLive_NO_OBJ");
    return vlive::VhallLive_NO_OBJ;
}

void SubScribeStreamManager::MuteAllSubScribeAudio(bool mute) {
    LOGD_SDK("enter :%d", mute);
    CThreadLockHandle lockHandle(&mSubScribeStreamMutex);
    std::map<std::wstring, std::shared_ptr<vhall::VHStream>>::iterator iter = mSubScribeStreamMap.begin();
    while (iter != mSubScribeStreamMap.end()) {
        if (iter->second != nullptr) {
            iter->second->MuteAudio(mute);
        }
        iter++;
    }
    LOGD_SDK("end");
}

bool SubScribeStreamManager::ChangeSubScribeUserSimulCast(const std::wstring& user_id,const std::wstring& subStreamIndex, vlive::VHSimulCastType type) {
    LOGD_SDK("enter");
    CThreadLockHandle lockHandle(&mSubScribeStreamMutex);
    LOGD_SDK("%s start", __FUNCTION__);
    std::map<std::wstring, std::shared_ptr<vhall::VHStream>>::iterator iter = mSubScribeStreamMap.find(subStreamIndex);
    if (iter != mSubScribeStreamMap.end()) {
        if (iter->second != nullptr) {
            int dual = (int)type;
            VRtcEngineEventDelegate* obj = mWebRtcSDKEventReciver;
            mLiveRoom->SetSimulCast(iter->second, dual, [&, user_id, dual, subStreamIndex, obj](const vhall::RespMsg& resp) -> void {
                LOGD_SDK("SetSimulCast %d msg %s dual:%d", resp.mCode, resp.mMsg.c_str(), dual);
                if (obj) {
                   obj->OnChangeSubScribeUserSimulCast(user_id, (vlive::VHSimulCastType)dual, resp.mCode, resp.mMsg);
                }
            });
            return true;
        }
    }
    LOGD_SDK("%s end", __FUNCTION__);
    return false;
}


std::string SubScribeStreamManager::WString2String(const std::wstring& ws)
{
    std::string strLocale = setlocale(LC_ALL, "");
    const wchar_t* wchSrc = ws.c_str();
    size_t nDestSize = wcstombs(NULL, wchSrc, 0) + 1;
    char *chDest = new char[nDestSize];
    memset(chDest, 0, nDestSize);
    wcstombs(chDest, wchSrc, nDestSize);
    std::string strResult = chDest;
    delete[]chDest;
    setlocale(LC_ALL, strLocale.c_str());
    return strResult;
}
// string => wstring
std::wstring SubScribeStreamManager::String2WString(const std::string& str)
{
    int num = MultiByteToWideChar(CP_UTF8, 0, str.c_str(), -1, NULL, 0);
    wchar_t *wide = new wchar_t[num];
    MultiByteToWideChar(CP_UTF8, 0, str.c_str(), -1, wide, num);
    std::wstring w_str(wide);
    delete[] wide;
    return w_str;
}

int SubScribeStreamManager::GetAudioLevel(std::wstring user_id) {
   LOGD_SDK("enter");
   CThreadLockHandle lockHandle(&mSubScribeStreamMutex);
   LOGD_SDK("%s start", __FUNCTION__);
   std::map<std::wstring, std::shared_ptr<vhall::VHStream>>::iterator iter = mSubScribeStreamMap.find(user_id);
   if (iter != mSubScribeStreamMap.end()) {
      if (iter->second != nullptr) {
         vhall::ReceiveAudioSsrc pSsrc;
         iter->second->GetAudioSubscribeStates(&pSsrc);
         return pSsrc.AudioOutputLevel;
      }
   }
   LOGD_SDK("%s end", __FUNCTION__);
   return -1;
}

double SubScribeStreamManager::GetVideoLostRate(std::wstring user_id) {
   LOGD_SDK("enter");
   CThreadLockHandle lockHandle(&mSubScribeStreamMutex);
   LOGD_SDK("%s start", __FUNCTION__);
   std::map<std::wstring, std::shared_ptr<vhall::VHStream>>::iterator iter = mSubScribeStreamMap.find(user_id);
   if (iter != mSubScribeStreamMap.end()) {
      if (iter->second != nullptr) {
         std::map<std::wstring, std::shared_ptr<vhall::ReceiveVideoSsrc>>::iterator iterSsrc = mVideoSsrcMap.find(user_id);
         if (iterSsrc != mVideoSsrcMap.end()) {
            iter->second->GetVideoSubscribeStates(iterSsrc->second.get());
            iterSsrc->second->calc();
            if (iterSsrc->second->CalcData.Bitrate == 0) {
               return 100.0;
            }
            else{
               return iterSsrc->second->CalcData.PacketsLostRate;
            }
         }
      }
   }
   LOGD_SDK("%s end", __FUNCTION__);
   return -1;
}