#ifndef AsyncProcManager_H_Xiqiang_20190907
#define AsyncProcManager_H_Xiqiang_20190907

#if defined(_WIN32) || defined(_WIN64)
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#elif defined(__LINUX__)
#include <pthread.h>
#endif

#include "AsyncProcDef.h"
#include "AsyncProcThread.h"

class AsyncProcManager
{
public:
	friend class AsyncProcThread;

public:
	AsyncProcManager();
	~AsyncProcManager();

public:
	void Startup(int threadCount = 1);
	void Shutdown(void);
	void Terminate(void);

	void Schedule(AsyncProc* proc);
	void TickCallback(void);

private:
	void _ClearProcs(void);

	ProcQueue& _GetWaitQueue(void);
	ResultQueue& _GetDoneQueue(void);

	void _GetQueueLock(void);
	void _ReleaseQueueLock(void);

	void _GetThreadLock(void);
	void _ReleaseThreadLock(void);

	void _ThreadWaitNewProc(void);

private:
	ThreadVector m_threads;
	ProcQueue m_waitQueue;
	ResultQueue m_doneQueue;

#if defined(_WIN32) || defined(_WIN64)
	CRITICAL_SECTION m_lockQueue;
	CRITICAL_SECTION m_lockThread;
	CONDITION_VARIABLE m_condNewProc;
#elif defined(__LINUX__)
	pthread_mutex_t m_lockQueue;	// 队列锁
	pthread_mutex_t m_lockThread;	// 线程锁
	pthread_cond_t m_condNewProc;	// 条件-有proc存在
#endif

};

#endif
