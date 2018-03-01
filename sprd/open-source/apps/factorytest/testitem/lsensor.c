#include "testitem.h"




static int thread_run;
static int proximity_value=1;
static int proximity_modifies=0;
static int light_value=0;
static int light_pass=0;

static void lsensor_show(void)
{
	int row = 3;
	char buf[64];

	ui_clear_rows(row, 2);
	if(proximity_modifies >= 2) {
		ui_set_color(CL_GREEN);
	} else {
		ui_set_color(CL_RED);
	}

	if(proximity_value == 0){
		row = ui_show_text(row, 0, TEXT_PS_NEAR);
	} else {
		row = ui_show_text(row, 0, TEXT_PS_FAR);
	}

	memset(buf, 0, sizeof(buf));
	sprintf(buf, "%s %d", TEXT_LS_LUX, light_value);

	if(light_pass == 1) {
		ui_set_color(CL_GREEN);
	} else {
		ui_set_color(CL_RED);
	}
	ui_show_text(row, 0, buf);
	gr_flip();
}

static int lsensor_enable(int enable)
{
	int fd;
	int ret = -1;
	fd = open(SPRD_PLS_CTL, O_RDWR);
	if(fd < 0) {
		LOGD("[%s]:open %s fail\n", __FUNCTION__, SPRD_PLS_CTL);
		return -1;
	}

	if(fd > 0) {
		if(ioctl(fd, LTR_IOCTL_SET_LFLAG, &enable) < 0) {
			LOGD("[%s]:set lflag %d fail, err:%s\n", __FUNCTION__, enable, strerror(errno));
			ret = -1;
		}
		if(ioctl(fd, LTR_IOCTL_SET_PFLAG, &enable) < 0) {
			LOGD("[%s]:set pflag %d fail, err:%s\n", __FUNCTION__, enable, strerror(errno));
			ret = -1;
		}
		close(fd);
	}

	return ret;
}

static void *lsensor_thread(void *param)
{
	int fd = -1;
	fd_set rfds;
	int cur_row = 3;
	time_t start_time,now_time;
	struct input_event ev;
	struct timeval timeout;
	int ret;
	int count=0;
	LOGD("mmitest lsensor=%s\n",SPRD_PLS_INPUT_DEV);
	fd = find_input_dev(O_RDONLY, SPRD_PLS_INPUT_DEV);
	if(fd < 0) {
		ui_set_color(CL_RED);
		ui_show_text(cur_row, 0, TEXT_OPEN_DEV_FAIL);
		gr_flip();
		return NULL;
	}

	lsensor_enable(1);
	//start_time=time(NULL);
	while(thread_run == 1) {
		//now_time=time(NULL);
		FD_ZERO(&rfds);
		FD_SET(fd, &rfds);
		timeout.tv_sec = 1;
		timeout.tv_usec = 0;
		ret = select(fd+1, &rfds, NULL, NULL, &timeout);
		if(FD_ISSET(fd, &rfds)) {
			ret = read(fd, &ev, sizeof(ev));
			if(ret == sizeof(ev)){
				if(ev.type == EV_ABS) {
					switch(ev.code){
						case ABS_DISTANCE:
							proximity_modifies++;
							proximity_value = ev.value;
							LOGD("P:%d\n", ev.value);
							lsensor_show();
							break;
						case ABS_MISC:
							LOGD("L:%d\n", ev.value);
							if(light_value!=ev.value)
								count++;
							if(count>=2)
								light_pass = 1;
							light_value = ev.value;
							lsensor_show();
							break;
					}
				}
			}
		}

		if((light_pass == 1 && proximity_modifies > 1)) //||(now_time-start_time)>LSENSOR_TIMEOUT
		{
			ui_push_result(RL_PASS);
			ui_set_color(CL_GREEN);
			ui_show_text(5, 0, TEXT_TEST_PASS);
			gr_flip();
			goto func_end;
		}
	}
func_end:
	lsensor_enable(0);
	return NULL;
}

int test_lsensor_start(void)
{
	int ret = 0;
	pthread_t thread;
	proximity_value=1;
	proximity_modifies=0;
	light_value=0;
	light_pass=0;

	ui_fill_locked();
	ui_show_title(MENU_TEST_LSENSOR);

	lsensor_show();
	thread_run = 1;
	pthread_create(&thread, NULL, (void*)lsensor_thread, NULL);
	ret = ui_handle_button(NULL, NULL);//, TEXT_GOBACK
	thread_run = 0;
	pthread_join(thread, NULL); /* wait "handle key" thread exit. */
	save_result(CASE_TEST_LSENSOR,ret);
	return ret;
}

