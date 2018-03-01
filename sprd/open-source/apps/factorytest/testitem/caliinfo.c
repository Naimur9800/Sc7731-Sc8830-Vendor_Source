#include "testitem.h"
#include <stdlib.h>
#include <ctype.h>
#include <string.h>


extern char* test_modem_get_caliinfo(void);





/********************************************************************
*  Function: my_strstr()
*********************************************************************/
char * my_strstr(char * ps,char *pd)
{
    char *pt = pd;
    int c = 0;
    while(*ps != '\0')
    {
        if(*ps == *pd)
        {
            while(*ps == *pd && *pd!='\0')
            {
                ps++;
                pd++;
                c++;
            }
        }else
        {
            ps++;
        }
        if(*pd == '\0')
        {
            return (ps - c);
        }
        c = 0;
        pd = pt;
    }
    return 0;
}


/********************************************************************
*  Function:str_replace()
*********************************************************************/
int str_replace(char *p_result,char* p_source,char* p_seach,char *p_repstr)
{
    int c = 0;
    int repstr_leng = 0;
    int searchstr_leng = 0;
    char *p1;
    char *presult = p_result;
    char *psource = p_source;
    char *prep = p_repstr;
    char *pseach = p_seach;
    int nLen = 0;
    repstr_leng = strlen(prep);
    searchstr_leng = strlen(pseach);
    do{
        p1 = my_strstr(psource,p_seach);
        if (p1 == 0)
        {
            strcpy(presult,psource);
            return c;
        }
        c++;
        nLen = p1 - psource;
        memcpy(presult, psource, nLen);
        memcpy(presult + nLen,p_repstr,repstr_leng);
        psource = p1 + searchstr_leng;
        presult = presult + nLen + repstr_leng;
    }while(p1);
    return c;
}


