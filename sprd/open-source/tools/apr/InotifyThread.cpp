
#include "common.h"
#include "InotifyThread.h"
#include "AprData.h"

#define EVENT_NUM 16
#define MAX_BUF_SIZE 1024

static char* events[] =
{
	(char*)"File was accessed",
	(char*)"File was modified",
	(char*)"File attributes were changed",
	(char*)"writeable file closed",
	(char*)"Unwriteable file closed",
	(char*)"File was opened",
	(char*)"File was moved from X",
	(char*)"File was moved to Y",
	(char*)"Subfile was created",
	(char*)"Subfile was deleted",
	(char*)"Self was deleted",
	(char*)"Self was moved",
	(char*)"",
	(char*)"Backing fs was unmounted",
	(char*)"Event queued overflowed",
	(char*)"File was ignored"
};

InotifyThread::InotifyThread(AprData* aprData)
{
	m_aprData = aprData;
}

InotifyThread::~InotifyThread()
{
	m_aprData = NULL;
}

void InotifyThread::Setup()
{
	m_wds[0].wd = -1;
	m_wds[0].pathname = (char*)"/data";
	m_wds[0].mask = IN_CREATE | IN_DELETE | IN_MOVED_FROM;

	m_wds[1].wd = -1;
	m_wds[1].pathname = (char*)"/data/anr";
	m_wds[1].mask = IN_CREATE | IN_DELETE | IN_MOVED_FROM;

	m_wds[2].wd = -1;
	m_wds[2].pathname = (char*)"/data/tombstones";
	m_wds[2].mask = IN_CLOSE_WRITE;
}

void InotifyThread::Execute(void* arg)
{
	APR_LOGD("anrThread::Execute()\n");
	char buffer[MAX_BUF_SIZE];
	struct inotify_event* event;
	int len;
	int index;

	_inotify_init();

	while (1)
	{
		memset(buffer, 0, sizeof(buffer));
		len = read(m_ifd, buffer, MAX_BUF_SIZE);
		if (len < 0) {
			APR_LOGE("read()\n");
			continue;
		}
		APR_LOGD("Some event happens, len = %d.\n", len);
		index = 0;
		while (index < len)
		{
			event = (struct inotify_event *)(buffer+index);

			do {
				/* /data/anr */
				if (event->wd == m_wds[1].wd)
				{
					anr_inotify_handle_event1(event);
					break;
				}
				/* /data/tombstones */
				if (event->wd == m_wds[2].wd)
				{
					native_inotify_handle_event2(event);
					break;
				}
				/* /data */
				if (event->wd == m_wds[0].wd)
				{
					anr_inotify_handle_event0(event);
					break;
				}
			} while (0);

			index += sizeof(struct inotify_event) + event->len;
		}
	}

	_inotify_dele();
}

int InotifyThread::_inotify_init()
{
	int i;
	int wd;
	m_ifd = inotify_init();
	if (m_ifd < 0) {
		APR_LOGD("anr_inotify_init(): Fail to initialize inotify.\n");
		exit(-1);
	}

	for (i=0; i<WD_NUM; i++) {
		wd = inotify_add_watch(m_ifd, m_wds[i].pathname, m_wds[i].mask);
		if (wd < 0) {
			APR_LOGE("inotify_add_watch(): Can't add watch for %s.\n", m_wds[i].pathname);
			if (errno == ENOENT)
				break;
			exit(-1);
		}
		m_wds[i].wd = wd;
	}

	return 0;
}

int InotifyThread::anr_inotify_handle_event0(struct inotify_event* event)
{
	int wd;
	if ((event->mask & IN_ISDIR)
			&& !strcmp(event->name, "anr"))
	{
		if (event->mask & IN_CREATE) {
			APR_LOGD("inotify_add_watch(fd, %s)\n", m_wds[1].pathname);
			wd = inotify_add_watch(m_ifd, m_wds[1].pathname, m_wds[1].mask);
			if (wd < 0) {
				APR_LOGE("Can't add watch for %s.\n", m_wds[1].pathname);
				exit(-1);
			}
			m_wds[1].wd = wd;
		} else if (event->mask & IN_MOVED_FROM) {
			APR_LOGD("inotify_rm_watch, %s\n", m_wds[1].pathname);
			wd = inotify_rm_watch(m_ifd, m_wds[1].wd);
			if (wd < 0) {
				APR_LOGE("Can't del watch for %s.\n", m_wds[1].pathname);
				exit(-1);
			}
			m_wds[1].wd = -1;
		} else if (event->mask & IN_DELETE) {
			APR_LOGD("inotify_rm_watch, %s\n", m_wds[1].pathname);
			wd = inotify_rm_watch(m_ifd, m_wds[1].wd);
			if (wd < 0) {
				APR_LOGE("Can't rm watch for %s.\n", m_wds[1].pathname);
				if (errno != EINVAL)
					exit(-1);
			}
			m_wds[1].wd = -1;
		}
	}

	return 0;
}

int InotifyThread::anr_inotify_handle_event1(struct inotify_event* event)
{
	int wd;
	if (!(event->mask & IN_ISDIR)
			&& !strcmp(event->name, "traces.txt"))
	{
		if (event->mask & IN_CREATE) {
			// anr
			m_aprData->setChanged();
			m_aprData->notifyObservers((void*)"anr");
		}
	}

	return 0;
}

int InotifyThread::native_inotify_handle_event2(struct inotify_event* event)
{
	int wd;
	if (!(event->mask & IN_ISDIR)
			&& strstr(event->name, "tombstone_0"))
	{
		if (event->mask & IN_CLOSE_WRITE) {
			// anr
			m_aprData->setChanged();
			m_aprData->notifyObservers((void*)"native crash");
		}
	}

	return 0;
}

int InotifyThread::_inotify_dele()
{
	int wd, i;
	for (i=0; i<WD_NUM; i++) {
		if (m_wds[i].wd != -1)
		{
			wd = inotify_rm_watch(m_ifd, m_wds[i].wd);
			if (wd < 0) {
				APR_LOGE("inotify_add_watch(): Can't add watch for %s.\n", m_wds[i].pathname);
				if (errno != EINVAL)
					exit(-1);
			}
			m_wds[i].wd = -1;
		}
	}

	return 0;
}
