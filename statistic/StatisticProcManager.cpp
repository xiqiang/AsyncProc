#include "StatisticProcManager.h"
#include "StatisticProc.h"

StatisticProcManager::StatisticProcManager()
	: m_totalScheduled(0)
	, m_totalOverflowed(0)
	, m_totalSuccess(0)
	, m_totalExecuteException(0)
	, m_totalCallbackError(0)
{

}

void StatisticProcManager::OnProcScheduled(AsyncProc* proc)
{
	StatisticProc* sProc = dynamic_cast<StatisticProc*>(proc);
	assert(sProc);

	AutoMutex am(m_infoMapMutex);
	++m_totalScheduled;

	StatisticProcInfo* spi = ObtainInfo(sProc->GetName());
	if (spi)
		++spi->countScheduled;
}

void StatisticProcManager::OnProcOverflowed(AsyncProc* proc)
{
	StatisticProc* sProc = dynamic_cast<StatisticProc*>(proc);
	assert(sProc);

	AutoMutex am(m_infoMapMutex);
	++m_totalOverflowed;

	StatisticProcInfo* spi = ObtainInfo(sProc->GetName());
	if (spi)
		++spi->countOverflowed;
}

void StatisticProcManager::OnProcDone(const AsyncProcResult& result)
{
	StatisticProc* sProc = dynamic_cast<StatisticProc*>(result.proc);
	assert(sProc);

	float queueSeconds = (sProc->GetExeBeginClock() - sProc->GetScheduleClock()) / (float)CLOCKS_PER_SEC;
	float executeSeconds = (sProc->GetExeEndClock() - sProc->GetExeBeginClock()) / (float)CLOCKS_PER_SEC;
	float callbackSeconds = (clock() - sProc->GetExeEndClock()) / (float)CLOCKS_PER_SEC;

	AutoMutex am(m_infoMapMutex);
	switch (result.type)
	{
	case AsyncProcResult::SUCCESS:				++m_totalSuccess;			break;
	case AsyncProcResult::EXECUTE_EXCEPTION:	++m_totalExecuteException;	break;
	case AsyncProcResult::CALLBACK_ERROR:		++m_totalCallbackError;		break;
	}

	StatisticProcInfo* spi = ObtainInfo(sProc->GetName());
	if (!spi)
		return;

	if (spi->queueSecondsMax < queueSeconds)
		spi->queueSecondsMax = queueSeconds;
	if (spi->executeSecondsMax < executeSeconds)
		spi->executeSecondsMax = executeSeconds;
	if (spi->callbackSecondsMax < callbackSeconds)
		spi->callbackSecondsMax = callbackSeconds;

	if (spi->countFinish() > 0)
	{
		if (spi->queueSecondsMin > queueSeconds)
			spi->queueSecondsMin = queueSeconds;
		if (spi->executeSecondsMin > executeSeconds)
			spi->executeSecondsMin = executeSeconds;
		if (spi->callbackSecondsMin > callbackSeconds)
			spi->callbackSecondsMin = callbackSeconds;
	}
	else
	{
		spi->queueSecondsMin = queueSeconds;
		spi->executeSecondsMin = executeSeconds;
		spi->callbackSecondsMin = callbackSeconds;
	}

	spi->queueSecondsTotal += queueSeconds;
	spi->executeSecondsTotal += executeSeconds;
	spi->callbackSecondsTotal += callbackSeconds;

	switch (result.type)
	{
	case AsyncProcResult::SUCCESS:				++spi->countSuccess;			break;
	case AsyncProcResult::EXECUTE_EXCEPTION:	++spi->countExecuteException;	break;
	case AsyncProcResult::CALLBACK_ERROR:		++spi->countCallbackError;		break;
	}
}

void StatisticProcManager::OnThreadSleep(AP_Thread thread_id)
{
	// NOTICE: under m_waitQueueMutex be locked outside

	AutoMutex am(m_workingProcMapMutex);
	m_workingProcMap.erase(thread_id);
}

void StatisticProcManager::OnThreadPickWork(AP_Thread thread_id, AsyncProc* proc)
{
	StatisticProc* sProc = dynamic_cast<StatisticProc*>(proc);
	assert(sProc);

	AutoMutex am(m_workingProcMapMutex);
	m_workingProcMap[thread_id] = sProc->GetName();
}

void StatisticProcManager::GetStatisticInfos(StatisticProcInfoMap& outInfoMap)
{
	AutoMutex am(m_infoMapMutex);
	outInfoMap.clear();
	outInfoMap.insert(m_infoMap.begin(), m_infoMap.end());
}

void StatisticProcManager::GetWorkingNameMap(WorkingNameMap& outInfoMap)
{
	AutoMutex am(m_workingProcMapMutex);
	outInfoMap.clear();
	outInfoMap.insert(m_workingProcMap.begin(), m_workingProcMap.end());
}

StatisticProcInfo* StatisticProcManager::ObtainInfo(const std::string& name)
{
	// NOTICE: m_infoMapMutex must be locked outside!

	StatisticProcInfoMap::iterator it = m_infoMap.find(name);
	if (it != m_infoMap.end())
		return &(it->second);

	std::pair<StatisticProcInfoMap::iterator, bool> ret = m_infoMap.insert(
		std::pair<std::string, StatisticProcInfo>(name, StatisticProcInfo()));

	if (!ret.second)
		return NULL;
	else
		return &(ret.first->second);
}
