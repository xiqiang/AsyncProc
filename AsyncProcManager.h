#ifndef AsyncProcManager_H_Xiqiang_20190907
#define AsyncProcManager_H_Xiqiang_20190907

#include "AsyncProcDef.h"
#include "AsyncProcThread.h"
#include "Condition.h"

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
	void Tick(void);

private:
	ProcQueue& GetWaitQueue(void) {
		return m_waitQueue;
	}

	ResultQueue& GetDoneQueue(void) {
		return m_doneQueue;
	}

	Mutex& GetQueueMutex(void) {
		return m_queueMutex;
	}

	Condition& GetProcCondition(void) {
		return m_procCondition;
	}

private:
	AsyncProcThread* _AddThread(void);
	void _ShutdownThreads(void);
	void _TerminateThreads(void);
	void _ClearProcs(void);

private:
	ThreadVector m_threads;
	ProcQueue m_waitQueue;
	ResultQueue m_doneQueue;
	unsigned int m_idAlloc;

	Mutex m_queueMutex;
	Mutex m_threadMutex;
	Condition m_procCondition;
};

#endif
