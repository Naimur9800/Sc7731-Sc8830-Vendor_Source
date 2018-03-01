#include "testitem.h"


/*
Global source used by wifi test.
*/
int eng_wifi_finish=0;
int eng_wifi_restart=1;
char *eng_wifi_filedata=NULL;
char* wifi_text[10][10];

pthread_mutex_t wifi_mutex;
pthread_cond_t wifi_cond;
pthread_mutex_t wifi_cond_mutex;

char* locstrchr(const char *s, char q)
{
	char*p=(char*)s;
	if(s==NULL)
		return NULL;

	while(*p!='\0')
	{
		if(*p==q)
			return p;
		p++;
	}
	return NULL;
}

int seek_file(const char *fn)
{
	int sz;
    int fd;
	fd = open(fn, O_RDONLY);
    if(fd < 0)
		return 0;

    sz = lseek(fd, 0, SEEK_END);
    if(sz < 0)
    {
		sz=0;
		goto oops;
    }
    if(lseek(fd, 0, SEEK_SET) != 0)
		sz=0;
oops:
	close(fd);
	return sz;
}

int read_file(const char *fn, void *data, int sz)
{
    int fd=-1;
	int ret=0;

	if(data == NULL)
		goto oops;

    if((fd=open(fn, O_RDONLY))<0)
		goto oops;

    if((ret=read(fd, data, sz)) != sz)
		ret=0;
oops:
	if(fd>0)
        close(fd);

    return ret;
}

int find_pid_by_name( char* ProcName, int* foundpid,int max)
{
        DIR             *dir;
        struct dirent   *d;
        int             pid, i;
        char            *s;
        int pnlen;

        i = 0;
        foundpid[0] = 0;
        pnlen = strlen(ProcName);

        /* Open the /proc directory. */
        dir = opendir("/proc");
        if (!dir)
        {
                SPRD_ERR("cannot open /proc");
                return -1;
        }

        /* Walk through the directory. */
        while ((d = readdir(dir)) != NULL&&i<max) {

                char exe [PATH_MAX+1];
                char path[PATH_MAX+1];
                int len;
                int namelen;

                /* See if this is a process */
                if ((pid = atoi(d->d_name)) == 0)
                    continue;

                snprintf(exe, sizeof(exe), "/proc/%s/exe", d->d_name);
                if ((len = readlink(exe, path, PATH_MAX)) < 0)
                        continue;
                path[len] = '\0';

                /* Find ProcName */
                s = strrchr(path, '/');
                if(s == NULL) continue;
                s++;

                /* we don't need small name len */
                namelen = strlen(s);
                if(namelen < pnlen)
                    continue;
                if(!strncmp(ProcName, s, pnlen)) {
                        /* to avoid subname like search proc tao but proc taolinke matched */
                        if(s[pnlen] == ' ' || s[pnlen] == '\0') {
                                foundpid[i] = pid;
                                i++;
                        }
                }
        }

        /*foundpid[i] = 0;*/
        closedir(dir);
        return  0;
}
/*
*Function:
*	Open the wpa_supplicant and scan AP, store the result in file 'wifi_test.txt'.
*ARG:
*RET:
*	OK:0, FAIL:-1
*/
#ifdef SC9630
static char* wifi_config="driver_param=p2p_device=1\r\nupdate_config=1\r\ndevice_name=Android_e03b\r\ndevice_type=10-0050F204-5\r\np2p_ssid_postfix=-Android_e03b\r\npersistent_reconnect=1\r\n";
#else
static char* wifi_config="update_config=1\r\ndevice_name=sp7731geahdplusoversea\r\nmanufacturer=SPRD\r\nmodel_name=SP7730A\r\nmodel_number=SP7730A\r\nserial_number=7731G11034B240576\r\ndevice_type=10-0050F204-5\r\nconfig_methods=physical_display virtual_push_button\r\np2p_disabled=1\r\nexternal_sim=1\r\n";
#endif

