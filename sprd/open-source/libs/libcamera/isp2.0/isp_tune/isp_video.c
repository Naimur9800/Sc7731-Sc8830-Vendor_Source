/*
 * Copyright (C) 2012 The Android Open Source Project
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
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <fcntl.h>
#include <netdb.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <stdint.h>
#include <sys/types.h>
#include <semaphore.h>
#include <pthread.h>
#define LOG_TAG "isp-video"
#include <cutils/log.h>
#include <hardware/camera.h>

#include "isp_param_tune_com.h"
#include "isp_log.h"
#include "isp_video.h"
#include "isp_otp.h"

#define     UNUSED(param)  (void)(param)

enum {
	CMD_START_PREVIEW = 1,
	CMD_STOP_PREVIEW,
	CMD_READ_ISP_PARAM,
	CMD_WRITE_ISP_PARAM,
	CMD_GET_PREVIEW_PICTURE,
	CMD_AUTO_UPLOAD,
	CMD_UPLOAD_MAIN_INFO,
	CMD_TAKE_PICTURE,
	CMD_ISP_LEVEL,
	CMD_READ_SENSOR_REG,
	CMD_WRITE_SENSOR_REG,
	CMD_GET_INFO,
	CMD_OTP_WRITE,
	CMD_OTP_READ,
	CMD_SFT_READ,//15
	CMD_SFT_WRITE,//16
	CMD_SFT_TRIG,//17
	CMD_SFT_SET_POS,//18
	CMD_SFT_GET_POS,//19
	CMD_SFT_TAKE_PICTURE,//20
	CMD_SFT_TAKE_PICTURE_NEW,//21
	CMD_SFT_OPEN_FILTER,//22
	CMD_SFT_GET_AF_VALUE,//23
	
	CMD_ASIC_TAKE_PICTURE,//24
	CMD_ASIC_TAKE_PICTURE_NEW//25
};

// This is the communication frame head
typedef struct msg_head_tag
{
	unsigned int  seq_num;      // Message sequence number, used for flow control
	unsigned short  len;          // The totoal size of the packet "sizeof(MSG_HEAD_T)
	                      // + packet size"
	unsigned char   type;         // Main command type
	unsigned char   subtype;      // Sub command type
}__attribute__((packed)) MSG_HEAD_T;

typedef struct {
	uint32_t headlen;
	uint32_t img_format;
	uint32_t img_size;
	uint32_t totalpacket;
	uint32_t packetsn;
}ISP_IMAGE_HEADER_T;
//add
typedef struct {
	uint32_t totalpacket;
	uint32_t packetsn;
	uint32_t reserve[3];
}ISP_DATA_HEADER_T;

struct camera_func{
	int32_t(*start_preview) (uint32_t param1, uint32_t param2);
	int32_t(*stop_preview) (uint32_t param1, uint32_t param2);
	int32_t(*take_picture) (uint32_t param1, uint32_t param2);
	int32_t(*set_capture_size) (uint32_t width, uint32_t height);
};

#define PREVIEW_MAX_WIDTH 640
#define PREVIEW_MAX_HEIGHT 480

#define CMD_BUF_SIZE  65536 // 64k
#define SEND_IMAGE_SIZE 64512 // 63k
#define SEND_DATA_SIZE 64512 //63k
#define DATA_BUF_SIZE 65536 //64k
#define PORT_NUM 16666        /* Port number for server */
#define BACKLOG 5
#define ISP_CMD_SUCCESS             0x0000
#define ISP_CMD_FAIL                0x0001
#define IMAGE_RAW_TYPE 0
#define IMAGE_YUV_TYPE 1

#define CLIENT_DEBUG
#ifdef CLIENT_DEBUG
#define DBG ISP_LOGE
#endif
static uint8_t* preview_buf_ptr = 0;
static unsigned char diag_rx_buf[CMD_BUF_SIZE];
static unsigned char diag_cmd_buf[CMD_BUF_SIZE];
static unsigned char eng_rsp_diag[DATA_BUF_SIZE];
static int preview_flag = 0; // 1: start preview
static int preview_img_end_flag = 1; // 1: preview image send complete
static int capture_img_end_flag = 1; // 1: capture image send complete
static int capture_format = 1; // 1: start preview
static int capture_flag = 0; // 1: call get pic

static sem_t preview_sem_lock;
static sem_t capture_sem_lock;
static pthread_mutex_t ispstream_lock;
static int wire_connected = 0;
static int sockfd = 0;
static int sock_fd=0;
static int rx_packet_len=0;
static int rx_packet_total_len=0;
int sequence_num = 0;
struct camera_func s_camera_fun={PNULL, PNULL, PNULL, PNULL};
struct camera_func* s_camera_fun_ptr=&s_camera_fun;
static void* isp_handler;

static uint32_t g_af_pos=0;// the af position
static uint32_t g_type = 8;// the mipi
static uint32_t g_command=CMD_TAKE_PICTURE;

struct camera_func* ispvideo_GetCameraFunc(void)
{
	return s_camera_fun_ptr;
}

int ispvideo_SetreTurnValue(uint8_t* rtn_ptr, uint32_t rtn)
{
	uint32_t* rtn_value=(uint32_t*)rtn_ptr;

	*rtn_value=rtn;

	return 0x04;
}

uint32_t ispvideo_GetIspParamLenFromSt(unsigned char* dig_ptr)
{
	uint32_t data_len=0x00;
	uint16_t* data_ptr=(uint16_t*)(dig_ptr+0x05);

	if(NULL!=dig_ptr)
	{
		data_len=data_ptr[0];
		data_len-=0x08;
	}

	return data_len;
}

int ispvideo_GetIspParamFromSt(unsigned char* dig_ptr, struct isp_parser_buf_rtn* isp_ptr)
{
	int rtn=0x00;

	if((NULL != dig_ptr)
		&& (NULL != (void *)(isp_ptr->buf_addr))
		&& (0x00 != isp_ptr->buf_len))
	{
		memcpy((void*)isp_ptr->buf_addr, (void*)dig_ptr, isp_ptr->buf_len);
	}

	return rtn;
}

void*  ispvideo_GetIspHandle(void)
{
	return isp_handler;
}

uint32_t ispvideo_SetIspParamToSt(unsigned char* dig_ptr, struct isp_parser_buf_in* isp_ptr)
{
	uint32_t buf_len=0x00;

	if((NULL != dig_ptr)
		&& (NULL != (void *)(isp_ptr->buf_addr))
		&& (0x00!=isp_ptr->buf_len))
	{
		memcpy((void*)dig_ptr, (void*)isp_ptr->buf_addr, isp_ptr->buf_len);

		buf_len=isp_ptr->buf_len;
	}

	return buf_len;
}

