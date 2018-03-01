#include "testitem.h"

#include <dlfcn.h>
#include <pthread.h>
#include <sys/time.h>
#include <time.h>

#include <hardware/gps.h>
#include <hardware_legacy/power.h>



static void * processThread_show( void * param );
static void   location_callback(GpsLocation* loc);
static void   status_callback(GpsStatus* status);
static void   sv_status_callback(GpsSvStatus* sv_status);
static void   nmea_callback(GpsUtcTime timestamp, const char* nmea, int length);
static void   set_capabilities_callback(uint32_t capabilities);
static void   acquire_wakelock_callback();
static void   release_wakelock_callback();
static void   request_utc_time_callback();
static pthread_t create_thread_callback(const char* name, void (*start)(void *), void* arg);

//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
#define GPS_ACT_UNKOWN	     0x0000
#define GPS_ACT_INIT_ING     0x0001
#define GPS_ACT_INIT_FAIL    0x0002
#define GPS_ACT_INIT_DONE    0x0004
#define	GPS_ACT_START        0x0008
#define	GPS_ACT_STOP         0x0010

//------------------------------------------------------------------------------
static const  GpsInterface  * sGpsIntrfc = NULL;
static int sSVNum = 0;
static int sSvSnr[10] = {0};
static int sPreTest = 0;
static int sTimeout = 0;
static unsigned int nCount=0;
static time_t gOpenTime;
static int thread_run;
static unsigned int gps_is_enable = 0;
//------------------------------------------------------------------------------

static GpsCallbacks sGpsCallbacks = {
	sizeof(GpsCallbacks),
	location_callback,
	status_callback,
	sv_status_callback,
	nmea_callback,
	set_capabilities_callback,
	acquire_wakelock_callback,
	release_wakelock_callback,
	create_thread_callback,
	request_utc_time_callback,
};

int gpsOpen( void )
{
	struct gps_device_t* device;
	struct hw_module_t* module;
	int err,ret;

	sPreTest = 0;
	FUN_ENTER;
	if(gps_is_enable == 1)
	{
		SPRD_DBG("%s Gps is already enabled \n",__FUNCTION__);
		return 0;
	}
	remove("data/gps_outdirfile.txt");
	system("rm /data/gps/log/*");


	err = hw_get_module(GPS_HARDWARE_MODULE_ID, (hw_module_t const**)&module);
	if (err == 0) {
		SPRD_DBG("mmitest %s call hw_get_module() success \n",__FUNCTION__);

		err = module->methods->open(module, GPS_HARDWARE_MODULE_ID,(hw_device_t**)&device);
		if (err == 0) {
			SPRD_DBG("mmitest %scall open() GPS MODULE success \n",__FUNCTION__);
			sGpsIntrfc = device->get_gps_interface(device);
		}
	}

	if (sGpsIntrfc)
		SPRD_DBG("mmitest %scall get_gps_interface() success \n",__FUNCTION__);
	else
	{
		SPRD_DBG("mmitest %scall get_gps_interface() failed \n",__FUNCTION__);
		return -1;
	}

//	sGpsIntrfc->cleanup();

	if(sGpsIntrfc->init(&sGpsCallbacks) != 0)
	{
		SPRD_DBG("mmitest sGpsIntrfc->init(&sGpsCallbacks) failed \n");
		return -1;
	}
	ret = sGpsIntrfc->start();
	if (ret != 0)
	{
		SPRD_DBG("mmitest sGpsIntrfc->start() failed ret = %d \n", ret);
	}
	else
		SPRD_DBG("mmitest sGpsIntrfc->start()  %s \n", "success");

	gps_is_enable = 1;
//	gOpenTime = time(NULL);
	SPRD_DBG("mmitest gOpenTime=%ld \n", gOpenTime);
	sSVNum = 0;
	FUN_EXIT;
	return 0;
}

int gpsStop( void )
{
	int ret = 0;
	FUN_ENTER;


	ret = sGpsIntrfc->stop();

	SPRD_DBG("sGpsIntrfc->stop()  %d \n", ret);
	sSVNum = 0;
	FUN_EXIT;
	return ret;

}

