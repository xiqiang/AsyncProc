#ifndef StatisticProcManager_H_Xiqiang_20201009
#define StatisticProcManager_H_Xiqiang_20201009

#include <map>
#include "StatisticProcInfo.h"
#include "../base/AsyncProcManager.h"
#include "../base/AutoMutex.h"

class StatisticProcManager
	: public AsyncProcManager
{
public:
	virtual ~StatisticProcManager() {}

public:
	virtual void OnProcScheduled(AsyncProc* proc);
	virtual void OnProcOverflowed(AsyncProc* proc);
	virtual void OnProcDone(const AsyncProcResult& result);
	virtual void OnThreadPickWork(AP_Thread thread_id, AsyncProc* proc);
	virtual void OnThreadSleep(AP_Thread thread_id);

public:
	void GetStatisticInfos(StatisticProcInfoMap& outInfoMap);
	void GetWorkingNameMap(WorkingNameMap& outInfoMap);

private:
	StatisticProcInfo* ObtainInfo(const std::string& name);

private:
	StatisticProcInfoMap m_infoMap;
	WorkingNameMap m_workingProcMap;
	Mutex m_infoMapMutex;
};

#endif
