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
	{
	}

	const std::string& GetName() const {
		return m_name;
	}

private:
	std::string m_name;
};

#endif
