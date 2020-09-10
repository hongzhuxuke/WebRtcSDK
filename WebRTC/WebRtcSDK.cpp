#include "WebRtcSDK.h"
#include "vhall_log.h"
#include "signalling\BaseStack\AdmConfig.h"
#include "./WebRTC/vhall_sdk_log.h"
#include "signalling\BaseStack\AdmConfig.h"
#include "pictureDecoder.h"
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

#define SDK_TIMEOUT 25000

using namespace webrtc_sdk;

WebRtcSDK::WebRtcSDK()
{
    //vhall::AdmConfig::InitializeAdm();
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
	mCameraStreamConfig.videoOpt.lockRatio = false;
   mAuxiliaryStreamConfig.videoOpt.lockRatio = false;

    mLocalDevTools = std::make_shared<vhall::VHTools>();

    LOGD_SDK("VhallWebRTC_VERSION : %s", VhallWebRTC_VERSION);
    bThreadRun = true;
    mProcessThread = new std::thread(WebRtcSDK::ThreadProcess, this);
    if (mProcessThread) {
        LOGD_SDK("create task thread success");

        gTaskEvent = ::CreateEvent(NULL, FALSE, FALSE, NULL);
        gThreadExitEvent = ::CreateEvent(NULL, FALSE, FALSE, NULL);
    }
}

WebRtcSDK::~WebRtcSDK()
{
    LOGD_SDK("WebRtcSDK::~WebRtcSDK");
    ExitThread();
    DisConnetWebRtcRoom();
    mtaskEventList.clear();
    vhall::AdmConfig::DeInitAdm();
    LOGD_SDK("WebRtcSDK::~WebRtcSDK end");
}

double WebRtcSDK::GetPushDesktopVideoLostRate()
{
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
    LOGD_SDK("WebRtcSDK::end");
}

void WebRtcSDK::InitSDK(WebRtcSDKEventInterface* obj, std::wstring logPath/* = std::wstring()*/) {
    if (!mbIsInit) {
        mbIsInit = true;
        mWebRtcSDKEventReciver = obj;
        SDKInitLog(logPath);
    }

    vhall::AdmConfig::InitializeAdm();
    LOGD_SDK("WebRtcSDK::~WebRtcSDK end");

    LOGD_SDK("VhallWebRTC_VERSION : %s", VhallWebRTC_VERSION);
}

void WebRtcSDK::SendAudioData(const void * audio_data, int bits_per_sample, int sample_rate, size_t number_of_channels, size_t number_frames_perChannel) {

}

void WebRtcSDK::EnableSubScribeStream() {
    bAutoSubScribeStream = true;
}