int gpsClose( void )
{
	FUN_ENTER;

	if( NULL != sGpsIntrfc ) {
		sGpsIntrfc->cleanup();
	}
	sGpsIntrfc = NULL;
	gps_is_enable = 0;
	FUN_EXIT;
	return 0;
}

static int gpsGetSVNum( void )
{
	return sSVNum;
}

static void gps_show_result(unsigned int result)
{
	char buffer[64];
	int row = 3;
	int i;

	FUN_ENTER;
	if(result == 1)
	{
		ui_clear_rows(row, 2);
		memset(buffer, 0, sizeof(buffer));
		sprintf(buffer, GPS_TEST_PASS);
	}
	else if(result == 0)
	{
		ui_clear_rows(row, 2);
		memset(buffer, 0, sizeof(buffer));
		sprintf(buffer, GPS_TEST_FAILED);

	}
	else if(result == 2)
	{
		ui_clear_rows(row, 2);
		memset(buffer, 0, sizeof(buffer));
		sprintf(buffer, "%s, %ds", GPS_TESTING, sTimeout);
	}
	else
	{
		// Must never go to here
		SPRD_DBG("%s, wrong show result !!!\n",__FUNCTION__);
	}
	ui_set_color(CL_WHITE);
	row = ui_show_text(row, 0, buffer);
	memset(buffer, 0, sizeof(buffer));
	if(1)//result==1
	{
	     ui_clear_rows(row, sSVNum);
	     ui_set_color(CL_WHITE);
	     sprintf(buffer, "GPS NUM: %d",sSVNum);
	     row = ui_show_text(row, 0, buffer);
	     for(i=0;i<sSVNum;i++)
	     {
		  memset(buffer, 0, sizeof(buffer));
		  sprintf(buffer, "GPS SNR: %d",sSvSnr[i]);
		  row = ui_show_text(row, 0, buffer);
	     }
	}
	gr_flip();
	FUN_EXIT;
}


int gps_result=0;
static void * processThread_show( void * param )
{
	time_t now_time=time(NULL);
	FUN_ENTER;
	sTimeout = GPS_TEST_TIME_OUT;
	int snr_count=0;
	int snr_count1=0;
	int i;
	sSVNum=0;
	while(thread_run == 1)
	{
		now_time = time(NULL);
		SPRD_DBG("now_time = %ld, open_time=%ld \n", now_time,gOpenTime);
		// Catch the Star
		if(sSVNum >= 4)
		{
		     for(i=0;i<sSVNum;i++)
			{
			     if(sSvSnr[i]>=35)
				 snr_count++;
			     if(sSvSnr[i]>0)
				 snr_count1++;
			}
			if(snr_count>0&&snr_count1>=4)
			{
			    gps_show_result(1);
			    SPRD_DBG("mmitest GPS Test PASS Catch the Star\n");
			    ui_push_result(RL_PASS);
			    sleep(3);
			    break;
			}
		}
		else
		{
			if(now_time - gOpenTime > GPS_TEST_TIME_OUT)
			{
				// Timeout to catch the star, gps test failed
				gps_show_result(0);
				SPRD_DBG("mmitest GPS Test FAILED\n");
				ui_push_result(RL_FAIL);
				sleep(1);
				break;
			}
			else
			{
				gps_show_result(2);
				SPRD_DBG("mmitest GPS Testing ......\n");
				sleep(1);
				sTimeout--;
			}
		}
	}

	FUN_EXIT;
	return NULL;
}

static void location_callback(GpsLocation* loc)
{
	FUN_ENTER;

	if( NULL != loc )
	{

		SPRD_DBG("loc: latitude  = %02f\n", loc->latitude);
		SPRD_DBG("loc: longitude = %02f\n", loc->longitude);
		SPRD_DBG("loc: altitude  = %02f\n", loc->altitude);
		SPRD_DBG("loc: speed     = %02f\n", loc->speed);
		SPRD_DBG("loc: bearing   = %02f\n", loc->bearing);
		SPRD_DBG("loc: accuracy  = %02f\n", loc->accuracy);
		//SPRD_DBG("loc: time      = %s\n",   ctime64(&(loc->timestamp)));
	}
	FUN_EXIT;
}

