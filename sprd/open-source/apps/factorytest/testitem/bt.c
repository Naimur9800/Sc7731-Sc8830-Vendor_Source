#include <stdio.h>
#include <dlfcn.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <pthread.h>
#include <unistd.h>
#include <ctype.h>
#include <fcntl.h>
#include <sys/prctl.h>
#include <sys/capability.h>
#include <semaphore.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <netdb.h>
#include <private/android_filesystem_config.h>
#include <android/log.h>
#include <hardware/bluetooth.h>
#include <hardware/bt_sock.h>

//MMI include file
#include <unistd.h>
#include <linux/input.h>
#include <fcntl.h>
#include "testitem.h"

/************************************************************************************
 **  Local type definitions
 ************************************************************************************/
typedef char bdstr_t[18];

typedef struct
{
    bt_bdaddr_t bd_addr;
    bdstr_t bdstr;
    bt_bdname_t bd_name;
}remote_bt_t;

/************************************************************************************
 **  Static variables
 ************************************************************************************/

unsigned char BT_STATE=BT_STATE_OFF;
//static void bdt_log(const char *fmt_str, ...);
static bluetooth_device_t* bt_device;
static bt_status_t status;
const  bt_interface_t* sBtInterface = NULL;

/* Set to 1 when the Bluedroid stack is enabled */
static bdstr_t local_bd_str;
static sem_t eng_bt_sem;
static int remote_bt_num = 0;
static int eng_btfound   = 0;
static int remote_bd_new = 1;
static unsigned char main_done  = 0;
static unsigned char bt_enabled = 0;
remote_bt_t remote_bt_devs[MAX_REMOTE_DEVICES];
extern char *rfkill_state_path;


/************************************************************************************
 **  Static functions
 ************************************************************************************/
void eng_bt_start_discovery(void);

int  eng_bt_close(void);
int eng_bt_display(void);

void eng_bt_cancel_discovery(void);
/*
void bdt_log(const char *fmt_str, ...)
{
    static char buffer[1024];
    va_list ap;

    va_start(ap, fmt_str);
    vsnprintf(buffer, 1024, fmt_str, ap);
    va_end(ap);

    fprintf(stdout, "%s\n", buffer);
}
*/

static const char* dump_bt_status(bt_status_t status)
{
    switch(status)
    {
            CASE_RETURN_STR(BT_STATUS_SUCCESS)
            CASE_RETURN_STR(BT_STATUS_FAIL)
            CASE_RETURN_STR(BT_STATUS_NOT_READY)
            CASE_RETURN_STR(BT_STATUS_NOMEM)
            CASE_RETURN_STR(BT_STATUS_BUSY)
            CASE_RETURN_STR(BT_STATUS_DONE)
            CASE_RETURN_STR(BT_STATUS_UNSUPPORTED)
        default:
            return "unknown status code";
    }
}

void check_return_status(bt_status_t status)
{
    if (status != BT_STATUS_SUCCESS)
    {
        SPRD_DBG("mmitest HAL REQUEST FAILED status : %d (%s)", status, dump_bt_status(status));
    }
    else
    {
        SPRD_DBG("mmitest HAL REQUEST SUCCESS");
    }
}

static int bdcmp(const BD_ADDR a, const BD_ADDR b)
{
    int i;

    for (i = BD_ADDR_LEN; i != 0; i--)
    {
        if (*a++ != *b++)
        {
            return -1;
        }
    }
    return 0;
}

static char *btaddr2str(const bt_bdaddr_t *bdaddr, bdstr_t *bdstr)
{
    char *addr = (char *) bdaddr->address;

    sprintf((char*)bdstr, "%02X:%02X:%02X:%02X:%02X:%02X",
            (int)addr[0],(int)addr[1],(int)addr[2],
            (int)addr[3],(int)addr[4],(int)addr[5]);
    return (char *)bdstr;
}

