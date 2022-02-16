#include "StatisticProcManager.h"
#include "StatisticProc.h"

void StatisticProcManager::OnProcScheduled(AsyncProc* proc)
{
	AutoMutex am(m_infoMapMutex);

	StatisticProc* sProc = dynamic_cast<StatisticProc*>(proc);
	assert(sProc);

	StatisticProcInfo* spi = ObtainInfo(sProc->GetName());
	if (!spi)
		return;

	++spi->countScheduled;
}

void StatisticProcManager::OnProcOverflowed(AsyncProc* proc)
{
	AutoMutex am(m_infoMapMutex);

	StatisticProc* sProc = dynamic_cast<StatisticProc*>(proc);
	assert(sProc);

	StatisticProcInfo* spi = ObtainInfo(sProc->GetName());
	if (!spi)
		return;

	++spi->countOverflowed;
}

void StatisticProcManager::OnProcDone(const AsyncProcResult& result)
{
	AutoMutex am(m_infoMapMutex);

	StatisticProc* sProc = dynamic_cast<StatisticProc*>(result.proc);
	assert(sProc);

	StatisticProcInfo* spi = ObtainInfo(sProc->GetName());
	if (!spi)
		return;

	if (spi->costSecondsMax < result.costSeconds)
		spi->costSecondsMax = result.costSeconds;

	if (spi->countDone() > 0)
	{
		if (spi->costSecondsMin > result.costSeconds)
			spi->costSecondsMin = result.costSeconds;
	}
	else
	{
		spi->costSecondsMin = result.costSeconds;
	}

	switch (result.type)
	{
	case AsyncProcResult::FINISH:		++spi->countFinish;		break;
	case AsyncProcResult::EXCEPTION:	++spi->countException;	break;
	}

	spi->costSecondsTotal += result.costSeconds;
}

void StatisticProcManager::OnThreadPickWork(AP_Thread thread_id, AsyncProc* proc)
{
	StatisticProc* sProc = dynamic_cast<StatisticProc*>(proc);
	assert(sProc);
	m_workingProcMap[thread_id] = sProc->GetName();
}

void StatisticProcManager::OnThreadSleep(AP_Thread thread_id)
{
	m_workingProcMap.erase(thread_id);
}

void StatisticProcManager::GetStatisticInfos(StatisticProcInfoMap& outInfoMap)
{
	AutoMutex am(m_infoMapMutex);
	outInfoMap.clear();
	outInfoMap.insert(m_infoMap.begin(), m_infoMap.end());
}

void StatisticProcManager::GetWorkingNameMap(WorkingNameMap& outInfoMap)
{
	AutoMutex am(GetWaitDequeMutex());
	outInfoMap.clear();
	outInfoMap.insert(m_workingProcMap.begin(), m_workingProcMap.end());
}

StatisticProcInfo* StatisticProcManager::ObtainInfo(const std::string& name)
{
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
