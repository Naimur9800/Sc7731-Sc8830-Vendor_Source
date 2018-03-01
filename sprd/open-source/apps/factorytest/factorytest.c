#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/mount.h>
#include <sys/reboot.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <dirent.h>
#include <errno.h>
#include <pthread.h>
#include "minui.h"
#include "common.h"

#include "resource.h"
#include "testitem.h"
#include "eng_sqlite.h"


typedef struct {
    unsigned char num;
	unsigned int id;
	char* title;
	int (*func)(void);
}menu_info;



typedef struct {
	unsigned int id;
	char* name;
	int value;
}result_info;


typedef struct {
	char* name;
	unsigned int id;
	char yes_no;
	char status;
}new_result_info;




typedef struct {
	unsigned char num;
	unsigned int rid;
	int id;
	int (*func)(void);
}multi_test_info;


static int show_phone_info_menu(void);
static int show_not_auto_test_menu(void);
static int show_not_auto_test_result(void);



static int show_phone_test_menu(void);
static int show_pcba_test_menu(void);





static int auto_all_test(void);
static int PCBA_auto_all_test(void);


static int test_result_mkdir(void);
//static void write_txt(char * pathname ,int n);
static void write_bin(char * pathname );
static void read_bin(void);


static int show_pcba_test_result(void);
static int show_phone_test_result(void);

static int phone_shutdown(void);
static pthread_t bt_wifi_init_thread;
static pthread_t gps_init_thread;
static pthread_t test_tel_init_thread;
extern void temp_set_visible(int is_show);


