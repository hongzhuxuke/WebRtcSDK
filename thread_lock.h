//thread_lock.h
#ifndef THREAD_LOCK_HEAD_FILE
#define THREAD_LOCK_HEAD_FILE
#include<windows.h>
////////////////////////////////////////////////////////////////////////////

//临界区同步类
class CThreadLock
{
    //变量定义
private:
    CRITICAL_SECTION                m_csLock;            //临界区变量

                                                         //函数定义
public:
    //构造函数
    inline CThreadLock() { ::InitializeCriticalSection(&m_csLock); };
    //析构函数
    inline ~CThreadLock() { ::DeleteCriticalSection(&m_csLock); };

    //功能函数
public:
    //锁定函数
    inline void Lock() { ::EnterCriticalSection(&m_csLock); };
    //解锁函数
    inline void UnLock() { ::LeaveCriticalSection(&m_csLock); };
};

////////////////////////////////////////////////////////////////////////////

//安全同步锁定句柄
class CThreadLockHandle
{
    //变量定义
private:
    int                                m_nLockCount;        //锁定计数
    CThreadLock                        *m_pThreadLock;        //锁定对象

                                                              //函数定义
public:
    //构造函数
    CThreadLockHandle(CThreadLock *pThreadLock, bool bAutoLock = true);
    //析构函数
    virtual ~CThreadLockHandle();

    //功能函数
public:
    //锁定函数
    void Lock();
    //解锁函数
    void UnLock();
    //火枪锁定次数
    int inline GetLockCount() { return m_nLockCount; };
};

#endif