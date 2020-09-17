#include "WebRtcSDK.h"
#include "./WebRTC/vhall_sdk_log.h"
#include "signalling\BaseStack\AdmConfig.h"
#include "pictureDecoder.h"
#include "signalling\BaseStack\statsInfo.h"
#include "lib/rapidjson/include/rapidjson/rapidjson.h"
#include "lib/rapidjson/include/rapidjson/stringbuffer.h"
#include "lib/rapidjson/include/rapidjson/writer.h"
#include "lib/rapidjson/include/rapidjson/reader.h"
#include "lib/rapidjson/include/rapidjson/document.h"

HANDLE WebRtcSDK::gTaskEvent = nullptr;
HANDLE WebRtcSDK::gThreadExitEvent = nullptr;
std::atomic_bool WebRtcSDK::bThreadRun = true;
CThreadLock WebRtcSDK::taskListMutex;
std::list<CTaskEvent> WebRtcSDK::mtaskEventList;
SubScribeStreamManager WebRtcSDK::mSubScribeStreamManager;

using namespace webrtc_sdk;

WebRtcSDK::WebRtcSDK(){
   vhall::AdmConfig::InitializeAdm();
   mSettingCurrentLayoutMode = false;
   mEnableSimulCast = false;
   SetEnableConfigMixStream(false);
   mCameraStreamConfig.mAudio = true;
   mCameraStreamConfig.mData = true;
   mCameraStreamConfig.mVideo = true;
   mCameraStreamConfig.mStreamType = VHStreamType_AVCapture;
   mCameraStreamConfig.videoOpt.maxWidth = 640;
   mCameraStreamConfig.videoOpt.minWidth = 640;
   mCameraStreamConfig.videoOpt.maxHeight = 480;
   mCameraStreamConfig.videoOpt.minHeight = 480;
   mCameraStreamConfig.videoOpt.maxVideoBW = 500;
   mCameraStreamConfig.videoOpt.maxFrameRate = 15;
   mCameraStreamConfig.videoOpt.minFrameRate = 15;
   mCameraStreamConfig.reconnect = true;
   mCameraStreamConfig.videoOpt.lockRatio = true;

   bThreadRun = true;
   mProcessThread = new std::thread(WebRtcSDK::ThreadProcess, this);
   if (mProcessThread) {
      gTaskEvent = ::CreateEvent(NULL, FALSE, FALSE, NULL);
      gThreadExitEvent = ::CreateEvent(NULL, FALSE, FALSE, NULL);
   }
   mMediaCoreProcessThread = new std::thread(WebRtcSDK::MediaCoreThreadProcess, this);
}

WebRtcSDK::~WebRtcSDK(){
   LOGD_SDK("WebRtcSDK::~WebRtcSDK");
   ExitThread();
   DisConnetWebRtcRoom();
   mtaskEventList.clear();
   vhall::AdmConfig::DeInitAdm();
   LOGD_SDK("WebRtcSDK::~WebRtcSDK end");
}

void WebRtcSDK::ExitThread() {
   LOGD_SDK("WebRtcSDK::ExitThread");
   bThreadRun = false;
   if (gTaskEvent) {
      ::SetEvent(gTaskEvent);
   }
   if (mProcessThread) {
      WaitForSingleObject(gThreadExitEvent, 5000);
      ::CloseHandle(gThreadExitEvent);
      gThreadExitEvent = nullptr;
      LOGD_SDK("join");
      mProcessThread->join();
      LOGD_SDK("join end");
      delete mProcessThread;
      mProcessThread = nullptr;
   }
   if (gTaskEvent) {
      ::CloseHandle(gTaskEvent);
      gTaskEvent = nullptr;
   }
   if (mMediaCoreProcessThread) {
      mMediaCoreProcessThread->join();
      delete mMediaCoreProcessThread;
      mMediaCoreProcessThread = nullptr;
   }
   LOGD_SDK("WebRtcSDK::end");
}

void WebRtcSDK::InitSDK(VRtcEngineEventDelegate* obj, std::wstring logPath/* = std::wstring()*/) {
   if (!mbIsInit) {
      mbIsInit = true;
      mWebRtcSDKEventReciver = obj;
      SDKInitLog(logPath);
   }
   LOGD_SDK("VhallWebRTC_VERSION : %s", VhallWebRTC_VERSION);
}

void WebRtcSDK::SendAudioData(const void * audio_data, int bits_per_sample, int sample_rate, size_t number_of_channels, size_t number_frames_perChannel) {

}

void WebRtcSDK::SetDebugLogAddr(const std::string& upAddr) {
   vhall::VHStream::OpenUpStats(upAddr.c_str());
}

int WebRtcSDK::ConnetWebRtcRoom(const vlive::WebRtcRoomOption& option) {
   LOGD_SDK("");
   int bRet = vlive::VhallLive_OK;
   mWebRtcRoomOption = option;
   //创建房间管理
   vhall::RoomOptions specInput;
   //参数说明地址http://wiki.vhallops.com/pages/viewpage.action?pageId=918066
   specInput.upLogUrl = SubScribeStreamManager::WString2String(mWebRtcRoomOption.strLogUpLogUrl);// QString("http:%1").arg(mWebRtcRoomOption.strLogUpLogUrl).toStdString().c_str();
   specInput.biz_role = option.role;
   specInput.biz_id = SubScribeStreamManager::WString2String(option.strRoomId);
   specInput.biz_des01 = option.classType;
   specInput.biz_des02 = 1;
   specInput.uid = SubScribeStreamManager::WString2String(mWebRtcRoomOption.strUserId);
   specInput.bu = SubScribeStreamManager::WString2String(option.strBusinesstype);
   specInput.vid = SubScribeStreamManager::WString2String(option.strAccountId);
   specInput.vfid = SubScribeStreamManager::WString2String(option.strAccountId);
   specInput.app_id = SubScribeStreamManager::WString2String(option.strappid);
   specInput.attempts = mWebRtcRoomOption.nWebRtcSdkReConnetTime; //SDK 3秒重连一次。);
   specInput.token = SubScribeStreamManager::WString2String(mWebRtcRoomOption.strRoomToken);
   specInput.enableRoomReconnect = false;
   LOGD_SDK("roomlockHandle roomlockHandle");
   CThreadLockHandle lockHandle(&mRoomThreadLock);
   if (mLiveRoom) {
      mLiveRoom->Disconnect();
      mLiveRoom.reset();
      mLiveRoom = nullptr;
      LOGD_SDK("mLiveRoom reset");
   }
   LOGD_SDK("make_shared mLiveRoom");
   mLiveRoom = std::make_shared<vhall::VHRoom>(specInput);
   if (mLiveRoom) {
      mSubScribeStreamManager.RegisterRoomObj(mLiveRoom, mWebRtcSDKEventReciver);
   }
   LOGD_SDK("token:%s strThirdPushStreamUrl:%s local_user_id:%s logUrl:%s room_id:%s",
      specInput.token.c_str(), mWebRtcRoomOption.strThirdPushStreamUrl.c_str(), SubScribeStreamManager::WString2String(mWebRtcRoomOption.strUserId).c_str(), mWebRtcRoomOption.strLogUpLogUrl.c_str(), mWebRtcRoomOption.strRoomId.c_str());
   LOGD_SDK("app_id:%s specInput.bu:%s vfid:%s", specInput.app_id.c_str(), specInput.bu.c_str(), specInput.vid.c_str());

   std::wstring curJoinUid = mWebRtcRoomOption.strUserId;
   if (mLiveRoom) {
      mLiveRoom->mRoomId = SubScribeStreamManager::WString2String(option.strRoomId);
      RegisterRoomConnected();
      RegisterRoomError();
      RegisterDeviceEvent();
      RegisterStreamEvent();
      LOGD_SDK("Connect room");
      mLiveRoom->Connect();
   }
   else {
      bRet = vlive::VhallLive_NO_OBJ;
   }
   return bRet;
}

int WebRtcSDK::DisConnetWebRtcRoom() {
   LOGD_SDK("DisConnetWebRtcRoom");
   {
      CThreadLockHandle lockHandle(&mRoomThreadLock);
      if (mLiveRoom) {
         mLiveRoom->RemoveAllEventListener();
         LOGD_SDK("RemoveAllEventListener end");
      }
   }
   int bRet = vlive::VhallLive_NO_OBJ;
   LOGD_SDK("get mLiveRoom");
   mSubScribeStreamManager.ClearSubScribeStream();
   ReleaseAllLocalStream();
   mEnableSimulCast = false;
   mLocalStreamIsCapture = false;
   mbIsOpenCamera = true;
   mMediaStreamIsCapture = false;
   mDesktopStreamIsCapture = false;
   mCurrentMicIndex = -1;
   mCurrentPlayerIndex = 0;
   mCurrentCameraGuid.clear();
   mCurrentPlayerDevid.clear();
   mCurrentMicDevid.clear();
   mCurMicVol = 100;
   mCurPlayVol = 100;
   mbIsOpenMic = true;
   mbIsOpenPlayer = true;
   SetEnableConfigMixStream(false);
   mLocalStreamId.clear();
   mDesktopStreamId.clear();
   mMediaFileStreamId.clear();
   mCurPlayFileName.clear();
   mCurPlayFile.clear();
   mbIsWebRtcRoomConnected = false;
   mbIsEnablePlayFileAndShare = true; //判断当前用户是否能够进行插播和桌面共享，插播和桌面共享只能处理一种，属于互斥的。
   mbIsMixedAlready = false;
   LOGD_SDK("to get mRoomThreadLock");
   CThreadLockHandle lockHandle(&mRoomThreadLock);
   if (mLiveRoom) {
      LOGD_SDK("to Disconnect");
      mLiveRoom->Disconnect();
      LOGD_SDK("Disconnect end");
      mLiveRoom.reset();
      mLiveRoom = nullptr;
   }
   bRet = vlive::VhallLive_OK;
   LOGD_SDK("DisConnetWebRtcRoom end");
   return bRet;
}

void WebRtcSDK::RegisterDeviceEvent() {
   mAudioDeviceDispatcher = vhall::AdmConfig::GetAudioDeviceEventDispatcher();
   if (mAudioDeviceDispatcher) {
      //监听音频采集异常
      mAudioDeviceDispatcher->AddEventListener(AUDIO_CAPTURE_ERROR, [&](vhall::BaseEvent &Event)->void {
         LOGD_SDK("AUDIO_CAPTURE_ERROR");
         CTaskEvent task;
         task.mEventType = TaskEventEnum_CaptureLocalStream;
         task.mErrMsg = AUDIO_CAPTURE_ERROR;
         PostTaskEvent(task);
      });
      //DSHOW监听音频设备拔出
      mAudioDeviceDispatcher->AddEventListener(AUDIO_DEVICE_REMOVED, [&](vhall::BaseEvent &Event)->void {
         LOGD_SDK("AUDIO_DEVICE_REMOVED");
         CTaskEvent task;
         task.mEventType = TaskEventEnum_CaptureLocalStream;
         task.mErrMsg = AUDIO_DEVICE_REMOVED;
         PostTaskEvent(task);
      });
      //监听音频采集状态，不活跃则表示采集失败
      mAudioDeviceDispatcher->AddEventListener(AUDIO_DEVICE_STATE, [&](vhall::BaseEvent &Event)->void {
         vhall::AudioDeviceEvent* event = dynamic_cast<vhall::AudioDeviceEvent*>(&Event);
         if (nullptr == event) {
            return;
         }
         if (event->state != vhall::AudioDeviceEvent::AudioDeviceEvent::ACTIVE) {
            LOGD_SDK("AUDIO_DEVICE_STATE");
            CTaskEvent task;
            task.mEventType = TaskEventEnum_CaptureLocalStream;
            task.mErrMsg = AUDIO_DEVICE_STATE;
            task.devId = SubScribeStreamManager::WString2String(event->deviceId);
            PostTaskEvent(task);
         }
      });
      //Core Audio 设备初始化失败
      mAudioDeviceDispatcher->AddEventListener(IAUDIOCLIENT_INITIALIZE, [&](vhall::BaseEvent &Event)->void {
         vhall::AudioDeviceEvent* event = dynamic_cast<vhall::AudioDeviceEvent*>(&Event);
         if (nullptr == event) {
            return;
         }
         LOGD_SDK("IAUDIOCLIENT_INITIALIZE");
         CTaskEvent task;
         task.mEventType = TaskEventEnum_CaptureLocalStream;
         task.mErrMsg = IAUDIOCLIENT_INITIALIZE;
         PostTaskEvent(task);
      });
      //Core Audio 设备采集失败
      mAudioDeviceDispatcher->AddEventListener(COREAUDIOCAPTUREERROR, [&](vhall::BaseEvent &Event)->void {
         vhall::AudioDeviceEvent* event = dynamic_cast<vhall::AudioDeviceEvent*>(&Event);
         if (nullptr == event) {
            return;
         }
         LOGD_SDK("COREAUDIOCAPTUREERROR");
         CTaskEvent task;
         task.mEventType = TaskEventEnum_CaptureLocalStream;
         task.mErrMsg = COREAUDIOCAPTUREERROR;
         PostTaskEvent(task);
      });
   }
   //音频设备采集失败
   mLiveRoom->AddEventListener(AUDIO_IN_ERROR, [&](vhall::BaseEvent &Event)->void {
      LOGD_SDK("ROOM_ERROR");
      vhall::RoomEvent* event = dynamic_cast<vhall::RoomEvent*>(&Event);
      std::string msg("AUDIO_IN_ERROR  error");
      if (event) {
         msg = event->mMessage;
      }
      LOGD_SDK("msg:%s", msg.c_str());
   });
   //播放设备采集失败
   mLiveRoom->AddEventListener(AUDIO_OUT_ERROR, [&](vhall::BaseEvent &Event)->void {
      LOGD_SDK("ROOM_ERROR");
      vhall::RoomEvent* event = dynamic_cast<vhall::RoomEvent*>(&Event);
      std::string msg("AUDIO_OUT_ERROR  error");
      if (event) {
         msg = event->mMessage;
      }
      LOGD_SDK("msg:%s", msg.c_str());
   });
}

void WebRtcSDK::RegisterRoomConnected() {
   //连接房间成功。有可能已经有人，获取房间成员列表并订阅。
   std::wstring curJoinUid = mWebRtcRoomOption.strUserId;
   mLiveRoom->AddEventListener(ROOM_CONNECTED, [&, curJoinUid](vhall::BaseEvent &Event)->void {
      LOGD_SDK("ROOM_CONNECTED");
      CTaskEvent roomConnectTask;
      roomConnectTask.mEventType = TaskEventEnum_ROOM_CONNECTED;
      PushFrontEvent(roomConnectTask);
      vhall::RoomEvent *rEvent = static_cast<vhall::RoomEvent *>(&Event);
      if (rEvent) {
         //房间链接成功之后，如果有流则进行订阅。但是不订阅自己的流
         for (auto &stream : rEvent->mStreams) {
            std::string joinUid = stream->GetUserId();
            std::string stream_id = stream->GetID();
            std::string context = stream->mStreamAttributes;
            if (!context.empty()) {
               int type = ParamPushType(context);
               if (type == 0) {
                  continue;
               }
            }
            if (SubScribeStreamManager::String2WString(joinUid) != curJoinUid) {
               LOGD_SDK("ROOM_CONNECTED TaskEventEnum_ADD_STREAM joinUid:%s stream_id:%s", joinUid.c_str(), stream_id.c_str());
               CTaskEvent task;
               task.mEventType = TaskEventEnum_ADD_STREAM;
               task.streamId = stream_id;
               task.mJoinUid = joinUid;
               task.mStreamType = stream->mStreamType;
               LOGD_SDK("TaskEventEnum_ADD_STREAM stream_id:%s", stream_id.c_str());
               PostTaskEvent(task);
            }
         }
      }
   });
}

void WebRtcSDK::RegisterStreamEvent() {
   //当推流到媒体服务，会回调STREAM_READY，此时可以提前进行互动混流和大屏。其中插播例外，不在此进行处理，ON_STREAM_MIXED处理插播推流成功。
   std::wstring curJoinUid = mWebRtcRoomOption.strUserId;
   mLiveRoom->AddEventListener(STREAM_READY, [&](vhall::BaseEvent &Event)->void {
      LOGD_SDK("STREAM_READY");
      vhall::StreamEvent *event = dynamic_cast<vhall::StreamEvent *>(&Event);
      if (event) {
         int mediaType = event->mStreams->mStreamType;
         LOGD_SDK("STREAM_READY %d", mediaType);
         if (mediaType == vlive::VHStreamType_MediaFile) {
            return;
         }
         CTaskEvent roomConnectTask;
         roomConnectTask.mEventType = TaskEventEnum_ON_STREAM_MIXED;
         roomConnectTask.mHasAudio = event->mStreams->HasAudio();
         roomConnectTask.mHasVideo = event->mStreams->HasVideo();
         std::string context = event->mStreams->mStreamAttributes;
         if (!context.empty()) {
            int type = ParamPushType(context);
            if (type == 1) {
               mediaType = VHStreamType_Auxiliary_CamerCapture;
            }
         }
         roomConnectTask.mStreamType = mediaType;
         roomConnectTask.streamId = std::string(event->mStreams->mStreamId);
         roomConnectTask.mJoinUid = std::string(event->mStreams->GetUserId());
         roomConnectTask.mStreamAttributes = event->mStreams->mStreamAttributes;
         PostTaskEvent(roomConnectTask);
      }
      LOGD_SDK("STREAM_READY end");
   });
   //当推流到媒体服务，会回调ON_STREAM_MIXED，此时可以进行互动混流和大屏。
   mLiveRoom->AddEventListener(ON_STREAM_MIXED, [&](vhall::BaseEvent &Event)->void {
      LOGD_SDK("ON_STREAM_MIXED");
      vhall::AddStreamEvent *event = dynamic_cast<vhall::AddStreamEvent *>(&Event);
      if (event) {
         int mediaType = event->mStream->mStreamType;
         LOGD_SDK("ON_STREAM_MIXED %d", mediaType);
         if (mediaType == vlive::VHStreamType_MediaFile) {
            CTaskEvent roomConnectTask;
            roomConnectTask.mEventType = TaskEventEnum_ON_STREAM_MIXED;
            roomConnectTask.mHasAudio = event->mStream->HasAudio();
            roomConnectTask.mHasVideo = event->mStream->HasVideo();
            std::string context = event->mStream->mStreamAttributes;
            if (!context.empty()) {
               int type = ParamPushType(context);
               if (type == 1) {
                  mediaType = VHStreamType_Auxiliary_CamerCapture;
               }
            }
            roomConnectTask.mStreamType = mediaType;
            roomConnectTask.streamId = std::string(event->mStream->mStreamId);
            roomConnectTask.mJoinUid = std::string(event->mStream->GetUserId());
            roomConnectTask.mStreamAttributes = event->mStream->mStreamAttributes;
            PostTaskEvent(roomConnectTask);
         }
      }
      LOGD_SDK("ON_STREAM_MIXED end");
   });
   //远端流订阅失败,重新订阅。
   mLiveRoom->AddEventListener(STREAM_RECONNECT, [&, curJoinUid](vhall::BaseEvent &Event)->void {
      LOGD_SDK("STREAM_RECONNECT");
      vhall::ReconnectStreamEvent *newStream = static_cast<vhall::ReconnectStreamEvent *>(&Event);
      std::string joinUid = newStream->NewStream->GetUserId();
      if (newStream) {
         LOGD_SDK("STREAM_RECONNECT joinUid:%s stream_id:%s ", joinUid.c_str(), newStream->NewStream->GetID());
         if (SubScribeStreamManager::String2WString(joinUid) != curJoinUid) {
            std::string stream_id = newStream->NewStream->mId;
            CTaskEvent task;
            task.mEventType = TaskEventEnum_RE_SUBSCRIB;
            task.streamId = stream_id;
            task.mStreamType = newStream->NewStream->mStreamType;
            task.mSleepTime = 3000;  //延迟三秒
            task.mJoinUid = joinUid;
            task.mErrorType = 1;
            LOGD_SDK("TaskEventEnum_RE_SUBSCRIB joinUid:%s stream_id:%s", joinUid.c_str(), stream_id.c_str());
            PostTaskEvent(task);
         }
      }
      LOGD_SDK("STREAM_RECONNECT end");
   });
   //底层重推流，流id会发生变化。
   mLiveRoom->AddEventListener(STREAM_REPUBLISHED, [&](vhall::BaseEvent &Event)->void {
      LOGD_SDK("STREAM_REPUBLISHED");
      vhall::StreamEvent *stream = static_cast<vhall::StreamEvent *>(&Event);
      if (stream && stream->mStreams) {
         std::string new_stream_id = stream->mStreams->GetID();
         std::string user_id = stream->mStreams->GetUserId();
         CTaskEvent roomConnectTask;
         roomConnectTask.mEventType = TaskEventEnum_STREAM_REPUBLISHED;
         roomConnectTask.mStreamType = stream->mStreams->mStreamType;
         roomConnectTask.streamId = new_stream_id;
         roomConnectTask.mJoinUid = stream->mStreams->GetUserId();
         PostTaskEvent(roomConnectTask);
      }
   });
   // 本地推流失败监听
   mLiveRoom->AddEventListener(STREAM_FAILED, [&](vhall::BaseEvent &Event)->void {
      LOGD_SDK("STREAM_FAILED");
      std::string join_uid;
      std::string stream_id;
      int nStreamType = -1;
      vhall::StreamEvent *streamEvent = static_cast<vhall::StreamEvent *>(&Event);
      if (streamEvent && streamEvent->mStreams) {
         join_uid = streamEvent->mStreams->GetUserId();
         stream_id = streamEvent->mStreams->GetID();
         nStreamType = streamEvent->mStreams->mStreamType;
         LOGD_SDK("STREAM_FAILED join_uid:%s stream_id:%s nStreamType:%d", join_uid.c_str(), stream_id.c_str(), nStreamType);
         CTaskEvent roomConnectTask;
         roomConnectTask.mEventType = TaskEventEnum_STREAM_FAILED;
         roomConnectTask.mStreamType = streamEvent->mStreams->mStreamType;
         roomConnectTask.streamId = stream_id;
         roomConnectTask.mJoinUid = streamEvent->mStreams->GetUserId();
         PostTaskEvent(roomConnectTask);
         LOGD_SDK("STREAM_FAILED end");
      }
   });
   // 远端流退出监听回调
   mLiveRoom->AddEventListener(STREAM_REMOVED, [&, curJoinUid](vhall::BaseEvent &Event)->void {
      LOGD_SDK("STREAM_REMOVED");
      vhall::StreamEvent *AddStream = static_cast<vhall::StreamEvent *>(&Event);
      if (AddStream && AddStream->mStreams) {
         std::string join_uid = AddStream->mStreams->GetUserId();
         std::string stream_id = AddStream->mStreams->mId;
         std::wstring join_uid_temp = SubScribeStreamManager::String2WString(join_uid);
         LOGD_SDK("STREAM_REMOVED join_uid:%s  stream_id:%s streamType:%d", join_uid.c_str(), stream_id.c_str(), AddStream->mStreams->mStreamType);
         std::string mediaType = std::to_string(AddStream->mStreams->mStreamType);
         std::wstring subStreamIndex = join_uid_temp + SubScribeStreamManager::String2WString(mediaType);
         if (curJoinUid == join_uid_temp) {
            LOGD_SDK("STREAM_REMOVED local user");
            return;
         }
         CTaskEvent roomConnectTask;
         roomConnectTask.mEventType = TaskEventEnum_STREAM_REMOVED;
         roomConnectTask.mStreamType = AddStream->mStreams->mStreamType;
         roomConnectTask.streamId = stream_id;
         roomConnectTask.mJoinUid = join_uid;
         PostTaskEvent(roomConnectTask);
         LOGD_SDK("STREAM_REMOVED end");
      }
   });
   ////过程中有人加入，需要去除自己。
   mLiveRoom->AddEventListener(STREAM_ADDED, [&, curJoinUid](vhall::BaseEvent &Event)->void {
      LOGD_SDK("STREAM_ADDED");
      std::string join_uid;
      std::string stream_id;
      vhall::AddStreamEvent *AddStream = static_cast<vhall::AddStreamEvent *>(&Event);
      if (AddStream &&  AddStream->mStream) {
         AddStream->mStream->reconnect = true;
         join_uid = AddStream->mStream->GetUserId();
         stream_id = AddStream->mStream->mId;
         std::string context = AddStream->mStream->mStreamAttributes;
         if (!context.empty()) {
            int type = ParamPushType(context);
            if (type == 0) {
               return;
            }
         }
         std::wstring user_id = SubScribeStreamManager::String2WString(join_uid);
         LOGD_SDK("STREAM_ADDED userId:%s stream:%s ", join_uid.c_str(), stream_id.c_str());
         if (user_id != curJoinUid) {
            //不是自己的流订阅
            CTaskEvent task;
            task.mEventType = TaskEventEnum_ADD_STREAM;
            task.streamId = stream_id;
            task.mStreamType = AddStream->mStream->mStreamType;
            task.mJoinUid = join_uid;
            LOGD_SDK("TaskEventEnum_ADD_STREAM stream_id:%s", stream_id.c_str());
            PostTaskEvent(task);
         }
         LOGD_SDK("STREAM_ADDED end");
      }
   });
   ////订阅流成功，会触发。成功之后才进行流的渲染处理。
   mLiveRoom->AddEventListener(STREAM_SUBSCRIBED, [&, curJoinUid](vhall::BaseEvent &Event)->void {
      LOGD_SDK("STREAM_SUBSCRIBED");
      vhall::StreamEvent *AddStream = static_cast<vhall::StreamEvent *>(&Event);
      std::string stream_id;
      std::string joinUid;
      if (AddStream && AddStream->mStreams && AddStream->mStreams->GetUserId()) {
         //不再订阅自己的远端流
         AddStream->mStreams->reconnect = true;
         joinUid = AddStream->mStreams->GetUserId();
         stream_id = AddStream->mStreams->mId;
         bool bHasVideo = AddStream->mStreams->HasVideo();
         bool bHasAudio = AddStream->mStreams->HasAudio();
         LOGD_SDK("STREAM_SUBSCRIBED user:%s stream:%s type:%d bHasVideo:%d bHasAudio:%d", joinUid.c_str(), stream_id.c_str(), AddStream->mStreams->mStreamType, bHasVideo, bHasAudio);
         if (SubScribeStreamManager::String2WString(joinUid) != curJoinUid) {
            std::wstring user_id = SubScribeStreamManager::String2WString(joinUid);
            std::string mediaType = std::to_string(AddStream->mStreams->mStreamType);
            std::string context = AddStream->mStreams->mStreamAttributes;
            int curStreamType = AddStream->mStreams->mStreamType;
            if (!context.empty()) {
               int type = ParamPushType(context);
               if (type == 1) {
                  mediaType = std::to_string(VHStreamType_Auxiliary_CamerCapture);
                  curStreamType = VHStreamType_Auxiliary_CamerCapture;
               }
               else if (type == 2) {
                  mediaType = std::to_string(AddStream->mStreams->mStreamType);
               }
            }
            std::wstring subStreamIndex = user_id + SubScribeStreamManager::String2WString(mediaType);
            LOGD_SDK("STREAM_SUBSCRIBED subStreamIndex:ws context:%s", subStreamIndex.c_str(), context.c_str());
            AddStream->mStreams->AddEventListener(VIDEO_RECEIVE_LOST, [&, joinUid, subStreamIndex, stream_id, curStreamType](vhall::BaseEvent &Event)->void {
               LOGD_SDK("VIDEO_RECEIVE_LOST :%s", stream_id.c_str());
               CTaskEvent task;
               task.mEventType = TaskEventEnum_STREAM_RECV_LOST;
               task.mJoinUid = joinUid;
               task.mStreamIndex = subStreamIndex;
               task.streamId = stream_id;
               task.mStreamType = curStreamType;
               PostTaskEvent(task);
            });
            //保存已经订阅的流
            mSubScribeStreamManager.InsertSubScribeStreamObj(subStreamIndex, AddStream->mStreams);
            LOGD_SDK("STREAM_SUBSCRIBED userId:%s stream:%s type:%d", joinUid.c_str(), stream_id.c_str(), AddStream->mStreams->mStreamType);
            CTaskEvent roomConnectTask;
            roomConnectTask.mEventType = TaskEventEnum_STREAM_SUBSCRIBED_SUCCESS;
            roomConnectTask.mStreamType = curStreamType;// AddStream->mStreams->mStreamType;
            roomConnectTask.streamId = stream_id;
            roomConnectTask.mJoinUid = joinUid;
            roomConnectTask.mHasAudio = bHasAudio;
            roomConnectTask.mHasVideo = bHasVideo;
            roomConnectTask.mStreamAttributes = AddStream->mStreams->mStreamAttributes;
            PostTaskEvent(roomConnectTask);
         }
      }
   });
}

