#ifndef AsyncProc_H_Xiqiang_20190907
#define AsyncProc_H_Xiqiang_20190907

#include <time.h>
#include <set>
#include "AsyncProcDef.h"

struct AsyncProcResult;

class AsyncProc
{
public:
	friend class AsyncProcManager;
	friend class AsyncProcThread;

public:
	AsyncProc()
	    : m_callback(NULL)
    	, m_caller(NULL) 
		, m_priority(0)
		, m_scheduleThreadId(0)
		, m_scheduleClock(0)
		, m_exeBeginClock(0)
		, m_exeEndClock(0) {
	}

	virtual ~AsyncProc(void) {
		if(m_caller) {
			delete m_caller;
			m_caller = NULL;
		}		
	}

	virtual void Execute(void) = 0;

public:
	template<typename T>
	bool SetCallback(T* pVar, void(T::*pMemberFun)(const AsyncProcResult& result)) {
		try {
			m_caller = new AsyncProcMemberCaller<T>(pVar, pMemberFun);
			return m_caller != NULL;
		} catch (std::bad_alloc&) {
			return false;
		}
	}

	void SetCallback(AsyncProcCallback fun) {
		m_callback = fun;
	}

	bool HasCallback(void) const {
		return m_caller || m_callback;
	}

	void InvokeCallback(const AsyncProcResult& result) {
		if(m_callback)
			m_callback(result);

		if(m_caller)
			m_caller->Invoke(result);
	}

public:
	AP_Thread GetScheduleThreadId(void) const {
		return m_scheduleThreadId;
	}

	void SetPriority(int value) {
		m_priority = value;
	}

	int GetPriority() {
		return m_priority;
	}

	clock_t GetScheduleClock() {
		return m_scheduleClock;
	}

	clock_t GetExeBeginClock(){
		return m_exeBeginClock;
	}

	clock_t GetExeEndClock(){
		return m_exeEndClock;
	}

private:
	void Init(AP_Thread thread_id) {
		m_scheduleThreadId = thread_id;
		m_scheduleClock = clock();
	}

	void ResetExeBeginClock() {
		m_exeBeginClock = clock();
	}

	void ResetExeEndClock() {
		m_exeEndClock = clock();
	}

private:
	AsyncProcCallback	m_callback;
	AsyncProcCaller*	m_caller;
	int					m_priority;
	AP_Thread			m_scheduleThreadId;
	clock_t				m_scheduleClock;
	clock_t				m_exeBeginClock;
	clock_t				m_exeEndClock;
};

struct AsyncProcLess
{
	bool operator()(AsyncProc* l, AsyncProc* r) const {
		return l->GetPriority() > r->GetPriority();
	}
};

typedef std::multiset<AsyncProc*, AsyncProcLess> ProcMultiset;

#endif
