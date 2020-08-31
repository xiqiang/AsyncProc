#ifndef AsyncProcThread_H_Xiqiang_20200831
#define AsyncProcThread_H_Xiqiang_20200831

#include <queue>
#include "AsyncProc.h"

#if defined(__WINDOWS__)
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#elif defined(__LINUX__)
#include <pthread.h>
#endif

class AsyncProcThread
{
public:	
	typedef std::queue<AsyncProc*> ProcQueue;
	enum State 
	{
		State_None,
		State_Running,
		State_Sleeping,
		State_Stopping,
	};

public:
	AsyncProcThread();
	~AsyncProcThread();

public:
	bool Startup(void);
	void Enqueue(AsyncProc* proc);
	void Shutdown(void);
	void Terminate(void);
	void CallbackTick(void);
	size_t GetProcCount(void);

private:
#if defined(__WINDOWS__)
	static DWORD WINAPI ThreadFunc (PVOID arg);
#elif defined(__LINUX__)
	static void* ThreadFunc(void* arg);
#endif

	void _ThreadStart(void);
	void _ThreadCycle(void);	

	void _GetLock();
	void _ReleaseLock();

private:
	ProcQueue m_waitQueue;
	ProcQueue m_executeQueue;
	ProcQueue m_doneQueue;
	ProcQueue m_callbackQueue;

	size_t m_count;
	State m_state;

#if defined(__WINDOWS__)
	CRITICAL_SECTION m_queueLock;
	CONDITION_VARIABLE m_awakeCondition;
	DWORD m_tid;
	HANDLE m_hThread;
#elif defined(__LINUX__)
	pthread_mutex_t m_queueLock;
	pthread_cond_t m_awakeCondition;
	pthread_t m_tid;
#endif
};

#endif
