#ifndef AsyncProcDef_H_Xiqiang_20200901
#define AsyncProcDef_H_Xiqiang_20200901

#include <string>
#include <vector>
#include <queue>
#include <map>

#if defined(_WIN32) || defined(_WIN64)

#define WIN32_LEAN_AND_MEAN
#include <windows.h>

typedef DWORD AP_Thread;
#define AP_GetThreadId	GetCurrentThreadId

#elif defined(__LINUX__)

#include <pthread.h>

typedef pthread_t AP_Thread;
#define AP_GetThreadId	pthread_self

#endif

class AsyncProc;
class AsyncProcThread;

// Result
// -----------------

struct AsyncProcResult
{
	enum Type
	{
		FINISH,
		EXCEPTION,
	};

	AsyncProcResult()
		: proc(NULL)
		, costSeconds(0.0f)
		, type(Type::FINISH)
		, thread_id(-1)
	{}

	AsyncProc* proc;
	float costSeconds;
	Type type;
	std::string what;
	AP_Thread thread_id;

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
	T*			m_ptr;
	MemberFun	m_fun;
};

typedef std::vector<AsyncProcThread*>		ThreadVector;
typedef std::queue<AsyncProc*>				ProcQueue;
typedef std::queue<AsyncProcResult>			ResultQueue;
typedef std::map<AP_Thread, ResultQueue>	ResultQueueMap;

#endif
