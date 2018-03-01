#include "testitem.h"



int x_pass, y_pass, z_pass;
static int thread_run;
static char device_info[32] = {0};

static int gsensor_open(void)
{
    int fd;
    int enable = 1;

    fd = open(SPRD_GSENSOR_DEV, O_RDWR);
    LOGD("Open %s fd:%d", SPRD_GSENSOR_DEV, fd);
    if (ioctl(fd, GSENSOR_IOCTL_SET_ENABLE, &enable)) {
        LOGD("Set G-sensor enable error: %s", strerror(errno));
        close(fd);
        return -1;
    }
    return fd;
}

static int gsensor_close(int fd)
{
    int enable = 0;
	if(fd > 0) {
		if (ioctl(fd, GSENSOR_IOCTL_SET_ENABLE, &enable)) {
			LOGD("Set G-sensor disable error: %s", strerror(errno));
		}
		close(fd);
	}
    return 0;
}

static int gsensor_get_devinfo(int fd, char * devinfo)
{
    if (fd < 0 || devinfo == NULL)
        return -1;

    if (ioctl(fd, LIS3DH_ACC_IOCTL_GET_CHIP_ID, devinfo)) {
		LOGD("[%s]: Get device info error. %s]", __func__, strerror(errno));
    }
	return 0;
}

static int gsensor_get_data(int fd, int *data)
{
    int tmp[3];
    int ret;

    ret = ioctl(fd, GSENSOR_IOCTL_GET_XYZ, tmp);
    data[0] = tmp[0];
    data[1] = tmp[1];
    data[2] = tmp[2];
    return ret;
}

static int gsensor_check(int data)
{
	int ret = -1;
	int start_1 = SPRD_GSENSOR_1G-SPRD_GSENSOR_OFFSET;
	int end_1 = SPRD_GSENSOR_1G+SPRD_GSENSOR_OFFSET;
	int start_2 = -SPRD_GSENSOR_1G-SPRD_GSENSOR_OFFSET;
	int end_2 = -SPRD_GSENSOR_1G+SPRD_GSENSOR_OFFSET;

	if( ((start_1<data)&&(data<end_1))||
		((start_2<data)&&(data<end_2)) ){
		ret = 0;
	}

	return ret;
}

int gsensor_result=0;//++++++++++++
static void *gsensor_thread(void *param)
{
	time_t start_time,now_time;//++++++++++++++++++++
	int fd;
	int counter;
	int cur_row = 2;
	int x_row, y_row, z_row;
	int col = 5;
	int data[3];
	x_pass = y_pass = z_pass = 0;

	//enable gsensor
	fd = gsensor_open();
	if(fd < 0) {
		ui_set_color(CL_RED);
		cur_row = ui_show_text(cur_row, 0, TEXT_GS_OPEN_FAIL);
	}
	gsensor_get_devinfo(fd, device_info);

	ui_set_color(CL_WHITE);
	cur_row = ui_show_text(cur_row, 0, TEXT_GS_DEV_INFO);
	cur_row = ui_show_text(cur_row, 0, device_info);
	cur_row = ui_show_text(cur_row+1, 0, TEXT_GS_OPER1);
	cur_row = ui_show_text(cur_row, 0, TEXT_GS_OPER2);
	ui_set_color(CL_GREEN);
	x_row = cur_row;
	cur_row = ui_show_text(cur_row, 0, TEXT_GS_X);
	y_row = cur_row;
	cur_row = ui_show_text(cur_row, 0, TEXT_GS_Y);
	z_row = cur_row;
	cur_row = ui_show_text(cur_row, 0, TEXT_GS_Z);
    start_time=time(NULL);//++++++++++++++++++++++
	gr_flip();
	while(!(x_pass&y_pass&z_pass)) {
		//get data
		now_time=time(NULL);//++++++++++++++++++
		gsensor_get_data(fd, data);

		ui_set_color(CL_GREEN);
		if(x_pass == 0 && gsensor_check(data[0]) == 0) {
			x_pass = 1;
			LOGD("[%s] x_pass", __func__);
			ui_show_text(x_row, col, TEXT_GS_PASS);
			gr_flip();
		}

		if(y_pass == 0 && gsensor_check(data[1]) == 0) {
			y_pass = 1;
			LOGD("[%s] y_pass", __func__);
			ui_show_text(y_row, col, TEXT_GS_PASS);
			gr_flip();
		}

		if(z_pass == 0 && gsensor_check(data[2]) == 0) {
			z_pass = 1;
			LOGD("[%s] z_pass", __func__);
			ui_show_text(z_row, col, TEXT_GS_PASS);
			gr_flip();
		}
		usleep(2*1000);
		if((now_time-start_time)>=GSENSOR_TIMEOUT) break;//++++++++++++++++++++
	}

	gsensor_close(fd);

	if(x_pass&y_pass&z_pass)
	gsensor_result = RL_PASS;//++++++++++
	else
	gsensor_result = RL_FAIL;//++++++++++
	return NULL;
}

int test_gsensor_start(void)
{
	pthread_t thread;
	int ret;

	ui_fill_locked();
	ui_show_title(MENU_TEST_GSENSOR);

	thread_run = 1;
	pthread_create(&thread, NULL, (void*)gsensor_thread, NULL);
	//ui_handle_button(LEFT_BTN_NAME, NULL, RIGHT_BTN_NAME);
	//thread_run = 0;
	//ret = ui_handle_button(TEXT_PASS, TEXT_FAIL, TEXT_GOBACK);
	pthread_join(thread, NULL); /* wait "handle key" thread exit. */
	if(RL_PASS == gsensor_result)//++++++++++++
	{
		ui_set_color(CL_GREEN);//++++++++++
		ui_show_text(12, 0, TEXT_TEST_PASS);//++++++
	}
	else if(RL_FAIL == gsensor_result)//+++++++++++++++++
	{
		ui_set_color(CL_RED);//+++++++++++
		ui_show_text(12, 0, TEXT_TEST_FAIL);//+++++++++
	}
	else//+++++++++++++++
	{
		ui_set_color(CL_WHITE);//+++++++++++
		ui_show_text(12, 0, TEXT_TEST_NA);//+++++++++++
	}
	gr_flip();
	save_result(CASE_TEST_GSENSOR,gsensor_result);
	return gsensor_result;//++++++++++++++++
}

