#include "AsyncProcManager.h"
#include <cstdio>
#include <algorithm>


AsyncProcManager::AsyncProcManager()
	: m_activeThreadCount(0)
	, m_waitDequeMutex()
	, m_procCondition(&m_waitDequeMutex)
	, m_callbackSize(0)
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
			IncActiveThread(thread->GetId());
		}
		else
		{
			delete thread;
		}
	}
}

void AsyncProcManager::Shutdown(AsyncProcShutdownMode mode /*= AsyncProcShutdown_Normal*/)
{
	//printf("AsyncProcManager::Shutdown(mode=%d)\n", mode);

	_ShutdownThreads(mode);
	_ClearProcs();
}

void AsyncProcManager::Tick()
{
	if (0 == m_callbackSize)
		return;

	m_callbackDequeMutex.Lock();
	ResultDeque* resultDeque = GetCallbackDeque(AP_GetThreadId());
	if (!resultDeque || resultDeque->empty())
	{
		m_callbackDequeMutex.Unlock();
		return;
	}

	ResultVector results(resultDeque->begin(), resultDeque->end());
	m_callbackSize -= resultDeque->size();
	resultDeque->clear();
	m_callbackDequeMutex.Unlock();

	for (ResultVector::iterator it = results.begin(); it != results.end(); ++it)
		NotifyProcDone(*it);
}

struct AsyncProcGreater
{
	bool operator()(AsyncProc* l, AsyncProc* r) {
		return l->GetPriority() > r->GetPriority();
	}
};

void AsyncProcManager::Schedule(AsyncProc* proc, int priority /*= 0*/, bool sortNow /*= true*/)
{
	assert(proc);
	proc->SetScheduleThreadId(AP_GetThreadId());

	AutoMutex am(m_waitDequeMutex);
	m_waitDeque.push_back(proc);

	proc->SetPriority(priority);
	if(sortNow)
		std::sort(m_waitDeque.begin(), m_waitDeque.end(), AsyncProcGreater());

	m_procCondition.Wake();
	OnProcScheduled(proc);
}

void AsyncProcManager::Schedule(AsyncProc* proc, AsyncProcCallback fun, int priority /*= 0*/, bool sortNow /*= true*/)
{
	assert(proc);
	proc->SetCallback(fun);
	Schedule(proc, priority, sortNow);
}

template<typename T>
void AsyncProcManager::Schedule(AsyncProc* proc, T* pVar, void(T::* pMemberFun)(const AsyncProcResult& result), int priority /*= 0*/, bool sortNow /*= true*/)
{
	assert(proc);
	proc->SetCallback(pVar, pMemberFun);
	Schedule(proc, priority, sortNow);
}

void AsyncProcManager::Sort()
{
	AutoMutex am(m_waitDequeMutex);
	std::sort(m_waitDeque.begin(), m_waitDeque.end(), AsyncProcGreater());
}

void AsyncProcManager::GetCallbackDequeMap(ResultDequeMap& outResultDequeMap) 
{
	AutoMutex am(m_callbackDequeMutex);
	outResultDequeMap.clear();
	outResultDequeMap.insert(m_callbackDequeMap.begin(), m_callbackDequeMap.end());
}

void AsyncProcManager::EnqueueCallback(const AsyncProcResult& result)
{
	AutoMutex am(m_callbackDequeMutex);
	ResultDeque* resultDeque = GetCallbackDeque(result.proc->GetScheduleThreadId());
	if (resultDeque)
	{
		resultDeque->push_back(result);
		++m_callbackSize;
	}
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

void AsyncProcManager::_ShutdownThreads(AsyncProcShutdownMode mode)
{
	//printf("AsyncProcManager::_ShutdownThreads(mode=%d)\n", mode);

	ThreadVector waitingThread;
	{
		AutoMutex am(m_threadMutex);
		if (m_threads.empty())
			return;

		for (ThreadVector::iterator it = m_threads.begin();
			it != m_threads.end(); ++it)
		{
			(*it)->ShutdownNotify(mode);
		}

		waitingThread = m_threads;
	}

	m_procCondition.WakeAll();

	for (ThreadVector::iterator it = waitingThread.begin();
		it != waitingThread.end(); ++it)
	{
		(*it)->ShutdownWait();
	}

	{
		AutoMutex am(m_threadMutex);
		for (ThreadVector::iterator it = m_threads.begin();
			it != m_threads.end(); ++it)
		{
			delete* it;
		}
		m_threads.clear();
		m_activeThreadCount = 0;
	}
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
