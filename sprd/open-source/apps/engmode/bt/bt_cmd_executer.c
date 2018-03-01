#include <errno.h>
#include <stdio.h>
#include <string.h>
#include <net/if.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdbool.h>
#include <utils/Log.h>
#include <hardware/hardware.h>
#include <hardware/bluetooth.h>
#include <pthread.h>
#include <ctype.h>
#include <sys/prctl.h>
#include <sys/capability.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <netdb.h>
#include <private/android_filesystem_config.h>
#include <android/log.h>
#include <hardware/hardware.h>
#include <hardware/bluetooth.h>

#ifdef LOG_TAG
#undef  LOG_TAG
#endif

#define LOG_TAG "BTENG"


#define BTENG_DEBUG
#ifdef BTENG_DEBUG
#define BTENG_LOGD(x...) ALOGD( x )
#define BTENG_LOGE(x...) ALOGE( x )
#else
#define BTENG_LOGD(x...) do {} while(0)
#define BTENG_LOGE(x...) do {} while(0)
#endif

#define OK_STR "OK"
#define FAIL_STR "FAIL"
#define CMD_RESULT_BUFFER_LEN (128)
#define CASE_RETURN_STR(const) case const: return #const;
#define UNUSED __attribute__((unused))


typedef enum
{
    BT_MODE_NONE,
    BT_MODE_BT_ON,
    BT_MODE_BT_OFF,
    BT_MODE_DUTMODE_CFGON,
    BT_MODE_DUTMODE_CFGOFF,
    BT_MODE_DUTMODE_STATUS,
    BT_MODE_SET_NOSIG_TX,
    BT_MODE_SET_NOSIG_RX,
    BT_MODE_SET_NOSIG_RECV_DATA,
    /* more modes supported to add here */
    BT_MODE_INVALID
}BT_MODE_IN_TESTING;

typedef enum
{
    BT_ENG_NONE_ERROR,
    BT_ENG_CMD_INVALID,
    BT_ENG_PARA_INVALID,
    BT_ENG_HAL_LOAD_ERROR,
    BT_ENG_INIT_ERROR,
    BT_ENG_ENABLE_ERROR,
    BT_ENG_DISABLE_ERROR,
    BT_ENG_SET_DUT_MODE_ERROR,
    BT_ENG_CLEANUP_ERROR,
    BT_ENG_STATUS_ERROR,
    BT_ENG_TX_TEST_ERROR,
    BT_ENG_RX_TEST_ERROR,
    BT_ENG_RX_TEST_RECV_DATA_ERROR
}BT_ENG_ERROR_E;

/************************************************************************************
**  Static variables
************************************************************************************/

static int bt_mode_testing = BT_MODE_NONE;
static unsigned char main_done = 0;
static bt_status_t status;
static bluetooth_device_t* bt_device = NULL;
const bt_interface_t* sBtInterface = NULL;

static gid_t groups[] = { AID_NET_BT, AID_INET, AID_NET_BT_ADMIN,
                  AID_SYSTEM, AID_MISC, AID_SDCARD_RW,
                  AID_NET_ADMIN, AID_VPN};

/* Set to 1 when the Bluedroid stack is enabled */
static unsigned char bt_enabled = 0;
static int bteng_client_fd = -1;
static unsigned char bt_hal_load = 0;
static unsigned char bt_init = 0;
/************************************************************************************
**  Static functions
************************************************************************************/
static void bteng_check_return_status(bt_status_t status);
static BT_ENG_ERROR_E bteng_dut_mode_configure(char *p);
static int bteng_send_back_cmd_result(int client_fd, char *str, int isOK);
static void bteng_hal_unload(void);
static void bteng_cleanup(void);
static BT_ENG_ERROR_E bteng_set_nonsig_tx_testmode(uint32_t *data);
static BT_ENG_ERROR_E bteng_set_nonsig_rx_testmode(uint32_t *data, char *paddr);

/************************************************************************************
**  Functions
************************************************************************************/
static const char* bteng_dump_bt_status(bt_status_t status)
{
    switch(status)
    {
        CASE_RETURN_STR(BT_STATUS_SUCCESS)
        CASE_RETURN_STR(BT_STATUS_FAIL)
        CASE_RETURN_STR(BT_STATUS_NOT_READY)
        CASE_RETURN_STR(BT_STATUS_NOMEM)
        CASE_RETURN_STR(BT_STATUS_BUSY)
        CASE_RETURN_STR(BT_STATUS_UNSUPPORTED)

        default:
            return "unknown status code";
    }
}

