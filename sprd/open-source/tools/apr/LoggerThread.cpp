
#include "common.h"
#include "LoggerThread.h"
#include "AprData.h"

struct log_device_t {
	const char* device;
	bool binary;
	struct logger *logger;
	struct logger_list *logger_list;
	bool printed;
	char label;

	log_device_t* next;

	log_device_t(const char* d, bool b, char l) {
		device = d;
		binary = b;
		label = l;
		next = NULL;
		printed = false;
	}
};

LoggerThread::LoggerThread(AprData* aprData)
{
	m_aprData = aprData;

	m_devices = NULL;
	m_logger_list = NULL;
	m_devCount = 0;
}

LoggerThread::~LoggerThread()
{
	m_aprData = NULL;
}

void LoggerThread::Setup()
{
	APR_LOGD("LoggerThread::Setup()\n");
	log_device_t* dev;
	int mode = O_RDONLY;

	dev = m_devices = new log_device_t("main", false, 'm');
	m_devCount = 1;
	if (android_name_to_log_id("system") == LOG_ID_SYSTEM) {
		dev = dev->next = new log_device_t("system", false, 's');
		m_devCount++;
	}
	if (android_name_to_log_id("crash") == LOG_ID_CRASH) {
		dev = dev->next = new log_device_t("crash", false, 'c');
		m_devCount++;
	}
	dev = m_devices;
	m_logger_list = android_logger_list_alloc(mode, 0, 0);

	while (dev) {
		dev->logger_list = m_logger_list;
		dev->logger = android_logger_open(m_logger_list,
				android_name_to_log_id(dev->device));
		if (!dev->logger) {
			APR_LOGE("Unable to open log device '%s'\n", dev->device);
			exit(EXIT_FAILURE);
		}

		dev = dev->next;
	}
}

void LoggerThread::Execute(void* arg)
{
	APR_LOGD("LoggerThread::Execute()\n");
	struct log_msg log_msg;
	log_device_t* dev;
	int ret;

	while (1) {
		ret = android_logger_list_read(m_logger_list, &log_msg);

		if (ret == 0) {
			APR_LOGE("read: Unexpected EOF!\n");
			exit(EXIT_FAILURE);
		}

		if (ret < 0) {
			if (ret == -EAGAIN) {
				break;
			}

			if (ret == -EIO) {
				APR_LOGE("read: Unexpected EOF!\n");
				exit(EXIT_FAILURE);
			}
			if (ret == -EINVAL) {
				APR_LOGE("read: Unexpected EOF!\n");
				exit(EXIT_FAILURE);
			}
			APR_LOGE("logcat read failure\n");
			exit(EXIT_FAILURE);
		}

		for(dev = m_devices; dev; dev = dev->next) {
			if (android_name_to_log_id(dev->device) == log_msg.id()) {
				break;
			}
		}
		if (!dev) {
			APR_LOGE("read: Unexpected log ID!\n");
			exit(EXIT_FAILURE);
		}
		processBuffer(dev, &log_msg);
	}

	android_logger_list_free(m_logger_list);
}

void LoggerThread::processBuffer(log_device_t* dev, struct log_msg *tbuf)
{
	static int32_t pid = 0;
	struct logger_entry* buf = &tbuf->entry_v1;
	if (dev->binary) {

	} else {
		/*
		 * format: <priority:1><tag:N>\0<message:N>\0
		 *
		 * tag str
		 *   starts at buf->msg+1
		 * msg
		 *   starts at buf->msg+1+len(tag)+1
		 *
		 * The message may have been truncated by the kernel log driver.
		 * When that happens, we must null-terminate the message ourselves.
		 */
		if (buf->len < 3) {
			// An well-formed entry must consist of at least a priority
			// and two null characters
			APR_LOGE("+++ LOG: entry too small\n");
			return;
		}

		int msgStart = -1;
		int msgEnd = -1;

		int i;
		char* msg = buf->msg;
		struct logger_entry_v2* buf2 = (struct logger_entry_v2 *)buf;
		if (buf2->hdr_size) {
			msg = ((char*)buf2) + buf2->hdr_size;
		}
		for (i = 1; i < buf->len; i++) {
			if (msg[i] == '\0') {
				if (msgStart == -1) {
					msgStart = i + 1;
				} else {
					msgEnd = i;
					break;
				}
			}
		}

		if (msgStart == -1) {
			APR_LOGE("+++ LOG: malformed log message\n");
			return;
		}
		if (msgEnd == -1) {
			// incoming message not null-terminated; force it
			msgEnd = buf->len - 1;
			msg[msgEnd] = '\0';
		}

		if (strstr(msg+1, "AndroidRuntime")) {

		//	printf("tag:%s,", msg+1);
		//	printf("pid:%d,", buf->pid);
		//	printf("msg:%s\n", msg+msgStart);

			if (pid == 0) {
				if (strstr(msg+msgStart, "Shutting down VM")) {
					pid = buf->pid;
				}
			} else {
				if (buf->pid == pid) {
					if (strstr(msg+msgStart, "FATAL EXCEPTION:")) {
						m_aprData->setChanged();
						m_aprData->notifyObservers((void*)"java crash");
						pid = 0;
					}
				} else {
					pid = 0;
				}
			}
		}
	}
}

