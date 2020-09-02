#include "AsyncProcThread.h"
#include <cstdio>
#include <assert.h>

#if defined(__LINUX__)
#include <cxxabi.h>
#endif	

#if defined(_WIN32) || defined(_WIN64)
DWORD WINAPI AsyncProcThread::ThreadFunc (PVOID arg)
#elif defined(__LINUX__)
void* AsyncProcThread::ThreadFunc(void* arg)
#endif	
{
	AsyncProcThread* procMgr = (AsyncProcThread*)arg;
	procMgr->_ThreadStart();
	return 0;
}

AsyncProcThread::AsyncProcThread(int customID)
	: m_customID(customID)
	, m_count(0)
	, m_state(State_None)
	, m_exeProc(NULL)
{
	printf("AsyncProcThread(addr=%p) :%d\n", this, m_customID);
#if defined(_WIN32) || defined(_WIN64)
	InitializeCriticalSection(&m_queueLock);
	InitializeConditionVariable(&m_awakeCondition);
	m_hThread = INVALID_HANDLE_VALUE;
	m_tid = -1;
#elif defined(__LINUX__)
	pthread_mutex_init(&m_queueLock, NULL);
	pthread_cond_init(&m_awakeCondition, NULL);
#endif	
}

AsyncProcThread::~AsyncProcThread()
{
	_GetLock();
	if(m_state != State_None)
		Shutdown();
	_ReleaseLock();	

#if defined(_WIN32) || defined(_WIN64)
	DeleteCriticalSection(&m_queueLock);
#elif defined(__LINUX__)
	pthread_mutex_destroy(&m_queueLock);
	pthread_cond_destroy(&m_awakeCondition);
#endif	
}

bool AsyncProcThread::Startup(void)
{
	_GetLock();
	assert(State_None == m_state);

	bool ret = false;
#if defined(_WIN32) || defined(_WIN64)
	m_hThread = CreateThread (NULL, 0, ThreadFunc, (PVOID)this, 0, &m_tid);
	ret = (m_hThread != NULL);
#elif defined(__LINUX__)
	ret = pthread_create(&m_tid, NULL, ThreadFunc, (void*)this) == 0;
#endif	

	if(ret)
		m_state = State_Running;
	else
		m_state = State_None;
		
	_ReleaseLock();	
	return ret;
}

void AsyncProcThread::Enqueue(AsyncProc* proc)
{
	_GetLock();

	m_waitQueue.push(proc);
	m_count += 1;

	if(m_state == State_Sleeping)
	{
		printf("AsyncProcThread::Enqueue...Awake %d\n", m_customID);
		
#if defined(_WIN32) || defined(_WIN64)
		WakeConditionVariable (&m_awakeCondition);
#elif defined(__LINUX__)
		pthread_cond_signal(&m_awakeCondition); 
#endif	
		m_state = State_Running;
	}

	_ReleaseLock();
}

void AsyncProcThread::Shutdown(void)
{
	printf("AsyncProcThread::Shutdown...Begin %d\n", m_customID);

	_GetLock();
	assert(m_state != State_None);

	if(m_state == State_Sleeping)
	{
		printf("AsyncProcThread::Shutdown...Awake %d\n", m_customID);

#if defined(_WIN32) || defined(_WIN64)
		WakeConditionVariable (&m_awakeCondition);
#elif defined(__LINUX__)
		pthread_cond_signal(&m_awakeCondition); 
#endif
	}
	m_state = State_Stopping;
	_ReleaseLock();

	printf("AsyncProcThread::Shutdown...Wait: %d\n", m_customID);
#if defined(_WIN32) || defined(_WIN64)
	WaitForSingleObject(m_hThread, INFINITE);
	CloseHandle(m_hThread);
	m_hThread = INVALID_HANDLE_VALUE;
#elif defined(__LINUX__)
	pthread_join(m_tid, NULL);
#endif

	_GetLock();
	m_state = State_None;
	_ReleaseLock();
	_ClearProcs();

	printf("AsyncProcThread::Shutdown...OK: %d\n", m_customID);
}

