
#include "common.h"
#include "AnrThread.h"
#include "AprData.h"

void AnrThread::Setup()
{
	struct stat fstat;
	APR_LOGD("anrThread::Setup()\n");
	strcpy(m_tracesFile, "/data/anr/traces.txt");

	if (access(m_tracesFile, F_OK) < 0) {
		m_isFileExist = 0;
	} else {
		m_isFileExist = 1;
	}
}

void AnrThread::Execute(void* arg)
{
	APR_LOGD("anrThread::Execute()\n");
	FILE* fd = NULL;
	char strbuf[128];
	struct stat fstat;
	unsigned long st_ino;
	AprData *aprData = static_cast<AprData *>(arg);

	while (1)
	{
		if (access(m_tracesFile, F_OK) < 0) {
			m_isFileExist = 0;
			sleep(30);
			continue;
		}

		if (NULL == (fd = fopen(m_tracesFile, "r"))) {
			continue;
		}
		if (m_isFileExist) {
			fseek(fd, 0, SEEK_END);
		}
		if (stat(m_tracesFile, &fstat) < 0) {
			fclose(fd);
			continue;
		}
		st_ino = fstat.st_ino;
		while (1) {
			if (access(m_tracesFile, F_OK) < 0) break;
			if (stat(m_tracesFile, &fstat) < 0) continue;

			if (st_ino != fstat.st_ino) {
				st_ino = 0;
				break;
			}

			bzero(strbuf, sizeof(strbuf));
			if(NULL == fgets(strbuf, sizeof(strbuf), fd)) {
				if (ferror(fd)) {
					sleep(30);
					clearerr(fd);
					continue;
				}
				if (feof(fd)) {
					clearerr(fd);
					sleep(30);
					continue;
				}
			}

			if (strstr(strbuf, "----- end ")) {
				aprData->setChanged();
				aprData->notifyObservers((void*)"anr");
				continue;
			}
		}
		fclose(fd);
	}
}

