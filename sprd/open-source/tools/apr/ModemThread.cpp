
#include "common.h"
#include "ModemThread.h"
#include "AprData.h"

#define MODEM_SOCKET_NAME         "modemd"

ModemThread::ModemThread(AprData* aprData)
{
	m_aprData = aprData;
}

ModemThread::~ModemThread()
{
	m_aprData = NULL;
}

void ModemThread::Setup()
{
	APR_LOGD("ModemThread::Setup()\n");
}

void ModemThread::Execute(void* arg)
{
	APR_LOGD("ModemThread::Execute()\n");
	int cfd = -1;
	char buf[256];
	int numRead;

reconnect:
	cfd = socket_local_client(MODEM_SOCKET_NAME,
			ANDROID_SOCKET_NAMESPACE_ABSTRACT, SOCK_STREAM);
	while(cfd < 0) {
		usleep(10*1000);
		cfd = socket_local_client(MODEM_SOCKET_NAME,
				ANDROID_SOCKET_NAMESPACE_ABSTRACT, SOCK_STREAM);
	}

	if (cfd < 0) {
		APR_LOGE("%s: cannot create local socket server", __FUNCTION__);
		exit(-1);
	}

	for(;;) {
		memset(buf, 0, sizeof(buf));
		do {
			numRead = read(cfd, buf, sizeof(buf));
		} while(numRead < 0 && errno == EINTR);

		if (numRead <= 0) {
			close(cfd);
			goto reconnect;
		}
		buf[255] = '\0';

		APR_LOGD("receive \"%s\"\n", buf);
		if (strstr(buf, "Modem Blocked") != NULL) {
			m_aprData->setChanged();
			m_aprData->notifyObservers((char*)"modem blocked");
		} else if (strstr(buf, "Assert") != NULL) {
			m_aprData->setChanged();
			m_aprData->notifyObservers((char*)"modem assert");
		} else if (strstr(buf, "Modem Alive") != NULL) {
		} else {
			continue;
		}
	}

	close(cfd);
}
