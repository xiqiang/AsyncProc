#ifndef AsyncProcThread_H_Xiqiang_20200831
#define AsyncProcThread_H_Xiqiang_20200831

#include <time.h>
#include "AsyncProcDef.h"
#include "AsyncProcResult.h"

class AsyncProc;
class AsyncProcManager;

class AsyncProcThread
{
public:
	friend class AsyncProcManager;

public:
	enum State 
	{
		State_None,
		State_Running,
		State_Quiting,
		State_FastQuiting,
	};

private:
	AsyncProcThread(AsyncProcManager* manager);

public:
	~AsyncProcThread();

	AP_Thread GetId(void) const {
		return m_tid;
	}

private:
	bool Startup(void);
	void ShutdownNotify(AsyncProcShutdownMode mode);
	void ShutdownWait(void);

private:
#if defined(_WIN32) || defined(_WIN64)
	static DWORD WINAPI ThreadFunc (PVOID arg);
#elif defined(__LINUX__)
	static void* ThreadFunc(void* arg);
#endif

	void _Execute(void);
	void _ProcDone(AsyncProcResult::Type type, const char* what);

private:
	AsyncProcManager*	m_manager;
	AP_Thread			m_tid;
	State				m_state;
	AsyncProc*			m_proc;	

#if defined(_WIN32) || defined(_WIN64)
	HANDLE m_hThread;
#endif

};

#endif
