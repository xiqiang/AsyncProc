#ifndef AsyncProc_H_Xiqiang_20190907
#define AsyncProc_H_Xiqiang_20190907

class AsyncProc
{
public:
	friend class AsyncProcMgr;
	typedef void (*Callback)(AsyncProc*);

	class Caller
	{
	public:
		virtual ~Caller() = 0;
		virtual void Invoke(AsyncProc* proc) = 0;
	};

	template<class T>
	class MemberCaller 
		: public Caller
	{
		typedef void (T::*MemberFun)(AsyncProc*);

	public:
		MemberCaller(T* p, MemberFun fun)
			: m_ptr(p)
			, m_fun(fun) 
		{
		}

		virtual void Invoke(AsyncProc* proc) 
		{
			(m_ptr->*m_fun)(proc);
		}

	private:
		T*					m_ptr;
		MemberFun			m_fun;
	};

public:
	AsyncProc(void)
	    : m_callback(NULL)
    	, m_caller(NULL) 
	{
	}

	virtual ~AsyncProc(void) 
	{
		if(m_caller) 
		{
			delete m_caller;
			m_caller = NULL;
		}		
	}

	virtual void Execute(void) {}

public:
	template<class T>
	void SetCallback( T* pVar, void(T::*pMemberFun)(AsyncProc*)) 
	{
		m_caller = new MemberCaller<T>(pVar, pMemberFun);
	}

	void SetCallback(Callback fun) 
	{
		m_callback = fun;
	}	

	void InvokeCallback(void) 
	{
		if(m_callback)
			m_callback(this);

		if(m_caller)
			m_caller->Invoke(this);
	}		

private:
	Callback m_callback;
	Caller* m_caller;
};

#endif
