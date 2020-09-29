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
	AsyncProcThread* procThread = (AsyncProcThread*)arg;
	procThread->_ThreadStart();
	return 0;
}

AsyncProcThread::AsyncProcThread(AsyncProcManager* manager, int customID)
	: m_manager(manager)
	, m_customID(customID)
	, m_state(State_None)
	, m_exeProc(NULL)
{
	printf("AsyncProcThread::AsyncProcThread(addr=%p, customID=%d)\n", this, m_customID);
	
	Assert(manager);

#if defined(_WIN32) || defined(_WIN64)
	InitializeCriticalSection(&m_lock);
#elif defined(__LINUX__)
	pthread_mutex_init(&m_lock, NULL);
#endif		
}

AsyncProcThread::~AsyncProcThread()
{
	printf("AsyncProcThread::~AsyncProcThread()\n");

	if(State_Running == m_state)
		Shutdown();
		
#if defined(_WIN32) || defined(_WIN64)
	DeleteCriticalSection(&m_lock);
#elif defined(__LINUX__)
	pthread_mutex_destroy(&m_lock);
#endif	
}

bool AsyncProcThread::Startup(void)
{
	printf("AsyncProcThread::Startup()\n");

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

void AsyncProcThread::Shutdown(void)
{
	printf("AsyncProcThread::Shutdown(m_customID=%d)...Begin\n", m_customID);

	_GetLock();
	assert(m_state != State_None);
	m_state = State_Stopping;
	_ReleaseLock();

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

	printf("AsyncProcThread::Shutdown(m_customID=%d)...OK\n", m_customID);
}

void AsyncProcThread::Terminate(void)
{	
	printf("AsyncProcThread::Terminate(m_customID=%d)...Begin\n", m_customID);

	_GetLock();
	assert(m_state != State_None);

#if defined(_WIN32) || defined(_WIN64)
	TerminateThread(m_hThread, 0);
#elif defined(__LINUX__)
	pthread_cancel(m_tid);
#endif	

	m_state = State_None;
	_ReleaseLock();

	if(m_exeProc)
		_OnExeEnd(APRT_TERMINATE, NULL);

	printf("AsyncProcThread::Terminate(m_customID=%d)...OK\n", m_customID);
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
	m_manager->_GetQueueLock();
	ProcQueue& waitQueue = m_manager->_GetWaitQueue();
	while(procQueue.size() == 0)
		_ThreadWaitNewProc();
	
	m_exeProc = waitQueue.front();
	assert(m_exeProc);

	waitQueue.pop();
	m_manager->_ReleaseQueueLock();

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
		printf("AsyncProcThread::__forced_unwind(m_customID=%d)\n", m_customID);
		throw;
	}
#endif
	catch (...)
	{
		_OnExeEnd(APRT_EXCEPTION, NULL);
	}
}

void AsyncProcThread::_OnExeEnd(AsyncProcResultType type, const char* what)
{
	m_manager->_GetQueueLock();
	AsyncProcResult result;
	result.proc = m_exeProc;
	result.type = type;
	if(what)
		result.what = what;

	ResultQueue& doneQueue = m_manager->_GetDoneQueue();
	doneQueue.push(result);
	m_exeProc = NULL;
	m_manager->_ReleaseQueueLock();
}

void AsyncProcThread::_GetLock(void)
{
#if defined(_WIN32) || defined(_WIN64)
	EnterCriticalSection(&m_lock);
#elif defined(__LINUX__)
	pthread_mutex_lock(&m_lock);
#endif	
}

void AsyncProcThread::_ReleaseLock(void)
{
#if defined(_WIN32) || defined(_WIN64)
	LeaveCriticalSection(&m_lock);
#elif defined(__LINUX__)
	pthread_mutex_unlock(&m_lock);
#endif	
}
