#include "testitem.h"


int test_tel_start(void)
{
	int cur_row=2;
	int ret,fd;

	ui_fill_locked();
	ui_show_title(MENU_TEST_TEL);
	ui_set_color(CL_GREEN);
	cur_row = ui_show_text(cur_row, 0, TEL_TEST_START);
	cur_row = ui_show_text(cur_row, 0, TEL_TEST_TIPS);
	gr_flip();

	fd=open(TEL_DEVICE_PATH,O_RDWR);
	LOGD("mmitest tel test %s",TEL_DEVICE_PATH);
	if(fd<0)
	{
		LOGD("mmitest tel test is faild");
		save_result(CASE_TEST_TEL,RL_FAIL);
		return RL_FAIL;
	}
	//open sim card
	tel_send_at(fd,"AT+SFUN=2",NULL,0, 0);
	//open protocol stack and wait 100s,if exceed more than 20s,we regard registering network fail
	ret = tel_send_at(fd,"AT+SFUN=4",NULL,0, 100);
	if(ret < 0 ){
	    ui_set_color(CL_RED);
	    ui_show_text(cur_row,0,TEL_DIAL_FAIL);
	    gr_flip();
	    sleep(1);
	    return ret;
	}
	//call 112
	ret = tel_send_at(fd, "ATD112;", NULL,NULL, 0);
	cur_row = ui_show_text(cur_row, 0, TEL_DIAL_OVER);
	usleep(200*1000);
	//open speaker
	tel_send_at(fd,"AT+SSAM=1",NULL,0,0);
	gr_flip();

	ret = ui_handle_button(NULL, NULL);
	//hang up
	tel_send_at(fd,"ATH",NULL,0, 0);
	tel_send_at(fd,"AT",NULL,0, 0);
	close(fd);

	save_result(CASE_TEST_TEL,ret);
	return ret;
}
