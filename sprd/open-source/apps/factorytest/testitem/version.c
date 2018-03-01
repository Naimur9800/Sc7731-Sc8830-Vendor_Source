#include "testitem.h"
#include <stdio.h>
#include <string.h>
#include <sys/time.h>
#include <stdlib.h>
#include <unistd.h>


extern char imei_buf1[128];
extern char imei_buf2[128];

int test_version_show(void)
{
	char androidver[64];
	char tmpbuf[64];
	char sprdver[256];
	char kernelver[256];
	char* modemver;
	int fd;
	int cur_row = 3;
	int ret=0;

	ui_fill_locked();

	ui_show_title(MENU_TITLE_VERSION);
	ui_set_color(CL_WHITE);

	//get android version
	memset(androidver, 0, sizeof(androidver));
	memset(tmpbuf, 0, sizeof(tmpbuf));
	property_get(PROP_ANDROID_VER, tmpbuf, "");
	sprintf(androidver, "Android %s",tmpbuf);
	cur_row = ui_show_text(cur_row, 0, androidver);

	// get kernel version
	fd = open(PATH_LINUX_VER, O_RDONLY);
	if(fd < 0){
		LOGD("open %s fail!", PATH_LINUX_VER);
	} else {
		memset(kernelver, 0, sizeof(kernelver));
		read(fd, kernelver, sizeof(kernelver));
		cur_row = ui_show_text(cur_row+1, 0, kernelver);
	}


	modemver = test_modem_get_ver();
	cur_row = ui_show_text(cur_row+1, 0, modemver);

	gr_flip();

	while(ui_handle_button(NULL,NULL)!=RL_FAIL);

	return ret;
}

char phrase[300];
unsigned char phrash_valid=0;

static int checkPhaseCheck(void)
{
	if ((phrase[0] == '9' || phrase[0] == '5')
                && phrase[1] == '0'
                && phrase[2] == 'P'
                && phrase[3] == 'S')
    {
			phrash_valid=1;
			return 1;
    }

		LOGD("mmitest out of checkPhaseCheck");
		phrash_valid=0;
        return 0;
}


int  isAscii(unsigned char b)
{
    if ((b >= 48 && b <= 57) || (b >= 65 && b <= 90) || (b >= 97 && b <= 122))
    {
        return 1;
    }
    return 0;
}



 static char * getSn1(void) {
		char string[32];
		if ( phrash_valid== 0) 
		{
            strcpy(string,TEXT_INVALIDSN1);
			LOGD("mmitest out of sn11");
			return string;
        }
        if (!isAscii(phrase[SN1_START_INDEX])) {
            strcpy(string,TEXT_INVALIDSN1);
			LOGD("mmitest out of sn12");
			return string;
        }

        memcpy(string, phrase+SN1_START_INDEX, SP09_MAX_SN_LEN);
		LOGD("mmitest out of sn13");
        return string;
    }

static char * getSn2(void) {
		char string[32];
	   if (phrash_valid == 0) {
		   strcpy(string,TEXT_INVALIDSN2);
		   LOGD("mmitest out of sn21");
		   return string;
	   }
	   if (!isAscii(phrase[SN2_START_INDEX])) {
		   strcpy(string,TEXT_INVALIDSN2);
		   LOGD("mmitest out of sn22");
		   return string;
	   }

	   memcpy(string, phrase+SN2_START_INDEX, SP09_MAX_SN_LEN);
	   LOGD("mmitest out of sn23");
	   return string;
   }


static int isStationTest(int station) {
        unsigned char flag = 1;
        if (station < 8) {
            return (0 == ((flag << station) & phrase[TESTFLAG_START_INDEX]));
        } else if (station >= 8 && station < 16) {
            return (0 == ((flag << (station - 8)) & phrase[TESTFLAG_START_INDEX + 1]));
        }
        return 0;
    }

 static int  isStationPass(int station) {
        unsigned char flag = 1;
        if (station < 8) {
            return (0 == ((flag << station) & phrase[RESULT_START_INDEX]));
        } else if (station >= 8 && station < 16) {
            return (0 == ((flag << (station - 8)) & phrase[RESULT_START_INDEX + 1]));
        }
        return 0;
   }