int WebRtcSDK::ConnetWebRtcRoom(const vlive::WebRtcRoomOption& option) {
   LOGD_SDK("");
   int bRet = vlive::VhallLive_OK;
   mWebRtcRoomOption = option;
   //创建房间管理
   struct vhall::RoomOptions specInput;
   //参数说明地址http://wiki.vhallops.com/pages/viewpage.action?pageId=918066
   specInput.upLogUrl = SubScribeStreamManager::WString2String(mWebRtcRoomOption.strLogUpLogUrl);// QString("http:%1").arg(mWebRtcRoomOption.strLogUpLogUrl).toStdString().c_str();
   
   specInput.biz_role = option.role;
   specInput.biz_id = SubScribeStreamManager::WString2String(option.strRoomId);  //参会id，webinar，id
   specInput.biz_des01 = option.classType;
   specInput.biz_des02 = 1;
   //specInput.aid = SubScribeStreamManager::WString2String(option.strPassRoomId);
   
   specInput.uid = SubScribeStreamManager::WString2String(mWebRtcRoomOption.strUserId); //参会id  join接口返回 字段join 里获取 id
   specInput.bu = SubScribeStreamManager::WString2String(option.strBusinesstype);
   if (option.strVid.length() == 0) {
      specInput.vid = SubScribeStreamManager::WString2String(option.strAccountId);
   }
   else {
      specInput.vid = option.strVid;
   }

   if (option.strVfid.length() == 0) {
      specInput.vfid = SubScribeStreamManager::WString2String(option.strAccountId);
   }
   else {
      specInput.vfid = option.strVfid;
   }
   
   LOGD_SDK("strUserId %d role %d  vfid %s vid %s bu %s biz_des01 %d", 
      mWebRtcRoomOption.strUserId, option.role, specInput.vfid.c_str(), specInput.vid.c_str(), specInput.bu.c_str() , specInput.biz_des01);

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
   /**
   ROOM_DISCONNECTED   房间已断开，已停止推拉流
   ROOM_RECONNECTED    已重新进入房间，已停止推拉流； 无法自动拉流（app结合业务从最新流列表判断是否拉流）；若正在推本地流，SDK自动重新推流（重新推流失败应当跑出stream_fail事件,新信令版本实现）；
   ROOM_CONNECTED      进入房间成功，返回最新流列表
   ROOM_NETWORKCHANGED 发现本地IP（网卡）发生变化且已重新进入房间，SDK自动恢复推拉流，完成返回ROOM_NETWORKRECOVER
   ROOM_NETWORKRECOVER 推拉流恢复操作完成
   ROOM_CONNECTING     首次连接房间失败，重新建立连接
   ROOM_RECONNECTING   连接断开，正在尝试重连
   */

   std::wstring curJoinUid = mWebRtcRoomOption.strUserId;
   if (mLiveRoom) {
      mLiveRoom->mRoomId = SubScribeStreamManager::WString2String(option.strRoomId);
      //连接房间成功。有可能已经有人，获取房间成员列表并订阅。
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
               //if (!context.empty()) {
                  int type = ParamPushType(context);
                  //if (type <= 0) {
                  //   continue;
                  //}
               //}

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

      mAudioDeviceDispatcher = vhall::AdmConfig::GetAudioDeviceEventDispatcher();
      if (mAudioDeviceDispatcher) {
         mAudioDeviceDispatcher->AddEventListener(AUDIO_CAPTURE_ERROR, [&](vhall::BaseEvent &Event)->void {
            LOGD_SDK("AUDIO_CAPTURE_ERROR");
            CTaskEvent task;
            task.mEventType = TaskEventEnum_CaptureLocalStream;
            task.mErrMsg = AUDIO_CAPTURE_ERROR;
            PostTaskEvent(task);
         });

         mAudioDeviceDispatcher->AddEventListener(AUDIO_DEVICE_REMOVED, [&](vhall::BaseEvent &Event)->void {
            LOGD_SDK("AUDIO_DEVICE_REMOVED");
            CTaskEvent task;
            task.mEventType = TaskEventEnum_CaptureLocalStream;
            task.mErrMsg = AUDIO_DEVICE_REMOVED;
            PostTaskEvent(task);
         });

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

      //mLiveRoom->AddEventListener(STREAM_READY, [&](vhall::BaseEvent &Event)->void {
      //   vhall::StreamEvent *stream = static_cast<vhall::StreamEvent *>(&Event);
      //   QApplication::postEvent(this, new LogEvent(std::string("STREAM_READY: ") + stream->mStreams->mId, CustomEvent_Log));
      //});

      // 连接房间失败监听回调
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

      mLiveRoom->AddEventListener(AUDIO_IN_ERROR, [&](vhall::BaseEvent &Event)->void {
         LOGD_SDK("ROOM_ERROR");
         vhall::RoomEvent* event = dynamic_cast<vhall::RoomEvent*>(&Event);
         std::string msg("AUDIO_IN_ERROR  error");
         if (event) {
            msg = event->mMessage;
         }
         LOGD_SDK("msg:%s", msg.c_str());
      });

      mLiveRoom->AddEventListener(AUDIO_OUT_ERROR, [&](vhall::BaseEvent &Event)->void {
         LOGD_SDK("ROOM_ERROR");
         vhall::RoomEvent* event = dynamic_cast<vhall::RoomEvent*>(&Event);
         std::string msg("AUDIO_OUT_ERROR  error");
         if (event) {
            msg = event->mMessage;
         }
         LOGD_SDK("msg:%s", msg.c_str());
      });

      //互动配置混流和大屏准备好事件
      // 连接房间失败监听回调
      mLiveRoom->AddEventListener(STREAM_READY, [&, option](vhall::BaseEvent &Event)->void {
         LOGD_SDK("STREAM_READY");
         vhall::StreamEvent *event = dynamic_cast<vhall::StreamEvent *>(&Event);
         if (event) {
            int mediaType = event->mStreams->mStreamType;
            LOGD_SDK("STREAM_READY %d", mediaType);
            if (mediaType == vlive::VHStreamType_MediaFile) {
               return;
            }
            else {

               CTaskEvent roomConnectTask;
               roomConnectTask.mEventType = TaskEventEnum_ON_STREAM_MIXED;
               roomConnectTask.mHasAudio = event->mStreams->HasAudio();
               roomConnectTask.mHasVideo = event->mStreams->HasVideo();
               std::string context = event->mStreams->mStreamAttributes;

               std::string text = context;
               //if (!text.empty()) {
                  int type = ParamPushType(text);
                  if (type == VHDoubleStreamType_Camera) {
                     mediaType = VHStreamType_Auxiliary_CamerCapture;
                  }
                  else if (type == VHDoubleStreamType_Desktop) {
                     mediaType = VHStreamType_Stu_Desktop;
                  }
               //}

               roomConnectTask.mStreamType = mediaType;
               roomConnectTask.streamId = std::string(event->mStreams->mStreamId);
               roomConnectTask.mJoinUid = std::string(event->mStreams->GetUserId());
               roomConnectTask.mStreamAttributes = event->mStreams->mStreamAttributes;
               PostTaskEvent(roomConnectTask);
            }
            LOGD_SDK("STREAM_READY end");
         }
      });

      //互动配置混流和大屏准备好事件
      // 连接房间失败监听回调
      mLiveRoom->AddEventListener(ON_STREAM_MIXED, [&, option](vhall::BaseEvent &Event)->void {
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
               //std::string context = event->mStream->mStreamAttributes;
               //if (!context.empty()) {
               //   int type = ParamPushType(context);
               //   if (type == 1) {
               //      mediaType = VHStreamType_Auxiliary_CamerCapture;
               //   }
               //}
               roomConnectTask.mStreamType = mediaType;
               roomConnectTask.streamId = std::string(event->mStream->mStreamId);
               roomConnectTask.mJoinUid = std::string(event->mStream->GetUserId());
               roomConnectTask.mStreamAttributes = event->mStream->mStreamAttributes;
               PostTaskEvent(roomConnectTask);
            }
         }
         LOGD_SDK("ON_STREAM_MIXED end");
      });
        /*mLiveRoom->AddEventListener(ON_STREAM_MIXED, [&, option](vhall::BaseEvent &Event)->void {
            LOGD_SDK("ON_STREAM_MIXED");
            vhall::AddStreamEvent* event = dynamic_cast<vhall::AddStreamEvent*>(&Event);
            if (event) {
                CTaskEvent roomConnectTask;
                roomConnectTask.mEventType = TaskEventEnum_ON_STREAM_MIXED;
                roomConnectTask.mHasAudio = event->mStream->HasAudio();
                roomConnectTask.mHasVideo = event->mStream->HasVideo();
                int mediaType = event->mStream->mStreamType;
                std::string context = event->mStream->mStreamAttributes;

                std::string text = context;
                if (!text.empty()) {
                   int type = ParamPushType(text);
                   if (type == VHDoubleStreamType_Camera) {
                      mediaType = VHStreamType_Auxiliary_CamerCapture;
                   }
                   else if (type == VHDoubleStreamType_Desktop) {
                      mediaType = VHStreamType_Stu_Desktop;
                   }
                }

                roomConnectTask.mStreamType = mediaType;
                roomConnectTask.streamId = std::string(event->mStream->mStreamId);
                roomConnectTask.mJoinUid = std::string(event->mStream->GetUserId());
                roomConnectTask.mStreamAttributes = event->mStream->mStreamAttributes;
                PostTaskEvent(roomConnectTask);
            }
            LOGD_SDK("ON_STREAM_MIXED end");
        });*/

        //sdk检测到网络异常正在尝试进行重连。
        mLiveRoom->AddEventListener(ROOM_RECONNECTING, [&](vhall::BaseEvent &Event)->void {
           LOGD_SDK("ROOM_RECONNECTING");
           CTaskEvent roomConnectTask;
           roomConnectTask.mEventType = TaskEventEnum_ROOM_RECONNECTING;
           PostTaskEvent(roomConnectTask);
           LOGD_SDK("ROOM_RECONNECTING end");
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
      //远端流订阅失败,重新订阅。
      mLiveRoom->AddEventListener(STREAM_RECONNECT, [&, curJoinUid](vhall::BaseEvent &Event)->void {
         LOGD_SDK("STREAM_RECONNECT");
         vhall::ReconnectStreamEvent *newStream = static_cast<vhall::ReconnectStreamEvent *>(&Event);
         std::string joinUid = newStream->NewStream->GetUserId();
         if (newStream) {
            LOGD_SDK("STREAM_RECONNECT joinUid:%s stream_id:%s bAutoSubScribeStream:%d", joinUid.c_str(), newStream->NewStream->GetID(), (int)bAutoSubScribeStream);
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
            if (streamEvent->mMessage.compare("MsgCreateSDPFail") == 0
               || strcmp(stream_id.c_str(), mLocalStreamId.c_str()) == 0
               || strcmp(stream_id.c_str(), mLocalAuxiliaryStreamId.c_str()) == 0
               || strcmp(stream_id.c_str(), mDesktopStreamId.c_str()) == 0
               || strcmp(stream_id.c_str(), mMediaFileStreamId.c_str()) == 0
               || strcmp(stream_id.c_str(), mSoftwareStreamId.c_str()) == 0
               || (strcmp(stream_id.c_str(), "local") == 0  && streamEvent->mStreams->mStreamAttributes.length() == 0 )
            ) 
            {
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
            
            std::string context = AddStream->mStreams->mStreamAttributes;
            int type = ParamPushType(context);
            if (type == VHDoubleStreamType_Camera) {
               
               roomConnectTask.mStreamType = VHStreamType_Auxiliary_CamerCapture;
            }
            else if (type == VHDoubleStreamType_Desktop) {
               roomConnectTask.mStreamType = VHStreamType_Desktop;
            }
            else {
               roomConnectTask.mStreamType = AddStream->mStreams->mStreamType;
            }
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
            //if (!context.empty()) {
               int type = ParamPushType(context);
               //if (type <= 0) {
               //   return;
               //}

            //}
            std::wstring user_id = SubScribeStreamManager::String2WString(join_uid);
            LOGD_SDK("STREAM_ADDED userId:%s curJoinUid:%s stream:%s user_id: %s  mStreamType: %d type: %d", 
               join_uid.c_str(), curJoinUid.c_str(),
               stream_id.c_str(), user_id.c_str(), AddStream->mStream->mStreamType, type);
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
               //if (!context.empty()) {
                  int type = ParamPushType(context);
                  if (type == VHDoubleStreamType_Camera) {
                     mediaType = std::to_string(VHStreamType_Auxiliary_CamerCapture);
                     curStreamType = VHStreamType_Auxiliary_CamerCapture;
                  }
                  else if (type == VHDoubleStreamType_Desktop) {
                     mediaType = std::to_string(AddStream->mStreams->mStreamType);
                  }
               //}
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

      // 远端流退出监听回调
      mLiveRoom->AddEventListener(STREAM_REMOVED, [&](vhall::BaseEvent &Event)->void {
         LOGD_SDK("STREAM_REMOVED");
         vhall::StreamEvent *AddStream = static_cast<vhall::StreamEvent *>(&Event);
         if (AddStream && AddStream->mStreams) {
            std::string join_uid = AddStream->mStreams->GetUserId();
            std::string stream_id = AddStream->mStreams->mId;
            std::wstring join_uid_temp = SubScribeStreamManager::String2WString(join_uid);
            LOGD_SDK("STREAM_REMOVED join_uid:%s stream_id:%s streamType:%d", join_uid.c_str(), stream_id.c_str(), AddStream->mStreams->mStreamType);
            int curStreamType = AddStream->mStreams->mStreamType;
            std::string mediaType = std::to_string(AddStream->mStreams->mStreamType);
            std::string context = AddStream->mStreams->mStreamAttributes;
            //if (!context.empty()) {
               int type = ParamPushType(context);
               if (type == VHDoubleStreamType_Camera) {
                  //mediaType = std::to_string(VHStreamType_Auxiliary_CamerCapture);
                  curStreamType = VHStreamType_Auxiliary_CamerCapture;
               }
               else if (type == VHDoubleStreamType_Desktop) {
                  //mediaType = std::to_string(AddStream->mStreams->mStreamType);
                  curStreamType = VHStreamType_Stu_Desktop;
               }
            //}
            //std::wstring subStreamIndex = join_uid_temp + SubScribeStreamManager::String2WString(mediaType);

            CTaskEvent roomConnectTask;
            roomConnectTask.mEventType = TaskEventEnum_STREAM_REMOVED;
            roomConnectTask.mStreamType = curStreamType;// AddStream->mStreams->mStreamType;
            roomConnectTask.streamId = stream_id;
            roomConnectTask.mJoinUid = join_uid;
            PostTaskEvent(roomConnectTask);
            LOGD_SDK("STREAM_REMOVED end");
         }
      });

      //远端流订阅失败,重新订阅。
      mLiveRoom->AddEventListener(STREAM_RECONNECT, [&, curJoinUid](vhall::BaseEvent &Event)->void {
         LOGD_SDK("STREAM_RECONNECT");
         vhall::ReconnectStreamEvent *newStream = static_cast<vhall::ReconnectStreamEvent *>(&Event);
         std::string joinUid = newStream->NewStream->GetUserId();
         if (newStream) {
            LOGD_SDK("STREAM_RECONNECT joinUid:%s stream_id:%s bAutoSubScribeStream:%d", joinUid.c_str(), newStream->NewStream->GetID(), (int)bAutoSubScribeStream);
            if (SubScribeStreamManager::String2WString(joinUid) != curJoinUid) {
               std::string stream_id = newStream->NewStream->mId;
               CTaskEvent task;
               task.mEventType = TaskEventEnum_RE_SUBSCRIB;
               task.streamId = stream_id;
               task.mStreamType = newStream->NewStream->mStreamType;
               task.mSleepTime = 3000;  //延迟三秒
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
         if (streamEvent && streamEvent->mStreams&& IsStreamExit(streamEvent->mStreams->GetID())) {
            join_uid = streamEvent->mStreams->GetUserId();
            stream_id = streamEvent->mStreams->GetID();

            if( strcmp(stream_id.c_str(), mLocalStreamId.c_str())==0  
               || strcmp(stream_id.c_str(), mLocalAuxiliaryStreamId.c_str()) == 0
               || strcmp(stream_id.c_str(), mDesktopStreamId.c_str()) == 0 
               || strcmp(stream_id.c_str(), mMediaFileStreamId.c_str()) == 0
               || strcmp(stream_id.c_str(), mSoftwareStreamId.c_str()) == 0 ){
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

         }
      });

      LOGD_SDK("Connect room");
      mLiveRoom->Connect();
   }
   else {
      bRet = vlive::VhallLive_NO_OBJ;
   }
   return bRet;

}

bool WebRtcSDK::IsStreamExit(std::string id)
{
   //bool bRef = false;
   if (nullptr != mLocalStream) {
      if (mLocalStreamId.compare(id) == 0) {
         return true;
      }
   }

   if (nullptr != mLocalAuxiliaryStream) {
      if (mLocalAuxiliaryStreamId.compare(id) == 0) {
         return true;
      }
   }

   if (nullptr != mMediaStream) {
      if (mMediaFileStreamId.compare(id) == 0) {
         return true;
      }
   }

   if (nullptr != mDesktopStream) {
      if (mDesktopStreamId.compare(id) == 0) {
         return true;
      }
   }

   if (nullptr != mSoftwareStream) {
      if (mSoftwareStreamId.compare(id) == 0) {
         return true;
      }
   }    
   return false;
}

void WebRtcSDK::ClearAllLocalStream() {
   mLocalStreamThreadLock.Lock();
   RelaseStream(mLocalStream);
   mLocalStreamId = "";
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

void WebRtcSDK::DisableSubScribeStream(){
   LOGD_SDK("DisableSubScribeStream");
   //mSubScribeStreamManager.StopSubScribeStream();
}

void WebRtcSDK::StopRecvAllRemoteStream() {
   LOGD_SDK("StopRecvAllRemoteStream");
   mSubScribeStreamManager.StopSubScribeStream();
}

void WebRtcSDK::ClearSubScribeStream()
{
   LOGD_SDK("ClearSubScribeStream");
   mSubScribeStreamManager.ClearSubScribeStream();
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
    ClearAllLocalStream();
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

    //ClearSubScribeStream();
    SetEnableConfigMixStream(false);
    mLocalStreamId.clear();
    mDesktopStreamId.clear();
    mMediaFileStreamId.clear();
    mCurPlayFile.clear();
    mbIsWebRtcRoomConnected = false;
    mbIsEnablePlayFileAndShare = true; //判断当前用户是否能够进行插播和桌面共享，插播和桌面共享只能处理一种，属于互斥的。
    mbIsMixedAlready = false;

    CThreadLockHandle lockHandle(&mRoomThreadLock);
    if (mLiveRoom) {
        mLiveRoom->Disconnect();
        LOGD_SDK("Disconnect end");
        mLiveRoom.reset();
        mLiveRoom = nullptr;
    }
    bRet = vlive::VhallLive_OK;
    LOGD_SDK("DisConnetWebRtcRoom end");
    return bRet;
}

bool WebRtcSDK::isConnetWebRtcRoom()
{
   return nullptr!=mLiveRoom;
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

bool WebRtcSDK::ChangeSubScribeUserSimulCast(const std::wstring& user_id,vlive::VHStreamType streamType, VHSimulCastType type) {
    std::string mediaType = std::to_string(streamType);
    std::wstring subStreamIndex = user_id + SubScribeStreamManager::String2WString(mediaType);
    mSubScribeStreamManager.ChangeSubScribeUserSimulCast(subStreamIndex, type);
    return true;
}

bool WebRtcSDK::IsWebRtcRoomConnected() {
    return mbIsWebRtcRoomConnected;
}

void WebRtcSDK::ChangePreViewMic(int micIndex) {
    LOGD_SDK("get mPreviewStreamThreadLock");
    if (mPreviewLocalStream) {
        mPreviewLocalStream->SetAudioInDevice(micIndex);
    }
}

bool WebRtcSDK::IsSupported(const std::string& devGuid, VideoProfileIndex index) {
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

//
//bool WebRtcSDK::IsSupported(const std::string devGuid, VideoProfileIndex index) {
//   vhall::VHTools tl;
//   const std::vector<std::shared_ptr<vhall::VideoDevProperty>> CameraList = tl.VideoInDeviceList();
//   for (int i = 0; i < CameraList.size(); i++) {
//      std::shared_ptr<vhall::VideoDevProperty> videoDev = CameraList.at(i);
//      if (devGuid == videoDev->mDevId) {
//         if (tl.IsSupported(videoDev, index)) {
//            LOGD_SDK("mCurrentCameraIndex VhallLive_SUPPROT id:%s", devGuid.c_str());
//            return true;
//         }
//         else {
//            LOGD_SDK("mCurrentCameraIndex VhallLive_NOT_SUPPROT id:%s", devGuid.c_str());
//            return false;
//         }
//      }
//   }
//   LOGD_SDK("not device");
//   return false;
//}
/**
* 摄像头画面预览，当预览结束之后需要停止预览，否则摄像头将被一直占用
*/
int WebRtcSDK::PreviewCamera(void* wnd, const std::string& devGuid, VideoProfileIndex index, int micIndex /*= -1*/) {
    if (devGuid.length() <= 0) {
        return vlive::VhallLive_NO_DEVICE;
    }
    LOGD_SDK("devGuid :%s", devGuid.c_str());
    CTaskEvent task;
    task.mEventType = TaskEventEnum_StartPreviewLocalCapture;
    task.devId = devGuid;
    task.index = index;
    task.wnd = wnd;
    LOGD_SDK("TaskEventEnum_StartLocalCapture");
    PostTaskEvent(task);
    LOGD_SDK("TaskEventEnum_StartLocalCapture");
    mbStopPreviewCamera = false;
    return vlive::VhallLive_OK;
}

int WebRtcSDK::HandlePreviewCamera(void* wnd, const std::string& devGuid, VideoProfileIndex index, int micIndex /*= -1*/) {
    bool bRet = false;
    std::string camera_guid = devGuid;
    previewWnd = HWND(wnd);
    previewStreamConfig.mVideo = devGuid.length() > 0 ? true : false;
    previewStreamConfig.mAudio = false;
    previewStreamConfig.videoOpt.mProfileIndex = index;
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

            mPreviewLocalStream->AddEventListener(AUDIO_DENIED, [&](vhall::BaseEvent &Event)->void {
                LOGD_SDK("AUDIO_DENIED");
                //音频设备异常，请调整系统设置中音频设备的音频采样率为44100格式
                CTaskEvent task;
                task.mEventType = TaskEventEnum_PREVIE_PLAY;
                task.mErrMsg = AUDIO_DENIED;
                PostTaskEvent(task);
            });
            bInit = true;
        }
    }
    if (bInit) {
        mPreviewLocalStream->Init();
        vhall::LiveVideoFilterParam filter;
        filter.enableEdgeEnhance = false;
        mPreviewLocalStream->SetFilterParam(filter);
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

int WebRtcSDK::GetMicVolumValue() {
    return mPreviewAudioVolume.GetAudioVolume();
}

/*
* 停止摄像头预览
*/
int WebRtcSDK::StopPreviewCamera() {
    LOGD_SDK("%s start",__FUNCTION__);
    CThreadLockHandle lockHandle(&mPreviewStreamThreadLock);
    if (mPreviewLocalStream) {
        LOGD_SDK("RemoveAllEventListener");
        mPreviewLocalStream->RemoveAllEventListener();
        LOGD_SDK("RelaseStream start");
        RelaseStream(mPreviewLocalStream);
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

int WebRtcSDK::StartLocalCapturePicture(const std::string filePath,VideoProfileIndex index, bool doublePush/* = false*/) {
    LOGD_SDK("get StartLocalCapturePicture :%d filePath:%s mCurrentMicIndex:%d", index, filePath.c_str(), (int)mCurrentMicIndex);
    mCameraStreamConfig.mVideo = true;
    if (mCurrentMicIndex >= 0) {
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
   mCurrentCameraGuid = "";
   mLastCaptureIsCamera = false;
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
   CThreadLockHandle lockHandle(&mLocalStreamThreadLock);
   LOGD_SDK("get mLocalStreamThreadLock");

   if (mLocalStream == nullptr) {
      LOGD_SDK(" make mCameraStreamConfig ");
      mLocalStream = std::make_shared<vhall::VHStream>(mCameraStreamConfig);
      if (mLocalStream) {
         //当调用摄像头，会触发ACCESS_ACCEPTED回掉。
         mLocalStream->AddEventListener(ACCESS_ACCEPTED, [&](vhall::BaseEvent &Event)->void {
            LOGD_SDK("ACCESS_ACCEPTED");
            CTaskEvent task;
            task.mEventType = TaskEventEnum_CaptureLocalStream;
            task.mErrMsg = ACCESS_ACCEPTED;
            PostTaskEvent(task);
         });
         // 获取本地摄像头失败监听回调
         mLocalStream->AddEventListener(ACCESS_DENIED, [&](vhall::BaseEvent &Event)->void {
            LOGD_SDK("ACCESS_DENIED");
            CTaskEvent task;
            task.mEventType = TaskEventEnum_CaptureLocalStream;
            task.mErrMsg = ACCESS_DENIED;
            PostTaskEvent(task);
         });
         mLocalStream->AddEventListener(CAMERA_LOST, [&](vhall::BaseEvent &Event)->void {
            LOGD_SDK("CAMERA_LOST");
            CTaskEvent task;
            task.mEventType = TaskEventEnum_CaptureLocalStream;
            task.mErrMsg = CAMERA_LOST;
            PostTaskEvent(task);
         });
         mLocalStream->AddEventListener(VIDEO_CAPTURE_ERROR, [&](vhall::BaseEvent &Event)->void {
            LOGD_SDK("VIDEO_CAPTURE_ERROR");
            CTaskEvent task;
            task.mEventType = TaskEventEnum_CaptureLocalStream;
            task.mErrMsg = VIDEO_CAPTURE_ERROR;
            PostTaskEvent(task);
         });
         mLocalStream->AddEventListener(STREAM_SOURCE_LOST, [&](vhall::BaseEvent &Event)->void {
            LOGD_SDK("STREAM_SOURCE_LOST");
            CTaskEvent task;
            task.mEventType = TaskEventEnum_CaptureLocalStream;
            task.mErrMsg = STREAM_SOURCE_LOST;
            PostTaskEvent(task);
         });
         mLocalStream->AddEventListener(VIDEO_DENIED, [&](vhall::BaseEvent &Event)->void {
            LOGD_SDK("VIDEO_DENIED");
            CTaskEvent task;
            task.mEventType = TaskEventEnum_CaptureLocalStream;
            task.mErrMsg = VIDEO_DENIED;
            PostTaskEvent(task);
         });
         mLocalStream->AddEventListener(AUDIO_DENIED, [&](vhall::BaseEvent &Event)->void {
            LOGD_SDK("AUDIO_DENIED");
            CTaskEvent task;
            task.mEventType = TaskEventEnum_CaptureLocalStream;
            task.mErrMsg = ACCESS_ACCEPTED;
            PostTaskEvent(task);
         });
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
      vhall::LiveVideoFilterParam filter;
      filter.enableEdgeEnhance = false;
      mLocalStream->SetFilterParam(filter);
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
            VHStreamType  iStreamType = CalcStreamType(mCameraStreamConfig.mAudio, mCameraStreamConfig.mVideo);
            mWebRtcSDKEventReciver->OnOpenCaptureCallback(iStreamType, vlive::VHCapture_ACCESS_DENIED, mCameraStreamConfig.mVideo, mCameraStreamConfig.mAudio);
         }
      }
      else {
         mediaInfo->mI420Data = mI420Data;
         mLocalStream->mVideo = mCameraStreamConfig.mVideo;
         mLocalStream->Getlocalstream(mediaInfo);
         LOGD_SDK("Getlocalstream");
         if (mLocalStream && bChangeCapability) {
            mLocalStream->ResetCapability(mCameraStreamConfig.videoOpt);
         }
      }
   }
   else {
      if (mWebRtcSDKEventReciver) {
         LOGD_SDK("vlive::VHCapture_ACCESS_DENIED");
         VHStreamType  iStreamType = CalcStreamType(mCameraStreamConfig.mAudio, mCameraStreamConfig.mVideo);
         mWebRtcSDKEventReciver->OnOpenCaptureCallback(iStreamType, vlive::VHCapture_ACCESS_DENIED, mCameraStreamConfig.mVideo, mCameraStreamConfig.mAudio);
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
int WebRtcSDK::StartLocalCapturePlayer(const int dev_index, const std::wstring dev_id, const int volume) {
    LOGD_SDK("StartLocalCapturePlayer volume:%d dev_index:%d dev_id:%s", volume, dev_index, SubScribeStreamManager::WString2String(dev_id).c_str());   
    //vhall::VHStream::SetLoopBackVolume(volume);
    //int nRet = vhall::VHStream::SetAudioInDevice(dev_index, dev_id) ? VhallLive_OK : VhallLive_NO_DEVICE;

    CTaskEvent task;
    task.mEventType = TaskEventEnum_SET_PLAYER_DEV;
    task.mVolume = volume;
    task.devIndex = dev_index;
    task.desktopCaptureId = dev_id;
    PostTaskEvent(task);

    mCurrentMicIndex = dev_index;
    mDesktopCaptureId = dev_id;
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
//   int WebRtcSDK::ChangeUserPlayDev(int index) {
//   mSubScribeStreamManager.ChangeUserPlayDev(index, mCurPlayVol);
//   mCurrentPlayerIndex = index;
//   vhall::VHStream::SetAudioOutDevice(index);
//   LOGD_SDK("index:%d mCurPlayVol:%d", index, (int)mCurPlayVol);
//   return 0;
//}

int WebRtcSDK::StopLocalCapturePlayer() {
    LOGD_SDK("StopLocalCapturePlayer :%d", (int)mCurrentMicIndex);
    int nRet = vhall::VHStream::SetAudioInDevice(mCurrentMicIndex, L"") ? VhallLive_OK : VhallLive_NO_DEVICE;
    mDesktopCaptureId = L"";
    mLocalStreamReceive = nullptr;
    LOGD_SDK("StopLocalCapturePlayer :%d", nRet);
    return nRet;

}

int WebRtcSDK::StartLocalCapture(const std::string devId, VideoProfileIndex index, bool doublePush) {
    LOGD_SDK(" devId :%s index:%d", devId.c_str(), index);
    if (devId.size() > 0) {
        mCameraStreamConfig.mVideo = true;
    }
    else {
        mCameraStreamConfig.mVideo = false;
    }

    if (mCurrentMicIndex >= 0) {
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
    StopPushLocalAuxiliaryStream();
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
        vhall::LiveVideoFilterParam filter;
        filter.enableEdgeEnhance = false;
        mLocalAuxiliaryStream->SetFilterParam(filter);
        LOGD_SDK("Init");
        std::shared_ptr<vhall::MediaCaptureInfo> mediaInfo = std::make_shared<vhall::MediaCaptureInfo>();
        mediaInfo->VideoDevID = devId;
        mLocalAuxiliaryStream->Getlocalstream(mediaInfo);
        LOGD_SDK("Getlocalstream");
    }
    else {
        if (mWebRtcSDKEventReciver) {
            LOGD_SDK("vlive::VHCapture_ACCESS_DENIED");
			VHStreamType  iStreamType = CalcStreamType(mCameraStreamConfig.mAudio, mCameraStreamConfig.mVideo);
            mWebRtcSDKEventReciver->OnOpenCaptureCallback(iStreamType, vlive::VHCapture_ACCESS_DENIED, mCameraStreamConfig.mVideo, mCameraStreamConfig.mAudio);
        }
    }
    LOGD_SDK("VhallLive_OK");
    return vlive::VhallLive_OK;
}


int WebRtcSDK::HandleStartLocalCapture(const std::string devId, VideoProfileIndex index, bool doublePush) {
   bool bChangeCapability = false;
   
   if (doublePush) { //设置推 大/小 流
      if (mCameraStreamConfig.mNumSpatialLayers != 2) {//非大小流
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

   if (mCameraStreamConfig.videoOpt.mProfileIndex != index) {//改变分辨率
      bChangeCapability = true;
   }

   if (mCurrentCameraGuid != devId) {//改变摄像头
      mCameraStreamConfig.videoOpt.devId = devId;
      bChangeCapability = true;
   }

   //if (mCameraStreamConfig.mVideo && mCameraStreamConfig.mStreamType != VHStreamType_AVCapture) {
   //   mCameraStreamConfig.mStreamType = VHStreamType_AVCapture;
   //   mCameraStreamConfig.mNumSpatialLayers = 2;
   //   if (mLocalStream) {
   //      mLocalStream->mStreamType = VHStreamType_AVCapture;
   //   }
   //   bChangeCapability = true;
   //   StopPushLocalStream();
   //}
   //else if (!mCameraStreamConfig.mVideo && mCameraStreamConfig.mStreamType != VHStreamType_PureAudio) {
   //   mCameraStreamConfig.mStreamType = VHStreamType_PureAudio;
   //   mCameraStreamConfig.mNumSpatialLayers = 0;
   //   if (mLocalStream) {
   //      mLocalStream->mStreamType = VHStreamType_PureAudio;
   //   }
   //   bChangeCapability = true;
   //   StopPushLocalStream();
   //}

    bool bRet = false;
    bool bNeedInit = false;
    mCameraStreamConfig.videoOpt.mProfileIndex = index;
    mCameraStreamConfig.mLocal = true;
    CThreadLockHandle lockHandle(&mLocalStreamThreadLock);

    if (mLocalStream && bChangeCapability) {
        mLocalStream->ResetCapability(mCameraStreamConfig.videoOpt);
    }
    mCurrentCameraGuid = devId;
    LOGD_SDK(" get mLocalStreamThreadLock devId： %s", devId.c_str());
    if (mLocalStream == nullptr) {
        LOGD_SDK(" make mCameraStreamConfig ");
        mLocalStream = std::make_shared<vhall::VHStream>(mCameraStreamConfig);
        if (mLocalStream) {
           vhall::LiveVideoFilterParam filter;
           filter.enableEdgeEnhance = false;
           mLocalStream->SetFilterParam(filter);
            //当调用摄像头，会触发ACCESS_ACCEPTED回掉。
            mLocalStream->AddEventListener(ACCESS_ACCEPTED, [&](vhall::BaseEvent &Event)->void {
                LOGD_SDK("ACCESS_ACCEPTED");
                CTaskEvent task;
                task.mEventType = TaskEventEnum_CaptureLocalStream;
                task.mErrMsg = ACCESS_ACCEPTED;
                PostTaskEvent(task);
            });
            // 获取本地摄像头失败监听回调
            mLocalStream->AddEventListener(ACCESS_DENIED, [&](vhall::BaseEvent &Event)->void {
                LOGD_SDK("ACCESS_DENIED");
                CTaskEvent task;
                task.mEventType = TaskEventEnum_CaptureLocalStream;
                task.mErrMsg = ACCESS_DENIED;
                PostTaskEvent(task);
            });
            //摄像头移除
            mLocalStream->AddEventListener(CAMERA_LOST, [&](vhall::BaseEvent &Event)->void {
                LOGD_SDK("STREAM_SOURCE_LOST");
                CTaskEvent task;
                task.mEventType = TaskEventEnum_CaptureLocalStream;
                task.mErrMsg = CAMERA_LOST;
                PostTaskEvent(task);
            });
            mLocalStream->AddEventListener(VIDEO_CAPTURE_ERROR, [&](vhall::BaseEvent &Event)->void {
               LOGD_SDK("VIDEO_CAPTURE_ERROR");
               CTaskEvent task;
               task.mEventType = TaskEventEnum_CaptureLocalStream;
               task.mErrMsg = VIDEO_CAPTURE_ERROR;
               PostTaskEvent(task);
            });
            mLocalStream->AddEventListener(STREAM_SOURCE_LOST, [&](vhall::BaseEvent &Event)->void {
               LOGD_SDK("STREAM_SOURCE_LOST");
               CTaskEvent task;
               task.mEventType = TaskEventEnum_CaptureLocalStream;
               task.mErrMsg = CAMERA_LOST;
               PostTaskEvent(task);
            });
            mLocalStream->AddEventListener(VIDEO_DENIED, [&](vhall::BaseEvent &Event)->void {
                CTaskEvent task;
                task.mEventType = TaskEventEnum_CaptureLocalStream;
                task.mErrMsg = VIDEO_DENIED;
                PostTaskEvent(task);
            });
            mLocalStream->AddEventListener(AUDIO_DENIED, [&](vhall::BaseEvent &Event)->void {
                LOGD_SDK("AUDIO_DENIED");
                CTaskEvent task;
                task.mEventType = TaskEventEnum_CaptureLocalStream;
                task.mErrMsg = AUDIO_DENIED;
                PostTaskEvent(task);
            });
        }
        bNeedInit = true;
        LOGD_SDK("make new local stream");
    }
    if (mLocalStream) {
        mLocalStreamIsCapture = true;
    }
    //if (bNeedInit && mLocalStream) {
    //    //初始化
    //    LOGD_SDK("Init");
    //    mLocalStream->Init();
    //}

    if (bNeedInit && mLocalStream) {
       //初始化
       LOGD_SDK("Init");
       mLocalStream->Init();
       vhall::LiveVideoFilterParam filter;
       filter.enableEdgeEnhance = false;
       mLocalStream->SetFilterParam(filter);
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

        LOGD_SDK("mediaInfo->VideoDevID : %s", mediaInfo->VideoDevID.c_str());
        mLocalStream->Getlocalstream(mediaInfo);
        mLastCaptureIsCamera = true;
        LOGD_SDK("end");
    }
    else {
   //     if (mWebRtcSDKEventReciver) {
   //         LOGD_SDK("vlive::VHCapture_ACCESS_DENIED");
			//VHStreamType  iStreamType = CalcStreamType(mCameraStreamConfig.mAudio, mCameraStreamConfig.mVideo);
   //         mWebRtcSDKEventReciver->OnOpenCaptureCallback(iStreamType, vlive::VHCapture_ACCESS_DENIED, mCameraStreamConfig.mVideo, mCameraStreamConfig.mAudio);
   //     }
    }
    LOGD_SDK("VhallLive_OK");
    return vlive::VhallLive_OK;
}

void WebRtcSDK::StopLocalCapture() {
    LOGD_SDK("");
    {
        CThreadLockHandle lockHandle(&mLocalStreamThreadLock);
        LOGD_SDK("mLocalStreamThreadLock");
        RelaseStream(mLocalStream);
        mLocalStreamId = "";
    }
    mLocalStreamIsCapture = false;
    mbIsOpenCamera = true;
    mbIsOpenMic = true;
    mLocalStreamId.clear();
    mCurrentCameraGuid.clear();
    LOGD_SDK("StopLocalCapture ok");
}

bool WebRtcSDK::GetCaptureStreamType(const std::wstring& user_id, vlive::VHStreamType streamType, vlive::CaptureStreamAVType type) {
    LOGD_SDK("user_id:%ws streamType:%d type:%d", user_id.c_str(), streamType, type);
    if (user_id == mWebRtcRoomOption.strUserId) {
        switch (streamType)
        {
		case vlive::VHStreamType_AUDIO:
		case vlive::VHStreamType_VIDEO:
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
    if (mLiveRoom && mLocalStream ) {
        LOGD_SDK("Publish mLocalStream");
        bool hasVideo = mLocalStream->HasVideo();
        bool hasAudio = mLocalStream->HasAudio();
        std::string StreamId = mLocalStreamId;
		std::string join_uid = SubScribeStreamManager::WString2String(mWebRtcRoomOption.strUserId);
      LOGD_SDK("Publish mLocalStream mLocalStreamId:%s join_uid %d", mLocalStreamId.c_str(), join_uid);
        mLiveRoom->Publish(mLocalStream, [&, StreamId, hasVideo, hasAudio , join_uid](const vhall::PublishRespMsg& resp) -> void {
            if (resp.mResult == SDK_SUCCESS) {
                mLocalStreamId = resp.mStreamId;
                LOGD_SDK("Publish mLocalStream mLocalStreamId:%s join_uid %d", mLocalStreamId.c_str(), join_uid);
                //推流成功移动到 “ON_STREAM_MIXED”事件处
     //           if (mWebRtcSDKEventReciver) {
					//std::string streamAttributes;
     //               mWebRtcSDKEventReciver->OnPushStreamSuc(vlive::VHStreamType_AVCapture, mLocalStreamId, hasVideo, hasAudio, streamAttributes);
     //           }
            
                
				/*CTaskEvent roomConnectTask;
				roomConnectTask.mEventType = TaskEventEnum_ON_STREAM_MIXED;
				roomConnectTask.mHasAudio = hasAudio;
				roomConnectTask.mHasVideo = hasVideo;
				roomConnectTask.mStreamType = VHStreamType_AVCapture;
				roomConnectTask.streamId = resp.mStreamId;
				roomConnectTask.mJoinUid = join_uid;
            LOGD_SDK("***iStreamMixed: %d*** mStreamType :%d streamId:%s", ++iStreamMixed, roomConnectTask.mStreamType, roomConnectTask.streamId.c_str());

				PostTaskEvent(roomConnectTask);*/
            }
            else {
                LOGD_SDK("Publish mLocalStream err:%s resp.mCode:%d", resp.mMsg.c_str(), resp.mCode);
                if (vhall::StreamPublished == resp.mCode || vhall::StreamIsEmpty == resp.mCode) {
                    return;
                }
                if (mWebRtcSDKEventReciver) {
					VHStreamType  iStreamType = CalcStreamType(hasAudio, hasVideo);
                    mWebRtcSDKEventReciver->OnPushStreamError(StreamId, iStreamType, resp.mCode, resp.mMsg);
                }
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
                LOGD_SDK("UnPublish LocalStream msg:%s", resp.mMsg.c_str());
                if (mWebRtcSDKEventReciver) {
					VHStreamType  iStreamType = CalcStreamType(mCameraStreamConfig.mAudio, mCameraStreamConfig.mVideo);
                    if (resp.mResult == SDK_SUCCESS) {
                        mWebRtcSDKEventReciver->OnStopPushStreamCallback(iStreamType, vlive::VhallLive_OK, resp.mMsg);
                    }
                    else {
                        mWebRtcSDKEventReciver->OnStopPushStreamCallback(iStreamType, vlive::VhallLive_ERROR, resp.mMsg);
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
    Options.precast_pic_exist = true;

    if (mLiveRoom) {
        Options.layoutMode = layoutMode; 
        mLiveRoom->configRoomBroadCast(Options, [&, fun](const vhall::RespMsg& resp) -> void {
            LOGD_SDK("configRoomBroadCast  mResult:%s msg:%s", resp.mResult.c_str(), resp.mMsg.c_str());
            if (fun) {
                fun(resp.mResult, resp.mMsg);
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

int WebRtcSDK::StartConfigBroadCast(LayoutMode layoutMode, BroadCastVideoProfileIndex profileIndex, bool showBorder, std::string boradColor , std::string backGroundColor, SetBroadCastEventCallback fun/* = nullptr*/) {
      LOGD_SDK("layoutMode:%d profileIndex: %d ", layoutMode, profileIndex);
      if (layoutMode > CANVAS_LAYOUT_PATTERN_GRID_9_E)
      {
         int i = 0;
      }
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
    Options.mProfileIndex = profileIndex;
    Options.publishUrl = SubScribeStreamManager::WString2String(mWebRtcRoomOption.strThirdPushStreamUrl);
    Options.backgroundColor = backGroundColor;
    Options.borderColor = boradColor;
    Options.borderWidth = 4;
    Options.borderExist = showBorder;
    Options.precast_pic_exist = true;

    if (mLiveRoom) {
        Options.layoutMode = layoutMode; 
        mLiveRoom->configRoomBroadCast(Options, [&, fun](const vhall::RespMsg& resp) -> void {
            LOGD_SDK("configRoomBroadCast  mResult:%s msg:%s", resp.mResult.c_str(), resp.mMsg.c_str());
            if (fun) {
                fun(resp.mResult, resp.mMsg);
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
                fun(resp.mResult, resp.mMsg);
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

int WebRtcSDK::StopBroadCast() {
    if (mLiveRoom) {
        LOGD_SDK("StopBroadCast");
        mLiveRoom->stopRoomBroadCast([&](const vhall::RespMsg& resp) -> void {
            LOGD_SDK("StopBroadCast callback:%s ", resp.mMsg.c_str());
        }, SDK_TIMEOUT);
    }
    return 0;
}

int WebRtcSDK::SetMixLayoutMainView(std::string stream, SetBroadCastEventCallback fun /*= nullptr*/) {
    LOGD_SDK("%s ", __FUNCTION__ );
    if (!mbIsWebRtcRoomConnected) {
        LOGD_SDK("VhallLive_ROOM_DISCONNECT");
        return vlive::VhallLive_ROOM_DISCONNECT;
    }
    if (!mbIsMixedAlready) {
        LOGD_SDK("VhallLive_SERVER_NOT_READY");
        return VhallLive_SERVER_NOT_READY;
    }        
    LOGD_SDK("");
    {
        LOGD_SDK("mainViewMutex");
        CThreadLockHandle lockHandle(&mainViewMutex);
        mMainViewCallback = fun;
    }
    {
        LOGD_SDK("mainViewMutex");
        CThreadLockHandle lockHandle(&mainViewMutex);
        mMainViewCallback = fun;
    }
    if (mLiveRoom) {
        LOGD_SDK("setMixLayoutMainScreen stream:%s", stream.c_str());
        mLiveRoom->setMixLayoutMainScreen(stream.c_str(), [&](const vhall::RespMsg& resp) -> void {
            LOGD_SDK("1526 setMixLayoutMainScreen resp.mResult %s callback:%s ", resp.mResult.c_str(), resp.mMsg.c_str());
            if (mMainViewCallback) {
                mMainViewCallback(resp.mResult,resp.mMsg);
               //mMainViewCallback("failed ", "not defined");
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
      std::string StreamId = mSoftwareStreamId;
		mLiveRoom->Publish(mSoftwareStream, [& , StreamId,join_uid](const vhall::PublishRespMsg& resp) -> void {
			if (resp.mResult == SDK_SUCCESS) {
				mSoftwareStreamId = resp.mStreamId;
				LOGD_SDK("Publish mSoftwareStreamId :%s", mSoftwareStreamId.c_str());
    //            //推流成功移动到 “ON_STREAM_MIXED”事件处
				//if (mWebRtcSDKEventReciver) {
				//	std::string streamAttributes;
				//	mWebRtcSDKEventReciver->OnPushStreamSuc(vlive::VHStreamType_SoftWare, mSoftwareStreamId, true, false, streamAttributes);
				//}
				//CTaskEvent roomConnectTask;
				//roomConnectTask.mEventType = TaskEventEnum_ON_STREAM_MIXED;
				//roomConnectTask.mStreamType = VHStreamType_SoftWare;
				//roomConnectTask.streamId = resp.mStreamId;
				//roomConnectTask.mJoinUid = join_uid;
    //        LOGD_SDK("***iStreamMixed: %d*** mStreamType :%d streamId:%s", ++iStreamMixed, roomConnectTask.mStreamType, roomConnectTask.streamId.c_str());
				//PostTaskEvent(roomConnectTask);
			}
			else {
				LOGD_SDK("Publish mSoftwareStream err:%s resp.mCode:%d", resp.mMsg.c_str(), resp.mCode);
				if (vhall::StreamPublished == resp.mCode || vhall::StreamIsEmpty == resp.mCode) {
					return;
				}
				if (mWebRtcSDKEventReciver) {
					mWebRtcSDKEventReciver->OnPushStreamError(StreamId,vlive::VHStreamType_SoftWare, resp.mCode, resp.mMsg);
				}
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

   //mDesktopStreamIsCapture = true;
	CThreadLockHandle lockHandle(&mSoftwareStreamThreadLock);
	if (mSoftwareStream) {
		//StopPushDesktopStream();
		/*RelaseStream(mSoftwareStream);
		mSoftWareStreamIsCapture = false;*/
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

		mSoftSourceStreamConfig.videoOpt.mProfileIndex = RTC_VIDEO_PROFILE_720P_16x9_H/*VIDEO_PROFILE_720P_1_15F*/;
      //HandleGetSoftwareStream   分辨力修改 这里也要同步修改
		LOGD_SDK("StreamConfig width:%d height:%d fps:%d wb:%d", mSoftSourceStreamConfig.videoOpt.maxWidth, mSoftSourceStreamConfig.videoOpt.maxHeight, mSoftSourceStreamConfig.videoOpt.maxFrameRate, mSoftSourceStreamConfig.videoOpt.minVideoBW);
		LOGD_SDK("make_shared StreamConfig");
		mSoftWareStreamIsCapture = true;
        mSoftSourceStreamConfig.mLocal = true;
		mSoftwareStream = std::make_shared<vhall::VHStream>(mSoftSourceStreamConfig);
		if (mSoftwareStream) {
			LOGD_SDK("index:%d", index);

			mSoftwareStream->Init();
         vhall::LiveVideoFilterParam filter;
         filter.enableEdgeEnhance = false;
         mSoftwareStream->SetFilterParam(filter);
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
   //profileIndex = RTC_VIDEO_PROFILE_1080P_16x9_L;
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
      mDesktopStream->ResetCapability(desktopStreamConfig.videoOpt);
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
            LOGD_SDK("index:%d", index);
            vhall::LiveVideoFilterParam filter;
            filter.enableEdgeEnhance = false;
            mDesktopStream->SetFilterParam(filter);
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
	if (mDesktopStream){
		mDesktopStream->stop();
		RelaseStream(mDesktopStream);
      mDesktopVideoSsrc.reset();
      mDesktopVideoSsrc = nullptr;
	}
    mDesktopStreamIsCapture = false;
    LOGD_SDK("");
}

/*停止软件源共享采集*/
void WebRtcSDK::StopSoftwareCapture()
{
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

int WebRtcSDK::StartPushDesktopStream(std::string context) {
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
        mDesktopStream->mStreamAttributes = context;
        std::string StreamId = mDesktopStreamId;
		std::string join_uid = SubScribeStreamManager::WString2String(mWebRtcRoomOption.strUserId);
        mLiveRoom->Publish(mDesktopStream, [& , StreamId, join_uid , context](const vhall::PublishRespMsg& resp) -> void {
            if (resp.mResult == SDK_SUCCESS) {
                mDesktopStreamId = resp.mStreamId;
                LOGD_SDK("Publish mDesktopStreamId :%s", mDesktopStreamId.c_str());
                //推流成功移动到 “ON_STREAM_MIXED”事件处
                //if (mWebRtcSDKEventReciver) {
                //    mWebRtcSDKEventReciver->OnPushStreamSuc(vlive::VHStreamType_Desktop, mDesktopStreamId,true,false, mDesktopStream->mStreamAttributes);
                //}
				/*CTaskEvent roomConnectTask;
				roomConnectTask.mEventType = TaskEventEnum_ON_STREAM_MIXED;
				roomConnectTask.mStreamType = VHStreamType_Desktop;
				roomConnectTask.streamId = resp.mStreamId;
				roomConnectTask.mJoinUid = join_uid;
				int mediaType = VHStreamType_Desktop;

				std::string text = context;
				if (!text.empty()) {
					int type = ParamPushType(text);
					if (type == VHDoubleStreamType_Camera) {
						mediaType = VHStreamType_Auxiliary_CamerCapture;
					}
					else if (type == VHDoubleStreamType_Desktop) {
						mediaType = VHStreamType_Stu_Desktop;
					}
				}
				roomConnectTask.mStreamAttributes = mediaType;

            LOGD_SDK("***iStreamMixed: %d*** mStreamType :%d streamId:%s", ++iStreamMixed, roomConnectTask.mStreamType, roomConnectTask.streamId.c_str());
				PostTaskEvent(roomConnectTask);*/
            }
            else {
                LOGD_SDK("Publish mDesktopStream err:%s resp.mCode:%d", resp.mMsg.c_str(), resp.mCode);
                if (vhall::StreamPublished == resp.mCode || vhall::StreamIsEmpty == resp.mCode) {
                    return;
                }
                if (mWebRtcSDKEventReciver) {
                    mWebRtcSDKEventReciver->OnPushStreamError(StreamId, vlive::VHStreamType_Desktop, resp.mCode, resp.mMsg);
                }
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
	case vlive::VHStreamType_AUDIO:
	case vlive::VHStreamType_VIDEO:
    case vlive::VHStreamType_AVCapture: {
        CThreadLockHandle lockHandle(&mLocalStreamThreadLock);
        LOGD_SDK("mLocalStreamThreadLock");
        if (mLocalStream && mLocalStream->GetStreamState() == VhallStreamState::Publish) {
            LOGD_SDK("LocalStream is publishing stream");
            return true;
        }
        else {
            LOGD_SDK("LocalStream is not publish stream");
            return false;
        }
    }
        break;
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
    }
        break;
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
	}
									  break;
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
    }
        break;
    case vlive::VHStreamType_Auxiliary_CamerCapture: {
       LOGD_SDK("mLocalAuxiliaryStreamThreadLock");
       CThreadLockHandle lockHandle(&mLocalAuxiliaryStreamThreadLock);
       if (mLocalAuxiliaryStream && mLocalAuxiliaryStream->GetStreamState() == VhallStreamState::Publish) {
          LOGD_SDK("mLocalAuxiliaryStream is publishing stream");
          return true;
       }
       else {
          LOGD_SDK("mLocalAuxiliaryStream is not publish stream");
          return false;
       }
    }
                                        break;
    default:
        break;
    }
    return false;
}

bool WebRtcSDK::IsExitSubScribeStream(const vlive::VHStreamType& streamType)
{
   return mSubScribeStreamManager.IsExitStream(streamType);
}

bool WebRtcSDK::IsExitSubScribeStream(const std::string& id, const vlive::VHStreamType& streamType)
{
   return mSubScribeStreamManager.IsCurIdExitStream(id, streamType);
}

std::string WebRtcSDK::GetUserStreamID(const std::wstring user_id, VHStreamType streamType) {
    LOGD_SDK("%s", __FUNCTION__);
    return mSubScribeStreamManager.GetSubScribeStreamId(user_id, streamType);
}


bool WebRtcSDK::IsCapturingStream(vlive::VHStreamType streamType) {
    switch (streamType)
    {
	case vlive::VHStreamType_AUDIO:
	case vlive::VHStreamType_VIDEO:
    case vlive::VHStreamType_AVCapture: {
        CThreadLockHandle lockHandle(&mLocalStreamThreadLock);
        LOGD_SDK("mLocalStreamThreadLock");
        if (mLocalStream) {
            LOGD_SDK("Local Stream is capture stream");
            return mLocalStreamIsCapture;
        }
        else {
            LOGD_SDK("Local Stream is not capture stream");
            return false;
        }
        break;
    }                        
    case vlive::VHStreamType_Desktop: {
        LOGD_SDK("mDesktopStreamThreadLock");
        CThreadLockHandle lockHandle(&mDesktopStreamThreadLock);
        if (mDesktopStream) {
            LOGD_SDK("VHStreamType_Desktop is capture stream");
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
			LOGD_SDK("VHStreamType_Desktop is capture stream");
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
            LOGD_SDK("VHStreamType_MediaFile is capture stream");
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

void WebRtcSDK::GetStreams(std::list<std::string>& streams)
{
   mSubScribeStreamManager.GetStreams(streams);
}

std::string WebRtcSDK::LocalStreamId()
{
   std::string strRef = "";
   if (mLocalStream) {
      strRef = mLocalStream->mId;
   }
   //mLocalStreamId;
   return strRef;
}

int WebRtcSDK::PrepareMediaFileCapture(VideoProfileIndex profileIndex,long long seekPos) {
    LOGD_SDK("mMediaStreamThreadLock");
    CThreadLockHandle lockHandle(&mMediaStreamThreadLock);
    LOGD_SDK("enter");

    mediaStreamConfig.mAudio = true;
    mediaStreamConfig.mVideo = true;
    mediaStreamConfig.mStreamType = vlive::VHStreamType_MediaFile;
    if (mMediaStream) {
        LOGD_SDK("StopPushMediaFileStream");
        RelaseStream(mMediaStream);
        mMediaStreamIsCapture = false;
    }
    mediaStreamConfig.videoOpt.mProfileIndex = profileIndex;
    mMediaStreamIsCapture = true;
    int width = 0;
    int height = 0;
    vhall::FFmpegDemuxer ffmpegFileDemuxer;
    int nRet = ffmpegFileDemuxer.Init(mCurPlayFile.c_str());
    if (nRet == 0) {
        if (ffmpegFileDemuxer.GetVideoPar() != NULL) {
            width = ffmpegFileDemuxer.GetVideoPar()->width;
            height = ffmpegFileDemuxer.GetVideoPar()->height;
        }
        mediaStreamConfig.mAudio = ffmpegFileDemuxer.hasAudio;
        mediaStreamConfig.mVideo = ffmpegFileDemuxer.hasVideo;
    }

    LOGD_SDK("file w:%d h:%d audio:%d video:%d", width, height, mediaStreamConfig.mAudio, mediaStreamConfig.mVideo);
    mMediaStream.reset(new vhall::VHStream(mediaStreamConfig));
    if (mMediaStream) {
        mMediaStream->SetEventCallBack(OnPlayMediaFileCb, this);
        //开启插播流
        mMediaStream->Init();
        vhall::LiveVideoFilterParam filter;
        filter.enableEdgeEnhance = false;
        mMediaStream->SetFilterParam(filter);
        //当调用插播成功。
        mMediaStream->AddEventListener(ACCESS_ACCEPTED, [&, seekPos](vhall::BaseEvent &Event)->void {
            CTaskEvent task;
            task.mEventType = TaskEventEnum_CaptureMediaFile;
            task.mErrMsg = ACCESS_ACCEPTED;
            task.mSeekPos = seekPos;
            PostTaskEvent(task);
        });
        // 获取本地插播失败监听回调
        mMediaStream->AddEventListener(ACCESS_DENIED, [&](vhall::BaseEvent &Event)->void {
            CTaskEvent task;
            task.mEventType = TaskEventEnum_CaptureMediaFile;
            task.mErrMsg = ACCESS_DENIED;
            PostTaskEvent(task);
        });
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
        //WebRtcSDK* sdk = (WebRtcSDK*)(param);
        //sdk->PlayFileCallback(code);
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

int WebRtcSDK::StartPlayMediaFile(std::string filePath, VideoProfileIndex profileIndex, long long seekPos) {
    LOGD_SDK("enter filePath:%s", filePath.c_str());
    if (!mbIsEnablePlayFileAndShare) {
        LOGD_SDK("mbIsEnablePlayFileAndShare VhallLive_MUTEX_OPERATE");
        return vlive::VhallLive_MUTEX_OPERATE;
    }
    if (mDesktopStreamIsCapture) {
        LOGD_SDK("mbIsEnablePlayFileAndShare VhallLive_MUTEX_OPERATE");
        return vlive::VhallLive_MUTEX_OPERATE;
    }

    mCurPlayFile = filePath;
    return PrepareMediaFileCapture(profileIndex, seekPos);
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

/*
*   获取订阅流自定义消息
*/
std::string WebRtcSDK::GetSubStreamUserData(const std::wstring& user) {
    return mSubScribeStreamManager.GetUserData(user);
}

/*停止插播文件*/
void WebRtcSDK::StopMediaFileCapture() {
    LOGD_SDK("mMediaStreamThreadLock");
    
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
        std::string StreamId = mMediaFileStreamId;
		std::string join_uid = SubScribeStreamManager::WString2String(mWebRtcRoomOption.strUserId);
        mLiveRoom->Publish(mMediaStream, [&, StreamId, hasVideo, hasAudio , join_uid](const vhall::PublishRespMsg& resp) -> void {
            if (resp.mResult == SDK_SUCCESS) {
                LOGD_SDK("Publish mMediaStream success %s join_uid %d", resp.mStreamId.c_str(), join_uid.c_str() );
                mMediaFileStreamId = resp.mStreamId;
                //推流成功移动到ON_STREAM_MIXED事件处。
                //if (mWebRtcSDKEventReciver) {
                //    mWebRtcSDKEventReciver->OnPushStreamSuc(vlive::VHStreamType_MediaFile, mMediaFileStreamId, hasVideo, hasAudio, mMediaStream->mStreamAttributes);
                //}
				//CTaskEvent roomConnectTask;
				//roomConnectTask.mEventType = TaskEventEnum_ON_STREAM_MIXED;
				//roomConnectTask.mHasAudio = hasAudio;
				//roomConnectTask.mHasVideo = hasVideo;
				//roomConnectTask.mStreamType = VHStreamType_MediaFile;
				//roomConnectTask.streamId = resp.mStreamId;
				//roomConnectTask.mJoinUid = join_uid;
    //        LOGD_SDK("***iStreamMixed: %d*** mStreamType :%d streamId:%s", ++iStreamMixed, roomConnectTask.mStreamType, roomConnectTask.streamId.c_str());

				//PostTaskEvent(roomConnectTask);
            }
            else {
                LOGD_SDK("Publish mMediaStream error:%d msg:%s", resp.mCode, resp.mMsg.c_str());
                if (vhall::StreamPublished == resp.mCode || vhall::StreamIsEmpty == resp.mCode) {
                    return;
                }
                if (mWebRtcSDKEventReciver) {
                    mWebRtcSDKEventReciver->OnPushStreamError(StreamId, vlive::VHStreamType_MediaFile, resp.mCode, resp.mMsg);
                }
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
    //return vlive::VhallLive_OK;
}

//插播文件暂停处理
void WebRtcSDK::MediaFilePause() {
    LOGD_SDK(" MediaFilePause");
    CThreadLockHandle lockHandle(&mMediaStreamThreadLock);
    if (mMediaStream) {
        LOGD_SDK("FilePause");
        mMediaStream->FilePause();
    }
    //return vlive::VhallLive_OK;
}

//插播文件恢复处理
void WebRtcSDK::MediaFileResume() {
    LOGD_SDK(" MediaFileResume");
    CThreadLockHandle lockHandle(&mMediaStreamThreadLock);
    if (mMediaStream) {
        LOGD_SDK("FileResume");
        mMediaStream->FileResume();
    }
    //return vlive::VhallLive_OK;
}

//插播文件快进处理
void WebRtcSDK::MediaFileSeek(const unsigned long long& seekTimeInMs) {
    LOGD_SDK(" MediaFileSeek seekTimeInMs:%lld", seekTimeInMs);
    CThreadLockHandle lockHandle(&mMediaStreamThreadLock);
    if (mMediaStream) {
        LOGD_SDK("FileSeek");
        mMediaStream->FileSeek(seekTimeInMs);
    }
    //return vlive::VhallLive_OK;
}

int WebRtcSDK::MediaFileVolumeChange(int vol) {
    if (vol < 0 || vol > 100) {
        return VhallLive_PARAM_ERR;
    }
    if (mMediaStream) {
        mMediaStream->SetFileVolume(vol);
        return VhallLive_OK;
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
    vhall::VHTools vh;

    const std::vector<std::shared_ptr<vhall::VideoDevProperty>> CameraList = vh.VideoInDeviceList();
    for (int i = 0; i < CameraList.size(); i++) {
        std::shared_ptr<vhall::VideoDevProperty> videoDev = CameraList.at(i);
        vhall::VideoDevProperty info; 
        info.mDevId = videoDev->mDevId;
        info.mDevName = videoDev->mDevName;
        info.mIndex = videoDev->mIndex;
        LOGD_SDK("GetCameraDevices name:%s", videoDev->mDevName.c_str());
        for (int i = 0; i < videoDev->mDevFormatList.size(); i++) {
            std::shared_ptr<vhall::VideoFormat> cap =  videoDev->mDevFormatList.at(i);
            std::shared_ptr<vhall::VideoFormat> format = std::make_shared<vhall::VideoFormat>();
            format->height = cap->height;
            format->width = cap->width;
            format->maxFPS = cap->maxFPS;
            format->videoType = (int)cap->videoType;
            format->interlaced = cap->interlaced;
            info.mDevFormatList.push_back(format);
            LOGD_SDK("GetCameraDevices format width:%d heigth:%d", format->width, format->height);
        }
        cameras.push_back(info);
    }
    LOGD_SDK(" GetCameraDevices :%d", cameras.size());
    return vlive::VhallLive_OK;
}

/**获取麦克风列表**/
int WebRtcSDK::GetMicDevices(std::list<vhall::AudioDevProperty> &micDevList) {
    LOGD_SDK(" GetMicDevices");
    vhall::VHTools vh;
    std::vector<std::shared_ptr<vhall::AudioDevProperty>> micList = vh.AudioRecordingDevicesList();
    for (int i = 0; i < micList.size(); i++) {
        std::shared_ptr<vhall::AudioDevProperty> mic = micList.at(i);
        vhall::AudioDevProperty info;
        info.mDevGuid = mic->mDevGuid;
        info.mDevName = mic->mDevName;
        info.mIndex = i;
        info.mFormat = mic->mFormat;
        info.m_sDeviceType = mic->m_sDeviceType;
        if (info.m_sDeviceType != vhall::TYPE_DSHOW_AUDIO) {
            micDevList.push_back(info);
        }
        LOGD_SDK("GetMicDevices name:%s index:%d  devid:%s", mic->mDevName.c_str(), mic->mIndex, mic->mDevGuid.c_str());
    }
    LOGD_SDK("GetMicDevices micDevList:%d", micDevList.size());
    return vlive::VhallLive_OK;
}

/*
*    获取窗口共享和桌面共享列表
**  vlive::VHStreamType      // 3:Screen,5:window
*/
std::vector<DesktopCaptureSource> WebRtcSDK::GetDesktops(int streamType) {
    vhall::VHTools vh;
    std::vector<DesktopCaptureSource> Desktops = vh.GetDesktops(streamType);
    return Desktops;
}


int WebRtcSDK::GetCameraDevDetails(std::list<CameraDetailsInfo> &cameraDetails) {
    vhall::VHTools vh;
    const std::vector<std::shared_ptr<vhall::VideoDevProperty>> CameraList = vh.VideoInDeviceList();
    for (int i = 0; i < CameraList.size(); i++) {
        std::shared_ptr<vhall::VideoDevProperty> videoDev = CameraList.at(i);
        CameraDetailsInfo cameraInfo;
        cameraInfo.mDevId = videoDev->mDevId;
        cameraInfo.mDevName = videoDev->mDevName;
        cameraInfo.mIndex = videoDev->mIndex;
        std::unordered_map<VideoProfileIndex, bool> profileMap = vh.GetSupportIndex(videoDev->mDevFormatList);
        std::unordered_map<VideoProfileIndex, bool>::iterator iter = profileMap.begin();
        VideoProfileList mVideoProfiles;
        while (iter != profileMap.end()) {
            if (iter->second) {
                std::shared_ptr<VideoProfile> profile = mVideoProfiles.GetProfile(iter->first);
                VideoProfile profileTemp;
                profileTemp.mIndex = profile->mIndex;
                profileTemp.mMaxWidth = profile->mMaxWidth;
                profileTemp.mMinWidth = profile->mMinWidth;
                profileTemp.mMaxHeight = profile->mMaxHeight;
                profileTemp.mMinHeight = profile->mMinHeight;

                profileTemp.mRatioW = profile->mRatioW;
                profileTemp.mRatioH = profile->mRatioH;


                profileTemp.mMaxFrameRate = profile->mMaxFrameRate;
                profileTemp.mMinFrameRate = profile->mMinFrameRate;
                profileTemp.mMaxBitRate = profile->mMaxBitRate;
                profileTemp.mMinBitRate = profile->mMinBitRate;
                profileTemp.mStartBitRate = profile->mStartBitRate;

                cameraInfo.profileList.push_back(profileTemp);
                LOGD_SDK("MaxWidth :%d  MinWidth:%d MaxHeight:%d MinHeight:%d", 
                   profile->mMaxWidth, profile->mMinWidth, profile->mMaxHeight, profile->mMinHeight);

            }
            iter++;
        }
        cameraDetails.push_back(cameraInfo);
    }
    LOGD_SDK(" GetCameraDevDetails :%d", cameraDetails.size());
    return vlive::VhallLive_OK;
}


/**获取扬声器列表**/
int WebRtcSDK::GetPlayerDevices(std::list<vhall::AudioDevProperty> &playerDevList) {
    vhall::VHTools vh;
    std::vector<std::shared_ptr<vhall::AudioDevProperty>> playList = vh.AudioPlayoutDevicesList();
    for (int i = 0; i < playList.size(); i++) {
        std::shared_ptr<vhall::AudioDevProperty> play = playList.at(i);
        vhall::AudioDevProperty info;
        info.mDevGuid = play->mDevGuid;
        info.mDevName = play->mDevName;
        info.mIndex = play->mIndex;
        info.mFormat = play->mFormat;
        playerDevList.push_back(info);
        LOGD_SDK("GetPlayerDevices name:%s index:%d devID:%s", info.mDevName.c_str(), play->mIndex, play->mDevGuid.c_str());
    }
    LOGD_SDK(" micDevList:%d", playerDevList.size());
    return vlive::VhallLive_OK;
}

/*
*  是否存在音频或视频设备。
*  返回值：只要存在一类设备 返回true, 如果音视频设备都没有则返回false
**/
bool WebRtcSDK::HasVideoOrAudioDev() {
    vhall::VHTools vh;
    const std::vector<std::shared_ptr<vhall::VideoDevProperty>> CameraList = vh.VideoInDeviceList();
    if (CameraList.size() > 0) {
        return true;
    }
    std::vector<std::shared_ptr<vhall::AudioDevProperty>> micList = vh.AudioRecordingDevicesList();
    if (micList.size() > 0) {
        return true;
    }
    return false;
}

bool WebRtcSDK::HasVideoDev()
{
   vhall::VHTools vh;
   const std::vector<std::shared_ptr<vhall::VideoDevProperty>> CameraList = vh.VideoInDeviceList();
   if (CameraList.size() > 0) {
      return true;
   }
   return false;
}

bool WebRtcSDK::HasAudioDev()
{
   vhall::VHTools vh;
   std::vector<std::shared_ptr<vhall::AudioDevProperty>> micList = vh.AudioRecordingDevicesList();
   if (micList.size() > 0) {
      return true;
   }
   return false;
}

int WebRtcSDK::SetUsedMic(int micIndex, std::string micDevId, std::wstring desktopCaptureId) {
    LOGD_SDK("SetUsedMic:%d devId:%s", micIndex, micDevId.c_str());
    //vhall::VHStream::SetAudioInDevice(micIndex, desktopCaptureId);
    //vhall::VHStream::SetMicrophoneVolume(mbIsOpenMic == true ? (int)mCurMicVol : 0);

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

//关闭摄像头
bool WebRtcSDK::CloseCamera() {
    LOGD_SDK("mLocalStreamThreadLock");
    CThreadLockHandle lockHandle(&mLocalStreamThreadLock);
    LOGD_SDK(" CloseCamera");
    if (mLocalStream && mLocalStreamIsCapture && (mLocalStream->mStreamType == vlive::VHStreamType_AVCapture || mLocalStream->mStreamType == vlive::VHStreamType_VIDEO)) {
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
    if (mLocalStream && mLocalStreamIsCapture && (mLocalStream->mStreamType == vlive::VHStreamType_AVCapture || mLocalStream->mStreamType == vlive::VHStreamType_VIDEO)) {
        LOGD_SDK("Camera is open");
        mLocalStream->MuteVideo(false);
        mbIsOpenCamera = true;
        if (mbReceive && mLocalStreamReceive) {
           LOGD_SDK("Camera is open play(mLocalStreamReceive)");
           mLocalStream->play(mLocalStreamReceive);
        }
        else {
           LOGD_SDK("Camera is open play(mLocalStreamWnd)");
           mLocalStream->play(mLocalStreamWnd);
        }
        
        return true;
    }
    return false;
}

bool WebRtcSDK::IsCameraOpen() {
    if (mLocalStream && mLocalStreamIsCapture && (mLocalStream->mStreamType == vlive::VHStreamType_AVCapture || mLocalStream->mStreamType == vlive::VHStreamType_VIDEO)) {
        LOGD_SDK("camera is open");
        return mbIsOpenCamera;
    }
    LOGD_SDK("camera is not open");
	return false;
}

bool WebRtcSDK::CloseMic() {
    //if (mLocalStream && mLocalStreamIsCapture && (mLocalStream->mStreamType == vlive::VHStreamType_AVCapture)) {
    //    LOGD_SDK("mic is closed");
    //   // mLocalStream->MuteAudio(true);
    //    vhall::VHStream::SetMicrophoneVolume(0);
    //    mbIsOpenMic = false;
    //    return true;
    //}
    //return false;
    LOGD_SDK("SetMicrophoneVolume 0");
    vhall::VHStream::SetMicrophoneVolume(0);
    mbIsOpenMic = false;
    return true;
}
//打开麦克风
bool WebRtcSDK::OpenMic() {
    //if (mLocalStream && mLocalStreamIsCapture && (mLocalStream->mStreamType == vlive::VHStreamType_AVCapture)) {
    //    LOGD_SDK("open mic");
    //    //mLocalStream->MuteAudio(false);
    //    vhall::VHStream::SetMicrophoneVolume(mCurMicVol);
    //    mbIsOpenMic = true;
    //    return true;
    //}
    //return false;
    LOGD_SDK("open mic :%d", (int)mCurMicVol);
    vhall::VHStream::SetMicrophoneVolume(mCurMicVol);
    mbIsOpenMic = true;
    return true;
}

bool WebRtcSDK::IsMicOpen() {
    if (mLocalStream && mLocalStreamIsCapture && (mLocalStream->mStreamType == vlive::VHStreamType_AVCapture || mLocalStream->mStreamType == vlive::VHStreamType_AUDIO)) {
        LOGD_SDK("mic is open");
        return mbIsOpenMic;
    }
    return false;
}

//打开、关闭远端用户本地视频
int WebRtcSDK::OperateRemoteUserVideo(std::wstring user_id, bool close/* = false*/) {
    return mSubScribeStreamManager.OperateRemoteUserVideo(user_id, close);
}

//打开、关闭远端用户本地声音
int WebRtcSDK::OperateRemoteUserAudio(std::wstring user_id, bool close /*= false*/) {
    return mSubScribeStreamManager.OperateRemoteUserAudio(user_id, close);
}

//bool WebRtcSDK::StartRenderStream(const std::wstring& user, void* wnd, vlive::VHStreamType streamType) {
//    LOGD_SDK("streamType:%d", streamType);
//    if (user == mWebRtcRoomOption.strUserId) {
//        LOGD_SDK("StartRenderStream");
//        if (streamType == vlive::VHStreamType_AVCapture || streamType == vlive::VHStreamType_PureAudio || streamType == vlive::VHStreamType_PureVideo) {
//            CThreadLockHandle lockHandle(&mLocalStreamThreadLock);
//            LOGD_SDK("mLocalStreamThreadLock");
//            if (mLocalStream) {
//                LOGD_SDK("play mLocalStream");
//                mLocalStream->stop();
//                HWND playWnd = (HWND)(wnd);
//                mLocalStream->play(playWnd);
//                LOGD_SDK("play mLocalStream end");
//                return true;
//            }
//        }
//        else if (streamType == vlive::VHStreamType_Desktop) {
//            LOGD_SDK("mDesktopStreamThreadLock");
//            CThreadLockHandle lockHandle(&mDesktopStreamThreadLock);
//            if (mDesktopStream) {
//                LOGD_SDK("play local mDesktopStream");
//                mDesktopStream->stop();
//                HWND playWnd = (HWND)(wnd);
//                mDesktopStream->play(playWnd);
//                LOGD_SDK("play local mDesktopStream end");
//                return true;
//            }
//        }
//        else if (streamType == vlive::VHStreamType_MediaFile) {
//            LOGD_SDK("mMediaStreamThreadLock");
//            CThreadLockHandle lockHandle(&mMediaStreamThreadLock);
//            if (mMediaStream) {
//                LOGD_SDK("play local mMediaStream");
//                mMediaStream->stop();
//                HWND playWnd = (HWND)(wnd);
//                mMediaStream->play(playWnd);
//                LOGD_SDK("play local mMediaStream end");
//                return true;
//            }
//        }
//    }
//    else {
//       LOGD_SDK("mSubScribeStreamMutex");
//       std::wstring user_id = user;
//       if (streamType == VHStreamType_Desktop) {
//          user_id = user_id + DESKTOP_SHARE_INDEX;
//          LOGD_SDK("find  streamType VHStreamType_Desktop");
//       }
//       else if (streamType == VHStreamType_MediaFile) {
//          LOGD_SDK("find  streamType VHStreamType_MediaFile");
//          user_id = user_id + MEDIA_FILE_INDEX;
//       }
//       mSubScribeStreamManager.StartRenderStream(user_id, wnd);
//       //mSubScribeStreamManager.ChangeUserPlayDev(mCurrentPlayerIndex, mCurPlayVol);
//    }
//    LOGD_SDK("not find");
//    return false;
//}
/**
*   开始渲染"本地"摄像设备、桌面共享、文件插播媒体数据流.
*/
bool WebRtcSDK::StartRenderLocalStream(vlive::VHStreamType streamType, void* wnd) {
   LOGD_SDK("StartRenderStream");
   if (streamType <= vlive::VHStreamType_AVCapture) {
      CThreadLockHandle lockHandle(&mLocalStreamThreadLock);
      LOGD_SDK("mLocalStreamThreadLock");
      if (mLocalStream && mLocalStream->HasVideo()) {
         LOGD_SDK("play mLocalStream");
         mLocalStream->stop();
         HWND playWnd = (HWND)(wnd);
         mLocalStreamWnd = playWnd;
         mLocalStream->play(playWnd);
         mbReceive = false;
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

bool WebRtcSDK::StartRenderLocalStreamByInterface(vlive::VHStreamType streamType, std::shared_ptr<vhall::VideoRenderReceiveInterface> receive)
{
   LOGD_SDK("StartRenderStream");
   if (streamType <= vlive::VHStreamType_AVCapture) {
      CThreadLockHandle lockHandle(&mLocalStreamThreadLock);
      LOGD_SDK("mLocalStreamThreadLock");
      if (mLocalStream && mLocalStream->HasVideo()) {
         LOGD_SDK("play mLocalStream");
         mLocalStream->stop();
         mbReceive = true;
         mLocalStreamReceive = receive;
         mLocalStream->play(receive);
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
         //HWND playWnd = (HWND)(wnd);
         mDesktopStream->play(receive);
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
         //HWND playWnd = (HWND)(wnd);
         mMediaStream->play(receive);
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
         //HWND playWnd = (HWND)(wnd);
         mSoftwareStream->play(receive);
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
         //HWND playWnd = (HWND)(wnd);
         mLocalAuxiliaryStream->play(receive);
         LOGD_SDK("play local mLocalAuxiliaryStream end");
         return true;
      }
   }
   return false;
}
/**
*   开始渲染"远端"摄像设备、桌面共享、文件插播媒体数据流.
*/
bool WebRtcSDK::StartRenderRemoteStream(const std::wstring& user, vlive::VHStreamType streamType, void* wnd) {
    LOGD_SDK("%s",__FUNCTION__);
    std::string mediaType = std::to_string(streamType);
    std::wstring subStreamIndex = user + SubScribeStreamManager::String2WString(mediaType);
    return mSubScribeStreamManager.StartRenderStream(subStreamIndex, wnd);
}

bool WebRtcSDK::StartRenderRemoteStreamByInterface(const std::wstring& user, vlive::VHStreamType streamType, std::shared_ptr<vhall::VideoRenderReceiveInterface> receive)
{
   LOGD_SDK("%s", __FUNCTION__);
   std::string mediaType = std::to_string(streamType);
   std::wstring subStreamIndex = user + SubScribeStreamManager::String2WString(mediaType);
   return mSubScribeStreamManager.StartRenderRemoteStreamByInterface(subStreamIndex, receive);
}

bool WebRtcSDK::IsRemoteStreamIsExist(const std::wstring & user, vlive::VHStreamType streamType)
{
   std::string mediaType = std::to_string(streamType);
   std::wstring subStreamIndex = user + SubScribeStreamManager::String2WString(mediaType);
   return mSubScribeStreamManager.IsRemoteStreamIsExist(subStreamIndex);
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
    }
    break;
    case vlive::VHStreamType_Desktop: {
        if (mWebRtcSDKEventReciver) {
            mWebRtcSDKEventReciver->OnRePublishStreamIDChanged(vlive::VHStreamType_Desktop, user_id, mDesktopStreamId, new_streamId);
            LOGD_SDK("type %d new_streamId:%s mDesktopStreamId:%s", type, new_streamId.c_str(), mDesktopStreamId.c_str());
            mDesktopStreamId = new_streamId;
        }
    }
    break;
    case vlive::VHStreamType_MediaFile: {
        if (mWebRtcSDKEventReciver) {
            mWebRtcSDKEventReciver->OnRePublishStreamIDChanged(vlive::VHStreamType_MediaFile, user_id, mMediaFileStreamId, new_streamId);
            LOGD_SDK("type %d new_streamId:%s mMediaFileStreamId:%s", type, new_streamId.c_str(), mMediaFileStreamId.c_str());
            mMediaFileStreamId = new_streamId;
        }
    }
    break;
    default:
        break;
    }
}

DWORD WINAPI WebRtcSDK::ThreadProcess(LPVOID p) {
    LOGD_SDK("ThreadProcess");
    while (bThreadRun) {
        if (gTaskEvent) {
            DWORD ret = WaitForSingleObject(gTaskEvent, 2000);
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
        LOGD_SDK("TaskEventEnum_RE_SUBSCRIB");
        if (event.mSleepTime > 0) {
            Sleep(event.mSleepTime);
        }
        CThreadLockHandle lockRoom(&mRoomThreadLock);
        if (mLiveRoom) {
            std::string stream_id = event.streamId;
            mLiveRoom->Subscribe(stream_id, nullptr, [&, stream_id](const vhall::RespMsg& resp) -> void {
                //在此输出订阅成功或失败信息。
                LOGD_SDK("ROOM_CONNECTED mLiveRoom->Subscribe stream_id %s resp.mResult:%s msg.mMsg:%s resp.mCode:%d",
                                                                                                         stream_id.c_str(), 
                                                                                                         resp.mResult.c_str(), resp.mMsg.c_str(),   resp.mCode);
                if (resp.mResult != SDK_SUCCESS) {
                    if (resp.mCode == vhall::StreamIsEmpty) {
                        LOGD_SDK("Subscribe vhall::StreamIsEmpty");
                        return;
                    }
                    if (resp.mCode == vhall::StreamSubscribed) {
                        LOGD_SDK("Subscribe vhall::StreamSubscribed");
                        return;
                    }

                    CTaskEvent task;
                    task.mEventType = TaskEventEnum_RE_SUBSCRIB;
                    task.streamId = stream_id;
                    task.mSleepTime = 3000;
                    LOGD_SDK("Subscribe stream_id uid:%s", stream_id.c_str());
                    PostTaskEvent(task);
                }
            }, SDK_TIMEOUT, 3000);
        }
        LOGD_SDK("TaskEventEnum_RE_SUBSCRIB");
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
    case TaskEventEnum_ROOM_RECONNECTING: {
        HandleRoomReConnecting(event);
        break;
    }
    case TaskEventEnum_ROOM_RECONNECTED: {
        HandleRoomReConnected(event);
        break;
    }
    //case TaskEventEnum_ROOM_RECONNECTED: {
    //   LOGD_SDK("TaskEventEnum_ROOM_RECONNECTED");
    //   CThreadLockHandle lockRoom(&mRoomThreadLock);
    //   if (mLiveRoom) {
    //      Sleep(2000);
    //      LOGD_SDK("Disconnect");
    //      mLiveRoom->Disconnect();
    //      LOGD_SDK("Connect");
    //      mLiveRoom->Connect();
    //   }
    //   LOGD_SDK("TaskEventEnum_ROOM_RECONNECTED end");
    //   break;
    //}
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
    
    case TaskEventEnum_SetSubscribe_PlayOutVol: {
       LOGD_SDK("TaskEventEnum_SetSubscribe_PlayOutVol");
       mSubScribeStreamManager.SetPlayIndexAndVolume(event.mUserId, mCurrentPlayerIndex, mCurPlayVol);
       LOGD_SDK("TaskEventEnum_SetSubscribe_PlayOutVol end");
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
    case TaskEventEnum_CaptureAuxiliaryLocalStream: {
        HandleCaptureAuxiliaryLocalStrea(event);
        break;
    }
    case TaskEventEnum_StartPreviewLocalCapture: {
        LOGD_SDK("TaskEventEnum_StartPreviewLocalCapture");
        HandlePreviewCamera(event.wnd, event.devId, event.index, -1);
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
    if (0 == player->SetPlay(fileName, devIndex, 48000, 2)) {
        player->Start();
        player->SetAudioListen(&mPlayFileAudioVolume);
    }
    return 0;
}

int WebRtcSDK::GetPlayAudioFileVolum() {
    return mPlayFileAudioVolume.GetAudioVolume();
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
        LOGD_SDK("%s RemoveAllEventListener", __FUNCTION__);
        stream->stop();
        LOGD_SDK("%s stop", __FUNCTION__);
        stream->close();

        LOGD_SDK("%s close", __FUNCTION__);
        stream->Destroy();
        stream.reset();
        LOGD_SDK("%s reset", __FUNCTION__);
        stream = nullptr;
    }
}

void WebRtcSDK::SetDebugLogAddr(const std::string& upAddr) {
   vhall::VHStream::OpenUpStats(upAddr.c_str());
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
        mWebRtcSDKEventReciver->OnWebRtcRoomConnetEventCallback(vlive::VHRoomConnect_ROOM_CONNECTED);
        LOGD_SDK("callback VHRoomConnect_ROOM_CONNECTED");
    }

	//配置混流
	SetEnableConfigMixStream(true);
	if (mWebRtcSDKEventReciver) {
		std::string msg = "Server-side mixing ready";

		mWebRtcSDKEventReciver->OnWebRtcRoomConnetEventCallback(vlive::VHRoomConnect_ROOM_MIXED_STREAM_READY, task.mJoinUid);
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
        LOGD_SDK("TaskEventEnum_RE_SUBSCRIB Subscribe bAutoSubScribeStream:%d stream_id:%s event.mSleepTime:%d", (int)bAutoSubScribeStream, stream_id.c_str(), event.mSleepTime);
        if (bAutoSubScribeStream) {
            mLiveRoom->Subscribe(stream_id, nullptr, [&, stream_id](const vhall::RespMsg& resp) -> void {
                //在此输出订阅成功或失败信息。
                LOGD_SDK("mLiveRoom->Subscribe resp:%s msg:%s resp.mCode:%d", resp.mResult.c_str(), resp.mMsg.c_str(), resp.mCode);
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
    }
    LOGD_SDK("TaskEventEnum_RE_SUBSCRIB end");
}

void WebRtcSDK::HandleRoomError(CTaskEvent& event) {
    mbIsWebRtcRoomConnected = false;
    LOGD_SDK("mbIsWebRtcRoomConnected = false");
    if (mWebRtcSDKEventReciver) {
        mWebRtcSDKEventReciver->OnWebRtcRoomConnetEventCallback(vlive::VHRoomConnect_ROOM_ERROR);
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
            LOGD_SDK("HandleRoomError Connect");
            mLiveRoom->Connect();
        }
    }
    LOGD_SDK("HandleRoomError end");
}

void WebRtcSDK::HandleOnStreamMixed(CTaskEvent& event) {
    LOGD_SDK("HandleOnStreamMixed  mJoinUid %s mWebRtcRoomOption.strUserId %s", event.mJoinUid.c_str() , SubScribeStreamManager::WString2String(mWebRtcRoomOption.strUserId).c_str());

    if (event.mJoinUid == SubScribeStreamManager::WString2String(mWebRtcRoomOption.strUserId)) {
        LOGD_SDK("Publish stream success stream id:%s streamType:%d video:%d audio:%d  iHandleStreamMixed: %d mStreamType :%d streamId:%s", 
           event.streamId.c_str(), event.mStreamType, event.mHasVideo, event.mHasAudio,
           ++iHandleStreamMixed, event.mStreamType, event.streamId.c_str());
		switch (event.mStreamType)
		{
		case VHStreamType_AUDIO:					//纯音频
		case VHStreamType_VIDEO:						//纯视频，没有音频
		case VHStreamType_AVCapture:
			if (mWebRtcSDKEventReciver){
			VHStreamType  iStreamType = CalcStreamType(event.mHasAudio, event.mHasVideo);
			mWebRtcSDKEventReciver->OnPushStreamSuc(iStreamType, event.streamId, event.mHasVideo, event.mHasAudio, event.mStreamAttributes);
			}
			break;
		case vlive::VHStreamType_MediaFile: {
			if (mWebRtcSDKEventReciver) {
				mWebRtcSDKEventReciver->OnPushStreamSuc(vlive::VHStreamType_MediaFile, event.streamId, event.mHasVideo, event.mHasAudio, event.mStreamAttributes);
			}
			break;
		}
		case vlive::VHStreamType_Desktop: {
			if (mWebRtcSDKEventReciver) {
				mWebRtcSDKEventReciver->OnPushStreamSuc(vlive::VHStreamType_Desktop, event.streamId, true, false, event.mStreamAttributes);
			}
			break;
		}
		case vlive::VHStreamType_SoftWare: {
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
      case VHStreamType_Stu_Desktop:{
         mWebRtcSDKEventReciver->OnPushStreamSuc(vlive::VHStreamType_Stu_Desktop, event.streamId, true, false, event.mStreamAttributes);
         break;
      }
		default:
			break;
		}
        
    }
    else
    {
       
    }


    LOGD_SDK("HandleOnStreamMixed end");
}

void WebRtcSDK::HandleRoomReConnecting(CTaskEvent& event) {
    mbIsWebRtcRoomConnected = false;
    LOGD_SDK("mbIsWebRtcRoomConnected = false");
    if (mWebRtcSDKEventReciver) {
        mWebRtcSDKEventReciver->OnWebRtcRoomConnetEventCallback(vlive::VHRoomConnect_ROOM_RECONNECTING);
    }
    LOGD_SDK("HandleRoomReConnecting end");
}

void WebRtcSDK::HandleRoomReConnected(CTaskEvent& event) {
    mbIsWebRtcRoomConnected = true;
    LOGD_SDK("mbIsWebRtcRoomConnected = true");
    if (mWebRtcSDKEventReciver) {
        mWebRtcSDKEventReciver->OnWebRtcRoomConnetEventCallback(vlive::VHRoomConnect_RE_CONNECTED);
    }
    LOGD_SDK("HandleRoomReConnected end");
}

void WebRtcSDK::HandleRoomRecover(CTaskEvent& event) {
    mbIsWebRtcRoomConnected = true;
    LOGD_SDK("mbIsWebRtcRoomConnected = true");
    if (mWebRtcSDKEventReciver) {
        mWebRtcSDKEventReciver->OnWebRtcRoomConnetEventCallback(vlive::VHRoomConnect_ROOM_RECOVER);
    }
    LOGD_SDK("HandleRoomRecover end");
}  

void WebRtcSDK::HandleRoomNetworkChanged(CTaskEvent& event) {
    mbIsWebRtcRoomConnected = true;
    LOGD_SDK("mbIsWebRtcRoomConnected = true");
    if (mWebRtcSDKEventReciver) {
        mWebRtcSDKEventReciver->OnWebRtcRoomConnetEventCallback(vlive::VHRoomConnect_ROOM_NETWORKCHANGED);
    }
    LOGD_SDK("HandleRoomNetworkChanged end");
}


void WebRtcSDK::HandleSetPlayerDev(CTaskEvent& event) {
    LOGD_SDK("enter");
    vhall::VHStream::SetLoopBackVolume(event.mVolume);
    int nRet = vhall::VHStream::SetAudioInDevice(event.devIndex, event.desktopCaptureId) ? VhallLive_OK : VhallLive_NO_DEVICE;
    LOGD_SDK("end nRet %d", nRet);
}

VHStreamType  WebRtcSDK::CalcStreamType(const bool& bAudio, const bool& bVedio)
{
	VHStreamType  iRef = VHStreamType_Count;
	if (bAudio && bVedio)
	{
		iRef = VHStreamType_AVCapture;
	}
	else if(bAudio && !bVedio)
	{
		iRef = VHStreamType_AUDIO;
	}
	else if(!bAudio && bVedio)
	{
		iRef = VHStreamType_VIDEO;
	}
	return iRef;
}

void WebRtcSDK::HandleSetMicDev(CTaskEvent& event) {
    LOGD_SDK("HandleSetMicDev");
    vhall::VHStream::SetAudioInDevice(event.devIndex, event.desktopCaptureId);
    vhall::VHStream::SetMicrophoneVolume(mbIsOpenMic == true ? (int)mCurMicVol : 0);
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
                mWebRtcSDKEventReciver->OnPushStreamError(event.streamId, VHStreamType_Desktop, 40001, "STREAM_FAILED");
            }
        }
        else if (event.mStreamType == VHStreamType_SoftWare) {
            LOGD_SDK("STREAM_FAILED VHStreamType_SoftWare :%d", (int)mSoftWareStreamIsCapture);
            if (mSoftWareStreamIsCapture) {
                mWebRtcSDKEventReciver->OnPushStreamError(event.streamId, VHStreamType_SoftWare, 40001, "STREAM_FAILED");
            }
        }
        else if (event.mStreamType == VHStreamType_MediaFile) {
            LOGD_SDK("STREAM_FAILED VHStreamType_MediaFile :%d", (int)mMediaStreamIsCapture);
            if (mMediaStreamIsCapture) {
                mWebRtcSDKEventReciver->OnPushStreamError(event.streamId, vlive::VHStreamType_MediaFile, 40001, "STREAM_FAILED");
            }
        }
        else {
            LOGD_SDK("STREAM_FAILED VHStreamType_AVCapture mLocalStreamIsCapture:%d", (int)mLocalStreamIsCapture);
            if (mLocalStreamIsCapture) {
				VHStreamType  iStreamType = CalcStreamType(mCameraStreamConfig.mAudio, mCameraStreamConfig.mVideo);
                mWebRtcSDKEventReciver->OnPushStreamError(event.streamId, iStreamType, 40001, "STREAM_FAILED");
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

void WebRtcSDK::HandleStreamSubSuccess(CTaskEvent& event) {
    LOGD_SDK("HandleStreamSubSuccess");
    std::wstring user_id = SubScribeStreamManager::String2WString(event.mJoinUid);
    //如果类型是插播视频类型的，重新设置joinuid.
    if (event.mStreamType == vlive::VHStreamType_MediaFile) {//插播
        CTaskEvent task;
        task.mEventType = TaskEventEnum_RECV_MEDIA_STREAM;
        LOGD_SDK("recv media stream");
        PostTaskEvent(task);
    }
    else if (event.mStreamType == vlive::VHStreamType_Desktop)//桌面共享
    {
        //user_id = SubScribeStreamManager::String2WString(joinUid) + DESKTOP_SHARE_INDEX;
        CTaskEvent task;
        task.mEventType = TaskEventEnum_RECV_MEDIA_STREAM;
        LOGD_SDK("recv desktop stream");
        PostTaskEvent(task);
    }
    else if (event.mStreamType <= vlive::VHStreamType_AVCapture) {
        //if (mEnableSimulCast) {
        //    ///* 设置大小流 0小流 1大流 */
        //    //mLiveRoom->SetSimulCast(AddStream->mStreams, 0, [&](const vhall::RespMsg& resp) -> void {
        //    //    LOGD_SDK("SetSimulCast %d msg %s", resp.mCode, resp.mMsg.c_str());
        //    //}, 30000);
        //}
    }
    LOGD_SDK("mSubScribeStreamManager mMuteAllAudio:%d", (int)mMuteAllAudio);
    mSubScribeStreamManager.MuteAllSubScribeAudio(mMuteAllAudio);
    if (mWebRtcSDKEventReciver) {
        LOGD_SDK("OnReciveRemoteUserLiveStream :%s event.streamId:%s event.mStreamType:%d video:%d audio:%d", user_id.c_str(), event.streamId.c_str(), event.mStreamType, event.mHasVideo, event.mHasAudio);
        mWebRtcSDKEventReciver->OnReciveRemoteUserLiveStream(user_id, event.streamId, (vlive::VHStreamType)event.mStreamType, event.mHasVideo, event.mHasAudio);
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
            mPreviewLocalStream->play(previewWnd);
            LOGD_SDK("play");
            //return;
            if (mWebRtcSDKEventReciver) {
                mWebRtcSDKEventReciver->OnOpenCaptureCallback(vlive::VHStreamType_Preview_Video, vlive::VHCapture_OK, previewStreamConfig.mVideo, previewStreamConfig.mAudio);
            }
        }
        else if (event.mErrMsg == VIDEO_DENIED || event.mErrMsg == ACCESS_DENIED) {
            if (mWebRtcSDKEventReciver) {
                mWebRtcSDKEventReciver->OnOpenCaptureCallback(vlive::VHStreamType_Preview_Video, vlive::VHCapture_VIDEO_DENIED, previewStreamConfig.mVideo, previewStreamConfig.mAudio);
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
        std::string StreamId = mLocalAuxiliaryStreamId;
        mLocalAuxiliaryStream->mStreamAttributes = context;
		std::string join_uid = SubScribeStreamManager::WString2String(mWebRtcRoomOption.strUserId);
        mLiveRoom->Publish(mLocalAuxiliaryStream, [&, StreamId, hasVideo, hasAudio , join_uid , context](const vhall::PublishRespMsg& resp) -> void {
            if (resp.mResult == SDK_SUCCESS) {
                mLocalAuxiliaryStreamId = resp.mStreamId;
                LOGD_SDK("Publish mLocalAuxiliaryStream mLocalAuxiliaryStreamId:%s", mLocalAuxiliaryStreamId.c_str());
				//if (mWebRtcSDKEventReciver) {
				//	mWebRtcSDKEventReciver->OnPushStreamSuc(vlive::VHStreamType_Auxiliary_CamerCapture, mLocalAuxiliaryStreamId, true, false, mLocalAuxiliaryStream->mStreamAttributes);
				//}
	       	   /*CTaskEvent roomConnectTask;
               roomConnectTask.mEventType = TaskEventEnum_ON_STREAM_MIXED;
               roomConnectTask.mHasAudio = hasAudio;
               roomConnectTask.mHasVideo = hasVideo;
			   int mediaType = VHStreamType_Auxiliary_CamerCapture;
			   std::string text = context;
			   if (!text.empty()) {
				   int type = ParamPushType(text);
                if (type == VHDoubleStreamType_Camera) {
                    mediaType = VHStreamType_Auxiliary_CamerCapture;
                }
                else if(type == VHDoubleStreamType_Desktop) {
                      mediaType = VHStreamType_Stu_Desktop;
                }
            }
               roomConnectTask.mStreamType = mediaType;
               roomConnectTask.streamId = resp.mStreamId;
               roomConnectTask.mJoinUid = join_uid;
               roomConnectTask.mStreamAttributes = mLocalAuxiliaryStream->mStreamAttributes;
               LOGD_SDK("***iStreamMixed: %d*** mStreamType :%d streamId:%s", ++iStreamMixed, roomConnectTask.mStreamType, roomConnectTask.streamId.c_str());
               PostTaskEvent(roomConnectTask);*/
            }
            else {
                LOGD_SDK("Publish mLocalAuxiliaryStream err:%s resp.mCode:%d", resp.mMsg.c_str(), resp.mCode);
                if (vhall::StreamPublished == resp.mCode || vhall::StreamIsEmpty == resp.mCode) {
                    return;
                }
                if (mWebRtcSDKEventReciver) {
                    mWebRtcSDKEventReciver->OnPushStreamError(StreamId, vlive::VHStreamType_Auxiliary_CamerCapture, resp.mCode, resp.mMsg);
                }
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
   LOGD_SDK("");
   if (!mbIsWebRtcRoomConnected) {
      LOGD_SDK("VhallLive_ROOM_DISCONNECT");
      return vlive::VhallLive_ROOM_DISCONNECT;
   }
   CThreadLockHandle lockHandle(&mLocalAuxiliaryStreamThreadLock);
   LOGD_SDK("mLocalAuxiliaryStream");
   if (mLiveRoom && mLocalAuxiliaryStream) {
      LOGD_SDK("mLiveRoom->UnPublish");
      if (mLocalAuxiliaryStream->GetStreamState() == VhallStreamState::Publish) {
         LOGD_SDK("mLocalAuxiliaryStream->GetStreamState() == VhallStreamState::Publish");
         mLiveRoom->UnPublish(mLocalAuxiliaryStream, [&](const vhall::RespMsg& resp) -> void {
            mLocalAuxiliaryStreamId.clear();
            LOGD_SDK("UnPublish LocalStream msg:%s", resp.mMsg.c_str());
            if (mWebRtcSDKEventReciver) {
               VHStreamType  iStreamType = CalcStreamType(mAuxiliaryStreamConfig.mAudio, mAuxiliaryStreamConfig.mVideo);
               if (resp.mResult == SDK_SUCCESS) {
                  mWebRtcSDKEventReciver->OnStopPushStreamCallback(iStreamType, vlive::VhallLive_OK, resp.mMsg);
               }
               else {
                  mWebRtcSDKEventReciver->OnStopPushStreamCallback(iStreamType, vlive::VhallLive_ERROR, resp.mMsg);
               }
            }
         }, SDK_TIMEOUT);
         mLocalAuxiliaryStream->SetStreamState(UnPublish);
      }
      else {
         LOGD_SDK("mLocalAuxiliaryStreamId->GetStreamState() != VhallStreamState::Publish");
      }
   }
   else {
      LOGD_SDK("VhallLive_NO_OBJ");
      return vlive::VhallLive_NO_OBJ;
   }
   LOGD_SDK("end");
   return vlive::VhallLive_OK;
}

void WebRtcSDK::HandleCaptureLocalStream(CTaskEvent &event) {
    LOGD_SDK("HandleCaptureLocalStream");
    CThreadLockHandle lockHandle(&mLocalStreamThreadLock);
    LOGD_SDK("get mLocalStreamThreadLock");
    if (mLocalStream) {
		VHStreamType  iStreamType = CalcStreamType(mCameraStreamConfig.mAudio, mCameraStreamConfig.mVideo);
        if (event.mErrMsg == ACCESS_ACCEPTED) {
            mLocalStreamIsCapture = true;
            if (mWebRtcSDKEventReciver) {
                mLocalStreamId = mLocalStream->mId;
                mWebRtcSDKEventReciver->OnOpenCaptureCallback(iStreamType, vlive::VHCapture_OK, mCameraStreamConfig.mVideo, mCameraStreamConfig.mAudio);
            }
        }
        else if (event.mErrMsg == ACCESS_DENIED) {
            mLocalStreamIsCapture = false;
            mbIsOpenCamera = false;
            mbIsOpenMic = false;
            if (mWebRtcSDKEventReciver) {
                LOGD_SDK("vlive::VHCapture_ACCESS_DENIED");
                mWebRtcSDKEventReciver->OnOpenCaptureCallback(iStreamType, vlive::VHCapture_ACCESS_DENIED, mCameraStreamConfig.mVideo, mCameraStreamConfig.mAudio);
            }
        }
        else if (event.mErrMsg == VIDEO_DENIED) {
            mLocalStreamIsCapture = false;
            mbIsOpenCamera = false;
            mbIsOpenMic = false;
            //视频设备打开失败，请您检测设备是否连接正常
            if (mWebRtcSDKEventReciver) {
                LOGD_SDK("vlive::VHCapture_VIDEO_DENIED");
                mWebRtcSDKEventReciver->OnOpenCaptureCallback(iStreamType, vlive::VHCapture_VIDEO_DENIED, mCameraStreamConfig.mVideo, mCameraStreamConfig.mAudio);
            }
        }
        else if (event.mErrMsg == AUDIO_DENIED) {
            mLocalStreamIsCapture = false;
            mbIsOpenCamera = false;
            mbIsOpenMic = false;
            //视频设备打开失败，请您检测设备是否连接正常
            if (mWebRtcSDKEventReciver) {
                LOGD_SDK("vlive::VHCapture_AUDIO_DENIED");
                mWebRtcSDKEventReciver->OnOpenCaptureCallback(iStreamType, vlive::VHCapture_AUDIO_DENIED, mCameraStreamConfig.mVideo, mCameraStreamConfig.mAudio);
            }
        }
        else if (event.mErrMsg == CAMERA_LOST) {
            if (mWebRtcSDKEventReciver) {
                LOGD_SDK("vlive::VHCapture_STREAM_SOURCE_LOST");
                mWebRtcSDKEventReciver->OnOpenCaptureCallback(iStreamType, vlive::VHCapture_STREAM_SOURCE_LOST, mCameraStreamConfig.mVideo, mCameraStreamConfig.mAudio);
            }
        }
        else if (event.mErrMsg == AUDIO_CAPTURE_ERROR) {
            if (mWebRtcSDKEventReciver) {
                LOGD_SDK("vlive::VHCapture_AUDIO_DENIED");
				
                mWebRtcSDKEventReciver->OnOpenCaptureCallback(iStreamType, vlive::VHCapture_AUDIO_DENIED, mCameraStreamConfig.mVideo, mCameraStreamConfig.mAudio);
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
            bool bRet = mMediaStream->FilePlay(mCurPlayFile.c_str());
            LOGD_SDK("ACCESS_ACCEPTED bRet:%d", bRet);
            if (bRet) {
                mMediaStream->FileSeek(event.mSeekPos);
                mMediaStreamIsCapture = true;
                if (mWebRtcSDKEventReciver) {
                    mWebRtcSDKEventReciver->OnOpenCaptureCallback(vlive::VHStreamType_MediaFile, vlive::VHCapture_OK, mediaStreamConfig.mVideo, mediaStreamConfig.mAudio);
                }
            }
            else {
                mMediaStreamIsCapture = false;
                if (mWebRtcSDKEventReciver) {
                    mWebRtcSDKEventReciver->OnOpenCaptureCallback(vlive::VHStreamType_MediaFile, vlive::VHCapture_PLAY_FILE_ERR, mediaStreamConfig.mVideo, mediaStreamConfig.mAudio);
                }
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
        mWebRtcSDKEventReciver->OnRemoteStreamAdd(event.mJoinUid,event.streamId, (vlive::VHStreamType)event.mStreamType);
    }
    LOGD_SDK("HandleAddStream end");
}

void WebRtcSDK::HandleRecvStreamLost(CTaskEvent& event) {
    LOGD_SDK("HandleRecvStreamLost %s", event.streamId.c_str());
    if(event.streamId.length() > 0){
        CThreadLockHandle roomLockHandle(&mRoomThreadLock);
        LOGD_SDK("mLiveRoom");
        if (mLiveRoom) {
            LOGD_SDK("UnSubscribe");
            mLiveRoom->UnSubscribe(event.streamId);
        }
    }
    LOGD_SDK("HandleRoomStreamSubScribe");
    if (event.streamId.length() > 0) {
        HandleRoomStreamSubScribe(event);
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



std::string WebRtcSDK::GetAuxiliaryId()
{
   return mSubScribeStreamManager.GetAuxiliaryId();
}

std::string WebRtcSDK::GetLocalAuxiliaryId()
{
	std::string strRef = "";
	if (mLocalAuxiliaryStream) {
		strRef = mLocalAuxiliaryStream->GetUserId();
	}
	return strRef;
}

int WebRtcSDK::ParamPushType(std::string& context) {
   int type = -1;
   rapidjson::Document doc;
   doc.Parse<0>(context.c_str());
   if (doc.HasParseError()) {
      //return type;
   }
   else {
      if (doc.IsObject()) {
         if (doc.HasMember("double_push") && doc["double_push"].IsInt()) {
            type = doc["double_push"].GetInt();
         }
         else if (doc.HasMember("double_push") && doc["double_push"].IsString()) {
            std::string  strtype = doc["msg"].GetString();
            type = atoi(strtype.c_str());
         }
      }
   }

   return type;//VHDoubleStreamType   1摄像头  2桌面共享

}
