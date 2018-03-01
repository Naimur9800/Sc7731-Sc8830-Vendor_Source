#define LOG_TAG     "IQDATA"

#include <sys/mman.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <pthread.h>
#include <fcntl.h>
#include <dirent.h>

#include <cutils/properties.h>
#include <cutils/log.h>

#define SPRD_IQ_SYSFS               "/sys/module/sprd_iq/parameters/iq_base"
#define SPRD_IQ_BASE_LENGTH         32
#define SPRD_INVALID_IQ_BASE        0xffffffff
#define SPRD_IQ_BUFFER_SIZE         (128 * 1024 * 1024)
#define SPRD_IQ_TRANSFER_SIZE       (512 * 1024)
#define SPRD_IQ_FILE_NAME_LEN       12

#define IQ_AP_WRITE_NEW             0xC3005A5A
#define IQ_AP_WRITE_CONTINUE        0xC3FF5A5A
#define IQ_AP_WRITE_FINISHED        0x3C3CA5A5

typedef struct {
	char file_name[256];
	uint32_t base_addr;
	uint8_t *map_addr;
	pthread_t thread;
	int thread_exit;
	uint32_t state;
	int started;
} sprd_iq_params;

static sprd_iq_params g_iq_params = {
	.base_addr = SPRD_INVALID_IQ_BASE,
	.thread_exit = 0,
	.state = 0,
	.started = 0,
};

typedef struct {
	uint32_t wr_rd_flag;	// Write/Read flag
	char iq_file_name[SPRD_IQ_FILE_NAME_LEN];	// IQ data file
} sprd_iq_jnjection_header;

/* Write IQ data form file to buffer */
static uint32_t sprd_iq_write(FILE * fp, uint8_t * addr, uint32_t length)
{
	uint8_t *vaddr;
	uint32_t send_len = 0;
	uint32_t len;
	static int i = 0;
	int8_t buf[SPRD_IQ_TRANSFER_SIZE];

	if (NULL == fp || NULL == addr)
		return -1;

	while (length - send_len > 0) {
		len = length - send_len;
		len =
		    (len > SPRD_IQ_TRANSFER_SIZE) ? SPRD_IQ_TRANSFER_SIZE : len;
		vaddr = addr + send_len;

		if (fread(buf, 1, len, fp) != len) {
			ALOGE("%s read len 0x%x failed\n", __func__, len);
			if (feof(fp))
				ALOGE("%s reach the end of file\n", __func__);

			return send_len;
		}

		memcpy(vaddr, buf, len);
		send_len += len;
	}

	return send_len;
}

