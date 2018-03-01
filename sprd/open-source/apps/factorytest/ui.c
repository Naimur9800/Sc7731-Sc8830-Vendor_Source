/*
 * Copyright (C) 2007 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include <linux/input.h>
#include <pthread.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/reboot.h>
#include <sys/time.h>
#include <time.h>
#include <unistd.h>
#include <linux/input.h>

#include "common.h"
#include "minui.h"
#include "ui.h"
#include "testitem.h"


static pthread_mutex_t gUpdateMutex = PTHREAD_MUTEX_INITIALIZER;
static gr_surface gBackgroundIcon[NUM_BACKGROUND_ICONS];
static gr_surface gProgressBarIndeterminate[PROGRESSBAR_INDETERMINATE_STATES];
static gr_surface gProgressBarEmpty;
static gr_surface gProgressBarFill;

static const struct { gr_surface* surface; const char *name; } BITMAPS[] = {
    { &gBackgroundIcon[BACKGROUND_ICON_INSTALLING], "icon_installing" },
    { &gBackgroundIcon[BACKGROUND_ICON_ERROR],      "icon_error" },
    { &gProgressBarIndeterminate[0],    "indeterminate1" },
    { &gProgressBarIndeterminate[1],    "indeterminate2" },
    { &gProgressBarIndeterminate[2],    "indeterminate3" },
    { &gProgressBarIndeterminate[3],    "indeterminate4" },
    { &gProgressBarIndeterminate[4],    "indeterminate5" },
    { &gProgressBarIndeterminate[5],    "indeterminate6" },
    { &gProgressBarEmpty,               "progress_empty" },
    { &gProgressBarFill,                "progress_fill" },
    { NULL,                             NULL },
};

gr_surface gCurrentIcon = NULL;

static enum ProgressBarType {
    PROGRESSBAR_TYPE_NONE,
    PROGRESSBAR_TYPE_INDETERMINATE,
    PROGRESSBAR_TYPE_NORMAL,
} gProgressBarType = PROGRESSBAR_TYPE_NONE;

// Progress bar scope of current operation
static float gProgressScopeStart = 0, gProgressScopeSize = 0, gProgress = 0;
static time_t gProgressScopeTime, gProgressScopeDuration;

// Set to 1 when both graphics pages are the same (except for the progress bar)
static int gPagesIdentical = 0;

// Log text overlay, displayed when a magic key is pressed
static char text[MAX_ROWS][MAX_COLS];
int text_cols = 0, text_rows = 0;
static int text_col = 0, text_row = 0, text_top = 0;
static int show_text = 0;

static char menu[MAX_ROWS][MAX_COLS];
static char menu_title[MAX_ROWS];
static int show_menu = 0;
static int menu_top = 0, menu_items = 0, menu_sel = 0;
unsigned char menu_change=0;
static unsigned char menu_change_last=0;


// Key event input queue
static pthread_mutex_t key_queue_mutex = PTHREAD_MUTEX_INITIALIZER;
static pthread_cond_t key_queue_cond = PTHREAD_COND_INITIALIZER;
static int key_queue[256], key_queue_len = 0;
static volatile char key_pressed[KEY_MAX + 1];

static int _utf8_strlen(const char* str);
static int _utf8_to_clen(const char* str, int len);
int ui_start_menu(char* title, char** items, int sel,int itemnum);

// Clear the screen and draw the currently selected background icon (if any).
// Should only be called with gUpdateMutex locked.
void draw_background_locked(gr_surface icon)
{
    gPagesIdentical = 0;
    //gr_color(0, 0, 0, 255);
	ui_set_color(CL_SCREEN_BG);
    gr_fill(0, 0, gr_fb_width(), gr_fb_height());

    if (icon) {
        int iconWidth = gr_get_width(icon);
        int iconHeight = gr_get_height(icon);
        int iconX = (gr_fb_width() - iconWidth) / 2;
        int iconY = (gr_fb_height() - iconHeight) / 2;
        gr_blit(icon, 0, 0, iconWidth, iconHeight, iconX, iconY);
    }
}

// Draw the progress bar (if any) on the screen.  Does not flip pages.
// Should only be called with gUpdateMutex locked.
void draw_progress_locked()
{
    if (gProgressBarType == PROGRESSBAR_TYPE_NONE) return;

    int iconHeight = gr_get_height(gBackgroundIcon[BACKGROUND_ICON_INSTALLING]);
    int width = gr_get_width(gProgressBarEmpty);
    int height = gr_get_height(gProgressBarEmpty);

    int dx = (gr_fb_width() - width)/2;
    int dy = (3*gr_fb_height() + iconHeight - 2*height)/4;

    // Erase behind the progress bar (in case this was a progress-only update)
    gr_color(0, 0, 0, 255);
    gr_fill(dx, dy, width, height);

    if (gProgressBarType == PROGRESSBAR_TYPE_NORMAL) {
        float progress = gProgressScopeStart + gProgress * gProgressScopeSize;
        int pos = (int) (progress * width);

        if (pos > 0) {
          gr_blit(gProgressBarFill, 0, 0, pos, height, dx, dy);
        }
        if (pos < width-1) {
          gr_blit(gProgressBarEmpty, pos, 0, width-pos, height, dx+pos, dy);
        }
    }

    if (gProgressBarType == PROGRESSBAR_TYPE_INDETERMINATE) {
        static int frame = 0;
        gr_blit(gProgressBarIndeterminate[frame], 0, 0, width, height, dx, dy);
        frame = (frame + 1) % PROGRESSBAR_INDETERMINATE_STATES;
    }
}

static void draw_text_line(int row, const char* t) {
  if (t[0] != '\0') {
    gr_text(0, (row+1)*CHAR_HEIGHT-1, t);
  }
}

// Redraw everything on the screen.  Does not flip pages.
// Should only be called with gUpdateMutex locked.
static void draw_screen_locked(void)
{
    draw_background_locked(gCurrentIcon);
    draw_progress_locked();
	int menu_sel_tmp;
    if (show_text) {
        //gr_color(0, 0, 0, 160);
		ui_set_color(CL_SCREEN_BG);
        gr_fill(0, 0, gr_fb_width(), gr_fb_height());

		if(menu_sel>=(text_rows-2))
			menu_sel_tmp=menu_sel-text_rows+2;
		else
			menu_sel_tmp=menu_sel;
        int i = 0;
        if (show_menu) {
            //gr_color(64, 96, 255, 255);
			ui_show_title(menu_title);
			ui_set_color(CL_MENU_HL_BG);
            gr_fill(0, (menu_top+menu_sel_tmp) * CHAR_HEIGHT,
                    gr_fb_width(), (menu_top+menu_sel_tmp+1)*CHAR_HEIGHT+1);

			ui_set_color(CL_SCREEN_FG);
            for (; i < menu_top + menu_items; ++i) {
                if (i == menu_top + menu_sel_tmp) {
                    //gr_color(255, 255, 255, 255);
					ui_set_color(CL_MENU_HL_FG);
                    draw_text_line(i, menu[i]);
                    //gr_color(64, 96, 255, 255);
					ui_set_color(CL_SCREEN_FG);
                } else {
                    draw_text_line(i, menu[i]);
                }
            }
            gr_fill(0, i*CHAR_HEIGHT+CHAR_HEIGHT/2-1,
                    gr_fb_width(), i*CHAR_HEIGHT+CHAR_HEIGHT/2+1);
            ++i;
        }

        gr_color(255, 255, 0, 255);

        for (; i < text_rows; ++i) {
            draw_text_line(i, text[(i+text_top) % text_rows]);
        }
    }
}

// Redraw everything on the screen and flip the screen (make it visible).
// Should only be called with gUpdateMutex locked.
static void update_screen_locked(void)
{
    draw_screen_locked();
    gr_flip();
}

// Updates only the progress bar, if possible, otherwise redraws the screen.
// Should only be called with gUpdateMutex locked.
static void update_progress_locked(void)
{
    if (show_text || !gPagesIdentical) {
        draw_screen_locked();    // Must redraw the whole screen
        gPagesIdentical = 1;
    } else {
        draw_progress_locked();  // Draw only the progress bar
    }
    gr_flip();
}

// Keeps the progress bar updated, even when the process is otherwise busy.
static void *progress_thread(void *cookie)
{
    for (;;) {
        usleep(1000000 / PROGRESSBAR_INDETERMINATE_FPS);
        pthread_mutex_lock(&gUpdateMutex);

        // update the progress bar animation, if active
        // skip this if we have a text overlay (too expensive to update)
        if (gProgressBarType == PROGRESSBAR_TYPE_INDETERMINATE && !show_text) {
            update_progress_locked();
        }

        // move the progress bar forward on timed intervals, if configured
        int duration = gProgressScopeDuration;
        if (gProgressBarType == PROGRESSBAR_TYPE_NORMAL && duration > 0) {
            int elapsed = time(NULL) - gProgressScopeTime;
            float progress = 1.0 * elapsed / duration;
            if (progress > 1.0) progress = 1.0;
            if (progress > gProgress) {
                gProgress = progress;
                update_progress_locked();
            }
        }

        pthread_mutex_unlock(&gUpdateMutex);
    }
    return NULL;
}

extern void touch_handle_input(int, struct input_event*);
// Reads input events, handles special hot keys, and adds to the key queue.
static void *input_thread(void *cookie)
{
    int rel_sum = 0;
    int fake_key = 0;
	int fd = -1;
    for (;;) {
        // wait for the next key event
        struct input_event ev;
        do {
            fd = ev_get(&ev, 0);
			LOGD("eventtype %d,eventcode %d,eventvalue %d",ev.type,ev.code,ev.value);
			if(fd != -1) {
				touch_handle_input(fd, &ev);
			}
            LOGD("eventtype %d,eventcode %d,eventvalue %d",ev.type,ev.code,ev.value);
            if (ev.type == EV_SYN) {
                continue;
            } else if (ev.type == EV_REL) {
                if (ev.code == REL_Y) {
                    // accumulate the up or down motion reported by
                    // the trackball.  When it exceeds a threshold
                    // (positive or negative), fake an up/down
                    // key event.
                    rel_sum += ev.value;
                    if (rel_sum > 3) {
                        fake_key = 1;
                        ev.type = EV_KEY;
                        ev.code = KEY_DOWN;
                        ev.value = 1;
                        rel_sum = 0;
                    } else if (rel_sum < -3) {
                        fake_key = 1;
                        ev.type = EV_KEY;
                        ev.code = KEY_UP;
                        ev.value = 1;
                        rel_sum = 0;
                    }
                }
            } else {
                rel_sum = 0;
            }
        }
		while (ev.type != EV_KEY || ev.code > KEY_MAX);
        pthread_mutex_lock(&key_queue_mutex);
        if (!fake_key) {
            // our "fake" keys only report a key-down event (no
            // key-up), so don't record them in the key_pressed
            // table.
            key_pressed[ev.code] = ev.value;
			LOGD("%s: %d\n",__FUNCTION__, ev.value);
        }
        fake_key = 0;
        const int queue_max = sizeof(key_queue) / sizeof(key_queue[0]);
        if (ev.value > 0 && key_queue_len < queue_max) {
            key_queue[key_queue_len++] = ev.code;
            pthread_cond_signal(&key_queue_cond);
        }
        pthread_mutex_unlock(&key_queue_mutex);

    }
    return NULL;
}

void ui_push_result(int result)
{
	pthread_mutex_lock(&key_queue_mutex);
	const int queue_max = sizeof(key_queue) / sizeof(key_queue[0]);
	if (key_queue_len < queue_max) {
		switch(result) {
			//case 0: //cancel
			//	key_queue[key_queue_len++] = KEY_BACK;
			//	break;
			case 1: //fail
				key_queue[key_queue_len++] = KEY_POWER;
				break;
			case 2: //ok
				key_queue[key_queue_len++] = KEY_VOLUMEDOWN;
				break;
			default: //cancel
			//	key_queue[key_queue_len++] = KEY_BACK;
				break;
		}
		pthread_cond_signal(&key_queue_cond);
	}
	pthread_mutex_unlock(&key_queue_mutex);
}

void ui_init(void)
{
    gr_init();
    ev_init();
    text_col = text_row = 0;
    text_rows = gr_fb_height() / CHAR_HEIGHT;
	LOGD("mmitest fb_h=%d, rows=%d\n", gr_fb_height(), text_rows);
    if (text_rows > MAX_ROWS) text_rows = MAX_ROWS;
    text_top = 1;

    text_cols = gr_fb_width() / CHAR_WIDTH;
	LOGD("mmitest fb_w=%d, cols=%d\n", gr_fb_width(), text_cols);
    if (text_cols > MAX_COLS - 1) text_cols = MAX_COLS - 1;

    pthread_t t;
    pthread_create(&t, NULL, progress_thread, NULL);
    pthread_create(&t, NULL, input_thread, NULL);
}

void ui_set_background(int icon)
{
    pthread_mutex_lock(&gUpdateMutex);
    gCurrentIcon = gBackgroundIcon[icon];
    update_screen_locked();
    pthread_mutex_unlock(&gUpdateMutex);
}

void ui_show_indeterminate_progress()
{
    pthread_mutex_lock(&gUpdateMutex);
    if (gProgressBarType != PROGRESSBAR_TYPE_INDETERMINATE) {
        gProgressBarType = PROGRESSBAR_TYPE_INDETERMINATE;
        update_progress_locked();
    }
    pthread_mutex_unlock(&gUpdateMutex);
}

void ui_show_progress(float portion, int seconds)
{
    pthread_mutex_lock(&gUpdateMutex);
    gProgressBarType = PROGRESSBAR_TYPE_NORMAL;
    gProgressScopeStart += gProgressScopeSize;
    gProgressScopeSize = portion;
    gProgressScopeTime = time(NULL);
    gProgressScopeDuration = seconds;
    gProgress = 0;
    update_progress_locked();
    pthread_mutex_unlock(&gUpdateMutex);
}

void ui_set_progress(float fraction)
{
    pthread_mutex_lock(&gUpdateMutex);
    if (fraction < 0.0) fraction = 0.0;
    if (fraction > 1.0) fraction = 1.0;
    if (gProgressBarType == PROGRESSBAR_TYPE_NORMAL && fraction > gProgress) {
        // Skip updates that aren't visibly different.
        int width = gr_get_width(gProgressBarIndeterminate[0]);
        float scale = width * gProgressScopeSize;
        if ((int) (gProgress * scale) != (int) (fraction * scale)) {
            gProgress = fraction;
            update_progress_locked();
        }
    }
    pthread_mutex_unlock(&gUpdateMutex);
}

void ui_reset_progress()
{
    pthread_mutex_lock(&gUpdateMutex);
    gProgressBarType = PROGRESSBAR_TYPE_NONE;
    gProgressScopeStart = gProgressScopeSize = 0;
    gProgressScopeTime = gProgressScopeDuration = 0;
    gProgress = 0;
    update_screen_locked();
    pthread_mutex_unlock(&gUpdateMutex);
}

void ui_print(const char *fmt, ...)
{
    char buf[256];
    va_list ap;
    va_start(ap, fmt);
    vsnprintf(buf, 256, fmt, ap);
    va_end(ap);

    fputs(buf, stderr);

    // This can get called before ui_init(), so be careful.
    pthread_mutex_lock(&gUpdateMutex);
    if (text_rows > 0 && text_cols > 0) {
        char *ptr;
        for (ptr = buf; *ptr != '\0'; ++ptr) {
            if (*ptr == '\n' || text_col >= text_cols) {
                text[text_row][text_col] = '\0';
                text_col = 0;
                text_row = (text_row + 1) % text_rows;
                if (text_row == text_top) text_top = (text_top + 1) % text_rows;
            }
            if (*ptr != '\n') text[text_row][text_col++] = *ptr;
        }
        text[text_row][text_col] = '\0';
        update_screen_locked();
    }
    pthread_mutex_unlock(&gUpdateMutex);
}

#if 0
void ui_start_menu(char** headers, char** items) {
    int i;
    pthread_mutex_lock(&gUpdateMutex);
    if (text_rows > 0 && text_cols > 0) {
        for (i = 0; i < text_rows; ++i) {
            if (headers[i] == NULL) break;
            strncpy(menu[i], headers[i], text_cols-1);
            menu[i][text_cols-1] = '\0';
        }
        menu_top = i;
        for (; i < text_rows; ++i) {
            if (items[i-menu_top] == NULL) break;
            strncpy(menu[i], items[i-menu_top], text_cols-1);
            menu[i][text_cols-1] = '\0';
        }
        menu_items = i - menu_top;
        show_menu = 1;
        menu_sel = 0;
        update_screen_locked();
    }
    pthread_mutex_unlock(&gUpdateMutex);
}

int ui_start_menu(char* title, char** items, int sel) {
    int i;
    pthread_mutex_lock(&gUpdateMutex);
    if (text_rows > 0 && text_cols > 0) {
        for (i = 0; i < 2; ++i) {
            menu[i][0] = '\0';
        }
		memset(menu_title, 0, sizeof(menu_title));
		strncpy(menu_title, title, text_cols-1);
		menu_title[text_cols-1] = '\0';
        menu_top = i;
        for (; i < text_rows; ++i) {
            if (items[i-menu_top] == NULL) break;
            strncpy(menu[i], items[i-menu_top], text_cols-1);
            menu[i][text_cols-1] = '\0';
        }
        menu_items = i - menu_top;
		LOGD("uishow menu_items=%d text_rows=%d",menu_items, text_rows);
        show_menu = 1;
		if(sel >= 0 && sel <= menu_items)
			menu_sel = sel;
		else
			menu_sel = 0;
        update_screen_locked();
        //ui_show_title(menu_title);
        //gr_flip();
    }
    pthread_mutex_unlock(&gUpdateMutex);
	return menu_sel;
}
#endif

#if 1
int ui_start_menu(char* title, char** items, int sel,int itemnum) 
{
    int i;
    pthread_mutex_lock(&gUpdateMutex);
    LOGD("mmitest text_rows=%d,text_cols=%d \n",text_rows,text_cols);
    if (text_rows > 0 && text_cols > 0) 
    {
        for (i = 0; i < 2; ++i) 
	{
            menu[i][0] = '\0';
        }
		memset(menu_title, 0, sizeof(menu_title));
		strncpy(menu_title, title, text_cols-1);
		menu_title[text_cols-1] = '\0';
		menu_top = i;
		if(menu_change==0||menu_change==1)
			for (i=2; i < text_rows; ++i) {
	            if (items[i-menu_top] == NULL) break;
	            LOGD("mmitest strncpy1");
		    strncpy(menu[i], items[i-menu_top], text_cols-1);
	            menu[i][text_cols-1] = '\0';
	        }
		else if(menu_change==2)
			for (i=2; i < itemnum+4-text_rows; ++i) 
			{
			     LOGD("mmitest strncpy2");
			     strncpy(menu[i], items[itemnum+i-3], text_cols-1);
			     menu[i][text_cols-1] = '\0';
			     LOGD("uishow itemnum=%d,text_rows=%d,i=%d",itemnum,text_rows,i);
			}
		menu_items = i - menu_top;
		LOGD("uishow menu_items=%d text_rows=%d",menu_items, text_rows);

		show_menu = 1;
		if(itemnum>(text_rows-2))
		{
			if(sel >= 0 )//&& sel <= menu_items
				menu_sel = sel;
			else
				menu_sel = 0;
		}
		else
		{
			if(sel >= 0 && sel <= menu_items)//
				menu_sel = sel;
			else
				menu_sel = 0;
		}
        update_screen_locked();
    }
    pthread_mutex_unlock(&gUpdateMutex);
    return menu_sel;
}

#else
int ui_start_menu(char* title, char** items, int sel,int itemnum) {
    int i;
    pthread_mutex_lock(&gUpdateMutex);
    if (text_rows > 0 && text_cols > 0) {
        for (i = 0; i < 2; ++i) {
            menu[i][0] = '\0';
        }
		memset(menu_title, 0, sizeof(menu_title));
		strncpy(menu_title, title, text_cols-1);
		menu_title[text_cols-1] = '\0';
        menu_top = i;
        for (; i < text_rows; ++i) {
            if (items[i-menu_top] == NULL) break;
            strncpy(menu[i], items[i-menu_top], text_cols-1);
            menu[i][text_cols-1] = '\0';
        }
        menu_items = i - menu_top;
        show_menu = 1;
		if(sel >= 0 && sel <= menu_items)
			menu_sel = sel;
		else 
			menu_sel = 0;
        update_screen_locked();
        //ui_show_title(menu_title);
        //gr_flip();
    }
    pthread_mutex_unlock(&gUpdateMutex);
	return menu_sel;
}
#endif

int ui_menu_select(int sel) {
    int old_sel;
	LOGD("%s: %d\n",__FUNCTION__, sel);
    pthread_mutex_lock(&gUpdateMutex);
    if (show_menu > 0) {
        old_sel = menu_sel;
        menu_sel = sel;
        if (menu_sel < 0) menu_sel = menu_items-1;
        if (menu_sel >= menu_items) menu_sel = 0;
        sel = menu_sel;
        if (menu_sel != old_sel) update_screen_locked();
    }
    pthread_mutex_unlock(&gUpdateMutex);
    return sel;
}

int ui_update_menu(int sel,int itemnum) {
    int old_sel;
	LOGD("%s: %d\n",__FUNCTION__, sel);
    pthread_mutex_lock(&gUpdateMutex);
    if (show_menu > 0) {
        old_sel = menu_sel;
        menu_sel = sel;
		if(itemnum>(text_rows-2))
		{
			menu_sel=menu_sel;
		}
		else
		{
	        if (menu_sel < 0) menu_sel = menu_items-1;
	        if (menu_sel >= menu_items) menu_sel = 0;
		}
        sel = menu_sel;
        if (menu_sel != old_sel) {
			update_screen_locked();
			//ui_show_title(menu_title);
			//gr_flip();
		}
    }
    pthread_mutex_unlock(&gUpdateMutex);
    return sel;
}

void ui_end_menu() {
    int i;
    pthread_mutex_lock(&gUpdateMutex);
    if (show_menu > 0 && text_rows > 0 && text_cols > 0) {
        show_menu = 0;
        update_screen_locked();
    }
    pthread_mutex_unlock(&gUpdateMutex);
}

void temp_set_visible(int is_show)
{
	show_text = is_show;
}

int ui_text_visible()
{
    pthread_mutex_lock(&gUpdateMutex);
    int visible = show_text;
    pthread_mutex_unlock(&gUpdateMutex);
	LOGD("ui_text_visible %d\n",visible);
    return visible;
}

int ui_wait_key(struct timespec *ntime)
{
	int ret=0;//++++
	int key=-1;//++++
	pthread_mutex_lock(&key_queue_mutex);
    while (key_queue_len == 0) {
        ret=pthread_cond_timedwait(&key_queue_cond, &key_queue_mutex,ntime);//+++change++++
		if(ret==ETIMEDOUT)//+++++++++++
        {
            break;
        }
    }

    if(0==ret)//++++++++++++
	{
		key = key_queue[0];
        memcpy(&key_queue[0], &key_queue[1], sizeof(int) * --key_queue_len);
	}
	else
		key=ret;//++++++++++
    pthread_mutex_unlock(&key_queue_mutex);
	LOGD("[%s]: key=%d\n", __FUNCTION__, key);
    return key;
}

int ui_key_pressed(int key)
{
    // This is a volatile static array, don't bother locking
    return key_pressed[key];
}

void ui_clear_key_queue() {
    pthread_mutex_lock(&key_queue_mutex);
    key_queue_len = 0;
    pthread_mutex_unlock(&key_queue_mutex);
}


int device_handle_key(int key_code, int visible) {
	if (visible) {
		switch (key_code) {
			case KEY_VOLUMEDOWN:
				return HIGHLIGHT_DOWN;
			case KEY_VOLUMEUP:
				return SELECT_ITEM;
			case KEY_POWER:
				return GO_BACK;
			default:
				return NO_ACTION;

		}
	}

	return NO_ACTION;
}

#if 0
int get_menu_selection(char** headers, char** items, int menu_only) {
    // throw away keys pressed previously, so user doesn't
    // accidentally trigger menu items.

	ui_clear_key_queue();
	ui_start_menu(headers, items);

    int selected = 0;
    int chosen_item = -1;
    while (chosen_item < 0) {
        int key = ui_wait_key();
		LOGD("ui_wait_key is %d\n",key);

        int visible = ui_text_visible();
        int action = device_handle_key(key, visible);
		LOGD("device_handle_key %d\n",action);
        if (action < 0) {
            switch (action) {
                case HIGHLIGHT_UP:
					LOGD("HIGHLIGHT_UP");
                    --selected;
                    selected = ui_menu_select(selected);
                    break;
                case HIGHLIGHT_DOWN:
					LOGD("HIGHLIGHT_DOWN");
                    ++selected;
                    selected = ui_menu_select(selected);
                    break;
                case SELECT_ITEM:
					LOGD("SELECT_ITEM");
                    chosen_item = selected;
                    break;
                case NO_ACTION:
					LOGD("NO_ACTION");
                    break;
            }
        } else if (!menu_only) {
            chosen_item = action;
        }
    }

    ui_end_menu();
    return chosen_item;
}


int ui_show_menu(char* title, char** items, int is_root, int sel) {
	int selectedtmp=0;
	int selected = 0;
    int chosen_item = -1;
    ui_clear_key_queue();
    selected = ui_start_menu(title, items, sel);

    while (chosen_item < 0) {
        int key = ui_wait_key(NULL);
		LOGD("mmitest ui_wait_key is %d\n",key);

        int visible = ui_text_visible();
        int action = device_handle_key(key, visible);
		LOGD("mmitest device_handle_key %d\n",action);
        if (action < 0) {
            switch (action) {
                case HIGHLIGHT_DOWN:
					LOGD("mmitest HIGHLIGHT_DOWN\n");
                    ++selected;
                    selected = ui_update_menu(selected);
					LOGD("uishow selected=%d\n",selected);
                    break;
                case SELECT_ITEM:
					LOGD("mmitest SELECT_ITEM\n");
                    chosen_item = selected;
                    break;
                case GO_BACK:
					LOGD("mmitest GO_BACK\n");
					if(is_root) { break; }
					else {
						chosen_item = -1;
						goto end;
					}
                case NO_ACTION:
					LOGD("mmitest NO_ACTION");
                    break;
            }
        } /*else if (!menu_only) {
            chosen_item = action;
        }
		*/
    }

