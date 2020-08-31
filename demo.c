#include <pthread.h> 
#include <stdio.h> 
#include <unistd.h> 
#include "AsyncProcManager.h"
#include "demoProc.h"

#if defined(__WINDOWS__)
	CRITICAL_SECTION cycleLock;
	DWORD cycleThreadId;
	HANDLE cycleHandle;
#elif defined(__LINUX__)
	pthread_mutex_t cycleLock;
	pthread_t cycleThreadId;
#endif

AsyncProcManager apm;
bool alive;

#if defined(__WINDOWS__)
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
#if defined(__WINDOWS__)
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
#if defined(__WINDOWS__)
				WaitForSingleObject(cycleHandle, INFINITE);
#elif defined(__LINUX__)
				pthread_join(cycleThreadId, NULL);
#endif			
				break;
			case 's':
				{
					int threadIndex = apm.Enqueue(new SleepProc(5));
					printf("enqued index:%d\n", threadIndex);
				}
				break;
			case 't':
				apm.Terminate();
				alive = false;
#if defined(__WINDOWS__)
				WaitForSingleObject(cycleHandle, INFINITE);
#elif defined(__LINUX__)
				pthread_join(cycleThreadId, NULL);
#endif							
				break;
		}
	}

	return 0; 
} 
