#ifndef AsyncProcCallback_H_Xiqiang_20200901
#define AsyncProcCallback_H_Xiqiang_20200901

#include <string>

class AsyncProc;

// Result struct
// -----------------

enum AsyncProcResultType
{
	APRT_FINISH,
	APRT_EXCEPTION,
	APRT_TIMEOUT,
};

struct AsyncProcResult
{
	AsyncProcResult()
		: proc(NULL)
		, type(APRT_FINISH) {}

	AsyncProc* proc;
	AsyncProcResultType	type;
	std::string what;
};

// Callback invokers
// -----------------

typedef void (*AsyncProcCallback)(const AsyncProcResult& result);

class AsyncProcCaller
{
public:
	virtual ~AsyncProcCaller() = 0;
	virtual void Invoke(const AsyncProcResult& result) = 0;
};

template<class T>
class AsyncProcMemberCaller
	: public AsyncProcCaller
{
	typedef void (T::*MemberFun)(const AsyncProcResult& result);

public:
	AsyncProcMemberCaller(T* p, MemberFun fun)
		: m_ptr(p)
		, m_fun(fun) 
	{
	}

	virtual void Invoke(const AsyncProcResult& result)
	{
		(m_ptr->*m_fun)(result);
	}

private:
	T*					m_ptr;
	MemberFun			m_fun;
};

#endif
