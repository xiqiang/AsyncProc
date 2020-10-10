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
	virtual void OnProcDone(const AsyncProcResult& result);

public:
	void GetStatisticInfos(StatisticProcInfoMap& outInfoMap);

private:
	StatisticProcInfo* ObtainInfo(const std::string& name);

private:
	StatisticProcInfoMap m_infoMap;
	Mutex m_infoMapMutex;
};

#endif