void AsyncProcThread::Terminate(void)
{	
	printf("AsyncProcThread::Terminate...: %d\n", m_customID);

	_GetLock();
	assert(m_state != State_None);

#if defined(_WIN32) || defined(_WIN64)
	TerminateThread(m_hThread, 0);
#elif defined(__LINUX__)
	pthread_cancel(m_tid);
#endif	

	m_state = State_None;
	_ReleaseLock();
	_ClearProcs();

	printf("AsyncProcThread::Terminate...OK: %d\n", m_customID);
}

void AsyncProcThread::CallbackTick()
{
	AsyncProcResult result;
	_GetLock();
	if(m_doneQueue.size() > 0)
	{
		result = m_doneQueue.front();
		m_doneQueue.pop();
		m_count -= 1;
	}
	_ReleaseLock();

	if(result.proc)
	{
		result.proc->InvokeCallback(result);
		delete result.proc;
	}
}

size_t AsyncProcThread::GetProcCount(void)
{
	size_t num = 0;
	_GetLock();
	num = m_count;
	_ReleaseLock();
	return num;
}

void AsyncProcThread::_ThreadStart()
{
	while(true)
	{
		_ThreadCycle();

		_GetLock();
		if(m_state == State_Stopping)
			break;
		_ReleaseLock();	
	}
		
	_ReleaseLock();	
}

void AsyncProcThread::_ThreadCycle()
{
	_GetLock();
	if(m_waitQueue.size() > 0)
	{
		m_exeProc = m_waitQueue.front();
		m_waitQueue.pop();
	}
	_ReleaseLock();

	if(m_exeProc)
	{
		try 
		{
			m_exeProc->Execute();
			_OnExeEnd(APRT_FINISH, NULL);
		}
		catch (const std::exception & e) 
		{
			_OnExeEnd(APRT_EXCEPTION, e.what());
		}
#if defined(__LINUX__)
		catch (abi::__forced_unwind&) 
		{
			printf("AsyncProcThread::__forced_unwind: %d\n", m_customID);
			throw;
		}
#endif
		catch (...)
		{
			_OnExeEnd(APRT_EXCEPTION, NULL);
		}
	}

	_GetLock();
	if(m_waitQueue.size() == 0 && State_Running == m_state)
	{
		printf("AsyncProcThread::_ThreadCycle...Sleep %d\n", m_customID);

		m_state = State_Sleeping;

#if defined(_WIN32) || defined(_WIN64)
		SleepConditionVariableCS (&m_awakeCondition, &m_queueLock, INFINITE);
#elif defined(__LINUX__)
		pthread_cond_wait(&m_awakeCondition, &m_queueLock); 
#endif					
	}
	_ReleaseLock();
}

void AsyncProcThread::_OnExeEnd(AsyncProcResultType type, const char* what)
{
	_GetLock();
	AsyncProcResult result;
	result.proc = m_exeProc;
	result.type = type;
	if(what)
		result.what = what;

	m_doneQueue.push(result);
	m_exeProc = NULL;
	_ReleaseLock();
}

void AsyncProcThread::_ClearProcs(void)
{
	_GetLock();
	while(m_waitQueue.size() > 0)
	{
		delete m_waitQueue.front();
		m_waitQueue.pop();
	}

	while(m_doneQueue.size() > 0)
	{
		delete m_doneQueue.front().proc;
		m_doneQueue.pop();
	}

	if(m_exeProc)
	{
		delete m_exeProc;
		m_exeProc = NULL;
	}

	_ReleaseLock();
}

void AsyncProcThread::_GetLock()
{
#if defined(_WIN32) || defined(_WIN64)
	EnterCriticalSection(&m_queueLock);
#elif defined(__LINUX__)
	pthread_mutex_lock(&m_queueLock);
#endif	
}

void AsyncProcThread::_ReleaseLock()
{
#if defined(_WIN32) || defined(_WIN64)
	LeaveCriticalSection(&m_queueLock);
#elif defined(__LINUX__)
	pthread_mutex_unlock(&m_queueLock);
#endif	
}
