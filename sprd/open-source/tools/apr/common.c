
#include "common.h"
#include <linux/android_alarm.h>

int64_t getdate(char* strbuf, size_t max, const char* format)
{
	struct tm tm;
	time_t t;

	tzset();

	time(&t);
	localtime_r(&t, &tm);
	strftime(strbuf, max, format, &tm);

	return t;
}

int64_t elapsedRealtime(char* strbuf)
{
	struct timespec ts;
	int fd, result;

	fd = open("/dev/alarm", O_RDONLY);
	if (fd < 0)
		return fd;

	result = ioctl(fd, ANDROID_ALARM_GET_TIME(ANDROID_ALARM_ELAPSED_REALTIME), &ts);
	close(fd);

	if (result == 0) {
		if (strbuf) {
			sprintf(strbuf, "%d", ts.tv_sec);
		}
		return ts.tv_sec;
	}

	return -1;
}

