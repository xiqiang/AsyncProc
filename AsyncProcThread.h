#ifndef AsyncProcThread_H_Xiqiang_20200831
#define AsyncProcThread_H_Xiqiang_20200831

#if defined(_WIN32) || defined(_WIN64)
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#elif defined(__LINUX__)
#include <pthread.h>
#endif

#include <queue>
#include "AsyncProc.h"

class AsyncProcThread
{
public:	
	typedef std::queue<AsyncProc*>		ProcQueue;
	typedef std::queue<AsyncProcResult> ResultQueue;

	enum State 
	{
		State_None,
		State_Running,
		State_Sleeping,
		State_Stopping,
	};

	struct ProcResult
	{

	};

public:
	AsyncProcThread(int curstomID);
	~AsyncProcThread();

public:
	bool Startup(void);
	void Enqueue(AsyncProc* proc);
	void Shutdown(void);
	void Terminate(void);
	void CallbackTick(void);
	size_t GetProcCount(void);

private:
#if defined(_WIN32) || defined(_WIN64)
	static DWORD WINAPI ThreadFunc (PVOID arg);
#elif defined(__LINUX__)
	static void* ThreadFunc(void* arg);
#endif

	void _ThreadStart(void);
	void _ThreadCycle(void);
	void _OnExeEnd(AsyncProcResultType type, const char* what);
	void _ClearProcs(void);
	void _GetLock();
	void _ReleaseLock();

private:
	ProcQueue m_waitQueue;
	ResultQueue m_doneQueue;

	int m_customID;
	size_t m_count;
	State m_state;
	AsyncProc* m_exeProc;

#if defined(_WIN32) || defined(_WIN64)
	CRITICAL_SECTION m_queueLock;
	CONDITION_VARIABLE m_awakeCondition;
	HANDLE m_hThread;
	DWORD m_tid;
#elif defined(__LINUX__)
	pthread_mutex_t m_queueLock;
	pthread_cond_t m_awakeCondition;
	pthread_t m_tid;
#endif
};

#endif