void WebRtcSDK::RegisterRoomError() {
   //房间链接失败，需要重新链接房间
   mLiveRoom->AddEventListener(ROOM_ERROR, [&](vhall::BaseEvent &Event)->void {
      LOGD_SDK("ROOM_ERROR");
      vhall::RoomEvent* event = dynamic_cast<vhall::RoomEvent*>(&Event);
      LOGD_SDK("ROOM_ERROR ParseError code:%d error_type:%d %s\n", event->mCode, event->mErrorType, event->mMessage.c_str());
      CTaskEvent roomConnectTask;
      roomConnectTask.mEventType = TaskEventEnum_ROOM_ERROR;
      roomConnectTask.mErrorType = event->mErrorType;
      PostTaskEvent(roomConnectTask);
      LOGD_SDK("ROOM_ERROR end");
   });
   //sdk检测到网络异常正在尝试进行重连。
   mLiveRoom->AddEventListener(ROOM_RECONNECTING, [&](vhall::BaseEvent &Event)->void {
      LOGD_SDK("ROOM_RECONNECTING");
      CTaskEvent roomConnectTask;
      roomConnectTask.mEventType = TaskEventEnum_ROOM_RECONNECTING;
      PostTaskEvent(roomConnectTask);
      LOGD_SDK("ROOM_RECONNECTING end");
   });

   //ROOM_RECONNECTED  已重新进入房间，已停止推拉流； 无法自动拉流（app结合业务从最新流列表判断是否拉流）；
   //若正在推本地流，SDK自动重新推流（重新推流失败应当抛出stream_fail事件,新信令版本实现）；
   mLiveRoom->AddEventListener(ROOM_RECONNECTED, [&](vhall::BaseEvent &Event)->void {
      LOGD_SDK("ROOM_RECONNECTED");
      CTaskEvent roomConnectTask;
      roomConnectTask.mEventType = TaskEventEnum_ROOM_RECONNECTED;
      PostTaskEvent(roomConnectTask);
      LOGD_SDK("ROOM_RECONNECTED end");
   });
   //底层网络重连恢复成功
   mLiveRoom->AddEventListener(ROOM_RECOVER, [&](vhall::BaseEvent &Event)->void {
      LOGD_SDK("ROOM_RECOVER");
      CTaskEvent roomConnectTask;
      roomConnectTask.mEventType = TaskEventEnum_ROOM_RECOVER;
      PostTaskEvent(roomConnectTask);
      LOGD_SDK("ROOM_RECOVER end");
   });
   //底层网络变化
   mLiveRoom->AddEventListener(ROOM_NETWORKCHANGED, [&](vhall::BaseEvent &Event)->void {
      LOGD_SDK("ROOM_NETWORKCHANGED");
      CTaskEvent roomConnectTask;
      roomConnectTask.mEventType = TaskEventEnum_ROOM_NETWORKCHANGED;
      PostTaskEvent(roomConnectTask);
      LOGD_SDK("ROOM_NETWORKCHANGED end");
   });
}

void WebRtcSDK::ReleaseAllLocalStream() {
   mLocalStreamThreadLock.Lock();
   RelaseStream(mLocalStream);
   mLastCaptureIsCamera = false;
   mLocalStreamThreadLock.UnLock();

   mMediaStreamThreadLock.Lock();
   RelaseStream(mMediaStream);
   mMediaStreamThreadLock.UnLock();

   mDesktopStreamThreadLock.Lock();
   RelaseStream(mDesktopStream);
   mDesktopStreamThreadLock.UnLock();

   mSoftwareStreamThreadLock.Lock();
   RelaseStream(mSoftwareStream);
   mSoftwareStreamThreadLock.UnLock();

   mPreviewStreamThreadLock.Lock();
   RelaseStream(mPreviewLocalStream);
   mPreviewStreamThreadLock.UnLock();

   mLocalAuxiliaryStreamThreadLock.Lock();
   RelaseStream(mLocalAuxiliaryStream);
   mLocalAuxiliaryStreamThreadLock.UnLock();
}

void WebRtcSDK::EnableSimulCast(bool enable) {
   mEnableSimulCast = enable;
}

void WebRtcSDK::SubScribeRemoteStream(const std::string &stream_id, int delayTimeOut) {
   CTaskEvent task;
   task.mEventType = TaskEventEnum_RE_SUBSCRIB;
   task.streamId = stream_id;
   task.mSleepTime = delayTimeOut;
   LOGD_SDK("SubScribeRemoteStream stream_id:%s", stream_id.c_str());
   PostTaskEvent(task);
}

bool WebRtcSDK::ChangeSubScribeUserSimulCast(const std::wstring& user_id, vlive::VHStreamType streamType, VHSimulCastType type) {
   std::string mediaType = std::to_string(streamType);
   std::wstring subStreamIndex = user_id + SubScribeStreamManager::String2WString(mediaType);
   mSubScribeStreamManager.ChangeSubScribeUserSimulCast(user_id, subStreamIndex, type);
   return true;
}

bool WebRtcSDK::IsWebRtcRoomConnected() {
   return mbIsWebRtcRoomConnected;
}

/**
* 摄像头画面预览，当预览结束之后需要停止预览，否则摄像头将被一直占用
*/
int WebRtcSDK::StartPreviewCamera(void* wnd, const std::string& devGuid, VideoProfileIndex index, int micIndex /*= -1*/) {
   if (devGuid.length() <= 0) {
      return vlive::VhallLive_NO_DEVICE;
   }
   LOGD_SDK("devGuid :%s", devGuid.c_str());
   CTaskEvent task;
   task.mEventType = TaskEventEnum_StartPreviewLocalCapture;
   task.devId = devGuid;
   task.index = index;
   task.mMicIndex = micIndex;
   task.wnd = wnd;
   LOGD_SDK("TaskEventEnum_StartLocalCapture");
   PostTaskEvent(task);
   LOGD_SDK("TaskEventEnum_StartLocalCapture");
   mbStopPreviewCamera = false;
   return vlive::VhallLive_OK;
}

int WebRtcSDK::StartPreviewCamera(std::shared_ptr<vhall::VideoRenderReceiveInterface> recv, const std::string& devGuid, VideoProfileIndex index, int micIndex) {
   if (devGuid.length() <= 0) {
      return vlive::VhallLive_NO_DEVICE;
   }
   LOGD_SDK("devGuid :%s", devGuid.c_str());
   CTaskEvent task;
   task.mEventType = TaskEventEnum_StartPreviewLocalCapture;
   task.devId = devGuid;
   task.index = index;
   task.mMicIndex = micIndex;
   mPreviewVideoRecver = recv;
   LOGD_SDK("TaskEventEnum_StartLocalCapture");
   PostTaskEvent(task);
   LOGD_SDK("TaskEventEnum_StartLocalCapture");
   mbStopPreviewCamera = false;
   return vlive::VhallLive_OK;
}

void WebRtcSDK::RegisterLocalCaptureEvent(std::shared_ptr<vhall::VHStream>& local_stream) {
   //当调用摄像头，会触发ACCESS_ACCEPTED回掉。
   local_stream->AddEventListener(ACCESS_ACCEPTED, [&](vhall::BaseEvent &Event)->void {
      LOGD_SDK("ACCESS_ACCEPTED");
      CTaskEvent task;
      task.mEventType = TaskEventEnum_CaptureLocalStream;
      task.mErrMsg = ACCESS_ACCEPTED;
      PostTaskEvent(task);
   });
   // 获取本地摄像头失败监听回调
   local_stream->AddEventListener(ACCESS_DENIED, [&](vhall::BaseEvent &Event)->void {
      LOGD_SDK("ACCESS_DENIED");
      CTaskEvent task;
      task.mEventType = TaskEventEnum_CaptureLocalStream;
      task.mErrMsg = ACCESS_DENIED;
      PostTaskEvent(task);
   });
   local_stream->AddEventListener(CAMERA_LOST, [&](vhall::BaseEvent &Event)->void {
      LOGD_SDK("CAMERA_LOST");
      CTaskEvent task;
      task.mEventType = TaskEventEnum_CaptureLocalStream;
      task.mErrMsg = CAMERA_LOST;
      PostTaskEvent(task);
   });
   local_stream->AddEventListener(VIDEO_CAPTURE_ERROR, [&](vhall::BaseEvent &Event)->void {
      LOGD_SDK("VIDEO_CAPTURE_ERROR");
      CTaskEvent task;
      task.mEventType = TaskEventEnum_CaptureLocalStream;
      task.mErrMsg = VIDEO_CAPTURE_ERROR;
      PostTaskEvent(task);
   });
   local_stream->AddEventListener(STREAM_SOURCE_LOST, [&](vhall::BaseEvent &Event)->void {
      LOGD_SDK("STREAM_SOURCE_LOST");
      CTaskEvent task;
      task.mEventType = TaskEventEnum_CaptureLocalStream;
      task.mErrMsg = CAMERA_LOST;
      PostTaskEvent(task);
   });
   local_stream->AddEventListener(VIDEO_DENIED, [&](vhall::BaseEvent &Event)->void {
      LOGD_SDK("VIDEO_DENIED");
      CTaskEvent task;
      task.mEventType = TaskEventEnum_CaptureLocalStream;
      task.mErrMsg = VIDEO_DENIED;
      PostTaskEvent(task);
   });
   local_stream->AddEventListener(AUDIO_DENIED, [&](vhall::BaseEvent &Event)->void {
      LOGD_SDK("AUDIO_DENIED");
      CTaskEvent task;
      task.mEventType = TaskEventEnum_CaptureLocalStream;
      task.mErrMsg = ACCESS_ACCEPTED;
      PostTaskEvent(task);
   });
}

int WebRtcSDK::HandlePreviewCamera(void* wnd, const std::string& devGuid, VideoProfileIndex index, int micIndex /*= -1*/) {
   bool bRet = false;
   std::string camera_guid = devGuid;
   previewWnd = HWND(wnd);
   previewStreamConfig.mVideo = devGuid.length() > 0 ? true : false;
   if (micIndex > 0) {
      previewStreamConfig.mAudio = true;
   }
   else {
      previewStreamConfig.mAudio = false;
   }

   previewStreamConfig.videoOpt.mProfileIndex = index;
   previewStreamConfig.mStreamType = VHStreamType_AVCapture;
   LOGD_SDK("get mPreviewStreamThreadLock");
   CThreadLockHandle lockHandle(&mPreviewStreamThreadLock);
   if (mbStopPreviewCamera) {
      LOGD_SDK("mbStopPreviewCamera");
      return 0;
   }
   bool bInit = false;
   if (mPreviewLocalStream == nullptr) {
      LOGD_SDK("RelaseStream and make_shared VHStream");
      mPreviewLocalStream = std::make_shared<vhall::VHStream>(previewStreamConfig);
      if (mPreviewLocalStream) {
         //当调用摄像头，会触发ACCESS_ACCEPTED回掉。
         mPreviewLocalStream->AddEventListener(ACCESS_ACCEPTED, [&](vhall::BaseEvent &Event)->void {
            LOGD_SDK("ACCESS_ACCEPTED");
            CTaskEvent task;
            task.mEventType = TaskEventEnum_PREVIE_PLAY;
            task.mErrMsg = ACCESS_ACCEPTED;
            PostTaskEvent(task);
         });
         // 获取本地摄像头失败监听回调
         mPreviewLocalStream->AddEventListener(ACCESS_DENIED, [&](vhall::BaseEvent &Event)->void {
            LOGD_SDK("ACCESS_DENIED");
            CTaskEvent task;
            task.mEventType = TaskEventEnum_PREVIE_PLAY;
            task.mErrMsg = ACCESS_DENIED;
            PostTaskEvent(task);
         });
         mPreviewLocalStream->AddEventListener(VIDEO_DENIED, [&](vhall::BaseEvent &Event)->void {
            LOGD_SDK("VIDEO_DENIED");
            //视频设备打开失败，请您检测设备是否连接正常
            CTaskEvent task;
            task.mEventType = TaskEventEnum_PREVIE_PLAY;
            task.mErrMsg = VIDEO_DENIED;
            PostTaskEvent(task);
         });
         mPreviewLocalStream->AddEventListener(VIDEO_CAPTURE_ERROR, [&](vhall::BaseEvent &Event)->void {
            LOGD_SDK("VIDEO_DENIED");
            //视频设备打开失败，请您检测设备是否连接正常
            CTaskEvent task;
            task.mEventType = TaskEventEnum_PREVIE_PLAY;
            task.mErrMsg = VIDEO_DENIED;
            PostTaskEvent(task);
         });
         mPreviewLocalStream->AddEventListener(AUDIO_DENIED, [&](vhall::BaseEvent &Event)->void {
            LOGD_SDK("AUDIO_DENIED");
            //音频设备异常，请调整系统设置中音频设备的音频采样率为44100格式
            CTaskEvent task;
            task.mEventType = TaskEventEnum_PREVIE_PLAY;
            task.mErrMsg = AUDIO_DENIED;
            PostTaskEvent(task);
         });
         mPreviewLocalStream->AddEventListener(CAMERA_LOST, [&](vhall::BaseEvent &Event)->void {
            LOGD_SDK("STREAM_SOURCE_LOST");
            CTaskEvent task;
            task.mEventType = TaskEventEnum_PREVIE_PLAY;
            task.mErrMsg = CAMERA_LOST;
            PostTaskEvent(task);
         });
         bInit = true;
      }
   }
   if (bInit) {
      mPreviewLocalStream->Init();
      mPreviewLocalStream->SetFilterParam(mLocalStreamFilterParam);
   }
   if (mPreviewLocalStream) {
      mPreviewLocalStream->SetAudioListener(&mPreviewAudioVolume);
   }
   if (mPreviewLocalStream) {
      std::shared_ptr<vhall::MediaCaptureInfo> mediaInfo = std::make_shared<vhall::MediaCaptureInfo>();
      mediaInfo->VideoDevID = camera_guid;
      mPreviewLocalStream->Getlocalstream(mediaInfo);
   }
   else {
      if (mWebRtcSDKEventReciver) {
         LOGD_SDK("vlive::VHCapture_ACCESS_DENIED");
         mWebRtcSDKEventReciver->OnOpenCaptureCallback(vlive::VHStreamType_Preview_Video, vlive::VHCapture_ACCESS_DENIED, previewStreamConfig.mVideo, previewStreamConfig.mAudio);
      }
   }
   LOGD_SDK("VhallLive_OK");
   return vlive::VhallLive_OK;
}

int WebRtcSDK::GetPrevieMicVolumValue() {
   return mPreviewAudioVolume.GetAudioVolume();
}

/*
* 停止摄像头预览
*/
int WebRtcSDK::StopPreviewCamera() {
   LOGD_SDK("%s start", __FUNCTION__);
   CThreadLockHandle lockHandle(&mPreviewStreamThreadLock);
   if (mPreviewLocalStream) {
      LOGD_SDK("RemoveAllEventListener");
      mPreviewLocalStream->RemoveAllEventListener();
      LOGD_SDK("RelaseStream start");
      RelaseStream(mPreviewLocalStream);
      mPreviewVideoRecver = nullptr;
      LOGD_SDK("RelaseStream end");
   }
   mbStopPreviewCamera = true;
   LOGD_SDK("%s end", __FUNCTION__);
   return vlive::VhallLive_OK;
}

int WebRtcSDK::CheckPicEffectiveness(const std::string filePath) {
   vhall::PictureDecoder dec;
   std::shared_ptr<vhall::MediaCaptureInfo> mediaInfo = std::make_shared<vhall::MediaCaptureInfo>();
   std::shared_ptr<vhall::I420Data> mI420Data = std::make_shared<vhall::I420Data>();
   int nRet = dec.Decode(filePath.c_str(), mI420Data->mData, mI420Data->width, mI420Data->height);
   return nRet;
}

