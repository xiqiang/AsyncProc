#include "AsyncProcManager.h"
#include "AsyncProc.h"
#include <cstdio>


AsyncProcManager::AsyncProcManager()
	: m_queueMutex()
	, m_procCondition(&m_queueMutex)
	, m_activeThreadCount(0)
{
}

AsyncProcManager::~AsyncProcManager()
{
}

void AsyncProcManager::Startup(int threadCount /*= 1*/)
{
	printf("AsyncProcManager::Startup(threadCount=%d)\n", threadCount);

	assert(threadCount > 0);

	AutoMutex am_thread(m_threadMutex);
	for (int t = 0; t < threadCount; ++t)
	{
		AsyncProcThread* thread = new AsyncProcThread(this);
		if (!thread)
			break;

		if (thread->Startup())
		{
			m_threads.push_back(thread);
			AutoMutex am_queue(m_queueMutex);
			IncActiveThreadCount();
		}
		else
		{
			delete thread;
		}
	}
}

void AsyncProcManager::Shutdown(void)
{
	printf("AsyncProcManager::Shutdown()\n");

	_ShutdownThreads();
	_ClearProcs();
}

void AsyncProcManager::Schedule(AsyncProc* proc)
{
	AutoMutex am(m_queueMutex);
	m_waitQueue.push(proc);
	m_procCondition.Wake();
	OnProcScheduled(proc);
}

void AsyncProcManager::Tick()
{
	AutoMutex am_queue(m_queueMutex);
	while (m_doneQueue.size() > 0)
	{
		AsyncProcResult result = m_doneQueue.front();
		m_doneQueue.pop();

		if (result.proc)
		{
			OnProcDone(result);
			result.proc->InvokeCallback(result);
			delete result.proc;
		}
	}
}

void AsyncProcManager::_ShutdownThreads(void)
{
	printf("AsyncProcManager::_ShutdownThreads()\n");

	AutoMutex am(m_threadMutex);
	for (ThreadVector::iterator it = m_threads.begin();
		it != m_threads.end(); ++it)
	{
		(*it)->ShutdownNotify();
	}

	m_procCondition.WakeAll();

	assert(m_threads.size() > 0);
	for (ThreadVector::iterator it = m_threads.begin();
		it != m_threads.end(); ++it)
	{
		(*it)->ShutdownWait();
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