end:
    ui_end_menu();
    return chosen_item;
}
#endif



int ui_show_menu(char* title, char** items, int is_root, int sel,int itemnum)
{
     int selectedtmp=0;
     int selected = 0;
     int chosen_item = -1;
     ui_clear_key_queue();

    LOGD("mmitest uishow menu_change=%d\n",menu_change);
    selected = ui_start_menu(title, items, sel,itemnum);

    while (chosen_item < 0) {
        int key = ui_wait_key(NULL);
		LOGD("mmitest ui_wait_key is %d\n",key);

        int visible = ui_text_visible();
        int action = device_handle_key(key, visible);
		LOGD("mmitest device_handle_key %d\n",action);
        if (action < 0) {
            switch (action) {
                case HIGHLIGHT_DOWN:
					LOGD("mmitest HIGHLIGHT_DOWN\n");
					++selected;
					if(itemnum>(text_rows-2))
					{
						if(selected>=itemnum)
						{
							selected=0;
							selectedtmp=selected;
							menu_change=1;
							LOGD("uishow here1");
						}
						else if(selected>=text_rows-2)
						{
							selectedtmp=selected-text_rows+2;
							menu_change=2;
							LOGD("uishow here2");
						}
						else
						{
							selectedtmp=selected;
							menu_change=0;
							LOGD("uishow here3");
						}
						if(menu_change!=0&&menu_change!=menu_change_last)
						{
							ui_start_menu(title, items, sel,itemnum);
							LOGD("uishow here5");
							selectedtmp = ui_update_menu(selectedtmp,itemnum);
						}
						else
						{
							selectedtmp = ui_update_menu(selectedtmp,itemnum);
							LOGD("uishow here6");
						}
						menu_change_last=menu_change;
					}
					else
					{
						menu_change=0;
						LOGD("uishow here4");
						selected = ui_update_menu(selected,itemnum);
					}
					LOGD("uishow select=%d \n",selected);
                    break;
                case SELECT_ITEM:
		    LOGD("mmitest SELECT_ITEM\n");
                    chosen_item = selected;
                    break;
                case GO_BACK:
		    LOGD("mmitest GO_BACK\n");
		    if(is_root) { break; }
		    else{
			 chosen_item = -1;
			 goto end;
			}
                case NO_ACTION:
		    LOGD("mmitest NO_ACTION");
                    break;
            }
        } /*else if (!menu_only) {
            chosen_item = action;
        }
		*/
    }

end:
    ui_end_menu();
    return chosen_item;
}