int WebRtcSDK::StartLocalCapturePicture(const std::string filePath, VideoProfileIndex index, bool doublePush/* = false*/) {
   LOGD_SDK("get StartLocalCapturePicture :%d filePath:%s mCurrentMicIndex:%d", index, filePath.c_str(), (int)mCurrentMicIndex);
   mCameraStreamConfig.mVideo = true;
   if (mCurrentMicIndex >= 0) {
      if (!mCameraStreamConfig.mAudio) {
         CThreadLockHandle lockHandle(&mLocalStreamThreadLock);
         LOGD_SDK("mLocalStreamThreadLock");
         if (mLiveRoom && mLocalStream) {
            LOGD_SDK("mLiveRoom->UnPublish");
            if (mLocalStream->GetStreamState() == VhallStreamState::Publish) {
               LOGD_SDK("mLocalStream->GetStreamState() == VhallStreamState::Publish");
               mLiveRoom->UnPublish(mLocalStream, [&](const vhall::RespMsg& resp) -> void {
                  
               }, SDK_TIMEOUT);
            }
         }
      }
      mCameraStreamConfig.mAudio = true;
   }
   else {
      mCameraStreamConfig.mAudio = false;
   }
   if (!mCameraStreamConfig.mAudio && !mCameraStreamConfig.mVideo) {
      return vlive::VhallLive_NO_DEVICE;
   }
   CTaskEvent task;
   task.mEventType = TaskEventEnum_StartLocalPicCapture;
   task.filePath = filePath;
   task.index = index;
   task.doublePush = doublePush;
   LOGD_SDK("StartLocalCapturePicture");
   PostTaskEvent(task);
   LOGD_SDK("PostTaskEvent StartLocalCapturePicture");
   return vlive::VhallLive_OK;
}

int WebRtcSDK::HandleStartLocalCapturePicture(const std::string filePath, VideoProfileIndex index, bool doublePush) {
   bool bRet = false;
   bool bNeedInit = false;
   bool bChangeCapability = false;
   if (doublePush) {
      if (mCameraStreamConfig.mNumSpatialLayers != 2) {
         bChangeCapability = true;
      }
      mCameraStreamConfig.mNumSpatialLayers = 2;
   }
   else {
      if (mCameraStreamConfig.mNumSpatialLayers != 0) {
         bChangeCapability = true;
      }
      mCameraStreamConfig.mNumSpatialLayers = 0;
   }

   if (mCameraStreamConfig.videoOpt.mProfileIndex != index) {
      bChangeCapability = true;
   }
   if (mCameraStreamConfig.mVideo && mCameraStreamConfig.mStreamType != VHStreamType_AVCapture) {
      mCameraStreamConfig.mStreamType = VHStreamType_AVCapture;
      if (mLocalStream) {
         mLocalStream->mStreamType = VHStreamType_AVCapture;
      }
      bChangeCapability = true;
      StopPushLocalStream();
   }
   mCameraStreamConfig.mStreamType = VHStreamType_AVCapture;
   mCameraStreamConfig.videoOpt.mProfileIndex = index;
   mCameraStreamConfig.videoOpt.lockRatio = true;
   mCameraStreamConfig.videoOpt.ratioH = 9;
   mCameraStreamConfig.videoOpt.ratioW = 16;
   mCameraStreamConfig.mLocal = true;
   mCameraStreamConfig.mAudio = true;
   CThreadLockHandle lockHandle(&mLocalStreamThreadLock);
   LOGD_SDK("get mLocalStreamThreadLock");
   if (mLocalStream == nullptr) {
      LOGD_SDK(" make mCameraStreamConfig ");
      mLocalStream = std::make_shared<vhall::VHStream>(mCameraStreamConfig);
      if (mLocalStream) {
         mLocalStream->mStreamAttributes = mWebRtcRoomOption.userData;
         RegisterLocalCaptureEvent(mLocalStream);
      }
      bNeedInit = true;
      LOGD_SDK("make new local stream");
   }
   if (mLocalStream) {
      mLocalStreamIsCapture = true;
   }
   if (bNeedInit && mLocalStream) {
      //初始化
      mLocalStream->Init();
      mLocalStream->SetFilterParam(mLocalStreamFilterParam);
      LOGD_SDK("Init");
   }

   if (mLocalStream) {
      mLocalStreamIsCapture = true;
      vhall::PictureDecoder dec;
      std::shared_ptr<vhall::MediaCaptureInfo> mediaInfo = std::make_shared<vhall::MediaCaptureInfo>();
      std::shared_ptr<vhall::I420Data> mI420Data = std::make_shared<vhall::I420Data>();
      dec.Decode(filePath.c_str(), mI420Data->mData, mI420Data->width, mI420Data->height);
      if (mI420Data->mData == nullptr) {
         if (mWebRtcSDKEventReciver) {
            LOGD_SDK("vlive::VHCapture_ACCESS_DENIED");
            mWebRtcSDKEventReciver->OnOpenCaptureCallback(vlive::VHStreamType_AVCapture, vlive::VHCapture_ACCESS_DENIED, mCameraStreamConfig.mVideo, mCameraStreamConfig.mAudio);
         }
      }
      else {
         mediaInfo->mI420Data = mI420Data;
         mLocalStream->mVideo = mCameraStreamConfig.mVideo;
         mLocalStream->Getlocalstream(mediaInfo);
         mLastCaptureIsCamera = false;
         LOGD_SDK("Getlocalstream");
         if (mLocalStream && bChangeCapability) {
            mLocalStream->ResetCapability(mCameraStreamConfig.videoOpt);
         }
      }
   }
   else {
      if (mWebRtcSDKEventReciver) {
         LOGD_SDK("vlive::VHCapture_ACCESS_DENIED");
         mWebRtcSDKEventReciver->OnOpenCaptureCallback(vlive::VHStreamType_AVCapture, vlive::VHCapture_ACCESS_DENIED, mCameraStreamConfig.mVideo, mCameraStreamConfig.mAudio);
      }
   }
   LOGD_SDK("VhallLive_OK");
   return vlive::VhallLive_OK;
}

/**
*   开始桌面采集与采集音量控制
*   参数：
*       dev_index:设备索引
*       dev_id;设备id
*       float:采集音量
**/
int WebRtcSDK::StartLocalCapturePlayer(const std::wstring dev_id, const int volume) {
   LOGD_SDK("StartLocalCapturePlayer volume:%d dev_id:%s", volume, SubScribeStreamManager::WString2String(dev_id).c_str());
   mDesktopCaptureId = dev_id;
   CTaskEvent task;
   task.mEventType = TaskEventEnum_SET_PLAYER_DEV;
   task.mVolume = volume;
   task.devIndex = mCurrentMicIndex;
   task.desktopCaptureId = dev_id;
   PostTaskEvent(task);
   return 0;
}

int WebRtcSDK::SetLocalCapturePlayerVolume(const int volume) {
   LOGD_SDK("SetLocalCapturePlayerVolume :%d", volume);
   vhall::VHStream::SetLoopBackVolume(volume);
   return 0;
}

/*
*   停止桌面音频采集
*/
int WebRtcSDK::StopLocalCapturePlayer() {
   LOGD_SDK("StopLocalCapturePlayer :%d", (int)mCurrentMicIndex);
   int nRet = vhall::VHStream::SetAudioInDevice(mCurrentMicIndex, L"") ? VhallLive_OK : VhallLive_NO_DEVICE;
   mDesktopCaptureId = L"";
   LOGD_SDK("StopLocalCapturePlayer :%d", nRet);
   return nRet;
}

int WebRtcSDK::StartLocalCapture(const std::string devId, VideoProfileIndex index, bool doublePush) {
   LOGD_SDK("devId :%s index:%d", devId.c_str(), index);
   if (devId.size() > 0) {
      mCameraStreamConfig.mVideo = true;
   }
   else {
      mCameraStreamConfig.mVideo = false;
   }
   if (mCurrentMicIndex >= 0) {
      if (!mCameraStreamConfig.mAudio) {
         CThreadLockHandle lockHandle(&mLocalStreamThreadLock);
         LOGD_SDK("mLocalStreamThreadLock");
         if (mLiveRoom && mLocalStream) {
            LOGD_SDK("mLiveRoom->UnPublish");
            if (mLocalStream->GetStreamState() == VhallStreamState::Publish) {
               LOGD_SDK("mLocalStream->GetStreamState() == VhallStreamState::Publish");
               mLiveRoom->UnPublish(mLocalStream, [&](const vhall::RespMsg& resp) -> void {

               }, SDK_TIMEOUT);
            }
         }
      }
      mCameraStreamConfig.mAudio = true;
   }
   else {
      mCameraStreamConfig.mAudio = false;
   }

   if (!mCameraStreamConfig.mAudio && !mCameraStreamConfig.mVideo) {
      LOGD_SDK("VhallLive_NO_DEVICE");
      return vlive::VhallLive_NO_DEVICE;
   }

   CTaskEvent task;
   task.mEventType = TaskEventEnum_StartLocalCapture;
   task.devId = devId;
   task.index = index;
   task.doublePush = doublePush;
   LOGD_SDK("StartLocalCapture");
   PostTaskEvent(task);
   LOGD_SDK("PostTaskEvent");
   return vlive::VhallLive_OK;
}

int WebRtcSDK::StartLocalAuxiliaryCapture(const std::string devId, VideoProfileIndex index) {
   LOGD_SDK("devId :%s index:%d", devId.c_str(), index);
   if (devId.size() > 0) {
      mAuxiliaryStreamConfig.mVideo = true;
   }
   else {
      LOGD_SDK("VhallLive_NO_DEVICE");
      return vlive::VhallLive_NO_DEVICE;
   }

   CTaskEvent task;
   task.mEventType = TaskEventEnum_StartLocalAuxiliaryCapture;
   task.devId = devId;
   task.index = index;
   LOGD_SDK("TaskEventEnum_StartLocalAuxiliaryCapture");
   PostTaskEvent(task);
   LOGD_SDK("TaskEventEnum_StartLocalAuxiliaryCapture");
   return vlive::VhallLive_OK;
}

void WebRtcSDK::StopLocalAuxiliaryCapture() {
   LOGD_SDK("");
   CThreadLockHandle lockHandle(&mLocalAuxiliaryStreamThreadLock);
   LOGD_SDK("mLocalAuxiliaryStreamThreadLock");
   RelaseStream(mLocalAuxiliaryStream);
   LOGD_SDK("mLocalAuxiliaryStream");
   mLocalAuxiliaryStreamId.clear();
   LOGD_SDK("StopLocalAuxiliaryCapture end");
}

int WebRtcSDK::HandleStartLocalAuxiliaryCapture(const std::string devId, VideoProfileIndex index) {

   bool bRet = false;
   mAuxiliaryStreamConfig.videoOpt.mProfileIndex = index;
   mAuxiliaryStreamConfig.mLocal = true;
   CThreadLockHandle lockHandle(&mLocalAuxiliaryStreamThreadLock);
   if (mLocalAuxiliaryStream) {
      RelaseStream(mLocalAuxiliaryStream);
   }
   LOGD_SDK("get mLocalAuxiliaryStreamThreadLock");
   if (mLocalAuxiliaryStream == nullptr) {
      LOGD_SDK(" make mCameraStreamConfig ");
      mLocalAuxiliaryStream = std::make_shared<vhall::VHStream>(mAuxiliaryStreamConfig);
      if (mLocalAuxiliaryStream) {
//         mLocalAuxiliaryStream->SetBeautifyLevel(0);
         //当调用摄像头，会触发ACCESS_ACCEPTED回掉。
         mLocalAuxiliaryStream->AddEventListener(ACCESS_ACCEPTED, [&](vhall::BaseEvent &Event)->void {
            LOGD_SDK("ACCESS_ACCEPTED");
            CTaskEvent task;
            task.mEventType = TaskEventEnum_CaptureAuxiliaryLocalStream;
            task.mErrMsg = ACCESS_ACCEPTED;
            PostTaskEvent(task);
         });

         // 获取本地摄像头失败监听回调
         mLocalAuxiliaryStream->AddEventListener(ACCESS_DENIED, [&](vhall::BaseEvent &Event)->void {
            LOGD_SDK("ACCESS_DENIED");
            CTaskEvent task;
            task.mEventType = TaskEventEnum_CaptureAuxiliaryLocalStream;
            task.mErrMsg = ACCESS_DENIED;
            PostTaskEvent(task);
         });


         mLocalAuxiliaryStream->AddEventListener(CAMERA_LOST, [&](vhall::BaseEvent &Event)->void {
            LOGD_SDK("STREAM_SOURCE_LOST");
            CTaskEvent task;
            task.mEventType = TaskEventEnum_CaptureAuxiliaryLocalStream;
            task.mErrMsg = CAMERA_LOST;
            PostTaskEvent(task);
         });

         mLocalAuxiliaryStream->AddEventListener(VIDEO_DENIED, [&](vhall::BaseEvent &Event)->void {
            CTaskEvent task;
            task.mEventType = TaskEventEnum_CaptureAuxiliaryLocalStream;
            task.mErrMsg = VIDEO_DENIED;
            PostTaskEvent(task);
         });

         mLocalAuxiliaryStream->AddEventListener(AUDIO_DENIED, [&](vhall::BaseEvent &Event)->void {
            LOGD_SDK("AUDIO_DENIED");
            CTaskEvent task;
            task.mEventType = TaskEventEnum_CaptureAuxiliaryLocalStream;
            task.mErrMsg = AUDIO_DENIED;
            PostTaskEvent(task);
         });
      }
      LOGD_SDK("make new local stream");
   }
   if (mLocalAuxiliaryStream) {
      LOGD_SDK("mLocalAuxiliaryStream");
      mLocalStreamIsCapture = true;
      mLocalAuxiliaryStream->Init();
      LOGD_SDK("Init");
      std::shared_ptr<vhall::MediaCaptureInfo> mediaInfo = std::make_shared<vhall::MediaCaptureInfo>();
      mediaInfo->VideoDevID = devId;
      mLocalAuxiliaryStream->Getlocalstream(mediaInfo);
      LOGD_SDK("Getlocalstream");
   }
   else {
      if (mWebRtcSDKEventReciver) {
         LOGD_SDK("vlive::VHCapture_ACCESS_DENIED");
         mWebRtcSDKEventReciver->OnOpenCaptureCallback(vlive::VHStreamType_AVCapture, vlive::VHCapture_ACCESS_DENIED, mCameraStreamConfig.mVideo, mCameraStreamConfig.mAudio);
      }
   }
   LOGD_SDK("VhallLive_OK");
   return vlive::VhallLive_OK;
}


int WebRtcSDK::HandleStartLocalCapture(const std::string devId, VideoProfileIndex index, bool doublePush) {
   bool bRet = false;
   bool bNeedInit = false;
   bool bChangeCapability = false;
   if (doublePush) {
      if (mCameraStreamConfig.mNumSpatialLayers != 2) {
         bChangeCapability = true;
      }
      mCameraStreamConfig.mNumSpatialLayers = 2;
   }
   else {
      if (mCameraStreamConfig.mNumSpatialLayers != 0) {
         bChangeCapability = true;
      }
      mCameraStreamConfig.mNumSpatialLayers = 0;
   }

   if (mCameraStreamConfig.videoOpt.mProfileIndex != index) {
      bChangeCapability = true;
   }

   if (mCameraStreamConfig.mVideo && mCameraStreamConfig.mStreamType != VHStreamType_AVCapture) {
      mCameraStreamConfig.mStreamType = VHStreamType_AVCapture;
      mCameraStreamConfig.mNumSpatialLayers = 2;
      if (mLocalStream) {
         mLocalStream->mStreamType = VHStreamType_AVCapture;
      }
      bChangeCapability = true;
      StopPushLocalStream();
   }
   else if (!mCameraStreamConfig.mVideo && mCameraStreamConfig.mStreamType != VHStreamType_PureAudio) {
      mCameraStreamConfig.mStreamType = VHStreamType_PureAudio;
      mCameraStreamConfig.mNumSpatialLayers = 0;
      if (mLocalStream) {
         mLocalStream->mStreamType = VHStreamType_PureAudio;
      }
      bChangeCapability = true;
      StopPushLocalStream();
   }
   if (mCurrentCameraGuid != devId) {
      mCameraStreamConfig.videoOpt.devId = devId;
      bChangeCapability = true;
   }
   else {
      mLastCaptureIsCamera = false;
   }

   mCameraStreamConfig.videoOpt.mProfileIndex = index;
   mCameraStreamConfig.videoOpt.lockRatio = true;
   mCameraStreamConfig.videoOpt.ratioH = 9;
   mCameraStreamConfig.videoOpt.ratioW = 16;
   mCameraStreamConfig.mLocal = true;
   CThreadLockHandle lockHandle(&mLocalStreamThreadLock);
   if (mLocalStream && bChangeCapability) {
      mLocalStream->ResetCapability(mCameraStreamConfig.videoOpt);
   }
   mCurrentCameraGuid = devId;
   LOGD_SDK("get mLocalStreamThreadLock mCameraStreamConfig.mStreamType %d mCameraStreamConfig.mNumSpatialLayers %d", mCameraStreamConfig.mStreamType, mCameraStreamConfig.mNumSpatialLayers);
   if (mLocalStream == nullptr) {
      LOGD_SDK(" make mCameraStreamConfig ");
      mLocalStream = std::make_shared<vhall::VHStream>(mCameraStreamConfig);
      if (mLocalStream) {
         mLocalStream->mStreamAttributes = mWebRtcRoomOption.userData;
         mLocalStream->SetFilterParam(mLocalStreamFilterParam);
         RegisterLocalCaptureEvent(mLocalStream);
      }
      bNeedInit = true;
      LOGD_SDK("make new local stream");
   }
   if (mLocalStream) {
      mLocalStreamIsCapture = true;
   }
   if (bNeedInit && mLocalStream) {
      //初始化
      LOGD_SDK("Init");
      mLocalStream->Init();
      mLocalStream->SetFilterParam(mLocalStreamFilterParam);
      std::shared_ptr<vhall::MediaCaptureInfo> mediaInfo = std::make_shared<vhall::MediaCaptureInfo>();
      mediaInfo->VideoDevID = devId;
      mLocalStream->mVideo = mCameraStreamConfig.mVideo;
      mLocalStream->Getlocalstream(mediaInfo);
      mLastCaptureIsCamera = true;
   }
   else if (mLocalStream && !mLastCaptureIsCamera) {
      LOGD_SDK("Getlocalstream");
      std::shared_ptr<vhall::MediaCaptureInfo> mediaInfo = std::make_shared<vhall::MediaCaptureInfo>();
      mediaInfo->VideoDevID = devId;
      mLocalStream->mVideo = mCameraStreamConfig.mVideo;
      mLocalStream->Getlocalstream(mediaInfo);
      mLastCaptureIsCamera = true;
      LOGD_SDK("end");
   }
   LOGD_SDK("VhallLive_OK");
   return vlive::VhallLive_OK;
}

void WebRtcSDK::StopLocalCapture() {
   LOGD_SDK("");
   {
      CThreadLockHandle lockHandle(&mLocalStreamThreadLock);
      LOGD_SDK("mLocalStreamThreadLock");
      LOGD_SDK("RelaseStream");
      RelaseStream(mLocalStream);
      mLastCaptureIsCamera = false;
      mVideoSsrc.reset();
      mVideoSsrc = nullptr;
   }
   mLocalStreamIsCapture = false;
   mbIsOpenCamera = true;
   mbIsOpenMic = true;
   mLocalStreamId.clear();
   mCurrentCameraGuid.clear();
   mLocalCameraRecv = nullptr;
   mLocalStreamWnd = nullptr;
   LOGD_SDK("StopLocalCapture ok");
}

bool WebRtcSDK::GetCaptureStreamType(const std::wstring& user_id, vlive::VHStreamType streamType, vlive::CaptureStreamAVType type) {
   LOGD_SDK("user_id:%ws streamType:%d type:%d", user_id.c_str(), streamType, type);
   if (user_id == mWebRtcRoomOption.strUserId) {
      switch (streamType)
      {
      case vlive::VHStreamType_AVCapture: {
         CThreadLockHandle lockHandle(&mLocalStreamThreadLock);
         LOGD_SDK("mLocalStreamThreadLock");
         if (mLocalStream == nullptr) {
            LOGD_SDK(""); ("mLocalStream == nullptr");
            return false;
         }
         if (type == CaptureStreamAVType_Audio) {
            LOGD_SDK("HasAudio:%d", mLocalStream->HasAudio());
            return mLocalStream->HasAudio();
         }
         else {
            LOGD_SDK("HasVideo:%d", mLocalStream->HasVideo());
            return mLocalStream->HasVideo();
         }
         break;
      }
      case vlive::VHStreamType_Desktop: {
         LOGD_SDK("mDesktopStreamThreadLock");
         CThreadLockHandle lockHandle(&mDesktopStreamThreadLock);
         if (mDesktopStream == nullptr) {
            LOGD_SDK(""); ("mDesktopStream == nullptr");
            return false;
         }
         if (type == CaptureStreamAVType_Audio) {
            LOGD_SDK("HasAudio:%d", mDesktopStream->HasAudio());
            return mDesktopStream->HasAudio();
         }
         else {
            LOGD_SDK("HasVideo:%d", mDesktopStream->HasVideo());
            return mDesktopStream->HasVideo();
         }
         break;
      }
      case vlive::VHStreamType_MediaFile: {
         LOGD_SDK("mMediaStreamThreadLock");
         CThreadLockHandle lockHandle(&mMediaStreamThreadLock);
         if (mMediaStream == nullptr) {
            LOGD_SDK("mMediaStream == nullptr");
            return false;
         }
         if (type == CaptureStreamAVType_Audio) {
            LOGD_SDK("HasAudio:%d", mMediaStream->HasAudio());
            return mMediaStream->HasAudio();
         }
         else {
            LOGD_SDK("HasVideo:%d", mMediaStream->HasVideo());
            return mMediaStream->HasVideo();
         }
         break;
      }
      default:
         break;
      }
   }
   else {
      std::wstring user_id_temp = user_id;
      switch (streamType)
      {
      case vlive::VHStreamType_Desktop: {
         user_id_temp = user_id_temp + SubScribeStreamManager::String2WString(std::to_string(streamType));
         break;
      }
      case vlive::VHStreamType_MediaFile: {
         user_id_temp = user_id_temp + SubScribeStreamManager::String2WString(std::to_string(streamType));
         break;
      }
      default:
         break;
      }
      return mSubScribeStreamManager.IsHasMediaType(user_id_temp, CaptureStreamAVType_Audio);
   }
   return false;
}


