#include "testitem.h"

int at_cmd_audio_loop(int enable, int mode, int volume,int loopbacktype,int voiceformat,int delaytime)
{
    char buf[89];
    char *at_cmd = buf;
    if(volume >9) {
        volume = 9;
    }
    //AT+SPVLOOP=1,1,8,2,3,0 //mic+speaker;1,0 //mic + receiver;1,2 //headmic + head;1,4//main mic + headphone
    LOGD("mmitest junzhong [%s]:en:%d,mode:%d,vol:%d,type:%d,format:%d,delay:%d\n",
			__FUNCTION__,
            enable,mode,volume,loopbacktype,voiceformat,delaytime);

    snprintf(at_cmd, sizeof buf, "AT+SPVLOOP=%d,%d,%d,%d,%d,%d", enable,mode,volume,loopbacktype,voiceformat,delaytime);

    modem_send_at(-1,at_cmd,NULL,0,0);

    return 0;

}

int test_mainloopback_start(void)
{
	int ret = 0;
	int row = 3;

	ui_fill_locked();
	ui_show_title(MENU_TEST_MAINLOOP);
	ui_set_color(CL_WHITE);

	row = 3;
	ui_clear_rows(row, 2);
	ui_set_color(CL_WHITE);
	row = ui_show_text(row, 0, TEXT_LB_MICSPEAKER);
	gr_flip();
	at_cmd_audio_loop(0,0,0,0,0,0);
	usleep(5*1000);
	at_cmd_audio_loop(1,1,8,2,3,0);
	ret = ui_handle_button(NULL, NULL);//, TEXT_GOBACK
	at_cmd_audio_loop(0,1,8,2,3,0);

	save_result(CASE_TEST_MAINLOOP,ret);
	return ret;
}


int test_assisloopback_start(void)
{
	int ret = 0;
	int row = 3;

	ui_fill_locked();
	ui_show_title(MENU_TEST_ASSISLOOP);
	ui_clear_rows(row, 2);
	ui_set_color(CL_WHITE);
	row = ui_show_text(row, 0, TEXT_LB_MICRECEIVER);
	gr_flip();
	at_cmd_audio_loop(0,0,0,0,0,0);
	usleep(5*1000);
	at_cmd_audio_loop(1,3,8,2,3,0);
	ret = ui_handle_button(NULL, NULL);//, TEXT_GOBACK
	at_cmd_audio_loop(0,3,8,2,3,0);
	save_result(CASE_TEST_ASSISLOOP,ret);
	return ret;
}