void ui_show_title(const char* title)
{
	int width = gr_fb_width();
	ui_set_color(CL_TITLE_BG);
	gr_fill(0, 0, gr_fb_width(), 2*CHAR_HEIGHT);
	ui_set_color(CL_TITLE_FG);
	gr_text(0, CHAR_HEIGHT + (CHAR_HEIGHT>>1), title);
}

#if 0
void ui_show_button(
	const char* left,
	const char* centor,
	const char* right
)
{
	int y = gr_fb_height()-(CHAR_HEIGHT>>1);
	if(left) {
		gr_color(0, 255, 0, 255);
		gr_text(0, y, left);
		y -= CHAR_HEIGHT;
	}
	if(right) {
		// for utf8 word
		gr_color(255, 0, 0, 255);
		gr_text(0, y, right);
		// for en word
		//gr_text(gr_fb_width()-CHAR_WIDTH*strlen(right), gr_fb_height()-(CHAR_HEIGHT>>1), right);
	}
}
#else
void ui_show_button(//const char* centor,
	const char* left,

	const char* right
)
{
	int width = gr_fb_width();
	//ui_set_color(CL_TITLE_BG);
	//gr_fill(0, gr_fb_height()-2*CHAR_HEIGHT,
	//		gr_fb_width(), gr_fb_height());
	//gr_fill(gr_fb_width()-3*CHAR_WIDTH,7,
	//		gr_fb_width()-3*CHAR_WIDTH, 9);
	ui_set_color(CL_GREEN);
	if(left) {
		gr_text(gr_fb_width()-4*CHAR_WIDTH, PASS_POSITION, left);
	}

	//if(centor) {
	//	gr_text(gr_fb_width()/2-(CHAR_WIDTH*2), gr_fb_height()-(CHAR_HEIGHT>>1), centor);
	//}
	ui_set_color(CL_RED);
	if(right) {
		gr_text(FAIL_POSITION+(CHAR_WIDTH>>1), CHAR_HEIGHT+(CHAR_HEIGHT>>1), right);
	}
}
#endif

