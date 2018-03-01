
#include "common.h"
#include "Thread.h"

using namespace std;

Thread::Thread() {}
Thread::~Thread() {}

int Thread::Start(void* arg)
{
	Arg(arg); // store user data
	int code = pthread_create(&ThreadId_, NULL, Thread::EntryPoint, this);
	return code;
}

int Thread::Run(void* arg)
{
	Setup();
	Execute(arg);

	return 0;
}

/* static */
void* Thread::EntryPoint(void* pthis)
{
	Thread* pt = static_cast<Thread*>(pthis);
	pt->Run(pt->Arg());

	return NULL;
}

void Thread::Setup()
{
	APR_LOGD("Thread::Setup()\n");
	// Do any setup here
}

void Thread::Execute(void* arg)
{
	APR_LOGD("Thread::Execute()\n");
	// Your code goes here
}

