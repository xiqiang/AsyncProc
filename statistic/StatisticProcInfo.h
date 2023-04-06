#ifndef StatisticProcInfo_H_Xiqiang_20201009
#define StatisticProcInfo_H_Xiqiang_20201009

#include <string>

struct StatisticProcInfo
{
	StatisticProcInfo() 
		: countScheduled(0)
		, countOverflowed(0)
		, countSuccess(0)
		, countExecuteException(0)
		, countCallbackError(0)

		, queueSecondsMax(0.0f)
		, queueSecondsMin(0.0f)
		, queueSecondsTotal(0.0f)

		, executeSecondsMax(0.0f)
		, executeSecondsMin(0.0f)
		, executeSecondsTotal(0.0f)

		, callbackSecondsMax(0.0f)
		, callbackSecondsMin(0.0f)
		, callbackSecondsTotal(0.0f) {
	}

	inline size_t countFinish() const {
		return countSuccess + countExecuteException + countCallbackError;
	}

	inline size_t countQueue() const {
		return countScheduled - countOverflowed;
	}

	inline float queueSecondsAverage() const {
		return countFinish() > 0 ? queueSecondsTotal / countFinish() : 0.0f;
	}

	inline float executeSecondsAverage() const {
		return countFinish() > 0 ? executeSecondsTotal / countFinish() : 0.0f;
	}

	inline float callbackSecondsAverage() const {
		return countFinish() > 0 ? callbackSecondsTotal / countFinish() : 0.0f;
	}

	size_t countScheduled;
	size_t countOverflowed;

	size_t countSuccess;
	size_t countExecuteException;
	size_t countCallbackError;

	float queueSecondsMax;
	float queueSecondsMin;
	float queueSecondsTotal;

	float executeSecondsMax;
	float executeSecondsMin;
	float executeSecondsTotal;

	float callbackSecondsMax;
	float callbackSecondsMin;
	float callbackSecondsTotal;
};

typedef std::map<std::string, StatisticProcInfo> StatisticProcInfoMap;

#endif
