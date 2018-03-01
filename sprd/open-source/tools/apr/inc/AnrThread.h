


#ifndef ANRTHREAD_H
#define ANRTHREAD_H
#include <string>
#include "Thread.h"
using namespace std;

class AnrThread : public Thread
{
protected:
	virtual void Setup();
	virtual void Execute(void* arg);

private:
	char m_tracesFile[48];
	int m_isFileExist;
};

#endif