static int handle_img_data(uint32_t format, uint32_t width,uint32_t height, char *ch0_ptr, int ch0_len,char *ch1_ptr, int ch1_len,char *ch2_ptr, int ch2_len)
{
	int i,  res, total_number;
	int send_number=0;
	int chn0_number, chn1_number, chn2_number;
	int len = 0, rlen = 0, rsp_len = 0, extra_len = 0;
	MSG_HEAD_T *msg_ret;
	ISP_IMAGE_HEADER_T isp_msg;

	chn0_number = (ch0_len + SEND_IMAGE_SIZE - 1) /SEND_IMAGE_SIZE;
	chn1_number = (ch1_len + SEND_IMAGE_SIZE - 1) /SEND_IMAGE_SIZE;
	chn2_number = (ch2_len + SEND_IMAGE_SIZE - 1) /SEND_IMAGE_SIZE;

	total_number = chn0_number + chn1_number + chn2_number;
	//total_number = (ch0_len + SEND_IMAGE_SIZE - 1) /SEND_IMAGE_SIZE;

	msg_ret = (MSG_HEAD_T *)(eng_rsp_diag+1);

	for (i=0; i<chn0_number; i++, send_number++)
	{// chn0
		if (i < chn0_number-1)
			len = SEND_IMAGE_SIZE;
		else
			len = ch0_len-SEND_IMAGE_SIZE*i;
		rsp_len = sizeof(MSG_HEAD_T)+1;

		// combine data
		isp_msg.headlen = 20;
		isp_msg.img_format = format;
		isp_msg.img_size = (width<<0x10)|height;
		isp_msg.totalpacket = total_number;
		isp_msg.packetsn = send_number+1;

		memcpy(eng_rsp_diag+rsp_len, (char *)&isp_msg, sizeof(ISP_IMAGE_HEADER_T));
		rsp_len += sizeof(ISP_IMAGE_HEADER_T);

		//DBG("%s:ISP_TOOL: request rsp_len[%d]\n",__FUNCTION__, rsp_len);
		memcpy(eng_rsp_diag+rsp_len, (char *)ch0_ptr+i*SEND_IMAGE_SIZE, len);
		rsp_len += len;

		eng_rsp_diag[rsp_len] = 0x7e;
		msg_ret->len = rsp_len-1;
		msg_ret->seq_num = sequence_num++;
		//DBG("%s:ISP_TOOL: request rsp_len[%d]\n",__FUNCTION__, rsp_len);
		res = send(sockfd, eng_rsp_diag, rsp_len+1, 0);

	}

	for (i=0; i<chn1_number; i++, send_number++)
	{// chn1
		if (i < chn1_number-1)
			len = SEND_IMAGE_SIZE;
		else
			len = ch1_len-SEND_IMAGE_SIZE*i;
		rsp_len = sizeof(MSG_HEAD_T)+1;

		// combine data
		isp_msg.headlen = 20;
		isp_msg.img_format = format;
		isp_msg.img_size = (width<<0x10)|height;
		isp_msg.totalpacket = total_number;
		isp_msg.packetsn = send_number+1;

		memcpy(eng_rsp_diag+rsp_len, (char *)&isp_msg, sizeof(ISP_IMAGE_HEADER_T));
		rsp_len += sizeof(ISP_IMAGE_HEADER_T);

		//DBG("%s:ISP_TOOL: request rsp_len[%d]\n",__FUNCTION__, rsp_len);
		memcpy(eng_rsp_diag+rsp_len, (char *)ch1_ptr+i*SEND_IMAGE_SIZE, len);
		rsp_len += len;

		eng_rsp_diag[rsp_len] = 0x7e;
		msg_ret->len = rsp_len-1;
		msg_ret->seq_num = sequence_num++;
		//DBG("%s:ISP_TOOL: request rsp_len[%d]\n",__FUNCTION__, rsp_len);
		res = send(sockfd, eng_rsp_diag, rsp_len+1, 0);

	}

	for (i=0; i<chn2_number; i++, send_number++)
	{// chn2
		if (i < chn2_number-1)
			len = SEND_IMAGE_SIZE;
		else
			len = ch2_len-SEND_IMAGE_SIZE*i;
		rsp_len = sizeof(MSG_HEAD_T)+1;

		// combine data
		isp_msg.headlen = 20;
		isp_msg.img_format = format;
		isp_msg.img_size = (width<<0x10)|height;
		isp_msg.totalpacket = total_number;
		isp_msg.packetsn = send_number+1;

		memcpy(eng_rsp_diag+rsp_len, (char *)&isp_msg, sizeof(ISP_IMAGE_HEADER_T));
		rsp_len += sizeof(ISP_IMAGE_HEADER_T);

		//DBG("%s:ISP_TOOL: request rsp_len[%d]\n",__FUNCTION__, rsp_len);
		memcpy(eng_rsp_diag+rsp_len, (char *)ch2_ptr+i*SEND_IMAGE_SIZE, len);
		rsp_len += len;

		eng_rsp_diag[rsp_len] = 0x7e;
		msg_ret->len = rsp_len-1;
		msg_ret->seq_num = sequence_num++;
		//DBG("%s:ISP_TOOL: request rsp_len[%d]\n",__FUNCTION__, rsp_len);
		res = send(sockfd, eng_rsp_diag, rsp_len+1, 0);

	}

	return 0;
}

static int handle_img_data_asic(uint32_t format, uint32_t width,uint32_t height, char *ch0_ptr, int ch0_len,char *ch1_ptr, int ch1_len,char *ch2_ptr, int ch2_len)
{
	UNUSED(format);
	UNUSED(width);
	UNUSED(height);
	char name[100] = {'\0'};
	FILE* fp = NULL;
	switch( capture_format ){
	case 8:// the mipi
		sprintf(name,"/data/internal-asec/the_pic_%u.mipi_raw",g_af_pos);
		break;
	case 16://jpeg
		sprintf(name,"/data/internal-asec/the_pic_%u.jpg",g_af_pos);
		break;
	}
	fp = fopen(name,"wb");

	if( fp==NULL ){
		ISP_LOGI("Chj--create file fails");
		return 0;
	}

	fwrite(ch0_ptr, 1,ch0_len,fp);

	fwrite(ch1_ptr, 1,ch1_len,fp);

	fwrite(ch2_ptr, 1,ch2_len,fp);
	fclose(fp);
	ISP_LOGI("Chj--writing one pic succeeds");
	return 0;
}

static int handle_img_data_sft(uint32_t format, uint32_t width,uint32_t height, char *ch0_ptr, int ch0_len,char *ch1_ptr, int ch1_len,char *ch2_ptr, int ch2_len)
{
	UNUSED(format);
	UNUSED(width);
	UNUSED(height);
	char name[100] = {'\0'};
	FILE* fp = NULL;
	switch( capture_format ){
	case 8:// the mipi
		sprintf(name,"/data/misc/media/the_pic_%u.mipi_raw",g_af_pos);
		break;
	case 16://jpeg
		sprintf(name,"/data/misc/media/the_pic_%u.jpg",g_af_pos);
		break;
	}
	fp = fopen(name,"wb");

	if( fp==NULL ){
		ISP_LOGI("Chj--create file fails");
		return 0;
	}

	fwrite(ch0_ptr, 1,ch0_len,fp);

	fwrite(ch1_ptr, 1,ch1_len,fp);

	fwrite(ch2_ptr, 1,ch2_len,fp);
	fclose(fp);
	ISP_LOGI("Chj--writing one pic succeeds");
	return 0;
}

int isp_sft_read(isp_handle handler, uint8_t *data_buf, uint32_t* data_size){
	int ret = 0;
	struct isp_parser_buf_rtn rtn_param = {0x00, 0x00};

	rtn_param.buf_addr = (unsigned long)data_buf;
	
	ret = isp_ioctl(handler, ISP_CTRL_SFT_READ, (void*)&rtn_param);// get af info after auto focus

	*data_size = rtn_param.buf_len;

	return ret;
}
int isp_sft_write(isp_handle handler, uint8_t *data_buf, uint32_t *data_size){//DATA
	int ret = 0;
	struct isp_parser_buf_rtn rtn_param = {0x00, 0x00};
	rtn_param.buf_addr = (unsigned long)data_buf;
	rtn_param.buf_len = *data_size;
	ret = isp_ioctl(handler, ISP_CTRL_SFT_WRITE, (void*)&rtn_param);// set af info after auto focus

	return ret;
}