static void bteng_adapter_state_changed(bt_state_t state)
{
    char dut_mode;
    char *str_on[]  = {"enter eut mode ok\n", "bt_status=1"};
    char *str_off[] = {"exit eut mode ok\n", "bt_status=0"};
    char *p = NULL;

    BTENG_LOGD("ADAPTER STATE UPDATED : %s", (state == BT_STATE_OFF)?"OFF":"ON");
    if (state == BT_STATE_ON)
    {
        bt_enabled = 1;
        dut_mode = '1';
        if(bt_mode_testing == BT_MODE_DUTMODE_CFGON)
            p = str_on[0];
        else
            p = str_on[1];

        BTENG_LOGD("ON str return to app, %s", p);

        if(bt_mode_testing == BT_MODE_DUTMODE_CFGON)
            bteng_dut_mode_configure(&dut_mode);

        bteng_send_back_cmd_result(bteng_client_fd, p, 1);
    }
    else
    {
        bt_enabled = 0;
        //bteng_cleanup();
        //bteng_hal_unload();
        if(bt_mode_testing == BT_MODE_DUTMODE_CFGOFF)
            p = str_off[0];
        else
            p = str_off[1];

        BTENG_LOGD("OFF str return to app, %s", p);

        //send cmd result
        bteng_send_back_cmd_result(bteng_client_fd, p, 1);
    }
}

static void bteng_dut_mode_recv(uint16_t UNUSED opcode, uint8_t UNUSED *buf, uint8_t UNUSED len)
{
    BTENG_LOGD("DUT MODE RECV : NOT IMPLEMENTED");
}

static void bteng_le_test_mode(bt_status_t status, uint16_t packet_count)
{
    BTENG_LOGD("LE TEST MODE END status:%s number_of_packets:%d", bteng_dump_bt_status(status), packet_count);
}

static void bteng_thread_evt_cb(bt_cb_thread_evt event)
{
    if (event  == ASSOCIATE_JVM)
    {
        BTENG_LOGD("Callback thread ASSOCIATE_JVM");
    }
    else if (event == DISASSOCIATE_JVM)
    {
        BTENG_LOGD("Callback thread DISASSOCIATE_JVM");
    }

    return;
}


/*
        rssi       : Rx RSSI
        pkt_cnt    :Bit Error Rate
        pkt_err_cnt:Rx Byte Rate
        bit_cnt    :Packet Error Rate
        bit_err_cnt:Rx Packet Count
*/
#if defined (SPRD_WCNBT_MARLIN) || defined (SPRD_WCNBT_SR2351)
static void bteng_nonsig_test_rx_recv(bt_status_t status, uint8_t rssi, uint32_t pkt_cnt, uint32_t pkt_err_cnt, uint32_t bit_cnt, uint32_t bit_err_cnt)
{
    char *p = "bteng_nonsig_test_rx_recv response from controller is invalid";
    char buf[255] = {0};

    if(status != BT_STATUS_SUCCESS)
    {
        BTENG_LOGD("%s, controller return status FAIL", __FUNCTION__);
        bteng_send_back_cmd_result(bteng_client_fd, p, 0);

        return;
    }

    snprintf(buf, 255, "rssi:%d, pkt_cnt:%d, pkt_err_cnt:%d, bit_cnt:%d, bit_err_cnt:%d", rssi, pkt_cnt, pkt_err_cnt, bit_cnt, bit_err_cnt);
    BTENG_LOGD("%s, status SUCEESS, %s", __FUNCTION__, buf);

    bteng_send_back_cmd_result(bteng_client_fd, buf, 1);

    return;
}
#endif

static bt_callbacks_t bt_callbacks = {
        sizeof(bt_callbacks_t),
        bteng_adapter_state_changed,
        NULL, /*adapter_properties_cb */
        NULL, /* remote_device_properties_cb */
        NULL, /* device_found_cb */
        NULL, /* discovery_state_changed_cb */
        NULL, /* pin_request_cb  */
        NULL, /* ssp_request_cb  */
        NULL, /*bond_state_changed_cb */
        NULL, /* acl_state_changed_cb */
        bteng_thread_evt_cb, /* thread_evt_cb */
        bteng_dut_mode_recv, /*dut_mode_recv_cb */
        //    NULL, /*authorize_request_cb */
#if BLE_INCLUDED == TRUE
        bteng_le_test_mode, /* le_test_mode_cb */
#else
        NULL,
#endif
        NULL, /* energy_info_cb */
#if defined (SPRD_WCNBT_MARLIN) || defined (SPRD_WCNBT_SR2351)
        bteng_nonsig_test_rx_recv,
#endif
};

