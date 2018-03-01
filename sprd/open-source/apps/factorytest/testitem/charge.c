#include "testitem.h"





static int thread_run=0;

unsigned int charge_get_adccali(void)
{
	unsigned int ret=0;
	char cali[16];
	char adccali[20];
	char *ptr, *start_ptr, *end_ptr;

	memset(adccali,0, sizeof(adccali));
	if(modem_send_at(-1, "AT+SGMR=0,0,4", adccali, sizeof(adccali), 0) < 0) {
		LOGD("[%s]: get adc cali fail\n", __FUNCTION__);
		return 0;
	}

	if(strstr(adccali,"ERR")) {
		LOGD("[%s]: get adc cali error info\n", __FUNCTION__);
		return 0;
	}

	ptr = strchr(adccali, ':');
	ptr++;
	while(isspace(*ptr)||(*ptr==0x0d)||(*ptr==0x0a))
		ptr++;

	start_ptr = ptr;

	while(!isspace(*ptr)&&(*ptr!=0x0d)&&(*ptr!=0x0a)&&(*ptr))
		ptr++;

	end_ptr = ptr;

	memset(cali, 0, sizeof(cali));
	snprintf(cali, end_ptr-start_ptr+1, "%s", start_ptr);
	ret = strtoul(cali, 0, 16);
	LOGD("cali=%s, ret=0x%x\n",  cali, ret);
	if((ret&SPRD_CALI_MASK)>0) {
		ret = 1;
	} else {
		ret = 0;
	}

	return ret;
}


float charge_get_batvol(void)
{
	int fd=-1;
	int voltage=0, n=0;
	float vol=0.000;
	char buffer[16];

	fd = open(ENG_BATVOL, O_RDONLY);
	if(fd > 0){
		memset(buffer, 0, sizeof(buffer));
		n = read(fd, buffer, sizeof(buffer));
		if(n > 0) {
			voltage = atoi(buffer);
			vol = ((float) voltage) * 0.001;
			LOGD("[%s]: buffer=%s; voltage=%d; vol=%f\n",__FUNCTION__, buffer, voltage, vol);
		}
		close(fd);
	}

	return vol;
}

float charge_get_chrvol(void)
{
	int fd=-1;
	int voltage=0, n=0;
	float vol=0.000;
	char buffer[16];

	fd = open(ENG_CHRVOL, O_RDONLY);
	if(fd > 0){
		memset(buffer, 0, sizeof(buffer));
		n = read(fd, buffer, sizeof(buffer));
		if(n > 0) {
			voltage = atoi(buffer);
			vol = ((float) voltage) * 0.001;
			LOGD("[%s]: buffer=%s; voltage=%d; vol=%f\n",__FUNCTION__, buffer, voltage, vol);
		}
		close(fd);
	}
	return vol;
}


void charge_get_chrcur(char *current, int length)
{
	int fd=-1;
	int n=0;

	fd = open(ENG_CURRENT, O_RDONLY);
	if(fd > 0){
		n = read(fd, current, length);
		if(n > 0) {
			LOGD("[%s]: current=%s;\n",__FUNCTION__, current);
		}
		close(fd);
	}
}

int charge_get_usbin(void)
{
	int fd=-1;
	int usbin=0, n=0;
	char buffer[16];

	fd = open(ENG_USBONLINE, O_RDONLY);
	if(fd > 0){
		memset(buffer, 0, sizeof(buffer));
		n = read(fd, buffer, sizeof(buffer));
		if(n > 0) {
			usbin = atoi(buffer);
			LOGD("[%s]: buffer=%s; usbin=%d;\n",__FUNCTION__, buffer, usbin);
		}
		close(fd);
	}

	return usbin;
}


int charge_get_acin(void)
{
	int fd=-1;
	int acin=0, n=0;
	char buffer[16];

	fd = open(ENG_ACONLINE, O_RDONLY);
	if(fd > 0){
		memset(buffer, 0, sizeof(buffer));
		n = read(fd, buffer, sizeof(buffer));
		if(n > 0) {
			acin = atoi(buffer);
			LOGD("[%s]: buffer=%s; usbin=%d;\n",__FUNCTION__, buffer, acin);
		}
		close(fd);
	}

	return acin;
}


static int  battery_online(void)
{
    int fd;
    char online[32];

    fd=open(ENG_BATONLINE,O_RDONLY);
    read(fd,online,sizeof(online));
    close(fd);

    LOGD("mmitest charge online %s\n",online);

    if(strncmp(online,"Good",4)==0)
        return 1;
    else
        return 0;
}

static int battery_current(void)
{
    int fd=-1;
    int n=0;
    unsigned char current[16];

    memset(current,0,sizeof(current));

    fd = open(ENG_BATCURRENT, O_RDONLY);
    if(fd > 0){
        n = read(fd, current, sizeof(current));
        if(n > 0) {
            LOGD("mmitest charge[%s]: current=%s;\n",__FUNCTION__, current);
        }
        close(fd);
    }
    return(atoi(current));
}



static void bubble_sort(int a[], int n)
{
    int temp,i,j;
    for (i = 0; i < n; i++)
    {
        for (j = i; j < n; j++)
        {
            if (a[i] > a[j])
            {
                temp = a[i];
                a[i] = a[j];
                a[j] = temp;
            }
        }
    }
}

