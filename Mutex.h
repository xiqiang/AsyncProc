#ifndef Mutex_H_Xiqiang_20201007
#define Mutex_H_Xiqiang_20201007

#if defined(_WIN32) || defined(_WIN64)
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#elif defined(__LINUX__)
#include <pthread.h>
#endif

class Mutex
{
#if defined(_WIN32) || defined(_WIN64)
public:
	Mutex() {
		InitializeCriticalSection(&m_lock);
	}

	~Mutex() {
		DeleteCriticalSection(&m_lock);
	}

	void Lock() {
		EnterCriticalSection(&m_lock);
	}

	void Unlock() {
		LeaveCriticalSection(&m_lock);
	}

	CRITICAL_SECTION& GetInst(void) {
		return m_lock;
	}

private:
	CRITICAL_SECTION m_lock;

#elif defined(__LINUX__)

public:
	Mutex() {
		pthread_mutex_init(&m_lock, NULL);
	}

	~Mutex() {
		pthread_mutex_destroy(&m_lock);
	}

	void Lock() {
		pthread_mutex_lock(&m_lock);
	}

	void Unlock() {
		pthread_mutex_unlock(&m_lock);
	}

	pthread_mutex_t& GetInst(void) {
		return m_lock;
	}

private:
	pthread_mutex_t m_lock;

#endif
};

#endif
