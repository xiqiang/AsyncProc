#if defined(_WIN32) || defined(_WIN64)
#define _CRTDBG_MAP_ALLOC
#define _CRT_SECURE_NO_WARNINGS
#include <crtdbg.h>
#elif defined(__LINUX__)
#include <pthread.h> 
#include <unistd.h> 
#endif

#include <cstdio> 
#include <stdlib.h> 
#include <assert.h>
#include <time.h>

#include "statistic/StatisticProc.h"
#include "statistic/StatisticProcManager.h"

const int	THREAD_COUNT = 10;
const int	PROC_COUNT_BASE = 5;
const int	PROC_COUNT_RAND = 15;
const float	PROC_BLOCK_TIME_BASE = 0.0f;
const float	PROC_BLOCK_TIME_RAND = 10.0f;
const float PROC_ERROR_RATIO = 0.5f;

class DemoProc : public StatisticProc {
public:
	DemoProc(const std::string& name, float duration, float errRatio)
		: StatisticProc(name)
		, m_duration(duration)
		, m_errRatio(errRatio) {
		printf("DemoProc::DemoProc(proc=%p, time=%f)\n", this, m_duration);
	}

	virtual void Execute(void) {
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

#if defined(_WIN32) || defined(_WIN64)
DWORD cycleThreadId;
HANDLE cycleHandle;
#elif defined(__LINUX__)
pthread_t cycleThreadId;
#endif

StatisticProcManager* apm;
bool alive;

#if defined(_WIN32) || defined(_WIN64)
DWORD WINAPI CycleThreadProc(PVOID arg)
#elif defined(__LINUX__)
void* CycleThreadProc(void* arg)
#endif	
{
	while (alive)
		apm->Tick();
	return 0;
}

void demoProcCallback(const AsyncProcResult& result)
{
	DemoProc* proc = dynamic_cast<DemoProc*>(result.proc);
	assert(proc);
	printf("demoProcCallback(proc=%p, thread=%lu, costSeconds=%f, result=%d, what=%s)\n", proc, result.thread, result.costSeconds, result.type, result.what.c_str());
}

int main()
{
	printf(
		"\n======================================================================\n"
		"<any char> = shedule procs\n"
		"'q' = exit\n"
		"'s' = statistic\n"
		"======================================================================\n\n"
	);

	srand(clock());
	apm = new StatisticProcManager();
	apm->Startup(THREAD_COUNT);
	alive = true;

#if defined(_WIN32) || defined(_WIN64)
	cycleHandle = CreateThread(NULL, 0, CycleThreadProc, NULL, 0, &cycleThreadId);
#elif defined(__LINUX__)
	pthread_create(&cycleThreadId, NULL, CycleThreadProc, NULL);
#endif

	while (alive)
	{
		char c = getchar();
		switch (c)
		{
		case '\n':
			break;
		case 'q':
		{
			apm->Shutdown();
			alive = false;
#if defined(_WIN32) || defined(_WIN64)
			WaitForSingleObject(cycleHandle, INFINITE);
#elif defined(__LINUX__)
			pthread_join(cycleThreadId, NULL);
#endif			
		}
		break;
		case 's':
		{
			printf("++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++\n");
			StatisticProcInfoMap infoMap;
			apm->GetStatisticInfos(infoMap);
			for (StatisticProcInfoMap::iterator
				it = infoMap.begin(); it != infoMap.end(); ++it)
			{
				printf("%s: schedule=%lu, finish=%lu, exception=%lu, cost=%.2f (%.2f-%.2f)\n",
					it->first.c_str(), it->second.countScheduled, it->second.countFinish, it->second.countException,
					it->second.costSecondsAverage(), it->second.costSecondsMin, it->second.costSecondsMax);
			}
			printf("------------------------------------------------------------\n");
		}
		break;
		default:
		{
			int count = rand() % PROC_COUNT_RAND + PROC_COUNT_BASE;
			for (int i = 0; i < count; ++i) {
				char name[16] = { 0 };
				sprintf(name, "demo-%c", c);
				float duration = rand() / (float)RAND_MAX * PROC_BLOCK_TIME_RAND + PROC_BLOCK_TIME_BASE;
				DemoProc* proc = new DemoProc(name, duration, PROC_ERROR_RATIO);
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