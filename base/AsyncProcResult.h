#ifndef AsyncProcResult_H_Xiqiang_20201012
#define AsyncProcResult_H_Xiqiang_20201012

#include <string>

struct AsyncProcResult
{
	enum Type
	{
		FINISH,
		EXCEPTION,
	};

	AsyncProcResult()
		: proc(NULL)
		, costSeconds(0.0f)
		, type(FINISH)
		, thread_id(0)
	{}

	AsyncProc* proc;
	float costSeconds;
	Type type;
	std::string what;
	AP_Thread thread_id;

};

#endif