static int handle_isp_data(unsigned char *buf, unsigned int len)
{
	uint32_t handler_id=0x00;
	int rlen = 0, rsp_len = 0, extra_len = 0;
	int ret = 1, res = 0;
	int image_type = 0;
	MSG_HEAD_T *msg, *msg_ret;
	//add
	ISP_DATA_HEADER_T isp_msg;
	int preview_tmpflag = 0;
	//
	struct camera_func* fun_ptr=ispvideo_GetCameraFunc();
	int is_stop_preview = 0;

	if (len < sizeof(MSG_HEAD_T)+2){
		DBG("ISP_TOOL:the formal cmd is 0x7e + diag + 0x7e,which is 10Bytes,but the cmd has less than 10 bytes\n");
		return -1;
	}

	msg = (MSG_HEAD_T *)(buf+1);
	if(msg->type != 0xfe)
		return -1;

	rsp_len = sizeof(MSG_HEAD_T)+1;
	memset(eng_rsp_diag,0,sizeof(eng_rsp_diag));
	memcpy(eng_rsp_diag,buf,rsp_len);
	msg_ret = (MSG_HEAD_T *)(eng_rsp_diag+1);

	if(CMD_GET_PREVIEW_PICTURE != msg->subtype) {
		msg_ret->seq_num = sequence_num++;
	}

	switch ( msg->subtype ) {
		case CMD_SFT_READ:
		{
			ISP_LOGI("Chj--get af info");
			ret = isp_sft_read(isp_handler, &eng_rsp_diag[rsp_len], &len);
			DBG("ISP_SFT:CMD_SFT_READ rsp_len %d len %d\n",rsp_len,len);
			rsp_len += len;
			eng_rsp_diag[rsp_len] = 0x7e;
			msg_ret->len = rsp_len-1;
			res = send(sockfd, eng_rsp_diag, rsp_len+1, 0);
			break;
		}
		case CMD_SFT_WRITE:
		{
			ISP_LOGI("Chj--set af info");
			len = msg_ret->len - 8;
			ret = isp_sft_write(isp_handler,buf+rsp_len, &len);
			DBG("ISP_SFT:CMD_SFT_WRITE rsp_len %d len %d\n",rsp_len,len);
			rsp_len+=ispvideo_SetreTurnValue((uint8_t*)&eng_rsp_diag[rsp_len], ISP_CMD_SUCCESS);
			eng_rsp_diag[rsp_len] = 0x7e;
			msg_ret->len = rsp_len-1;
			res = send(sockfd, eng_rsp_diag, rsp_len+1, 0);
			break;
		}
		case CMD_SFT_TRIG:
		{
			ISP_LOGI("Chj--af trig");
			ret = isp_ioctl(isp_handler, ISP_CTRL_AF, NULL);// set af info after auto focus
			if(!ret){
				rsp_len+=ispvideo_SetreTurnValue((uint8_t*)&eng_rsp_diag[rsp_len], ISP_CMD_SUCCESS);
			}else{
				rsp_len+=ispvideo_SetreTurnValue((uint8_t*)&eng_rsp_diag[rsp_len], ISP_CMD_FAIL);
			}
			eng_rsp_diag[rsp_len] = 0x7e;
			msg_ret->len = rsp_len-1;
			res = send(sockfd, eng_rsp_diag, rsp_len+1, 0);
			break;
		}
		case CMD_SFT_SET_POS:
		{
			ISP_LOGI("Chj--set pos");
			uint32_t bypass = 0;
			g_af_pos = *(uint32_t*)(buf+9);
			ret = isp_ioctl(isp_handler, ISP_CTRL_SET_AF_POS, buf+9);
			ret = isp_ioctl(isp_handler, ISP_CTRL_SFT_SET_PASS, NULL);// open the filter
			usleep(1000*100);
			if(!ret){
				rsp_len+=ispvideo_SetreTurnValue((uint8_t*)&eng_rsp_diag[rsp_len], ISP_CMD_SUCCESS);
			}else{
				rsp_len+=ispvideo_SetreTurnValue((uint8_t*)&eng_rsp_diag[rsp_len], ISP_CMD_FAIL);
			}
			eng_rsp_diag[rsp_len] = 0x7e;
			msg_ret->len = rsp_len-1;
			res = send(sockfd, eng_rsp_diag, rsp_len+1, 0);
			break;
		}
		case CMD_SFT_GET_POS:
		{
			ISP_LOGI("Chj--get pos");
			uint32_t pos;
			ret = isp_ioctl(isp_handler, ISP_CTRL_GET_AF_POS, &pos);// set af info after auto focus
			if(!ret){
				rsp_len+=ispvideo_SetreTurnValue((uint8_t*)&eng_rsp_diag[rsp_len], ISP_CMD_SUCCESS);
			}else{
				rsp_len+=ispvideo_SetreTurnValue((uint8_t*)&eng_rsp_diag[rsp_len], ISP_CMD_FAIL);
			}
			*(uint32_t*)(eng_rsp_diag+rsp_len) = pos;
			rsp_len += sizeof(uint32_t);
			eng_rsp_diag[rsp_len] = 0x7e;
			msg_ret->len = rsp_len-1;
			res = send(sockfd, eng_rsp_diag, rsp_len+1, 0);
			break;
		}
		case CMD_SFT_OPEN_FILTER:
		{
			ISP_LOGI("Chj--open the filter");
			ret = isp_ioctl(isp_handler, ISP_CTRL_SFT_SET_PASS, NULL);// open the filter
			usleep(1000*100);
			if(!ret){
				rsp_len+=ispvideo_SetreTurnValue((uint8_t*)&eng_rsp_diag[rsp_len], ISP_CMD_SUCCESS);
			}else{
				rsp_len+=ispvideo_SetreTurnValue((uint8_t*)&eng_rsp_diag[rsp_len], ISP_CMD_FAIL);
			}
			eng_rsp_diag[rsp_len] = 0x7e;
			msg_ret->len = rsp_len-1;
			res = send(sockfd, eng_rsp_diag, rsp_len+1, 0);
			break;
		}
		case CMD_SFT_GET_AF_VALUE:// value back to PC
		{
			uint32_t statistic[50]={0};
			ISP_LOGI("Chj-- get af value 1");
			ret = isp_ioctl(isp_handler, ISP_CTRL_SFT_GET_AF_VALUE,statistic);// set af info after auto focus
			ISP_LOGI("Chj-- get af value 2 ret=%d,af_value=%d,af_value=%d",ret,statistic[0],statistic[25]);
			//ret = isp_ioctl(isp_handler, ISP_CTRL_SFT_SET_BYPASS, NULL);// close the filter
			if(!ret){
				rsp_len+=ispvideo_SetreTurnValue((uint8_t*)&eng_rsp_diag[rsp_len], ISP_CMD_SUCCESS);
				memmove(eng_rsp_diag+rsp_len,statistic,sizeof(statistic));
				rsp_len+=sizeof(statistic);
			}else{
				rsp_len+=ispvideo_SetreTurnValue((uint8_t*)&eng_rsp_diag[rsp_len], ISP_CMD_FAIL);
			}
			eng_rsp_diag[rsp_len] = 0x7e;
			msg_ret->len = rsp_len-1;
			res = send(sockfd, eng_rsp_diag, rsp_len+1, 0);
			break;
		}
		case CMD_SFT_TAKE_PICTURE:// pic back to PC
		{
			ISP_LOGI("Chj--sft take picture");
			uint8_t* isp_ptr=buf+sizeof(MSG_HEAD_T)+1;
			uint32_t width,height,interval;

			g_command = CMD_SFT_TAKE_PICTURE;
			if( NULL!=fun_ptr->take_picture ) {
				capture_img_end_flag=0;
				capture_flag=1;
				capture_format=*(uint32_t*)isp_ptr;// capture format
				width = (*(uint32_t*)(isp_ptr+4))>>16;
				height = (*(uint32_t*)(isp_ptr+4))&0x0000ffff;
				interval = *(uint32_t*)(isp_ptr+8);
				usleep(1000*interval);
				if(NULL!=fun_ptr->set_capture_size)
				{
					fun_ptr->set_capture_size(width,height);
				}
				fun_ptr->take_picture(0, capture_format);
				ISP_LOGI("Chj--1 width=%d,height=%d,capture_format=%d",width,height,capture_format);
				sem_wait(&capture_sem_lock);
				ISP_LOGI("Chj--2 width=%d,height=%d,capture_format=%d",width,height,capture_format);
				if(NULL!=fun_ptr->stop_preview){
					fun_ptr->stop_preview(0, 0);
				}
				usleep(1000*10);

				if(NULL!=fun_ptr->start_preview){
					fun_ptr->start_preview(0, 0);
				}

				capture_flag = 0;
				g_command = CMD_TAKE_PICTURE;
			}
			break;
		}
		case CMD_SFT_TAKE_PICTURE_NEW:// save pic to file system
		{
			ISP_LOGI("Chj--sft take picture new");
			uint8_t* isp_ptr=buf+sizeof(MSG_HEAD_T)+1;
			uint32_t startpos,endpos,step,width,height,interval;
			uint32_t pos;
			uint32_t statistic[50]={0};
			g_command = CMD_SFT_TAKE_PICTURE_NEW;
			if( NULL!=fun_ptr->take_picture ) {
				ret = isp_ioctl(isp_handler, ISP_CTRL_STOP_3A, NULL);
				////////////////////////////
				startpos = *(uint32_t*)isp_ptr;
				endpos = *(uint32_t*)(isp_ptr+4);
				step = *(uint32_t*)(isp_ptr+8);
				capture_format=*(uint32_t*)(isp_ptr+12);// capture format
				width = (*(uint32_t*)(isp_ptr+16))>>16;
				height = (*(uint32_t*)(isp_ptr+16))&0x0000ffff;
				interval = *(uint32_t*)(isp_ptr+20);
				for( pos=startpos;pos<=endpos;pos+=step ){
					g_af_pos = pos;
					ret = isp_ioctl(isp_handler, ISP_CTRL_SET_AF_POS, &pos);// set af pos after auto focus
					usleep(1000*10);
					capture_img_end_flag=0;
					capture_flag=1;
					usleep(1000*interval);
					if(NULL!=fun_ptr->set_capture_size)
					{
						fun_ptr->set_capture_size(width,height);
					}
					fun_ptr->take_picture(0, capture_format);
					ISP_LOGI("Chj--1 width=%d,height=%d,capture_format=%d",width,height,capture_format);
					sem_wait(&capture_sem_lock);
					ISP_LOGI("Chj--2 width=%d,height=%d,capture_format=%d",width,height,capture_format);
					if(NULL!=fun_ptr->stop_preview){
						fun_ptr->stop_preview(0, 0);
					}
					usleep(1000*10);

					if(NULL!=fun_ptr->start_preview){
						fun_ptr->start_preview(0, 0);
					}
					capture_flag = 0;

				}
				g_command = CMD_TAKE_PICTURE;
				ret = isp_ioctl(isp_handler, ISP_CTRL_START_3A, NULL);
			}
			break;
		}
		case CMD_ASIC_TAKE_PICTURE:// save pic to file system
		{
			ISP_LOGI("Chj--asic take picture");
			uint8_t* isp_ptr=buf+sizeof(MSG_HEAD_T)+1;
			uint32_t width,height;
			g_command = CMD_ASIC_TAKE_PICTURE;
			if( NULL!=fun_ptr->take_picture ) {
				capture_img_end_flag=0;
				capture_flag=1;
				capture_format=*(uint32_t*)isp_ptr;// capture format
				width = (*(uint32_t*)(isp_ptr+4))>>16;
				height = (*(uint32_t*)(isp_ptr+4))&0x0000ffff;
				if(NULL!=fun_ptr->set_capture_size)
				{
					fun_ptr->set_capture_size(width,height);
				}
				fun_ptr->take_picture(0, capture_format);
				ISP_LOGI("Chj--1 width=%d,height=%d,capture_format=%d",width,height,capture_format);
				sem_wait(&capture_sem_lock);
				ISP_LOGI("Chj--2 width=%d,height=%d,capture_format=%d",width,height,capture_format);
				if(NULL!=fun_ptr->stop_preview){
					fun_ptr->stop_preview(0, 0);
				}
				usleep(1000*10);

				if(NULL!=fun_ptr->start_preview){
					fun_ptr->start_preview(0, 0);
				}

				capture_flag = 0;
				g_command = CMD_TAKE_PICTURE;
				
				rsp_len+=ispvideo_SetreTurnValue((uint8_t*)&eng_rsp_diag[rsp_len], ISP_CMD_SUCCESS);
				eng_rsp_diag[rsp_len] = 0x7e;
				msg_ret->len = rsp_len-1;
				res = send(sockfd, eng_rsp_diag, rsp_len+1, 0);
			}
			break;
		}
		case CMD_ASIC_TAKE_PICTURE_NEW:// save pic to file system
		{
			ISP_LOGI("Chj--asic take picture new");
			uint8_t* isp_ptr=buf+sizeof(MSG_HEAD_T)+1;;
			uint32_t startpos,endpos,step,width,height;
			uint32_t pos;
			g_command = CMD_ASIC_TAKE_PICTURE_NEW;
			if( NULL!=fun_ptr->take_picture ) {
				////////////////////////////
				startpos = *(uint32_t*)isp_ptr;
				endpos = *(uint32_t*)(isp_ptr+4);
				step = *(uint32_t*)(isp_ptr+8);
				capture_format=*(uint32_t*)(isp_ptr+12);// capture format
				width = (*(uint32_t*)(isp_ptr+16))>>16;
				height = (*(uint32_t*)(isp_ptr+16))&0x0000ffff;

				for( pos=startpos;pos<=endpos;pos+=step ){
					g_af_pos = pos;
					ret = isp_ioctl(isp_handler, ISP_CTRL_SET_AF_POS, &pos);// set af pos after auto focus
					usleep(1000*10);
					capture_img_end_flag=0;
					capture_flag=1;
					if(NULL!=fun_ptr->set_capture_size)
					{
						fun_ptr->set_capture_size(width,height);
					}
					fun_ptr->take_picture(0, capture_format);
					ISP_LOGI("Chj--1 width=%d,height=%d,capture_format=%d",width,height,capture_format);
					sem_wait(&capture_sem_lock);
					ISP_LOGI("Chj--2 width=%d,height=%d,capture_format=%d",width,height,capture_format);
					if(NULL!=fun_ptr->stop_preview){
						fun_ptr->stop_preview(0, 0);
					}
					usleep(1000*10);

					if(NULL!=fun_ptr->start_preview){
						fun_ptr->start_preview(0, 0);
					}
					capture_flag = 0;
				}
				g_command = CMD_TAKE_PICTURE;
			}
			break;
		}
		case CMD_START_PREVIEW:
		{// ok
			DBG("ISP_TOOL:CMD_START_PREVIEW \n");
			if(NULL!=fun_ptr->start_preview){
				ret = fun_ptr->start_preview(0, 0);
			}
			if(!ret){
				rsp_len+=ispvideo_SetreTurnValue((uint8_t*)&eng_rsp_diag[rsp_len], ISP_CMD_SUCCESS);
				preview_flag = 1;
			}else{
				rsp_len+=ispvideo_SetreTurnValue((uint8_t*)&eng_rsp_diag[rsp_len], ISP_CMD_FAIL);
			}
			eng_rsp_diag[rsp_len] = 0x7e;
			msg_ret->len = rsp_len-1;
			res = send(sockfd, eng_rsp_diag, rsp_len+1, 0);
			break;
		}
		case CMD_STOP_PREVIEW:
		{// ok
			DBG("ISP_TOOL:CMD_STOP_PREVIEW \n");
			if(NULL!=fun_ptr->stop_preview){
//				fun_ptr->stop_preview(0, 0);
			}

			preview_flag = 0;
			rsp_len+=ispvideo_SetreTurnValue((uint8_t*)&eng_rsp_diag[rsp_len], ISP_CMD_SUCCESS);
			eng_rsp_diag[rsp_len] = 0x7e;
			msg_ret->len = rsp_len-1;
			res = send(sockfd, eng_rsp_diag, rsp_len+1, 0);
			break;
		}
		case CMD_GET_PREVIEW_PICTURE:
		{ // ok
			//DBG("ISP_TOOL:CMD_GET_PREVIEW_PICTURE \n");
			if(1==preview_flag)
			{
				preview_img_end_flag = 0;
				sem_wait(&preview_sem_lock);
				preview_img_end_flag = 1;
			} else {
				DBG("ISP_TOOL:CMD_GET_PREVIEW_PICTURE \n");
				rsp_len += rlen;
				eng_rsp_diag[rsp_len] = 0x7e;
				msg_ret->len = rsp_len-1;
				res = send(sockfd, eng_rsp_diag, rsp_len+1, 0);
			}
			break;
		}
		case CMD_READ_ISP_PARAM:
		{
		DBG("ISP_TOOL:CMD_READ_ISP_PARAM \n");
		struct isp_parser_buf_in in_param = {0x00, 0x00};
		struct isp_parser_buf_rtn rtn_param = {0x00, 0x00};
		struct isp_parser_cmd_param rtn_cmd;
		uint32_t packet_num = 0;
		uint32_t send_number = 0;
		uint8_t* dig_ptr=buf;
		uint32_t i = 0;
		uint8_t* isp_ptr=buf+sizeof(MSG_HEAD_T)+1;

			memset(&rtn_cmd, 0, sizeof(rtn_cmd));
			in_param.buf_len=ispvideo_GetIspParamLenFromSt(dig_ptr);
			in_param.buf_addr=(unsigned long)ispParserAlloc(in_param.buf_len);

			if((0x00!=in_param.buf_len)
				&&(0x00!=in_param.buf_addr))
			{
				ret=ispvideo_GetIspParamFromSt(isp_ptr, (struct isp_parser_buf_rtn*)&in_param);
				ret=ispParser(isp_handler, ISP_PARSER_DOWN, (void*)in_param.buf_addr, (void*)&rtn_cmd);
				ret=ispParserFree((void*)in_param.buf_addr);

			if(ISP_UP_PARAM==rtn_cmd.cmd)
			{
				ret=ispParser(isp_handler, ISP_PARSER_UP_PARAM, (void*)in_param.buf_addr, (void*)&rtn_param);
				
				if(0x00==ret)
				{

						packet_num = (rtn_param.buf_len+SEND_DATA_SIZE-1)/SEND_DATA_SIZE;
					for (i=0; i<packet_num; i++, send_number++)
					{
						if (i < (packet_num-1))
								len = SEND_DATA_SIZE;
						else
								len = rtn_param.buf_len -SEND_DATA_SIZE*i;

						rsp_len = sizeof(MSG_HEAD_T)+1;

						isp_msg.totalpacket = packet_num;
						isp_msg.packetsn = send_number;
						
						memcpy(eng_rsp_diag+rsp_len, (char *)&isp_msg, sizeof(ISP_DATA_HEADER_T));
						rsp_len += sizeof(ISP_DATA_HEADER_T);

		
						//DBG("%s:ISP_TOOL: request rsp_len[%d]\n",__FUNCTION__, rsp_len);
							memcpy(eng_rsp_diag+rsp_len, (char *)rtn_param.buf_addr+i*SEND_DATA_SIZE, len);

						
						rsp_len += len;
						eng_rsp_diag[rsp_len] = 0x7e;
						msg_ret->len = rsp_len-1;						
						msg_ret->seq_num = sequence_num++;
						res = send(sockfd, eng_rsp_diag, rsp_len+1, 0);

					}
						
				}
			}
		}


		break;
	}
		case CMD_WRITE_ISP_PARAM:
		{
			DBG("ISP_TOOL:CMD_WRITE_ISP_PARAM \n");
			struct isp_parser_buf_in in_param = {0x00, 0x00};
			static struct isp_parser_buf_in packet_param = {0x00, 0x00};
			uint8_t* dig_ptr=buf;
			uint8_t* isp_ptr=buf+sizeof(MSG_HEAD_T)+sizeof(ISP_DATA_HEADER_T)+1;
			uint32_t* packet = (uint32_t*)(buf+sizeof(MSG_HEAD_T)+1);
			uint32_t packet_num = 0;
			static uint32_t offset = 0;
			uint32_t param_len = 0;
			uint32_t packet_total = 0;
			packet_total = packet[0];
			packet_num = packet[1];
			if(0==packet_num) {
				packet_param.buf_addr = (unsigned long)ispParserAlloc(packet_total*SEND_DATA_SIZE);
				packet_param.buf_len = packet_total*SEND_DATA_SIZE;
			}

			in_param.buf_len=ispvideo_GetIspParamLenFromSt(dig_ptr)-sizeof(ISP_DATA_HEADER_T);
			in_param.buf_addr=(unsigned long)ispParserAlloc(in_param.buf_len);

			if((0x00!=in_param.buf_len)
				&&(0x00!=in_param.buf_addr))
			{
				if(NULL!=fun_ptr->stop_preview){
//					fun_ptr->stop_preview(0, 0);
				}

				ret=ispvideo_GetIspParamFromSt(isp_ptr, (struct isp_parser_buf_rtn*)&in_param);
				memcpy((char*)packet_param.buf_addr+offset,(void*)in_param.buf_addr,in_param.buf_len);	
				offset += in_param.buf_len;

				if(in_param.buf_addr) {
					ret=ispParserFree((void*)in_param.buf_addr);
				}
				if(NULL!=fun_ptr->start_preview){
//					fun_ptr->start_preview(0, 0);
				}
			}

			if(packet_num==(packet_total-1)) {
				offset = 0;
				DBG("ISP_TOOL:CMD_WRITE_ISP_PARAM1 \n");
				ret=ispParser(isp_handler, ISP_PARSER_DOWN, (void*)packet_param.buf_addr, NULL);
				DBG("ISP_TOOL:CMD_WRITE_ISP_PARAM2  ret  %d\n",ret);
				ret=ispParserFree((void*)packet_param.buf_addr);
				rsp_len+=ispvideo_SetreTurnValue((uint8_t*)&eng_rsp_diag[rsp_len], ret);
				eng_rsp_diag[rsp_len] = 0x7e;
				msg_ret->len = rsp_len-1;
				DBG("ISP_TOOL:CMD_WRITE_ISP_PARAM3 \n");
				res = send(sockfd, eng_rsp_diag, rsp_len+1, 0);
				DBG("ISP_TOOL:CMD_WRITE_ISP_PARAM4 \n");

			}

			break;
		}
		case CMD_UPLOAD_MAIN_INFO:
		{ // ok
			DBG("ISP_TOOL:CMD_UPLOAD_MAIN_INFO \n");
			/* TODO:read isp param operation */
			// rlen is the size of isp_param
			// pass eng_rsp_diag+rsp_len

			struct isp_parser_buf_in in_param = {0x00, 0x00};
			struct isp_parser_buf_rtn rtn_param = {0x00, 0x00};
			struct isp_parser_cmd_param rtn_cmd;
			uint8_t* dig_ptr=buf;
			uint8_t* isp_ptr=buf+sizeof(MSG_HEAD_T)+1;
			uint8_t i=0x00;
			uint32_t* addr=(uint32_t*)isp_ptr;

			memset(&rtn_cmd, 0, sizeof(rtn_cmd));
			in_param.buf_len=ispvideo_GetIspParamLenFromSt(dig_ptr);
			in_param.buf_addr=(unsigned long)ispParserAlloc(in_param.buf_len);

			if((0x00!=in_param.buf_len)
				&&(0x00!=in_param.buf_addr))
			{
				ret=ispvideo_GetIspParamFromSt(isp_ptr, (struct isp_parser_buf_rtn*)&in_param);
				ret=ispParser(isp_handler, ISP_PARSER_DOWN, (void*)in_param.buf_addr, (void*)&rtn_cmd);
				ret=ispParserFree((void*)in_param.buf_addr);

				if(ISP_MAIN_INFO==rtn_cmd.cmd)
				{
					ret=ispParser(isp_handler, ISP_PARSER_UP_MAIN_INFO, (void*)in_param.buf_addr, (void*)&rtn_param);
					if(0x00==ret)
					{
						isp_ptr=eng_rsp_diag+sizeof(MSG_HEAD_T)+1;
						rlen=ispvideo_SetIspParamToSt(isp_ptr, (struct isp_parser_buf_in*)&rtn_param);
						ret=ispParserFree((void*)rtn_param.buf_addr);
					}
				}
			}

			rsp_len += rlen;
			eng_rsp_diag[rsp_len] = 0x7e;
			msg_ret->len = rsp_len-1;
			res = send(sockfd, eng_rsp_diag, rsp_len+1, 0);
			break ;
		}
		case CMD_TAKE_PICTURE:
		{
			DBG("ISP_TOOL:CMD_TAKE_PICTURE\n");
			struct isp_parser_buf_in in_param = {0x00, 0x00};
			struct isp_parser_buf_rtn rtn_param = {0x00, 0x00};
			struct isp_parser_cmd_param rtn_cmd;
			uint8_t* dig_ptr=buf;
			uint8_t* isp_ptr=buf+sizeof(MSG_HEAD_T)+1;

			memset(&rtn_cmd, 0, sizeof(rtn_cmd));
			in_param.buf_len=ispvideo_GetIspParamLenFromSt(dig_ptr);
			in_param.buf_addr=(unsigned long)ispParserAlloc(in_param.buf_len);

			if((0x00!=in_param.buf_len)
				&&(0x00!=in_param.buf_addr))
			{
				ret=ispvideo_GetIspParamFromSt(isp_ptr, (struct isp_parser_buf_rtn*)&in_param);
				ret=ispParser(isp_handler, ISP_PARSER_DOWN, (void*)in_param.buf_addr, (void*)&rtn_cmd);
				ret=ispParserFree((void*)in_param.buf_addr);

				if((ISP_CAPTURE==rtn_cmd.cmd)
					&&(NULL!=fun_ptr->take_picture)) {

					capture_img_end_flag=0;
					capture_flag=1;

					capture_format=rtn_cmd.param[0];// capture format

					if(NULL!=fun_ptr->set_capture_size)
					{
						fun_ptr->set_capture_size(rtn_cmd.param[1], rtn_cmd.param[2]);
					}

					fun_ptr->take_picture(0, capture_format);
					sem_wait(&capture_sem_lock);

					if(NULL!=fun_ptr->stop_preview){
						fun_ptr->stop_preview(0, 0);
					}

					usleep(1000*10);

					if(NULL!=fun_ptr->start_preview){
						fun_ptr->start_preview(0, 0);
					}

				}
			}

			capture_flag = 0;
			break;
		}
		case CMD_ISP_LEVEL:
		{ // ok need test
			DBG("ISP_TOOL:CMD_ISP_LEVEL \n");
			struct isp_parser_buf_in in_param = {0x00, 0x00};
			struct isp_parser_cmd_param rtn_cmd;
			uint8_t* dig_ptr=buf;
			uint8_t* isp_ptr=buf+sizeof(MSG_HEAD_T)+1;

			memset(&rtn_cmd, 0, sizeof(rtn_cmd));
			in_param.buf_len=ispvideo_GetIspParamLenFromSt(dig_ptr);
			in_param.buf_addr=(unsigned long)ispParserAlloc(in_param.buf_len);

			if((0x00!=in_param.buf_len)
				&&(0x00!=in_param.buf_addr))
			{
				ret=ispvideo_GetIspParamFromSt(isp_ptr, (struct isp_parser_buf_rtn*)&in_param);
				ret=ispParser(isp_handler, ISP_PARSER_DOWN, (void*)in_param.buf_addr, (void*)&rtn_cmd);
				ispParserFree((void*)in_param.buf_addr);
			}

			rsp_len+=ispvideo_SetreTurnValue((uint8_t*)&eng_rsp_diag[rsp_len], ret);
			eng_rsp_diag[rsp_len] = 0x7e;
			msg_ret->len = rsp_len-1;
			res = send(sockfd, eng_rsp_diag, rsp_len+1, 0);
			break;
		}

		case CMD_READ_SENSOR_REG:
		{// ok need test
			DBG("ISP_TOOL:CMD_READ_SENSOR_REG \n");
			struct isp_parser_buf_in in_param = {0x00, 0x00};
			struct isp_parser_cmd_param rtn_cmd;
			struct isp_parser_buf_rtn rtn_param = {0x00, 0x00};
			uint8_t* dig_ptr=buf;
			uint8_t* isp_ptr=buf+sizeof(MSG_HEAD_T)+1;

			memset(&rtn_cmd, 0, sizeof(rtn_cmd));
			in_param.buf_len=ispvideo_GetIspParamLenFromSt(dig_ptr);
			in_param.buf_addr=(unsigned long)ispParserAlloc(in_param.buf_len);

			if((0x00!=in_param.buf_len)
				&&(0x00!=in_param.buf_addr))
			{
				ret=ispvideo_GetIspParamFromSt(isp_ptr, (struct isp_parser_buf_rtn*)&in_param);
				ret=ispParser(isp_handler, ISP_PARSER_DOWN, (void*)in_param.buf_addr, (void*)&rtn_cmd);
				ret=ispParserFree((void*)in_param.buf_addr);

				if(ISP_READ_SENSOR_REG==rtn_cmd.cmd)
				{
					ret=ispParser(isp_handler, ISP_READ_SENSOR_REG, (void*)&rtn_cmd, (void*)&rtn_param);
					if(0x00==ret)
					{
						isp_ptr=eng_rsp_diag+sizeof(MSG_HEAD_T)+1;
						rlen=ispvideo_SetIspParamToSt(isp_ptr, (struct isp_parser_buf_in*)&rtn_param);
						ret=ispParserFree((void*)rtn_param.buf_addr);
					}
				}
			}

			rsp_len += rlen;
			eng_rsp_diag[rsp_len] = 0x7e;
			msg_ret->len = rsp_len-1;
			res = send(sockfd, eng_rsp_diag, rsp_len+1, 0);
			break;
		}
		case CMD_WRITE_SENSOR_REG:
		{ // ok need test
			DBG("ISP_TOOL:CMD_WRITE_SENSOR_REG \n");
			struct isp_parser_buf_in in_param = {0x00, 0x00};
			struct isp_parser_cmd_param rtn_cmd;
			uint8_t* dig_ptr=buf;
			uint8_t* isp_ptr=buf+sizeof(MSG_HEAD_T)+1;

			memset(&rtn_cmd, 0, sizeof(rtn_cmd));
			in_param.buf_len=ispvideo_GetIspParamLenFromSt(dig_ptr);
			in_param.buf_addr=(unsigned long)ispParserAlloc(in_param.buf_len);

			if((0x00!=in_param.buf_len)
				&&(0x00!=in_param.buf_addr))
			{
				ret=ispvideo_GetIspParamFromSt(isp_ptr, (struct isp_parser_buf_rtn*)&in_param);
				ret=ispParser(isp_handler, ISP_PARSER_DOWN, (void*)in_param.buf_addr, (void*)&rtn_cmd);
				ret=ispParserFree((void*)in_param.buf_addr);
			}

			rsp_len+=ispvideo_SetreTurnValue((uint8_t*)&eng_rsp_diag[rsp_len], ISP_CMD_SUCCESS);
			eng_rsp_diag[rsp_len] = 0x7e;
			msg_ret->len = rsp_len-1;
			res = send(sockfd, eng_rsp_diag, rsp_len+1, 0);
			break;
		}
		case CMD_GET_INFO:
		{ // ok
			DBG("ISP_TOOL:CMD_GET_INFO \n");
			/* TODO:read isp param operation */
			// rlen is the size of isp_param
			// pass eng_rsp_diag+rsp_len

			struct isp_parser_buf_in in_param = {0x00, 0x00};
			struct isp_parser_buf_rtn rtn_param = {0x00, 0x00};
			struct isp_parser_cmd_param rtn_cmd;
			uint8_t* dig_ptr=buf;
			uint8_t* isp_ptr=buf+sizeof(MSG_HEAD_T)+1;
			uint8_t i=0x00;
			uint32_t* addr=(uint32_t*)isp_ptr;
			uint32_t* ptr=NULL;

			memset(&rtn_cmd, 0, sizeof(rtn_cmd));
			in_param.buf_len=ispvideo_GetIspParamLenFromSt(dig_ptr);
			in_param.buf_addr=(unsigned long)ispParserAlloc(in_param.buf_len);

			if((0x00!=in_param.buf_len)
				&&(0x00!=in_param.buf_addr))
			{
				ret=ispvideo_GetIspParamFromSt(isp_ptr, (struct isp_parser_buf_rtn*)&in_param);
				ret=ispParser(isp_handler, ISP_PARSER_DOWN, (void*)in_param.buf_addr, (void*)&rtn_cmd);
				ret=ispParserFree((void*)in_param.buf_addr);

				DBG("ISP_TOOL:CMD_GET_INFO rtn cmd:%d \n", rtn_cmd.cmd);

				if(ISP_INFO==rtn_cmd.cmd)
				{
					ret=ispParser(isp_handler, ISP_PARSER_UP_INFO, (void*)&rtn_cmd, (void*)&rtn_param);
					if(0x00==ret)
					{
						isp_ptr=eng_rsp_diag+sizeof(MSG_HEAD_T)+1;
						rlen=ispvideo_SetIspParamToSt(isp_ptr, (struct isp_parser_buf_in*)&rtn_param);
						ptr=(uint32_t*)rtn_param.buf_addr;
						for(i=0x00; i<rtn_param.buf_len; i+=0x04){

							DBG("ISP_TOOL:CMD_GET_INFO param:0x%08x \n", *ptr++);

						}
						ret=ispParserFree((void*)rtn_param.buf_addr);
					}
				}
			}

			rsp_len += rlen;
			eng_rsp_diag[rsp_len] = 0x7e;
			msg_ret->len = rsp_len-1;
			res = send(sockfd, eng_rsp_diag, rsp_len+1, 0);
			break ;
		}
		case CMD_OTP_WRITE:
			DBG("ISP_OTP:CMD_OTP_WRITE \n");
			preview_tmpflag = preview_flag;
			len -= rsp_len;
			is_stop_preview = isp_otp_needstopprev(&buf[rsp_len], &len);
			if(is_stop_preview && preview_tmpflag)
			{
				if(NULL!=fun_ptr->stop_preview){
					fun_ptr->stop_preview(0, 0);
				}
			}

			ret = isp_otp_write(isp_handler, &buf[rsp_len], &len);
			if (!ret) {
				memset(&eng_rsp_diag[rsp_len],0,len);
			} else {
				memset(&eng_rsp_diag[rsp_len],0,len);
				eng_rsp_diag[rsp_len] = 0x01;
			}
			rsp_len += len;
			eng_rsp_diag[rsp_len] = 0x7e;
			msg_ret->len = rsp_len-1;

			if(is_stop_preview && preview_tmpflag)
			{
				if(NULL!=fun_ptr->start_preview){
					fun_ptr->start_preview(0, 0);
				}
				preview_flag = preview_tmpflag;
			}

			res = send(sockfd, eng_rsp_diag, rsp_len+1, 0);
			DBG("ISP_OTP:CMD_OTP_WRITE done\n");
			break;

		case CMD_OTP_READ:
			DBG("ISP_OTP:CMD_OTP_READ \n");
			preview_tmpflag = preview_flag;
			len -= rsp_len;
			is_stop_preview = isp_otp_needstopprev(&buf[rsp_len], &len);
			if(is_stop_preview && preview_tmpflag)
			{
				if(NULL!=fun_ptr->stop_preview){
					fun_ptr->stop_preview(0, 0);
				}
			}

			memcpy(eng_rsp_diag,buf,(msg_ret->len+1));
			ret = isp_otp_read(isp_handler, &eng_rsp_diag[rsp_len], &len);
			DBG("ISP_OTP:CMD_OTP_READ rsp_len %d len %d\n",rsp_len,len);
			rsp_len += len;
			eng_rsp_diag[rsp_len] = 0x7e;
			msg_ret->len = rsp_len-1;

			if(is_stop_preview && preview_tmpflag)
			{
				if(NULL!=fun_ptr->start_preview){
					fun_ptr->start_preview(0, 0);
				}
				preview_flag = preview_tmpflag;
			}
			res = send(sockfd, eng_rsp_diag, rsp_len+1, 0);
			DBG("ISP_OTP:CMD_OTP_READ  done\n");

			break;
		default:
			break;
	}				/* -----  end switch  ----- */
	return EXIT_SUCCESS;
}				/* ----------  end of function main  ---------- */