int usbin_state=0;

static void charge_thread(void *param)
{
    float vol=0.0, chrvol=0.0;
    int current=0, usbin=0, acin=0, chrin=0;
    unsigned int cali =0;
    char buffer[64];
    char tmpbuf[32];
    int current_array[50];
    int start_row = 4;
    int first_row = 3;
    int last_row;
    int cnt = 0;
    int i,j,vol_count=0;
    int current_avr;
    int current_sum=0;
    int bat_cur,bat_cursum;

    if(battery_online()==0)
        system("echo 0 >/sys/class/power_supply/battery/stop_charge");//open charge

    while(thread_run == 1) {
        //get value
        memset(tmpbuf, 0, sizeof(tmpbuf));
        vol = charge_get_batvol();
        chrvol = charge_get_chrvol();
        usbin = charge_get_usbin();
        acin = charge_get_acin(); 
        for(i=0;i<50;i++)
        {
            usleep(10);
            charge_get_chrcur(tmpbuf, sizeof(tmpbuf));
            current_array[i]=atoi(tmpbuf);
            LOGD("mmitest current before=%d  %d\n",i,current_array[i]);
        }
        bubble_sort(current_array,50);
        for(i=0;i<50;i++)
        {
            LOGD("mmitest current after=%d   %d\n",i,current_array[i]);
        }
        current_sum=0;
        for(i=10;i<40;i++)
            current_sum+=current_array[i];
        current_avr=current_sum/30;

        LOGD("mmitest current after=%d\n",current_sum);
        LOGD("mmitest current after=%d\n",current_sum);

        bat_cursum=0;
        for(i=0;i<10;i++)
        {
            usleep(200);
            bat_cursum+=battery_current();
        }
        bat_cur=bat_cursum/10;

        LOGD("mmitest charge current=%d   battery_current=%d\n",current_avr,bat_cur);
        //check charger in or not
        if(usbin==1 || acin==1)
            {
                chrin = 1;
            }
        else
            {
                chrin = 0;
            }

        ui_set_color(CL_WHITE);
        //show battery voltage
        memset(buffer, 0, sizeof(buffer));
        sprintf(buffer, "%s%.2f %s", TEXT_CHG_BATTER_VOL, vol, "V");
        last_row = ui_show_text(start_row, 0, buffer);

        //show charing
        if(chrin==1) {
            usbin_state=0;
            ui_set_color(CL_GREEN);
            last_row = ui_show_text(last_row, 0, TEXT_CHG_RUN_Y);
        } else {
            usbin_state=1;
            ui_set_color(CL_RED);
            last_row = ui_show_text(last_row, 0, TEXT_CHG_RUN_N);
            gr_flip();
            cnt = 0;
        }
        //show charger type
        memset(buffer, 0, sizeof(buffer));
        ui_set_color(CL_WHITE);
        if(usbin==1) {
            sprintf(buffer, "%s%s", TEXT_CHG_TYPE, "USB");
        } else if (acin==1){
            sprintf(buffer, "%s%s", TEXT_CHG_TYPE, "AC");
        } else {
        //  sprintf(buffer, "%s", "Charger Type: NO");
        }
        last_row = ui_show_text(last_row, 0, buffer);

        if(usbin || acin) {
            memset(buffer, 0, sizeof(buffer));
            //show charger voltage
            sprintf(buffer, "%s%.2f %s", TEXT_CHG_VOL, chrvol, "V");
            last_row = ui_show_text(last_row, 0, buffer);

            //show charger current
            memset(buffer, 0, sizeof(buffer));
            sprintf(buffer, "%s%d%s", TEXT_CHG_CUR, current_avr, "mA");
            last_row = ui_show_text(last_row, 0, buffer);

            if(current_avr<200)
            {
                ui_set_color(CL_RED);
                if(vol>=4.15)
                    last_row=ui_show_text(last_row, 0, CHARGE_TIPS);
                ui_show_text(last_row, 0, TEXT_TEST_FAIL);
                gr_flip();
                sleep(3);
                ui_push_result(RL_FAIL);
                return ;
            }
            else
            {
                ui_push_result(RL_PASS);
                ui_set_color(CL_GREEN);
                ui_show_text(last_row, 0, TEXT_TEST_PASS);
                gr_flip();
                sleep(1);
                return ;
            }
        }
        gr_flip();
        sleep(1);
        ui_clear_rows(start_row, last_row-start_row);
    }
}



int test_charge_start(void)
{
	int ret = 0;
	pthread_t thead;

    ui_fill_locked();
    ui_show_title(MENU_TEST_CHARGE);
	gr_flip();
	LOGD("%s start", __FUNCTION__);
	thread_run=1;
	pthread_create(&thead, NULL, (void*)charge_thread, NULL);

	ret = ui_handle_button(NULL, NULL);//, TEXT_GOBACK
	thread_run=0;
	pthread_join(thead, NULL);
	LOGD("%s end", __FUNCTION__);
	save_result(CASE_TEST_CHARGE,ret);
	return ret;
}