/*******************************************************************************
** Load stack lib
*******************************************************************************/
static BT_ENG_ERROR_E bteng_hal_load(void)
{
    int err = 0;
    BT_ENG_ERROR_E ret_val;

    hw_module_t* module;
    hw_device_t* device;

    if(bt_hal_load)
        return BT_ENG_NONE_ERROR;

    BTENG_LOGD("Loading HAL lib");

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

    BTENG_LOGD("HAL library loaded (err: %d, %s)", err, strerror(err));

    if(err == 0)
    {
        ret_val = BT_ENG_NONE_ERROR;
        bt_hal_load = 1;
    }
    else
    {
        ret_val = BT_ENG_HAL_LOAD_ERROR;
    }

    return ret_val;
}

static void bteng_hal_unload(void)
{
    BTENG_LOGD("Unloading HAL lib");

    sBtInterface = NULL;
    bt_hal_load = 0;

    BTENG_LOGD("HAL library unloaded Success");

    return;
}

static void bteng_config_permissions(void)
{
    struct __user_cap_header_struct header;
    struct __user_cap_data_struct cap;

    BTENG_LOGD("set_aid_and_cap : pid %d, uid %d gid %d", getpid(), getuid(), getgid());

    header.pid = 0;

    prctl(PR_SET_KEEPCAPS, 1, 0, 0, 0);

    setuid(AID_ROOT);
    setgid(AID_NET_BT_STACK);

    header.version = _LINUX_CAPABILITY_VERSION;

    cap.effective = cap.permitted =  cap.inheritable =
                1 << CAP_NET_RAW |
                1 << CAP_NET_ADMIN |
                1 << CAP_NET_BIND_SERVICE |
                1 << CAP_SYS_RAWIO |
                1 << CAP_SYS_NICE |
                1 << CAP_SETGID;

    capset(&header, &cap);
    setgroups(sizeof(groups)/sizeof(groups[0]), groups);
}