static void eng_bt_clear_list(void)
{
    memset(remote_bt_devs, 0, sizeof(remote_bt_devs));

    remote_bt_num = 0;
}

void eng_bt_get_discovery_result(int num_bt_devices)
{
    int i;

    SPRD_DBG(":::::::::::::::::::::::::::::::::::::::::::::::");

    SPRD_DBG("The Discoveryed Bluetooth Device List:");

    for(i = 0; i < num_bt_devices; i++)
    {
        SPRD_DBG("Number:%02X  MAC: %s Name: %s ",i,remote_bt_devs[i].bdstr,remote_bt_devs[i].bd_name.name);
    }

}

static int eng_bt_check_bd_new(int num_devices,bt_bdaddr_t check_bd_addr)
{
    int i;
    int bd_new_flag = 1;

    for(i = 0; i < num_devices; i++)
    {
       if (bdcmp((BD_ADDR_PTR)check_bd_addr.address, (BD_ADDR_PTR)remote_bt_devs[i].bd_addr.address) == 0)
        {
           bd_new_flag = 0;
           //SPRD_DBG("the old mac: %s ", (char*)remote_bt_devs[i].bdstr);
           break;
        }
    }

    return bd_new_flag;
}

static void device_found_cb(int num_properties, bt_property_t *properties)
{
    int i = 0;
    int j = 0;
    char datatmp[256];
    int16_t  name_len = 0;
    bt_bdaddr_t remote_bd_addr;
    bt_bdname_t remote_bd_name;
    bdstr_t remote_bd_str;

    memset(&remote_bd_addr, 0, sizeof(bt_bdaddr_t));
    memset(&remote_bd_str,  0, sizeof(bdstr_t));
    memset(&remote_bd_name, 0, sizeof(bt_bdname_t));
    LOGD("mmitest stepinto device_found_cb");
    for(i = 0; i < num_properties; i++)
    {
        switch(properties[i].type)
        {
            case BT_PROPERTY_BDNAME:
                {
                    name_len = properties[i].len;
                    memcpy(remote_bd_name.name,properties[i].val, name_len);
                    remote_bd_name.name[name_len] = '\0';
                }break;

            case BT_PROPERTY_BDADDR:
                {
                    bt_bdaddr_t *bd_addr = (bt_bdaddr_t*)properties[i].val;
                    memcpy(&remote_bd_addr,(bt_bdaddr_t*)bd_addr, sizeof(bt_bdaddr_t));
                    btaddr2str(&remote_bd_addr, &remote_bd_str);
                }break;

            default:
                ;
        }
    }
    //if the mac is new , insert it into devices list
    if( eng_bt_check_bd_new(remote_bt_num,remote_bd_addr) == 1 )
    {
        memset(&datatmp, 0, sizeof(datatmp));
        memcpy(&remote_bt_devs[remote_bt_num].bd_addr, &remote_bd_addr, sizeof(bt_bdaddr_t));
        memcpy(&remote_bt_devs[remote_bt_num].bd_name.name,remote_bd_name.name,sizeof(bt_bdname_t));
        memcpy(&remote_bt_devs[remote_bt_num].bdstr,remote_bd_str,sizeof(bdstr_t));
        remote_bt_num++;

        SPRD_DBG("mmi_bt: Insert the new bt mac : %s name: %s",remote_bt_devs[remote_bt_num].bdstr,remote_bt_devs[remote_bt_num].bd_name.name);
        SPRD_DBG("mmi_bt:::::::Found the first device && eng_bt_cancel_discovery():::::::::::::::::::::::::");
        eng_bt_cancel_discovery();

    }

}

