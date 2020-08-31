#include "AsyncProcManager.h"
#include <cstdio>
#include <assert.h>

AsyncProcManager::AsyncProcManager()
{
#if defined(_WIN32) || defined(_WIN64)
	InitializeCriticalSection(&m_queueLock);
#elif defined(__LINUX__)
	pthread_mutex_init(&m_queueLock, NULL);
#endif	
}

AsyncProcManager::~AsyncProcManager()
{
	printf("AsyncProcManager::~AsyncProcManager\n");

	_GetLock();
	if(m_threads.size() > 0)
		Shutdown();
	_ReleaseLock();

#if defined(_WIN32) || defined(_WIN64)
	DeleteCriticalSection(&m_queueLock);
#elif defined(__LINUX__)
	pthread_mutex_destroy(&m_queueLock);
#endif	
}

void AsyncProcManager::Startup(int threadCount /*= 1*/)
{
	_GetLock();
	assert(threadCount > 0);
	assert(m_threads.size() == 0);

	for(int t = 0; t < threadCount; ++t)
	{
		AsyncProcThread* thread = new AsyncProcThread(t);
		if(!thread)
			break;

		thread->Startup();
		m_threads.push_back(thread);
	}

	_ReleaseLock();
}

int AsyncProcManager::Enqueue(AsyncProc* proc)
{
	_GetLock();
	if(m_threads.size() == 0)
		return -1;

	assert(proc);
	int minCount = m_threads[0]->GetProcCount();
	size_t threadIndex = 0;


	for(size_t t = 1; t < m_threads.size(); ++t)
	{
		int count = m_threads[t]->GetProcCount();
		if(count < minCount)
		{
			minCount = count;
			threadIndex = t;
		}
	}

	_ReleaseLock();
	return Enqueue(proc, threadIndex);
}

int AsyncProcManager::Enqueue(AsyncProc* proc, size_t threadIndex)
{
	_GetLock();
	if(m_threads.size() == 0)
		return -1;

	assert(proc);
	assert(threadIndex < m_threads.size());
	m_threads[threadIndex]->Enqueue(proc);

	_ReleaseLock();
	return threadIndex;
}

void AsyncProcManager::Shutdown(void)
{
	_GetLock();
	assert(m_threads.size() > 0);
	for(std::vector<AsyncProcThread*>::iterator it = m_threads.begin();
		it != m_threads.end(); ++it)
	{
		(*it)->Shutdown();
		delete(*it);
	}
	m_threads.clear();
	_ReleaseLock();
}

void AsyncProcManager::Terminate(void)
{
	_GetLock();
	assert(m_threads.size() > 0);
	for(std::vector<AsyncProcThread*>::iterator it = m_threads.begin();
		it != m_threads.end(); ++it)
	{
		(*it)->Terminate();
		delete(*it);
	}
	m_threads.clear();
	_ReleaseLock();
}

void AsyncProcManager::CallbackTick()
{
	_GetLock();
	for(std::vector<AsyncProcThread*>::iterator it = m_threads.begin();
		it != m_threads.end(); ++it)
	{
		(*it)->CallbackTick();
	}
	_ReleaseLock();
}

size_t AsyncProcManager::GetProcCount(void)
{
	_GetLock();
	size_t count = 0;
	for(std::vector<AsyncProcThread*>::iterator it = m_threads.begin();
		it != m_threads.end(); ++it)
	{
		count += (*it)->GetProcCount();
	}
	_ReleaseLock();
	return count;
}

void AsyncProcManager::_GetLock()
{
#if defined(_WIN32) || defined(_WIN64)
	EnterCriticalSection(&m_queueLock);
#elif defined(__LINUX__)
	pthread_mutex_lock(&m_queueLock);
#endif	
}

void AsyncProcManager::_ReleaseLock()
{
#if defined(_WIN32) || defined(_WIN64)
	LeaveCriticalSection(&m_queueLock);
#elif defined(__LINUX__)
	pthread_mutex_unlock(&m_queueLock);
#endif	
}
