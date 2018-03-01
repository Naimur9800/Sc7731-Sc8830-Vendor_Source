#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include "common.h"

#define LCD_BACKLIGHT_DEV			"/sys/class/backlight/sprd_backlight/brightness"
#define LCD_BACKLIGHT_MAX_DEV		"/sys/class/backlight/sprd_backlight/max_brightness"
/* TODO fix the following definition to control keyboard backlight 
 * then add HAVE_KEYBOARD_BACKLIGHT in BoardConfig.mk to enable them
 * */
#define KEY_BACKLIGHT_DEV 			"/sys/class/leds/keyboard-backlight/brightness"
#define KEY_BACKLIGHT_MAX_DEV 		"/sys/class/leds/keyboard-backlight/max_brightness"

#define SPRD_DBG(...) LOGE(__VA_ARGS__)
static int max_lcd, max_key;

/*
 *Set LCD backlight brightness level
 */
static int eng_lcdbacklight_test(int brightness)
{
	int fd;
	int ret;
	char buffer[8];

	fd = open(LCD_BACKLIGHT_DEV, O_RDWR);

	if(fd < 0) {
		SPRD_DBG("%s: open %s fail",__func__, LCD_BACKLIGHT_DEV);
		return -1;
	}

	memset(buffer, 0, sizeof(buffer));
	sprintf(buffer, "%d", brightness);
	ret = write(fd, buffer, strlen(buffer));

	close(fd);

	return 0;
}
static int eng_lcdbacklight_get(void)
{
	int fd;
	int ret;
	char buffer[8];

	fd = open(LCD_BACKLIGHT_DEV, O_RDWR);

	if(fd < 0) {
		SPRD_DBG("%s: open %s fail",__func__, LCD_BACKLIGHT_DEV);
		return -1;
	}

	memset(buffer, 0, sizeof(buffer));
	ret = read(fd, buffer, 8);
	close(fd);
	buffer[7]='\0';
	LOGE("lcd brightness: %s\n", buffer);

	return 0;
}

/*
 *Set LED backlight brightness level
 */
#ifdef K_BACKLIGHT
static int eng_keybacklight_test(int brightness)
{
	int fd;
	int ret;
	char buffer[8];

	fd = open(KEY_BACKLIGHT_DEV, O_RDWR);

	if(fd < 0) {
		SPRD_DBG("%s: open %s fail",__func__, KEY_BACKLIGHT_DEV);
		return -1;
	}

	memset(buffer, 0, sizeof(buffer));
	sprintf(buffer, "%d", brightness);
	ret = write(fd, buffer, strlen(buffer));

	close(fd);

	return 0;
}
static int eng_keybacklight_get(void)
{
	int fd;
	int ret;
	char buffer[8];

	fd = open(KEY_BACKLIGHT_DEV, O_RDWR);

	if(fd < 0) {
		SPRD_DBG("%s: open %s fail",__func__, LCD_BACKLIGHT_DEV);
		return -1;
	}

	memset(buffer, 0, sizeof(buffer));
	ret = read(fd, buffer, 8);
	close(fd);
	buffer[7]='\0';
	LOGE("key brightness: %s \n", buffer);

	return 0;
}
#endif

void backlight_on(void)
{
	eng_lcdbacklight_test(max_lcd/2);
	eng_lcdbacklight_get();
#ifdef K_BACKLIGHT
	eng_keybacklight_test(max_key/2);
	eng_keybacklight_get();
#endif
}

void backlight_off(void)
{
	eng_lcdbacklight_test(0);
#ifdef K_BACKLIGHT
	eng_keybacklight_test(0);
#endif
}



void backlight_init(void)
{
	int i;
	int fd, ret;
	int light_on;
	char buffer[8];

	max_lcd=0;
	max_key=0;

	fd = open(LCD_BACKLIGHT_MAX_DEV, O_RDONLY);
	if(fd < 0) {
		SPRD_DBG("%s: open %s fail",__func__, LCD_BACKLIGHT_MAX_DEV);
	} else {
		memset(buffer, 0, sizeof(buffer));
		ret = read(fd, buffer, sizeof(buffer));
		max_lcd = atoi(buffer);
		if(ret < 0) {
			SPRD_DBG("%s: read %s fail",__func__, LCD_BACKLIGHT_MAX_DEV);
		}
		close(fd);
	}

#ifdef K_BACKLIGHT
	fd = open(KEY_BACKLIGHT_MAX_DEV, O_RDONLY);
	if(fd < 0) {
		SPRD_DBG("%s: open %s fail",__func__, KEY_BACKLIGHT_MAX_DEV);
	} else {
		memset(buffer, 0, sizeof(buffer));
		ret = read(fd, buffer, sizeof(buffer));
		max_key = atoi(buffer);
		if(ret < 0) {
			SPRD_DBG("%s: read %s fail",__func__, KEY_BACKLIGHT_MAX_DEV);
		}
		close(fd);
	}
#endif

	SPRD_DBG("%s max_lcd=%d; max_key=%d",__func__, max_lcd, max_key);

}