static void adapter_state_changed(bt_state_t state)
{
    SPRD_DBG("ADAP TER STATE UPDATED : %s", (state == BT_STATE_OFF)?"OFF":"ON");

	BT_STATE=state;

	SPRD_DBG("!!!!!!!!!!!!!!!error is here!!!!!!!!!!!!!!!!!!!!");

	if (state == BT_STATE_ON)
    {

     SPRD_DBG("mmitest :::::::::::::::state = BT_STATE_ON:::::::::::::::::::::::::::::::::");

     bt_enabled = 1;

     sem_post(&eng_bt_sem);

    } else
    {

     SPRD_DBG("adapter_state_changed mmitest :::::::::::::::state = BT_STATE_OFF::::::::::::::::::::::::::::::::");

     bt_enabled = 0;

     sem_post(&eng_bt_sem);

    }

	SPRD_DBG("bt_callbacks eng_bt_init eng_bt_open eng_bt_sem post = %d\n",eng_bt_sem);
}

void discovery_state_changed_cb(bt_discovery_state_t state)
{
    switch(state)
    {
        case BT_DISCOVERY_STOPPED:

            SPRD_DBG("mmitest BT Discovery State Changed: BT_DISCOVERY_STOPPED");

            sem_post(&eng_bt_sem);

            break;

        case BT_DISCOVERY_STARTED:

            SPRD_DBG("mmitest BT Discovery State Changed: BT_DISCOVERY_STARTED");

            break;

        default:
            SPRD_DBG("mmitest BT Discovery State Changed: BT_DISCOVERY_ERROR");
    }
}

/*******************************************************************************
 ** Load stack lib
 *******************************************************************************/


static int eng_bt_hal_load(void)
{
    int err = 0;
    hw_module_t* module;
    hw_device_t* device;

    SPRD_DBG("mmitest Loading HAL lib + extensions");

    err = hw_get_module(BT_HARDWARE_MODULE_ID, (hw_module_t const**)&module);

    if (err == 0)
    {
        err = module->methods->open(module, BT_HARDWARE_MODULE_ID, &device);

        if (err == 0)
        {
            bt_device = (bluetooth_device_t *)device;
            sBtInterface = bt_device->get_bluetooth_interface();
        }
    }

    SPRD_DBG("mmitest HAL library loaded (%s)", strerror(err));

    return err;
}

static int eng_bt_hal_unload(void)
{
    int err = 0;

    sBtInterface = NULL;

    SPRD_DBG("HAL library unloaded (%s)", strerror(err));

    return err;
}

static bt_callbacks_t bt_callbacks = {
    sizeof(bt_callbacks_t),
    adapter_state_changed,

    NULL, /*adapter_properties_cb */
    NULL, /* remote_device_properties_cb */

	device_found_cb, /* device_found_cb */
    discovery_state_changed_cb,

	NULL, /* pin_request_cb  */
    NULL, /* ssp_request_cb  */
    NULL, /*bond_state_changed_cb */
    NULL, /* acl_state_changed_cb */
    NULL, /* thread_evt_cb */
    NULL, /*dut_mode_recv_cb */
    //    NULL, /*authorize_request_cb */
#if BLE_INCLUDED == TRUE
    NULL, /* le_test_mode_cb */
#else
    NULL
#endif
};


static bool set_wake_alarm(uint64_t delay_millis, bool should_wake, alarm_cb cb, void *data) {
    static timer_t timer;
    static bool timer_created;

    if (!timer_created) {
        struct sigevent sigevent;
        memset(&sigevent, 0, sizeof(sigevent));
        sigevent.sigev_notify = SIGEV_THREAD;
        sigevent.sigev_notify_function = (void (*)(union sigval))cb;
        sigevent.sigev_value.sival_ptr = data;
        timer_create(CLOCK_MONOTONIC, &sigevent, &timer);
        timer_created = true;
    }

    struct itimerspec new_value;
    new_value.it_value.tv_sec = delay_millis / 1000;
    new_value.it_value.tv_nsec = (delay_millis % 1000) * 1000 * 1000;
    new_value.it_interval.tv_sec = 0;
    new_value.it_interval.tv_nsec = 0;
    timer_settime(timer, 0, &new_value, NULL);

    return true;
}



