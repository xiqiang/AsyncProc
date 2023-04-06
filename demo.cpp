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

const int	WORK_THREAD_COUNT = 1024;
const int	TICK_THREAD_COUNT = 1024;
const int	MAX_WAIT_SIZE = 200000;

const int	ADD_WORK_THREAD_COUNT_MIN = 1;
const int	ADD_WORK_THREAD_COUNT_MAX = 4;

const int	AUTO_SCHEDULE_SLEEP_MILLISECONDS = 10;
const float AUTO_SCHEDULE_SECONDS_MIN = 0.0f;
const float AUTO_SCHEDULE_SECONDS_MAX = 1.0f;

const int	NEW_PROC_COUNT_MIN = 1;
const int	NEW_PROC_COUNT_MAX = 3;

const float	PROC_SLEEP_SECONDS_MIN = 0.0f;
const float	PROC_SLEEP_SECONDS_MAX = 1.0f;
const float PROC_HAS_CALLBACK_RATIO = 0.5f;
const float PROC_ERROR_RATIO = 0.5f;
const float PROC_SORT_RATIO = 0.f;

const bool	PROC_PRIORITY_RANDOM = false;
const int	PROC_PRIORITY_RANGE = 10000;

const int	ECHO_SLEEP_MILLISECONDS = 1000;

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
		if (rand() / (float)RAND_MAX < m_errRatio)
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
int infoThreadCreateRet;
#endif

StatisticProcManager* apm = NULL;
int tickThreadCount = 0;
bool aliveTick = false;
bool aliveInfo = false;
bool autoSchedule = true;
bool autoInfo = true;
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

void Schedule(const char* name, int priority)
{
	try {
		int count = RangeRand(NEW_PROC_COUNT_MIN, NEW_PROC_COUNT_MAX);
		for (int i = 0; i < count; ++i) {
			float duration = RangeRand(PROC_SLEEP_SECONDS_MIN, PROC_SLEEP_SECONDS_MAX);

			DemoProc* proc = new DemoProc(name, duration, PROC_ERROR_RATIO);
			if (!proc)
				return;

			proc->SetPriority(priority);
			if (rand() / (float)RAND_MAX < PROC_HAS_CALLBACK_RATIO)
				apm->Schedule(proc, demoProcCallback);
			else
				apm->Schedule(proc);
		}
	}
	catch (std::bad_alloc&) {
		autoSchedule = false;
		printf("\nNot enough memory! autoSchedule stopped.\n");
	}
}

void ShowStatistics()
{
	printf("++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++\n");
	printf("threads: active=%lu/%lu\n", 
		(unsigned long)apm->GetActiveThreadCount(), (unsigned long)apm->GetThreadCount());

	WorkingNameMap workingNameMap;
	apm->GetWorkingNameMap(workingNameMap);
	for (WorkingNameMap::iterator it = workingNameMap.begin(); it != workingNameMap.end(); ++it)
		printf("thread[%lu].proc: \"%s\"\n", (unsigned long)it->first, it->second.c_str());

	printf("\nprocs:\ttotal=%lu, wait=%lu, drop=%lu, finish=%lu/%lu(%lu.ok, %lu.exc, %lu.err)\n", 
		(unsigned long)apm->GetTotalScheduled(), (unsigned long)apm->GetWaitDequeSize(), (unsigned long)apm->GetTotalOverflowed(), 
		(unsigned long)apm->GetTotalFinishCount(), (unsigned long)apm->GetTotalQueuedCount(), (unsigned long)apm->GetTotalSucceeded(), 
		(unsigned long)apm->GetTotalExecuteException(), (unsigned long)apm->GetTotalCallbackError());

	StatisticProcInfoMap infoMap;
	apm->GetStatisticInfos(infoMap);
	for (StatisticProcInfoMap::iterator it = infoMap.begin(); it != infoMap.end(); ++it)
	{
		printf("\n\"%s\":\n\ttotal=%lu, drop=%lu, finish=%lu/%lu(%lu.ok, %lu.exc, %lu.err)\n\twait=%.2f(%.2f-%.2f), execute=%.2f(%.2f-%.2f), callback=%.2f(%.2f-%.2f)\n",
			it->first.c_str(), (unsigned long)it->second.countScheduled, (unsigned long)it->second.countOverflowed, (unsigned long)it->second.countFinish(), (unsigned long)it->second.countQueue(),
			(unsigned long)it->second.countSuccess, (unsigned long)it->second.countExecuteException, (unsigned long)it->second.countCallbackError,
			it->second.queueSecondsAverage(), it->second.queueSecondsMin, it->second.queueSecondsMax,
			it->second.executeSecondsAverage(), it->second.executeSecondsMin, it->second.executeSecondsMax,
			it->second.callbackSecondsAverage(), it->second.callbackSecondsMin, it->second.callbackSecondsMax);
	}
	printf("------------------------------------------------------------\n");
}