int WebRtcSDK::StartPushLocalStream() {
   LOGD_SDK("");
   if (!mbIsWebRtcRoomConnected) {
      LOGD_SDK("VhallLive_ROOM_DISCONNECT");
      return vlive::VhallLive_ROOM_DISCONNECT;
   }
   CThreadLockHandle lockHandle(&mLocalStreamThreadLock);
   LOGD_SDK("mLocalStreamThreadLock");
   if (mLiveRoom && mLocalStream) {
      LOGD_SDK("Publish mLocalStream");
      bool hasVideo = mLocalStream->HasVideo();
      bool hasAudio = mLocalStream->HasAudio();
      if (mVideoSsrc == nullptr) {
         mVideoSsrc = std::make_shared<vhall::SendVideoSsrc>();
      }
      std::string join_uid = SubScribeStreamManager::WString2String(mWebRtcRoomOption.strUserId);
      LOGD_SDK("Publish mLocalStream hasVideo %d hasAudio %d mCameraStreamConfig.mStreamType %d", mCameraStreamConfig.mVideo, mCameraStreamConfig.mAudio, mCameraStreamConfig.mStreamType);
      mLiveRoom->Publish(mLocalStream, [&, hasVideo, hasAudio, join_uid](const vhall::PublishRespMsg& resp) -> void {
         if (resp.mResult == SDK_SUCCESS) {
            LOGD_SDK("Publish mLocalStream mLocalStreamId:%s", resp.mStreamId.c_str());
         }
         else {
            LOGD_SDK("Publish mLocalStream err:%s resp.mCode:%d", resp.mMsg.c_str(), resp.mCode);
            if (vhall::StreamPublished == resp.mCode || vhall::StreamIsEmpty == resp.mCode) {
               return;
            }
            CTaskEvent roomConnectTask;
            roomConnectTask.mEventType = TaskEventEnum_STREAM_PUSH_ERROR;
            roomConnectTask.mStreamType = vlive::VHStreamType_AVCapture;
            roomConnectTask.mErrorType = resp.mCode;
            roomConnectTask.mErrMsg = resp.mMsg;
            PostTaskEvent(roomConnectTask);
            
         }
      }, SDK_TIMEOUT);
   }
   else {
      LOGD_SDK("VhallLive_NO_OBJ");
      return vlive::VhallLive_NO_OBJ;
   }
   LOGD_SDK("VhallLive_OK");
   return vlive::VhallLive_OK;
}

int WebRtcSDK::StopPushLocalStream() {
   LOGD_SDK("");
   if (!mbIsWebRtcRoomConnected) {
      LOGD_SDK("VhallLive_ROOM_DISCONNECT");
      return vlive::VhallLive_ROOM_DISCONNECT;
   }
   CThreadLockHandle lockHandle(&mLocalStreamThreadLock);
   LOGD_SDK("mLocalStreamThreadLock");
   if (mLiveRoom && mLocalStream) {
      LOGD_SDK("mLiveRoom->UnPublish");
      if (mLocalStream->GetStreamState() == VhallStreamState::Publish) {
         LOGD_SDK("mLocalStream->GetStreamState() == VhallStreamState::Publish");
         mLiveRoom->UnPublish(mLocalStream, [&](const vhall::RespMsg& resp) -> void {
            mLocalStreamId.clear();
            LOGD_SDK("UnPublish VHStreamType_AVCapture msg:%s", resp.mMsg.c_str());
            if (mWebRtcSDKEventReciver) {
               if (resp.mResult == SDK_SUCCESS) {
                  mWebRtcSDKEventReciver->OnStopPushStreamCallback(vlive::VHStreamType_AVCapture, vlive::VhallLive_OK, resp.mMsg);
               }
               else {
                  mWebRtcSDKEventReciver->OnStopPushStreamCallback(vlive::VHStreamType_AVCapture, resp.mCode, resp.mMsg);
               }
            }
         }, SDK_TIMEOUT);
         mLocalStream->SetStreamState(UnPublish);
      }
      else {
         LOGD_SDK("mLocalStream->GetStreamState() != VhallStreamState::Publish");
      }
   }
   else {
      LOGD_SDK("VhallLive_NO_OBJ");
      return vlive::VhallLive_NO_OBJ;
   }
   LOGD_SDK("end");
   return vlive::VhallLive_OK;
}

bool WebRtcSDK::IsEnableConfigMixStream() {
   return mbIsMixedAlready;
}

void WebRtcSDK::SetEnableConfigMixStream(bool enable) {
   LOGD_SDK("enable :%d", enable);
   mbIsMixedAlready = enable;
}

int WebRtcSDK::StartConfigBroadCast(LayoutMode layoutMode, int width, int height, int fps, int bitrate, bool showBorder, std::string boradColor, std::string backGroundColor, SetBroadCastEventCallback fun/* = nullptr*/) {
   LOGD_SDK("layoutMode:%d", layoutMode);
   if (!mbIsWebRtcRoomConnected) {
      LOGD_SDK("VhallLive_ROOM_DISCONNECT");
      return vlive::VhallLive_ROOM_DISCONNECT;
   }
   LOGD_SDK("");
   if (mSettingCurrentLayoutMode) {
      LOGD_SDK("VhallLive_IS_PROCESSING");
      return vlive::VhallLive_IS_PROCESSING;
   }

   if (!mbIsMixedAlready) {
      LOGD_SDK("VhallLive_SERVER_NOT_READY");
      return VhallLive_SERVER_NOT_READY;
   }
   mSettingCurrentLayoutMode = true;
   vhall::RoomBroadCastOpt Options;
   Options.bitrate = bitrate;
   Options.framerate = fps;
   Options.height = height;
   Options.width = width;
   Options.publishUrl = SubScribeStreamManager::WString2String(mWebRtcRoomOption.strThirdPushStreamUrl);

   Options.backgroundColor = backGroundColor;
   Options.borderColor = boradColor;
   Options.borderWidth = 4;
   Options.borderExist = showBorder;
   Options.precast_pic_exist = false;

   if (mLiveRoom) {
      Options.layoutMode = layoutMode;
      mLiveRoom->configRoomBroadCast(Options, [&, fun](const vhall::RespMsg& resp) -> void {
         LOGD_SDK("configRoomBroadCast  mResult:%s msg:%s", resp.mResult.c_str(), resp.mMsg.c_str());
         if (fun) {
            fun(resp.mResult, resp.mMsg, resp.mCode);
         }
         mSettingCurrentLayoutMode = false;
      }, SDK_TIMEOUT);
   }
   else {
      LOGD_SDK("VhallLive_NO_OBJ");
      return vlive::VhallLive_NO_OBJ;
   }
   return vlive::VhallLive_OK;
}

int WebRtcSDK::StartConfigBroadCast(LayoutMode layoutMode, BroadCastVideoProfileIndex profileIndex, bool showBorder, std::string boradColor, std::string backGroundColor, SetBroadCastEventCallback fun/* = nullptr*/) {
   LOGD_SDK("layoutMode:%d", layoutMode);
   mSettingCurrentLayoutMode = true;
   vhall::RoomBroadCastOpt Options;
   Options.mProfileIndex = profileIndex;
   Options.publishUrl = SubScribeStreamManager::WString2String(mWebRtcRoomOption.strThirdPushStreamUrl);
   Options.backgroundColor = backGroundColor;
   Options.borderColor = boradColor;
   Options.borderWidth = 4;
   Options.borderExist = showBorder;
   Options.precast_pic_exist = false;
   if (mLiveRoom) {
      Options.layoutMode = layoutMode;
      mLiveRoom->configRoomBroadCast(Options, [&, fun](const vhall::RespMsg& resp) -> void {
         LOGD_SDK("configRoomBroadCast  mResult:%s msg:%s code %d", resp.mResult.c_str(), resp.mMsg.c_str(), resp.mCode);
         if (fun) {
            fun(resp.mResult, resp.mMsg, resp.mCode);
         }
         mSettingCurrentLayoutMode = false;
      }, SDK_TIMEOUT);
   }
   else {
      LOGD_SDK("VhallLive_NO_OBJ");
      return vlive::VhallLive_NO_OBJ;
   }
   LOGD_SDK("leave", layoutMode);
   return vlive::VhallLive_OK;
}


int WebRtcSDK::SetConfigBroadCastLayOut(LayoutMode mode, SetBroadCastEventCallback fun) {
   LOGD_SDK("layoutMode:%d", mode);
   if (!mbIsWebRtcRoomConnected) {
      LOGD_SDK("VhallLive_ROOM_DISCONNECT");
      return vlive::VhallLive_ROOM_DISCONNECT;
   }
   if (mLiveRoom) {
      mLiveRoom->setMixLayoutMode(mode, [&, fun](const vhall::RespMsg& resp) -> void {
         LOGD_SDK("SetConfigBroadCastLayOut  mResult:%s msg:%s", resp.mResult.c_str(), resp.mMsg.c_str());
         if (fun) {
            fun(resp.mResult, resp.mMsg, resp.mCode);
         }
         mSettingCurrentLayoutMode = false;
      }, SDK_TIMEOUT);
   }
   else {
      LOGD_SDK("VhallLive_NO_OBJ");
      return vlive::VhallLive_NO_OBJ;
   }
   return vlive::VhallLive_OK;
}

int WebRtcSDK::StopBroadCast(SetBroadCastEventCallback fun) {
   if (mLiveRoom) {
      LOGD_SDK("StopBroadCast");
      mLiveRoom->stopRoomBroadCast([&, fun](const vhall::RespMsg& resp) -> void {
         LOGD_SDK("StopBroadCast callback:%s ", resp.mMsg.c_str());
         if (fun) {
            fun(resp.mResult, resp.mMsg, resp.mCode);
         }
      }, SDK_TIMEOUT);
   }
   return 0;
}

int WebRtcSDK::SetMixLayoutMainView(std::string stream, SetBroadCastEventCallback fun /*= nullptr*/) {
   LOGD_SDK("");
   if (!mbIsWebRtcRoomConnected) {
      LOGD_SDK("VhallLive_ROOM_DISCONNECT");
      return vlive::VhallLive_ROOM_DISCONNECT;
   }
   if (!mbIsMixedAlready) {
      LOGD_SDK("VhallLive_SERVER_NOT_READY");
      return VhallLive_SERVER_NOT_READY;
   }

   if (mLiveRoom) {
      LOGD_SDK("setMixLayoutMainScreen stream:%s", stream.c_str());
      mLiveRoom->setMixLayoutMainScreen(stream.c_str(), [&, fun](const vhall::RespMsg& resp) -> void {
         LOGD_SDK("setMixLayoutMainScreen callback:%s resp.result %s %d", resp.mMsg.c_str(), resp.mResult.c_str(),resp.mCode);
         if (fun) {
            fun(resp.mResult, resp.mMsg, resp.mCode);
         }
      }, SDK_TIMEOUT);
   }
   else {
      LOGD_SDK("VhallLive_NO_OBJ");
      return vlive::VhallLive_NO_OBJ;
   }
   return vlive::VhallLive_OK;
}

int WebRtcSDK::StartPushSoftWareStream() {
   LOGD_SDK("");
   if (!mbIsEnablePlayFileAndShare) {
      LOGD_SDK("VhallLive_MUTEX_OPERATE");
      return vlive::VhallLive_MUTEX_OPERATE;
   }
   if (!mbIsWebRtcRoomConnected) {
      LOGD_SDK("VhallLive_ROOM_DISCONNECT");
      return vlive::VhallLive_ROOM_DISCONNECT;
   }
   LOGD_SDK("mSoftwareStreamThreadLock");
   CThreadLockHandle lockHandle(&mSoftwareStreamThreadLock);
   if (mSoftwareStream && mLiveRoom) {
      LOGD_SDK("Publish mSoftwareStream");
      std::string join_uid = SubScribeStreamManager::WString2String(mWebRtcRoomOption.strUserId);
      mLiveRoom->Publish(mSoftwareStream, [&, join_uid](const vhall::PublishRespMsg& resp) -> void {
         if (resp.mResult == SDK_SUCCESS) {
            LOGD_SDK("Publish mSoftwareStreamId :%s", resp.mStreamId.c_str());
         }
         else {
            LOGD_SDK("Publish mSoftwareStream err:%s resp.mCode:%d", resp.mMsg.c_str(), resp.mCode);
            if (vhall::StreamPublished == resp.mCode || vhall::StreamIsEmpty == resp.mCode) {
               return;
            }
            CTaskEvent roomConnectTask;
            roomConnectTask.mEventType = TaskEventEnum_STREAM_PUSH_ERROR;
            roomConnectTask.mStreamType = vlive::VHStreamType_SoftWare;
            roomConnectTask.mErrorType = resp.mCode;
            roomConnectTask.mErrMsg = resp.mMsg;
            PostTaskEvent(roomConnectTask);
         }
      }, SDK_TIMEOUT);
   }
   else {
      LOGD_SDK("VhallLive_NO_OBJ");
      return vlive::VhallLive_NO_OBJ;
   }
   LOGD_SDK("");
   return vlive::VhallLive_OK;
}

/*开始软件共享采集*/
int WebRtcSDK::SelectSource(int64_t index) {
   LOGD_SDK("enter index:%d ", index);
   if (!mbIsEnablePlayFileAndShare) {
      LOGD_SDK("VhallLive_MUTEX_OPERATE");
      return vlive::VhallLive_MUTEX_OPERATE;
   }
   if (mMediaStreamIsCapture) {
      LOGD_SDK("mMediaStreamIsCapture VhallLive_MUTEX_OPERATE");
      return vlive::VhallLive_MUTEX_OPERATE;
   }

   CThreadLockHandle lockHandle(&mSoftwareStreamThreadLock);
   if (mSoftwareStream) {
      if (mWebRtcSDKEventReciver) {
         mWebRtcSDKEventReciver->OnOpenCaptureCallback(vlive::VHStreamType_SoftWare, vlive::VHCapture_OK, true, false);
      }
      mSoftwareStream->SelectSource(index);
   }
   else {
      mSoftSourceStreamConfig.mAudio = false;
      mSoftSourceStreamConfig.mData = true;
      mSoftSourceStreamConfig.mVideo = true;
      mSoftSourceStreamConfig.mStreamType = vlive::VHStreamType_SoftWare;
      mSoftSourceStreamConfig.mVideoCodecPreference = "VP8";
      mSoftSourceStreamConfig.videoOpt.mProfileIndex = RTC_VIDEO_PROFILE_720P_16x9_M;
      LOGD_SDK("StreamConfig width:%d height:%d fps:%d wb:%d", mSoftSourceStreamConfig.videoOpt.maxWidth, mSoftSourceStreamConfig.videoOpt.maxHeight, mSoftSourceStreamConfig.videoOpt.maxFrameRate, mSoftSourceStreamConfig.videoOpt.minVideoBW);
      LOGD_SDK("make_shared StreamConfig");
      mSoftWareStreamIsCapture = true;
      mSoftSourceStreamConfig.mLocal = true;
      mSoftwareStream = std::make_shared<vhall::VHStream>(mSoftSourceStreamConfig);
      if (mSoftwareStream) {
         LOGD_SDK("index:%d", index);
         mSoftwareStream->Init();
         int64_t iTemp = index;
         //当调用软件共享成功。
         mSoftwareStream->AddEventListener(ACCESS_ACCEPTED, [&, iTemp](vhall::BaseEvent &Event)->void {
            LOGD_SDK("ACCESS_ACCEPTED");
            CTaskEvent task;
            task.mEventType = TaskEventEnum_CaptureSoftwareStream;
            task.mErrMsg = ACCESS_ACCEPTED;
            task.devIndex = iTemp;
            PostTaskEvent(task);
         });
         //当调用软件共享失败。
         mSoftwareStream->AddEventListener(ACCESS_DENIED, [&](vhall::BaseEvent &Event)->void {//错误提醒
            CTaskEvent task;
            task.mEventType = TaskEventEnum_CaptureSoftwareStream;
            task.mErrMsg = ACCESS_DENIED;
            PostTaskEvent(task);
         });
         mSoftwareStream->Getlocalstream();
      }
      LOGD_SDK("VhallLive_NO_OBJ");
      return vlive::VhallLive_NO_OBJ;
   }

   LOGD_SDK("");
   return vlive::VhallLive_OK;
}

/*开始桌面共享采集*/
int WebRtcSDK::StartDesktopCapture(int64_t index, VideoProfileIndex profileIndex) {
   LOGD_SDK("enter index:%d profileIndex:%d", index, profileIndex);
   GetDesktops(vlive::VHStreamType_Desktop);
   if (!mbIsEnablePlayFileAndShare) {
      LOGD_SDK("VhallLive_MUTEX_OPERATE");
      return vlive::VhallLive_MUTEX_OPERATE;
   }
   if (mMediaStreamIsCapture) {
      LOGD_SDK("mMediaStreamIsCapture VhallLive_MUTEX_OPERATE");
      return vlive::VhallLive_MUTEX_OPERATE;
   }
   LOGD_SDK("");

   desktopStreamConfig.mAudio = false;
   desktopStreamConfig.mData = true;
   desktopStreamConfig.mVideo = true;
   desktopStreamConfig.mStreamType = vlive::VHStreamType_Desktop;
   desktopStreamConfig.videoOpt.mProfileIndex = profileIndex;
   desktopStreamConfig.mLocal = true;
   mDesktopStreamIsCapture = true;
   int64_t iTempId = index;
   CThreadLockHandle lockHandle(&mDesktopStreamThreadLock);
   if (mDesktopStream) {
      mDesktopStream->SelectSource(index);
      if (mWebRtcSDKEventReciver) {
         mWebRtcSDKEventReciver->OnOpenCaptureCallback(vlive::VHStreamType_Desktop, vlive::VHCapture_OK, true, false);
      }
   }
   else
   {
      RelaseStream(mDesktopStream);
      LOGD_SDK("make_shared StreamConfig");
      if (mDesktopStream == nullptr) {
         mDesktopStream = std::make_shared<vhall::VHStream>(desktopStreamConfig);
         if (mDesktopStream) {
            mDesktopStream->mStreamAttributes = mWebRtcRoomOption.userData;
            LOGD_SDK("index:%d", index);
            mDesktopStreamFilterParam.beautyLevel = 0;
            mDesktopStream->SetFilterParam(mDesktopStreamFilterParam);
            mDesktopStream->Init();
            //当调用桌面共享成功。
            mDesktopStream->AddEventListener(ACCESS_ACCEPTED, [&, iTempId](vhall::BaseEvent &Event)->void {
               LOGD_SDK("ACCESS_ACCEPTED");
               CTaskEvent task;
               task.mEventType = TaskEventEnum_CaptureDesktopStream;
               task.mErrMsg = ACCESS_ACCEPTED;
               task.devIndex = iTempId;
               PostTaskEvent(task);
            });
            //当调用桌面共享失败。
            mDesktopStream->AddEventListener(ACCESS_DENIED, [&](vhall::BaseEvent &Event)->void {//错误提醒
               CTaskEvent task;
               task.mEventType = TaskEventEnum_CaptureDesktopStream;
               task.mErrMsg = ACCESS_DENIED;
               LOGD_SDK("ACCESS_DENIED");
               PostTaskEvent(task);
            });
         }
      }
      if (mDesktopStream) {
         mDesktopStream->Getlocalstream();
      }
   }
   LOGD_SDK("");
   return vlive::VhallLive_OK;
}

/*停止桌面共享采集*/
void WebRtcSDK::StopDesktopCapture() {
   LOGD_SDK("mDesktopStreamThreadLock");
   StopPushDesktopStream();
   CThreadLockHandle lockHandle(&mDesktopStreamThreadLock);
   if (mDesktopStream) {
      mDesktopStream->stop();
      RelaseStream(mDesktopStream);
      mDesktopVideoSsrc.reset();
      mDesktopVideoSsrc = nullptr;
   }
   mDesktopStreamIsCapture = false;
   LOGD_SDK("");
}

int WebRtcSDK::SetDesktopEdgeEnhance(bool enable) {
   LOGD_SDK("SetDesktopEdgeEnhance");
   CThreadLockHandle lockHandle(&mDesktopStreamThreadLock);
   if (mDesktopStream) {
      vhall::LiveVideoFilterParam param;
      param.enableEdgeEnhance = enable;
      mDesktopStream->SetFilterParam(param);
      LOGD_SDK("SetDesktopEdgeEnhance end");
      return 0;
   }
   LOGD_SDK("SetDesktopEdgeEnhance end");
   return -1;
}

bool WebRtcSDK::IsSupprotBeauty() {
   bool is_support = false;
   std::shared_ptr<vhall::VHStream> stream = std::make_shared<vhall::VHStream>(mCameraStreamConfig);
   if (stream) {
      is_support = stream->IsSupportBeauty();
      stream.reset();
   }
   return is_support;
}

int WebRtcSDK::SetCameraBeautyLevel(int level) {
   CTaskEvent task;
   task.mEventType = TaskEventEnum_LocalCaptureBeautyLevel;
   task.mBeautyLevel = level;
   LOGD_SDK("TaskEventEnum_LocalCaptureBeautyLevel %d", level);
   PostTaskEvent(task);
   LOGD_SDK("PostTaskEvent TaskEventEnum_LocalCaptureBeautyLevel");
   return 0;
}

int WebRtcSDK::SetPreviewCameraBeautyLevel(int level) {
   CTaskEvent task;
   task.mEventType = TaskEventEnum_PreviewBeautyLevel;
   task.mBeautyLevel = level;
   LOGD_SDK("SetPreviewCameraBeautyLevel %d", level);
   PostTaskEvent(task);
   LOGD_SDK("PostTaskEvent SetPreviewCameraBeautyLevel");
   return 0;
}

/*停止软件源共享采集*/
void WebRtcSDK::StopSoftwareCapture(){
   LOGD_SDK("mDesktopStreamThreadLock");
   StopPushSoftWareStream();
   CThreadLockHandle lockHandle(&mSoftwareStreamThreadLock);
   if (mSoftwareStream) {
      mSoftwareStream->stop();
      RelaseStream(mSoftwareStream);
   }
   mSoftWareStreamIsCapture = false;
   LOGD_SDK("");
}

