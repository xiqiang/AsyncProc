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
	virtual void OnProcBlocked(AsyncProc* proc) {}
	virtual void OnProcDropped(AsyncProc* proc, bool replace) {}
	virtual void OnProcDone(const AsyncProcResult& result) {}
	
	virtual void OnThreadAwake(AP_Thread thread_id) {}							// NOTICE: under m_waitQueueMutex be locked outside
	virtual void OnThreadSleep(AP_Thread thread_id) {}							// NOTICE: under m_waitQueueMutex be locked outside
	virtual void OnThreadPickWork(AP_Thread thread_id, AsyncProc* proc) {}

protected:
	virtual bool BlockCheck(AsyncProc* proc) { return false; }

public:
	void Startup(int threadCount = 4, size_t maxWaitSize = 65535);
	void Shutdown(AsyncProcShutdownMode mode = AsyncProcShutdown_Normal);
	void Tick(void);

	bool Schedule(AsyncProc* proc);
	bool Schedule(AsyncProc* proc, AsyncProcCallback fun);

	template<typename T>
	bool Schedule(AsyncProc* proc, T* pVar, void(T::* pMemberFun)(const AsyncProcResult& result)) {
		assert(proc);
		proc->SetCallback(pVar, pMemberFun);
		return Schedule(proc);
	}

	bool AddThread();

public:
	void SetMaxWaitSize(size_t maxWaitSize) {
		AutoMutex am(m_waitQueueMutex);
		m_maxWaitSize = maxWaitSize;
	}

	size_t GetMaxWaitSize() const {
		return m_maxWaitSize;								// No need lock for outside use.
	}

	size_t GetCallbackSize() const {
		return m_callbackSize;
	}

	size_t GetActiveThreadCount() {
		AutoMutex am(m_waitQueueMutex);
		return m_activeThreadCount;
	}

	size_t GetThreadCount() {
		AutoMutex am(m_threadMutex);
		return m_threads.size();
	}

	size_t GetWaitDequeSize() {
		AutoMutex am(m_waitQueueMutex);
		return m_waitQueue.size();
	}

protected:
	ResultDeque* GetCallbackDeque(AP_Thread thread_id, bool autoCreate);

	ProcMultiset& GetWaitDeque(void) {
		return m_waitQueue;									// m_waitQueueMutex must be locked outside
	}

	Mutex& GetWaitDequeMutex(void) {
		return m_waitQueueMutex;
	}

	Mutex& GetCallbackDequeMutex(void) {
		return m_callbackDequeMutex;
	}

	Condition& GetProcCondition(void) {
		return m_procCondition;
	}

private:
	void NotifyProcDone(const AsyncProcResult& result);		// called by AsyncProcThread
	void EnqueueCallback(AsyncProcResult& result);			// called by AsyncProcThread
	void IncActiveThread(AP_Thread thread_id);				// called by AsyncProcThread
	void DecActiveThread(AP_Thread thread_id);				// called by AsyncProcThread

private:
	void _ShutdownThreads(AsyncProcShutdownMode mode);
	void _ClearProcs(void);

private:
	size_t				m_activeThreadCount;
	ThreadVector		m_threads;
	Mutex				m_threadMutex;

	ProcMultiset		m_waitQueue;
	Mutex				m_waitQueueMutex;
	Condition			m_procCondition;

	ResultDequeMap		m_callbackDequeMap;
	Mutex				m_callbackDequeMutex;

	size_t				m_maxWaitSize;
	size_t				m_callbackSize;
};

#endif