static void getTestsAndResult(void ) {

		int i;
		int flag = 1;
		char testResult[64];
		char testname[32];


		if (phrash_valid == 0) {
            strcpy(testResult,TEXT_PHASE_NOTTEST);
			LOGD("mmitest out of Result1");
			ui_set_color(CL_RED);
			ui_show_text(13, 0, testResult);
			return;
        }

        if (!isAscii(phrase[STATION_START_INDEX])) {
            strcpy(testResult,TEXT_PHASE_NOTTEST);
			LOGD("mmitest out of Result2");
			ui_set_color(CL_RED);
			ui_show_text(13, 0, testResult);
			return;
        }

        for (i = 0; i < SP09_MAX_STATION_NUM; i++) {
            if (0 == phrase[STATION_START_INDEX + i * SP09_MAX_STATION_NAME_LEN]) {
                break;
            }
			memset(testname,0,sizeof(testname));
			memset(testResult,0,sizeof(testResult));
            memcpy(testname, phrase+STATION_START_INDEX + i * SP09_MAX_STATION_NAME_LEN,
                    SP09_MAX_STATION_NAME_LEN);
            if (!isStationTest(i)) {
                sprintf(testResult,"%s %s\n",testname,TEXT_PHASE_NOTTEST);
        		ui_set_color(CL_WHITE);
            } else if (isStationPass(i)) {
                sprintf(testResult,"%s %s\n",testname,TEXT_PHASE_PASS);
		        ui_set_color(CL_GREEN);
            } else {
                sprintf(testResult,"%s %s\n",testname,TEXT_PHASE_FAILED);
		        ui_set_color(CL_RED);
            }
            flag = flag << 1;
			ui_show_text(13+i, 0, testResult);
        }
		return;
    }



long get_seed()
{
    struct timeval t;
    unsigned long seed = 0;
    gettimeofday(&t, NULL);
    seed = 1000000 * t.tv_sec + t.tv_usec;
    LOGD("generate seed: %lu", seed);
    return seed;
}

/* This function is for internal test only */
void create_random_mac(unsigned char *mac)
{
    int i;

    LOGD("generate random mac");
    memset(mac, 0, MAC_LEN);

    srand(get_seed()); /* machine run time in us */
    for(i=0; i<MAC_LEN; i++) {
	mac[i] = rand() & 0xFF;
    }

    /* Set Spreadtrum 24bit OUI */
    mac[0] = 0x40;
    mac[1] = 0x45;
    mac[2] = 0xDA;
}

bool is_zero_ether_addr(const unsigned char *mac)
{
    return !(mac[0] | mac[1] | mac[2] | mac[3] | mac[4] | mac[5]);
}

bool is_file_exists(const char *file_path)
{
    return access(file_path, 0) == 0;
}

void force_replace_file(const char *dst_file_path, const char *src_file_path)
{
    FILE *f_src, *f_dst;
    char buf[100];
    int ret = -1;

    f_src = fopen(src_file_path, "r");
    if (f_src == NULL) return;
    fgets(buf, sizeof(buf), f_src);
    fclose(f_src);

    if(strcmp(src_file_path, BT_MAC_FACTORY_CONFIG_FILE_PATH) == 0)
	if (0 != access("/data/misc/bluedroid/",F_OK) ){
	    ret = mkdir("/data/misc/bluedroid/",0666);
	    if ( ret < 0) {
		LOGD("mkdir /data/misc/bluedroid/ failed.");
		return;
	    }
	}
    else
        if (0 != access("/data/misc/wifi/",F_OK) ){
            ret = mkdir("/data/misc/wifi/",0666);
            if ( ret < 0) {
                LOGD("mkdir /data/misc/wifi/ failed.");
                return;
            }
        }
    f_dst = fopen(dst_file_path, "w");
    if (f_dst == NULL) {
	LOGD("force_replace_config_file open fail.fd = %d,errno=%d, strerror(errno)=%s", f_dst, errno, strerror(errno));
	return;
    }
    fputs(buf, f_dst);
    fclose(f_dst);

    sprintf(buf, "chmod 666 %s", dst_file_path);
    system(buf);
    LOGD("force_replace_config_file: %s", buf);
}

