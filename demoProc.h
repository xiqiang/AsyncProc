#include "AsyncProc.h"


class SleepProc
    : public AsyncProc
{
public:
    SleepProc(unsigned int seconds) : m_seconds(seconds) {}
    
	virtual void Execute(void) 
    {
        printf("SleepProc(%d) begin\n", m_seconds);
#if defined(__WINDOWS__)
        Sleep(m_seconds * 1000);
#elif defined(__LINUX__)
        sleep(m_seconds);
#endif        
    printf("SleepProc(%d) end\n", m_seconds);
    }

private:
    unsigned int m_seconds;
};
