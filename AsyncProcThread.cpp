#include "AsyncProcThread.h"

#if defined(__WINDOWS__)
DWORD WINAPI AsyncProcThread::ThreadProc (PVOID arg)
#elif defined(__LINUX__)
void* AsyncProcThread::ThreadProc(void* arg)
#endif	
{
	AsyncProcThread* procMgr = (AsyncProcThread*)arg;
	procMgr->_ThreadStart();
	return 0;
}

AsyncProcThread::AsyncProcThread()
	: m_count(0)
	, m_state(State_None)
{
#if defined(__WINDOWS__)
	InitializeCriticalSection(&m_queueLock);
	InitializeConditionVariable(&m_awakeCondition);
#elif defined(__LINUX__)
	pthread_mutex_init(&m_queueLock, NULL);
	pthread_cond_init(&m_awakeCondition, NULL);
#endif	
}

AsyncProcThread::~AsyncProcThread()
{
#if defined(__WINDOWS__)
	DeleteCriticalSection(&m_queueLock);
#elif defined(__LINUX__)
	pthread_mutex_destroy(&m_queueLock);
	pthread_cond_destroy(&m_awakeCondition);
#endif	
}

void AsyncProcThread::Startup(void)
{
	printf("AsyncProcThread::Startup...\n");
#if defined(__WINDOWS__)
	m_hThread = CreateThread (NULL, 0, ThreadProc, (PVOID)this, 0, &m_tid);
#elif defined(__LINUX__)
	pthread_create(&m_tid, NULL, ThreadProc, (void*)this); 
#endif	
	printf("AsyncProcThread::Startup...OK.\n");	
}

void AsyncProcThread::Enqueue(AsyncProc* proc)
{
	_LockQueue();

	m_waitQueue.push(proc);
	m_count += 1;

	if(m_state == State_Sleeping)
	{
#if defined(__WINDOWS__)
		WakeConditionVariable (&m_awakeCondition);
#elif defined(__LINUX__)
		pthread_cond_signal(&m_awakeCondition); 
#endif	
		m_state = State_Running;
	}

	_UnlockQueue();
}

void AsyncProcThread::Shutdown(void)
{
	printf("AsyncProcThread::Shutdown...\n");
	if(m_state == State_Sleeping)
	{
#if defined(__WINDOWS__)
		WakeConditionVariable (&m_awakeCondition);
#elif defined(__LINUX__)
		pthread_cond_signal(&m_awakeCondition); 
#endif
	}

	_LockQueue();
	m_state = State_Stopping;
	_UnlockQueue();

#if defined(__WINDOWS__)
	WaitForSingleObject(m_hThread, INFINITE);
#elif defined(__LINUX__)
	pthread_join(m_tid, NULL);
#endif
	printf("AsyncProcThread::Shutdown...OK.\n");
}

void AsyncProcThread::Terminate(void)
{
	printf("AsyncProcThread::Terminate...\n");
#if defined(__WINDOWS__)
	TerminateThread(m_hThread, 0);
#elif defined(__LINUX__)
	pthread_cancel(m_tid);
#endif	
}

void AsyncProcThread::CallbackTick()
{
	_LockQueue();
	while(m_doneQueue.size() > 0)
	{
		m_callbackQueue.push(m_doneQueue.front());
		m_doneQueue.pop();
	}
	_UnlockQueue();

	while(m_callbackQueue.size() > 0)
	{
		AsyncProc* proc = m_callbackQueue.front();
		proc->InvokeCallback();

		delete proc;
		m_callbackQueue.pop();

		_LockQueue();
		m_count -= 1;
		_UnlockQueue();
	}
}

size_t AsyncProcThread::Count(void)
{
	size_t num = 0;

	_LockQueue();
	num = m_count;
	_UnlockQueue();

	return num;
}

void AsyncProcThread::_ThreadStart()
{
	_LockQueue();
	m_state = State_Running;
	_UnlockQueue();

	while(true)
	{
		bool stopping = false;
		_LockQueue();
		stopping = (m_state == State_Stopping);
		_UnlockQueue();	

		if(stopping)
			return;

		_ThreadCycle();
	}
}

void AsyncProcThread::_ThreadCycle()
{
	_LockQueue();
	while(m_waitQueue.size() > 0)
	{
		m_executeQueue.push(m_waitQueue.front());
		m_waitQueue.pop();
	}
	_UnlockQueue();

	while(m_executeQueue.size() > 0)
	{
		AsyncProc* proc = m_executeQueue.front();
		proc->Execute();

		_LockQueue();
		m_doneQueue.push(proc);
		m_executeQueue.pop();
		_UnlockQueue();		
	}

	_LockQueue();
	while (m_waitQueue.size() == 0 && m_state == State_Running)
	{
		m_state = State_Sleeping;
#if defined(__WINDOWS__)
		SleepConditionVariableCS (&m_awakeCondition, &m_queueLock, INFINITE);
#elif defined(__LINUX__)
		pthread_cond_wait(&m_awakeCondition, &m_queueLock); 
#endif					
	}
	_UnlockQueue();
}

void AsyncProcThread::_LockQueue()
{
#if defined(__WINDOWS__)
	EnterCriticalSection(&m_queueLock);
#elif defined(__LINUX__)
	pthread_mutex_lock(&m_queueLock);
#endif	
}

void AsyncProcThread::_UnlockQueue()
{
#if defined(__WINDOWS__)
	LeaveCriticalSection(&m_queueLock);
#elif defined(__LINUX__)
	pthread_mutex_unlock(&m_queueLock);
#endif	
}
