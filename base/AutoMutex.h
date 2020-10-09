#ifndef AutoMutex_H_Xiqiang_20201007
#define AutoMutex_H_Xiqiang_20201007

#include <assert.h>
#include "Mutex.h"

class AutoMutex
{
public:
	AutoMutex(Mutex& mutex)
		: m_mutex(&mutex) {
		m_mutex->Lock();
	}

	AutoMutex(Mutex* mutex)
		: m_mutex(mutex) {
		m_mutex->Lock();
	}

	~AutoMutex() {
		m_mutex->Unlock();
	}

private:
	Mutex* m_mutex;
};

#define AutoMutex(x) static_assert(false, "missing var name")

#endif
