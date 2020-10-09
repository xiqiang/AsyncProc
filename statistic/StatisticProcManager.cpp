#include "StatisticProcManager.h"
#include "StatisticProc.h"

void StatisticProcManager::OnProcScheduled(AsyncProc* proc)
{
	StatisticProc* sProc = dynamic_cast<StatisticProc*>(proc);
	assert(sProc);

	StatisticProcInfo* spi = ObtainInfo(sProc->GetName());
	if (!spi)
		return;

	++spi->countScheduled;
}

void StatisticProcManager::OnProcDone(const AsyncProcResult& result)
{
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
