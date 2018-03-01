
#include "common.h"
#include "AprData.h"
#include "XmlStorage.h"
#include "InotifyThread.h"
#include "ModemThread.h"
#include "LoggerThread.h"

int main(int argc, char *argv[])
{
	AprData aprData;
	ModemThread modemThread(&aprData);
	InotifyThread inotifyThread(&aprData);
//	LoggerThread loggerThread(&aprData);

	XmlStorage xmlStorage(&aprData, (char*)"/data/sprdinfo", (char*)"apr.xml");
	aprData.addObserver(&xmlStorage);

	modemThread.Start(NULL);
	inotifyThread.Start(NULL);
//	loggerThread.Start(NULL);
	while(1)
	{
		// waiting 60 seconds.
		sleep(60);
		/* update the <endTime></endTime> for all Observers */
		aprData.setChanged();
		aprData.notifyObservers(NULL);
	}

	aprData.deleteObserver(&xmlStorage);
	return 0;
}

