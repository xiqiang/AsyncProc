#ifndef AsyncProcManager_H_Xiqiang_20190907
#define AsyncProcManager_H_Xiqiang_20190907

#include <vector>
#include "AsyncProcThread.h"

class AsyncProcManager
{
public:
	AsyncProcManager();
	~AsyncProcManager();

public:
	void Startup(int threadCount = 1);
	int Enqueue(AsyncProc* proc);
	int Enqueue(AsyncProc* proc, size_t threadIndex);
	void Shutdown(void);
	void Terminate(void);
	void CallbackTick(void);
	size_t GetProcCount(void);

private:
	void _GetLock();
	void _ReleaseLock();

private:
	std::vector<AsyncProcThread*> m_threads;

#if defined(_WIN32) || defined(_WIN64)
	CRITICAL_SECTION m_queueLock;
#elif defined(__LINUX__)
	pthread_mutex_t m_queueLock;
#endif
};

#endif
