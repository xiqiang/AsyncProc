#ifndef Condition_H_Xiqiang_20201007
#define Condition_H_Xiqiang_20201007

#include "Mutex.h"

class Condition
{
#if defined(_WIN32) || defined(_WIN64)
public:
	Condition(Mutex* mutex)
		: m_mutex(mutex) {
		InitializeConditionVariable(&m_cond);
	}

	~Condition() {
	}

	void Sleep() {
		SleepConditionVariableCS(&m_cond, &m_mutex->GetInst(), INFINITE);	
	}

	void Wake() {
		WakeConditionVariable(&m_cond);
	}

	void WakeAll() {
		WakeAllConditionVariable(&m_cond);
	}

private:
	CONDITION_VARIABLE m_cond;

#elif defined(__LINUX__)

public:
	Condition(Mutex* mutex)
		: m_mutex(mutex) {
		pthread_cond_init(&m_cond, NULL);
	}

	~Condition() {
		pthread_cond_destroy(&m_cond);
	}

	void Sleep() {
		pthread_cond_wait(&m_condNewProc, &m_mutex->GetInst());
	}

	void Wake() {
		pthread_cond_signal(&m_cond);
	}

	void WakeAll() {
		pthread_cond_broadcast(&m_cond);
	}

private:
	pthread_cond_t m_cond;

#endif

private:
	Mutex* m_mutex;
};

#endif
