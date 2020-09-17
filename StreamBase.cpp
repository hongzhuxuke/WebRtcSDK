#include "../StreamBaseInterface.h"
#include "../vhall_sdk_log.h"

using namespace webrtc_sdk;

StreamBase::StreamBase() {
}

StreamBase::~StreamBase() {

}


bool StreamBase::HasVideo() {
    return mWebRtcStream == nullptr ? false : mWebRtcStream->HasVideo();
}

bool StreamBase::HasAudio() {
    return mWebRtcStream == nullptr ? false : mWebRtcStream->HasAudio();
}

void StreamBase::ReleaseStream() {
    if (mWebRtcStream) {
        mWebRtcStream->RemoveAllEventListener();
        LOGD_SDK("RemoveAllEventListener");
        mWebRtcStream->stop();
        LOGD_SDK("stop");
        mWebRtcStream->close();
        LOGD_SDK("close");
        mWebRtcStream->Destroy();
        mWebRtcStream.reset();
        LOGD_SDK("reset");
        mWebRtcStream = nullptr;
        mbIsCapture = false;
    }
}

void StreamBase::StartRender(void* wnd) {
    if (mWebRtcStream) {
        LOGD_SDK("play mLocalStream");
        mWebRtcStream->stop();
        HWND playWnd = (HWND)(wnd);
        mStreamWnd = playWnd;
        mWebRtcStream->play(playWnd);
        LOGD_SDK("play mLocalStream end");
    }
}