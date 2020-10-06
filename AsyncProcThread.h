#ifndef AsyncProcThread_H_Xiqiang_20200831
#define AsyncProcThread_H_Xiqiang_20200831

#if defined(_WIN32) || defined(_WIN64)
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#elif defined(__LINUX__)
#include <pthread.h>
#endif

#include "AsyncProcDef.h"

class AsyncProc;
class AsyncProcManager;

class AsyncProcThread
{
public:
	enum State 
	{
		State_None,
		State_Running,
		State_Quiting,
	};

public:
	AsyncProcThread(AsyncProcManager* manager, int curstomID);
	~AsyncProcThread();

public:
	bool Startup(void);
	void NotifyQuit(void);
	void QuitWait(void);
	void Terminate(void);

private:
#if defined(_WIN32) || defined(_WIN64)
	static DWORD WINAPI ThreadFunc (PVOID arg);
#elif defined(__LINUX__)
	static void* ThreadFunc(void* arg);
#endif

	void _ThreadStart(void);
	void _OnExeEnd(AsyncProcResultType type, const char* what);

private:
#if defined(_WIN32) || defined(_WIN64)
	HANDLE m_hThread;
	DWORD m_tid;
#elif defined(__LINUX__)
	pthread_t m_tid;
#endif

	AsyncProcManager* m_manager;
	int m_customID;
	State m_state;
	AsyncProc* m_exeProc;	
};

#endif