int eng_wpa_scan()
{
	int count = 5;
	int len = 0;
	int err;
	int fd;
	int fd_address;
	/*if (is_on){*/
	system("mkdir /data/misc/");
	system("chown system.misc /data/misc/");
	system("chmod 1777 /data/misc");

	system("mkdir /data/misc/wifi");
	system("chown wifi.wifi /data/misc/wifi");
	system("chmod 0777 /data/misc/wifi");

	err=system("mkdir /data/misc/wifi/sockets");
	if(err<0)
		{
			SPRD_ERR("mmitest make sockets error\n");
			return -1;
		}
	err=system("chown wifi.wifi /data/misc/wifi/sockets");
	if(err<0)
		{
			SPRD_ERR("mmitest chowm error\n");
			return -1;
		}
	err=system("chmod 0777 /data/misc/wifi/sockets");
	if(err<0)
			{
				SPRD_ERR("mmitest chmod error\n");
				return -1;
			}

	system("cp /system/etc/wifi/wpa_supplicant.conf /data/misc/wifi/");
	system("chown system.wifi /data/misc/wifi/wpa_supplicant.conf");



	system("echo \"sta_mode\" > /data/misc/wifi/fwpath");
	system("chown wifi.wifi /data/misc/wifi/fwpath");

	system("chmod 0777 /data/misc/wifi/*");

	if(access("/data/misc/wifi/fwpath",F_OK)!=0 ||
		access("/data/misc/wifi/wpa_supplicant.conf",F_OK)!=0){
		system("echo \"Wifi cfg file err\n\" > /data/local/tmp/wifi_test.txt");
		SPRD_DBG("mmitest Wifi cfg file err\n");
		return -1;
	}
	err=system("rmmod sprdwl");
	if(err<0)
		{
			SPRD_ERR("mmitest rmmod sprdwl error\n");
			return -1;
		}
	sleep(2);
	err=system("insmod /system/lib/modules/sprdwl.ko");
	if(err<0)
		{
			SPRD_ERR("mmitest insmod sprdwl error\n");
			return -1;
		}
        //Attention: sleep here to ensure the wifi driver has loaded into system completely.
	sleep(1);
	err=system("wpa_supplicant -iwlan0 -Dnl80211 -C/data/misc/wifi/sockets -c/data/misc/wifi/wpa_supplicant.conf -dd &");
	if(err<0)
		{
			SPRD_ERR("mmitest wpa_supplicant config error\n");
			return -1;
		}
	sleep(2);



	while(count){
		if(system("wpa_cli -iwlan0 -p/data/misc/wifi/sockets scan")<0)
		{
			SPRD_ERR("mmitest wifi error: wpa_cli scan!\n");
			return -1;
		}
		sleep(1);
		/*wpa_supplicant should scan 14 channels so 1 sec is needed.
		If antenna conflicting between wifi and bt cause scan fail, process retried 5 times at most.*/
		//system("echo -n \"MAC: \" > /data/misc/wifi/wifi_test.txt");
		//system("cat /sys/class/net/wlan0/address >> /data/misc/wifi/wifi_test.txt");
		system("cat /sys/class/net/wlan0/address >> /data/misc/wifi/wifi_address.txt");
		if(system("wpa_cli -iwlan0 -p/data/misc/wifi/sockets scan_results >> /data/misc/wifi/wifi_test.txt")<0)
		{
			SPRD_ERR("mmitest wifi error: wpa_cli scan_results!\n");
			return -1;
		}

		len = seek_file("/data/misc/wifi/wifi_test.txt");
		if(len>110)
			break;
		count--;
	}
	if(count<=0)
	{
		SPRD_ERR("mmitest wifi error: no result!\n");
		return -1;
	}
	return 0;
}
/*
*Function:
*	This FUNC read "wifi_test.txt" and store the result as a string,
*	then separate the string into several parts marked with '\n' and '\t'.
*ARG:
*	data: To store the content of wifi_test.txt.
*	redata: To store the separated string parts.
*RET:
*	OK:0, FAIL:-1
*/
int separateFile_FreeDataOutside(char* data, char* redata[10][10])
{
	int len=0, len1=0, i=0,j=0;
	char* pdata=NULL;
	char* row=NULL;
	char* col=NULL;
	int myfd=-1;

	memset(redata,0,sizeof(char*)*100);

	if((len=seek_file("/data/misc/wifi/wifi_test.txt"))==0)
		return -1;

	data=(char*)malloc(len+1);	/* The "data" pointer isn't freed by "free()". You must call "free()" outside this function.*/
	if(data==NULL)
	{
		SPRD_DBG("MMI wifi wifitest malloc fail!\n");
		return -1;
	}
	memset(data,0,len+1);
	if((len1=read_file("/data/misc/wifi/wifi_test.txt",data,len))==0)
		return -1;

	data[len+1]='\0';
	pdata=data;
	SPRD_DBG("MMI wifi wifitest filedata: %s\n",pdata);

	i=0;
	do{
		row=strsep(&pdata,"\n");
		if(row!=NULL)
		{
			col=row;
			if(locstrchr(col,'/'))
			{
				strcpy(col,"bssid\tfreq\tsignal\tssid");
			}

			if(locstrchr(col,'\t'))
			{
				j=0;
				do{
					redata[i][j]=strsep(&col,"\t");
				}while(redata[i][j++]!=NULL&&j<10);
			}
			else if(strlen(row)>5 && NULL==strstr(row,"Selected"))
			{
				redata[i][0]=col;
			}
		}
		else
		{
			break;
		}
		i++;
	}while(i<10);

	SPRD_DBG("MMI wifi wifitest resort data:\n");
	for(i=0;i<10;i++)
	{
		for(j=0;j<10;j++)
		{
			if(redata[i][j]!=NULL)
				SPRD_DBG("i=%d j=%d:%s",i,j,redata[i][j]);
		}
	}
	return 0;
}

