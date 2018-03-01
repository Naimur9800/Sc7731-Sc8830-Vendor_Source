#ifndef __TEST_ITEM_H__
#define __TEST_ITEM_H__
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <pthread.h>
#include <termios.h>
#include <fcntl.h>
#include <string.h>
#include <errno.h>
#include <ctype.h>
#include <time.h>
#include <getopt.h>
#include <fcntl.h>
#include <dirent.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/sysmacros.h>
#include <sys/wait.h>
#include <sys/ioctl.h>
#include <sys/time.h>
#include <cutils/sockets.h>
#include <linux/input.h>
//#include "cutils/properties.h"

#include "minui.h"
#include "../common.h"
#include "../resource.h"
#include "../ui.h"
#include "../eng_tok.h"
#include "factorytest.h"


enum {
	RL_NA = 0,
	RL_FAIL = 1,
	RL_PASS = 2,
	RL_NS = 3,
};





#define SPRD_AUDIO_FILE	"/system/media/engtest_sample.wav"
typedef enum{
	SPEAKER,
	RECEIVER,
	HEADPHONE,
	FM_PLAY,
	PHONE
}eng_audio_type;

typedef struct eng_mode{
	eng_audio_type mode;
	int	finished;
	int phase;
	union{
		int data;
	}u;
}eng_audio_mode;

/************************/
void save_result(char id,char key_result);
int test_lcd_start(void);
int test_msensor_start(void);
int test_key_start(void);
int test_vibrator_start(void);
int test_version_show(void);
int test_phone_info_show(void);
int test_cali_info(void);
int test_backlight_start(void);
int test_vb_bl_start(void);
int test_tp_start(void);
int test_mainloopback_start(void);//+++++++++
int test_assisloopback_start(void);//++++++++++++
int test_sdcard_start(void);
int sdcard_read_fm(int *rd);
int sdcard_write_fm(int *rd);
int test_sdcard_pretest(void);
int test_gsensor_start(void);
int test_lsensor_start(void);
int test_bcamera_start(void);
int test_flash_start(void);
int flash_start(void);

int test_fcamera_start(void);
int test_charge_start(void);
int test_headset_start(void);
int test_tel_start(void);
int test_otg_start(void);
int test_fm_start(void);
void eng_bt_scan_start(void);
int test_bt_start(void);
int test_bt_pretest(void);
int eng_wifi_scan_start(void);
int test_wifi_start(void);
int test_wifi_pretest(void);
int test_gps_start(void);
int test_gps_pretest(void);
int gpsOpen(void);
int test_sim_start(void);
int test_sim_pretest(void);
int test_receiver_start(void);
int test_speaker_start(void);

int test_modem_init(void);
char* test_modem_get_ver(void);
int tel_send_at(int fd, char* cmd, char* buf, int buf_len, int wait);
int modem_send_at(int fd, char* cmd, char* buf, int buf_len, int wait);
int at_cmd_audio_loop(int enable, int mode, int volume,int loopbacktype,int voiceformat,int delaytime);
int find_input_dev(int mode, const char *event_name);

#endif /*__TEST_ITEM_H_*/