menu_info menu_root[] = {
#define MENUTITLE(num,id,title, func) \
	[ K_ROOT_##title ] = {num,id,MENU_##title, func, },
#include "./res/menu_root.h"
#undef MENUTITLE
    [K_MENU_ROOT_CNT]={0,0,MEUN_FACTORY_RESET, phone_shutdown,},
};


menu_info menu_phone_info_testmenu[] = {
#define MENUTITLE(num,id,title, func) \
	[ B_PHONE_##title ] = {num,id,MENU_##title, func, },
#include "./res/menu_phone_info.h"
#undef MENUTITLE
	//[K_MENU_INFO_CNT] = {0,0,MENU_BACK, 0,},
};







menu_info menu_phone_test[] = {
#define MENUTITLE(num,id,title, func) \
	[ K_PHONE_##title ] = {num,id,MENU_##title, func, },
#include "./res/menu_phone_test.h"
#undef MENUTITLE
	//[K_MENU_PHONE_TEST_CNT] = {0,0,MENU_BACK, 0,},
};


menu_info menu_pcba_test[] = {
#define MENUTITLE(num,id,title, func) \
	[ P_PHONE_##title ] = {num,id,MENU_##title, func, },
#include "./res/menu_pcba_test.h"
#undef MENUTITLE
	//[P_MENU_PHONE_TEST_CNT] = {0,0,MENU_BACK, 0,},
};



menu_info menu_auto_test[] = {
#define MENUTITLE(num,id,title, func) \
	[ A_PHONE_##title ] = {num,id,MENU_##title, func, },
#include "./res/menu_auto_test.h"
#undef MENUTITLE
	//[K_MENU_AUTO_TEST_CNT] = {0,0,MENU_BACK, 0,},
};

menu_info menu_not_auto_test[] = {
#define MENUTITLE(num,id,title, func) \
	[ D_PHONE_##title ] = {num,id,MENU_##title, func, },
#include "./res/menu_not_autotest.h"
#undef MENUTITLE
	//[K_MENU_AUTO_TEST_CNT] = {0,0,MENU_BACK, 0,},
};






char sdcard_fm_state;
static int fm_freq=875;

char pcba_phone=0;

mmi_result phone_result[TOTAL_NUM];
mmi_result pcba_result[TOTAL_NUM];
mmi_result_new phone_text_result[TOTAL_NUM];
mmi_result_new pcba_text_result[TOTAL_NUM];



char name[][TOTAL_NUM]=
{
	MENU_TEST_LCD,
	MENU_TEST_TP,
	MENU_TEST_VIBRATOR,
	MENU_TEST_BACKLIGHT,
	MENU_TEST_KEY,
	MENU_TEST_FCAMERA,
	MENU_TEST_BCAMERA,
	MENU_TEST_FLASH,
	MENU_TEST_MAINLOOP,
	MENU_TEST_ASSISLOOP,
	MENU_TEST_RECEIVER,
	MENU_TEST_CHARGE,
	MENU_TEST_SDCARD,
	MENU_TEST_SIMCARD,
	MENU_TEST_HEADSET,
	MENU_TEST_FM,
	MENU_TEST_GSENSOR,
	MENU_TEST_LSENSOR,
	MENU_TEST_BT,
	MENU_TEST_WIFI,
	MENU_TEST_GPS,
	MENU_TEST_TEL,
	MENU_TEST_OTG,
};



unsigned int  case_id[TOTAL_NUM]=
{
	1,
	2,
	5,
	6,
	4,
	7,
	8,
	9,
	10,
	11,
	13,
	17,
	15,
	16,
	14,
	19,
	35,
	36,
	22,
	23,
	24,
	27,
	26,
};


void save_result(char id,char key_result)
{
	if(pcba_phone==0)
	{
		phone_result[id].pass_faild=key_result;
		phone_text_result[id].pass_faild=key_result;
	}
	else if(pcba_phone==1)
	{
		pcba_result[id].pass_faild=key_result;
		pcba_text_result[id].pass_faild=key_result;
	}
    else
    {
        pcba_result[id].pass_faild=key_result;
        pcba_text_result[id].pass_faild=key_result;
        phone_result[id].pass_faild=key_result;
        phone_text_result[id].pass_faild=key_result;
    }
}


static void read_bin(void)
{
	FILE *fd;
	int num;

	fd=fopen(PCBATXTPATH,"r+");
	fseek(fd,0,SEEK_SET);
	num=fread(&pcba_text_result,sizeof(struct mmitest_result_new),TOTAL_NUM,fd);
	fclose(fd);
	LOGD("mmitest readbin1 num=%d\n");
	fd=fopen(PHONETXTPATH,"r+");
	fseek(fd,0,SEEK_SET);
	num=fread(&phone_text_result,sizeof(struct mmitest_result_new),TOTAL_NUM,fd);
	fclose(fd);
	LOGD("mmitest readbin2 num=%d\n");
}

static void write_bin(char * pathname )
{
	FILE *fd;
	int num;

	fd=fopen(pathname,"w+");
	fseek(fd,0,SEEK_SET);
	if(pcba_phone==1)
		num=fwrite(&pcba_text_result,sizeof(struct mmitest_result_new),TOTAL_NUM,fd);
	else
		num=fwrite(&phone_text_result,sizeof(struct mmitest_result_new),TOTAL_NUM,fd);
	fclose(fd);
	LOGD("mmitest writebin num=%d\n");
}

static int show_not_auto_test_menu(void)
{
	int chosen_item = -1;
	int i = 0;
	char* items[K_MENU_NOT_AUTO_TEST_CNT+1];
	int menu_cnt = K_MENU_NOT_AUTO_TEST_CNT;
	int result = 0;
	menu_info* pmenu = menu_not_auto_test;
	pcba_phone=2;

	for(i = 0; i < menu_cnt; i++) {
		items[i] = pmenu[i].title;
	}
	items[menu_cnt] = NULL;

	while(1) {
		LOGD("mmitest back to main");
		chosen_item = ui_show_menu(MENU_NOT_AUTO_TEST, items, 0, chosen_item,K_MENU_NOT_AUTO_TEST_CNT);
		LOGD("mmitest [%s] chosen_item = %d\n", __FUNCTION__, chosen_item);
		if(chosen_item >= 0 && chosen_item < menu_cnt) {
			LOGD("mmitest [%s] select menu = <%s>\n", __FUNCTION__, pmenu[chosen_item].title);
			if(chosen_item >= K_MENU_NOT_AUTO_TEST_CNT) {
				return 0;
			}
			if(pmenu[chosen_item].func != NULL) {
				result = pmenu[chosen_item].func();
				LOGD("mmitest result=%d id=0x%08x\n", result,pmenu[chosen_item].id);
			}
			write_bin(PHONETXTPATH);
			write_bin(PCBATXTPATH);
		}
		else if (chosen_item < 0)
		{
			return 0;
		}
    }
	return 0;
}

extern unsigned char menu_change;
static int show_root_menu(void)
{
	int chosen_item = -1;
	int i = 0;
	const char* items[K_MENU_ROOT_CNT+2];
	int menu_cnt = K_MENU_ROOT_CNT+1;
	menu_info* pmenu = menu_root;
	temp_set_visible(1);

	for(i = 0; i < menu_cnt; i++) {
		items[i] = pmenu[i].title;
	}
	items[menu_cnt] = NULL;

	while(1) {
		chosen_item = ui_show_menu(MENU_TITLE_ROOT, items, 1, chosen_item,K_MENU_ROOT_CNT+1);
		LOGD("[%s] chosen_item = %d\n", __FUNCTION__, chosen_item);
		if(chosen_item >= 0 && chosen_item < menu_cnt) {
			if(pmenu[chosen_item].func != NULL) {
				LOGD("mmitest [%s] select menu = <%s>\n", __FUNCTION__, pmenu[chosen_item].title);
				pmenu[chosen_item].func();
			}
		}
		menu_change=0;
    }

	return 0;
}



static int show_phone_info_menu(void)
{
		int chosen_item = -1;
		int i = 0;
		char* items[K_MENU_INFO_CNT+1];
		int menu_cnt = K_MENU_INFO_CNT;
		int result = 0;
		menu_info* pmenu = menu_phone_info_testmenu;
		for(i = 0; i < menu_cnt; i++) {
			items[i] = pmenu[i].title;
		}
		LOGD("mmitest menu_cnt=%d\n",menu_cnt);
		items[menu_cnt] = NULL;
		while(1) {
			chosen_item = ui_show_menu(MENU_PHONE_INFO, items, 0, chosen_item,K_MENU_INFO_CNT);
			LOGD("mmitest [%s] chosen_item = %d\n", __FUNCTION__, chosen_item);
			if(chosen_item >= 0 && chosen_item < menu_cnt) {
				LOGD("mmitest [%s] select menu = <%s>\n", __FUNCTION__, pmenu[chosen_item].title);
				if(chosen_item >= K_MENU_INFO_CNT) {
					return 0;
				}
				if(pmenu[chosen_item].func != NULL) {
					result = pmenu[chosen_item].func();
				}
			}
			else if (chosen_item < 0)
			{
				return 0;
			}
        }
        return 0;
}





static int show_phone_test_menu(void)
{
	int chosen_item = -1;
	int i = 0;
	char* items[K_MENU_PHONE_TEST_CNT+1];
	int menu_cnt = K_MENU_PHONE_TEST_CNT;
	int result = 0;
	menu_info* pmenu = menu_phone_test;
	pcba_phone=0;

	for(i = 0; i < menu_cnt; i++) {
		items[i] = pmenu[i].title;
	}
	items[menu_cnt] = NULL;

	while(1) {
		LOGD("mmitest back to main");
		chosen_item = ui_show_menu(MENU_TITLE_PHONETEST, items, 0, chosen_item,K_MENU_PHONE_TEST_CNT);
		LOGD("mmitest [%s] chosen_item = %d\n", __FUNCTION__, chosen_item);
		if(chosen_item >= 0 && chosen_item < menu_cnt) {
			LOGD("mmitest [%s] select menu = <%s>\n", __FUNCTION__, pmenu[chosen_item].title);
			if(chosen_item >= K_MENU_PHONE_TEST_CNT) {
				return 0;
			}
			if(pmenu[chosen_item].func != NULL) {
				result = pmenu[chosen_item].func();
				LOGD("mmitest result=%d id=0x%08x\n", result,pmenu[chosen_item].id);
			}
			//write_txt(PHONETXTPATH,TOTAL_NUM);
			write_bin(PHONETXTPATH);
		}
		else if (chosen_item < 0)
		{
			return 0;
		}
    }
	return 0;
}



static int show_pcba_test_menu(void)
{
	int chosen_item = -1;
	int i = 0;
	char* items[P_MENU_PHONE_TEST_CNT+1];
	int menu_cnt = P_MENU_PHONE_TEST_CNT;
	int result = 0;
	unsigned char txt_flag=1;
	menu_info* pmenu = menu_pcba_test;
	int ret;
	pcba_phone=1;

	for(i = 0; i < menu_cnt; i++) {
		items[i] = pmenu[i].title;
	}
	items[menu_cnt] = NULL;

	while(1) {
		chosen_item = ui_show_menu(MENU_TITLE_PHONETEST, items, 0, chosen_item,P_MENU_PHONE_TEST_CNT);
		LOGD("mmitest [%s] chosen_item = %d\n", __FUNCTION__, chosen_item);
		if(chosen_item >= 0 && chosen_item < menu_cnt) {
			LOGD("mmitest [%s] select menu = <%s>\n", __FUNCTION__, pmenu[chosen_item].title);
			if(chosen_item >= P_MENU_PHONE_TEST_CNT) {
				return 0;
			}
			if(pmenu[chosen_item].func != NULL) {
				result = pmenu[chosen_item].func();
				LOGD("mmitest result=%d id=0x%08x\n", result,pmenu[chosen_item].id);
			}
			//LOGD("leon%s","i am here");
		//write_txt(PCBATXTPATH,TOTAL_NUM);
		write_bin(PCBATXTPATH);
		}
		else if (chosen_item < 0)
		{
			return 0;
		}
    }
	return 0;
}



#define MULTI_TEST_CNT	5
multi_test_info multi_test_item[MULTI_TEST_CNT] = {
	{10,15,A_PHONE_TEST_SDCARD, test_sdcard_pretest},
	{11,16,A_PHONE_TEST_SIMCARD, test_sim_pretest},
	{17,23,A_PHONE_TEST_WIFI, test_wifi_pretest},
	{16,22,A_PHONE_TEST_BT, test_bt_pretest},
	{18,24,A_PHONE_TEST_GPS, test_gps_pretest}
};

static void show_multi_test_result(void)
{
	int ret = RL_NA;
	int row = 3;
	char tmp[128];
	char* rl_str;
	int i;
	multi_test_info* ptest=multi_test_item;
	menu_info* pmenu = menu_auto_test;
	ui_fill_locked();
	ui_show_title(MENU_MULTI_TEST);
	gr_flip();

	for(i = 0; i < MULTI_TEST_CNT; i++) {
		ret = ptest[i].func();
		if(ret == RL_PASS) {
			ui_set_color(CL_GREEN);
			rl_str = TEXT_PASS;
		} else {
			ui_set_color(CL_RED);
			rl_str = TEXT_FAIL;
		}
		memset(tmp, 0, sizeof(tmp));
		sprintf(tmp, "%s: %s", (pmenu[ptest[i].num].title+1), rl_str);
		row = ui_show_text(row, 0, tmp);
		gr_flip();
	}

	sleep(1);
}


static int auto_all_test(void)
{
	int i = 0;
	int j = 0;
	int result = 0;
	char* rl_str;
	test_gps_init();
	test_bt_wifi_init();
	menu_info* pmenu = menu_auto_test;
	pcba_phone=0;
	for(i = 0; i < K_MENU_AUTO_TEST_CNT; i++){
		for(j = 0; j < MULTI_TEST_CNT; j++) {
			if(i == multi_test_item[j].num) {
				LOGD("mmitest break, id=%d", i);
				break;
			}
		}
		if(j < MULTI_TEST_CNT) {
			continue;
		}
		LOGD("mmitest Do, id=%d", i);
		if(pmenu[i].func != NULL) {
			result = pmenu[i].func();
		}
	}
	show_multi_test_result();
	ui_handle_button(NULL,NULL);
	LOGD("mmitest before write");
	//write_txt(PHONETXTPATH,TOTAL_NUM);
	write_bin(PHONETXTPATH);
	return 0;
}


static int PCBA_auto_all_test(void)
{
	int i = 0;
	int j = 0;
	int k = 0;
	int result = 0;
	char* rl_str;
	test_gps_init();
	test_bt_wifi_init();
	menu_info* pmenu = menu_auto_test;
	pcba_phone=1;
	for(i = 1; i < K_MENU_AUTO_TEST_CNT; i++){
		for(j = 0; j < MULTI_TEST_CNT; j++) {
			if(i == multi_test_item[j].num) {
				LOGD("mmitest break, id=%d", i);
				break;
			}
		}
		if(j < MULTI_TEST_CNT) {
			continue;
		}
		if(pmenu[i].func != NULL) {
			LOGD("mmitest Do id=%d", i);
			result = pmenu[i].func();
		}

	}
	show_multi_test_result();
	ui_handle_button(NULL,NULL);
	LOGD("mmitest before write");
	//write_txt(PCBATXTPATH,TOTAL_NUM);
	write_bin(PCBATXTPATH);
	return 0;
}





static void test_init(void)
{
	int i = 0;
	int value = -1;

	ui_init();
    LOGD("mmitest after ui_init\n");
	ui_set_background(BACKGROUND_ICON_NONE);
	LOGD("=== show test result ===\n");
	read_bin();
	for(i = 0; i < TOTAL_NUM; i++){
		phone_result[i].name=name[i];
		pcba_result[i].name=name[i];
		phone_result[i].id=case_id[i];
		pcba_result[i].id=case_id[i];
		phone_result[i].pass_faild=phone_text_result[i].pass_faild;
		pcba_result[i].pass_faild=pcba_text_result[i].pass_faild;

	}
}


extern int text_rows;
static int show_phone_test_result(void)
{
	int row = 2;
	int i = 0;
	char tmp[128];
	char* rl_str;
	char* rl_str1;
	char* rl_str2;
	//char* ptrpass=TEXT_PASS;
	char* ptrfail=TEXT_FAIL;
	char* ptrna=TEXT_NA;
	menu_info* pmenu = menu_auto_test;
	char chang_page=0;
	unsigned char change=1;
	unsigned char changelast=0;
	ui_fill_locked();
	ui_show_title(TEST_REPORT);
	if(TOTAL_NUM-2<=text_rows-2)
	{
		ui_set_color(CL_SCREEN_BG);
		gr_fill(0, 0, gr_fb_width(), gr_fb_height());
		ui_fill_locked();
		ui_show_title(TEST_REPORT);
		gr_flip();
		for(i = 0; i < TOTAL_NUM-2; i++){
			LOGD("mmitest <%d>-%s,%d\n", i, phone_result[i].name, phone_result[i].pass_faild);
			switch(phone_result[i].pass_faild) {
				case RL_NA:
					ui_set_color(CL_WHITE);
					rl_str = TEXT_NA;
					break;
				case RL_FAIL:
					ui_set_color(CL_RED);
					rl_str = TEXT_FAIL;
					break;
				case RL_PASS:
					ui_set_color(CL_GREEN);
					rl_str = TEXT_PASS;
					break;
				case RL_NS:
					ui_set_color(CL_BLUE);
					rl_str = TEXT_NS;
					break;
				default:
					ui_set_color(CL_WHITE);
					rl_str = TEXT_NA;
					break;
			}
			memset(tmp, 0, sizeof(tmp));
			sprintf(tmp, "%s:%s", (phone_result[i].name+2), rl_str);

			row = ui_show_text(row, 0, tmp);
			gr_flip();
		}
		while(ui_handle_button(NULL,NULL)!=RL_FAIL);
}

	else
	{
		do{
			if(chang_page==RL_PASS)
			{
				change=!change;
				chang_page=RL_NA;
				LOGD("show result change=%d",change);
			}
			if(change==1)
			{
			ui_set_color(CL_SCREEN_BG);
            gr_fill(0, 0, gr_fb_width(), gr_fb_height());
			ui_fill_locked();
			ui_show_title(TEST_REPORT);
			row=2;
			for(i = 0; i < text_rows-2; i++){
				LOGD("mmitest <%d>-%s,%d\n", i, phone_result[i].name, phone_result[i].pass_faild);

				switch(phone_result[i].pass_faild) {
					case RL_NA:
						ui_set_color(CL_WHITE);
						rl_str = TEXT_NA;
						break;
					case RL_FAIL:
						ui_set_color(CL_RED);
						rl_str = TEXT_FAIL;
						break;
					case RL_PASS:
						ui_set_color(CL_GREEN);
						rl_str = TEXT_PASS;
						break;
					case RL_NS:
						ui_set_color(CL_BLUE);
						rl_str = TEXT_NS;
						break;
					default:
						ui_set_color(CL_WHITE);
						rl_str = TEXT_NA;
						break;
				}
				memset(tmp, 0, sizeof(tmp));
				sprintf(tmp, "%s:%s", (phone_result[i].name+2), rl_str);
				row = ui_show_text(row, 0, tmp);
				gr_flip();
			}
			}

			else if(change==0)
			{
			row=2;
			ui_set_color(CL_SCREEN_BG);
            gr_fill(0, 0, gr_fb_width(), gr_fb_height());
			ui_fill_locked();
			ui_show_title(TEST_REPORT);
			for(i = 0; i < TOTAL_NUM+2-text_rows-2; i++){
				LOGD("mmitest <%d>-%s,%d\n", i, phone_result[text_rows-2+i].name, phone_result[text_rows-2+i].pass_faild);

				switch(phone_result[text_rows-2+i].pass_faild) {
					case RL_NA:
						ui_set_color(CL_WHITE);
						rl_str = TEXT_NA;
						break;
					case RL_FAIL:
						ui_set_color(CL_RED);
						rl_str = TEXT_FAIL;
						break;
					case RL_PASS:
						ui_set_color(CL_GREEN);
						rl_str = TEXT_PASS;
						break;
					case RL_NS:
						ui_set_color(CL_BLUE);
						rl_str = TEXT_NS;
						break;
					default:
						ui_set_color(CL_WHITE);
						rl_str = TEXT_NA;
						break;
				}
				memset(tmp, 0, sizeof(tmp));
				sprintf(tmp, "%s:%s", (phone_result[text_rows-2+i].name+2), rl_str);
				row = ui_show_text(row, 0, tmp);
				gr_flip();
			}
			}
			changelast=chang_page;
		}while((chang_page=ui_handle_button(NULL,NULL))!=RL_FAIL);
	}
	return 0;
}



static int show_pcba_test_result(void)
{
	int row = 2;
	int i = 0;
	char tmp[128];
	char* rl_str;
	char* rl_str1;
	char* rl_str2;
	//char* ptrpass=TEXT_PASS;
	char* ptrfail=TEXT_FAIL;
	char* ptrna=TEXT_NA;
	menu_info* pmenu = menu_auto_test;
	char chang_page=0;
	unsigned char change=0;
	unsigned char changelast=0;
	ui_fill_locked();
	ui_show_title(TEST_REPORT);
	if(TOTAL_NUM-2<=text_rows-2)
	{
	for(i = 1; i < TOTAL_NUM-2; i++){
		LOGD("mmitest <%d>-%s,%d\n", i, pcba_result[i].name, pcba_result[i].pass_faild);

		switch(pcba_result[i].pass_faild) {
			case RL_NA:
				ui_set_color(CL_WHITE);
				rl_str = TEXT_NA;
				break;
			case RL_FAIL:
				ui_set_color(CL_RED);
				rl_str = TEXT_FAIL;
				break;
			case RL_PASS:
				ui_set_color(CL_GREEN);
				rl_str = TEXT_PASS;
				break;
			case RL_NS:
				ui_set_color(CL_BLUE);
				rl_str = TEXT_NS;
				break;
			default:
				ui_set_color(CL_WHITE);
				rl_str = TEXT_NA;
				break;
		}
		memset(tmp, 0, sizeof(tmp));
		sprintf(tmp, "%s:%s", (pcba_result[i].name+2), rl_str);
		row = ui_show_text(row, 0, tmp);
		gr_flip();
	}

	while(ui_handle_button(NULL,NULL)!=RL_FAIL);
}
	else
	{
		do{
			if(chang_page=RL_PASS)
			{
				change=!change;
				chang_page=RL_NA;
				LOGD("show result change=%d",change);
			}
			if(change==1)
			{
			ui_set_color(CL_SCREEN_BG);
            gr_fill(0, 0, gr_fb_width(), gr_fb_height());
			ui_fill_locked();
			ui_show_title(TEST_REPORT);
			row=2;
			for(i = 1; i < text_rows-2; i++){
				LOGD("mmitest <%d>-%s,%d\n", i, pcba_result[i].name, pcba_result[i].pass_faild);

				switch(pcba_result[i].pass_faild) {
					case RL_NA:
						ui_set_color(CL_WHITE);
						rl_str = TEXT_NA;
						break;
					case RL_FAIL:
						ui_set_color(CL_RED);
						rl_str = TEXT_FAIL;
						break;
					case RL_PASS:
						ui_set_color(CL_GREEN);
						rl_str = TEXT_PASS;
						break;
					case RL_NS:
						ui_set_color(CL_BLUE);
						rl_str = TEXT_NS;
						break;
					default:
						ui_set_color(CL_WHITE);
						rl_str = TEXT_NA;
						break;
				}
				memset(tmp, 0, sizeof(tmp));
				sprintf(tmp, "%s:%s", (pcba_result[i].name+2), rl_str);
				row = ui_show_text(row, 0, tmp);
				gr_flip();
			}
			}

			else if(change==0)
			{
			row=2;
			ui_set_color(CL_SCREEN_BG);
            gr_fill(0, 0, gr_fb_width(), gr_fb_height());
			ui_fill_locked();
			ui_show_title(TEST_REPORT);
			for(i = 1; i < TOTAL_NUM+2-text_rows-2; i++){
				LOGD("mmitest <%d>-%s,%d\n", i, pcba_result[text_rows-2+i].name, pcba_result[text_rows-2+i].pass_faild);

				switch(pcba_result[text_rows-2+i].pass_faild) {
					case RL_NA:
						ui_set_color(CL_WHITE);
						rl_str = TEXT_NA;
						break;
					case RL_FAIL:
						ui_set_color(CL_RED);
						rl_str = TEXT_FAIL;
						break;
					case RL_PASS:
						ui_set_color(CL_GREEN);
						rl_str = TEXT_PASS;
						break;
					case RL_NS:
						ui_set_color(CL_BLUE);
						rl_str = TEXT_NS;
						break;
					default:
						ui_set_color(CL_WHITE);
						rl_str = TEXT_NA;
						break;
				}
				memset(tmp, 0, sizeof(tmp));
				sprintf(tmp, "%s:%s", (pcba_result[text_rows-2+i].name+2), rl_str);
				row = ui_show_text(row, 0, tmp);
				gr_flip();
			}
			}
			changelast=chang_page;
		}while((chang_page=ui_handle_button(NULL,NULL))!=RL_FAIL);
	}
	return 0;
}

static int show_not_auto_test_result(void)
{
	int row = 2;
	int i = 0;
	char tmp[128];
	char* rl_str;
	char* rl_str1;
	char* rl_str2;
	//char* ptrpass=TEXT_PASS;
	char* ptrfail=TEXT_FAIL;
	char* ptrna=TEXT_NA;
	menu_info* pmenu = menu_not_auto_test;
	char chang_page=0;
	unsigned char change=0;
	unsigned char changelast=0;
	ui_fill_locked();
	ui_show_title(TEST_REPORT);
	for(i = TOTAL_NUM-2; i < TOTAL_NUM; i++){
		LOGD("mmitest <%d>-%s,%d\n", i, pcba_result[i].name, pcba_result[i].pass_faild);

		switch(pcba_result[i].pass_faild) {
			case RL_NA:
				ui_set_color(CL_WHITE);
				rl_str = TEXT_NA;
				break;
			case RL_FAIL:
				ui_set_color(CL_RED);
				rl_str = TEXT_FAIL;
				break;
			case RL_PASS:
				ui_set_color(CL_GREEN);
				rl_str = TEXT_PASS;
				break;
			case RL_NS:
				ui_set_color(CL_BLUE);
				rl_str = TEXT_NS;
				break;
			default:
				ui_set_color(CL_WHITE);
				rl_str = TEXT_NA;
				break;
		}
		memset(tmp, 0, sizeof(tmp));
		sprintf(tmp, "%s:%s", (pcba_result[i].name+2), rl_str);
		row = ui_show_text(row, 0, tmp);
		gr_flip();
	}

	while(ui_handle_button(NULL,NULL)!=RL_FAIL);

	return 0;
}


static int phone_shutdown(void)
{
    int fp;
    char p[64]="--wipe_data\n--locale=zh_CN";
    system("mkdir /cache/recovery/");
    fp=open("/cache/recovery/command",O_WRONLY|O_CREAT,0777);
    write(fp,p,sizeof(p));
    close(fp);
    sync();
    usleep(200*1000);
#ifdef SC9630
    __reboot( LINUX_REBOOT_MAGIC1, LINUX_REBOOT_MAGIC2,LINUX_REBOOT_CMD_RESTART2 , "recovery");
#elif SP7731
    __reboot( LINUX_REBOOT_MAGIC1, LINUX_REBOOT_MAGIC2,LINUX_REBOOT_CMD_RESTART2 , "recovery");
#else
    reboot(LINUX_REBOOT_CMD_RESTART);
#endif
	return 0;
}

void eng_bt_wifi_start(void)
{
    LOGD("==== eng_bt_wifi_start ====\n");
    eng_wifi_scan_start();
    eng_bt_scan_start();
    LOGD("==== eng_bt_wifi_end ====\n");
}

int test_bt_wifi_init(void)
{
    pthread_create(&bt_wifi_init_thread, NULL, (void *)eng_bt_wifi_start, NULL);
    return 0;
}

void test_gps_open(void)
{
        int ret;
        ret = gpsOpen();
        if( ret < 0)
        {
                LOGD("%s gps open error = %d \n", __FUNCTION__,ret);
        }
        LOGD("%s gps open success \n", __FUNCTION__);
}
int test_gps_init(void)
{
	pthread_create(&gps_init_thread, NULL, (void *)test_gps_open, NULL);
	return 0;
}

int test_tel_init(void)
{
        int fd0,fd1;
#ifdef SC9630
	fd0=open("/dev/stty_lte0",O_RDWR);
	fd1=open("/dev/stty_lte3",O_RDWR);
#else
	fd0=open("/dev/stty_w0",O_RDWR);
	fd1=open("/dev/stty_w3",O_RDWR);
#endif
	sleep(2);

        if(fd0<0||fd1<0)
        {
                LOGD("mmitest tel test is faild");
                return RL_FAIL;
	}
	while(1)
	{
	     tel_send_at(fd0,"AT",NULL,0, 0);
	     tel_send_at(fd1,"AT",NULL,0, 0);
	     sleep(1);
	}
       return 0;
}
int test_tel_init1(void)
{
         pthread_create(&test_tel_init_thread, NULL, (void *)test_tel_init, NULL);
        return 0;
}

static void test_item_init(void)
{
	test_modem_init();//SIM
	test_bt_wifi_init();//BT WIFI
	test_gps_init();//GPS
	test_tel_init1();//telephone
}

//unsigned char test_buff[]="hello world";
static int test_result_mkdir(void)
{
    LOGD("mmitest make file in");
    system("chmod 0777  /productinfo");
    if(access(PCBATXTPATH,0)==-1)
    {
        system("touch  /productinfo/PCBAtest.txt");
        LOGD("mmitest make pcbatest");
    }
    if(access(PHONETXTPATH,0)==-1)
    {
        system("touch /productinfo/wholephonetest.txt");
        LOGD("mmitest make wholephonetest");
    }
    LOGD("mmitest make file out");
    return 0;
}


void sdcard_fm_init(void)
{   int empty;

	empty=check_file_exit();

	LOGD("mmitest empty=%d\n",empty);
	if(empty==0)
		sdcard_fm_state=sdcard_write_fm(&fm_freq);
	else
		sdcard_fm_state=0;
}


int main(int argc, char **argv)
{

	//while(1)  {sleep(5);LOGD("sleep Zzz\n");}
	LOGD("==== factory test start ====\n");//}
#ifndef SC9630
//	system(SPRD_TS_MODULE);
#endif
	test_result_mkdir();//+++++++++++++++
    test_init();
	test_item_init();
	sdcard_fm_init();
	show_root_menu();

	//sleep(10);//delay for start phone

	return 1;
}