/*

#define ISP_DATA_YUV422_2FRAME (1<<0)
#define ISP_DATA_YUV420_2FRAME (1<<1)
#define ISP_DATA_NORMAL_RAW10 (1<<2)
#define ISP_DATA_MIPI_RAW10 (1<<3)
#define ISP_DATA_JPG (1<<4)

*/

void ispvideo_Scale(uint32_t format, uint32_t in_w, uint32_t in_h, char *in_imgptr, int in_imglen, uint32_t* out_w, uint32_t* out_h, char **out_imgptr, int* out_imglen)
{
	int ret;
	uint32_t x=0x00;
	uint32_t y=0x00;
	uint32_t src_img_w=in_w;
	uint32_t src_img_h=in_h;
	uint32_t img_w=in_w;
	uint32_t img_h=in_h;
	uint8_t* src_buf_ptr=(uint8_t*)in_imgptr;
	uint8_t* dst_buf_ptr=(uint8_t*)*out_imgptr;
	uint32_t shift_num=0x01;

	if((PREVIEW_MAX_WIDTH>=img_w)
		&&(PREVIEW_MAX_HEIGHT>=img_h))	{
		*out_w=img_w;
		*out_h=img_h;
		*out_imgptr=in_imgptr;
		*out_imglen=in_imglen;
		return ;
	}

	for(x=0x00; ; x++){
		if((PREVIEW_MAX_WIDTH>=img_w)
			&&(PREVIEW_MAX_HEIGHT>=img_h))	{
			break ;
		}
		img_w>>=0x01;
		img_h>>=0x01;
		shift_num<<=0x01;
	}

	*out_w=img_w;
	*out_h=img_h;

	if ((ISP_TOOL_YUV420_2FRAME == format)
		||(ISP_TOOL_YVU420_2FRAME == format)) {
		*out_imglen=img_w*img_h+(img_w*img_h)/2;
		shift_num-=0x01;
		shift_num<<=0x01;

		for(y=0x00; y<img_h; y+=0x02){/*y*/
			for(x=0x00; x<img_w; x+=0x02){
				*dst_buf_ptr++=*src_buf_ptr++;
				*dst_buf_ptr++=*src_buf_ptr++;
				src_buf_ptr+=shift_num;
			}
			for(x=0x00; x<img_w; x+=0x02){
				*dst_buf_ptr++=*src_buf_ptr++;
				*dst_buf_ptr++=*src_buf_ptr++;
				src_buf_ptr+=shift_num;
			}
			src_buf_ptr+=(src_img_w*shift_num);
		}

		src_buf_ptr=(uint8_t*)in_imgptr;
		dst_buf_ptr=(uint8_t*)*out_imgptr;
		src_buf_ptr=src_buf_ptr+src_img_w*src_img_h;
		dst_buf_ptr=dst_buf_ptr+img_w*img_h;


		img_h>>=0x01;
		for(y=0x00; y<img_h; y++){/*uv*/
			for(x=0x00; x<img_w; x+=0x02){
				*dst_buf_ptr++=*src_buf_ptr++;
				*dst_buf_ptr++=*src_buf_ptr++;
				src_buf_ptr+=shift_num;
			}
			src_buf_ptr+=(src_img_w*(shift_num/2));
		}

	}

}

