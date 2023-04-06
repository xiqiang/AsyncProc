#ifndef AsyncProcResult_H_Xiqiang_20201012
#define AsyncProcResult_H_Xiqiang_20201012

#include <string>

struct AsyncProcResult
{
	enum Type
	{
		SUCCESS,
		EXECUTE_EXCEPTION,
		CALLBACK_ERROR,
	};

	AsyncProcResult()
		: proc(NULL)
		, type(SUCCESS)
		, thread_id(0)
	{}

	AsyncProc*		proc;
	Type			type;
	AP_Thread		thread_id;
	std::string		what;

};

#endif