/* Polling IQ data buffer, and import data */
static void sprd_iq_thread(sprd_iq_params * params)
{
	FILE *fp = NULL;
	/* The header of buffer is IQ injectiong */
	sprd_iq_jnjection_header *header =
	    (sprd_iq_jnjection_header *) (params->map_addr);
	uint32_t length = SPRD_IQ_BUFFER_SIZE;
	char iq_file[SPRD_IQ_FILE_NAME_LEN];
	char file_name[256];

	ALOGI("injection header addr is 0x%x\n", (uint32_t) header);

	while (1) {
		/* Monitor exit flag */
		if (1 == params->thread_exit) {
			if (fp != NULL)
				fclose(fp);

			ALOGI("thread is eixt\n");
			params->thread_exit = 0;
			break;
		}

		/* check buffer flag */
		switch (header->wr_rd_flag) {
			/* New data, import it */
		case IQ_AP_WRITE_NEW:
			params->state = IQ_AP_WRITE_NEW;
			if (fp != NULL)
				fclose(fp);

			/* Get file name */
			strcpy(file_name, params->file_name);
			strncpy(iq_file, header->iq_file_name,
				SPRD_IQ_FILE_NAME_LEN);
			strcat(file_name, "/");
			strncat(file_name, iq_file, SPRD_IQ_FILE_NAME_LEN);
			ALOGI("IQ_AP_WRITE_NEW get file %s\n", file_name);

			fp = fopen(file_name, "r");
			if (NULL == fp) {
				ALOGE("%s open file %s failed\n", __func__,
				      file_name);
				break;
			}

			if (sprd_iq_write(fp, params->map_addr, length) !=
			    length) {
				fclose(fp);
				fp = NULL;
				ALOGI("Warning, IQ write failed\n");
			}

			ALOGI("import buffer from file finished\n");
			header->wr_rd_flag = IQ_AP_WRITE_FINISHED;
			params->state = IQ_AP_WRITE_FINISHED;
			break;

			/* Continue data, import it from current fp */
		case IQ_AP_WRITE_CONTINUE:
			params->state = IQ_AP_WRITE_CONTINUE;

			if (NULL == fp
			    || strncmp(iq_file, header->iq_file_name,
				       SPRD_IQ_FILE_NAME_LEN)) {
				strcpy(file_name, params->file_name);
				strncpy(iq_file, header->iq_file_name,
					SPRD_IQ_FILE_NAME_LEN);
				strncat(file_name, iq_file,
					SPRD_IQ_FILE_NAME_LEN);
				ALOGI("IQ_AP_WRITE_CONTINUE get file %s\n",
				      params->file_name);

				fp = fopen(file_name, "r");
				if (NULL == fp) {
					ALOGE("%s open file %s failed\n",
					      __func__, file_name);
					break;
				}
			}

			if (sprd_iq_write(fp, params->map_addr, length) !=
			    length) {
				fclose(fp);
				fp = NULL;
				ALOGI("Warning, IQ write failed\n");
			}

			ALOGI("import buffer from file finished\n");
			header->wr_rd_flag = IQ_AP_WRITE_FINISHED;
			params->state = IQ_AP_WRITE_FINISHED;
			break;

		default:
			ALOGE("Unknow flag 0x%x\n", header->wr_rd_flag);
			sleep(1);
			break;
		}
	}
}

/* Stop IQ data import */
void sprd_iq_stop(int arg)
{
	ALOGI("IQ data import exit\n");

	g_iq_params.thread_exit = 1;
	/* Waiting exit of thread */
	pthread_join(g_iq_params.thread, NULL);
	g_iq_params.state = 0;
	g_iq_params.started = 0;
}

/* Start IQ data import */
int sprd_iq_start(char *dir_path)
{
	int ret = 0;
	int mem_fd;

	/* Return if IQ data has started */
	if (1 == g_iq_params.started) {
		ALOGI("Warning: IQ data read has started\n");
		return 0;
	}

	/* Get IQ base at the first time */
	if (SPRD_INVALID_IQ_BASE == g_iq_params.base_addr) {
		int iq_fd;
		char iq_cbase[SPRD_IQ_BASE_LENGTH + 1] = { 0 };

		/* Get base address of IQ data through sysfs */
		iq_fd = open(SPRD_IQ_SYSFS, O_RDONLY);
		if (iq_fd < 0) {
			ALOGE("%s open IQ sysfs failed\n", __func__);
			return -1;
		}

		ret = read(iq_fd, iq_cbase, SPRD_IQ_BASE_LENGTH);
		if (ret < 0) {
			ALOGE("%s read IQ sysfs failed, error: %d\n", __func__,
			      ret);
			return -1;
		}

		close(iq_fd);

		g_iq_params.base_addr = strtoul(iq_cbase, (char **)NULL, 10);
		ALOGI("IQ base %s -- 0x%x\n", iq_cbase, g_iq_params.base_addr);

		/* 0 is a valid address? */
		if (0 == g_iq_params.base_addr) {
			ALOGE("%s IQ base address is 0 ?\n", __func__);
			g_iq_params.base_addr = SPRD_INVALID_IQ_BASE;
			return -1;
		}
	}

	mem_fd = open("/dev/iq_mem", O_RDWR | O_SYNC);
	if (-1 == mem_fd) {
		ALOGE("cannot open /dev/iq_mem\n");
		return -1;
	}

	/* Map kernel address to user space */
	g_iq_params.map_addr =
	    (uint8_t *) mmap(NULL, SPRD_IQ_BUFFER_SIZE, PROT_READ | PROT_WRITE,
			     MAP_SHARED, mem_fd, g_iq_params.base_addr);
	if (-1 == (int)g_iq_params.map_addr) {
		ALOGE("iq base mmap failed\n");
		return -1;
	}
	ALOGI("MMAP PA 0x%x --> VA 0x%x\n", g_iq_params.base_addr,
	      (uint32_t) g_iq_params.map_addr);

	memset(g_iq_params.file_name, 0, sizeof(g_iq_params.file_name));
	strcpy(g_iq_params.file_name, dir_path);
	g_iq_params.thread_exit = 0;
	g_iq_params.started = 1;
	ALOGI("IQ data monitor is running\n");

	/* run as daemon */
	if (daemon(0, 0)) {
		ALOGE("%s create daemon failed\n", __func__);
		return -1;
	}

	/* Signal handler for SIGTERM */
	signal(SIGTERM, sprd_iq_stop);

	/* Import IQ data here */
	sprd_iq_thread(&g_iq_params);

	return 0;
}

