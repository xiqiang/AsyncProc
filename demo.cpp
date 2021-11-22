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
const int	TICK_THREAD_COUNT = 500;

const float AUTO_SCHEDULE_SECONDS_MIN = 0.0f;
const float AUTO_SCHEDULE_SECONDS_MAX = 1.0f;

const int	NEW_PROC_COUNT_MIN = 0;
const int	NEW_PROC_COUNT_MAX = 10;

const float	PROC_SLEEP_SECONDS_MIN = 0.0f;
const float	PROC_SLEEP_SECONDS_MAX = 1.0f;
const float PROC_ERROR_RATIO = 0.5f;

const float ECHO_INFO_MILLISECONDS = 1000;

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
DWORD infoThreadId;
HANDLE infoHandle;
#elif defined(__LINUX__)
pthread_t cycleThreadId[TICK_THREAD_COUNT];
pthread_t infoThreadId;
#endif

StatisticProcManager* apm = NULL;
bool aliveTick = false;
bool aliveInfo = false;
bool autoSchedule = true;
bool fastQuit = false;
clock_t infoClock;

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

void Schedule(const char* name) 
{
	int count = RangeRand(NEW_PROC_COUNT_MIN, NEW_PROC_COUNT_MAX);
	for (int i = 0; i < count; ++i) {
		float duration = RangeRand(PROC_SLEEP_SECONDS_MIN, PROC_SLEEP_SECONDS_MAX);
		DemoProc* proc = new DemoProc(name, duration, PROC_ERROR_RATIO);

		if (rand() % 2 == 0)
			apm->Schedule(proc, demoProcCallback, rand() % 1000);
		else
			apm->Schedule(proc, rand() % 1000);
	}
}

void ShowStatistics()
{
	printf("++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++\n");
	printf("activeThread: %lu/%lu, waitQueue: %lu\n", (unsigned long)apm->GetActiveThreadCount(),
		(unsigned long)apm->GetThreadCount(), (unsigned long)apm->GetWaitDequeSize());

	WorkingNameMap workingNameMap;
	apm->GetWorkingNameMap(workingNameMap);
	for (WorkingNameMap::iterator it = workingNameMap.begin(); it != workingNameMap.end(); ++it)
		printf("workThread[%lu].proc: \"%s\"\n", (unsigned long)it->first, it->second.c_str());

	ResultDequeMap resultDequeMap;
	apm->GetCallbackDequeMap(resultDequeMap);
	for (ResultDequeMap::iterator it = resultDequeMap.begin(); it != resultDequeMap.end(); ++it)
		printf("tickThread[%lu].resultQueue: %lu\n", (unsigned long)it->first, (unsigned long)it->second.size());

	StatisticProcInfoMap infoMap;
	apm->GetStatisticInfos(infoMap);
	for (StatisticProcInfoMap::iterator it = infoMap.begin(); it != infoMap.end(); ++it)
	{
		printf("\"%s\": count=%lu/%lu (%luok, %luerr), cost=%.2f (%.2f-%.2f)\n",
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

	while (aliveTick)
	{
		clock_t curClock = clock();
		if (autoSchedule && nextClock <= curClock)
		{
			Schedule(name);
			nextClock = curClock + (clock_t)(RangeRand(AUTO_SCHEDULE_SECONDS_MIN, AUTO_SCHEDULE_SECONDS_MAX) * CLOCKS_PER_SEC);
		}
		apm->Tick();
	}

	delete name;
	return 0;
}

#if defined(_WIN32) || defined(_WIN64)
DWORD WINAPI InfoThreadProc(PVOID arg)
#elif defined(__LINUX__)
void* InfoThreadProc(void* arg)
#endif	
{
	while (aliveInfo)
	{
#if defined(_WIN32) || defined(_WIN64)
		Sleep((DWORD)(ECHO_INFO_MILLISECONDS));
#elif defined(__LINUX__)
		usleep((useconds_t)(ECHO_INFO_MILLISECONDS * 1000));
#endif
		printf("activeThread: %lu/%lu, waitQueue: %lu\n", (unsigned long)apm->GetActiveThreadCount(),
			(unsigned long)apm->GetThreadCount(), (unsigned long)apm->GetWaitDequeSize());
	}

	return 0;
}

int main()
{
	printf(
		"\n======================================================================\n"
		"<any char> = manual shedule\n"
		"'t' = manual tick\n"
		"'o' = auto schedule on\n"
		"'f' = auto schedule off\n"
		"'q' = fast quit\n"
		"'e' = normal quit\n"
		"'s' = statistic\n"
		"======================================================================\n\n"
	);

	srand(clock());
	apm = new StatisticProcManager();
	apm->Startup(WORK_THREAD_COUNT);
	aliveTick = true;
	aliveInfo = true;
	infoClock = clock();

	for (int i = 0; i < TICK_THREAD_COUNT; ++i)
	{
		char* name = new char[32];
		name[0] = '\0';
		sprintf(name, "auto-%d", i);

#if defined(_WIN32) || defined(_WIN64)
		cycleHandle[i] = CreateThread(NULL, 0, CycleThreadProc, (PVOID)name, 0, &cycleThreadId[i]);
#elif defined(__LINUX__)
		pthread_create(&cycleThreadId[i], NULL, CycleThreadProc, (void*)name);
#endif
	}

#if defined(_WIN32) || defined(_WIN64)
	infoHandle = CreateThread(NULL, 0, InfoThreadProc, (PVOID)0, 0, &infoThreadId);
#elif defined(__LINUX__)
	pthread_create(&infoThreadId, NULL, InfoThreadProc, (void*)0);
#endif

	while (aliveTick)
	{
		char c = getchar();
		switch (c)
		{
		case '\n':
			break;
		case 't':
			apm->Tick();
			break;
		case 'o':
			autoSchedule = true;
			break;
		case 'f':
			autoSchedule = false;
			break;
		case 'e':
			aliveTick = false;
			fastQuit = false;
			printf("exiting(normal)...\n");
			break;
		case 'q':
			aliveTick = false;
			fastQuit = true;
			printf("exiting(fast)...\n");
			break;
		case 's':
			ShowStatistics();
			break;
		default:
		{
			char name[16] = { 0 };
			sprintf(name, "manual-%c", c);
			Schedule(name);
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

		//printf("waiting thread... %d\n", i);
	}

	apm->Shutdown(fastQuit ? AsyncProcShutdown_Fast : AsyncProcShutdown_Normal);

	aliveInfo = false;
#if defined(_WIN32) || defined(_WIN64)
	WaitForSingleObject(infoHandle, INFINITE);
	CloseHandle(infoHandle);
#elif defined(__LINUX__)
	pthread_join(infoThreadId, NULL);
#endif	

	delete apm;

#if defined(_WIN32) || defined(_WIN64)		
	_CrtDumpMemoryLeaks();
#endif	

	return 0;
}