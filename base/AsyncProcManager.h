#ifndef AsyncProcManager_H_Xiqiang_20190907
#define AsyncProcManager_H_Xiqiang_20190907

#include "AsyncProcDef.h"
#include "AsyncProcThread.h"
#include "Condition.h"
#include "AutoMutex.h"

class AsyncProcManager
{
public:
	friend class AsyncProcThread;

public:
	AsyncProcManager();
	~AsyncProcManager();

public:
	virtual void OnProcScheduled(AsyncProc* proc) {}
	virtual void OnProcDone(const AsyncProcResult& result) {}

public:
	void Startup(int threadCount = 1);
	void Shutdown(void);

	void Schedule(AsyncProc* proc);
	void Tick(void);

	size_t GetThreadCount() {
		AutoMutex am(m_threadMutex);
		return m_threads.size();
	}

	size_t GetWaitQueueSize() {
		AutoMutex am(m_queueMutex);
		return m_waitQueue.size();
	}

	size_t GetDoneQueueSize() {
		AutoMutex am(m_queueMutex);
		return m_doneQueue.size();
	}

protected:
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
	void _ShutdownThreads(void);
	void _ClearProcs(void);

private:
	ThreadVector m_threads;
	ProcQueue m_waitQueue;
	ResultQueue m_doneQueue;

	Mutex m_queueMutex;
	Mutex m_threadMutex;
	Condition m_procCondition;
};

#endif