int ui_show_text(int row, int col, const char* text)
{
	int str_len = strlen(text);
	int utf8_len = _utf8_strlen(text);
	int clen = 0;
	int max_col = gr_fb_width() / CHAR_WIDTH;
	char temp[256];
	char* pos = text;
	int cur_row = row;

	//LOGD("[%s], len=%d, max_col=%d\n", __FUNCTION__, len, max_col);
	if(utf8_len == str_len) {
		while(str_len > 0) {
			memset(temp, 0, sizeof(temp));
			if(str_len > max_col) {
				memcpy(temp, pos, max_col);
				pos += max_col;
				str_len -= max_col;
			} else {
				memcpy(temp, pos, str_len);
				pos += str_len;
				str_len = 0;
			}
			gr_text(col*CHAR_WIDTH,
					(++cur_row)*CHAR_HEIGHT-1, temp);
		}
	} else {
		//LOGD("[%s], ut8_len=%d, max_col=%d\n", __FUNCTION__, utf8_len, max_col);
		//LOGD("[%s], clen=%d\n", __FUNCTION__, strlen(text));
		max_col -= 10; //for chinese word
		while(utf8_len > 0) {
			memset(temp, 0, sizeof(temp));
			if(utf8_len > max_col) {
				clen = _utf8_to_clen(pos, max_col);
				memcpy(temp, pos, clen);
				pos += clen;
				utf8_len -= max_col;
			} else {
				clen = _utf8_to_clen(pos, utf8_len);
				memcpy(temp, pos, clen);
				//pos += clen;
				utf8_len = 0;
			}
			//LOGD("[%s], clen=%d\n", __FUNCTION__, clen);
			gr_text(col*CHAR_WIDTH, (++cur_row)*CHAR_HEIGHT-1, temp);
		}
	}

	return cur_row;
}



