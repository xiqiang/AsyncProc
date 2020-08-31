#include "AsyncProc.h"

class SleepProc
    : public AsyncProc
{
public:
    SleepProc(unsigned int ms) 
		: m_ms(ms) 
    {
		printf("SleepProc(time=%d)\n", m_ms);
	}
    
	virtual ~SleepProc(void)
	{
		AsyncProc::~AsyncProc();
		printf("~SleepProc()\n");
    }

	virtual void Execute(void) 
    {
		printf("SleepProc(i=%d,time=%d,tid=%d).Execute...\n", m_procIndex, m_ms, m_threadIndex);
#if defined(_WIN32) || defined(_WIN64)
        Sleep(m_ms);
#elif defined(__LINUX__)
        usleep(m_ms * 1000);
#endif        
		printf("SleepProc(i=%d,time=%d,tid=%d).Execute...OK\n", m_procIndex, m_ms, m_threadIndex);
    }

	void SetThreadIndex(int threadIndex) 
    {
		m_threadIndex = threadIndex;
	}

	void SetProcIndex(int procIndex) 
    {
		m_procIndex = procIndex;
	}

private:
    unsigned int m_ms;
	int m_threadIndex;
	int m_procIndex;
};