static int acquire_wake_lock(const char *lock_name) {
      return BT_STATUS_SUCCESS;
}

static int release_wake_lock(const char *lock_name) {
      return BT_STATUS_SUCCESS;
}

static bt_os_callouts_t callouts = {
        sizeof(bt_os_callouts_t),
        set_wake_alarm,
        acquire_wake_lock,
        release_wake_lock,
};

void eng_bt_init(void)
{
    SPRD_DBG("INIT BT Start");

    status = sBtInterface->init(&bt_callbacks);//////error is here///////////////////

    //add by zhuzhenwu 2015/3/11
    if (status == BT_STATUS_SUCCESS) {
        status = sBtInterface->set_os_callouts(&callouts);
    }

    SPRD_DBG("INIT BT End");
    check_return_status(status);
}

void eng_bt_enable(void)
{
    SPRD_DBG("ENABLE BT");

    if (bt_enabled)
    {
        SPRD_DBG("Bluetooth is already enabled");
        return;
    }

    status = sBtInterface->enable();
    LOGD("mmitest status=%d ",status);
    check_return_status(status);
}

void eng_bt_disable(void)
{
    SPRD_DBG("DISABLE BT");

    if (!bt_enabled)
    {
        SPRD_DBG("Bluetooth is already disabled");

        return;
    }

    status = sBtInterface->disable();

    check_return_status(status);
}

void eng_bt_cleanup(void)
{
    SPRD_DBG("CLEANUP BT");

    sBtInterface->cleanup();

    check_return_status(status);
}

void eng_bt_start_discovery(void)
{
    if (!bt_enabled)
    {
        SPRD_DBG("mmitest Bluetooth must be enabled for bt scan to work.");
        return;
    }
    LOGD("mmitest before start_discovery");
    status = sBtInterface->start_discovery();
    LOGD("mmitest after start_discovery");
    check_return_status(status);
}

void eng_bt_cancel_discovery(void)
{
    if (!bt_enabled)
    {
        SPRD_DBG("Bluetooth must be enabled for bt cancel discovery to work.");
        return;
    }

    status = sBtInterface->cancel_discovery();

    check_return_status(status);
}



int eng_bt_open( void )
{
	int i;
	char *ptrtest;
	if ( eng_bt_hal_load() < 0 )
    {
        SPRD_DBG("mmitest BT MMI:HAL failed to initialize, exit\n");

		return -1;
    }

    LOGD("mmitest after hal load");

	eng_bt_init();
    LOGD("mmitest after init");

	SPRD_DBG("bt_callbacks eng_bt_init eng_bt_open eng_bt_sem = %d\n",eng_bt_sem);

    eng_bt_enable();
    LOGD("mmitest after enable");
    if(status != 0)
    {
        return -1;
    }

    return 0;
}

int eng_bt_close(void)
{
    int err = 0;

    eng_bt_disable();

    err = eng_bt_hal_unload();

    return err;
}