void send_img_data(uint32_t format, uint32_t width, uint32_t height, char *imgptr, int imagelen)
{
	uint32_t handler_id=0x00;
	int ret;

	if (0==preview_img_end_flag)
	{
		pthread_mutex_lock(&ispstream_lock);

		char *img_ptr = (char *)preview_buf_ptr;
		int img_len;
		uint32_t img_w;
		uint32_t img_h;
		/*if preview size more than vga that is subsample to less than vga for preview frame ratio*/
		ispvideo_Scale(format, width, height, imgptr, imagelen, &img_w, &img_h, &img_ptr, &img_len);

		ret = handle_img_data(format, img_w, img_h, img_ptr, img_len, 0, 0, 0, 0);

		sem_post(&preview_sem_lock);
		if (ret != 0) {
			DBG("ISP_TOOL:handle_img_data().error ret = %d.", ret);
		}

		pthread_mutex_unlock(&ispstream_lock);
	}
}

void send_capture_complete_msg()
{
	if ((capture_flag == 1) && (1 == capture_img_end_flag) )
	{
		usleep(1000*1000);
		sem_post(&capture_sem_lock);
		capture_flag = 0;
	}
}

void send_capture_data(uint32_t format, uint32_t width, uint32_t height, char *ch0_ptr, int ch0_len,char *ch1_ptr, int ch1_len,char *ch2_ptr, int ch2_len)
{
	uint32_t handler_id=0x00;
	int ret;

	if ((0 == capture_img_end_flag)&&(format == (uint32_t)capture_format))
	{
		pthread_mutex_lock(&ispstream_lock);

		DBG("ISP_TOOL: capture format: %d, width: %d, height: %d.\n", format, width, height);
		switch( g_command ){
			case CMD_ASIC_TAKE_PICTURE:
			case CMD_ASIC_TAKE_PICTURE_NEW:
				ret = handle_img_data_asic(format, width, height, ch0_ptr, ch0_len, ch1_ptr, ch1_len, ch2_ptr, ch2_len);
				break;
			//case CMD_SFT_TAKE_PICTURE_NEW:
				//ret = handle_img_data_sft(format, width, height, ch0_ptr, ch0_len, ch1_ptr, ch1_len, ch2_ptr, ch2_len);
				//break;
			default:
				ret = handle_img_data(format, width, height, ch0_ptr, ch0_len, ch1_ptr, ch1_len, ch2_ptr, ch2_len);
				break;
		}
		capture_img_end_flag=1;
		if (ret != 0) {
			DBG("ISP_TOOL: handle_img_data().error  ret = %d.", ret);
		}

		pthread_mutex_unlock(&ispstream_lock);
	}
}

