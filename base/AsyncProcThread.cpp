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
		
	//printf("AsyncProcThread::Startup(thread=%lu)\n", m_tid);

	return ret;
}

void AsyncProcThread::ShutdownNotify(AsyncProcShutdownMode mode)
{
	//printf("AsyncProcThread::NotifyShutdown(m_tid=%lu\n", m_tid);

	assert(m_state != State_None);
	m_state = AsyncProcShutdown_Fast == mode ? State_FastQuiting : State_Quiting;
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
	assert(m_manager);
	m_state = State_Running;
	AP_Thread thread_id = AP_GetThreadId();

	{
		AutoMutex am(m_manager->GetWaitDequeMutex());
		m_manager->IncActiveThread(thread_id);
	}

	while(true)
	{
		{
			AutoMutex am(m_manager->GetWaitDequeMutex());
			ProcMultiset& waitDeque = m_manager->GetWaitDeque();

			switch (m_state)
			{
			case State_FastQuiting:
				return;

			case State_Quiting:
				if (waitDeque.empty())
					return;
				break;

			case State_Running:
				while (waitDeque.empty() && State_Running == m_state)
				{
					//printf("AsyncProcThread::Sleep(m_tid=%lu)\n", m_tid);
					m_manager->DecActiveThread(thread_id);
					m_manager->GetProcCondition().Sleep();						// Auto unlock dequeMutex when sleeped, and lock dequeMutex when awoken
					m_manager->IncActiveThread(thread_id);
					//printf("AsyncProcThread::Wake(m_tid=%lu)\n", m_tid);
				}

				if (m_state != State_Running)
					continue;
				break;

			default:
				continue;
			}

			ProcMultiset::iterator it = waitDeque.begin();
			m_proc = *it;
			waitDeque.erase(it);
			assert(m_proc);
		}

		m_manager->OnThreadPickWork(thread_id, m_proc);

		try
		{
			m_proc->ResetExeBeginClock();
			m_proc->Execute();
			_ProcDone(AsyncProcResult::SUCCESS, NULL);
		}
		catch (const std::exception & e)
		{
			_ProcDone(AsyncProcResult::EXECUTE_EXCEPTION, e.what());
		}
#if defined(__LINUX__)
		catch (abi::__forced_unwind&)
		{
			//printf("AsyncProcThread::__forced_unwind(m_tid=%lu)\n", m_tid);
			throw;
		}
#endif
		catch (...)
		{
			_ProcDone(AsyncProcResult::EXECUTE_EXCEPTION, "exception...");
		}
	}	
}

void AsyncProcThread::_ProcDone(AsyncProcResult::Type type, const char* what)
{
	assert(m_proc);
	m_proc->ResetExeEndClock();

	AsyncProcResult result;
	result.proc = m_proc;
	result.type = type;
	result.thread_id = m_tid;
	if(what)
		result.what = what;

	if (m_proc->HasCallback())
		m_manager->EnqueueCallback(result);
	else
		m_manager->NotifyProcDone(result);

	m_proc = NULL;
}