static void eng_wifi_show(char* text[10][10])
{
	int i=0,j=0;

	int start_x = 0;
	int start_y = 2;
	int dwx=0,dwy=0;
	char *rowctl=NULL;

	SPRD_DBG("MMI wifi in eng_wifi_show\n");

	for(i=0;i<10;i++)
	{
		dwx=start_x+1;
		/*SPRD_DBG("MMI wifi show loop=%d,%d: 1\n",i,j);*/

		if(text[i][0]==NULL)
			continue;

		/*SPRD_DBG("MMI wifi show loop=%d,%d: 2\n",i,j);*/
		ui_set_color(i%3+2);
		if(text[i][1]==NULL)
		{
			/*SPRD_DBG("MMI wifi show loop=%d,%d: 3 %s\n",i,j,text[i][0]);*/
			ui_show_text(start_y, dwx, text[i][0]);
		}
		else
		{
			rowctl=strstr(text[i][0],"bssid");
			for(j=0;j<10;j++)
			{
				if(text[i][j]==NULL)
					break;
				if(*text[i][j]=='[')
					continue;
				/*SPRD_DBG("MMI wifi wifitest-t: %s\n",text[i][j]);*/
				ui_show_text(start_y, dwx, text[i][j]);
				if(rowctl)
				{
					dwx+=strlen(text[i][j])+2;
				}
				else
				{
					if(j==1)
					{
						dwx=start_x+1;
						start_y+=1;
					}
					else
					{
						dwx+=strlen(text[i][j])+1;
					}
				}
			}
		}
		start_y+=1;
		gr_flip();
	}
}

int eng_wifi_scan_start(void)
{
	#ifndef ENG_WIFI_ONCE
	while(1)
	{
		pthread_mutex_lock(&wifi_cond_mutex);
		while(eng_wifi_restart==0)
		{
			pthread_cond_wait(&wifi_cond,&wifi_cond_mutex);
		}
		pthread_mutex_unlock(&wifi_cond_mutex);

		if(system("wpa_cli -iwlan0 -p/data/misc/wifi/sockets terminate")<0)
		{
			SPRD_ERR("mmitest wifi error: wpa_cli terminate!\n");
		}
		usleep(100);
	#endif
		/*pthread_mutex_lock(&wifi_mutex);*/
		if(0==eng_wpa_scan())
		{
			/*take*/
			pthread_mutex_lock(&wifi_mutex);
			separateFile_FreeDataOutside(eng_wifi_filedata, wifi_text);
			eng_wifi_finish=1;
			#ifndef ENG_WIFI_ONCE
			pthread_mutex_lock(&wifi_cond_mutex);
			eng_wifi_restart=0;
			pthread_mutex_unlock(&wifi_cond_mutex);
			#endif
			pthread_mutex_unlock(&wifi_mutex);
			/*release*/
		}
		else
		{
			/*take*/
			pthread_mutex_lock(&wifi_mutex);
			eng_wifi_finish=-1;
			pthread_mutex_unlock(&wifi_mutex);
			/*release*/
			SPRD_ERR("MMI:eng_wifi_test fail!\n");
		}
		SPRD_DBG("%s End",__FUNCTION__);
	#ifndef ENG_WIFI_ONCE
	}
	#else
	if(system("wpa_cli -iwlan0 -p/data/misc/wifi/sockets terminate")<0)
	{
		SPRD_ERR("mmitest  wifi error: wpa_cli terminate!\n");
	}
	if(system("rmmod sprdwl"))
	{
		SPRD_ERR("mmitest wifi error: rmmod sprdwl!\n");
	}
	system("rm -f /data/misc/wifi/wifi_test.txt");
	#endif
	return 0;
}

