#ifndef __MODEMD_H__

#define __MODEMD_H__

#define MODEMD_DEBUG

#ifdef MODEMD_DEBUG
#define MODEMD_LOGD(x...) ALOGD( x )
#define MODEMD_LOGE(x...) ALOGE( x )
#else
#define MODEMD_LOGD(x...) do {} while(0)
#define MODEMD_LOGE(x...) do {} while(0)
#endif

#define TD_MODEM                    0x3434
#define W_MODEM                     0x5656
#define LTE_MODEM                   0x7878
#define LF_MODEM                    0x6868
#define TL_MODEM                    0x9898

#define MODEM_READY                 0
#define MODEM_ASSERT                1
#define MODEM_RESET                 2

#define MAX_CLIENT_NUM              10
#define min(A,B)                    (((A) < (B)) ? (A) : (B))

/* indicate which modem is enable */
#define TD_MODEM_ENABLE_PROP        "persist.modem.t.enable"
#define  W_MODEM_ENABLE_PROP        "persist.modem.w.enable"
#define LTE_MODEM_ENABLE_PROP        "persist.modem.l.enable"
#define LF_MODEM_ENABLE_PROP        "persist.modem.lf.enable"
#define TL_MODEM_ENABLE_PROP        "persist.modem.tl.enable"

// modem is external or not, default is internal
#define MODEM_EXTERNAL_PROP        "ro.modem.external.enable"

/* sim card num property */
#define TD_SIM_NUM_PROP             "ro.modem.t.count"
#define W_SIM_NUM_PROP                  "ro.modem.w.count"
#define LF_SIM_NUM_PROP            "ro.modem.lf.count"
#define TL_SIM_NUM_PROP             "ro.modem.tl.count"
#define LTE_SIM_NUM_PROP             "ro.modem.l.count"

/* stty interface property */
#define TD_TTY_DEV_PROP             "ro.modem.t.tty"
#define W_TTY_DEV_PROP              "ro.modem.w.tty"
#define LTE_TTY_DEV_PROP            "ro.modem.l.tty"
#define TL_TTY_DEV_PROP             "ro.modem.tl.tty"
#define LF_TTY_DEV_PROP            "ro.modem.lf.tty"

#define TD_PROC_PROP                "ro.modem.t.dev"
#define W_PROC_PROP                 "ro.modem.w.dev"
#define LTE_PROC_PROP               "ro.modem.l.dev"
#define LF_PROC_PROP                "ro.modem.lf.dev"
#define TL_PROC_PROP                "ro.modem.tl.dev"

#define DEFAULT_TD_PROC_DEV         "/proc/cpt/"
#define DEFAULT_W_PROC_DEV          "/proc/cpw/"
#define DEFAULT_LTE_PROC_DEV        "/proc/cpl/"
#define DEFAULT_LF_PROC_DEV         "/proc/cplf/"
#define DEFAULT_TL_PROC_DEV         "/proc/cptl/"

#define MODEM_BANK               "modem"
#define DSP_BANK                 "dsp"
#define TGDSP_BANK               "tgdsp"
#define LDSP_BANK                "ldsp"
#define WARM_BANK                "warm"
#define MODEM_START              "start"
#define MODEM_STOP               "stop"

/* load modem image interface */
#define TD_MODEM_BANK               "/proc/cpt/modem"
#define TD_DSP_BANK                 "/proc/cpt/dsp"
#define TD_MODEM_START              "/proc/cpt/start"
#define TD_MODEM_STOP               "/proc/cpt/stop"
#define W_MODEM_BANK                "/proc/cpw/modem"
#define W_DSP_BANK                  "/proc/cpw/dsp"
#define W_MODEM_START               "/proc/cpw/start"
#define W_MODEM_STOP                "/proc/cpw/stop"

/* modem/dsp partition */
#define TD_MODEM_SIZE               (10*1024*1024)
#define TD_DSP_SIZE                 (5*1024*1024)
#define W_MODEM_SIZE                (10*1024*1024)
#define W_DSP_SIZE                  (5*1024*1024)

#define L_MODEM_SIZE               (0xbb0000)
#define L_TGDSP_SIZE               (0x2e0000)
#define L_LDSP_SIZE                (0x1c0000)
#define L_WARM_SIZE                (0x280000)

#define LF_MODEM_SIZE               (0x9b0000)
#define LF_TGDSP_SIZE               (0x2e0000)
#define LF_LDSP_SIZE                (0x1c0000)
#define LF_WARM_SIZE                (0x280000)

#define TL_MODEM_SIZE               (0x8b0000)
#define TL_TGDSP_SIZE               (0x2e0000)
#define TL_LDSP_SIZE                (0x1c0000)