static bool set_wake_alarm(uint64_t delay_millis, bool UNUSED should_wake, alarm_cb cb, void *data) {
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

static int acquire_wake_lock(const UNUSED char *lock_name) {
  return BT_STATUS_SUCCESS;
}

static int release_wake_lock(const UNUSED char *lock_name) {
  return BT_STATUS_SUCCESS;
}

static bt_os_callouts_t callouts = {
    sizeof(bt_os_callouts_t),
    set_wake_alarm,
    acquire_wake_lock,
    release_wake_lock,
};

static BT_ENG_ERROR_E bteng_bt_init(void)
{
    bt_status_t status;
    BT_ENG_ERROR_E ret_val;

    if(bt_init)
    return   BT_ENG_NONE_ERROR;

    BTENG_LOGD("INIT BT ");
    status = sBtInterface->init(&bt_callbacks);

    if (status == BT_STATUS_SUCCESS) {
        status = sBtInterface->set_os_callouts(&callouts);
    }

    bteng_check_return_status(status);

    if(status == BT_STATUS_SUCCESS)
    {
        ret_val = BT_ENG_NONE_ERROR;
        bt_init = 1;
    }
    else
    {
        ret_val = BT_ENG_INIT_ERROR;
    }

    return ret_val;
}

static BT_ENG_ERROR_E bteng_bt_enable(void)
{
    bt_status_t status;
    BT_ENG_ERROR_E ret_val;

    BTENG_LOGD("ENABLE BT");

    if (bt_enabled)
    {
        BTENG_LOGD("Bluetooth is already enabled");
        return BT_ENG_ENABLE_ERROR;
    }
    status = sBtInterface->enable();

    bteng_check_return_status(status);

    if(status == BT_STATUS_SUCCESS)
    {
        ret_val = BT_ENG_NONE_ERROR;
    }
    else
    {
        ret_val = BT_ENG_ENABLE_ERROR;
    }

    return ret_val;
}

static BT_ENG_ERROR_E bteng_bt_disable(void)
{
    bt_status_t status;
    BT_ENG_ERROR_E ret_val;

    BTENG_LOGD("DISABLE BT");
    if (!bt_enabled)
    {
        BTENG_LOGD("Bluetooth is already disabled");
        return BT_ENG_DISABLE_ERROR;
    }
    status = sBtInterface->disable();

    bteng_check_return_status(status);

    if(status == BT_STATUS_SUCCESS)
    {
        ret_val = BT_ENG_NONE_ERROR;
    }
    else
    {
        ret_val = BT_ENG_DISABLE_ERROR;
    }

    return ret_val;
}

static BT_ENG_ERROR_E bteng_dut_mode_configure(char *p)
{
    int32_t mode = -1;
    bt_status_t status;
    BT_ENG_ERROR_E ret_val;

    BTENG_LOGD("BT DUT MODE CONFIGURE");
    if (!bt_enabled || sBtInterface == NULL)
    {
        BTENG_LOGD("Bluetooth must be enabled for test_mode to work.");
        return BT_ENG_SET_DUT_MODE_ERROR;
    }

    if(*p == '0')
    {
        mode = 0;
    }
    else if(*p == '1')
    {
        mode = 1;
    }

    if ((mode != 0) && (mode != 1))
    {
        BTENG_LOGD("Please specify mode: 1 to enter, 0 to exit");
        return BT_ENG_SET_DUT_MODE_ERROR;
    }
    status = sBtInterface->dut_mode_configure(mode);

    bteng_check_return_status(status);

    if(status == BT_STATUS_SUCCESS)
    {
        ret_val = BT_ENG_NONE_ERROR;
    }
    else
    {
        ret_val = BT_ENG_SET_DUT_MODE_ERROR;
    }

    return ret_val;
}

static BT_ENG_ERROR_E bteng_dut_mode_set( char **argv)
{
    char dut_mode;
    BT_ENG_ERROR_E err = BT_ENG_NONE_ERROR;

    if((*argv)[0] == '1')
    {
        BTENG_LOGD("dut_mode_set enable\n");
        bteng_config_permissions();
        err = bteng_hal_load();
        if(err == BT_ENG_NONE_ERROR)
        {
            err = bteng_bt_init();
            if(err == BT_ENG_NONE_ERROR)
            {
                err = bteng_bt_enable();

                //bteng_dut_mode_configure will bte called after bluetooth on.
                //cmd result will send after bluetooth on.
            }
        }
    }
    else
    {
        BTENG_LOGD("dut_mode_set disable\n");

        dut_mode = '0';
        err = bteng_dut_mode_configure(&dut_mode);
        if(err == BT_ENG_NONE_ERROR)
        {
            err = bteng_bt_disable();
            //cmd result will send after bluetooth off.
        }
    }

    return err;
}

static BT_ENG_ERROR_E bteng_bt_on(void)
{
	int err;
    char dut_mode = '1';

	BTENG_LOGD("bteng_bt_on\n");

    if(bt_enabled == 1)
    {
        BTENG_LOGD("%s, bt is already on\n", __FUNCTION__);
        bteng_adapter_state_changed(BT_STATE_ON);

        return BT_ENG_NONE_ERROR;
    }

    bteng_config_permissions();
    err = bteng_hal_load();
    if(err != BT_ENG_NONE_ERROR)
    {
        BTENG_LOGD("%s, bteng_hal_load, err: %d", __FUNCTION__, err);
        return err;
    }

    err = bteng_bt_init();
    if(err != BT_ENG_NONE_ERROR)
    {
        BTENG_LOGD("%s, bteng_bt_init, err: %d", __FUNCTION__, err);
        return err;
    }

    err = bteng_bt_enable();

    if(err != BT_ENG_NONE_ERROR)
    {
        BTENG_LOGD("%s, bteng_bt_enable, err: %d", __FUNCTION__, err);
        return err;
    }

	//result will send in callback bteng_adapter_state_changed
	return BT_ENG_NONE_ERROR;
}

static BT_ENG_ERROR_E bteng_bt_off(void)
{
	int err;

	BTENG_LOGD("bteng_bt_off\n");

    if(bt_enabled == 0)
    {
        BTENG_LOGD("%s, bt is already off\n", __FUNCTION__);
        bteng_adapter_state_changed(BT_STATE_OFF);

        return BT_ENG_NONE_ERROR;
    }

    err = bteng_bt_disable();

    if(err != BT_ENG_NONE_ERROR)
    {
        BTENG_LOGD("%s, bteng_bt_disable, err: %d", __FUNCTION__, err);
        return err;
    }

	//result will send in callback bteng_adapter_state_changed
	return BT_ENG_NONE_ERROR;
}

#if defined (SPRD_WCNBT_MARLIN) || defined (SPRD_WCNBT_SR2351)
static BT_ENG_ERROR_E bteng_set_nonsig_recv_data(uint16_t le)
{
    int err = BT_ENG_RX_TEST_RECV_DATA_ERROR;

    if (!bt_enabled || sBtInterface == NULL){
        BTENG_LOGD("%s, Bluetooth is disabled", __FUNCTION__);
		err = BT_ENG_STATUS_ERROR;
		return err;
	}

    if((sBtInterface->get_nonsig_rx_data(le)) != BT_STATUS_SUCCESS)
    {
        BTENG_LOGD("%s, get_nonsig_rx_data, err: %d", __FUNCTION__, err);

		return err;
    }

    return BT_ENG_NONE_ERROR;
}

/* paddr: The test bt address is BD_ADDR of Master, In other words, It is the BD_ADDR of sender */
static BT_ENG_ERROR_E bteng_set_nonsig_rx_testmode(uint32_t *data, char *paddr)
{
	int err = BT_ENG_PARA_INVALID;
    uint32_t *p = data;
    bt_bdaddr_t addr;

    memset(&addr, 0, sizeof(bt_bdaddr_t));
    if((NULL == data) ||(NULL == paddr))
    {
        BTENG_LOGD("%s, data or paddr(NULL) err: %d", __FUNCTION__, err);

        return err;
    }

	uint32_t enable     = *p++;
	uint32_t is_le      = *p++;
    uint32_t pattern    = *p++;
	uint32_t channel    = *p++;
    uint32_t pac_type   = *p++;
    uint32_t rx_gain    = *p;

    BTENG_LOGD("bteng_set_nonsig_rx_testmode");

    if(enable)
    {
	    if(pattern != 7)
	    {
	        BTENG_LOGD("%s, pattern invalid err: %d", __FUNCTION__, err);

            return err;
	    }

        if(channel > 78)
        {
            BTENG_LOGD("%s, channel invalid err: %d", __FUNCTION__, err);

            return err;
        }


        if(rx_gain > 32)
        {
            BTENG_LOGD("%s, rx_gain invalid err: %d", __FUNCTION__, err);
            return err;
        }
    }

	if (!bt_enabled || sBtInterface == NULL){
        BTENG_LOGD("Bluetooth is disabled");
		err = BT_ENG_STATUS_ERROR;
		return err;
	}

    sscanf(paddr, "%02x:%02x:%02x:%02x:%02x:%02x", &addr.address[0], &addr.address[1], &addr.address[2], &addr.address[3], &addr.address[4], &addr.address[5]);

    BTENG_LOGD("enable     = %d",enable);
	BTENG_LOGD("is_le      = %d",is_le);
    BTENG_LOGD("pattern    = %d",pattern);
	BTENG_LOGD("channel    = %d",channel);
	BTENG_LOGD("pac_type   = 0x%x",pac_type);
	BTENG_LOGD("rx_gain    = %d",rx_gain);
    BTENG_LOGD("addr : %02x:%02x:%02x:%02x:%02x:%02x", addr.address[0], addr.address[1], addr.address[2], addr.address[3], addr.address[4], addr.address[5]);

    /* enable: 0 NONSIG_RX_DISABLE      1 NONSIG_RX_ENABLE*/
	err = sBtInterface->set_nonsig_rx_testmode(enable, is_le, pattern, channel, pac_type, rx_gain, addr);
	if(err != BT_STATUS_SUCCESS)
	{
	    BTENG_LOGD("%s, set_nonsig_tx_testmode, err: %d", __FUNCTION__, err);
		return BT_ENG_RX_TEST_ERROR;
	}
	else
		bteng_send_back_cmd_result(bteng_client_fd, "set_nosig_rx_testmode_ok\n", 1);

    BTENG_LOGD("bteng_set_nonsig_rx_testmode return SUCCESS");

	return BT_ENG_NONE_ERROR;
}

static BT_ENG_ERROR_E bteng_set_nonsig_tx_testmode(uint32_t *data)
{
	int err = BT_ENG_PARA_INVALID;
    uint32_t *p = data;
    int power_gain_base = 0xce00;
    int power_level_base[2] = {0x1012, 0x0000};
    if(NULL == data)
    {
        BTENG_LOGD("%s, data(NULL) err: %d", __FUNCTION__, err);

        return err;
    }

    BTENG_LOGD("bteng_set_nonsig_tx_testmode");

	uint32_t enable     = *p++;
	uint32_t is_le      = *p++;
    uint32_t pattern    = *p++;
	uint32_t channel    = *p++;
    uint32_t pac_type   = *p++;
    uint32_t pac_len    = *p++;
	uint32_t power_type = *p++;
	uint32_t power_value= *p++;
	uint32_t pac_cnt    = *p;

    if(enable)
    {
	    if((pattern != 1) && (pattern != 2) && (pattern != 3) && (pattern != 4) && (pattern != 9))
	    {
	        BTENG_LOGD("%s, pattern invalid err: %d", __FUNCTION__, err);

            return err;
	    }

        if(!((channel <= 78) || (channel == 255)))
        {
            BTENG_LOGD("%s, channel invalid err: %d", __FUNCTION__, err);

            return err;
        }

	    if(!((pac_type <= 16) || ((pac_type >= 20) && (pac_type <= 31))))
	    {
	        BTENG_LOGD("%s, pac_type invalid err: %d", __FUNCTION__, err);

            return err;
	    }

        if(!(pac_len <= 65535))
        {
            BTENG_LOGD("%s, pac_len invalid err: %d", __FUNCTION__, err);

            return err;
        }

        if((power_type != 0) && (power_type != 1))
        {
            BTENG_LOGD("%s, power_type invalid err: %d", __FUNCTION__, err);

            return err;
        }

        if(!(power_value <= 33))
        {
            BTENG_LOGD("%s, power_value invalid err: %d", __FUNCTION__, err);

            return err;
        }

        if(!(pac_cnt <= 65535))
        {
            BTENG_LOGD("%s, pac_cnt invalid err: %d", __FUNCTION__, err);

            return err;
        }
    }

	if (!bt_enabled || sBtInterface == NULL){
        BTENG_LOGD("Bluetooth is disabled");
		err = BT_ENG_STATUS_ERROR;
		return err;
	}
/*
    if(power_type == 0) // power gain
    {
        power_value += power_gain_base;
    }
    else                // power level
    {
        if(power_value <= 17)
            power_value = power_level_base[0] - power_value;
        else
            power_value =  power_level_base[1] + (power_value - 18);
    }
*/
    BTENG_LOGD("enable     = %d",enable);
	BTENG_LOGD("is_le      = %d",is_le);
    BTENG_LOGD("pattern    = %d",pattern);
	BTENG_LOGD("channel    = %d",channel);
	BTENG_LOGD("pac_type   = 0x%x",pac_type);
	BTENG_LOGD("pac_len    = %d",pac_len);
    BTENG_LOGD("power_type = %d",power_type);
	BTENG_LOGD("power_value= 0x%x",power_value);
	BTENG_LOGD("pac_cnt    = %d",pac_cnt);

    /* enable: 0 NONSIG_TX_DISABLE      1 NONSIG_TX_ENABLE*/
	err = sBtInterface->set_nonsig_tx_testmode(enable, is_le, pattern, channel, pac_type, pac_len, power_type,
		power_value, pac_cnt);
	if(err != BT_STATUS_SUCCESS)
	{
	    BTENG_LOGD("%s, set_nonsig_tx_testmode, err: %d", __FUNCTION__, err);
		return BT_ENG_TX_TEST_ERROR;
	}
	else
		bteng_send_back_cmd_result(bteng_client_fd, "set_nosig_tx_testmode_ok\n", 1);


	return BT_ENG_NONE_ERROR;
}
#else
static BT_ENG_ERROR_E bteng_set_nonsig_recv_data(uint16_t le)
{
    BTENG_LOGD("bteng_set_nonsig_recv_data   : NOT IMPLEMENTED");
    return BT_ENG_RX_TEST_RECV_DATA_ERROR;
}

static BT_ENG_ERROR_E bteng_set_nonsig_rx_testmode(uint32_t *data, char *paddr)
{
    BTENG_LOGD("bteng_set_nonsig_rx_testmode : NOT IMPLEMENTED");
    return BT_ENG_RX_TEST_ERROR;
}

static BT_ENG_ERROR_E bteng_set_nonsig_tx_testmode(uint32_t *data)
{
    BTENG_LOGD("bteng_set_nonsig_tx_testmode : NOT IMPLEMENTED");
    return BT_ENG_TX_TEST_ERROR;
}
#endif

static void bteng_cleanup(void)
{
    BTENG_LOGD("CLEANUP");
    sBtInterface->cleanup();
}

static void bteng_check_return_status(bt_status_t status)
{
    if (status != BT_STATUS_SUCCESS)
    {
        BTENG_LOGD("HAL REQUEST FAILED status : %d (%s)", status, bteng_dump_bt_status(status));
    }
    else
    {
        BTENG_LOGD("HAL REQUEST SUCCESS");
    }
}

static int bteng_send_back_cmd_result(int client_fd, char *str, int isOK)
{
    char buffer[255] = {0};

    BTENG_LOGD("SEND CMD RESULT");
    if(client_fd < 0)
    {
        fprintf(stderr, "write %s to invalid fd \n", str);

        return -1;
    }

    memset(buffer, 0, sizeof(buffer));

    if(!str)
    {
        snprintf(buffer, 255, "%s",  (isOK?OK_STR:FAIL_STR));
    }
    else
    {
        snprintf(buffer, 255, "%s %s", (isOK?OK_STR:FAIL_STR), str);
    }

    int ret = write(client_fd, buffer, strlen(buffer)+1);

    BTENG_LOGD("%s, client fd:%d, cmd_result: %s, ret: %d", __FUNCTION__, client_fd, buffer, ret);
    if(ret < 0)
    {
        BTENG_LOGD("write %s to client_fd:%d fail (error(%d):%s)", buffer, client_fd, errno, strerror(errno));
        fprintf(stderr, "write %s to client_fd:%d fail (error:%s)", buffer, client_fd, strerror(errno));
        return -1;
    }

    return 0;
}

/**
* cmd: bt cmd arg1 arg2 ...
*/
int bt_runcommand(int client_fd, int argc, char **argv)
{
    BT_ENG_ERROR_E err = BT_ENG_NONE_ERROR;
    char result_buf[CMD_RESULT_BUFFER_LEN];
    int ret_val = 0;
    int i = 0;
    int m = 0;
    uint32_t data[10] = {0};

    memset(result_buf, 0, sizeof(result_buf));
    bteng_client_fd = client_fd;

    if (strncmp(*argv, "bt_on", 5) == 0)
    {
        BTENG_LOGD("recv bt_on.");

        bt_mode_testing = BT_MODE_BT_ON;
        err = bteng_bt_on();
    }
    else if (strncmp(*argv, "bt_off", 6) == 0)
    {
        BTENG_LOGD("recv bt_off.");

        bt_mode_testing = BT_MODE_BT_OFF;
        err = bteng_bt_off();
    }
    else if (strncmp(*argv, "dut_mode_configure", 18) == 0 && argc > 1)
    {
        argv++;
        argc--;

        if((*argv)[0] == '1')
        {
            BTENG_LOGD("recv dut_mode_configure cmd.");
            bt_mode_testing = BT_MODE_DUTMODE_CFGON;

            err = bteng_dut_mode_set(argv);
        }
        else if((*argv)[0] == '0')
        {
            BTENG_LOGD("recv dut_mode_configure cmd.");
            bt_mode_testing = BT_MODE_DUTMODE_CFGOFF;

            err = bteng_dut_mode_set(argv);
        }
        else
        {
            BTENG_LOGE("%s, dut_mode_configure parameter invalid.", __FUNCTION__);
            fprintf(stderr, "dut_mode_configure parameter invalid\n");
            err = BT_ENG_PARA_INVALID;
        }
    }
    else if(strncmp(*argv, "eut_status", 10) == 0)
    {
        BTENG_LOGD("recv eut_status.");

        if((bt_mode_testing == BT_MODE_BT_OFF) || (bt_mode_testing == BT_MODE_DUTMODE_CFGOFF))
        {
            BTENG_LOGE("eut_status: just close, we think always close success");
            sprintf(result_buf,"return value: %d",  0);
        }
        else
        {
            sprintf(result_buf,"return value: %d",  bt_enabled);
        }

        bt_mode_testing = BT_MODE_DUTMODE_STATUS;

        bteng_send_back_cmd_result(client_fd, result_buf, 1);

        err = BT_ENG_NONE_ERROR;
    }
    else if(strncmp(*argv, "set_nosig_tx_testmode", 21) == 0)
    {
        BTENG_LOGD("recv set_nosig_tx_testmode.");
		BTENG_LOGE("set_nosig_tx_testmode argc = %d",argc);
        bt_mode_testing = BT_MODE_SET_NOSIG_TX;

        for(m = 0; m < argc; m++)
            BTENG_LOGE("%s, set_nosig_tx_testmode argv[%d]: %s.", __FUNCTION__, m, argv[m]);

		if(argc != 10){
            BTENG_LOGE("%s, set_nosig_tx_testmode parameter invalid.", __FUNCTION__);
			err = BT_ENG_PARA_INVALID;
		}else{
			for(i=0; i < argc-1; i++)
				data[i] = atoi(argv[i+1]);

			err = bteng_set_nonsig_tx_testmode(data);
		}
	}
	else if(strncmp(*argv, "set_nosig_rx_testmode", 21) == 0)
    {
        BTENG_LOGD("recv set_nosig_rx_testmode.");
		BTENG_LOGE("set_nosig_rx_testmode argc = %d",argc);
        bt_mode_testing = BT_MODE_SET_NOSIG_RX;

        for(m = 0; m < argc; m++)
            BTENG_LOGE("%s, set_nosig_tx_testmode argv[%d]: %s.", __FUNCTION__, m, argv[m]);

        if(argc != 8)
        {
            BTENG_LOGE("%s, set_nosig_rx_testmode parameter invalid.", __FUNCTION__);
            err = BT_ENG_PARA_INVALID;
        }
        else
        {
            for(i=0; i < argc - 2; i++)
				data[i] = atoi(argv[i+1]);

			err = bteng_set_nonsig_rx_testmode(data, argv[argc - 1]);
        }
	}
    else if(strncmp(*argv, "set_nosig_rx_recv_data_le", 25) == 0)
    {
        BTENG_LOGD("recv set_nosig_rx_recv_data_le.");
        bt_mode_testing = BT_MODE_SET_NOSIG_RECV_DATA;

        err = bteng_set_nonsig_recv_data(1);
	}
    else if(strncmp(*argv, "set_nosig_rx_recv_data", 22) == 0)
    {
        BTENG_LOGD("recv set_nosig_rx_recv_data_classic.");
        bt_mode_testing = BT_MODE_SET_NOSIG_RECV_DATA;

        err = bteng_set_nonsig_recv_data(0);
	}
    else
    {
        bt_mode_testing = BT_MODE_INVALID;
        BTENG_LOGE("rcv cmd is invalid.");
        fprintf(stderr, "rcv cmd is invalid.\n");
        err = BT_ENG_CMD_INVALID;
    }

    if (err == BT_ENG_NONE_ERROR)
    {
        ret_val = 0;
        //cmd result will be sent by every cmd
    }
    else
    {
        ret_val = 1;

        switch(err)
            {
                case BT_ENG_CMD_INVALID:
                    sprintf(result_buf, "invalid cmd");
                    break;
                case BT_ENG_PARA_INVALID:
                    sprintf(result_buf, "invalid parameter");
                    break;
                case BT_ENG_HAL_LOAD_ERROR:
                    sprintf(result_buf, "hal load error");
                    break;
                case BT_ENG_INIT_ERROR:
                    sprintf(result_buf, "bt init error");
                    break;
                case BT_ENG_ENABLE_ERROR:
                    sprintf(result_buf, "bt enable error");
                    break;
                case BT_ENG_DISABLE_ERROR:
                    sprintf(result_buf, "bt disable error");
                    break;
                case BT_ENG_SET_DUT_MODE_ERROR:
                    sprintf(result_buf, "set dut mode error");
                    break;
                case BT_ENG_CLEANUP_ERROR:
                    sprintf(result_buf, "bt clean up error");
                    break;
                case BT_ENG_STATUS_ERROR:
                    sprintf(result_buf, "bt status error");
                    break;
				case BT_ENG_TX_TEST_ERROR:
                    sprintf(result_buf, "bt tx test error");
                    break;
				case BT_ENG_RX_TEST_ERROR:
                    sprintf(result_buf, "bt rx test error");
                    break;
                case BT_ENG_RX_TEST_RECV_DATA_ERROR:
                    sprintf(result_buf, "bt rx test recv data error");
                    break;
                default:
                    break;
            }

        bteng_send_back_cmd_result(client_fd, result_buf, 0);
    }

    return ret_val;
}
