// VhallPaasSDK.cpp: 定义 DLL 应用程序的导出函数。
//

#include "stdafx.h"
#include "WebRTC/WebRtcSDK.h"
#include <mutex>
using namespace vlive;

static WebRtcSDK *gWebRtcSDKImpl = nullptr;
static std::mutex gWebRtcSDKMutex;

namespace vlive {
    VHALL_PAAS_SDK_EXPORT WebRtcSDKInterface* GetWebRtcSDKInstance() {
        std::unique_lock<std::mutex> lock(gWebRtcSDKMutex);
        if (gWebRtcSDKImpl == nullptr) {
            gWebRtcSDKImpl = new WebRtcSDK();
        }
        return dynamic_cast<WebRtcSDKInterface*>(gWebRtcSDKImpl);
    }

    VHALL_PAAS_SDK_EXPORT  void DestroyWebRtcSDKInstance() {
        std::unique_lock<std::mutex> lock(gWebRtcSDKMutex);
        if (gWebRtcSDKImpl != nullptr) {
            delete gWebRtcSDKImpl;
            gWebRtcSDKImpl = nullptr;
        }
    }

}