static int find_pid_by_name(char *ProcName, int *foundpid)
{
	DIR *dir;
	struct dirent *d;
	int pid, i;
	char *s;
	int pnlen;

	i = 0;
	foundpid[0] = 0;
	pnlen = strlen(ProcName);

	/* Open the /proc directory. */
	dir = opendir("/proc");
	if (!dir) {
		printf("cannot open /proc");
		return -1;
	}

	/* Walk through the directory. */
	while ((d = readdir(dir)) != NULL) {
		char exe[PATH_MAX + 1];
		char path[PATH_MAX + 1];
		int len;
		int namelen;

		/* See if this is a process */
		if ((pid = atoi(d->d_name)) == 0)
			continue;

		snprintf(exe, sizeof(exe), "/proc/%s/exe", d->d_name);
		ALOGI("find_pid_by_name  exe = %s.\n", exe);
		if ((len = readlink(exe, path, PATH_MAX)) < 0)
			continue;
		path[len] = '\0';

		/* Find ProcName */
		s = strrchr(path, '/');
		if (s == NULL)
			continue;
		s++;

		/* we don't need small name len */
		namelen = strlen(s);
		ALOGI("s = %s.\n", s);
		if (namelen < pnlen)
			continue;

		if (!strncmp(ProcName, s, pnlen)) {
			/* to avoid subname like search proc tao but proc taolinke matched */
			if (s[pnlen] == ' ' || s[pnlen] == '\0') {
				foundpid[i] = pid;
				i++;
			}
		}
	}

	foundpid[i] = 0;
	closedir(dir);

	return 0;
}

#define SPRD_IQ_THREADS_MAX         16

int main(int argc, char *argv[])
{
	char value[PROPERTY_VALUE_MAX] = { '\0' };
	int pid_t[SPRD_IQ_THREADS_MAX];

#if 0
	/* Check bootmode property */
	property_get("ro.bootmode", value, "");
	ALOGI("IQ data import, ro.bootmode : %s\n", value);

	if (!value[0] || strncmp(value, "iqmode", 6)) {
		ALOGI("IQ data import, bootmode is not IQ mode\n");
		return 0;
	}
#endif

	if (argc < 2) {
		ALOGE("invalid option for iqdata_daemon\n");
		return -1;
	}

	if (!strncasecmp(argv[1], "start", 5)) {
		if (!argv[2]) {
			ALOGE("It need directory for iqdata_daemon\n");
			return -1;
		}

		return sprd_iq_start(argv[2]);
	}

	if (!strncasecmp(argv[1], "stop", 4)) {
		int i;

		find_pid_by_name("iqdata_daemon", pid_t);

		for (i = 0; pid_t[i] != 0; i++)
			kill(pid_t[i], SIGTERM);

		munmap((void *)g_iq_params.base_addr, SPRD_IQ_BUFFER_SIZE);
	}

	return 0;
}
