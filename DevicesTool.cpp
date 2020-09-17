#include "DevicesTool.h"
#include "./vhall_sdk_log.h"
#include "vlive_def.h"
#include "UnilityTool.h"

using namespace vlive;
using namespace webrtc_sdk;

vhall::VHTools DevicesTool::mVhTool;

int DevicesTool::EnumCameraDevices(std::list<vhall::VideoDevProperty> &cameras) {
   const std::vector<std::shared_ptr<vhall::VideoDevProperty>> CameraList = mVhTool.VideoInDeviceList();
   for (int i = 0; i < CameraList.size(); i++) {
      std::shared_ptr<vhall::VideoDevProperty> videoDev = CameraList.at(i);
      if (videoDev->state == vhall::VideoDevProperty::DeviceState::GetFormatInfoError) {
         continue;
      }
      vhall::VideoDevProperty info;
      info.mDevId = videoDev->mDevId;
      info.mDevName = videoDev->mDevName;
      info.mIndex = videoDev->mIndex;
      LOGD_SDK("GetCameraDevices name:%s", videoDev->mDevName.c_str());
      for (int i = 0; i < videoDev->mDevFormatList.size(); i++) {
         std::shared_ptr<vhall::VideoFormat> cap = videoDev->mDevFormatList.at(i);
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

int DevicesTool::EnumMicDevices(std::list<vhall::AudioDevProperty> &micDevList) {
   LOGD_SDK("%s", __FUNCTION__);
   std::vector<std::shared_ptr<vhall::AudioDevProperty>> micList = mVhTool.AudioRecordingDevicesList();
   LOGD_SDK("%s AudioRecordingDevicesList", __FUNCTION__);
   for (int i = 0; i < micList.size(); i++) {
      LOGD_SDK("%s AudioRecordingDevicesList %d ", __FUNCTION__, i);
      std::shared_ptr<vhall::AudioDevProperty> mic = micList.at(i);
      vhall::AudioDevProperty info;
      info.mDevGuid = mic->mDevGuid;
      info.mDevName = mic->mDevName;
      info.mIndex = i;
      info.mFormat = mic->mFormat;
      info.m_sDeviceType = mic->m_sDeviceType;
      micDevList.push_back(info);
      LOGD_SDK("%s AudioRecordingDevicesList %d name %s", __FUNCTION__, i, UnilityTool::WString2String(info.mDevName).c_str());
   }
   LOGD_SDK("GetMicDevices micDevList:%d", micDevList.size());
   return vlive::VhallLive_OK;
}


int DevicesTool::EnumPlayerDevices(std::list<vhall::AudioDevProperty> &playerDevList) {
   std::vector<std::shared_ptr<vhall::AudioDevProperty>> playList = mVhTool.AudioPlayoutDevicesList();
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

std::vector<DesktopCaptureSource> DevicesTool::EnumDesktops(int streamType) {
   vhall::VHTools vh;
   std::vector<DesktopCaptureSource> Desktops = vh.GetDesktops(streamType);
   std::vector<DesktopCaptureSource>::iterator iter = Desktops.begin();
   while (iter != Desktops.end()) {
      LOGD_SDK("%s index %d", __FUNCTION__, iter->id);
      iter++;
   }
   return Desktops;
}

bool DevicesTool::HasVideoDev() {
   const std::vector<std::shared_ptr<vhall::VideoDevProperty>> CameraList = mVhTool.VideoInDeviceList();
   if (CameraList.size() > 0) {
      return true;
   }
   return false;
}

bool DevicesTool::HasAudioDev() {
   std::vector<std::shared_ptr<vhall::AudioDevProperty>> micList = mVhTool.AudioRecordingDevicesList();
   if (micList.size() > 0) {
      return true;
   }
   return false;
}