int isp_RecDataCheck(uint8_t* rx_buf_ptr, int rx_bug_len, uint8_t* cmd_buf_ptr,  int* cmd_len)
{
	int rtn;
	uint8_t* rx_ptr=rx_buf_ptr;
	uint8_t* cmd_ptr=cmd_buf_ptr;
	uint8_t packet_header=rx_buf_ptr[0];
	uint16_t packet_len=(rx_buf_ptr[6]<<0x08)|(rx_buf_ptr[5]);
	uint8_t packet_end=rx_buf_ptr[rx_bug_len-1];
	int rx_len=rx_bug_len;
	int cmd_buf_offset=rx_packet_len;

	rtn=0;
	*cmd_len=rx_bug_len;

	if ((0x7e == packet_header)
		&&(0x00 == rx_packet_total_len)
		&&(0x00 == rx_packet_len)) {
		rx_packet_total_len=packet_len+2;
	}

	if ((0x7e == packet_header)
		&&(0x7e == packet_end)) {
	/* one packet */

	} else { /* mul packet */
		rx_packet_len+=rx_bug_len;

		if ((0x7e == packet_end)
			&&(rx_packet_len == rx_packet_total_len)) {
			*cmd_len=rx_packet_len;
		} else {
			rtn=1;
		}
	}

	memcpy((void*)&cmd_buf_ptr[cmd_buf_offset], rx_buf_ptr, rx_bug_len);

	if (0 == rtn) {
		rx_packet_len=0x00;
		rx_packet_total_len=0x00;
	}

	return rtn;
}