int test_wifi_pretest(void)
{
	int ret;
	if(eng_wifi_finish == 1)
		ret= RL_PASS;
	else
		ret= RL_FAIL;

	save_result(CASE_TEST_WIFI,ret);
	return ret;
}

int test_wifi_start(void)
{
	SPRD_DBG("%s enter", __FUNCTION__);
	int i=0;
	int ret=0;
	int midrow = gr_fb_height()/CHAR_HEIGHT/2;
	int midcol = gr_fb_width()/CHAR_WIDTH/2;

	ui_fill_locked();
	ui_show_title(SPRD_WIFI_TITLE);
	ui_set_color(CL_WHITE);
	ui_show_text(2, 0, TEXT_BT_SCANING);
	gr_flip();
	eng_wifi_scan_start(); //add by qing for test request

	for(i=0;i<7;i++)
	{
		/*take*/
		pthread_mutex_lock(&wifi_mutex);
		if(eng_wifi_finish==-1)
		{
			/*release*/
			#ifndef ENG_WIFI_ONCE
			eng_wifi_finish==0;
			pthread_mutex_lock(&wifi_cond_mutex);
			eng_wifi_restart=1;
			pthread_mutex_unlock(&wifi_cond_mutex);
			#endif
			pthread_mutex_unlock(&wifi_mutex);
			SPRD_ERR("MMI wifi: system fun error!\n");
			ui_set_color(CL_WHITE);
			ui_show_text(midrow, midcol-strlen(SPRD_WIFI_ERROR)/2, SPRD_WIFI_ERROR);
			gr_flip();
			ret= RL_FAIL;
			save_result(CASE_TEST_WIFI,ret);
			return ret;
		}
		else if(eng_wifi_finish==1)
		{
			eng_wifi_show(wifi_text);

			#ifndef ENG_WIFI_ONCE
			if(eng_wifi_filedata!=NULL)
			{
				free(eng_wifi_filedata);
			}
			eng_wifi_finish=0;
			pthread_mutex_lock(&wifi_cond_mutex);
			eng_wifi_restart=1;
			pthread_cond_signal(&wifi_cond);
			pthread_mutex_unlock(&wifi_cond_mutex);
			#else
			system("rm -f /data/misc/wifi/wifi_test.txt");
			#endif
			pthread_mutex_unlock(&wifi_mutex);
			ui_set_color(CL_WHITE);
			ui_show_text(midrow, midcol-strlen(SPRD_WIFI_PASSED)/2, SPRD_WIFI_PASSED);
			gr_flip();
			/*release*/
			break;
		}
		else
		{
			/*release*/
			pthread_mutex_unlock(&wifi_mutex);
			ui_set_color(CL_WHITE);
			ui_show_text(midrow-3, midcol-strlen(SPRD_WIFI_WAIT)/2, SPRD_WIFI_WAIT);
			gr_flip();
			sleep(1);
		}
	}
	if(i>=7)
	{
		SPRD_ERR("MMI wifi: system fun time out!\n");
		ui_set_color(CL_WHITE);
		ui_show_text(midrow, midcol-strlen(SPRD_WIFI_TIMEOUT)/2, SPRD_WIFI_TIMEOUT);
		gr_flip();
		ret= RL_FAIL;
		save_result(CASE_TEST_WIFI,ret);
		return ret;
	}
	sleep(1);
	ret= RL_PASS;
	save_result(CASE_TEST_WIFI,ret);
	return ret;
}



