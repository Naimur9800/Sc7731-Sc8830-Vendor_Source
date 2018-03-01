



#include "common.h"
#include "AprData.h"

AprData::AprData()
{
	init();
}

AprData::~AprData()
{

}

void AprData::getSubjectInfo()
{
	APR_LOGD("AprData::getSubjectInfo()\n");
	return;
}

void AprData::init()
{
	APR_LOGD("AprData::init()\n");
	char value[PROPERTY_VALUE_MAX];
	char *default_value = (char*)"unknown";
	int iter;

	//     <hardwareVersion> </hardwareVersion>
	property_get("ro.product.hardware", value, default_value);
	m_hardwareVersion.assign(value);
	//     <SN> </SN>
	property_get("ro.boot.serialno", value, default_value);
	m_SN.assign(value);
	//     <buildNumber> </buildNumber>
	property_get("ro.build.description", value, default_value);
	m_buildNumber.assign(value);
	//     <CPVersion> </CPVersion>
	for(iter=1; iter <= 36; iter++)
	{
		property_get("gsm.version.baseband", value, default_value);
		if (strcmp(value, default_value))
			break;
		sleep(5);
	}
	m_CPVersion.assign(value);
	// get ro.boot.mode
	property_get("ro.boot.mode", value, default_value);
	m_bootMode.assign(value);

	m_sTime = getdate(value, sizeof(value), "%s");
}

string AprData::getHardwareVersion()
{
	return m_hardwareVersion;
}

string AprData::getSN()
{
	return m_SN;
}

string AprData::getBuildNumber()
{
	return m_buildNumber;
}

string AprData::getCPVersion()
{
	return m_CPVersion;
}

int AprData::getTime(char* strbuf)
{
	int64_t eTime;
	eTime = m_sTime + elapsedRealtime(NULL);
	sprintf(strbuf, "%lld", eTime);
	return 0;
}

string AprData::getBootMode()
{
	return m_bootMode;
}
