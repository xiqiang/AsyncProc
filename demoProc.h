#include "AsyncProc.h"

class DemoProc
    : public AsyncProc
{
public:
    DemoProc(int id, unsigned int ms) 
		: m_id(id)
		, m_ms(ms) 
    {
		printf("DemoProc(time=%d, addr=%p) :%d\n", m_ms, this, m_id);
	}
    
	virtual ~DemoProc(void)
	{
    }

	virtual void Execute(void) 
    {
#if defined(_WIN32) || defined(_WIN64)
        Sleep(m_ms);
#elif defined(__LINUX__)
        usleep(m_ms * 1000);
#endif
		if(rand()%3==1)
			throw "error";
    }

	int GetID()
	{
		return m_id;
	}

	int GetMilliSeconds()
	{
		return m_ms;
	}

private:
	int m_id;
    unsigned int m_ms;
};
