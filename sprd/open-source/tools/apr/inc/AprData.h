



#ifndef APRDATA_H
#define APRDATA_H

#include "Observable.h"
#include "Thread.h"
#include <string>

class AprData:public Observable
{
public:
	AprData();
	~AprData();
	virtual void getSubjectInfo();

protected:
	void init();

public:
	// <hardwareVersion> </hardwareVersion>
	string getHardwareVersion();
	// <SN> </SN>
	string getSN();
	// <buildNumber> </buildNumber>
	string getBuildNumber();
	// <CPVersion> </CPVersion>
	string getCPVersion();
	// <xxxTime> </xxxTime>
	int getTime(char* strbuf);
	// get ro.boot.mode
	string getBootMode();

private:
	string m_hardwareVersion;
	string m_SN;
	string m_buildNumber;
	string m_CPVersion;
	string m_bootMode;
	int64_t m_sTime;
	// <exceptions>
	// <entry>
//	string m_timestamp;
//	string m_type;
	// </entry>
	// </exceptions>
};

#endif
