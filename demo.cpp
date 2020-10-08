

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
#include "demoProc.h"

#if defined(_WIN32) || defined(_WIN64)
	CRITICAL_SECTION cycleLock;
	DWORD cycleThreadId;
	HANDLE cycleHandle;
#elif defined(__LINUX__)
	pthread_mutex_t cycleLock;
	pthread_t cycleThreadId;
#endif

AsyncProcManager* apm;
bool alive;
int procIndex = 0;

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
	printf("demoProcCallback(time=%d,tid=%d).Execute...%d(%s) :%d\n", proc->GetMilliSeconds(), result.thread, result.type, result.what.c_str(), proc->GetID());
}

int main() 
{
	apm = new AsyncProcManager();
	apm->Startup(10);

	srand(clock());

#if defined(_WIN32) || defined(_WIN64)
	InitializeCriticalSection(&cycleLock);
	cycleHandle = CreateThread (NULL, 0, CycleThreadProc, NULL, 0, &cycleThreadId);
#elif defined(__LINUX__)
	pthread_mutex_init(&cycleLock, NULL);
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
					for(int i = 0; i < rand() % 5 + 5; ++i) {
						DemoProc* proc = new DemoProc(procIndex++, rand() % 3000 + 2000);
						apm->Schedule(proc);
						proc->SetCallback(demoProcCallback);
					}
				}
				break;
			case 't':
				apm->Terminate();
				alive = false;
#if defined(_WIN32) || defined(_WIN64)
				WaitForSingleObject(cycleHandle, INFINITE);		
#elif defined(__LINUX__)
				pthread_join(cycleThreadId, NULL);
#endif							
				break;
		}
	}

	delete apm;
	printf("main exit.\n");

#if defined(_WIN32) || defined(_WIN64)		
	DeleteCriticalSection(&cycleLock);
	CloseHandle(cycleHandle);
	_CrtDumpMemoryLeaks();
#elif defined(__LINUX__)
	pthread_mutex_destroy(&cycleLock);
#endif	

	return 0; 
}