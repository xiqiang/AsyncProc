#ifndef StatisticProcInfo_H_Xiqiang_20201009
#define StatisticProcInfo_H_Xiqiang_20201009

#include <string>

struct StatisticProcInfo
{
	StatisticProcInfo() 
		: countScheduled(0)
		, countFinish(0)
		, countException(0)
		, costSecondsMax(0.0f)
		, costSecondsMin(0.0f) {
	}

	size_t countDone() const {
		return countFinish + countException;
	}

	float costSecondsAverage() const { 
		return countDone() > 0 ? (costSecondsMax - costSecondsMin) / countDone() : 0.0f;
	}

	size_t countScheduled;
	size_t countFinish;
	size_t countException;
	float costSecondsMax;
	float costSecondsMin;
};

typedef std::map<std::string, StatisticProcInfo> StatisticProcInfoMap;

#endif