extern int text_rows;
extern char s_cali_info1[1024];
#ifdef SC9630
extern char s_cali_info2[1024];
#endif
int test_cali_info(void)
{
	int ret = 0;
	int i;
	int row = 2;
	char tmp[64][32];
	char tmp2[64][32];
	char tmp3[64][32];
	char tmp4[64][32];
	char tmp5[64][32];
	char tmp6[64][32];
	char* pcur;
	char* pos1;
	char* pos2;
	int len;
	int testlen;
	int row_num=0;
	int row_num1=0;
	int row_num2=0;
	int cali_size=0;
	unsigned char chang_page=0;
	unsigned char change=1;

	memset(tmp,0,sizeof(tmp));
	memset(tmp2,0,sizeof(tmp2));
	memset(tmp3,0,sizeof(tmp3));
	memset(tmp4,0,sizeof(tmp4));
	memset(tmp5,0,sizeof(tmp5));
	memset(tmp6,0,sizeof(tmp6));

	ui_fill_locked();
	ui_show_title(MENU_CALI_INFO);
	pcur = test_modem_get_caliinfo();
	len = strlen(pcur);
	cali_size=sizeof(tmp[0])/sizeof(tmp[0][0]);
	ui_set_color(CL_WHITE);

	/*delete the "BIT",and replace the calibrated with cali */

	while(len > 0) {
		pos1 = strchr(pcur, ':');
		if(pos1 == NULL) break;
		pos1++;
		pos2 = strstr(pos1, "BIT");
		if(pos2 == NULL) {
			strcpy(tmp, pos1);
			len = 0;
		} else {
			memcpy(tmp[row_num], pos1, (pos2-pos1));
			tmp[pos2-pos1][row_num] = '\0';
			len -= (pos2 - pcur);
			pcur = pos2;
		}
        testlen=str_replace(tmp2[row_num],tmp[row_num],"calibrated","cali");
        LOGD("mmitest test=%s   %p\n",tmp2[row_num],tmp2[row_num]);
		row_num++;
		//row = ui_show_text(row, 0, tmp2);
	}
#ifdef SC9630
	pcur = s_cali_info2;
	len = strlen(pcur);
	while(len > 0) {
		pos1 = strchr(pcur, ':');
		if(pos1 == NULL) break;
		pos1++;
		pos2 = strstr(pos1, "BIT");
		if(pos2 == NULL) {
			strcpy(tmp3, pos1);
			len = 0;
		} else {
			memcpy(tmp3[row_num1], pos1, (pos2-pos1));
			tmp3[pos2-pos1][row_num] = '\0';
			len -= (pos2 - pcur);
			pcur = pos2;
		}
        testlen=str_replace(tmp4[row_num1],tmp3[row_num1],"BAND","WCDMA BAND");
        LOGD("mmitest test=%s\n",tmp4[row_num1]);
		row_num1++;
		//row = ui_show_text(row, 0, tmp2);
	}
#endif

	pcur = s_cali_info1;
	len = strlen(pcur);
	while(len > 0) {
		pos1 = strchr(pcur, ':');
		if(pos1 == NULL) break;
		pos1++;
		pos2 = strstr(pos1, "BIT");
		if(pos2 == NULL) {
			strcpy(tmp3, pos1);
			len = 0;
		} else {
			memcpy(tmp5[row_num2], pos1, (pos2-pos1));
			tmp5[pos2-pos1][row_num] = '\0';
			len -= (pos2 - pcur);
			pcur = pos2;
		}
        testlen=str_replace(tmp6[row_num2],tmp5[row_num2],"BAND","WCDMA BAND");
        LOGD("mmitest test=%s\n",tmp6[row_num2]);
		row_num2++;
		//row = ui_show_text(row, 0, tmp2);
	}


if(1)
	{
		do{
			if(chang_page==RL_PASS)
			{
				//change=!change;
				change++;
				if(change==3)
				     change=0;
				chang_page=RL_NA;
				LOGD("show result change=%d",change);
			}
			if(change==1)
			{
				ui_set_color(CL_SCREEN_BG);
				gr_fill(0, 0, gr_fb_width(), gr_fb_height());
				ui_fill_locked();
				ui_show_title(MENU_CALI_INFO);
				row=2;
				for(i = 0; i < text_rows-2; i++)
				{
					if(strstr(tmp2[i],"Pass")!=NULL)
						ui_set_color(CL_GREEN);
					if(strstr(tmp2[i],"Not")!=NULL)
						ui_set_color(CL_RED);
					if(strstr(tmp2[i],"MMI Test")!=NULL)
					     continue;
					row = ui_show_text(row, 0, tmp2[i]);
					LOGD("mmitest test1=%s   %p\n",tmp2[i],tmp2[i]);
					gr_flip();
				}
			}

			else if(change==0)
			{
			row=2;
			ui_set_color(CL_SCREEN_BG);
			gr_fill(0, 0, gr_fb_width(), gr_fb_height());
			ui_fill_locked();
			ui_show_title(MENU_CALI_INFO);
			for(i=0;i<row_num+2-text_rows;i++)
			{
                if(strstr(tmp2[text_rows-2+i],"Pass")!=NULL)
                    ui_set_color(CL_GREEN);
				if(strstr(tmp2[text_rows-2+i],"Not")!=NULL)
					ui_set_color(CL_RED);
				if(strstr(tmp2[text_rows-2+i],"MMI Test")!=NULL)
				     continue;
				row = ui_show_text(row, 0, tmp2[text_rows-2+i]);
			}
			for(i=0;i<row_num1;i++)
				{
					if(strstr(tmp4[i],"Pass")!=NULL)
						ui_set_color(CL_GREEN);
					if(strstr(tmp4[i],"Not")!=NULL)
						ui_set_color(CL_RED);
					if(strstr(tmp4[i],"MMI Test")!=NULL)
					     continue;
					row = ui_show_text(row, 0, tmp4[i]);
				}
			gr_flip();
			}

			else if(change==2)
			{
			row=2;
			ui_set_color(CL_SCREEN_BG);
			gr_fill(0, 0, gr_fb_width(), gr_fb_height());
			ui_fill_locked();
			ui_show_title(MENU_CALI_INFO);
			for(i=0;i<row_num+2-text_rows;i++)
			{
				if(strstr(tmp6[text_rows-2+i],"Pass")!=NULL)
					ui_set_color(CL_GREEN);
				if(strstr(tmp6[text_rows-2+i],"Not")!=NULL)
					ui_set_color(CL_RED);
				if(strstr(tmp6[text_rows-2+i],"MMI Test")!=NULL)
				     continue;
				row = ui_show_text(row, 0, tmp6[text_rows-2+i]);
			}
			for(i=0;i<row_num2;i++)
				{
                    if(strstr(tmp6[i],"Pass")!=NULL)
						ui_set_color(CL_GREEN);
                    if(strstr(tmp6[i],"Not")!=NULL)
						ui_set_color(CL_RED);
					if(strstr(tmp6[i],"MMI Test")!=NULL)
					     continue;
					row = ui_show_text(row, 0, tmp6[i]);
				}
			gr_flip();
			}

		}while((chang_page=ui_handle_button(NULL,NULL))!=RL_FAIL);
	}

	LOGD("mmitest before button");
	return 0;
}
