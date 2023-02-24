#ifndef AsyncProcManager_H_Xiqiang_20190907
#define AsyncProcManager_H_Xiqiang_20190907

#include <assert.h>
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
	virtual void OnProcOverflowed(AsyncProc* proc) {}
	virtual void OnProcDone(const AsyncProcResult& result) {}
	virtual void OnThreadPickWork(AP_Thread thread_id, AsyncProc* proc) {}
	virtual void OnThreadSleep(AP_Thread thread_id) {}

public:
	void Startup(int threadCount = 4, size_t maxWaitSize = 65535);
	void Shutdown(AsyncProcShutdownMode mode = AsyncProcShutdown_Normal);
	void Tick(void);

	bool Schedule(AsyncProc* proc, int priority = 0, bool sortNow = true);
	bool Schedule(AsyncProc* proc, AsyncProcCallback fun, int priority = 0, bool sortNow = true);

	template<typename T>
	bool Schedule(AsyncProc* proc, T* pVar, void(T::* pMemberFun)(const AsyncProcResult& result), int priority = 0, bool sortNow = true) {
		assert(proc);
		proc->SetCallback(pVar, pMemberFun);
		return Schedule(proc, priority, sortNow);
	}

	void Sort();

	void GetCallbackDequeMap(ResultDequeMap& outResultDequeMap);
	size_t GetCallbackSize() {
		return m_callbackSize;
	}

	size_t GetActiveThreadCount() {
		AutoMutex am(m_waitDequeMutex);
		return m_activeThreadCount;
	}

	size_t GetThreadCount() {
		AutoMutex am(m_threadMutex);
		return m_threads.size();
	}

	size_t GetWaitDequeSize() {
		AutoMutex am(m_waitDequeMutex);
		return m_waitDeque.size();
	}

protected:
	void EnqueueCallback(const AsyncProcResult& result);
	void NotifyProcDone(const AsyncProcResult& result);
	ResultDeque* GetCallbackDeque(AP_Thread thread_id);

	ProcDeque& GetWaitDeque(void) {
		return m_waitDeque;					// lock outside
	}

	Mutex& GetWaitDequeMutex(void) {
		return m_waitDequeMutex;
	}

	Mutex& GetCallbackDequeMutex(void) {
		return m_callbackDequeMutex;
	}

	Condition& GetProcCondition(void) {
		return m_procCondition;
	}

	void IncActiveThread(AP_Thread thread_id) {
		++m_activeThreadCount;						// lock waitDeque outside
	}

	void DecActiveThread(AP_Thread thread_id) {
		--m_activeThreadCount;						// lock waitDeque outside
		OnThreadSleep(thread_id);
	}

private:
	void _ShutdownThreads(AsyncProcShutdownMode mode);
	void _ClearProcs(void);

private:
	size_t m_activeThreadCount;
	ThreadVector m_threads;
	Mutex m_threadMutex;

	ProcDeque m_waitDeque;
	Mutex m_waitDequeMutex;
	Condition m_procCondition;

	ResultDequeMap m_callbackDequeMap;
	Mutex m_callbackDequeMutex;

	size_t m_maxWaitSize;
	size_t m_callbackSize;
};

#endif