#if defined(_WIN32) || defined(_WIN64)
DWORD WINAPI TickThreadProc(PVOID arg)
#elif defined(__LINUX__)
void* TickThreadProc(void* arg)
#endif	
{
	int* index = (int*)arg;
	clock_t nextClock = clock();

	char name[64] = { 0 };
	sprintf(name, "proc-%d", *index);

	while (aliveTick)
	{
#if defined(_WIN32) || defined(_WIN64)
		Sleep(AUTO_SCHEDULE_SLEEP_MILLISECONDS);
#elif defined(__LINUX__)
		usleep(AUTO_SCHEDULE_SLEEP_MILLISECONDS * 1000);
#endif
		clock_t curClock = clock();
		if (autoSchedule && nextClock <= curClock)
		{
			int priority = PROC_PRIORITY_RANDOM ? (rand() % PROC_PRIORITY_RANGE) : *index;
			Schedule(name, priority);
			nextClock = curClock + (clock_t)(RangeRand(AUTO_SCHEDULE_SECONDS_MIN, AUTO_SCHEDULE_SECONDS_MAX) * CLOCKS_PER_SEC);
		}
		apm->Tick();
	}

	delete index;
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
		Sleep(ECHO_SLEEP_MILLISECONDS);
#elif defined(__LINUX__)
		usleep(ECHO_SLEEP_MILLISECONDS * 1000);
#endif
		if (!autoInfo)
			continue;

		printf("threads: %lu/%lu, procs: wait=%lu, drop=%lu, finish=%lu/%lu, callback=%lu\n", 
			(unsigned long)apm->GetActiveThreadCount(), (unsigned long)apm->GetThreadCount(), 
			(unsigned long)apm->GetWaitDequeSize(), (unsigned long)apm->GetTotalOverflowed(), 
			(unsigned long)apm->GetTotalFinishCount(), (unsigned long)apm->GetTotalQueuedCount(),
			(unsigned long)apm->GetCallbackSize());
	}

	return 0;
}

int main()
{
	srand(clock());
	apm = new StatisticProcManager();
	if (NULL == apm)
	{
		printf("Initialize apm failed: Not enough memory!\n");
		return 0;
	}
	apm->Startup(WORK_THREAD_COUNT, MAX_WAIT_SIZE);

	aliveTick = true;
	aliveInfo = true;
	infoClock = clock();

	for (int i = 0; i < TICK_THREAD_COUNT; ++i)
	{
		int* index = new int(i);
		if (NULL == index)
		{
			printf("Initialize tick thread(%d) failed: Not enough memory!\n", tickThreadCount);
			break;
		}

#if defined(_WIN32) || defined(_WIN64)
		cycleHandle[i] = CreateThread(NULL, 0, TickThreadProc, (PVOID)index, 0, &cycleThreadId[i]);
		if (NULL == cycleHandle[i])
		{
			printf("Create tick thread(%d) failed!\n", tickThreadCount);
			delete index;
			break;
		}
#elif defined(__LINUX__)
		int ptcRet = pthread_create(&cycleThreadId[i], NULL, TickThreadProc, (void*)index);
		if(ptcRet != 0)
		{
			printf("Create tick thread(%d) failed!\n", tickThreadCount);
			delete index;
			break;
		}
#endif

		++tickThreadCount;
	}

#if defined(_WIN32) || defined(_WIN64)
	infoHandle = CreateThread(NULL, 0, InfoThreadProc, (PVOID)0, 0, &infoThreadId);
	if (NULL == infoHandle)
		printf("Create info thread(%d) failed!\n", tickThreadCount);
#elif defined(__LINUX__)
	infoThreadCreateRet = pthread_create(&infoThreadId, NULL, InfoThreadProc, (void*)0);
	if (infoThreadCreateRet != 0)
		printf("Create info thread(%d) failed!\n", tickThreadCount);
#endif

	printf(
		"\n======================================================================\n"
		"<any char> = manual shedule\n"
		"'t' = add thread\n"
		"'m' = manual tick\n"
		"'o' = auto schedule on\n"
		"'f' = auto schedule off\n"
		"'i' = auto info on\n"
		"'c' = auto info off\n"
		"'s' = statistic\n"
		"'q' = fast quit\n"
		"'e' = normal quit\n"
		"======================================================================\n\n"
	);

	while (aliveTick)
	{
		char c = getchar();
		switch (c)
		{
		case '\n':
			break;
		case 't':
			{
				int count = RangeRand(ADD_WORK_THREAD_COUNT_MIN, ADD_WORK_THREAD_COUNT_MAX);
				for(int i = 0; i < count; ++i)
					apm->AddThread();
			}
			break;
		case 'm':
			apm->Tick();
			break;
		case 'o':
			autoSchedule = true;
			break;
		case 'f':
			autoSchedule = false;
			break;
		case 'i':
			autoInfo = true;
			break;
		case 'c':
			autoInfo = false;
			break;
		case 's':
			ShowStatistics();
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
		default:
		{
			char name[16] = { 0 };
			sprintf(name, "manual-%c", c);
			Schedule(name, 0);
		}
		break;
		}
	}

	for (int i = 0; i < tickThreadCount; ++i)
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
	if (infoHandle != NULL)
	{
		WaitForSingleObject(infoHandle, INFINITE);
		CloseHandle(infoHandle);
	}
#elif defined(__LINUX__)
	if(0 == infoThreadCreateRet)
		pthread_join(infoThreadId, NULL);
#endif	

	delete apm;

#if defined(_WIN32) || defined(_WIN64)		
	_CrtDumpMemoryLeaks();
#endif	

	return 0;
}