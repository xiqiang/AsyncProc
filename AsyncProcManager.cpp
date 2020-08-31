#include "AsyncProcManager.h"
#include <cstdio>
#include <assert.h>

AsyncProcManager::AsyncProcManager()
{

}

AsyncProcManager::~AsyncProcManager()
{
	printf("AsyncProcManager::~AsyncProcManager\n");

	if(m_threads.size() > 0)
		Shutdown();
}

void AsyncProcManager::Startup(int threadCount /*= 1*/)
{
	assert(threadCount > 0);
	assert(m_threads.size() == 0);
	for(int t = 0; t < threadCount; ++t)
	{
		AsyncProcThread* thread = new AsyncProcThread();
		if(!thread)
			break;

		thread->Startup();
		m_threads.push_back(thread);
	}
}

int AsyncProcManager::Enqueue(AsyncProc* proc)
{
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

	return Enqueue(proc, threadIndex);
}

int AsyncProcManager::Enqueue(AsyncProc* proc, size_t threadIndex)
{
	if(m_threads.size() == 0)
		return -1;

	assert(proc);
	assert(threadIndex < m_threads.size());

	m_threads[threadIndex]->Enqueue(proc);
	return threadIndex;
}

void AsyncProcManager::Shutdown(void)
{
	assert(m_threads.size() > 0);
	for(std::vector<AsyncProcThread*>::iterator it = m_threads.begin();
		it != m_threads.end(); ++it)
	{
		(*it)->Shutdown();
		delete(*it);
	}
	m_threads.clear();
}

void AsyncProcManager::Terminate(void)
{
	assert(m_threads.size() > 0);
	for(std::vector<AsyncProcThread*>::iterator it = m_threads.begin();
		it != m_threads.end(); ++it)
	{
		(*it)->Terminate();
		delete(*it);
	}
	m_threads.clear();
}

void AsyncProcManager::CallbackTick()
{
	for(std::vector<AsyncProcThread*>::iterator it = m_threads.begin();
		it != m_threads.end(); ++it)
	{
		(*it)->CallbackTick();
	}	
}

size_t AsyncProcManager::GetProcCount(void)
{
	size_t count = 0;
	for(std::vector<AsyncProcThread*>::iterator it = m_threads.begin();
		it != m_threads.end(); ++it)
	{
		count += (*it)->GetProcCount();
	}
	return count;
}