int WebRtcSDK::StartPushDesktopStream() {
   LOGD_SDK("");
   if (!mbIsEnablePlayFileAndShare) {
      LOGD_SDK("VhallLive_MUTEX_OPERATE");
      return vlive::VhallLive_MUTEX_OPERATE;
   }
   if (!mbIsWebRtcRoomConnected) {
      LOGD_SDK("VhallLive_ROOM_DISCONNECT");
      return vlive::VhallLive_ROOM_DISCONNECT;
   }
   LOGD_SDK("mDesktopStreamThreadLock");
   CThreadLockHandle lockHandle(&mDesktopStreamThreadLock);
   if (mDesktopStream && mLiveRoom) {
      if (mDesktopVideoSsrc == nullptr) {
         mDesktopVideoSsrc = std::make_shared<vhall::SendVideoSsrc>();
      }
      LOGD_SDK("Publish mDesktopStream");
      std::string join_uid = SubScribeStreamManager::WString2String(mWebRtcRoomOption.strUserId);
      mLiveRoom->Publish(mDesktopStream, [&, join_uid](const vhall::PublishRespMsg& resp) -> void {
         if (resp.mResult == SDK_SUCCESS) {
            LOGD_SDK("Publish mDesktopStreamId :%s", resp.mStreamId.c_str());
         }
         else {
            LOGD_SDK("Publish mDesktopStream err:%s resp.mCode:%d", resp.mMsg.c_str(), resp.mCode);
            if (vhall::StreamPublished == resp.mCode || vhall::StreamIsEmpty == resp.mCode) {
               return;
            }
            CTaskEvent roomConnectTask;
            roomConnectTask.mEventType = TaskEventEnum_STREAM_PUSH_ERROR;
            roomConnectTask.mStreamType = vlive::VHStreamType_Desktop;
            roomConnectTask.mErrorType = resp.mCode;
            roomConnectTask.mErrMsg = resp.mMsg;
            PostTaskEvent(roomConnectTask);
         }
      }, SDK_TIMEOUT);
   }
   else {
      LOGD_SDK("VhallLive_NO_OBJ");
      return vlive::VhallLive_NO_OBJ;
   }
   LOGD_SDK("");
   return vlive::VhallLive_OK;
}

/*停止桌面共享采集推流*/
int WebRtcSDK::StopPushDesktopStream() {
   LOGD_SDK("");
   if (!mbIsWebRtcRoomConnected) {
      LOGD_SDK("VhallLive_ROOM_DISCONNECT");
      return vlive::VhallLive_ROOM_DISCONNECT;
   }
   LOGD_SDK("mDesktopStreamThreadLock");
   CThreadLockHandle lockHandle(&mDesktopStreamThreadLock);
   if (mDesktopStream && mLiveRoom) {
      LOGD_SDK("UnPublish mDesktopStream");
      if (mDesktopStream->GetStreamState() == VhallStreamState::Publish) {
         LOGD_SDK("mDesktopStream->GetStreamState() == VhallStreamState::Publish");
         mLiveRoom->UnPublish(mDesktopStream, [&](const vhall::RespMsg& resp) -> void {
            mDesktopStreamId.clear();
            LOGD_SDK("UnPublish VHStreamType_Desktop msg:%s", resp.mMsg.c_str());
            if (mWebRtcSDKEventReciver) {
               if (resp.mResult == SDK_SUCCESS) {
                  mWebRtcSDKEventReciver->OnStopPushStreamCallback(vlive::VHStreamType_Desktop, vlive::VhallLive_OK, resp.mMsg);
               }
               else {
                  mWebRtcSDKEventReciver->OnStopPushStreamCallback(vlive::VHStreamType_Desktop, vlive::VhallLive_ERROR, resp.mMsg);
               }
            }
         }, SDK_TIMEOUT);
         mDesktopStream->SetStreamState(UnPublish);
      }
      else {
         LOGD_SDK("mDesktopStream->GetStreamState() != VhallStreamState::Publish");
      }
   }
   else {
      LOGD_SDK("VhallLive_NO_OBJ");
      return vlive::VhallLive_NO_OBJ;
   }
   LOGD_SDK("end");
   return vlive::VhallLive_OK;
}

/*停止软件共享采集推流*/
int WebRtcSDK::StopPushSoftWareStream() {
   LOGD_SDK("");
   if (!mbIsWebRtcRoomConnected) {
      LOGD_SDK("VhallLive_ROOM_DISCONNECT");
      return vlive::VhallLive_ROOM_DISCONNECT;
   }
   LOGD_SDK("mSoftwareStreamThreadLock");
   CThreadLockHandle lockHandle(&mSoftwareStreamThreadLock);
   if (mSoftwareStream && mLiveRoom) {
      LOGD_SDK("UnPublish mSoftwareStream");
      if (mSoftwareStream->GetStreamState() == VhallStreamState::Publish) {
         LOGD_SDK("mSoftwareStream->GetStreamState() == VhallStreamState::Publish");
         mLiveRoom->UnPublish(mSoftwareStream, [&](const vhall::RespMsg& resp) -> void {
            mSoftwareStreamId.clear();
            LOGD_SDK("UnPublish VHStreamType_Desktop msg:%s", resp.mMsg.c_str());
            if (mWebRtcSDKEventReciver) {
               if (resp.mResult == SDK_SUCCESS) {
                  mWebRtcSDKEventReciver->OnStopPushStreamCallback(vlive::VHStreamType_SoftWare, vlive::VhallLive_OK, resp.mMsg);
               }
               else {
                  mWebRtcSDKEventReciver->OnStopPushStreamCallback(vlive::VHStreamType_SoftWare, vlive::VhallLive_ERROR, resp.mMsg);
               }
            }
         }, SDK_TIMEOUT);
         mSoftwareStream->SetStreamState(UnPublish);
      }
      else {
         LOGD_SDK("mSoftwareStream->GetStreamState() != VhallStreamState::Publish");
      }
   }
   else {
      LOGD_SDK("VhallLive_NO_OBJ");
      return vlive::VhallLive_NO_OBJ;
   }
   LOGD_SDK("end");
   return vlive::VhallLive_OK;
}

bool WebRtcSDK::IsPushingStream(vlive::VHStreamType streamType) {
   switch (streamType)
   {
   case vlive::VHStreamType_AVCapture: {
      CThreadLockHandle lockHandle(&mLocalStreamThreadLock);
      LOGD_SDK("mLocalStreamThreadLock");
      if (mLocalStream && mLocalStream->GetStreamState() == VhallStreamState::Publish) {
         LOGD_SDK("VHStreamType_AVCapture is publishing stream");
         return true;
      }
      else {
         LOGD_SDK("VHStreamType_AVCapture is not publish stream");
         return false;
      }
      break;
   }
   case vlive::VHStreamType_Desktop: {
      LOGD_SDK("mDesktopStreamThreadLock");
      CThreadLockHandle lockHandle(&mDesktopStreamThreadLock);
      if (mDesktopStream && mDesktopStream->GetStreamState() == VhallStreamState::Publish) {
         LOGD_SDK("VHStreamType_Desktop is publishing stream");
         return true;
      }
      else {
         LOGD_SDK("VHStreamType_Desktop is not publish stream");
         return false;
      }
      break;
   }          
   case vlive::VHStreamType_SoftWare: {
      LOGD_SDK("mSoftwareStreamThreadLock");
      CThreadLockHandle lockHandle(&mSoftwareStreamThreadLock);
      if (mSoftwareStream && mSoftwareStream->GetStreamState() == VhallStreamState::Publish) {
         LOGD_SDK("VHStreamType_Desktop is publishing stream");
         return true;
      }
      else {
         LOGD_SDK("VHStreamType_Desktop is not publish stream");
         return false;
      }
      break;
   }              
   case vlive::VHStreamType_MediaFile: {
      LOGD_SDK("mMediaStreamThreadLock");
      CThreadLockHandle lockHandle(&mMediaStreamThreadLock);
      if (mMediaStream && mMediaStream->GetStreamState() == VhallStreamState::Publish) {
         LOGD_SDK("VHStreamType_MediaFile is publishing stream");
         return true;
      }
      else {
         LOGD_SDK("VHStreamType_MediaFile is not publish stream");
         return false;
      }   
      break;
   }                       
   default:
      break;
   }
   return false;
}

std::string WebRtcSDK::GetUserStreamID(const std::wstring user_id, VHStreamType streamType) {
   LOGD_SDK("%s", __FUNCTION__);
   return mSubScribeStreamManager.GetSubScribeStreamId(user_id, streamType);
}

bool WebRtcSDK::IsSupported(const std::string devGuid, VideoProfileIndex index) {
   vhall::VHTools tl;
   const std::vector<std::shared_ptr<vhall::VideoDevProperty>> CameraList = tl.VideoInDeviceList();
   for (int i = 0; i < CameraList.size(); i++) {
      std::shared_ptr<vhall::VideoDevProperty> videoDev = CameraList.at(i);
      if (devGuid == videoDev->mDevId) {
         if (tl.IsSupported(videoDev, index)) {
            LOGD_SDK("mCurrentCameraIndex VhallLive_SUPPROT id:%s", devGuid.c_str());
            return true;
         }
         else {
            LOGD_SDK("mCurrentCameraIndex VhallLive_NOT_SUPPROT id:%s", devGuid.c_str());
            return false;
         }
      }
   }
   LOGD_SDK("not device");
   return false;
}

bool WebRtcSDK::IsCapturingStream(vlive::VHStreamType streamType) {
   switch (streamType)
   {
   case vlive::VHStreamType_AVCapture: {
      CThreadLockHandle lockHandle(&mLocalStreamThreadLock);
      LOGD_SDK("mLocalStreamThreadLock");
      if (mLocalStream) {
         LOGD_SDK("VHStreamType_AVCapture is capture stream %d",(int)(mLocalStreamIsCapture));
         return mLocalStreamIsCapture;
      }
      else {
         LOGD_SDK("VHStreamType_AVCapture is not capture stream");
         return false;
      }
      break;
   }
   case vlive::VHStreamType_Desktop: {
      LOGD_SDK("mDesktopStreamThreadLock");
      CThreadLockHandle lockHandle(&mDesktopStreamThreadLock);
      if (mDesktopStream) {
         LOGD_SDK("VHStreamType_Desktop is capture stream %d", (int)(mDesktopStreamIsCapture));
         return  mDesktopStreamIsCapture;
      }
      else {
         LOGD_SDK("VHStreamType_Desktop is not capture stream");
         return false;
      }
      break;
   }
   case vlive::VHStreamType_SoftWare: {
      LOGD_SDK("mDesktopStreamThreadLock");
      CThreadLockHandle lockHandle(&mSoftwareStreamThreadLock);
      if (mSoftwareStream) {
         LOGD_SDK("VHStreamType_Desktop is capture stream", (int)(mSoftWareStreamIsCapture));
         return  mSoftWareStreamIsCapture;
      }
      else {
         LOGD_SDK("VHStreamType_Desktop is not capture stream");
         return false;
      }
      break;
   }
   case vlive::VHStreamType_MediaFile: {
      LOGD_SDK("mMediaStreamThreadLock");
      CThreadLockHandle lockHandle(&mMediaStreamThreadLock);
      if (mMediaStream) {
         LOGD_SDK("VHStreamType_MediaFile is capture stream %d",(int)(mMediaStreamIsCapture));
         return mMediaStreamIsCapture;
      }
      else {
         LOGD_SDK("VHStreamType_MediaFile is not capture stream");
         return false;
      }
      break;
   }
   default:
      break;
   }
   return false;
}

bool WebRtcSDK::IsSupportMediaFormat(CaptureStreamAVType type) {
   if (mMediaStream) {
      if (type == CaptureStreamAVType::CaptureStreamAVType_Audio) {
         return mMediaStream->HasAudio();
      }
      else if (type == CaptureStreamAVType::CaptureStreamAVType_Video) {
         return mMediaStream->HasVideo();
      }
   }
   return false;
}

bool WebRtcSDK::IsPlayFileHasVideo() {
   LOGD_SDK("IsPlayFileHasVideo");
   CThreadLockHandle lockHandle(&mMediaStreamThreadLock);
   LOGD_SDK("mediaStreamConfig.mVideo %d", mediaStreamConfig.mVideo);
   return mediaStreamConfig.mVideo;
}

int WebRtcSDK::PrepareMediaFileCapture(VideoProfileIndex profileIndex) {
   LOGD_SDK("mMediaStreamThreadLock");
   CThreadLockHandle lockHandle(&mMediaStreamThreadLock);
   LOGD_SDK("enter");
   bool bChangeProfile = false;
   if (profileIndex != mediaStreamConfig.videoOpt.mProfileIndex) {
      bChangeProfile = true;
   }
   mediaStreamConfig.mAudio = true;
   mediaStreamConfig.mVideo = true;
   mediaStreamConfig.mStreamType = vlive::VHStreamType_MediaFile;
   mediaStreamConfig.videoOpt.mProfileIndex = profileIndex;
   mediaStreamConfig.videoOpt.lockRatio = true;
   mMediaStreamIsCapture = true;
   bool bIsInit = false;
   if (mMediaStream == nullptr) {
      bIsInit = true;
      mMediaStream.reset(new vhall::VHStream(mediaStreamConfig));
      mMediaStream->SetEventCallBack(OnPlayMediaFileCb, this);
      mMediaStream->mStreamAttributes = mWebRtcRoomOption.userData;
      //开启插播流
      mMediaStream->Init();
      //当调用插播成功。
      mMediaStream->AddEventListener(ACCESS_ACCEPTED, [&](vhall::BaseEvent &Event)->void {
         CTaskEvent task;
         task.mEventType = TaskEventEnum_CaptureMediaFile;
         task.mErrMsg = ACCESS_ACCEPTED;
         task.mSeekPos = 0;
         PostTaskEvent(task);
      });
      // 获取本地插播失败监听回调
      mMediaStream->AddEventListener(ACCESS_DENIED, [&](vhall::BaseEvent &Event)->void {
         CTaskEvent task;
         task.mEventType = TaskEventEnum_CaptureMediaFile;
         task.mErrMsg = ACCESS_DENIED;
         PostTaskEvent(task);
      });
   }

   if (mMediaStream) {
      if (bChangeProfile) {
         mMediaStream->ResetCapability(mediaStreamConfig.videoOpt);
      }
      mMediaStream->Getlocalstream();
   }
   else {
      LOGD_SDK("VhallLive_NO_OBJ");
      return vlive::VhallLive_NO_OBJ;
   }
   LOGD_SDK("");
   return vlive::VhallLive_OK;
}

void WebRtcSDK::OnPlayMediaFileCb(int code, void *param) {
   if (param) {
      LOGD_SDK("code :%d", code);
   }
}

void WebRtcSDK::CheckAssistFunction(int type, bool streamRemove) {
   if (type == vlive::VHStreamType_Desktop || type == vlive::VHStreamType_MediaFile) {
      if (streamRemove) {
         mbIsEnablePlayFileAndShare = true;
      }
      else {
         mbIsEnablePlayFileAndShare = false;
      }
   }
}

void WebRtcSDK::PlayFileCallback(int code) {
   if (mWebRtcSDKEventReciver) {
      LOGD_SDK("vlive::VHCapture_ACCESS_DENIED");
      mWebRtcSDKEventReciver->OnOpenCaptureCallback(vlive::VHStreamType_MediaFile, vlive::VHCapture_ACCESS_DENIED, mediaStreamConfig.mVideo, mediaStreamConfig.mAudio);
   }
}

int WebRtcSDK::GetPlayMeidaFileWidthAndHeight(std::string filePath, int& srcWidth, int &srcHeight) {
   vhall::FFmpegDemuxer ffmpegFileDemuxer;
   int nRet = ffmpegFileDemuxer.Init(filePath.c_str());
   if (nRet == 0) {
      if (ffmpegFileDemuxer.GetVideoPar() != NULL) {
         srcWidth = ffmpegFileDemuxer.GetVideoPar()->width;
         srcHeight = ffmpegFileDemuxer.GetVideoPar()->height;
      }
   }
   return nRet;
}

int WebRtcSDK::InitMediaFile() {
   LOGD_SDK("enter ");
   if (!mbIsEnablePlayFileAndShare) {
      LOGD_SDK("mbIsEnablePlayFileAndShare VhallLive_MUTEX_OPERATE");
      return vlive::VhallLive_MUTEX_OPERATE;
   }
   if (mDesktopStreamIsCapture) {
      LOGD_SDK("mbIsEnablePlayFileAndShare VhallLive_MUTEX_OPERATE");
      return vlive::VhallLive_MUTEX_OPERATE;
   }
   return PrepareMediaFileCapture(RTC_VIDEO_PROFILE_720P_16x9_H);
}

/**
**  播放。推流成功后可以调用此接口进行文件播放
*/
bool WebRtcSDK::PlayFile(std::string file, VideoProfileIndex profileIndex) {
   LOGD_SDK("%s mMediaStreamThreadLock file %s",__FUNCTION__,file.c_str());
   CTaskEvent task;
   task.mEventType = TaskEventEnum_StartVLCCapture;
   task.mSleepTime = 1000;
   task.mProfile = profileIndex;
   task.filePath = file;
   LOGD_SDK("TaskEventEnum_StartVLCCapture");
   PostTaskEvent(task);
   return true;
}

void WebRtcSDK::ChangeMediaFileProfile(VideoProfileIndex profileIndex) {
   LOGD_SDK("ChangeMediaFileProfile");
   CThreadLockHandle lockHandle(&mMediaStreamThreadLock);
   LOGD_SDK("enter");
   mediaStreamConfig.videoOpt.mProfileIndex = profileIndex;
   if (mMediaStream) {
      mMediaStream->ResetCapability(mediaStreamConfig.videoOpt);
   }
   LOGD_SDK("end");
}

/*停止插播文件*/
void WebRtcSDK::StopMediaFileCapture() {
   CTaskEvent task;
   task.mEventType = TaskEventEnum_StopMediaCapture;
   task.mSleepTime = 1000;
   LOGD_SDK("StopMediaFileCapture");
   PostTaskEvent(task);
}

/*开始插播文件推流*/
int WebRtcSDK::StartPushMediaFileStream() {
   if (!mbIsEnablePlayFileAndShare) {
      LOGD_SDK("VhallLive_MUTEX_OPERATE");
      return vlive::VhallLive_MUTEX_OPERATE;
   }
   LOGD_SDK("mMediaStreamThreadLock");
   CThreadLockHandle lockHandle(&mMediaStreamThreadLock);
   if (!mbIsWebRtcRoomConnected) {
      LOGD_SDK("VhallLive_ROOM_DISCONNECT");
      return vlive::VhallLive_ROOM_DISCONNECT;
   }
   if (mLiveRoom && mMediaStream) {
      bool hasVideo = mMediaStream->HasVideo();
      bool hasAudio = mMediaStream->HasAudio();
      std::string join_uid = SubScribeStreamManager::WString2String(mWebRtcRoomOption.strUserId);
      mLiveRoom->Publish(mMediaStream, [&, hasVideo, hasAudio, join_uid](const vhall::PublishRespMsg& resp) -> void {
         if (resp.mResult == SDK_SUCCESS) {
            LOGD_SDK("Publish mMediaStream success %s", resp.mStreamId.c_str());
            //互动插播业务需要注意的实现和其他摄像头和桌面共享画面有些许区别。需要等待推流成功之后监听到Stream_mixed消息才能进行画面播放。
            //原因是如果提前播放文件，之后操作推流有可能在混流观看端会缺失推流成功前的几秒视频画面数据。
            //http://wiki.vhallops.com/pages/viewpage.action?pageId=113410413i
         }
         else {
            LOGD_SDK("Publish mMediaStream error:%d msg:%s", resp.mCode, resp.mMsg.c_str());
            if (vhall::StreamPublished == resp.mCode || vhall::StreamIsEmpty == resp.mCode) {
               return;
            }
            CTaskEvent roomConnectTask;
            roomConnectTask.mEventType = TaskEventEnum_STREAM_PUSH_ERROR;
            roomConnectTask.mStreamType = vlive::VHStreamType_MediaFile;
            roomConnectTask.mErrorType = resp.mCode;
            roomConnectTask.mErrMsg = resp.mMsg;
            PostTaskEvent(roomConnectTask);
         }
      }, SDK_TIMEOUT);
   }
   else {
      LOGD_SDK("VhallLive_NO_OBJ");
      return vlive::VhallLive_NO_OBJ;
   }
   LOGD_SDK("");
   return vlive::VhallLive_OK;
}

/*停止插播文件推流*/
void WebRtcSDK::StopPushMediaFileStream() {
   LOGD_SDK("mMediaStreamThreadLock");
   CThreadLockHandle lockHandle(&mMediaStreamThreadLock);
   if (mLiveRoom && mMediaStream) {
      LOGD_SDK("UnPublish mMediaStream");
      if (mMediaStream->mState == VhallStreamState::Publish) {
         LOGD_SDK("mMediaStream->mState == VhallStreamState::Publish");
         mLiveRoom->UnPublish(mMediaStream, [&](const vhall::RespMsg& resp) -> void {
            LOGD_SDK("UnPublish mMediaStream msg:%s", resp.mMsg.c_str());
            mMediaFileStreamId.clear();
            if (mWebRtcSDKEventReciver) {
               if (resp.mResult == SDK_SUCCESS) {
                  mWebRtcSDKEventReciver->OnStopPushStreamCallback(vlive::VHStreamType_MediaFile, vlive::VhallLive_OK, resp.mMsg);
               }
               else {
                  mWebRtcSDKEventReciver->OnStopPushStreamCallback(vlive::VHStreamType_MediaFile, vlive::VhallLive_ERROR, resp.mMsg);
               }
            }
         }, SDK_TIMEOUT);
         mMediaStream->SetStreamState(UnPublish);
      }
      else {
         LOGD_SDK("mMediaStream->mState != VhallStreamState::Publish");
      }
   }
}

//插播文件暂停处理
void WebRtcSDK::MediaFilePause() {
   LOGD_SDK(" MediaFilePause");
   CThreadLockHandle lockHandle(&mMediaStreamThreadLock);
   if (mMediaStream) {
      LOGD_SDK("FilePause");
      mMediaStream->FilePause();
   }
}

//插播文件恢复处理
void WebRtcSDK::MediaFileResume() {
   LOGD_SDK(" MediaFileResume");
   CThreadLockHandle lockHandle(&mMediaStreamThreadLock);
   if (mMediaStream) {
      LOGD_SDK("FileResume");
      mMediaStream->FileResume();
   }
}

//插播文件快进处理
void WebRtcSDK::MediaFileSeek(const unsigned long long& seekTimeInMs) {
   LOGD_SDK(" MediaFileSeek seekTimeInMs:%lld", seekTimeInMs);
   CThreadLockHandle lockHandle(&mMediaStreamThreadLock);
   if (mMediaStream) {
      LOGD_SDK("FileSeek");
      mMediaStream->FileSeek(seekTimeInMs);
   }
}

