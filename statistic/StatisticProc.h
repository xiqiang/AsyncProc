#ifndef StatisticProc_H_Xiqiang_20201009
#define StatisticProc_H_Xiqiang_20201009

#include <string>
#include "../base/AsyncProc.h"

class StatisticProc
	: public AsyncProc
{
public:
	StatisticProc(const std::string& name)
		: m_name(name)
		, m_scheduleClock(0)
		, m_executeClock(0) {
	}

	const std::string& GetName() const {
		return m_name;
	}

private:
	std::string		m_name;
	clock_t			m_scheduleClock;
	clock_t			m_executeClock;
};

#endif
