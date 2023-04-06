#include "AsyncProcManager.h"
#include <cstdio>
#include <algorithm>


AsyncProcManager::AsyncProcManager()
	: m_activeThreadCount(0)
	, m_waitQueueMutex()
	, m_procCondition(&m_waitQueueMutex)
	, m_maxWaitSize(0)
	, m_callbackSize(0)
{
}

AsyncProcManager::~AsyncProcManager()
{
	Shutdown();
}

void AsyncProcManager::Startup(int threadCount /*= 1*/, size_t maxWaitSize /*= 65535*/)
{
	//printf("AsyncProcManager::Startup(threadCount=%d)\n", threadCount);

	assert(threadCount > 0);
	assert(maxWaitSize > 0);
	m_maxWaitSize = maxWaitSize;

	for (int t = 0; t < threadCount; ++t)
		AddThread();
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

	ResultDeque tmpCallbacks;

	{
		AutoMutex am(m_callbackDequeMutex);
		ResultDeque* callbackDeque = GetCallbackDeque(AP_GetThreadId(), false);
		if (!callbackDeque || callbackDeque->empty())
			return;

		tmpCallbacks.swap(*callbackDeque);
		m_callbackSize -= tmpCallbacks.size();
	}

	for (ResultDeque::iterator it = tmpCallbacks.begin(); it != tmpCallbacks.end(); ++it)
		NotifyProcDone(*it);
}

bool AsyncProcManager::Schedule(AsyncProc* proc)
{
	assert(proc);
	proc->Init(AP_GetThreadId());
	OnProcScheduled(proc);

	bool overflow = false;
	try
	{
		AutoMutex am(m_waitQueueMutex);
		if (m_waitQueue.size() < m_maxWaitSize)
			m_waitQueue.push(proc);
		else
			overflow = true;
	}
	catch (std::bad_alloc&)
	{
		overflow = true;
	}

	if (overflow)
	{
		OnProcOverflowed(proc);
		delete proc;
		return false;
	}

	m_procCondition.Wake();
	return true;
}

bool AsyncProcManager::Schedule(AsyncProc* proc, AsyncProcCallback fun)
{
	assert(proc);
	proc->SetCallback(fun);
	return Schedule(proc);
}

bool AsyncProcManager::AddThread()
{
	try
	{
		AsyncProcThread* thread = new AsyncProcThread(this);
		if (!thread)
			return false;

		if (thread->Startup())
		{
			AutoMutex am_thread(m_threadMutex);
			m_threads.push_back(thread);
			return true;
		}
		else
		{
			delete thread;
			return false;
		}
	}
	catch (std::bad_alloc&)
	{
		return false;
	}
}

ResultDeque* AsyncProcManager::GetCallbackDeque(AP_Thread thread_id, bool autoCreate)
{
	// NOTICE: under m_callbackDequeMutex be locked outside

	ResultDequeMap::iterator it = m_callbackDequeMap.find(thread_id);
	if (it != m_callbackDequeMap.end())
		return &(it->second);

	if (!autoCreate)
		return NULL;

	try
	{
		std::pair<ResultDequeMap::iterator, bool> ret = m_callbackDequeMap.insert(
			std::pair<AP_Thread, ResultDeque>(thread_id, ResultDeque()));

		if (ret.second)
			return &(ret.first->second);
		else
			return NULL;
	}
	catch (std::bad_alloc&)
	{
		return NULL;
	}
}

void AsyncProcManager::NotifyProcDone(const AsyncProcResult& result)
{
	result.proc->InvokeCallback(result);	// Do nothing when !HasCallback()

	OnProcDone(result);
	delete result.proc;
}

void AsyncProcManager::EnqueueCallback(AsyncProcResult& result)
{
	try
	{
		AutoMutex am(m_callbackDequeMutex);
		ResultDeque* resultDeque = GetCallbackDeque(result.proc->GetScheduleThreadId(), true);
		if (resultDeque)
		{
			resultDeque->push_back(result);
			++m_callbackSize;
			return;
		}
	}
	catch (std::bad_alloc&)
	{
	}

	result.type = AsyncProcResult::CALLBACK_ERROR;
	OnProcDone(result);
	delete result.proc;
}

void AsyncProcManager::IncActiveThread(AP_Thread thread_id) 
{
	// NOTICE: under m_waitQueueMutex be locked outside

	++m_activeThreadCount;
	OnThreadAwake(thread_id);
}

void AsyncProcManager::DecActiveThread(AP_Thread thread_id) 
{
	// NOTICE: under m_waitQueueMutex be locked outside

	--m_activeThreadCount;
	OnThreadSleep(thread_id);
}

void AsyncProcManager::_ShutdownThreads(AsyncProcShutdownMode mode)
{
	//printf("AsyncProcManager::_ShutdownThreads(mode=%d)\n", mode);

	ThreadVector waitingThread;

	{
		AutoMutex am(m_threadMutex);
		if (m_threads.empty())
			return;

		waitingThread = m_threads;
	}

	for (ThreadVector::iterator it = waitingThread.begin();
		it != waitingThread.end(); ++it)
	{
		(*it)->ShutdownNotify(mode);
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
		AutoMutex am(m_waitQueueMutex);
		while (!m_waitQueue.empty())
		{
			AsyncProc* proc = m_waitQueue.top();
			delete proc;
			m_waitQueue.pop();
		}
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