int WebRtcSDK::MediaFileVolumeChange(int vol) {
   if (vol < 0 || vol > 100) {
      return VhallLive_PARAM_ERR;
   }
   CThreadLockHandle lockHandle(&mMediaStreamThreadLock);
   if (mMediaStream) {
      return mMediaStream->SetFileVolume(vol);
   }
   else {
      return VhallLive_NO_OBJ;
   }
}

//插播文件总时长
const long long WebRtcSDK::MediaFileGetMaxDuration() {
   CThreadLockHandle lockHandle(&mMediaStreamThreadLock);
   if (mMediaStream) {
      return mMediaStream->FileGetMaxDuration();
   }
   return -1;
}

//插播文件当前时长
const long long WebRtcSDK::MediaFileGetCurrentDuration() {
   CThreadLockHandle lockHandle(&mMediaStreamThreadLock);
   if (mMediaStream) {
      return mMediaStream->FileGetCurrentDuration();
   }
   return -1;
}

//插播文件的当前状态
int WebRtcSDK::MediaGetPlayerState() {
   CThreadLockHandle lockHandle(&mMediaStreamThreadLock);
   if (mMediaStream) {
      return mMediaStream->GetPlayerState();
   }
   return -1;
}

int WebRtcSDK::GetCameraDevices(std::list<vhall::VideoDevProperty> &cameras) {
   return DevicesTool::EnumCameraDevices(cameras);
}

/**获取麦克风列表**/
int WebRtcSDK::GetMicDevices(std::list<vhall::AudioDevProperty> &micDevList) {
   return DevicesTool::EnumMicDevices(micDevList);
}

/**获取扬声器列表**/
int WebRtcSDK::GetPlayerDevices(std::list<vhall::AudioDevProperty> &playerDevList) {
   return DevicesTool::EnumPlayerDevices(playerDevList);
}

/*
*    获取窗口共享和桌面共享列表
**  vlive::VHStreamType      // 3:Screen,5:window
*/
std::vector<DesktopCaptureSource> WebRtcSDK::GetDesktops(int streamType) {
   return DevicesTool::EnumDesktops(streamType);
}

/*
*  是否存在音频或视频设备。
*  返回值：只要存在一类设备 返回true, 如果音视频设备都没有则返回false
**/
bool WebRtcSDK::HasVideoOrAudioDev() {
   if (DevicesTool::HasAudioDev() || DevicesTool::HasVideoDev()) {
      return true;
   }
   return false;
}

/*
*  是否存在视频设备。
*  返回值：只要存在一类设备 返回true, 如果音视频设备都没有则返回false
**/
bool WebRtcSDK::HasVideoDev() {
   return DevicesTool::HasVideoDev();
}

/*
*  是否存在音频设备。
*  返回值：只要存在一类设备 返回true, 如果音视频设备都没有则返回false
**/
bool WebRtcSDK::HasAudioDev() {
   return DevicesTool::HasAudioDev();
}

int WebRtcSDK::SetUsedMic(int micIndex, std::string micDevId, std::wstring desktopCaptureId) {
   LOGD_SDK("SetUsedMic:%d devId:%s", micIndex, micDevId.c_str());
   CTaskEvent task;
   task.mEventType = TaskEventEnum_SET_MIC_DEV;
   task.devId = micDevId;
   task.devIndex = micIndex;
   task.desktopCaptureId = desktopCaptureId;
   PostTaskEvent(task);

   mCurrentMicIndex = micIndex;
   mCurrentMicDevid = micDevId;
   mDesktopCaptureId = desktopCaptureId;
   LOGD_SDK("VhallLive_OK");
   return vlive::VhallLive_OK;
}

void WebRtcSDK::GetCurrentMic(int &index, std::string& devId) {
   index = mCurrentMicIndex;
   devId = mCurrentMicDevid;
}

std::wstring WebRtcSDK::GetCurrentDesktopPlayCaptureId() {
   return mDesktopCaptureId;
}

void WebRtcSDK::GetCurrentCamera(std::string& devId) {
   devId = mCurrentCameraGuid;
}

int WebRtcSDK::SetUsedPlay(int index, std::string devId) {
   LOGD_SDK("SetUsedPlay:%d devId:%s  mCurrentPlayerDevid:%s", index, devId.c_str(), mCurrentPlayerDevid.c_str());
   if (mCurrentPlayerDevid != devId) {
      vhall::VHStream::SetAudioOutDevice(index);
   }
   mCurrentPlayerIndex = index;
   mCurrentPlayerDevid = devId;
   return vlive::VhallLive_OK;
}

void WebRtcSDK::GetCurrentPlayOut(int &index, std::string& devId) {
   index = mCurrentPlayerIndex;
   devId = mCurrentPlayerDevid;
}

int WebRtcSDK::SetCurrentMicVol(int vol) {
   CThreadLockHandle lockHandle(&mLocalStreamThreadLock);
   LOGD_SDK("mLocalStreamThreadLock");
   vhall::VHStream::SetMicrophoneVolume(vol);
   LOGD_SDK(" SetCurrentMicVol:%d", vol);
   mCurMicVol = vol;
   return vlive::VhallLive_OK;
}

int WebRtcSDK::SetCurrentPlayVol(int vol) {
   mCurPlayVol = vol;
   vhall::VHStream::SetSpeakerVolume(vol);
   LOGD_SDK("SetCurrentPlayVol :%d", (int)mCurPlayVol);
   return 0;
}

int WebRtcSDK::GetCurrentMicVol() {
   LOGD_SDK("GetCurrentMicVol");
   return mCurMicVol;
}

int WebRtcSDK::GetCurrentPlayVol() {
   LOGD_SDK("GetCurrentPlayVol");
   return mCurPlayVol;
}

//打开、关闭远端用户本地视频
int WebRtcSDK::OperateRemoteUserVideo(std::wstring user_id, bool close/* = false*/) {
   return mSubScribeStreamManager.OperateRemoteUserVideo(user_id, close);
}
//打开、关闭远端用户本地声音
int WebRtcSDK::OperateRemoteUserAudio(std::wstring user_id, bool close /*= false*/) {
   return mSubScribeStreamManager.OperateRemoteUserAudio(user_id, close);
}

//关闭摄像头
bool WebRtcSDK::CloseCamera() {
   LOGD_SDK("mLocalStreamThreadLock");
   CThreadLockHandle lockHandle(&mLocalStreamThreadLock);
   LOGD_SDK(" CloseCamera");
   if (mLocalStream && mLocalStreamIsCapture && (mLocalStream->mStreamType == vlive::VHStreamType_AVCapture)) {
      mLocalStream->MuteVideo(true);
      mLocalStream->stop();
      mbIsOpenCamera = false;
      LOGD_SDK(" CloseCamera end");
      return true;
   }
   LOGD_SDK(" CloseCamera end");
   return false;
}

//打开摄像头
bool WebRtcSDK::OpenCamera() {
   LOGD_SDK("mLocalStreamThreadLock");
   CThreadLockHandle lockHandle(&mLocalStreamThreadLock);
   LOGD_SDK("OpenCamera");
   if (mLocalStream && mLocalStreamIsCapture && (mLocalStream->mStreamType == vlive::VHStreamType_AVCapture)) {
      LOGD_SDK("Camera is open");
      mLocalStream->MuteVideo(false);
      mbIsOpenCamera = true;
      if (mLocalCameraRecv) {
         mLocalStream->play(mLocalCameraRecv);
      }
      else if (mLocalStreamWnd) {
         mLocalStream->play(mLocalStreamWnd);
      }
      return true;
   }
   return false;
}

bool WebRtcSDK::IsCameraOpen() {
   if (mLocalStream && mLocalStreamIsCapture && (mLocalStream->mStreamType == vlive::VHStreamType_AVCapture)) {
      LOGD_SDK("camera is open");
      return mbIsOpenCamera;
   }
   LOGD_SDK("camera is not open");
   return false;
}

bool WebRtcSDK::CloseMic() {
   LOGD_SDK("SetMicrophoneVolume 0");
   if (mLocalStream && mDesktopCaptureId.length() <= 0) {
      LOGD_SDK("MuteAudio");
      mLocalStream->MuteAudio(true);
   }
   vhall::VHStream::SetMicrophoneVolume(0);
   mbIsOpenMic = false;
   return true;
}
//打开麦克风
bool WebRtcSDK::OpenMic() {
   LOGD_SDK("open mic :%d", (int)mCurMicVol);
   if (mLocalStream) {
      mLocalStream->MuteAudio(false);
   }
   vhall::VHStream::SetMicrophoneVolume(mCurMicVol);
   mbIsOpenMic = true;
   return true;
}

bool WebRtcSDK::IsMicOpen() {
   if (mLocalStream && mLocalStreamIsCapture && (mLocalStream->mStreamType == vlive::VHStreamType_AVCapture)) {
      LOGD_SDK("mic is open");
      return mbIsOpenMic;
   }
   return false;
}

/**
*   开始渲染"本地"摄像设备、桌面共享、文件插播媒体数据流.
*/
bool WebRtcSDK::StartRenderLocalStream(vlive::VHStreamType streamType, void* wnd) {
   LOGD_SDK("StartRenderStream");
   if (streamType == vlive::VHStreamType_AVCapture) {
      CThreadLockHandle lockHandle(&mLocalStreamThreadLock);
      LOGD_SDK("mLocalStreamThreadLock");
      if (mLocalStream && mLocalStream->HasVideo()) {
         LOGD_SDK("play mLocalStream");
         mLocalStream->stop();
         HWND playWnd = (HWND)(wnd);
         mLocalStreamWnd = playWnd;
         mLocalStream->play(playWnd);
         LOGD_SDK("play mLocalStream end");
      }
      return true;
   }
   else if (streamType == vlive::VHStreamType_Desktop) {
      LOGD_SDK("mDesktopStreamThreadLock");
      CThreadLockHandle lockHandle(&mDesktopStreamThreadLock);
      if (mDesktopStream) {
         LOGD_SDK("play local mDesktopStream");
         mDesktopStream->stop();
         HWND playWnd = (HWND)(wnd);
         mDesktopStream->play(playWnd);
         LOGD_SDK("play local mDesktopStream end");
         return true;
      }
   }
   else if (streamType == vlive::VHStreamType_MediaFile) {
      LOGD_SDK("mMediaStreamThreadLock");
      CThreadLockHandle lockHandle(&mMediaStreamThreadLock);
      if (mMediaStream) {
         LOGD_SDK("play local mMediaStream");
         mMediaStream->stop();
         HWND playWnd = (HWND)(wnd);
         mMediaStream->play(playWnd);
         LOGD_SDK("play local mMediaStream end");
         return true;
      }
   }
   else if (streamType == vlive::VHStreamType_SoftWare) {
      LOGD_SDK("mDesktopStreamThreadLock");
      CThreadLockHandle lockHandle(&mSoftwareStreamThreadLock);
      if (mSoftwareStream) {
         LOGD_SDK("play local mDesktopStream");
         mSoftwareStream->stop();
         HWND playWnd = (HWND)(wnd);
         mSoftwareStream->play(playWnd);
         LOGD_SDK("play local mDesktopStream end");
         return true;
      }
   }
   else if (streamType == vlive::VHStreamType_Auxiliary_CamerCapture) {
      LOGD_SDK("mLocalAuxiliaryStreamThreadLock");
      CThreadLockHandle lockHandle(&mLocalAuxiliaryStreamThreadLock);
      if (mLocalAuxiliaryStream) {
         LOGD_SDK("play local mLocalAuxiliaryStream");
         mLocalAuxiliaryStream->stop();
         HWND playWnd = (HWND)(wnd);
         mLocalAuxiliaryStream->play(playWnd);
         LOGD_SDK("play local mLocalAuxiliaryStream end");
         return true;
      }
   }
   return false;
}

bool WebRtcSDK::StartRenderRemoteStream(const std::wstring& user, vlive::VHStreamType streamType, std::shared_ptr<vhall::VideoRenderReceiveInterface> recv) {
   LOGD_SDK("%s", __FUNCTION__);
   std::string mediaType = std::to_string(streamType);
   std::wstring subStreamIndex = user + SubScribeStreamManager::String2WString(mediaType);
   if (!mSubScribeStreamManager.StartRenderStream(subStreamIndex, recv)) {
      mediaType = std::to_string(VHStreamType_Preview_Video);
      subStreamIndex = user + SubScribeStreamManager::String2WString(mediaType);
      return mSubScribeStreamManager.StartRenderStream(subStreamIndex, recv);
   }
   return false;
}

/**
*   开始渲染"远端"摄像设备、桌面共享、文件插播媒体数据流.
*/
bool WebRtcSDK::StartRenderRemoteStream(const std::wstring& user, vlive::VHStreamType streamType, void* wnd) {
   LOGD_SDK("%s", __FUNCTION__);
   std::string mediaType = std::to_string(streamType);
   std::wstring subStreamIndex = user + SubScribeStreamManager::String2WString(mediaType);
   if (!mSubScribeStreamManager.StartRenderStream(subStreamIndex, wnd)) {
      mediaType = std::to_string(VHStreamType_Preview_Video);
      subStreamIndex = user + SubScribeStreamManager::String2WString(mediaType);
      return mSubScribeStreamManager.StartRenderStream(subStreamIndex, wnd);
   }
   return false;
}

/*
*   停止渲染远端流
*/
bool WebRtcSDK::StopRenderRemoteStream(const std::wstring& user, const std::string streamId, vlive::VHStreamType streamType) {
   LOGD_SDK("mSubScribeStreamMutex");
   std::string mediaType = std::to_string(streamType);
   std::wstring subStreamIndex = user + SubScribeStreamManager::String2WString(mediaType);
   return mSubScribeStreamManager.StopRenderRemoteStream(subStreamIndex);
}

void WebRtcSDK::StopRecvAllRemoteStream() {
   mSubScribeStreamManager.StopSubScribeStream();
}

bool WebRtcSDK::RemoveSubScribeStreamObj(const std::wstring &subStreamIndex, std::string stream_id) {
   return mSubScribeStreamManager.RemoveSubScribeStreamObj(subStreamIndex, stream_id);
}

int WebRtcSDK::GetSubScribeStreamFormat(const std::wstring& user, vlive::VHStreamType streamType, bool &hasVideo, bool &hasAudio) {
   return mSubScribeStreamManager.GetSubScribeStreamFormat(user, streamType, hasVideo, hasAudio);
}

std::string WebRtcSDK::GetSubScribeStreamId(const std::wstring& user, vlive::VHStreamType streamType) {
   LOGD_SDK("");
   return mSubScribeStreamManager.GetSubScribeStreamId(user, streamType);
}

void WebRtcSDK::HandleRePublished(int type, std::wstring& user_id, std::string& new_streamId) {
   switch (type)
   {
   case vlive::VHStreamType_AVCapture: {
      if (mWebRtcSDKEventReciver) {
         mWebRtcSDKEventReciver->OnRePublishStreamIDChanged((vlive::VHStreamType)type, user_id, mLocalStreamId, new_streamId);
         LOGD_SDK("type %d new_streamId:%s mLocalStreamId:%s", type, new_streamId.c_str(), mLocalStreamId.c_str());
         mLocalStreamId = new_streamId;
      }
      break;
   }                     
   case vlive::VHStreamType_Desktop: {
      if (mWebRtcSDKEventReciver) {
         mWebRtcSDKEventReciver->OnRePublishStreamIDChanged(vlive::VHStreamType_Desktop, user_id, mDesktopStreamId, new_streamId);
         LOGD_SDK("type %d new_streamId:%s mDesktopStreamId:%s", type, new_streamId.c_str(), mDesktopStreamId.c_str());
         mDesktopStreamId = new_streamId;
      }
      break;
   }       
   case vlive::VHStreamType_MediaFile: {
      if (mWebRtcSDKEventReciver) {
         mWebRtcSDKEventReciver->OnRePublishStreamIDChanged(vlive::VHStreamType_MediaFile, user_id, mMediaFileStreamId, new_streamId);
         LOGD_SDK("type %d new_streamId:%s mMediaFileStreamId:%s", type, new_streamId.c_str(), mMediaFileStreamId.c_str());
         mMediaFileStreamId = new_streamId;
      }
      break;
   }            
   default:
      break;
   }
}

DWORD WINAPI WebRtcSDK::MediaCoreThreadProcess(LPVOID p) {
   if (p) {
      WebRtcSDK* sdk = (WebRtcSDK*)(p);
      if (sdk) {
         sdk->FastInitMediaCore();
      }
   }
   return 0;
}

DWORD WINAPI WebRtcSDK::ThreadProcess(LPVOID p) {
   LOGD_SDK("ThreadProcess");
   while (bThreadRun) {
      if (gTaskEvent) {
         DWORD ret = WaitForSingleObject(gTaskEvent, 1000);
         if (p) {
            WebRtcSDK* sdk = (WebRtcSDK*)(p);
            if (sdk) {
               sdk->ProcessTask();
            }
         }
      }
   }
   if (gThreadExitEvent) {
      ::SetEvent(gThreadExitEvent);
   }
   LOGD_SDK("exit ThreadProcess");
   return 0;
}

void WebRtcSDK::ProcessTask() {
   CTaskEvent event;
   {
      CThreadLockHandle lockHandle(&taskListMutex);
      if (mtaskEventList.size() > 0) {
         event = mtaskEventList.front();
         mtaskEventList.pop_front();
         if (event.mEventType == TaskEventEnum_PreviewBeautyLevel) {
            std::list<CTaskEvent>::iterator iter = mtaskEventList.begin();
            while (iter != mtaskEventList.end()) {
               if ((*iter).mEventType == TaskEventEnum_PreviewBeautyLevel) {
                  event = (*iter);
                  iter = mtaskEventList.erase(iter);
               }
               else {
                  iter++;
               }
            }
         }
         else if (event.mEventType == TaskEventEnum_StartPreviewLocalCapture) {
            std::list<CTaskEvent>::iterator iter = mtaskEventList.begin();
            while (iter != mtaskEventList.end()) {
               if ((*iter).mEventType == TaskEventEnum_StartPreviewLocalCapture) {
                  event = (*iter);
                  iter = mtaskEventList.erase(iter);
               }
               else {
                  iter++;
               }
            }
         }
      }
      else {
         return;
      }
   }

   switch (event.mEventType)
   {
   case TaskEventEnum_ROOM_CONNECTED: {
      HandleRoomConnected(event);
      break;
   }
   case TaskEventEnum_RE_SUBSCRIB: {
      HandleRoomStreamSubScribe(event);
      break;
   }
   case TaskEventEnum_SUBSCRIB_ERROR: {
      HandleRoomStreamSubScribeError(event);
      break;
   }
   case TaskEventEnum_ROOM_ERROR: {
      HandleRoomError(event);
      break;
   }
   case TaskEventEnum_ON_STREAM_MIXED: {
      HandleOnStreamMixed(event);
      break;
   }
   case TaskEventEnum_STREAM_PUSH_ERROR: {
      HandleStreamPushError(event);
      break;
   }
   case TaskEventEnum_LocalCaptureBeautyLevel: {
      HandleLocalCaptureBeautyLevel(event);
      break;
   }
   case TaskEventEnum_PreviewBeautyLevel: {
      HandlePreviewBeautyLevel(event);
      break;
   }
   case TaskEventEnum_ROOM_RECONNECTING: {
      HandleRoomReConnecting(event);
      break;
   }
   case TaskEventEnum_ROOM_RECONNECTED: {
      HandleRoomReConnected(event);
      break;
   }
   case TaskEventEnum_ROOM_RECOVER: {
      HandleRoomRecover(event);
      break;
   }
   case TaskEventEnum_ROOM_NETWORKCHANGED: {
      HandleRoomNetworkChanged(event);
      break;
   }
   case TaskEventEnum_STREAM_REPUBLISHED: {
      HandleStreamRePublished(event);
      break;
   }
   case TaskEventEnum_SET_MIC_DEV: {
      HandleSetMicDev(event);
      break;
   }
   case TaskEventEnum_SET_PLAYER_DEV: {
      HandleSetPlayerDev(event);
      break;
   }
   case TaskEventEnum_STREAM_FAILED: {
      HandleStreamFailed(event);
      break;
   }
   case TaskEventEnum_STREAM_REMOVED: {
      HandleStreamRemoved(event);
      break;
   }
   case TaskEventEnum_STREAM_SUBSCRIBED_SUCCESS: {
      HandleStreamSubSuccess(event);
      break;
   }
   case TaskEventEnum_RECV_MEDIA_STREAM: {
      StopMediaFileCapture();
      StopDesktopCapture();
      break;
   }
   case TaskEventEnum_StopMediaCapture: {
      HandleStopMediaCapture();
      break;
   }
   case TaskEventEnum_StartVLCCapture: {
      HandleStartVLCCapture(event);
      break;
   }
   case TaskEventEnum_CaptureMediaFile: {
      HandleCaptureMediaFile(event);
      break;
   }
   case TaskEventEnum_ADD_STREAM: {
      HandleAddStream(event);
      break;
   }
   case TaskEventEnum_STREAM_RECV_LOST: {
      HandleRecvStreamLost(event);
      break;
   }
   case TaskEventEnum_CaptureSoftwareStream: {
      HandleCaptureSoftwareStream(event);
      break;
   }
   case TaskEventEnum_ROOM_INIT_MEDIA_READER: {

      break;
   }
   case TaskEventEnum_CaptureDesktopStream: {
      HandleCaptureDesktopStream(event);
      break;
   }
   case TaskEventEnum_StartLocalCapture: {
      HandleStartLocalCapture(event.devId, event.index, event.doublePush);
      break;
   }
   case TaskEventEnum_StartLocalAuxiliaryCapture: {
      HandleStartLocalAuxiliaryCapture(event.devId, event.index);
      break;
   }
   case TaskEventEnum_StartLocalPicCapture: {
      HandleStartLocalCapturePicture(event.filePath, event.index, event.doublePush);
      break;
   }
   case TaskEventEnum_CaptureLocalStream: {
      HandleCaptureLocalStream(event);
      break;
   }
   case TaskEventEnum_SET_MEIDA_VOL: {
      HandleSetPlayFileVol(event);
      break;
   }
   case TaskEventEnum_CaptureAuxiliaryLocalStream: {
      HandleCaptureAuxiliaryLocalStrea(event);
      break;
   }
   case TaskEventEnum_StartPreviewLocalCapture: {
      LOGD_SDK("TaskEventEnum_StartPreviewLocalCapture");
      HandlePreviewCamera(event.wnd, event.devId, event.index, event.mMicIndex);
      LOGD_SDK("TaskEventEnum_StartPreviewLocalCapture end");
      break;
   }
   case TaskEventEnum_PREVIE_PLAY: {
      HandlePrivew(event);
      break;
   }
   default:
      break;
   }
}

