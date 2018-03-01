#include "testitem.h"
#include <sys/vfs.h>


static const char TCARD_VOLUME_NAME[]  = "sdcard";
static char TCARD_DEV_NAME[128];
static const char SYS_BLOCK_PATH[]     = "/sys/block";
static const char TCARD_FILE_NAME[]    = "/mnt/sdcard/sciautotest";
static const char TCARD_TEST_CONTENT[] = "SCI TCard: 2012-11-12";
//------------------------------------------------------------------------------
int tcard_get_dev_path(char*path)
{
	DIR *dir;
	struct dirent *de;
	int found = 0;
	char dirpath_open[128] = "/sys/devices";
	char tcard_name[16];

	sprintf(path, "../devices");

	dir = opendir(dirpath_open);
	if (dir == NULL) {
		LOGD("%s open fail\n", dirpath_open);
		return -1;
	}
	/*try to find dir: sdio_sd, dt branch*/
	while((de = readdir(dir))) {
		if(strncmp(de->d_name, "sdio_sd", strlen("sdio_sd"))) {
			continue;
		}
		sprintf(dirpath_open, "%s/%s", dirpath_open, de->d_name);
		sprintf(path, "%s/%s", path, de->d_name);
		found = 1;
		break;
	}
	/*try to find dir: sdio_sd,no dt branch*/
	if(!found) {
		sprintf(dirpath_open, "%s/platform", dirpath_open);
		closedir(dir);
		dir = opendir(dirpath_open);
		if(dir == NULL) {
			LOGD("%s open fail\n", dirpath_open);
			return -1;
		}
		while((de = readdir(dir))) {
			if(strncmp(de->d_name, "sdio_sd", strlen("sdio_sd"))) {
				continue;
			}
			sprintf(dirpath_open, "%s/%s", dirpath_open, de->d_name);
			sprintf(path, "%s/platform/%s", path, de->d_name);
			found = 1;
			break;
		}
	}
	closedir(dir);
	if(!found) {
		LOGD("sdio_sd is not found \n");
		return -1;
	}
	found = 0;
	sprintf(dirpath_open, "%s/mmc_host", dirpath_open);
	dir = opendir(dirpath_open);
	if(dir == NULL) {
		LOGD("%s open fail\n", dirpath_open);
		return -1;
	}
	/*may be mmc0 or mmc1*/
	while((de = readdir(dir))) {
		if(strstr(de->d_name, "mmc") != NULL) {
			sprintf(dirpath_open, "%s/%s", dirpath_open, de->d_name);
			sprintf(path, "%s/mmc_host/%s", path, de->d_name);
			break;
		}
	}
	strncpy(tcard_name, de->d_name, sizeof(tcard_name)-1);
	if(strlen(de->d_name) < 16) {
		tcard_name[strlen(de->d_name)] = 0;
	} else {
		tcard_name[15] = 0;
	}
	closedir(dir);
	dir = opendir(dirpath_open);/*open dir: sdio_sd/mmc_host/mmc0 or mmc1*/
	if(dir == NULL) {
		LOGD("%s open fail\n", dirpath_open);
		return -1;
	}
	/* try to find: sdio_sd/mmc_host/mmcx/mmcx:xxxx,  if can find this dir ,T card work well */
	while((de = readdir(dir))) {
		if(strstr(de->d_name, tcard_name) == NULL) {
			continue;
		}
		sprintf(dirpath_open, "%s/%s", dirpath_open, de->d_name);
		sprintf(path, "%s/%s", path, de->d_name);
		found = 1;
		break;
	}
	closedir(dir);
	sprintf(dirpath_open, "%s/block",dirpath_open);
	dir = opendir(dirpath_open);
	if(dir == NULL) {
		LOGD("%s open fail\n", dirpath_open);
		return -1;
	}
	while((de = readdir(dir)))/*may be mmcblk0 or mmcblk1*/
	{
		if(strstr(de->d_name, "mmcblk") != NULL)
		{
			sprintf(dirpath_open, "%s/%s", dirpath_open, de->d_name);
			sprintf(path, "%s/block/%s", path, de->d_name);
			break;
		}
	}
	closedir(dir);
	if (found) {
		LOGD("the tcard dir is %s \n",path);
		return 0;
	}else {
		LOGD("the tcard dir is not found \n");
		return -1;
	}
}



//------------------------------------------------------------------------------
int tcardOpen( void )
{
	int ret = 0;
	ret = tcard_get_dev_path(TCARD_DEV_NAME);
	LOGD("ret %d \n",ret);
	return ret;
}