void ui_fill_locked(void)
{
	draw_background_locked(gCurrentIcon);
	draw_progress_locked();
}


extern int usbin_state;
int ui_handle_button(//const char* centor,
	const char* left,

	const char* right
)
{
	int key = -1;
	if(left != NULL || right != NULL) {//centor != NULL ||
		ui_show_button(left, right);// centor,
		gr_flip();
	}
	ui_clear_key_queue();
	LOGD("mmitest waite key");
	for(;;) {
		key = ui_wait_key(NULL);
		LOGD("mmitest key=%d",key);
		if(usbin_state==1)
			{
				if(key==KEY_VOLUMEDOWN)
					key=-1;
			}
		switch(key) {
			case KEY_VOLUMEDOWN:
				LOGD("mmitest keyV solved");
                usbin_state = 0;
				return 2;
			case KEY_POWER:
				LOGD("mmitest keyP solved");
                usbin_state = 0;
				return 1;

		}
	}
	//return 0;
}

void ui_fill_screen(
	unsigned char r,
	unsigned char g,
	unsigned char b
)
{
	gr_color(r, g, b, 255);
	gr_fill(0, 0, gr_fb_width(), gr_fb_height());
}

void ui_clear_rows(int start, int n)
{
	ui_set_color(CL_SCREEN_BG);
	gr_fill(0, CHAR_HEIGHT*start, gr_fb_width(), CHAR_HEIGHT*(start+n));
}

