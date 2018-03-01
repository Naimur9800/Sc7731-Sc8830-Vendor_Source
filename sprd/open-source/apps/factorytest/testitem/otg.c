#include "testitem.h"

int test_otg_start(void)
{
    int fp;
	int cur_row=2;
	char support;
	int ret;
	ui_fill_locked();
	ui_show_title(MENU_TEST_OTG);
	ui_set_color(CL_WHITE);
	cur_row = ui_show_text(cur_row, 0, OTG_TEST_START);
	gr_flip();
    if(access(OTG_FILE_PATH,F_OK)!=-1)
	{
		fp=open(OTG_FILE_PATH,O_RDONLY);
		read(fp,&support,sizeof(char));
		close(fp);
		LOGD("mmitest cat result=%d",support);
		if(support=='1')
		{
			ret=RL_PASS;
			ui_set_color(CL_GREEN);
			cur_row = ui_show_text(cur_row, 0, OTG_SUPPORT);
		}
		else
		{
			ret=RL_NS;
			ui_set_color(CL_BLUE);
			cur_row = ui_show_text(cur_row, 0, OTG_NOT_SUPPORT);
		}
    }
	else
	{
		ret=RL_FAIL;
		ui_set_color(CL_RED);
		cur_row = ui_show_text(cur_row, 0, OTG_NOT_SUPPORT);
	}
	gr_flip();
	sleep(1);
	save_result(CASE_TEST_OTG,ret);
	return ret;
}

