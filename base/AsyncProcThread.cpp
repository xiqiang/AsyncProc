#include "AsyncProcThread.h"
#include "AsyncProcManager.h"
#include "AsyncProc.h"
#include <cstdio>
#include <assert.h>

#if defined(__LINUX__)
#include <cxxabi.h>
#endif	

#include "AutoMutex.h"

#if defined(_WIN32) || defined(_WIN64)
DWORD WINAPI AsyncProcThread::ThreadFunc (PVOID arg)
#elif defined(__LINUX__)
void* AsyncProcThread::ThreadFunc(void* arg)
#endif	
{
	AsyncProcThread* procThread = (AsyncProcThread*)arg;
	procThread->_Execute();
	return 0;
}

AsyncProcThread::AsyncProcThread(AsyncProcManager* manager)
	: m_manager(manager)
	, m_tid(0)
	, m_state(State_None)
	, m_proc(NULL)
	, m_clock(0)
#if defined(_WIN32) || defined(_WIN64)
	, m_hThread(INVALID_HANDLE_VALUE)
#endif
{
}

AsyncProcThread::~AsyncProcThread()
{
}

bool AsyncProcThread::Startup(void)
{
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
		
	//printf("AsyncProcThread::Startup(thread=%lu)\n", m_tid);

	return ret;
}

void AsyncProcThread::ShutdownNotify(void)
{
	//printf("AsyncProcThread::NotifyShutdown(m_tid=%lu\n", m_tid);

	assert(m_state != State_None);
	m_state = State_Quiting;
}

void AsyncProcThread::ShutdownWait(void)
{
#if defined(_WIN32) || defined(_WIN64)
	WaitForSingleObject(m_hThread, INFINITE);
	CloseHandle(m_hThread);
	m_hThread = INVALID_HANDLE_VALUE;
#elif defined(__LINUX__)
	pthread_join(m_tid, NULL);
#endif

	m_tid = 0;
	m_state = State_None;

	//printf("AsyncProcThread::Quit(m_tid=%lu)\n", m_tid);
}

void AsyncProcThread::_Execute()
{
	while(true)
	{
		{
			AutoMutex am(m_manager->GetWaitQueueMutex());
			ProcQueue& waitQueue = m_manager->GetWaitQueue();
			while (waitQueue.size() == 0 && State_Running == m_state)
			{
				//printf("AsyncProcThread::Sleep(m_tid=%lu)\n", m_tid);
				m_manager->DecActiveThreadCount();
				m_manager->GetProcCondition().Sleep();					// Auto unlock queueMutex when sleeped
				m_manager->IncActiveThreadCount();						// Auto lock queueMutex when awoken
				//printf("AsyncProcThread::Wake(m_tid=%lu)\n", m_tid);
			}

			if (m_state != State_Running)
				return;

			m_proc = waitQueue.front();
			assert(m_proc);
			waitQueue.pop();
		}

		try
		{
			m_clock = clock();
			m_proc->Execute();
			_ProcDone(AsyncProcResult::FINISH, NULL);
		}
		catch (const std::exception & e)
		{
			_ProcDone(AsyncProcResult::EXCEPTION, e.what());
		}
#if defined(__LINUX__)
		catch (abi::__forced_unwind&)
		{
			printf("AsyncProcThread::__forced_unwind(m_tid=%lu)\n", m_tid);
			throw;
		}
#endif
		catch (...)
		{
			_ProcDone(AsyncProcResult::EXCEPTION, "exception...");
		}
	}	
}

void AsyncProcThread::_ProcDone(AsyncProcResult::Type type, const char* what)
{
	AsyncProcResult result;
	result.proc = m_proc;
	result.costSeconds = (clock() - m_clock) / (float)CLOCKS_PER_SEC;
	result.type = type;
	result.thread_id = m_tid;
	if(what)
		result.what = what;

	AutoMutex am(m_manager->GetCallbackQueueMutex());
	if (m_proc->HasCallback())
	{
		ResultQueue* resultQueue = m_manager->GetCallbackQueue(m_proc->GetScheduleThreadId());
		if (!resultQueue)
			return;

		resultQueue->push(result);
	}
	else
	{
		m_manager->NotifyProcDone(result);
	}

	m_proc = NULL;
}