void ui_clear_rows_cols(int row_start, int n1,int col_start,int n2)
{
	ui_set_color(CL_SCREEN_BG);
	gr_fill(CHAR_WIDTH*col_start, CHAR_HEIGHT*row_start,CHAR_WIDTH*(col_start+n2), CHAR_HEIGHT*(row_start+n1));
}


void ui_set_color(int cl)
{
	switch(cl) {
		case CL_WHITE:
			gr_color(255, 255, 255, 255);
			break;
		case CL_BLACK:
			gr_color(0, 0, 0, 255);
			break;
		case CL_RED:
			gr_color(255, 0, 0, 255);
			break;
		case CL_BLUE:
			gr_color(0, 0, 255, 255);
			break;
		case CL_GREEN:
			gr_color(0, 255, 0, 255);
			break;
		case CL_TITLE_BG:
			gr_color(62, 30, 51, 255);
			break;
		case CL_TITLE_FG:
			gr_color(235, 214, 228, 255);
			break;
		case CL_SCREEN_BG:
			gr_color(33, 16, 28, 255);
			break;
		case CL_SCREEN_FG:
			gr_color(255, 255, 255, 255);
			break;
		case CL_MENU_HL_BG:
			gr_color(201, 143, 182, 255);
			break;
		case CL_MENU_HL_FG:
			gr_color(17, 9, 14, 255);
			break;
		default:
			gr_color(0, 0, 0, 255);
			break;
	}

}

