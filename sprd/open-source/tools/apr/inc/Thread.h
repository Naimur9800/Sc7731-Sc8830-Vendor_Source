


#ifndef THREAD_H
#define	THREAD_H

#include <pthread.h>

class Thread
{
public:
	Thread();
	virtual ~Thread();
	int Start(void* arg);

protected:
	int Run(void* arg);
	static void * EntryPoint(void*);
	virtual void Setup();
	virtual void Execute(void*);
	void * Arg() const { return Arg_; }
	void Arg(void* a) { Arg_ = a; }

private:
	pthread_t ThreadId_;
	void* Arg_;
};

#endif /* THREAD_H */