static void * isp_diag_handler(void *args)
{
	uint32_t handler_id=0x00;
	int from = *((int *)args);
	int i, cnt, res, cmd_len, rtn;
	static char *code = "diag channel exit";
	fd_set rfds;
	struct timeval tv;

	FD_ZERO(&rfds);
	FD_SET(from, &rfds);

	sockfd = from;
	rx_packet_len=0x00;
	rx_packet_total_len=0x00;

	/* Read client request, send sequence number back */
	while (wire_connected) {

		/* Wait up to  two seconds. */
		tv.tv_sec = 2;
		tv.tv_usec = 0;
		FD_SET(from, &rfds);
		res = select(from + 1, &rfds, NULL, NULL, &tv);
		if (res <= 0) { //timeout or other error
			DBG("ISP_TOOL:No data within five seconds. res:%d\n", res);
			continue;
		}

		cnt = recv(from, diag_rx_buf, CMD_BUF_SIZE,MSG_DONTWAIT);

		if (cnt <= 0) {
			DBG("ISP_TOOL:read socket error %s\n", strerror(errno));
			break;
		}

		rtn = isp_RecDataCheck(diag_rx_buf, cnt, diag_cmd_buf, &cmd_len);

		if (0 == rtn) {
			handle_isp_data(diag_cmd_buf, cmd_len);
		} else {
			DBG("ISP_TOOL: rx packet comboine \n");
		}

	}
	DBG("exit %s\n", strerror(errno));
	return code;
}

