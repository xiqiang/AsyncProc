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
	procThread->_ThreadStart();
	return 0;
}

AsyncProcThread::AsyncProcThread(AsyncProcManager* manager, int customID)
	: m_manager(manager)
	, m_customID(customID)
	, m_state(State_None)
	, m_exeProc(NULL)
{
	printf("AsyncProcThread::AsyncProcThread(addr=%p, customID=%d)\n", this, m_customID);
	
	assert(manager);	
}

AsyncProcThread::~AsyncProcThread()
{
	printf("AsyncProcThread::~AsyncProcThread()\n");

	if(m_state != State_None)
		Terminate();
}

bool AsyncProcThread::Startup(void)
{
	printf("AsyncProcThread::Startup()\n");

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
		
	return ret;
}

void AsyncProcThread::NotifyQuit(void)
{
	printf("AsyncProcThread::NotifyQuit(m_customID=%d)...Begin\n", m_customID);

	assert(m_state != State_None);
	m_state = State_Quiting;
}

void AsyncProcThread::QuitWait(void)
{
	printf("AsyncProcThread::QuitWait(m_customID=%d)...Begin\n", m_customID);

#if defined(_WIN32) || defined(_WIN64)
	WaitForSingleObject(m_hThread, INFINITE);
	CloseHandle(m_hThread);
	m_hThread = INVALID_HANDLE_VALUE;
#elif defined(__LINUX__)
	pthread_join(m_tid, NULL);
#endif

	m_state = State_None;

	printf("AsyncProcThread::Shutdown(m_customID=%d)...OK\n", m_customID);
}

void AsyncProcThread::Terminate(void)
{	
	printf("AsyncProcThread::Terminate(m_customID=%d)...Begin\n", m_customID);

	assert(m_state != State_None);

#if defined(_WIN32) || defined(_WIN64)
	TerminateThread(m_hThread, 0);
#elif defined(__LINUX__)
	pthread_cancel(m_tid);
#endif	

	m_state = State_None;

	if(m_exeProc)
		_OnExeEnd(APRT_TERMINATE, NULL);

	printf("AsyncProcThread::Terminate(m_customID=%d)...OK\n", m_customID);
}

void AsyncProcThread::_ThreadStart()
{
	while(true)
	{
		{
			AutoMutex am(m_manager->GetQueueMutex());
			ProcQueue& waitQueue = m_manager->GetWaitQueue();
			while (waitQueue.size() == 0 && State_Running == m_state)
				m_manager->GetProcCondition().Sleep();					// auto unlock queueMutex when sleep

			if (m_state != State_Running)
				return;

			m_exeProc = waitQueue.front();								// auto lock queueMutex when wake
			assert(m_exeProc);
			waitQueue.pop();
		}

		try
		{
			m_exeProc->ResetTime();
			m_exeProc->Execute();
			_OnExeEnd(APRT_FINISH, NULL);
		}
		catch (const std::exception & e)
		{
			_OnExeEnd(APRT_EXCEPTION, e.what());
		}
#if defined(__LINUX__)
		catch (abi::__forced_unwind&)
		{
			printf("AsyncProcThread::__forced_unwind(m_customID=%d)\n", m_customID);
			throw;
		}
#endif
		catch (...)
		{
			_OnExeEnd(APRT_EXCEPTION, NULL);
		}
	}	
}

void AsyncProcThread::_OnExeEnd(AsyncProcResultType type, const char* what)
{
	AsyncProcResult result;
	result.proc = m_exeProc;
	result.type = type;
	result.thread = m_customID;
	if(what)
		result.what = what;

	AutoMutex am(m_manager->GetQueueMutex());
	m_manager->GetDoneQueue().push(result);
	m_exeProc = NULL;
}
