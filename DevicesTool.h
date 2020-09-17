#pragma once
#include "signalling/vh_tools.h"

#include <list>


class DevicesTool
{
public:
   static int EnumCameraDevices(std::list<vhall::VideoDevProperty> &cameras);
   static int EnumMicDevices(std::list<vhall::AudioDevProperty> &micDevList);
   static int EnumPlayerDevices(std::list<vhall::AudioDevProperty> &playerDevList);
   static std::vector<DesktopCaptureSource> EnumDesktops(int streamType);
   static bool HasVideoDev();
   static bool HasAudioDev();

private:
   static vhall::VHTools mVhTool;
};