int WebRtcSDK::PlayAudioFile(std::string fileName, int devIndex) {
   vhall::AudioPlayDeviceModule* player = vhall::AudioPlayDeviceModule::GetInstance();
   if (player) {
      player->Stop();
   }
   int nRet = player->SetPlay(fileName, devIndex, 48000, 2);
   if (0 == nRet) {
      player->Start();
      player->SetAudioListen(&mPlayFileAudioVolume);
   }
   return nRet;
}

int WebRtcSDK::GetPlayAudioFileVolum() {
   return mPlayFileAudioVolume.GetAudioVolume();
}

void WebRtcSDK::FastInitMediaCore() {
   vhall::StreamMsg tempStreamForMediaReaderMsg;
   tempStreamForMediaReaderMsg.mStreamType = 4;
   std::shared_ptr<vhall::VHStream> tempStreamForMediaReader = std::make_shared<vhall::VHStream>(tempStreamForMediaReaderMsg);
   if (tempStreamForMediaReader) {
      tempStreamForMediaReader->Init();
      tempStreamForMediaReader->GetFileVolume();
      tempStreamForMediaReader.reset();
      tempStreamForMediaReader = nullptr;
   }
}

int WebRtcSDK::StopPlayAudioFile() {
   vhall::AudioPlayDeviceModule* player = vhall::AudioPlayDeviceModule::GetInstance();
   if (player) {
      player->Stop();
      vhall::AudioPlayDeviceModule::Release();
   }
   return 0;
}

/**
*  打开或关闭所有订阅的音频
*/
int WebRtcSDK::MuteAllSubScribeAudio(bool mute) {
   LOGD_SDK("MuteAllSubScribeAudio mMuteAllAudio:%d", mute);
   mMuteAllAudio = mute;
   mSubScribeStreamManager.MuteAllSubScribeAudio(mute);
   LOGD_SDK("MuteAllSubScribeAudio end");
   return 0;
}
bool WebRtcSDK::GetMuteAllAudio() {
   return mMuteAllAudio;
}
void  WebRtcSDK::RelaseStream(std::shared_ptr<vhall::VHStream>& stream) {
   if (stream) {
      stream->RemoveAllEventListener();
      LOGD_SDK("RemoveAllEventListener");
      stream->stop();
      LOGD_SDK("stop");
      stream->close();
      LOGD_SDK("close");
      stream->Destroy();
      stream.reset();
      LOGD_SDK("reset");
      stream = nullptr;
   }
}

void WebRtcSDK::PushFrontEvent(CTaskEvent task) {
   if (bThreadRun) {
      CThreadLockHandle lockHandle(&taskListMutex);
      mtaskEventList.push_front(task);
      if (gTaskEvent) {
         ::SetEvent(gTaskEvent);
      }
   }
}

void WebRtcSDK::PostTaskEvent(CTaskEvent task) {
   if (bThreadRun) {
      CThreadLockHandle lockHandle(&taskListMutex);
      mtaskEventList.push_back(task);
      if (gTaskEvent) {
         ::SetEvent(gTaskEvent);
      }
   }
}

void WebRtcSDK::HandleRoomConnected(CTaskEvent &task) {
   mbIsWebRtcRoomConnected = true;
   LOGD_SDK("mbIsWebRtcRoomConnected = true");
   mbIsWebRtcRoomConnected = true;
   if (mWebRtcSDKEventReciver) {
      mWebRtcSDKEventReciver->OnRtcRoomNetStateCallback(vlive::VHRoomConnect_ROOM_CONNECTED);
      LOGD_SDK("callback VHRoomConnect_ROOM_CONNECTED");
   }
}

void WebRtcSDK::HandleRoomStreamSubScribeError(CTaskEvent &event) {
   if (mWebRtcSDKEventReciver) {
      mWebRtcSDKEventReciver->OnSubScribeStreamErr(event.streamId, event.mErrMsg, event.mErrorType);
   }
}

void WebRtcSDK::HandleRoomStreamSubScribe(CTaskEvent &event) {
   LOGD_SDK("TaskEventEnum_RE_SUBSCRIB");
   CThreadLockHandle roomLockHandle(&mRoomThreadLock);
   if (mLiveRoom) {
      LOGD_SDK("TaskEventEnum_RE_SUBSCRIB mLiveRoom");
      //加入房间时判断是有用户进行插播或桌面共享
      CheckAssistFunction(event.mStreamType, false);
      std::string stream_id = event.streamId;
      LOGD_SDK("TaskEventEnum_RE_SUBSCRIB event.mErrorType  %d", event.mErrorType);
      if (mWebRtcSDKEventReciver &&  event.mErrorType == 1) {
         LOGD_SDK("TaskEventEnum_RE_SUBSCRIB event.mErrorType  %d  OnSubScribeStreamErr", event.mErrorType);
         Sleep(2000);
         mWebRtcSDKEventReciver->OnSubScribeStreamErr(event.streamId, event.mErrMsg, event.mErrorType, event.mJoinUid);
         LOGD_SDK("OnSubScribeStreamErr");
         return;
      }
      Sleep(1000);
      LOGD_SDK("TaskEventEnum_RE_SUBSCRIB Subscribe stream_id:%s event.mSleepTime:%d ",  stream_id.c_str(), event.mSleepTime);
      mLiveRoom->Subscribe(stream_id, nullptr, [&, stream_id](const vhall::RespMsg& resp) -> void {
         //在此输出订阅成功或失败信息。
         LOGD_SDK("mLiveRoom->Subscribe resp:%s msg:%s resp.mCode:%d", resp.mResult.c_str(), resp.mMsg.c_str(), resp.mCode);
         if (vhall::StreamIsEmpty == resp.mCode) {
            return;
         }
         if (resp.mResult != SDK_SUCCESS) {
            CTaskEvent task;
            task.mEventType = TaskEventEnum_SUBSCRIB_ERROR;
            task.streamId = stream_id;
            task.mErrMsg = resp.mResult;
            task.mErrorType = resp.mCode;
            task.mSleepTime = 3000;
            LOGD_SDK("TaskEventEnum_RE_SUBSCRIB Subscribe stream_id uid:%s", stream_id.c_str());
            PostTaskEvent(task);
         }
      }, SDK_TIMEOUT, event.mSleepTime);
   }
   LOGD_SDK("TaskEventEnum_RE_SUBSCRIB end");
}

void WebRtcSDK::HandleRoomError(CTaskEvent& event) {
   mbIsWebRtcRoomConnected = false;
   LOGD_SDK("mbIsWebRtcRoomConnected = false");
   if (mWebRtcSDKEventReciver) {
      mWebRtcSDKEventReciver->OnRtcRoomNetStateCallback(vlive::VHRoomConnect_ROOM_ERROR);
   }
   //服务器主动拒绝，不进行重来。其它进行重连
   if (event.mErrorType == vhall::RoomEvent::ErrorType::ServerRefused) {
      LOGD_SDK("vhall::RoomEvent::ErrorType::ServerRefused");
   }
   else {
      mbIsWebRtcRoomConnected = false;
      LOGD_SDK("mbIsWebRtcRoomConnected = false");
      mSubScribeStreamManager.ClearSubScribeStream();
      LOGD_SDK("ROOM_ERROR start to call connect");
      CThreadLockHandle roomLockHandle(&mRoomThreadLock);
      if (mLiveRoom) {
         LOGD_SDK("HandleRoomError Disconnect");
         mLiveRoom->Disconnect();
         Sleep(2000);
         LOGD_SDK("HandleRoomError Connect");
         mLiveRoom->Connect();
      }
   }
   LOGD_SDK("HandleRoomError end");
}
void WebRtcSDK::HandleLocalCaptureBeautyLevel(CTaskEvent& event) {
    mLocalStreamFilterParam.beautyLevel = event.mBeautyLevel;
    LOGD_SDK("SetCameraBeautyLevel %d\n", event.mBeautyLevel);
    CThreadLockHandle lockHandle(&mLocalStreamThreadLock);
    if (mLocalStream) {
       mLocalStream->SetFilterParam(mLocalStreamFilterParam);
       LOGD_SDK("SetDesktopEdgeEnhance end");
    }
    LOGD_SDK("SetDesktopEdgeEnhance end");
}

void WebRtcSDK::HandlePreviewBeautyLevel(CTaskEvent& event) {
   mLocalStreamFilterParam.beautyLevel = event.mBeautyLevel;
   LOGD_SDK("SetCameraBeautyLevel %d\n", event.mBeautyLevel);
   CThreadLockHandle lockHandle(&mPreviewStreamThreadLock);
   if (mPreviewLocalStream) {
      mPreviewLocalStream->SetFilterParam(mLocalStreamFilterParam);
      LOGD_SDK("SetDesktopEdgeEnhance end");
   }
   LOGD_SDK("SetDesktopEdgeEnhance end");
}

void WebRtcSDK::HandleStreamPushError(CTaskEvent& event) {
   LOGD_SDK("HandleStreamPushError stream_type %d code %d msg %s", event.mStreamType,event.mErrorType,event.mErrMsg.c_str());
   if (mWebRtcSDKEventReciver) {
      mWebRtcSDKEventReciver->OnPushStreamError((vlive::VHStreamType)(event.mErrorType), event.mStreamType, event.mErrMsg);
   }
   LOGD_SDK("HandleStreamPushError end");
}

void WebRtcSDK::HandleOnStreamMixed(CTaskEvent& event) {
   LOGD_SDK("HandleOnStreamMixed");
   SetEnableConfigMixStream(true);
   if (mWebRtcSDKEventReciver) {
      std::string msg = "Server-side mixing ready";
      mWebRtcSDKEventReciver->OnRtcRoomNetStateCallback(vlive::VHRoomConnect_ROOM_MIXED_STREAM_READY);
   }
   if (event.mJoinUid == SubScribeStreamManager::WString2String(mWebRtcRoomOption.strUserId)) {
      LOGD_SDK("Publish stream success stream id:%s streamType:%d video:%d audio:%d", event.streamId.c_str(), event.mStreamType, event.mHasVideo, event.mHasAudio);
      if (event.mStreamType <= VHStreamType_AVCapture) {
         mLocalStreamId = event.streamId;
         mWebRtcSDKEventReciver->OnPushStreamSuc(vlive::VHStreamType_AVCapture, event.streamId, event.mHasVideo, event.mHasAudio, event.mStreamAttributes);
      }
      else {
         switch (event.mStreamType)
         {
         case vlive::VHStreamType_MediaFile: {
            mMediaFileStreamId = event.streamId;
            if (mWebRtcSDKEventReciver) {
               Sleep(2000);
               mWebRtcSDKEventReciver->OnPushStreamSuc(vlive::VHStreamType_MediaFile, event.streamId, event.mHasVideo, event.mHasAudio, event.mStreamAttributes);
            }
            break;
         }
         case vlive::VHStreamType_Desktop: {
            mDesktopStreamId = event.streamId;
            if (mWebRtcSDKEventReciver) {
               mWebRtcSDKEventReciver->OnPushStreamSuc(vlive::VHStreamType_Desktop, event.streamId, true, false, event.mStreamAttributes);
            }
            break;
         }
         case vlive::VHStreamType_SoftWare: {
            mSoftwareStreamId = event.streamId;
            if (mWebRtcSDKEventReciver) {
               mWebRtcSDKEventReciver->OnPushStreamSuc(vlive::VHStreamType_SoftWare, event.streamId, true, false, event.mStreamAttributes);
            }
            break;
         }
         case VHStreamType_Auxiliary_CamerCapture: {
            if (mWebRtcSDKEventReciver) {
               mWebRtcSDKEventReciver->OnPushStreamSuc(vlive::VHStreamType_Auxiliary_CamerCapture, event.streamId, true, false, event.mStreamAttributes);
            }
            break;
         }
         default:
            break;
         }
      }
   }
   LOGD_SDK("HandleOnStreamMixed end");
}

void WebRtcSDK::HandleRoomReConnecting(CTaskEvent& event) {
   mbIsWebRtcRoomConnected = false;
   LOGD_SDK("mbIsWebRtcRoomConnected = false");
   if (mWebRtcSDKEventReciver) {
      mWebRtcSDKEventReciver->OnRtcRoomNetStateCallback(vlive::VHRoomConnect_ROOM_RECONNECTING);
   }
   LOGD_SDK("HandleRoomReConnecting end");
}

void WebRtcSDK::HandleRoomReConnected(CTaskEvent& event) {
   mbIsWebRtcRoomConnected = true;
   LOGD_SDK("mbIsWebRtcRoomConnected = true");
   if (mWebRtcSDKEventReciver) {
      mWebRtcSDKEventReciver->OnRtcRoomNetStateCallback(vlive::VHRoomConnect_ROOM_RE_CONNECTED);
   }
   LOGD_SDK("HandleRoomReConnected end");
}

void WebRtcSDK::HandleRoomRecover(CTaskEvent& event) {
   mbIsWebRtcRoomConnected = true;
   LOGD_SDK("mbIsWebRtcRoomConnected = true");
   if (mWebRtcSDKEventReciver) {
      mWebRtcSDKEventReciver->OnRtcRoomNetStateCallback(vlive::VHRoomConnect_ROOM_RECOVER);
   }
   LOGD_SDK("HandleRoomRecover end");
}

void WebRtcSDK::HandleRoomNetworkChanged(CTaskEvent& event) {
   mbIsWebRtcRoomConnected = true;
   LOGD_SDK("mbIsWebRtcRoomConnected = true");
   if (mWebRtcSDKEventReciver) {
      mWebRtcSDKEventReciver->OnRtcRoomNetStateCallback(vlive::VHRoomConnect_ROOM_NETWORKCHANGED);
   }
   LOGD_SDK("HandleRoomNetworkChanged end");
}

void WebRtcSDK::HandleSetPlayerDev(CTaskEvent& event) {
   LOGD_SDK("%s enter event.devIndex %d desktopId %s volue %d",__FUNCTION__, event.devIndex, SubScribeStreamManager::WString2String(event.desktopCaptureId).c_str(), event.mVolume);
   vhall::VHStream::SetLoopBackVolume(event.mVolume);
   int nRet = vhall::VHStream::SetAudioInDevice(event.devIndex, event.desktopCaptureId) ? VhallLive_OK : VhallLive_NO_DEVICE;
   CThreadLockHandle lockHandle(&mLocalStreamThreadLock);
   if (mLocalStream && mLocalStream->IsAudioMuted()) {
      mLocalStream->MuteAudio(false);
   }
   LOGD_SDK("end nRet %d", nRet);
}

void WebRtcSDK::HandleSetMicDev(CTaskEvent& event) {
   LOGD_SDK("HandleSetMicDev event.devIndex %d desktopCaptureId %s", event.devIndex, SubScribeStreamManager::WString2String(event.desktopCaptureId).c_str());
   vhall::VHStream::SetAudioInDevice(event.devIndex, event.desktopCaptureId);
   vhall::VHStream::SetMicrophoneVolume(mbIsOpenMic == true ? (int)mCurMicVol : 0);
   CThreadLockHandle lockHandle(&mLocalStreamThreadLock);
   if (mLocalStream) {
      if (event.desktopCaptureId.length() > 0) {
         mLocalStream->MuteAudio(false);
      }
      else {
         if (!mbIsOpenMic) {
            mLocalStream->MuteAudio(true);
         }
      }
   }
   LOGD_SDK("end");
   return;
}

void WebRtcSDK::HandleStreamRePublished(CTaskEvent& event) {
   LOGD_SDK("HandleStreamRePublished stream:%s join_uid:%s type:%d", event.streamId.c_str(), event.mJoinUid.c_str(), event.mStreamType);
   std::wstring id = SubScribeStreamManager::String2WString(event.mJoinUid);
   HandleRePublished(event.mStreamType, id, event.streamId);
   LOGD_SDK("HandleRoomNetworkChanged end");
}

void WebRtcSDK::HandleStreamFailed(CTaskEvent& event) {
   LOGD_SDK("HandleStreamFailed");
   if (mWebRtcSDKEventReciver) {
      if (event.mStreamType == VHStreamType_Desktop) {
         LOGD_SDK("STREAM_FAILED VHStreamType_Desktop :%d", (int)mLocalStreamIsCapture);
         if (mLocalStreamIsCapture) {
            CThreadLockHandle lockHandle(&mDesktopStreamThreadLock);
            if (mDesktopStream == nullptr) {
               return;  
            }
            mWebRtcSDKEventReciver->OnPushStreamError(VHStreamType_Desktop, 40001, "STREAM_FAILED");
         }
      }
      else if (event.mStreamType == VHStreamType_SoftWare) {
         LOGD_SDK("STREAM_FAILED VHStreamType_SoftWare :%d", (int)mSoftWareStreamIsCapture);
         if (mSoftWareStreamIsCapture) {
            CThreadLockHandle lockHandle(&mSoftwareStreamThreadLock);
            if (mSoftwareStream == nullptr) {
               return;
            }
            mWebRtcSDKEventReciver->OnPushStreamError(VHStreamType_SoftWare, 40001, "STREAM_FAILED");
         }
      }
      else if (event.mStreamType == VHStreamType_MediaFile) {
         LOGD_SDK("STREAM_FAILED VHStreamType_MediaFile :%d", (int)mMediaStreamIsCapture);
         if (mMediaStreamIsCapture) {
            CThreadLockHandle lockHandle(&mMediaStreamThreadLock);
            if (mMediaStream == nullptr) {
               return;
            }
            mWebRtcSDKEventReciver->OnPushStreamError(vlive::VHStreamType_MediaFile, 40001, "STREAM_FAILED");
         }
      }
      else {
         LOGD_SDK("STREAM_FAILED VHStreamType_AVCapture mLocalStreamIsCapture:%d", (int)mLocalStreamIsCapture);
         if (mLocalStreamIsCapture) {
            CThreadLockHandle lockHandle(&mLocalStreamThreadLock);
            if (mLocalStream == nullptr) {
               return;
            }
            mWebRtcSDKEventReciver->OnPushStreamError(vlive::VHStreamType_AVCapture, 40001, "STREAM_FAILED");
         }
      }
   }
   LOGD_SDK("HandleStreamFailed end");
}

void WebRtcSDK::HandleStreamRemoved(CTaskEvent& event) {
   LOGD_SDK("HandleStreamRemoved");
   std::string join_uid = event.mJoinUid;
   std::string stream_id = event.streamId;
   std::wstring join_uid_temp = SubScribeStreamManager::String2WString(join_uid);
   LOGD_SDK("STREAM_REMOVED join_uid:%s stream_id:%s streamType:%d", join_uid.c_str(), stream_id.c_str(), event.mStreamType);
   std::string mediaType = std::to_string(event.mStreamType);
   std::wstring subStreamIndex = join_uid_temp + SubScribeStreamManager::String2WString(mediaType);
   CheckAssistFunction(event.mStreamType, true);
   RemoveSubScribeStreamObj(subStreamIndex, stream_id);
   if (mWebRtcSDKEventReciver) {
      mWebRtcSDKEventReciver->OnRemoteUserLiveStreamRemoved(SubScribeStreamManager::String2WString(join_uid), stream_id, (vlive::VHStreamType)event.mStreamType);
   }
   LOGD_SDK("HandleStreamRemoved end");
}

void WebRtcSDK::HandleStartVLCCapture(CTaskEvent& event) {
   bool bRet = false;
   CThreadLockHandle lockHandle(&mMediaStreamThreadLock);
   LOGD_SDK("enter");
   if (mMediaStream) {
      if (mediaStreamConfig.videoOpt.mProfileIndex != (VideoProfileIndex)event.mProfile) {
         mMediaStream->ResetCapability(mediaStreamConfig.videoOpt);
      }
      bRet = mMediaStream->FilePlay(event.filePath.c_str());
      LOGD_SDK("HandleCaptureMediaFile bRet:%d", bRet);
   }
}

void WebRtcSDK::HandleStopMediaCapture() {
   LOGD_SDK("mMediaStreamThreadLock");
   Sleep(1000);
   LOGD_SDK("enter");
   StopPushMediaFileStream();
   CThreadLockHandle lockHandle(&mMediaStreamThreadLock);
   if (mMediaStream) {
      mMediaStream->FileStop();
   }
   RelaseStream(mMediaStream);
   mMediaStreamIsCapture = false;
   LOGD_SDK("");
}

void WebRtcSDK::HandleStreamSubSuccess(CTaskEvent& event) {
   LOGD_SDK("HandleStreamSubSuccess");
   std::wstring user_id = SubScribeStreamManager::String2WString(event.mJoinUid);
   //如果类型是插播视频类型的，重新设置joinuid.
   if (event.mStreamType == vlive::VHStreamType_MediaFile) {
      CTaskEvent task;
      task.mEventType = TaskEventEnum_RECV_MEDIA_STREAM;
      LOGD_SDK("recv media stream");
      PostTaskEvent(task);
   }
   else if (event.mStreamType == vlive::VHStreamType_Desktop){
      CTaskEvent task;
      task.mEventType = TaskEventEnum_RECV_MEDIA_STREAM;
      LOGD_SDK("recv desktop stream");
      PostTaskEvent(task);
   }
   LOGD_SDK("mSubScribeStreamManager mMuteAllAudio:%d", (int)mMuteAllAudio);
   mSubScribeStreamManager.MuteAllSubScribeAudio(mMuteAllAudio);
   if (mWebRtcSDKEventReciver) {
      LOGD_SDK("OnReciveRemoteUserLiveStream :%s event.streamId:%s event.mStreamType:%d video:%d audio:%d", user_id.c_str(), event.streamId.c_str(), event.mStreamType, event.mHasVideo, event.mHasAudio);
      mWebRtcSDKEventReciver->OnReciveRemoteUserLiveStream(user_id, event.streamId, (vlive::VHStreamType)event.mStreamType, event.mHasVideo, event.mHasAudio,event.mStreamAttributes);
   }
   LOGD_SDK("HandleStreamSubSuccess end");
}

