#ifndef AsyncProcManager_H_Xiqiang_20190907
#define AsyncProcManager_H_Xiqiang_20190907

#include <vector>
#include "AsyncThread.h"

#if defined(__WINDOWS__)
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#elif defined(__LINUX__)
#include <pthread.h>
#endif

#if defined(__WINDOWS__)
static DWORD WINAPI ThreadProc (PVOID arg);
#elif defined(__LINUX__)
static void* ThreadProc(void* arg);
#endif	

class AsyncProcManager
{
public:
	AsyncProcManager();
	~AsyncProcManager();

public:
	void Startup(int threadCount = 1);
	int Enqueue(AsyncProc* proc, int threadIndex = -1);
	void Shutdown(void);
	void Terminate(void);
	void CallbackTick(void);

private:
	std::vector<AsyncProcThread*> m_threads;
};

#endif
