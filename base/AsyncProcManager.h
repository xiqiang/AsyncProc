#ifndef AsyncProcManager_H_Xiqiang_20190907
#define AsyncProcManager_H_Xiqiang_20190907

#include "AsyncProc.h"
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
	void Tick(void);
	void Schedule(AsyncProc* proc);

	void Schedule(AsyncProc* proc, AsyncProcCallback fun) {
		assert(proc);
		proc->SetCallback(fun);
		Schedule(proc);
	}

	template<typename T>
	void Schedule(AsyncProc* proc, T* pVar, void(T::* pMemberFun)(const AsyncProcResult& result)) {
		assert(proc);
		proc->SetCallback(pVar, pMemberFun);
		Schedule(proc);
	}

	void GetCallbackQueueMap(ResultQueueMap& outResultQueueMap);

	size_t GetActiveThreadCount() {
		AutoMutex am(m_waitQueueMutex);
		return m_activeThreadCount;
	}

	size_t GetThreadCount() {
		AutoMutex am(m_threadMutex);
		return m_threads.size();
	}

	size_t GetWaitQueueSize() {
		AutoMutex am(m_waitQueueMutex);
		return m_waitQueue.size();
	}

private:
	void NotifyProcDone(const AsyncProcResult& result);
	ResultQueue* GetCallbackQueue(AP_Thread thread_id);

	ProcQueue& GetWaitQueue(void) {
		return m_waitQueue;					// lock outside
	}

	Mutex& GetWaitQueueMutex(void) {
		return m_waitQueueMutex;
	}

	Mutex& GetCallbackQueueMutex(void) {
		return m_callbackQueueMutex;
	}

	Condition& GetProcCondition(void) {
		return m_procCondition;
	}

	void IncActiveThreadCount() {
		++m_activeThreadCount;				// lock outside
	}

	void DecActiveThreadCount() {
		--m_activeThreadCount;				// lock outside
	}

private:
	void _ShutdownThreads(void);
	void _ClearProcs(void);

private:
	size_t m_activeThreadCount;
	ThreadVector m_threads;
	Mutex m_threadMutex;

	ProcQueue m_waitQueue;
	Mutex m_waitQueueMutex;
	Condition m_procCondition;

	ResultQueueMap m_callbackQueueMap;
	Mutex m_callbackQueueMutex;
};

#endif
