#include <cstdio> 
#include <stdlib.h> 
#include <assert.h>
#include <time.h>

#if defined(_WIN32) || defined(_WIN64)
#define _CRTDBG_MAP_ALLOC
#include <crtdbg.h>
#elif defined(__LINUX__)
#include <pthread.h> 
#include <unistd.h> 
#endif

#include "AsyncProcManager.h"
#include "AsyncProc.h"

class DemoProc
	: public AsyncProc
{
public:
	DemoProc(float duration, float errRatio)
		: m_duration(duration)
		, m_errRatio(errRatio)
	{
		printf("DemoProc::DemoProc(proc=%p, time=%f)\n", this, m_duration);
	}

	virtual ~DemoProc(void)
	{
	}

	virtual void Execute(void)
	{
#if defined(_WIN32) || defined(_WIN64)
		printf("DemoProc::Execute(proc=%p, thread=%lu)\n", this, GetCurrentThreadId());
		Sleep((DWORD)(m_duration * 1000));
#elif defined(__LINUX__)
		printf("DemoProc::Execute(proc=%p, thread=%lu)\n", this, pthread_self());
		usleep((useconds_t)(m_duration * 1000 * 1000));
#endif
		if (rand() / (float)RAND_MAX <= m_errRatio)
			throw "error";
	}

private:
	float m_duration;
	float m_errRatio;
};

const int	THREAD_COUNT = 10;
const int	PROC_COUNT_BASE = 5;
const int	PROC_COUNT_RAND = 15;
const float	PROC_BLOCK_TIME_BASE = 0.0f;
const float	PROC_BLOCK_TIME_RAND = 10.0f;
const float PROC_ERROR_RATIO = 0.3f;

#if defined(_WIN32) || defined(_WIN64)
	DWORD cycleThreadId;
	HANDLE cycleHandle;
#elif defined(__LINUX__)
	pthread_t cycleThreadId;
#endif

AsyncProcManager* apm;
bool alive;

#if defined(_WIN32) || defined(_WIN64)
DWORD WINAPI CycleThreadProc(PVOID arg)
#elif defined(__LINUX__)
void* CycleThreadProc(void* arg)
#endif	
{
	while(alive)
		apm->Tick();
	return 0;
}

void demoProcCallback(const AsyncProcResult& result)
{
	DemoProc* proc = dynamic_cast<DemoProc*>(result.proc);
	assert(proc);
	printf("demoProcCallback(proc=%p, costSeconds=%f, result=%d, what=%s, thread=%lu)\n", proc, result.costSeconds, result.type, result.what.c_str(), result.thread);
}

int main() 
{
	srand(clock());
	apm = new AsyncProcManager();
	apm->Startup(THREAD_COUNT);

#if defined(_WIN32) || defined(_WIN64)
	cycleHandle = CreateThread (NULL, 0, CycleThreadProc, NULL, 0, &cycleThreadId);
#elif defined(__LINUX__)
	pthread_create(&cycleThreadId, NULL, CycleThreadProc, NULL); 
#endif

	alive = true;
	while(alive)
	{
		char c = getchar();
		switch(c)
		{
			case 'q':			
				apm->Shutdown();
				alive = false;
#if defined(_WIN32) || defined(_WIN64)
				WaitForSingleObject(cycleHandle, INFINITE);
#elif defined(__LINUX__)
				pthread_join(cycleThreadId, NULL);
#endif			
				break;
			case 's':
				{
					for(int i = 0; i < rand() % PROC_COUNT_RAND + PROC_COUNT_BASE; ++i) {
						DemoProc* proc = new DemoProc(rand() / (float)RAND_MAX * PROC_BLOCK_TIME_RAND + PROC_BLOCK_TIME_BASE, PROC_ERROR_RATIO);
						apm->Schedule(proc);
						proc->SetCallback(demoProcCallback);
					}
				}
				break;
		}
	}

	delete apm;
	printf("main exit.\n");

#if defined(_WIN32) || defined(_WIN64)		
	CloseHandle(cycleHandle);
	_CrtDumpMemoryLeaks();
#elif defined(__LINUX__)
#endif	

	return 0; 
}