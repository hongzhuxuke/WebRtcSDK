//thread_lock.cpp   programed by sany
//2014.9.2
//callme:sanyue9394@163.com


#include "thread_lock.h"
#include<assert.h>
////////////////////////////////////////////////////////////////////////////

//安全同步锁定句柄
//构造函数
CThreadLockHandle::CThreadLockHandle(CThreadLock *pThreadLock, bool bAutoLock)
{
    assert(pThreadLock != NULL);
    m_nLockCount = 0;
    m_pThreadLock = pThreadLock;
    if (bAutoLock)    Lock();
    return;
}
//析构函数
CThreadLockHandle::~CThreadLockHandle()
{
    while (m_nLockCount>0) UnLock();                                //生存周期结束自动解锁
}

//锁定函数
void CThreadLockHandle::Lock()
{
    //校验状态
    assert(m_nLockCount >= 0);
    assert(m_pThreadLock != NULL);

    //锁定对象
    m_nLockCount++;
    m_pThreadLock->Lock();
}

//解锁函数
void CThreadLockHandle::UnLock()
{
    //校验状态
    assert(m_nLockCount>0);
    assert(m_pThreadLock != NULL);

    //解除状态
    m_nLockCount--;
    m_pThreadLock->UnLock();
}