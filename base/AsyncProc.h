#ifndef AsyncProc_H_Xiqiang_20190907
#define AsyncProc_H_Xiqiang_20190907

#include "AsyncProcDef.h"

struct AsyncProcResult;

class AsyncProc
{
public:
	friend class AsyncProcManager;

public:
	AsyncProc()
	    : m_callback(NULL)
    	, m_caller(NULL) 
		, m_scheduleThreadId(0)
		, m_priority(0) {
	}

	virtual ~AsyncProc(void) {
		if(m_caller) {
			delete m_caller;
			m_caller = NULL;
		}		
	}

	virtual void Execute(void) {}

public:
	template<typename T>
	void SetCallback(T* pVar, void(T::*pMemberFun)(const AsyncProcResult& result)) {
		m_caller = new AsyncProcMemberCaller<T>(pVar, pMemberFun);
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

	AP_Thread GetScheduleThreadId(void) const {
		return m_scheduleThreadId;
	}

	int GetPriority() {
		return m_priority;
	}

private:
	void SetScheduleThreadId(AP_Thread thread_id) {
		m_scheduleThreadId = thread_id;
	}

	void SetPriority(int value) {
		m_priority = value;
	}

private:
	AsyncProcCallback m_callback;
	AsyncProcCaller* m_caller;
	AP_Thread m_scheduleThreadId;
	int m_priority;
};

#endif
