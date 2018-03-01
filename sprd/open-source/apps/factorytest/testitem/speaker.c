#include "testitem.h"
#include <tinyalsa/asoundlib.h>
#include <unistd.h>



struct pcm_config eng_fm_dl = {
    .channels = 2,
    .rate = 32000,
    .period_size = 160 * 8 * 6,
    .period_count = 2,
    .format = PCM_FORMAT_S16_LE,
};

int get_card_num_by_name(const char * dest_name){

	char s[101], name[101];
	int   index = -1;
	int   len;

	len = snprintf(s, 100, "/proc/asound/%s",dest_name);
	memset(name, 0, sizeof(name));
	len = readlink(s, name, 100);
	LOGD("mmitest %s: open %s len %d",__func__, name, len);

	if(!strncmp(name,"card",4) && strlen(name) == 5){
		index = name[4] - '0';
	}
	return index;
}

void* playback_thread(void* t_mode){

	int status, ret;
	struct pcm* fm_dl = NULL;
	char* cmd = malloc(1024);
	int card_num = get_card_num_by_name(CARD_SPRDPHONE);
	eng_audio_mode * mode = (eng_audio_mode*)t_mode;

	memset(cmd, 0, 1024);

	sprintf(cmd,"tinymix -D %d 0 2;",card_num);
	LOGD("[%s]:mode=%d, finish=%d\n", __func__, mode->mode, mode->finished);
#ifdef SC9630
	sprintf(cmd+strlen(cmd),"tinymix -D %d 107 0;",card_num);
	sprintf(cmd+strlen(cmd),"tinymix -D %d 79 0;",card_num);
	sprintf(cmd+strlen(cmd),"tinymix -D %d 80 0;",card_num);
#else
	sprintf(cmd+strlen(cmd),"tinymix -D %d 92 0;",card_num);
	sprintf(cmd+strlen(cmd),"tinymix -D %d 64 0;",card_num);
	sprintf(cmd+strlen(cmd),"tinymix -D %d 65 0;",card_num);
#endif
	system(cmd);
	memset(cmd, 0, 1024);

	if(mode->mode == SPEAKER){
#ifdef SC9630
		sprintf(cmd+strlen(cmd),"tinymix -D %d 79 1;",card_num);
		sprintf(cmd+strlen(cmd),"tinymix -D %d 80 1;",card_num);
		sprintf(cmd+strlen(cmd),"tinymix -D %d 107 1;",card_num);
#else
		sprintf(cmd+strlen(cmd),"tinymix -D %d 64 1;",card_num);
		sprintf(cmd+strlen(cmd),"tinymix -D %d 65 1;",card_num);
		sprintf(cmd+strlen(cmd),"tinymix -D %d 92 1;",card_num);
#endif
		sprintf(cmd+strlen(cmd), "%s %s -D %d;","tinyplay", SPRD_AUDIO_FILE,card_num);

		LOGD("mmitestsound:cmd= %s\n",cmd);
	} else if(mode->mode == FM_PLAY){
		fm_dl = pcm_open(card_num, 4, PCM_OUT, &eng_fm_dl);
		if (!pcm_is_ready(fm_dl)) {
			SPRD_DBG("%s:cannot open pcm_fm_dl : %s", __func__,pcm_get_error(fm_dl));
			pcm_close(fm_dl);
			ret =  -1;
			goto err;
		} else {
			if( 0 != pcm_start(fm_dl)){
				SPRD_DBG("%s:pcm_fm_dl start unsucessfully: %s", __func__,pcm_get_error(fm_dl));
				ret=  -1;
				goto err;
			}
		}
		sprintf(cmd+strlen(cmd),"tinymix -D %d 78 1;",card_num);
		sprintf(cmd+strlen(cmd),"tinymix -D %d 97 1;",card_num);
	}
	else{
#ifdef SC9630
		sprintf(cmd+strlen(cmd),"tinymix -D %d 109 1;",card_num);
		sprintf(cmd+strlen(cmd),"tinymix -D %d 43 7;",card_num);
		sprintf(cmd+strlen(cmd),"tinymix -D %d 42 7;",card_num);
		sprintf(cmd+strlen(cmd),"tinymix -D %d 39  15;",card_num);
#else
		sprintf(cmd+strlen(cmd),"tinymix -D %d 92 0;",card_num);
		sprintf(cmd+strlen(cmd),"tinymix -D %d 64 0;",card_num);
		sprintf(cmd+strlen(cmd),"tinymix -D %d 65 0;",card_num);
		sprintf(cmd+strlen(cmd),"tinymix -D %d 94 1;",card_num);
        sprintf(cmd+strlen(cmd),"tinymix -D %d 35 15;",card_num);
        sprintf(cmd+strlen(cmd),"tinymix -D %d 38 7;",card_num);
        sprintf(cmd+strlen(cmd),"tinymix -D %d 39 7;",card_num);
#endif
		sprintf(cmd+strlen(cmd), "%s %s -D %d","tinyplay", SPRD_AUDIO_FILE,card_num);
		LOGD("mmitestsound:cmd= %s\n",cmd);
	}
	while(mode->finished != 1){

		//SPRD_DBG("[%s]:cmd=%s\n", __func__, cmd);
		status = system(cmd);

		if(status<0) {
			SPRD_DBG("%s: Error[%d] [%s]", __func__, status, strerror(errno));
		} else {
			SPRD_DBG("%s: OK", __func__);
		}
	}

	memset(cmd,0,1024);

	if(mode->mode == SPEAKER){
#ifdef SC9630
		sprintf(cmd+strlen(cmd),"tinymix -D %d 107 0;",card_num);
		sprintf(cmd+strlen(cmd),"tinymix -D %d 79 0;",card_num);
		sprintf(cmd+strlen(cmd),"tinymix -D %d 80 0;",card_num);
#else
		sprintf(cmd+strlen(cmd),"tinymix -D %d 92 0;",card_num);
		sprintf(cmd+strlen(cmd),"tinymix -D %d 64 0;",card_num);
		sprintf(cmd+strlen(cmd),"tinymix -D %d 65 0;",card_num);
#endif
	}
	else if(mode->mode == FM_PLAY){
		sprintf(cmd+strlen(cmd),"tinymix -D %d 97 0;",card_num);
		sprintf(cmd+strlen(cmd),"tinymix -D %d 78 0;",card_num);
		pcm_close(fm_dl);fm_dl = NULL;
	}
	else{
#ifdef SC9630
		sprintf(cmd+strlen(cmd),"tinymix -D %d 109 0;",card_num);
#else
		sprintf(cmd+strlen(cmd),"tinymix -D %d 94 0;",card_num);
#endif
	}
	system(cmd);
err:
	if(cmd) free(cmd);
	if(fm_dl)pcm_close(fm_dl);
	return 0;
}

int test_speaker_start(void)
{
	int ret = 0;
	pthread_t t1;
	eng_audio_mode   t_mode;
	ui_fill_locked();
	ui_show_title(MENU_TEST_SPEAKER);
	ui_set_color(CL_WHITE);
	ui_show_text(3, 0, TEXT_SPE_PLAYING);
	gr_flip();
	t_mode.mode = SPEAKER;//speaker
	t_mode.finished = 0;

	pthread_create(&t1, NULL, (void*)playback_thread, (void*)&t_mode);
	ret = ui_handle_button(NULL, NULL);//, TEXT_GOBACK
	t_mode.finished =  1;
	pthread_join(t1, NULL); /* wait "handle key" thread exit. */
	return ret;
}

