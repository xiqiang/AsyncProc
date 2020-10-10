#include "AsyncProcManager.h"
#include <cstdio>


AsyncProcManager::AsyncProcManager()
	: m_activeThreadCount(0)
	, m_waitDequeMutex()
	, m_procCondition(&m_waitDequeMutex)
{
}

AsyncProcManager::~AsyncProcManager()
{
	Shutdown();
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
			AutoMutex am_deque(m_waitDequeMutex);
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
	m_callbackDequeMutex.Lock();
	ResultDeque* resultDeque = GetCallbackDeque(AP_GetThreadId());
	if (!resultDeque || resultDeque->empty())
	{
		m_callbackDequeMutex.Unlock();
		return;
	}

	ResultVector results(resultDeque->begin(), resultDeque->end());
	resultDeque->clear();
	m_callbackDequeMutex.Unlock();

	for (ResultVector::iterator it = results.begin(); it != results.end(); ++it)
		NotifyProcDone(*it);
}

void AsyncProcManager::Schedule(AsyncProc* proc)
{
	assert(proc);
	proc->SetScheduleThreadId(AP_GetThreadId());

	AutoMutex am(m_waitDequeMutex);
	m_waitDeque.push_back(proc);
	m_procCondition.Wake();
	OnProcScheduled(proc);
}

void AsyncProcManager::GetCallbackDequeMap(ResultDequeMap& outResultDequeMap) 
{
	AutoMutex am(m_callbackDequeMutex);
	outResultDequeMap.clear();
	outResultDequeMap.insert(m_callbackDequeMap.begin(), m_callbackDequeMap.end());
}

void AsyncProcManager::NotifyProcDone(const AsyncProcResult& result)
{
	result.proc->InvokeCallback(result);
	OnProcDone(result);
	delete result.proc;
}

ResultDeque* AsyncProcManager::GetCallbackDeque(AP_Thread thread_id)
{
	ResultDequeMap::iterator it = m_callbackDequeMap.find(thread_id);
	if (it != m_callbackDequeMap.end())
		return &(it->second);

	std::pair<ResultDequeMap::iterator, bool> ret = m_callbackDequeMap.insert(
		std::pair<AP_Thread, ResultDeque>(thread_id, ResultDeque()));

	if (!ret.second)
		return NULL;
	else
		return &(ret.first->second);
}

void AsyncProcManager::_ShutdownThreads(void)
{
	//printf("AsyncProcManager::_ShutdownThreads()\n");

	AutoMutex am(m_threadMutex);
	if (m_threads.empty())
		return;

	for (ThreadVector::iterator it = m_threads.begin();
		it != m_threads.end(); ++it)
	{
		(*it)->ShutdownNotify();
	}

	m_procCondition.WakeAll();

	for (ThreadVector::iterator it = m_threads.begin();
		it != m_threads.end(); ++it)
	{
		(*it)->ShutdownWait();
		delete *it;
	}
	m_threads.clear();
}

void AsyncProcManager::_ClearProcs(void)
{
	//printf("AsyncProcManager::_ClearProcs()\n");

	{
		AutoMutex am(m_waitDequeMutex);
		for (ProcDeque::iterator it = m_waitDeque.begin();
			it != m_waitDeque.end(); ++it)
		{
			delete *it;
		}
		m_waitDeque.clear();
	}

	{
		AutoMutex am(m_callbackDequeMutex);
		for (ResultDequeMap::iterator itMap = m_callbackDequeMap.begin();
			itMap != m_callbackDequeMap.end(); ++itMap)
		{
			ResultDeque& resultDeque = itMap->second;
			for (ResultDeque::iterator itDeque = resultDeque.begin();
				itDeque != resultDeque.end(); ++itDeque)
			{
				delete itDeque->proc;
			}
		}
		m_callbackDequeMap.clear();
	}
}
