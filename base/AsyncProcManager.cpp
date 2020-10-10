#include "AsyncProcManager.h"
#include <cstdio>


AsyncProcManager::AsyncProcManager()
	: m_waitQueueMutex()
	, m_procCondition(&m_waitQueueMutex)
	, m_activeThreadCount(0)
{
}

AsyncProcManager::~AsyncProcManager()
{
}

void AsyncProcManager::Startup(int threadCount /*= 1*/)
{
	//printf("AsyncProcManager::Startup(threadCount=%d)\n", threadCount);

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
			AutoMutex am_queue(m_waitQueueMutex);
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
	//printf("AsyncProcManager::Shutdown()\n");

	_ShutdownThreads();
	_ClearProcs();
}

void AsyncProcManager::Tick()
{
	AsyncProcResult* apr = NULL;
	size_t aprSize = 0;

	{
		AutoMutex am_queue(m_callbackQueueMutex);
		ResultQueue* resultQueue = GetCallbackQueue(AP_GetThreadId());
		if (!resultQueue)
			return;

		aprSize = resultQueue->size();
		if (0 == aprSize)
			return;

		apr = new AsyncProcResult[aprSize];
		if (!apr)
			return;

		for (size_t i = 0; i < aprSize; ++i)
		{
			apr[i] = resultQueue->front();
			resultQueue->pop();
		}
	}

	for (size_t i = 0; i < aprSize; ++i)
		NotifyProcDone(apr[i]);

	delete[] apr;
}

void AsyncProcManager::Schedule(AsyncProc* proc)
{
	assert(proc);
	proc->SetScheduleThreadId(AP_GetThreadId());

	AutoMutex am(m_waitQueueMutex);
	m_waitQueue.push(proc);
	m_procCondition.Wake();
	OnProcScheduled(proc);
}

void AsyncProcManager::GetCallbackQueueMap(ResultQueueMap& outResultQueueMap) 
{
	AutoMutex am(m_callbackQueueMutex);
	outResultQueueMap.clear();
	outResultQueueMap.insert(m_callbackQueueMap.begin(), m_callbackQueueMap.end());
}

void AsyncProcManager::NotifyProcDone(const AsyncProcResult& result)
{
	result.proc->InvokeCallback(result);
	OnProcDone(result);
	delete result.proc;
}

ResultQueue* AsyncProcManager::GetCallbackQueue(AP_Thread thread_id)
{
	ResultQueueMap::iterator it = m_callbackQueueMap.find(thread_id);
	if (it != m_callbackQueueMap.end())
		return &(it->second);

	std::pair<ResultQueueMap::iterator, bool> ret = m_callbackQueueMap.insert(
		std::pair<AP_Thread, ResultQueue>(thread_id, ResultQueue()));

	if (!ret.second)
		return NULL;
	else
		return &(ret.first->second);
}

void AsyncProcManager::_ShutdownThreads(void)
{
	//printf("AsyncProcManager::_ShutdownThreads()\n");

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
	//printf("AsyncProcManager::_ClearProcs()\n");

	{
		AutoMutex am(m_waitQueueMutex);
		while (m_waitQueue.size() > 0)
		{
			delete m_waitQueue.front();
			m_waitQueue.pop();
		}
	}

	{
		AutoMutex am(m_callbackQueueMutex);
		for (ResultQueueMap::iterator it = m_callbackQueueMap.begin();
			it != m_callbackQueueMap.end(); ++it)
		{
			while (it->second.size() > 0)
			{
				delete it->second.front().proc;
				it->second.pop();
			}
		}
	}
}
