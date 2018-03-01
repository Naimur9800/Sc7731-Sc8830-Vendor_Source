#include "testitem.h"

extern void* playback_thread(void* t_mode);

int test_receiver_start(void)
{
	int ret = 0;
	pthread_t t1;
	eng_audio_mode   t_mode;

	t_mode.mode = RECEIVER;
	t_mode.finished = 0;

	ui_fill_locked();
	ui_show_title(MENU_TEST_RECEIVER);
	ui_set_color(CL_WHITE);
	ui_show_text(3, 0, TEXT_RECV_PLAYING);
	gr_flip();
	pthread_create(&t1, NULL, playback_thread, (void*)&t_mode);
	ret = ui_handle_button(NULL, NULL);//, TEXT_GOBACK
	t_mode.finished = 1;
	pthread_join(t1, NULL); /* wait "handle key" thread exit. */
	save_result(CASE_TEST_RECEIVER,ret);
	return ret;
}