void ui_draw_line(int x1,int y1,int x2,int y2)
{
     int *xx1=&x1;
     int *yy1=&y1;
     int *xx2=&x2;
     int *yy2=&y2;

     int i=0;
     int j=0;
     int tekxx=*xx2-*xx1;
     int tekyy=*yy2-*yy1;

     if(*xx2>=*xx1)
     {
         for(i=*xx1;i<=*xx2;i++)
         {
             j=(i-*xx1)*tekyy/tekxx+*yy1;
	      gr_fill(i,j,i+1, j+1);
         }
     }
     else
     {
         for(i=*xx2;i<*xx1;i++)
         {
             j=(i-*xx2)*tekyy/tekxx+*yy2;
             gr_fill(i,j,i+1, j+1);
         }
     }
}

void ui_draw_line_mid(int x1,int y1,int x2,int y2)
{
	 int *xx1=&x1;
     int *yy1=&y1;
     int *xx2=&x2;
     int *yy2=&y2;

     int i=0;
     int j=0;
     int tekxx=*xx2-*xx1;
     int tekyy=*yy2-*yy1;

	 if(abs(tekxx) >= abs(tekyy)) {
		if(*xx2>=*xx1){
			for(i=*xx1;i<=*xx2;i++){
				j=(i-*xx1)*tekyy/tekxx+*yy1;
				if (j == *yy1 && i == *xx1)
					continue;
				gr_fill(i,j,i+1, j+1);
			}
		}
		else{
			for(i=*xx2;i<*xx1;i++){
				j=(i-*xx2)*tekyy/tekxx+*yy2;
				if (j == *yy2 && i == *xx2)
					continue;
				gr_fill(i,j,i+1, j+1);
			}
		}
	 } else {
		if(*yy2>=*yy1){
			for(j=*yy1;j<=*yy2;j++){
				i=(j-*yy1)*tekxx/tekyy+*xx1;
				if (j == *yy1 && i == *xx1)
					continue;
				gr_fill(i,j,i+1, j+1);
			}
		}
		else{
			for(j=*yy2;j<*yy1;j++){
				i=(j-*yy2)*tekxx/tekyy+*xx2;
				if (j == *yy2 && i == *xx2)
					continue;
				gr_fill(i,j,i+1, j+1);
			}
		}
	 }
 }


static int _utf8_strlen(const char* str)
{
	int i = 0;
	int count = 0;
	int len = strlen (str);
	unsigned char chr = 0;
	while (i < len)
	{
		chr = str[i];
		count++;
		i++;
		if(i >= len)
			break;

		if(chr & 0x80) {
			chr <<= 1;
			while (chr & 0x80) {
				i++;
				chr <<= 1;
			}
		}
	}
	return count;
}


static int _utf8_to_clen(const char* str, int len)
{
	int i = 0;
	int count = 0;
	//int min = strlen (str);
	unsigned char chr = 0;
	//min = min < len ? min : len;
	while (str[i]!='\0' && count < len)
	{
		chr = str[i];
		count++;
		i++;

		if(chr & 0x80) {
			chr <<= 1;
			while (chr & 0x80) {
				i++;
				chr <<= 1;
			}
		}
	}
	return i;
}
