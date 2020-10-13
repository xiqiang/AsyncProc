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
	AP_Thread thread_id = AP_GetThreadId();

	while(true)
	{
		{
			AutoMutex am(m_manager->GetWaitDequeMutex());
			ProcDeque& waitDeque = m_manager->GetWaitDeque();
			while (waitDeque.empty() && State_Running == m_state)
			{
				//printf("AsyncProcThread::Sleep(m_tid=%lu)\n", m_tid);
				m_manager->DecActiveThread(thread_id);
				m_manager->GetProcCondition().Sleep();					// Auto unlock dequeMutex when sleeped
				m_manager->IncActiveThread(thread_id);						// Auto lock dequeMutex when awoken
				//printf("AsyncProcThread::Wake(m_tid=%lu)\n", m_tid);
			}

			if (m_state != State_Running)
				return;

			m_proc = waitDeque.front();
			assert(m_proc);
			waitDeque.pop_front();
			m_manager->OnThreadPickWork(thread_id, m_proc);
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

	if (m_proc->HasCallback())
	{
		AutoMutex am(m_manager->GetCallbackDequeMutex());
		ResultDeque* resultDeque = m_manager->GetCallbackDeque(m_proc->GetScheduleThreadId());
		if (resultDeque)
			resultDeque->push_back(result);
	}
	else
	{
		m_manager->NotifyProcDone(result);
	}

	m_proc = NULL;
}