//------------------------------------------------------------------------------
int tcardIsPresent( void )
{
    DIR * dirBlck = opendir(SYS_BLOCK_PATH);
    if( NULL == dirBlck ) {
        LOGD("opendir '%s' fail: %s(%d)\n", SYS_BLOCK_PATH, strerror(errno), errno);
        return -1;
    }
    int present = -2;

    char realName[256];
    char linkName[128];
    strncpy(linkName, SYS_BLOCK_PATH, sizeof(linkName) - 1);
    char * pname = linkName + strlen(linkName);
    *pname++ = '/';

    struct dirent *de;
    while( (de = readdir(dirBlck)) ) {
        if (de->d_name[0] == '.' || DT_LNK != de->d_type )
            continue;

        strncpy(pname, de->d_name, 64);

        int len = readlink(linkName, realName, sizeof(realName)-1);
        if( len < 0 ) {
            LOGD("readlink error: %s(%d)\n", strerror(errno), errno);
            continue;
        }
		if(len < 256) {
			realName[len] = 0;
		} else {
			realName[255] = 0;
		}

        LOGD("link name = %s, real name = %s, TCARD_DEV_NAME=%s\n", linkName, realName,TCARD_DEV_NAME);
        if(strstr(realName, TCARD_DEV_NAME) != NULL) {
            present = 1;
            LOGD("TCard is present.\n");
            break;
        }
    }

    closedir(dirBlck);
    return present;
}

int tcardIsMount( void )
{
    char device[256];
    char mount_path[256];
    FILE *fp;
    char line[1024];

    if (!(fp = fopen("/proc/mounts", "r"))) {
        LOGD("Error opening /proc/mounts (%s)", strerror(errno));
        return -1;
    }

    int mount = 0;
    while(fgets(line, sizeof(line), fp)) {
        line[strlen(line)-1] = '\0';
        sscanf(line, "%255s %255s\n", device, mount_path);
        LOGD("dev = %s, mount = %s\n", device, mount_path);
        if( NULL != strstr(mount_path, TCARD_VOLUME_NAME)) {
            mount = 1;
            LOGD("TCard mount path: '%s'\n", mount_path);
            break;
        }
    }

    fclose(fp);
    return mount;
}

int tcardRWTest( void )
{

	FILE * fp = fopen(TCARD_FILE_NAME, "w+");
    if( NULL == fp ) {
        LOGD("mmitest open '%s'(rw) fail: %s(%d)\n", TCARD_FILE_NAME, strerror(errno), errno);
        return -1;
    }

    if( fwrite(TCARD_TEST_CONTENT, 1, sizeof(TCARD_TEST_CONTENT), fp) != sizeof(TCARD_TEST_CONTENT) ) {
        LOGD("mmitest write '%s' fail: %s(%d)\n", TCARD_FILE_NAME, strerror(errno), errno);
        fclose(fp);
        return -2;
    }
    fclose(fp);

    fp = fopen(TCARD_FILE_NAME, "r+");
    if( NULL == fp ) {
        LOGD("mmitest open '%s'(ronly) fail: %s(%d)\n", TCARD_FILE_NAME, strerror(errno), errno);
        return -3;
    }


    char buf[128];

    if( fread(buf, 1, sizeof(TCARD_TEST_CONTENT), fp) != sizeof(TCARD_TEST_CONTENT) ) {
        LOGD("mmitest read '%s' fail: %s(%d)\n", TCARD_FILE_NAME, strerror(errno), errno);
        fclose(fp);
        return -4;
    }
    fclose(fp);

    unlink(TCARD_FILE_NAME);

    if( strncmp(buf, TCARD_TEST_CONTENT, sizeof(TCARD_TEST_CONTENT) - 1) ) {
        LOGD("mmitest read = %s, dst = %s\n", buf, TCARD_TEST_CONTENT);
        return -5;
    }

    LOGD("mmitest TFlash Card rw OK.\n");
    return 0;
}

//------------------------------------------------------------------------------
int tcardClose( void )
{
	return 0;
}



