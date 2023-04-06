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
	virtual void OnProcOverflowed(AsyncProc* proc);
	virtual void OnProcDone(const AsyncProcResult& result);

	virtual void OnThreadSleep(AP_Thread thread_id);							// NOTICE: under m_waitQueueMutex be locked outside
	virtual void OnThreadPickWork(AP_Thread thread_id, AsyncProc* proc);

public:
	void GetStatisticInfos(StatisticProcInfoMap& outInfoMap);
	void GetWorkingNameMap(WorkingNameMap& outInfoMap);

	size_t GetTotalScheduled() const { return m_totalScheduled; }
	size_t GetTotalOverflowed() const { return m_totalOverflowed; }

	size_t GetTotalSucceeded() const { return m_totalSuccess; }
	size_t GetTotalExecuteException() const { return m_totalExecuteException; }
	size_t GetTotalCallbackError() const { return m_totalCallbackError; }

	size_t GetTotalQueuedCount() {
		return m_totalScheduled - m_totalOverflowed;
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
	size_t					m_totalOverflowed;

	size_t					m_totalSuccess;
	size_t					m_totalExecuteException;
	size_t					m_totalCallbackError;
};

#endif