void eng_bt_scan_start(void)
{
    int err,i;
    char *ptrtest;
    SPRD_DBG(":::::::::::::::::::::::::::::::::::::::::::::::::::");

    SPRD_DBG("v17 zzl  Bluedroid test app starting");
    system("rm -f /data/misc/bluedroid/bt_config.xml");
    system("rm -f /data/misc/bluedroid/bt_config.new");
    system("rm -f /data/misc/bluedroid/bt_config.old");

    //Semaphore initialization
    sem_init(&eng_bt_sem, 0, 0);


	SPRD_DBG("Bug Test start   ##############################\n");

    err = eng_bt_open();///////////Find Bug In This///////////////

	SPRD_DBG("Bug Test end	   ##############################\n");


    if(err)
    {
        return ;
    }
	SPRD_DBG("mmtest:waiting open bt\n");

    sem_wait(&eng_bt_sem);/////////////////////

    SPRD_DBG(":::::::::: BT_STATE_ON  && start: eng_bt_start_discovery();:::::::::::::::::::::::::::::::::::::::::::::");

    usleep(1000*1000);
    eng_bt_start_discovery();


    SPRD_DBG(":::::::::: sleep after && start: eng_bt_cancel_discovery();:::::::::::::::::::::::::::::::::::::::::");



    sem_wait(&eng_bt_sem);
    SPRD_DBG("::::::::::BT_DISCOVERY_STOPPED && start : eng_bt_disable ;::::::::::::::::::::::::::::::::::::");

	eng_bt_disable();


    sem_wait(&eng_bt_sem);

    SPRD_DBG("::::::::::get BT_STATE_OFF sem 3 && start : cleanup and unload HAL ;:::::::::::::::::::::::::::::::::::::::::");

    eng_bt_cleanup();


    eng_bt_hal_unload();

    sem_destroy(&eng_bt_sem);

    SPRD_DBG("Bluedroid test app eixt!");

    return ;
}

int eng_bt_display(void)
{
    int i;
    int ret;
    char datatmp[256];
	int row = 4;
    memset(&datatmp, 0, sizeof(datatmp));
    for(i = 0; i < remote_bt_num; i++)
    {
        sprintf(datatmp,"%s :",remote_bt_devs[i].bdstr);
		ui_set_color(CL_GREEN);
		row = ui_show_text(row, 0, datatmp);
		ui_set_color(CL_WHITE);
		row = ui_show_text(row, 0, (char *)remote_bt_devs[i].bd_name.name);
        SPRD_DBG("Display->Number:%02X  MAC: %s Name: %s ",
				i,remote_bt_devs[i].bdstr,remote_bt_devs[i].bd_name.name);
		if(i > 3) {
			row = ui_show_text(row, 0, "... ...");
			break;
		}
    }
    //display search results
    if(remote_bt_num > 0) {
		ui_set_color(CL_GREEN);
        ret = 0;
		row = ui_show_text(row, 0, TEXT_TEST_PASS);
    } else {
        ret = -1;
		ui_set_color(CL_RED);
		row = ui_show_text(row, 0, TEXT_TEST_FAIL);
    }

    gr_flip();

    return ret;
}
/************************************************************************************
 **  the bt fuctions
 ************************************************************************************/
int test_bt_pretest(void)
{
	int ret;
	if(remote_bt_num > 0)
		ret= RL_PASS;
	else
		ret= RL_FAIL;

	save_result(CASE_TEST_BT,ret);
	return ret;
}

int test_bt_start(void)
{
    LOGD("mmitest zhuzhenwu");
    int ret;
    SPRD_DBG("==================%s enter:====================", __FUNCTION__);
    //remote_bt_num=0;
    //sprd_ui_draw_title(SPRD_BT_TITLE_Y, SPRD_BT_TITLE, strlen(SPRD_BT_TITLE));
	ui_fill_locked();
	ui_show_title(MENU_TEST_BT);
	ui_set_color(CL_WHITE);
	if(BT_STATE==BT_STATE_OFF)
	{
		ui_show_text(2, 0, TEXT_BT_SCANING);
		gr_flip();

		LOGD("mmitest test BUG scan begin");

		eng_bt_scan_start(); //add by qing for test request

		LOGD("mmitest test BUG scan end");

		LOGD("mmitest after scan");

		//eng_bt_mac();

	    eng_bt_get_discovery_result(remote_bt_num);

	    if(!eng_bt_display())
	    {
	        ret = RL_PASS;
	    }else{
	        ret = RL_FAIL;
	    }
	}
	else
	{
		ui_show_text(3,0,BT_BACK_TEST);
		gr_flip();
		ret=RL_NA;
	}
    eng_bt_clear_list();
    sleep(1);

	save_result(CASE_TEST_BT,ret);
    return ret;
}