static int sdcard_rw(void)
{
	int fd;
	int ret = -1;
	unsigned char w_buf[RW_LEN];
	unsigned char r_buf[RW_LEN];
	int i = 0;

	for(i = 0; i < RW_LEN; i++) {
		w_buf[i] = 0xff & i;
	}

	fd = open(SPRD_SD_TESTFILE, O_CREAT|O_RDWR, 0666);
	if(fd < 0){
		LOGD("[%s]: create %s fail",__func__, SPRD_SD_TESTFILE);
		goto RW_END;
	}

	if(write(fd, w_buf, RW_LEN) != RW_LEN){
		LOGD("[%s]: write data error",	__FUNCTION__);
		goto RW_END;
	}

	lseek(fd, 0, SEEK_SET);
	memset(r_buf, 0, sizeof(r_buf));

	read(fd, r_buf, RW_LEN);
	if(memcmp(r_buf, w_buf, RW_LEN) != 0) {
		LOGD("[%s]: read data error", __FUNCTION__);
		goto RW_END;
	}

	ret = 0;
RW_END:
	if(fd > 0) close(fd);
	return ret;
}

int test_sdcard_pretest(void)
{
	int fd;
	int ret;
	system(SPRD_MOUNT_DEV);
	fd = open(SPRD_SD_DEV, O_RDWR);
	if(fd < 0) {
		ret= RL_FAIL;
	} else {
		close(fd);
		ret= RL_PASS;
	}

	save_result(CASE_TEST_SDCARD,ret);
	return ret;
}

int check_file_exit(void)
{
	int ret;
	int fd;
	if(access(SPRD_SD_FMTESTFILE,F_OK)!=-1)
		ret=1;
	else
		ret=0;
	close(fd);
	return ret;
}

int sdcard_read_fm(int *rd)
{
	int fd;
	int ret;
	fd = open(SPRD_SD_FMTESTFILE, O_CREAT|O_RDWR,0666);
	if(fd < 0)
		{
				return -1;
		}
	LOGD("mmitest opensdcard file is ok");
	lseek(fd, 0, SEEK_SET);
	ret=read(fd, rd, sizeof(int));
	LOGD("mmitest read file is=%d",*rd);
	close(fd);
	return ret;
}

int sdcard_write_fm(int *freq)
{
	int fd;

	fd = open(SPRD_SD_FMTESTFILE, O_CREAT|O_RDWR, 0666);
	if(fd < 0)
		{
			return -1;
		}
	LOGD("mmitest opensdcard file is ok");
	LOGD("mmitest write size=%d",write(fd, freq, sizeof(int)));
	if(write(fd, freq, sizeof(int)) != sizeof(int))
		{
			return -1;
		}
	LOGD("mmitest write file is ok");
	close(fd);
	return 0;
}


int test_sdcard_start(void)
{
	struct statfs fs;
	int fd;
	int ret = RL_FAIL; //fail
	int cur_row = 2;
	char temp[64];
	ui_fill_locked();
	ui_show_title(MENU_TEST_SDCARD);
	ui_set_color(CL_WHITE);
	cur_row = ui_show_text(cur_row, 0, TEXT_SD_START);
	gr_flip();
	fd = open(SPRD_SD_DEV, O_RDWR);
	if(fd < 0) {
		ui_set_color(CL_RED);
		cur_row = ui_show_text(cur_row, 0, TEXT_SD_OPEN_FAIL);
		gr_flip();
		goto TEST_END;
	} else {
		ui_set_color(CL_GREEN);
		cur_row = ui_show_text(cur_row, 0, TEXT_SD_OPEN_OK);
		gr_flip();
	}

	if(statfs(SPRD_SDCARD_PATH, &fs) < 0) {
		ui_set_color(CL_RED);
		cur_row = ui_show_text(cur_row, 0, TEXT_SD_STATE_FAIL);
		gr_flip();
		goto TEST_END;
	} else {
		ui_set_color(CL_GREEN);
		cur_row = ui_show_text(cur_row, 0, TEXT_SD_STATE_OK);
		sprintf(temp, "%d MB", (fs.f_bsize*fs.f_blocks/1024/1024));
		cur_row = ui_show_text(cur_row, 0, temp);
		gr_flip();
	}

	if(sdcard_rw()< 0) {
		ui_set_color(CL_RED);
		cur_row = ui_show_text(cur_row, 0, TEXT_SD_RW_FAIL);
		gr_flip();
		goto TEST_END;
	} else {
		ui_set_color(CL_GREEN);
		cur_row = ui_show_text(cur_row, 0, TEXT_SD_RW_OK);
		gr_flip();
	}

	ret = RL_PASS;
TEST_END:
	if(ret == RL_PASS) {
		ui_set_color(CL_GREEN);
		cur_row = ui_show_text(cur_row, 0, TEXT_TEST_PASS);
	} else {
		ui_set_color(CL_RED);
		cur_row = ui_show_text(cur_row, 0, TEXT_TEST_FAIL);
	}
	gr_flip();
	sleep(1);

	save_result(CASE_TEST_SDCARD,ret);
	return ret;
}
