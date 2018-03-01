#include "testitem.h"

static int en_lcd_backlight(int level)
{
	int fd;
	int ret;
	char buf[8];

	fd = open(LCD_BACKLIGHT_DEV, O_RDWR);
	if(fd < -1) {
		LOGD("[%s]: open %s fail", __FUNCTION__, LCD_BACKLIGHT_DEV);
		return -1;
	}

	memset(buf, 0, sizeof(buf));
	sprintf(buf, "%d", level);
	ret = write(fd, buf, strlen(buf));

	close(fd);

	return 0;
}

static int max_lcd_backlight(void)
{

	int fd;
	int ret;
	int max = 0;
	char buf[8];

	fd = open(LCD_BACKLIGHT_MAX_DEV, O_RDONLY);
	if(fd < -1) {
		LOGD("[%s]: open %s fail", __FUNCTION__, LCD_BACKLIGHT_MAX_DEV);
		return -1;
	} else {
		memset(buf, 0, sizeof(buf));
		ret = read(fd, buf, sizeof(buf));
		if(ret <= 0) {
			LOGD("[%s]: read fail, ret=%d", __FUNCTION__, ret);
			max = 0;
		} else {
			max = atoi(buf);
		}
	}

	close(fd);
	return max;
}


static int en_key_backlight(int level)
{
	int fd;
	int ret;
	char buf[8];

	fd = open(KEY_BACKLIGHT_DEV, O_RDWR);
	if(fd < -1) {
		LOGD("[%s]: open %s fail", __FUNCTION__, LCD_BACKLIGHT_DEV);
		return -1;
	}

	memset(buf, 0, sizeof(buf));
	sprintf(buf, "%d", level);
	ret = write(fd, buf, strlen(buf));

	close(fd);

	return 0;
}


static int max_key_backlight(void)
{

	int fd;
	int ret;
	int max = 0;
	char buf[8];

	fd = open(KEY_BACKLIGHT_MAX_DEV, O_RDONLY);
	if(fd < -1) {
		LOGD("[%s]: open %s fail", __FUNCTION__, LCD_BACKLIGHT_MAX_DEV);
		return -1;
	} else {
		memset(buf, 0, sizeof(buf));
		ret = read(fd, buf, sizeof(buf));
		if(ret <= 0) {
			LOGD("[%s]: read fail, ret=%d", __FUNCTION__, ret);
			max = 0;
		} else {
			max = atoi(buf);
		}
	}

	close(fd);
	return max;
}

int test_backlight_start(void)
{
	int i = 0;
	int ret = 0;
	int key_max = max_key_backlight();
	int lcd_max = max_lcd_backlight();
	LOGD("[%s]: key_max=%d, lcd_max=%d\n", __FUNCTION__, key_max, lcd_max);
	ui_fill_locked();
	ui_show_title(MENU_TEST_BACKLIGHT);
	ui_show_text(3, 0, TEXT_BL_ILLUSTRATE);
	gr_flip();

	for(i = 0; i < 5; i++) {
		en_lcd_backlight(lcd_max>>i);
		en_key_backlight(key_max>>i);
		usleep(500*1000);
	}
	en_lcd_backlight(lcd_max);
	en_key_backlight(0);
    ui_set_color(CL_GREEN);
    ui_show_text(5, 0, TEXT_BL_OVER);
    gr_flip();
	ret = ui_handle_button(NULL, NULL);//, TEXT_GOBACK
	return ret;
}

extern int enable_vibrator(int ms);
int test_vb_bl_start(void)
{
	int i = 0;
	int ret = 0;
	int key_max = max_key_backlight();
	int lcd_max = max_lcd_backlight();
	int row = 3;
	LOGD("mmitest [%s]: key_max=%d, lcd_max=%d\n", __FUNCTION__, key_max, lcd_max);
	ui_fill_locked();
	ui_show_title(MENU_TEST_VB_BL);
	row = ui_show_text(row, 0, TEXT_BL_ILLUSTRATE);
	row = ui_show_text(row, 0, TEXT_VIB_START);
	gr_flip();

	enable_vibrator(5*500);
	for(i = 0; i < 5; i++) {
		en_lcd_backlight(lcd_max>>i);
		en_key_backlight(key_max>>i);
		usleep(500*1000);
	}
	ui_set_color(CL_GREEN);
	row = ui_show_text(row, 0, TEXT_VIB_FINISH);
    gr_flip();
	en_lcd_backlight(lcd_max);
	en_key_backlight(0);
	ret = ui_handle_button(NULL,NULL);//, TEXT_GOBACK

	save_result(CASE_TEST_BACKLIGHT,ret);
	save_result(CASE_TEST_VIBRATOR,ret);
	return ret;
}