void read_mac_from_file(const char *file_path, unsigned char *mac)
{
    FILE *f;
    unsigned char mac_src[MAC_LEN];
    char buf[20];

    f = fopen(file_path, "r");
    if (f == NULL) return;

    if (fscanf(f, "%02x:%02x:%02x:%02x:%02x:%02x", &mac_src[0], &mac_src[1], &mac_src[2], &mac_src[3], &mac_src[4], &mac_src[5]) == 6) {
	memcpy(mac, mac_src, MAC_LEN);
	sprintf(buf, "%02x:%02x:%02x:%02x:%02x:%02x", mac_src[0], mac_src[1], mac_src[2], mac_src[3], mac_src[4], mac_src[5]);
	LOGD("mac from configuration file: %s", buf);
    } else {
	memset(mac, 0, MAC_LEN);
    }

    fclose(f);
}

void write_mac_to_file(const char *file_path, const unsigned char *mac)
{
    FILE *f;
    unsigned char mac_src[MAC_LEN];
    char buf[100];
    int ret = -1;

    if(strcmp(file_path, BT_MAC_CONFIG_FILE_PATH) == 0)
	if (0 != access("/data/misc/bluedroid/",F_OK) ){
	    ret = mkdir("/data/misc/bluedroid/",0666);
	    if ( ret < 0) {
		LOGD("mkdir /data/misc/bluedroid/ failed.");
		return;
	    }
	}
    else
        if (0 != access("/data/misc/wifi/",F_OK) ){
            ret = mkdir("/data/misc/wifi/",0666);
            if ( ret < 0) {
                LOGD("mkdir /data/misc/wifi/ failed.");
                return;
            }
        }

    f = fopen(file_path, "w");
    if (f == NULL) return;

    sprintf(buf, "%02x:%02x:%02x:%02x:%02x:%02x", mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
    fputs(buf, f);
    LOGD("write mac to configuration file: %s", buf);

    fclose(f);

    sprintf(buf, "chmod 666 %s", file_path);
    system(buf);
}

void generate_bt_mac(char * mac)
{
    int fd_btaddr=0;
    int ret = -1;
    char bt_mac[MAC_LEN] = {0};
    /* if vaild mac is in configuration file, use it*/
    if(is_file_exists(BT_MAC_CONFIG_FILE_PATH)) {
	LOGD("bt configuration file exists");
	read_mac_from_file(BT_MAC_CONFIG_FILE_PATH, mac);
	if(!is_zero_ether_addr(mac)) return;
    }
    /* force replace configuration file if vaild mac is in factory configuration file*/
    if(is_file_exists(BT_MAC_FACTORY_CONFIG_FILE_PATH)) {
	LOGD("bt factory configuration file exists");
	read_mac_from_file(BT_MAC_FACTORY_CONFIG_FILE_PATH, mac);
	if(!is_zero_ether_addr(mac)) {
	    force_replace_file(BT_MAC_CONFIG_FILE_PATH, BT_MAC_FACTORY_CONFIG_FILE_PATH);
	    return;
	}
    }
    /* generate random mac and write to configuration file*/
    create_random_mac(mac);
    write_mac_to_file(BT_MAC_CONFIG_FILE_PATH, mac);
}

void generate_wifi_mac(char * mac)
{
    int fd_wifiaddr=0;
    int ret = -1;
    char wifi_mac[MAC_LEN] = {0};
    //force replace configuration file if vaild mac is in factory configuration file
    if(is_file_exists(WIFI_MAC_FACTORY_CONFIG_FILE_PATH)) {
        LOGD("wifi factory configuration file exists");
        read_mac_from_file(WIFI_MAC_FACTORY_CONFIG_FILE_PATH, mac);
        if(!is_zero_ether_addr(mac)) {
            force_replace_file(WIFI_MAC_CONFIG_FILE_PATH, WIFI_MAC_FACTORY_CONFIG_FILE_PATH);
            return;
            }
    }
    //if vaild mac is in configuration file, use it
    if(is_file_exists(WIFI_MAC_CONFIG_FILE_PATH)) {
        LOGD("wifi configuration file exists");
        read_mac_from_file(WIFI_MAC_CONFIG_FILE_PATH, mac);
        if(!is_zero_ether_addr(mac)) return;
    }
    //generate random mac and write to configuration file
    create_random_mac(mac);
    write_mac_to_file(WIFI_MAC_CONFIG_FILE_PATH, mac);
}

int test_phone_info_show(void)
{
	char info[256];
	char mac[MAC_LEN];
	char bt_addr[18];
	char wifi_addre[18];
	char *sn1;
	char *sn2;
	char *phase_check;
	char *phase_result;
	FILE *fd;
	int i;

	ui_fill_locked();

	ui_show_title(MENU_PHONE_INFO_TEST);
	ui_set_color(CL_WHITE);
	memset(info, 0, sizeof(info));
	memset(bt_addr, 0, sizeof(bt_addr));
	memset(wifi_addre,0,sizeof(wifi_addre));
	memset(phrase, 0, sizeof(phrase));

	generate_bt_mac(mac);
	sprintf(bt_addr, "%02x:%02x:%02x:%02x:%02x:%02x", mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
	LOGD("mmitest info_show Local MAC address:%s",bt_addr);

	generate_wifi_mac(mac);
	sprintf(wifi_addre, "%02x:%02x:%02x:%02x:%02x:%02x", mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
	LOGD("mmitest info_show Local MAC address:%s",wifi_addre);

	LOGD("info_show phrase");
	fd=fopen("/dev/block/platform/sdio_emmc/by-name/miscdata","r");
	fread(phrase,sizeof(char),sizeof(phrase),fd);
	fclose(fd);
	LOGD("info_show phrase over");

	if(checkPhaseCheck()==1)
		phase_check=TEXT_VALID;
	else
		phase_check=TEXT_INVALID;

	LOGD("mmitest phase_check=%s",phase_check);
	sn1=getSn1();
	LOGD("mmitest sn1=%s",sn1);
	sn2=getSn2();
	LOGD("mmitest sn2=%s",sn2);

	ui_set_color(CL_WHITE);
	ui_show_text(2, 0, TEXT_BT_MAC);
	ui_set_color(CL_GREEN);
	ui_show_text(3, 0, bt_addr);

	ui_set_color(CL_WHITE);
	ui_show_text(4, 0, TEXT_SN);
	if(strcmp(sn1, TEXT_INVALIDSN1) == 0)
	    ui_set_color(CL_RED);
	else
	    ui_set_color(CL_GREEN);
	ui_show_text(5, 0, sn1);
	if(strcmp(sn2, TEXT_INVALIDSN2) == 0)
	    ui_set_color(CL_RED);
	else
	    ui_set_color(CL_GREEN);
	ui_show_text(6, 0, sn2);

	ui_set_color(CL_WHITE);
	ui_show_text(7, 0, TEXT_IMEI);
	ui_set_color(CL_GREEN);
	ui_show_text(8, 0, imei_buf1);
	ui_show_text(9, 0, imei_buf2);

	ui_set_color(CL_WHITE);
	ui_show_text(10, 0, TEXT_WIFI_ADDR);
	ui_set_color(CL_GREEN);
	ui_show_text(11, 0, wifi_addre);

	ui_set_color(CL_WHITE);
	ui_show_text(12, 0, TEXT_PHASE_CHECK_RESULT);
	ui_set_color(CL_GREEN);
	getTestsAndResult();

    gr_flip();


	while(ui_handle_button(NULL,NULL)!=RL_FAIL);

	return 0;
}