static void * ispserver_thread(void *args)
{
	UNUSED(args);
	uint32_t handler_id=0x00;
	struct sockaddr claddr;
	int lfd, cfd, optval;
	int log_fd;
	struct sockaddr_in sock_addr;
	socklen_t addrlen;
#ifdef CLIENT_DEBUG
#define ADDRSTRLEN (128)
	char addrStr[ADDRSTRLEN];
	char host[50];
	char service[30];
#endif
	pthread_t tdiag;
	pthread_attr_t attr;

	DBG("ISP_TOOL:isp-video server version 1.0\n");

	memset(&sock_addr, 0, sizeof (struct sockaddr_in));
	sock_addr.sin_family = AF_INET;        /* Allows IPv4*/
	sock_addr.sin_addr.s_addr = INADDR_ANY;/* Wildcard IP address;*/
	sock_addr.sin_port = htons(PORT_NUM);

	lfd = socket(sock_addr.sin_family, SOCK_STREAM, 0);
	if (lfd == -1) {
		 DBG("ISP_TOOL:socket error\n");
		 return NULL;
	}
	sock_fd=lfd;
	optval = 1;
	if (setsockopt(lfd, SOL_SOCKET, SO_REUSEADDR, &optval, sizeof(optval)) != 0) {
		 DBG("ISP_TOOL:setsockopt error\n");
		 return NULL;
	}

	if (bind(lfd, (struct sockaddr *)&sock_addr, sizeof (struct sockaddr_in)) != 0) {
		DBG("ISP_TOOL:bind error %s\n", strerror(errno));
		 return NULL;
	}

	if (listen(lfd, BACKLOG) == -1){
		DBG("ISP_TOOL:listen error\n");
		 return NULL;
	}

	sem_init(&preview_sem_lock, 0, 0);
	sem_init(&capture_sem_lock, 0, 0);
	pthread_mutex_init(&ispstream_lock, NULL);

	pthread_attr_init(&attr);
	for (;;) {                  /* Handle clients iteratively */
		void * res;
		int ret;

		DBG("ISP_TOOL:log server waiting client dail in...\n");
		/* Accept a client connection, obtaining client's address */
		wire_connected = 0;
		addrlen = sizeof(struct sockaddr);
		cfd = accept(lfd, &claddr, &addrlen);
		if (cfd == -1) {
			DBG("ISP_TOOL:accept error %s\n", strerror(errno));
			break;
		}
		DBG("ISP_TOOL:log server connected with client\n");
		wire_connected = 1;
		sequence_num = 0;
		/* Ignore the SIGPIPE signal, so that we find out about broken
		 * connection errors via a failure from write().
		 */
		if (signal(SIGPIPE, SIG_IGN) == SIG_ERR)
			DBG("ISP_TOOL:signal error\n");
#ifdef CLIENT_DEBUG
		addrlen = sizeof(struct sockaddr);
		if (getnameinfo(&claddr, addrlen, host, 50, service,
			 30, NI_NUMERICHOST) == 0)
			snprintf(addrStr, ADDRSTRLEN, "(%s, %s)", host, service);
		else
			snprintf(addrStr, ADDRSTRLEN, "(?UNKNOWN?)");
		DBG("ISP_TOOL:Connection from %s\n", addrStr);
#endif

		//create a thread for recv cmd
		ret = pthread_create(&tdiag, &attr, isp_diag_handler, &cfd);
		if (ret != 0) {
			DBG("ISP_TOOL:diag thread create success\n");
			break;
		}

		pthread_join(tdiag, &res);
		DBG("ISP_TOOL:diag thread exit success %s\n", (char *)res);
		if (close(cfd) == -1)           /* Close connection */
			DBG("ISP_TOOL:close socket cfd error\n");
	}
	pthread_attr_destroy(&attr);
	if (close(lfd) == -1)           /* Close connection */
		DBG("ISP_TOOL:close socket lfd error\n");

	sock_fd=0x00;

	pthread_mutex_destroy(&ispstream_lock);

	 return NULL;
}

int ispvideo_RegCameraFunc(uint32_t cmd, int(*func)(uint32_t, uint32_t))
{
	struct camera_func* fun_ptr=ispvideo_GetCameraFunc();
	
	switch(cmd)
	{
		case REG_START_PREVIEW:
		{
			fun_ptr->start_preview = func;
			break;
		}
		case REG_STOP_PREVIEW:
		{
			fun_ptr->stop_preview = func;
			break;
		}
		case REG_TAKE_PICTURE:
		{
			fun_ptr->take_picture = func;
			break;
		}
		case REG_SET_PARAM:
		{
			fun_ptr->set_capture_size = func;
			break;
		}
		default :
		{
			break;
		}
	}
	
	return 0x00;
}


void stopispserver()
{
	uint32_t handler_id=0x00;

	DBG("ISP_TOOL:stopispserver \n");
	wire_connected = 0;

	if((1 == preview_flag) && (0 == preview_img_end_flag)) {
		sem_post(&preview_sem_lock);
	}
#ifndef MINICAMERA
	if(0x00!=sock_fd)
	{
		shutdown(sock_fd, 0);
		shutdown(sock_fd, 1);
	}

	ispParserFree((void*)preview_buf_ptr);

	preview_flag = 0;
	capture_flag = 0;
	preview_img_end_flag=1;
	capture_img_end_flag=1;
	preview_buf_ptr=0;
#endif
}

void startispserver()
{
	uint32_t handler_id=0x00;
	pthread_t tdiag;
	pthread_attr_t attr;
#ifdef MINICAMERA
    static int ret=-1;
#else
    int ret=-1;
#endif

	DBG("ISP_TOOL:startispserver\n");

	preview_flag = 0;
	capture_flag = 0;
	preview_img_end_flag=1;
	capture_img_end_flag=1;

	if(ret!=0){
		pthread_attr_init(&attr);
		pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);
		ret = pthread_create(&tdiag, &attr, ispserver_thread, NULL);
		pthread_attr_destroy(&attr);
		if (ret < 0) {
			DBG("ISP_TOOL:pthread_create fail\n");
			return;
		}
		preview_buf_ptr=(uint8_t*)ispParserAlloc(PREVIEW_MAX_WIDTH*PREVIEW_MAX_HEIGHT*2);
	}else{
		DBG("ISP_TOOL:pthread already create now!\n");
	}
}

void setispserver(void* handle)
{
	isp_handler = handle;
	DBG("ISP_TOOL:setispserver %p\n", handle);
}
