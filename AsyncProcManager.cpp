#include "AsyncProcManager.h"
#include "AsyncProc.h"
#include <cstdio>

#include "AutoMutex.h"

AsyncProcManager::AsyncProcManager()
	: m_idAlloc(0)
	, m_queueMutex()
	, m_procCondition(&m_queueMutex)
{
	printf("AsyncProcManager(addr=%p)\n", this);
}

AsyncProcManager::~AsyncProcManager()
{
	printf("AsyncProcManager::~AsyncProcManager()\n");

	Terminate();
}

void AsyncProcManager::Startup(int threadCount /*= 1*/)
{
	printf("AsyncProcManager::Startup(threadCount=%d)\n", threadCount);

	assert(threadCount > 0);
	for (int t = 0; t < threadCount; ++t)
		_AddThread();
}

void AsyncProcManager::Shutdown(void)
{
	printf("AsyncProcManager::Shutdown()\n");

	_ShutdownThreads();
	_ClearProcs();
}

void AsyncProcManager::Terminate(void)
{
	printf("AsyncProcManager::Terminate()\n");

	_TerminateThreads();
	_ClearProcs();
}

void AsyncProcManager::Schedule(AsyncProc* proc)
{
	printf("AsyncProcManager::Schedule(proc=%p)\n", proc);

	AutoMutex am(m_queueMutex);
	m_waitQueue.push(proc);
	m_procCondition.Wake();
}

void AsyncProcManager::Tick()
{
	AutoMutex am(m_queueMutex);
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
}

AsyncProcThread* AsyncProcManager::_AddThread(void)
{
	printf("AsyncProcManager::_AddThread()\n");

	AutoMutex am(m_threadMutex);
	AsyncProcThread* thread = new AsyncProcThread(this, m_idAlloc++);
	if (thread)
	{
		thread->Startup();
		m_threads.push_back(thread);
	}

	return thread;
}

void AsyncProcManager::_ShutdownThreads(void)
{
	printf("AsyncProcManager::_ShutdownThreads()\n");

	AutoMutex am(m_threadMutex);
	for (ThreadVector::iterator it = m_threads.begin();
		it != m_threads.end(); ++it)
	{
		(*it)->NotifyQuit();
	}

	m_procCondition.WakeAll();

	assert(m_threads.size() > 0);
	for (ThreadVector::iterator it = m_threads.begin();
		it != m_threads.end(); ++it)
	{
		(*it)->QuitWait();
		delete(*it);
	}
	m_threads.clear();
}

void AsyncProcManager::_TerminateThreads(void)
{
	printf("AsyncProcManager::_TerminateThreads()\n");

	AutoMutex am(m_threadMutex);
	for (ThreadVector::iterator it = m_threads.begin();
		it != m_threads.end(); ++it)
	{
		(*it)->Terminate();
		delete(*it);
	}
	m_threads.clear();
}

void AsyncProcManager::_ClearProcs(void)
{
	printf("AsyncProcManager::_ClearProcs()\n");

	AutoMutex am(m_queueMutex);
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
}