void WebRtcSDK::HandlePrivew(CTaskEvent &event) {
   LOGD_SDK("HandlePrivew");
   CThreadLockHandle lockHandle(&mPreviewStreamThreadLock);
   if (mbStopPreviewCamera) {
      LOGD_SDK("mbStopPreviewCamera");
      return;
   }
   if (mPreviewLocalStream) {
      if (event.mErrMsg == ACCESS_ACCEPTED) {
         LOGD_SDK("TaskEventEnum_PREVIE_PLAY");
         LOGD_SDK("stop");
         mPreviewLocalStream->stop();
         if (mPreviewVideoRecver) {
            mPreviewLocalStream->play(mPreviewVideoRecver);
         }
         else {
            mPreviewLocalStream->play(previewWnd);
         }
         LOGD_SDK("play");
         if (mWebRtcSDKEventReciver) {
            mWebRtcSDKEventReciver->OnOpenCaptureCallback(vlive::VHStreamType_Preview_Video, vlive::VHCapture_OK, previewStreamConfig.mVideo, previewStreamConfig.mAudio);
         }
      }
      else if (event.mErrMsg == VIDEO_DENIED || event.mErrMsg == ACCESS_DENIED) {
         if (mWebRtcSDKEventReciver) {
            mWebRtcSDKEventReciver->OnOpenCaptureCallback(vlive::VHStreamType_Preview_Video, vlive::VHCapture_VIDEO_DENIED, previewStreamConfig.mVideo, previewStreamConfig.mAudio);
         }
      }
      else if (event.mErrMsg == CAMERA_LOST) {  //摄像头拔出
         if (mWebRtcSDKEventReciver) {
            mWebRtcSDKEventReciver->OnOpenCaptureCallback(vlive::VHStreamType_Preview_Video, vlive::VHCapture_CAMERA_LOST, previewStreamConfig.mVideo, previewStreamConfig.mAudio);
         }
      }
      else if (event.mErrMsg == AUDIO_DENIED) {
         if (mWebRtcSDKEventReciver) {
            mWebRtcSDKEventReciver->OnOpenCaptureCallback(vlive::VHStreamType_Preview_Video, vlive::VHCapture_AUDIO_DENIED, previewStreamConfig.mVideo, previewStreamConfig.mAudio);
         }
      }
   }
   LOGD_SDK("HandlePrivew end");
}

void WebRtcSDK::HandleCaptureAuxiliaryLocalStrea(CTaskEvent &event) {
   LOGD_SDK("HandleCaptureAuxiliaryLocalStrea");
   CThreadLockHandle lockHandle(&mLocalAuxiliaryStreamThreadLock);
   LOGD_SDK("get mLocalAuxiliaryStreamThreadLock");
   if (mLocalAuxiliaryStream) {
      if (event.mErrMsg == ACCESS_ACCEPTED) {
         if (mWebRtcSDKEventReciver) {
            mLocalAuxiliaryStreamId = mLocalAuxiliaryStream->mId;
            mWebRtcSDKEventReciver->OnOpenCaptureCallback(vlive::VHStreamType_Auxiliary_CamerCapture, vlive::VHCapture_OK, true, false);
         }
      }
      else if (event.mErrMsg == ACCESS_DENIED) {
         if (mWebRtcSDKEventReciver) {
            LOGD_SDK("vlive::VHCapture_ACCESS_DENIED");
            mWebRtcSDKEventReciver->OnOpenCaptureCallback(vlive::VHStreamType_Auxiliary_CamerCapture, vlive::VHCapture_ACCESS_DENIED, true, false);
         }
      }
      else if (event.mErrMsg == VIDEO_DENIED) {
         //视频设备打开失败，请您检测设备是否连接正常
         if (mWebRtcSDKEventReciver) {
            LOGD_SDK("vlive::VHCapture_VIDEO_DENIED");
            mWebRtcSDKEventReciver->OnOpenCaptureCallback(vlive::VHStreamType_Auxiliary_CamerCapture, vlive::VHCapture_VIDEO_DENIED, true, false);
         }
      }
      else if (event.mErrMsg == CAMERA_LOST) {
         if (mWebRtcSDKEventReciver) {
            LOGD_SDK("vlive::VHCapture_STREAM_SOURCE_LOST");
            mWebRtcSDKEventReciver->OnOpenCaptureCallback(vlive::VHStreamType_Auxiliary_CamerCapture, vlive::VHCapture_STREAM_SOURCE_LOST, true, false);
         }
      }
   }
   LOGD_SDK("HandleCaptureAuxiliaryLocalStrea end");
}

int WebRtcSDK::StartPushLocalAuxiliaryStream(std::string context) {
   LOGD_SDK("");
   if (!mbIsWebRtcRoomConnected) {
      LOGD_SDK("VhallLive_ROOM_DISCONNECT");
      return vlive::VhallLive_ROOM_DISCONNECT;
   }
   CThreadLockHandle lockHandle(&mLocalAuxiliaryStreamThreadLock);
   LOGD_SDK("mLocalAuxiliaryStreamThreadLock");
   if (mLiveRoom && mLocalAuxiliaryStream) {
      LOGD_SDK("Publish mLocalAuxiliaryStream");
      bool hasVideo = true;
      bool hasAudio = false;
      mLocalAuxiliaryStream->mStreamAttributes = context;
      mLiveRoom->Publish(mLocalAuxiliaryStream, [&, hasVideo, hasAudio](const vhall::PublishRespMsg& resp) -> void {
         if (resp.mResult == SDK_SUCCESS) {
            LOGD_SDK("Publish mLocalAuxiliaryStream mLocalAuxiliaryStreamId:%s", mLocalAuxiliaryStreamId.c_str());
         }
         else {
            LOGD_SDK("Publish mLocalAuxiliaryStream err:%s resp.mCode:%d", resp.mMsg.c_str(), resp.mCode);
            if (vhall::StreamPublished == resp.mCode || vhall::StreamIsEmpty == resp.mCode) {
               return;
            }
            CTaskEvent roomConnectTask;
            roomConnectTask.mEventType = TaskEventEnum_STREAM_PUSH_ERROR;
            roomConnectTask.mStreamType = vlive::VHStreamType_Auxiliary_CamerCapture;
            roomConnectTask.mErrorType = resp.mCode;
            roomConnectTask.mErrMsg = resp.mMsg;
            PostTaskEvent(roomConnectTask);
         }
      }, SDK_TIMEOUT);
   }
   else {
      LOGD_SDK("VhallLive_NO_OBJ");
      return vlive::VhallLive_NO_OBJ;
   }
   LOGD_SDK("VhallLive_OK");
   return vlive::VhallLive_OK;
}

int WebRtcSDK::StopPushLocalAuxiliaryStream() {
   return 0;
}

void WebRtcSDK::HandleSetPlayFileVol(CTaskEvent &event) {
   CThreadLockHandle lockHandle(&mMediaStreamThreadLock);
   if (mMediaStream) {
      mMediaStream->SetFileVolume(event.mVolume);
      return;
   }
   else {
      return;
   }
}

void WebRtcSDK::HandleCaptureLocalStream(CTaskEvent &event) {
   LOGD_SDK("HandleCaptureLocalStream");
   CThreadLockHandle lockHandle(&mLocalStreamThreadLock);
   LOGD_SDK("get mLocalStreamThreadLock");
   if (mLocalStream) {
      if (event.mErrMsg == ACCESS_ACCEPTED) {
         mLocalStreamIsCapture = true;
         if (mWebRtcSDKEventReciver) {
            mLocalStreamId = mLocalStream->mId;
            mWebRtcSDKEventReciver->OnOpenCaptureCallback(vlive::VHStreamType_AVCapture, vlive::VHCapture_OK, mCameraStreamConfig.mVideo, mCameraStreamConfig.mAudio);
         }
      }
      else if (event.mErrMsg == ACCESS_DENIED) {
         mLocalStreamIsCapture = false;
         mbIsOpenCamera = false;
         mbIsOpenMic = false;
         if (mWebRtcSDKEventReciver) {
            LOGD_SDK("vlive::VHCapture_ACCESS_DENIED");
            mWebRtcSDKEventReciver->OnOpenCaptureCallback(vlive::VHStreamType_AVCapture, vlive::VHCapture_ACCESS_DENIED, mCameraStreamConfig.mVideo, mCameraStreamConfig.mAudio);
         }
      }
      else if (event.mErrMsg == VIDEO_DENIED) {
         mLocalStreamIsCapture = false;
         mbIsOpenCamera = false;
         mbIsOpenMic = false;
         //视频设备打开失败，请您检测设备是否连接正常
         if (mWebRtcSDKEventReciver) {
            LOGD_SDK("vlive::VHCapture_VIDEO_DENIED");
            mWebRtcSDKEventReciver->OnOpenCaptureCallback(vlive::VHStreamType_AVCapture, vlive::VHCapture_VIDEO_DENIED, mCameraStreamConfig.mVideo, mCameraStreamConfig.mAudio);
         }
      }
      else if (event.mErrMsg == VIDEO_CAPTURE_ERROR) {
         //视频设备打开失败，请您检测设备是否连接正常
         mCurrentCameraGuid = "";
         if (mWebRtcSDKEventReciver) {
            LOGD_SDK("vlive::VIDEO_CAPTURE_ERROR");
            mWebRtcSDKEventReciver->OnOpenCaptureCallback(vlive::VHStreamType_AVCapture, vlive::VHCapture_VIDEO_CAPTURE_ERROR, mCameraStreamConfig.mVideo, mCameraStreamConfig.mAudio);
         }
      }
      else if (event.mErrMsg == AUDIO_DENIED) {
         mLocalStreamIsCapture = false;
         mbIsOpenCamera = false;
         mbIsOpenMic = false;
         //视频设备打开失败，请您检测设备是否连接正常
         if (mWebRtcSDKEventReciver) {
            LOGD_SDK("vlive::VHCapture_AUDIO_DENIED");
            mWebRtcSDKEventReciver->OnOpenCaptureCallback(vlive::VHStreamType_AVCapture, vlive::VHCapture_AUDIO_DENIED, mCameraStreamConfig.mVideo, mCameraStreamConfig.mAudio);
         }
      }
      else if (event.mErrMsg == CAMERA_LOST) {
         mCurrentCameraGuid = "";
         if (mWebRtcSDKEventReciver) {
            LOGD_SDK("vlive::VHCapture_STREAM_SOURCE_LOST");
            mWebRtcSDKEventReciver->OnOpenCaptureCallback(vlive::VHStreamType_AVCapture, vlive::VHCapture_STREAM_SOURCE_LOST, mCameraStreamConfig.mVideo, mCameraStreamConfig.mAudio);
         }
      }
      else if (event.mErrMsg == AUDIO_CAPTURE_ERROR) {
         if (mWebRtcSDKEventReciver) {
            LOGD_SDK("vlive::VHCapture_AUDIO_DENIED");
            mWebRtcSDKEventReciver->OnOpenCaptureCallback(vlive::VHStreamType_AVCapture, vlive::VHCapture_AUDIO_DENIED, mCameraStreamConfig.mVideo, mCameraStreamConfig.mAudio);
         }
      }
      else if (event.mErrMsg == IAUDIOCLIENT_INITIALIZE || event.mErrMsg == COREAUDIOCAPTUREERROR) {
         if (mWebRtcSDKEventReciver) {
            LOGD_SDK("vlive::IAUDIOCLIENT_INITIALIZE or COREAUDIOCAPTUREERROR");
            mWebRtcSDKEventReciver->OnOpenCaptureCallback(vlive::VHStreamType_AVCapture, vlive::VHCapture_AUDIO_CAPTRUE_ERR, mCameraStreamConfig.mVideo, mCameraStreamConfig.mAudio);
         }
      }
      else if (event.mErrMsg == AUDIO_DEVICE_REMOVED || event.mErrMsg == AUDIO_DEVICE_STATE) {
         if (mWebRtcSDKEventReciver) {
            LOGD_SDK("vlive::AUDIO_DEVICE_REMOVED event.devId %s mCurrentMicDevid %s ", event.devId.c_str(), mCurrentMicDevid.c_str());
            if (event.devId == mCurrentMicDevid) {
               mWebRtcSDKEventReciver->OnOpenCaptureCallback(vlive::VHStreamType_AVCapture, vlive::VHCapture_AUDIODEV_REMOVED, mCameraStreamConfig.mVideo, mCameraStreamConfig.mAudio);
            }
         }
      }
   }
   LOGD_SDK("HandleCaptureLocalStream end");
}

void WebRtcSDK::HandleCaptureMediaFile(CTaskEvent &event) {
   LOGD_SDK("HandleCaptureMediaFile");
   CThreadLockHandle lockHandle(&mMediaStreamThreadLock);
   if (mMediaStream) {
      if (event.mErrMsg == ACCESS_ACCEPTED) {
         mMediaStreamIsCapture = true;
         if (mWebRtcSDKEventReciver) {
            mWebRtcSDKEventReciver->OnOpenCaptureCallback(vlive::VHStreamType_MediaFile, vlive::VHCapture_OK, mediaStreamConfig.mVideo, mediaStreamConfig.mAudio);
         }
      }
      else if (event.mErrMsg == ACCESS_DENIED) {
         mMediaStreamIsCapture = false;
         LOGD_SDK("ACCESS_DENIED");
         if (mWebRtcSDKEventReciver) {
            LOGD_SDK("vlive::VHCapture_ACCESS_DENIED");
            mWebRtcSDKEventReciver->OnOpenCaptureCallback(vlive::VHStreamType_MediaFile, vlive::VHCapture_ACCESS_DENIED, mediaStreamConfig.mVideo, mediaStreamConfig.mAudio);
         }
      }
   }
   LOGD_SDK("HandleCaptureMediaFile end");
}

void WebRtcSDK::HandleAddStream(CTaskEvent &event) {
   LOGD_SDK("HandleAddStream");
   if (mWebRtcSDKEventReciver) {
      mWebRtcSDKEventReciver->OnRemoteStreamAdd(event.mJoinUid, event.streamId, (vlive::VHStreamType)event.mStreamType);
   }
   LOGD_SDK("HandleAddStream end");
}

void WebRtcSDK::HandleRecvStreamLost(CTaskEvent& event) {
   LOGD_SDK("HandleRecvStreamLost %s", event.streamId.c_str());
   if (event.streamId.length() > 0) {
      CThreadLockHandle roomLockHandle(&mRoomThreadLock);
      LOGD_SDK("mLiveRoom");
      if (mLiveRoom) {
         LOGD_SDK("UnSubscribe");
         mLiveRoom->UnSubscribe(event.streamId);
      }
   }
   mSubScribeStreamManager.RemoveSubScribeStreamObj(event.mStreamIndex, event.streamId);
   LOGD_SDK("HandleRoomStreamSubScribe");
   if (mWebRtcSDKEventReciver) {
      mWebRtcSDKEventReciver->OnSubScribeStreamErr(event.streamId, event.mErrMsg, 50001, event.mJoinUid);
   }
   LOGD_SDK("HandleRecvStreamLost end");
}

void WebRtcSDK::HandleCaptureSoftwareStream(CTaskEvent &event) {
   LOGD_SDK("HandleCaptureSoftwareStream");
   CThreadLockHandle lockHandle(&mSoftwareStreamThreadLock);
   if (mSoftwareStream) {
      if (event.mErrMsg == ACCESS_ACCEPTED) {
         mSoftWareStreamIsCapture = true;
         mSoftwareStream->SelectSource(event.devIndex);
         if (mWebRtcSDKEventReciver) {
            mWebRtcSDKEventReciver->OnOpenCaptureCallback(vlive::VHStreamType_SoftWare, vlive::VHCapture_OK, true, false);
         }
      }
      else if (event.mErrMsg == ACCESS_DENIED) {
         LOGD_SDK("ACCESS_DENIED");
         mSoftWareStreamIsCapture = false;
         if (mWebRtcSDKEventReciver) {
            LOGD_SDK("vlive::VHCapture_ACCESS_DENIED");
            mWebRtcSDKEventReciver->OnOpenCaptureCallback(vlive::VHStreamType_SoftWare, vlive::VHCapture_ACCESS_DENIED, true, false);
         }
      }
   }
   LOGD_SDK("HandleCaptureSoftwareStream end");
}


void WebRtcSDK::HandleCaptureDesktopStream(CTaskEvent &event) {
   LOGD_SDK("HandleCaptureSoftwareStream");
   CThreadLockHandle lockHandle(&mDesktopStreamThreadLock);
   if (mDesktopStream) {
      if (event.mErrMsg == ACCESS_ACCEPTED) {
         mDesktopStreamIsCapture = true;
         mDesktopStream->SelectSource(event.devIndex);
         if (mWebRtcSDKEventReciver) {
            mWebRtcSDKEventReciver->OnOpenCaptureCallback(vlive::VHStreamType_Desktop, vlive::VHCapture_OK, true, false);
         }
      }
      else if (event.mErrMsg == ACCESS_DENIED) {
         LOGD_SDK("ACCESS_DENIED");
         mDesktopStreamIsCapture = false;
         if (mWebRtcSDKEventReciver) {
            LOGD_SDK("vlive::VHCapture_ACCESS_DENIED");
            mWebRtcSDKEventReciver->OnOpenCaptureCallback(vlive::VHStreamType_Desktop, vlive::VHCapture_ACCESS_DENIED, true, false);
         }
      }
   }
   LOGD_SDK("HandleCaptureSoftwareStream end");
}

int WebRtcSDK::ParamPushType(std::string& context) {
   int type = -1;
   rapidjson::Document doc;
   doc.Parse<0>(context.c_str());
   if (doc.HasParseError()) {
      return type;
   }
   if (doc.IsObject()) {
      if (doc.HasMember("double_push")) {
         type = 0;
      }
   }
   return type;
}

bool WebRtcSDK::StartRenderLocalStream(vlive::VHStreamType streamType, std::shared_ptr<vhall::VideoRenderReceiveInterface> recv) {
   LOGD_SDK("StartRenderStream");
   if (streamType == vlive::VHStreamType_AVCapture) {
      CThreadLockHandle lockHandle(&mLocalStreamThreadLock);
      LOGD_SDK("mLocalStreamThreadLock");
      if (mLocalStream && mLocalStream->HasVideo()) {
         LOGD_SDK("play mLocalStream");
         mLocalCameraRecv = recv;
         mLocalStream->stop();
         mLocalStream->play(recv);
         LOGD_SDK("play mLocalStream end");
      }
      return true;
   }
   else if (streamType == vlive::VHStreamType_Desktop) {
      LOGD_SDK("mDesktopStreamThreadLock");
      CThreadLockHandle lockHandle(&mDesktopStreamThreadLock);
      if (mDesktopStream) {
         LOGD_SDK("play local mDesktopStream");
         mDesktopStream->stop();
         mDesktopStream->play(recv);
         LOGD_SDK("play local mDesktopStream end");
         return true;
      }
   }
   else if (streamType == vlive::VHStreamType_MediaFile) {
      LOGD_SDK("mMediaStreamThreadLock");
      CThreadLockHandle lockHandle(&mMediaStreamThreadLock);
      if (mMediaStream) {
         LOGD_SDK("play local mMediaStream");
         mMediaStream->stop();
         mMediaStream->play(recv);
         LOGD_SDK("play local mMediaStream end");
         return true;
      }
   }
   else if (streamType == vlive::VHStreamType_SoftWare) {
      LOGD_SDK("mDesktopStreamThreadLock");
      CThreadLockHandle lockHandle(&mSoftwareStreamThreadLock);
      if (mSoftwareStream) {
         LOGD_SDK("play local mDesktopStream");
         mSoftwareStream->stop();
         mSoftwareStream->play(recv);
         LOGD_SDK("play local mDesktopStream end");
         return true;
      }
   }
   else if (streamType == vlive::VHStreamType_Auxiliary_CamerCapture) {
      LOGD_SDK("mLocalAuxiliaryStreamThreadLock");
      CThreadLockHandle lockHandle(&mLocalAuxiliaryStreamThreadLock);
      if (mLocalAuxiliaryStream) {
         LOGD_SDK("play local mLocalAuxiliaryStream");
         mLocalAuxiliaryStream->stop();
         mLocalAuxiliaryStream->play(recv);
         LOGD_SDK("play local mLocalAuxiliaryStream end");
         return true;
      }
   }
   return false;
}

/**
*  获取推/拉流音频音量等级
*/
int WebRtcSDK::GetAudioLevel(std::wstring user_id) {
   if (user_id == mWebRtcRoomOption.strUserId) {
      CThreadLockHandle lockHandle(&mLocalStreamThreadLock);
      LOGD_SDK("mLocalStreamThreadLock");
      if (mLocalStream && mLocalStream->HasAudio()) {
         vhall::SendAudioSsrc pSsrc;
         mLocalStream->GetAudioPublishStates(&pSsrc);
         return pSsrc.AudioInputLevel;
      }
   }
   else {
      std::wstring subStreamIndex = user_id + SubScribeStreamManager::String2WString(std::to_string(VHStreamType_AVCapture));
      return mSubScribeStreamManager.GetAudioLevel(subStreamIndex);
   }
   return -1;
}

/**
*  获取推/拉流视频丢包率
*/
double WebRtcSDK::GetVideoLostRate(std::wstring user_id) {
   if (user_id == mWebRtcRoomOption.strUserId) {
      CThreadLockHandle lockHandle(&mLocalStreamThreadLock);
      LOGD_SDK("mLocalStreamThreadLock");
      if (mLocalStream) {
         if (mVideoSsrc) {
            mLocalStream->GetVideoPublishStates(mVideoSsrc.get());
            mVideoSsrc->calc();
            if (mVideoSsrc->CalcData.Bitrate == 0) {
               return 100.0;
            }
            else {
               return mVideoSsrc->CalcData.PacketsLostRate;
            }
         }
         else {
            return 0;
         }
      }
   }
   else {
      std::wstring subStreamIndex = user_id + SubScribeStreamManager::String2WString(std::to_string(VHStreamType_AVCapture));
      return mSubScribeStreamManager.GetVideoLostRate(subStreamIndex);
   }
   return -1;
}

double WebRtcSDK::GetPushDesktopVideoLostRate() {
   CThreadLockHandle lockHandle(&mDesktopStreamThreadLock);
   LOGD_SDK("mLocalStreamThreadLock");
   if (mDesktopStream) {
      if (mDesktopVideoSsrc) {
         mDesktopStream->GetVideoPublishStates(mDesktopVideoSsrc.get());
         mDesktopVideoSsrc->calc();
         if (mDesktopVideoSsrc->CalcData.Bitrate == 0) {
            return 100.0;
         }
         else {
            return mDesktopVideoSsrc->CalcData.PacketsLostRate;
         }
      }
      else {
         return 0;
      }
   }
   return -1;
}