static void status_callback(GpsStatus* status)
{

	FUN_ENTER;

	if(NULL != status )
		SPRD_DBG("GpsStatus: status  = %d\n", status->status);

	FUN_EXIT;
}

static void sv_status_callback(GpsSvStatus* sv_status)
{
	int i=0;
	FUN_ENTER;
	LOGD("mmitest GpsSvStatus");
	if( NULL != sv_status )
	{
		sSVNum = sv_status->num_svs;
		sPreTest = sSVNum;

		LOGD("mmitest GpsSvStatus: num_svs  = %d\n", sv_status->num_svs);
		for(; i < sv_status->num_svs; ++i ) {
			sSvSnr[i]=sv_status->sv_list[i].snr;
			SPRD_DBG("GpsSvStatus lst[%d]: prn       = %d\n",   i, sv_status->sv_list[i].prn);
			SPRD_DBG("GpsSvStatus lst[%d]: snr       = %02f\n", i, sv_status->sv_list[i].snr);
			SPRD_DBG("GpsSvStatus lst[%d]: elevation = %02f\n", i, sv_status->sv_list[i].elevation);
			SPRD_DBG("GpsSvStatus lst[%d]: azimuth   = %02f\n", i, sv_status->sv_list[i].azimuth);
		}
	}
	FUN_EXIT;
}

static void nmea_callback(GpsUtcTime timestamp, const char* nmea, int length)
{
	FUN_ENTER;
	if( length != 0 && NULL != nmea ) {
		SPRD_DBG("length = %d, nmea = %s\n", length, nmea);
	}
	FUN_EXIT;
}

static void set_capabilities_callback(uint32_t capabilities)
{
	FUN_ENTER;
	SPRD_DBG("%s cap = %d\n", __FUNCTION__, capabilities);
	FUN_EXIT;
}

static void acquire_wakelock_callback()
{
	FUN_ENTER;
        acquire_wake_lock(PARTIAL_WAKE_LOCK, GPSNATIVETEST_WAKE_LOCK_NAME);
	FUN_EXIT;
}

static void release_wakelock_callback()
{
	FUN_ENTER;
        release_wake_lock(GPSNATIVETEST_WAKE_LOCK_NAME);
	FUN_EXIT;
}

static void request_utc_time_callback()
{
	FUN_ENTER;

	FUN_EXIT;
}

typedef void (*PFUN_GPS_THREAD_CALLBACK)(void *);
static PFUN_GPS_THREAD_CALLBACK gps_thread_callback = NULL;

static void * gps_thread_callback_stub( void * prm )
{
	FUN_ENTER;
	if( NULL != gps_thread_callback ) {
		gps_thread_callback(prm);
	}
	FUN_EXIT;
	return NULL;
}

pthread_t create_thread_callback(const char* name, void (*start)(void *), void* arg)
{
	int ret = 0;
	FUN_ENTER;

	gps_thread_callback = start;

	pthread_t id = 0;
	pthread_attr_t attr;
	pthread_attr_init(&attr);
	pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);
	pthread_create(&id, &attr, gps_thread_callback_stub, arg);
	pthread_setname_np(id, name);

	FUN_EXIT;
	return id;
}

int test_gps_pretest(void)
{
	int ret;
	if(sPreTest >= 1)
		ret= RL_PASS;
	else
		ret= RL_FAIL;

	save_result(CASE_TEST_GPS,ret);
	return ret;
}

int test_gps_start(void)
{
	int ret;
	pthread_t t1;
	FUN_ENTER;
	SPRD_DBG("%s V0002 \n", __FUNCTION__);
	ui_fill_locked();
	ui_show_title(MENU_TEST_GPS);
	ret = gpsOpen();
	if( ret < 0)
	{
		SPRD_DBG("%s gps open error = %d \n", __FUNCTION__,ret);
		return -1;
	}
	gOpenTime=time(NULL);
	thread_run = 1;
	pthread_create(&t1, NULL, (void*)processThread_show, NULL);
	ret = ui_handle_button(NULL, NULL);//, TEXT_GOBACK
	thread_run = 0;
	pthread_join(t1,NULL);
	gpsStop();
	gpsClose();

	save_result(CASE_TEST_GPS,ret);
	return ret;
}


