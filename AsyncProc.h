#ifndef AsyncProc_H_Xiqiang_20190907
#define AsyncProc_H_Xiqiang_20190907

#include <time.h>
#include "AsyncProcDef.h"

class AsyncProc
{
public:
	AsyncProc(void)
	    : m_callback(NULL)
    	, m_caller(NULL) 
		, m_clockStart(0)
		, m_timeout(0.0f)
	{
	}

	virtual ~AsyncProc(void) 
	{
		if(m_caller) 
		{
			delete m_caller;
			m_caller = NULL;
		}		
	}

	virtual void Execute(void) {}

public:
	template<class T>
	void SetCallback( T* pVar, void(T::*pMemberFun)(const AsyncProcResult& result))
	{
		m_caller = new AsyncProcMemberCaller<T>(pVar, pMemberFun);
	}

	void SetCallback(AsyncProcCallback fun)
	{
		m_callback = fun;
	}	

	void InvokeCallback(const AsyncProcResult& result)
	{
		if(m_callback)
			m_callback(result);

		if(m_caller)
			m_caller->Invoke(result);
	}

	void SetTimeout(float seconds)
	{
		m_timeout = seconds;
	}

	void ResetTime(void)
	{
		m_clockStart = clock();
	}

	bool IsTimeout(void) const
	{
		if (m_timeout <= 0)
			return false;
		else
			return (clock() - m_clockStart) / (float)CLOCKS_PER_SEC >= m_timeout;
	}

private:
	AsyncProcCallback m_callback;
	AsyncProcCaller* m_caller;
	clock_t m_clockStart;
	float m_timeout;
};

#endif