/* detect assert/hangup interface */
#define TD_ASSERT_PROP               "ro.modem.t.assert"
#define W_ASSERT_PROP                "ro.modem.w.assert"
#define L_ASSERT_PROP                "ro.modem.l.assert"
#define TD_LOOP_PROP                 "ro.modem.t.loop"
#define W_LOOP_PROP                  "ro.modem.w.loop"
#define TL_ASSERT_PROP               "ro.modem.tl.assert"
#define LF_ASSERT_PROP              "ro.modem.lf.assert"
#define TL_LOOP_PROP                 "ro.modem.tl.loop"
#define LF_LOOP_PROP                "ro.modem.lf.loop"

#define L_LOOP_PROP                  "ro.modem.l.loop"


/* default value for detect assert/hangup interface */
#define DEFAULT_TD_ASSERT_DEV         "/dev/spipe_td2"
#define DEFAULT_W_ASSERT_DEV         "/dev/spipe_w2"

/*two spipe_lte2?? */
#define DEFAULT_TL_ASSERT_DEV         "/dev/spipe_lte2"
#define DEFAULT_LF_ASSERT_DEV         "/dev/spipe_lte2"
#define DEFAULT_L_ASSERT_DEV         "/dev/spipe_lte2"

#define DEFAULT_TD_LOOP_DEV         "/dev/spipe_td0"
#define DEFAULT_W_LOOP_DEV         "/dev/spipe_w0"
/*two spipe_lte0?? */
#define DEFAULT_TL_LOOP_DEV         "/dev/spipe_lte0"
#define DEFAULT_LF_LOOP_DEV         "/dev/spipe_lte0"
#define DEFAULT_L_LOOP_DEV         "/dev/spipe_l0"

#define TD_WATCHDOG_DEV              "/proc/cpt/wdtirq"
#define W_WATCHDOG_DEV               "/proc/cpw/wdtirq"
#define TL_WATCHDOG_DEV              "/proc/cptl/wdtirq"
#define LF_WATCHDOG_DEV               "/proc/cptl/wdtirq"
#define L_WATCHDOG_DEV               "/proc/cptl/wdtirq"

#define TTY_DEV_PROP             "persist.ttydev"
#define PHONE_APP_PROP             "sys.phone.app"
#define MODEMRESET_PROPERTY          "persist.sys.sprd.modemreset"
#define MODEM_RESET_PROP             MODEMRESET_PROPERTY
#define LTE_MODEM_START_PROP         "ril.service.l.enable"
#define SSDA_MODE_PROP               "persist.radio.ssda.mode"
#define SSDA_TESTMODE_PROP           "persist.radio.ssda.testmode"
#define SVLTE_MODE                   "svlte"

#define PHONE_APP             "com.android.phone"
#define MODEM_SOCKET_NAME         "modemd"
#define PHSTD_SOCKET_NAME         "phstd"
#define PHSW_SOCKET_NAME          "phsw"
#define PHSLTE_SOCKET_NAME        "phsl"
#define PHSTL_SOCKET_NAME         "phstl"
#define PHSLF_SOCKET_NAME         "phslf"
#define MODEM_ENGCTRL_NAME           "modemd_engpc"
#define MODEM_ENGCTRL_PRO            "persist.sys.engpc.disable"

#define ENGPC_REQUSETY_OPEN          "0"
#define ENGPC_REQUSETY_CLOSE         "1"
#define SSDA_TEST_MODE_SVLTE          0

#define IMEI1_CONFIG_FILE    "/productinfo/imei1.txt"
#define IMEI2_CONFIG_FILE    "/productinfo/imei2.txt"
#define IMEI3_CONFIG_FILE    "/productinfo/imei3.txt"
#define IMEI4_CONFIG_FILE    "/productinfo/imei4.txt"
#define MAX_IMEI_STR_LENGTH  15
#define IMEI_NUM             4

#define PERSIST_MODEM_CHAR        "persist.modem."


typedef enum {
    LTE_MODEM_DEFAULT = -1,
    LTE_MODEM_OFF,
    LTE_MODEM_ON
}LTE_MODEM_START_E;


extern int  client_fd[];

int stop_service(int modem, int is_vlx);
int start_service(int modem, int is_vlx, int restart);

int write_proc_file(char *file, int offset, char *string);
int vlx_reboot_init(void);
int detect_vlx_modem(int modem);
void start_modem(int *param);

void* detect_sipc_modem(void *param);
void* detect_modem_blocked(void *param);

extern void exit_modemd(void);
int open_modem_dev(char *path);
int wait_for_alive(int modem, int is_assert);
#endif
