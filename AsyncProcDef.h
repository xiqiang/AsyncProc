#ifndef AsyncProcCallback_H_Xiqiang_20200901
#define AsyncProcCallback_H_Xiqiang_20200901

#include <string>
#include <vector>
#include <queue>

class AsyncProc;
class AsyncProcThread;

// Result
// -----------------

enum AsyncProcResultType
{
	APRT_FINISH,
	APRT_EXCEPTION,
	APRT_TERMINATE,
};

struct AsyncProcResult
{
	AsyncProcResult()
		: proc(NULL)
		, type(APRT_FINISH) 
		, thread(-1)
	{}

	AsyncProc* proc;
	AsyncProcResultType	type;
	std::string what;
	int thread;
};

// Callback
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

typedef std::vector<AsyncProcThread*>	ThreadVector;
typedef std::queue<AsyncProc*>			ProcQueue;
typedef std::queue<AsyncProcResult>		ResultQueue;

#endif
