


#ifndef INOTIFYTHREAD_H
#define INOTIFYTHREAD_H
#include <string>
#include <sys/inotify.h>
#include <errno.h>

#include "Thread.h"
#include "AprData.h"
using namespace std;

struct wd_context {
	int wd;
	uint32_t mask;
	char *pathname;
};
#define WD_NUM 3

class InotifyThread : public Thread
{
public:
	InotifyThread(AprData* aprData);
	~InotifyThread();

protected:
	virtual void Setup();
	virtual void Execute(void* arg);
private:
	int _inotify_init();
	int anr_inotify_handle_event0(struct inotify_event* event);
	int anr_inotify_handle_event1(struct inotify_event* event);
	int native_inotify_handle_event2(struct inotify_event* event);
	int _inotify_dele();
private:
	AprData* m_aprData;
	/* inotify */
	int m_ifd;
	struct wd_context m_wds[WD_NUM];
};

#endif

