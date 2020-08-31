
AsyncProcThread::AsyncProcThread()
{

}

AsyncProcThread::~AsyncProcThread()
{
	for(std::vector<AsyncProcThread*>::iterator it = m_threads.bengin();
		it != m_threadList.end(); ++it)
	{
		delete(*it);
	}
}

void AsyncProcThread::Startup(int threadCount /*= 1*/)
{
	assert(threadCount > 0);
	assert(m_threads.size() == 0);
	for(int t = 0; t < threadCount; ++t)
	{
		AsyncProcThread* thread = new AsyncProcThread();
		if(!thread)
			break;

		thread->Startup();
		m_threads.push_back(thread);
	}
}

int AsyncProcThread::Enqueue(AsyncProc* proc, int threadIndex /*= -1*/)
{
	if(m_threads.size() == 0)
		return -1;

	assert(proc);
	assert(threadIndex < m_threads.size());
	if(threadIndex < 0)
	{
		int minCount = m_threads[0]->Count();
		threadIndex = 0;
		for(int t = 1; t < m_threads.size(); ++t)
		{
			int count = m_threads[t]->Count();
			if(count < minCount)
			{
				minCount = count;
				threadIndex = t;
			}
		}
	}

	m_threads[threadIndex]->Enqueue(proc);
	return threadIndex;
}

void AsyncProcThread::Shutdown(void)
{
	assert(m_threads.size() > 0);
	for(std::vector<AsyncProcThread*>::iterator it = m_threads.bengin();
		it != m_threadList.end(); ++it)
	{
		(*it)->Shutdown();
		delete(*it);
	}
	m_threads.clear();
}

void AsyncProcThread::Terminate(void)
{
	assert(m_threads.size() > 0);
	for(std::vector<AsyncProcThread*>::iterator it = m_threads.bengin();
		it != m_threadList.end(); ++it)
	{
		(*it)->Terminate();
		delete(*it);
	}
	m_threads.clear();
}

void AsyncProcThread::CallbackTick()
{
	for(std::vector<AsyncProcThread*>::iterator it = m_threads.bengin();
		it != m_threadList.end(); ++it)
	{
		(*it)->CallbackTick();
	}	
}

size_t AsyncProcThread::Count(void)
{
	size_t count = 0;
	for(std::vector<AsyncProcThread*>::iterator it = m_threads.bengin();
		it != m_threadList.end(); ++it)
	{
		count += (*it)->Count();
	}
	return count;
}
