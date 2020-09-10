#ifndef __VHALL_WEBRTC_LOG_H
#define __VHALL_WEBRTC_LOG_H

#include <stdio.h>
#include <string>
#include <time.h>
#include <vector>

#include "live_sys.h"

namespace webrtc_sdk {
    typedef enum
    {
        VHALL_LOG_LEVEL_DEBUG = 1, /**< The Debug log level */
        VHALL_LOG_LEVEL_INFO, /**< The Info log level */
        VHALL_LOG_LEVEL_WARN, /**< The Warn log level */
        VHALL_LOG_LEVEL_ERROR, /**< The Error log level */

        VHALL_LOG_LEVEL_NO_LOG = 10 /*"set this to stop logout"*/
    }VhallLogLevel;

    typedef enum {
        VHALL_LOG_TYPE_CONSOLE,
        VHALL_LOG_TYPE_FILE,
        VHALL_LOG_TYPE_SOCKET
    }VhallLogType;

    typedef struct {
        int nType; //type=0 stdout type=1 stderr
    }ConsoleInitParam;

    typedef struct {
        //if nPartionSize>0 or nPartionTime>0, (can't both > 0) this is file prefix, 
        //realy log is name_%d(nPartionSize>0) or name_time(nPartionTime>0) 
        const char* pFilePathName;
        int  nPartionSize;    //KB
        int  nPartionTime;    //sec
    }FileInitParam;

    typedef struct {
        const char* pServer; //type=0 stdout type=1 stderr
        int  nPort;
    }SocketInitParam;

    int GenerateLogId();

    class LogWriter {
    protected:
        int mLevel;
        int mId;
    public:
        virtual int Init(void * InitParam) = 0;
        virtual int WriteLog(int level, const char *header, const char* fmt, va_list ap) = 0;
        virtual void Destroy() = 0;
        virtual int GetId() { return mId; };
        virtual int SetLevel(int level) { mLevel = level; return 0; };
        LogWriter(int level);
    };

    class ConsoleLogWriter :public LogWriter {
    private:
        int mType;
        FILE *mfp;
    public:
        //dest = 0 stdout dest = 1 stderr
        virtual int Init(void * InitParam);
        virtual int WriteLog(int level, const char *header, const char* fmt, va_list ap);
        virtual void Destroy();
        ConsoleLogWriter(int level);
    };

    class FileLogWriter :public LogWriter {
    private:
        char mFilePathName[256];
        char mPartionFileName[256];
        int  mPartionSize;//KB
        int  mPartionTime;//sec
        FILE *mfp;
        int  mCPCount; //curent partion Byte count
        int  mCPPostfix;
        time_t  mCPStartTime; //curent partion start time;
    private:
        virtual int CheckPartion();
    public:
        virtual int Init(void * InitParam);
        virtual int WriteLog(int level, const char *header, const char* fmt, va_list ap);
        virtual void Destroy();
        FileLogWriter(int level);
    };

    class SocketLogWriter : public LogWriter {
    private:
        char mServerName[256];
        int mPort;
    public:
        SocketLogWriter(int level) :LogWriter(level) {};
    };

    class VhallSDKLog {
    private:
        std::vector<LogWriter*> mWriterList;
        vhall_lock_t mLock;
    public:
        VhallSDKLog();
        virtual ~VhallSDKLog();
        //if success return log_id which is >=0 . if error return -1
        virtual int  AddLog(VhallLogType type, void *InitParam, int level);
        virtual int  RemoveLog(int LogId);
        virtual int  SetLogLevel(int LogId, int level);
        virtual int  RmoveAllLog();
        virtual void Info(std::string header, const char* fmt, ...);
        virtual void Debug(std::string header, const char* fmt, ...);
        virtual void Warn(std::string header, const char* fmt, ...);
        virtual void Error(std::string header, const char* fmt, ...);
    private:
        //std::string GetHeader(const char * level_header);
    };

    extern VhallSDKLog *vhall_log_sdk;

#define DEBUG_HEADER "[DEBUG]"
#define INFO_HEADER  "[INFO]"
#define WARN_HEADER  "[WARN]"
#define ERROR_HEADER "[ERROR]"

#if defined (WIN32) || defined(_WIN32)
#define __func__ __FUNCTION__
#endif

    std::string SDKGetFormatDateTime();
    std::string SDKGetFormatDate2();
    std::string SDKint2str(int num);
    void SDKInitLog(std::wstring filePath);



#define LOGD_SDK(msg, ...)   { vhall_log_sdk->Debug(SDKGetFormatDateTime() + ":" + DEBUG_HEADER + ":" + __func__ + ":" + SDKint2str(__LINE__), msg, ##__VA_ARGS__);}
#define LOGI_SDK(msg, ...)   { vhall_log_sdk->Info(SDKGetFormatDateTime() + ":" + INFO_HEADER + ":" + __func__ + ":" + SDKint2str(__LINE__), msg, ##__VA_ARGS__);}
#define LOGW_SDK(msg, ...)   { vhall_log_sdk->Warn(SDKGetFormatDateTime() + ":" + WARN_HEADER+":" + __func__ + ":" + SDKint2str(__LINE__), msg, ##__VA_ARGS__);}
#define LOGE_SDK(msg, ...)   { vhall_log_sdk->Error(SDKGetFormatDateTime() + ":" + ERROR_HEADER + ":" + __func__ + ":" + SDKint2str(__LINE__), msg, ##__VA_ARGS__);}


#define ADD_NEW_LOG_SDK(type, param, level)  vhall_log_sdk->AddLog(type, param, level)
#define REMOVE_LOG_SDK(id)  vhall_log_sdk->RemoveLog(id)
#define SET_LOG_LEVEL_SDK(id, level)  vhall_log_sdk->SetLogLevel(id, level)
#define REMOVE_ALL_LOG_SDK  vhall_log_sdk->RmoveAllLog();
}
#endif