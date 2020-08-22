#include "AsyncProcManager.h"

#if defined(__WINDOWS__)
DWORD WINAPI AsyncProcManager::ThreadProc (PVOID arg)
#elif defined(__LINUX__)
void* AsyncProcManager::ThreadProc(void* arg)
#endif	
{
	AsyncProcManager* procMgr = (AsyncProcManager*)arg;
	procMgr->_ThreadStart();
	return 0;
}

AsyncProcManager::AsyncProcManager()
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

AsyncProcManager::~AsyncProcManager()
{
#if defined(__WINDOWS__)
	DeleteCriticalSection(&m_queueLock);
#elif defined(__LINUX__)
	pthread_mutex_destroy(&m_queueLock);
	pthread_cond_destroy(&m_awakeCondition);
#endif	
}

void AsyncProcManager::Startup(void)
{
	printf("AsyncProcManager::Startup...\n");
#if defined(__WINDOWS__)
	HANDLE h = CreateThread (NULL, 0, ThreadProc, (PVOID)this, 0, &m_tid);
#elif defined(__LINUX__)
	pthread_create(&m_tid, NULL, ThreadProc, (void*)this); 
#endif	
	printf("AsyncProcManager::Startup...OK.\n");	
}

void AsyncProcManager::Shutdown(void)
{
	printf("AsyncProcManager::Shutdown...\n");
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
	printf("AsyncProcManager::Shutdown...OK.\n");
}

void AsyncProcManager::Tick()
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

void AsyncProcManager::Enqueue(AsyncProc* proc)
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

size_t AsyncProcManager::Count(void)
{
	size_t num = 0;

	_LockQueue();
	num = m_count;
	_UnlockQueue();

	return num;
}

void AsyncProcManager::_ThreadStart()
{
	_LockQueue();
	m_state = State_Running;
	_UnlockQueue();

	while(1)
	{
		_ThreadCycle();

		bool stopping = false;
		_LockQueue();
		stopping = (m_state == State_Stopping);
		_UnlockQueue();	

		if(stopping)
			return;
	}
}

void AsyncProcManager::_ThreadCycle()
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

void AsyncProcManager::_LockQueue()
{
#if defined(__WINDOWS__)
	EnterCriticalSection(&m_queueLock);
#elif defined(__LINUX__)
	pthread_mutex_lock(&m_queueLock);
#endif	
}

void AsyncProcManager::_UnlockQueue()
{
#if defined(__WINDOWS__)
	LeaveCriticalSection(&m_queueLock);
#elif defined(__LINUX__)
	pthread_mutex_unlock(&m_queueLock);
#endif	
}
