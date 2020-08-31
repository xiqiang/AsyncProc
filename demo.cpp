

#include <cstdio> 
#include <stdlib.h> 
#if defined(_WIN32) || defined(_WIN64)

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

AsyncProcManager apm;
bool alive;
int procIndex = 0;

#if defined(_WIN32) || defined(_WIN64)
DWORD WINAPI CycleThreadProc(PVOID arg)
#elif defined(__LINUX__)
void* CycleThreadProc(void* arg)
#endif	
{
	apm.Startup(10);

	while(alive)
		apm.CallbackTick();

	return 0;
}

int main() 
{ 
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
				apm.Shutdown();
				alive = false;
#if defined(_WIN32) || defined(_WIN64)
				WaitForSingleObject(cycleHandle, INFINITE);
#elif defined(__LINUX__)
				pthread_join(cycleThreadId, NULL);
#endif			
				break;
			case 's':
				{
					for(int i = 0; i < rand() % 100; ++i) {
						SleepProc* proc = new SleepProc(rand() % 10000);
						int threadIndex = apm.Enqueue(proc);
						proc->SetThreadIndex(threadIndex);
						proc->SetProcIndex(procIndex++);
					}
				}
				break;
			case 't':
				apm.Terminate();
				alive = false;
#if defined(_WIN32) || defined(_WIN64)
				WaitForSingleObject(cycleHandle, INFINITE);
#elif defined(__LINUX__)
				pthread_join(cycleThreadId, NULL);
#endif							
				break;
		}
	}

	printf("main exit.\n");
	return 0; 
}