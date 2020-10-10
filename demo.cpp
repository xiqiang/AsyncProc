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

const int	WORK_THREAD_COUNT = 500;
const int	TICK_THREAD_COUNT = 10;

const float AUTO_SCHEDULE_SECONDS_MIN = 0.0f;
const float AUTO_SCHEDULE_SECONDS_MAX = 1.0f;

const int	NEW_PROC_COUNT_MIN = 0;
const int	NEW_PROC_COUNT_MAX = 100;

const float	PROC_SLEEP_SECONDS_MIN = 0.0f;
const float	PROC_SLEEP_SECONDS_MAX = 1.0f;
const float PROC_ERROR_RATIO = 0.5f;

class DemoProc : public StatisticProc {
public:
	DemoProc(const std::string& name, float duration, float errRatio)
		: StatisticProc(name)
		, m_duration(duration)
		, m_errRatio(errRatio) {
		//printf("DemoProc::DemoProc(proc=%p, time=%f)\n", this, m_duration);
	}

	virtual void Execute(void) {
		//printf("DemoProc::Execute(proc=%p, thread=%lu)\n", this, (unsigned long)AP_GetThreadId());
#if defined(_WIN32) || defined(_WIN64)
		Sleep((DWORD)(m_duration * 1000));
#elif defined(__LINUX__)
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
DWORD cycleThreadId[TICK_THREAD_COUNT];
HANDLE cycleHandle[TICK_THREAD_COUNT];
#elif defined(__LINUX__)
pthread_t cycleThreadId[TICK_THREAD_COUNT];
#endif

StatisticProcManager* apm = NULL;
bool alive = false;
bool autoSchedule = true;

int RangeRand(int min, int max)
{
	return rand() % (max - min) + min;
}

float RangeRand(float min, float max)
{
	return rand() / (float)RAND_MAX * (max - min) + min;
}

void demoProcCallback(const AsyncProcResult& result)
{
	DemoProc* proc = dynamic_cast<DemoProc*>(result.proc);
	assert(proc);
	//printf("demoProcCallback(proc=%p, thread=%lu, costSeconds=%f, result=%d, what=%s)\n", proc, result.thread_id, result.costSeconds, result.type, result.what.c_str());
}

void Schedule(const char* name, bool hasCallback) 
{
	int count = RangeRand(NEW_PROC_COUNT_MIN, NEW_PROC_COUNT_MAX);
	for (int i = 0; i < count; ++i) {
		float duration = RangeRand(PROC_SLEEP_SECONDS_MIN, PROC_SLEEP_SECONDS_MAX);
		DemoProc* proc = new DemoProc(name, duration, PROC_ERROR_RATIO);

		if (hasCallback)
			apm->Schedule(proc, demoProcCallback);
		else
			apm->Schedule(proc);
	}
}

void ShowStatistics()
{
	printf("++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++\n");
	printf("threads: %lu/%lu, waitQueue:%lu\n", (unsigned long)apm->GetActiveThreadCount(),
		(unsigned long)apm->GetThreadCount(), (unsigned long)apm->GetWaitQueueSize());

	ResultQueueMap resultQueueMap;
	apm->GetCallbackQueueMap(resultQueueMap);
	for (ResultQueueMap::iterator it = resultQueueMap.begin(); it != resultQueueMap.end(); ++it)
	{
		printf("resultQueue[%lu].size = %lu\n", (unsigned long)it->first, (unsigned long)it->second.size());
	}

	StatisticProcInfoMap infoMap;
	apm->GetStatisticInfos(infoMap);
	for (StatisticProcInfoMap::iterator it = infoMap.begin(); it != infoMap.end(); ++it)
	{
		printf("%s: proc=%lu/%lu (%luok, %luerror), cost=%.2f (%.2f-%.2f)\n",
			it->first.c_str(), (unsigned long)it->second.countDone(), (unsigned long)it->second.countScheduled,
			(unsigned long)it->second.countFinish, (unsigned long)it->second.countException,
			it->second.costSecondsAverage(), it->second.costSecondsMin, it->second.costSecondsMax);
	}
	printf("------------------------------------------------------------\n");
}

#if defined(_WIN32) || defined(_WIN64)
DWORD WINAPI CycleThreadProc(PVOID arg)
#elif defined(__LINUX__)
void* CycleThreadProc(void* arg)
#endif	
{
	char* name = (char*)arg;
	clock_t nextClock = clock();

	while (alive)
	{
		clock_t curClock = clock();
		if (autoSchedule && nextClock <= curClock)
		{
			Schedule(name, true);
			nextClock = curClock + (clock_t)(RangeRand(AUTO_SCHEDULE_SECONDS_MIN, AUTO_SCHEDULE_SECONDS_MAX) * CLOCKS_PER_SEC);
		}
		apm->Tick();
	}

	delete name;
	return 0;
}

int main()
{
	printf(
		"\n======================================================================\n"
		"<any char> = shedule procs\n"
		"'o' = auto schedule on\n"
		"'o' = auto schedule off\n"
		"'q' = exit\n"
		"'s' = statistic\n"
		"======================================================================\n\n"
	);

	srand(clock());
	apm = new StatisticProcManager();
	apm->Startup(WORK_THREAD_COUNT);
	alive = true;

	for (int i = 0; i < TICK_THREAD_COUNT; ++i)
	{
		char* name = new char[32];
		name[0] = '\0';
		sprintf(name, "cycle-%d", i);

#if defined(_WIN32) || defined(_WIN64)
		cycleHandle[i] = CreateThread(NULL, 0, CycleThreadProc, (PVOID)name, 0, &cycleThreadId[i]);
#elif defined(__LINUX__)
		pthread_create(&cycleThreadId[i], NULL, CycleThreadProc, (void*)name);
#endif
	}

	while (alive)
	{
		char c = getchar();
		switch (c)
		{
		case '\n':
			break;
		case 'o':
			autoSchedule = true;
			break;
		case 'f':
			autoSchedule = false;
			break;
		case 'q':
			alive = false;
			break;
		case 's':
			ShowStatistics();
			break;
		default:
		{
			char name[16] = { 0 };
			sprintf(name, "demo-%c", c);
			Schedule(name, false);
		}
		break;
		}
	}

	for (int i = 0; i < TICK_THREAD_COUNT; ++i)
	{
#if defined(_WIN32) || defined(_WIN64)
		WaitForSingleObject(cycleHandle[i], INFINITE);
		CloseHandle(cycleHandle[i]);
#elif defined(__LINUX__)
		pthread_join(cycleThreadId[i], NULL);
#endif			
	}

	apm->Shutdown();
	delete apm;

#if defined(_WIN32) || defined(_WIN64)		
	_CrtDumpMemoryLeaks();
#endif	

	return 0;
}