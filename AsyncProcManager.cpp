#include "AsyncProcManager.h"
#include "AsyncProc.h"
#include <cstdio>
#include <assert.h>

AsyncProcManager::AsyncProcManager()
{
	printf("AsyncProcManager(addr=%p)\n", this);

#if defined(_WIN32) || defined(_WIN64)
	InitializeCriticalSection(&m_lockQueue);
	InitializeCriticalSection(&m_lockThread);
	InitializeConditionVariable(&m_condNewProc);
#elif defined(__LINUX__)
	pthread_mutex_init(&m_lockQueue, NULL);
	pthread_mutex_init(&m_lockThread, NULL);
	pthread_cond_init(&m_condNewProc, NULL);
#endif	
}

AsyncProcManager::~AsyncProcManager()
{
	printf("AsyncProcManager::~AsyncProcManager()\n");

	_GetThreadLock();
	if(m_threads.size() > 0)
		Shutdown();
	_ReleaseThreadLock();

#if defined(_WIN32) || defined(_WIN64)
	DeleteCriticalSection(&m_lockQueue);
	DeleteCriticalSection(&m_lockThread);
#elif defined(__LINUX__)
	pthread_mutex_destroy(&m_lockQueue);
	pthread_mutex_destroy(&m_lockThread);
	pthread_cond_destroy(&m_condNewProc);
#endif	
}

void AsyncProcManager::Startup(int threadCount /*= 1*/)
{
	printf("AsyncProcManager::Startup(threadCount=%d)\n", threadCount);

	_GetThreadLock();
	assert(threadCount > 0);
	assert(m_threads.size() == 0);

	for(int t = 0; t < threadCount; ++t)
	{
		AsyncProcThread* thread = new AsyncProcThread(this, t);
		if(!thread)
			break;

		thread->Startup();
		m_threads.push_back(thread);
	}

	_ReleaseThreadLock();
}

void AsyncProcManager::Shutdown(void)
{
	printf("AsyncProcManager::Shutdown()\n");

	_GetThreadLock();
	assert(m_threads.size() > 0);
	for(ThreadVector::iterator it = m_threads.begin();
		it != m_threads.end(); ++it)
	{
		(*it)->Shutdown();
		delete(*it);
	}
	m_threads.clear();
	_ReleaseThreadLock();

#if defined(_WIN32) || defined(_WIN64)
    WakeAllConditionVariable (&m_condNewProc);	
#elif defined(__LINUX__)
	pthread_cond_broadcast(&m_condNewProc); 
#endif
}

void AsyncProcManager::Terminate(void)
{
	printf("AsyncProcManager::Terminate()\n");

	_GetThreadLock();
	assert(m_threads.size() > 0);
	for(ThreadVector::iterator it = m_threads.begin();
		it != m_threads.end(); ++it)
	{
		(*it)->Terminate();
		delete(*it);
	}
	m_threads.clear();
	_ReleaseThreadLock();

	_ClearProcs();
}

void AsyncProcManager::Schedule(AsyncProc* proc)
{
	printf("AsyncProcManager::Schedule(proc=%p)\n", proc);

	_GetQueueLock();
	m_waitQueue.push(proc);
	_ReleaseQueueLock();

#if defined(_WIN32) || defined(_WIN64)
	WakeConditionVariable (&m_condNewProc);
#elif defined(__LINUX__)
	pthread_cond_signal(&m_condNewProc); 
#endif
}

void AsyncProcManager::CallbackTick()
{
	_GetQueueLock();
	while(m_doneQueue.size() > 0)
	{
		AsyncProcResult result = m_doneQueue.front();
		m_doneQueue.pop();

		if(result.proc)
		{
			result.proc->InvokeCallback(result);
			delete result.proc;
		}
	}
	_ReleaseQueueLock();	
}

void AsyncProcManager::_ClearProcs(void)
{
	_GetQueueLock();
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

	_ReleaseQueueLock();
}

ProcQueue& AsyncProcManager::_GetWaitQueue(void)
{
	return m_waitQueue;
}

ResultQueue& AsyncProcManager::_GetDoneQueue(void)
{
	return m_doneQueue;
}

void AsyncProcManager::_GetQueueLock(void)
{
#if defined(_WIN32) || defined(_WIN64)
	EnterCriticalSection(&m_lockQueue);
#elif defined(__LINUX__)
	pthread_mutex_lock(&m_lockQueue);
#endif	
}

void AsyncProcManager::_ReleaseQueueLock(void)
{
#if defined(_WIN32) || defined(_WIN64)
	LeaveCriticalSection(&m_lockQueue);
#elif defined(__LINUX__)
	pthread_mutex_unlock(&m_lockQueue);
#endif	
}

void AsyncProcManager::_GetThreadLock(void)
{
#if defined(_WIN32) || defined(_WIN64)
	EnterCriticalSection(&m_lockThread);
#elif defined(__LINUX__)
	pthread_mutex_lock(&m_lockThread);
#endif	
}

void AsyncProcManager::_ReleaseThreadLock(void)
{
#if defined(_WIN32) || defined(_WIN64)
	LeaveCriticalSection(&m_lockThread);
#elif defined(__LINUX__)
	pthread_mutex_unlock(&m_lockThread);
#endif	
}

void AsyncProcManager::_ThreadWaitNewProc(void)
{
#if defined(_WIN32) || defined(_WIN64)
	SleepConditionVariableCS (&m_condNewProc, &m_lockQueue, INFINITE);
#elif defined(__LINUX__)
	pthread_cond_wait(&m_condNewProc, &m_lockQueue); 
#endif		
}
