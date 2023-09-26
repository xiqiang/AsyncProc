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
	StatisticProcManager();
	virtual ~StatisticProcManager() {}

public:
	virtual void OnProcScheduled(AsyncProc* proc);
	virtual void OnProcBlocked(AsyncProc* proc);
	virtual void OnProcDropped(AsyncProc* proc, bool replace);
	virtual void OnProcDone(const AsyncProcResult& result);

	virtual void OnThreadSleep(AP_Thread thread_id);							// NOTICE: under m_waitQueueMutex be locked outside
	virtual void OnThreadPickWork(AP_Thread thread_id, AsyncProc* proc);

public:
	void GetStatisticInfos(StatisticProcInfoMap& outInfoMap);
	void GetWorkingNameMap(WorkingNameMap& outInfoMap);

	size_t GetTotalScheduled() const { return m_totalScheduled; }
	size_t GetTotalBlocked() const { return m_totalBlocked; }
	size_t GetTotalDropped() const { return m_totalDropped; }

	size_t GetTotalSucceeded() const { return m_totalSuccess; }
	size_t GetTotalExecuteException() const { return m_totalExecuteException; }
	size_t GetTotalCallbackError() const { return m_totalCallbackError; }

	size_t GetTotalQueuedCount() {
		return m_totalScheduled - m_totalBlocked - m_totalDropped;
	}

	size_t GetTotalFinishCount() {
		return m_totalSuccess + m_totalExecuteException + m_totalCallbackError;
	}

private:
	StatisticProcInfo* ObtainInfo(const std::string& name);

private:
	StatisticProcInfoMap	m_infoMap;
	Mutex					m_infoMapMutex;

	WorkingNameMap			m_workingProcMap;
	Mutex					m_workingProcMapMutex;

	size_t					m_totalScheduled;
	size_t					m_totalBlocked;
	size_t					m_totalDropped;

	size_t					m_totalSuccess;
	size_t					m_totalExecuteException;
	size_t					m_totalCallbackError;
};

#endif
