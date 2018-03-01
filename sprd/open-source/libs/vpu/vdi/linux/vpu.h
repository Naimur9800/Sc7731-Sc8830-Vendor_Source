//--=========================================================================--
//  This file is linux device driver for VPU.
//-----------------------------------------------------------------------------
//
//       This confidential and proprietary software may be used only
//     as authorized by a licensing agreement from Chips&Media Inc.
//     In the event of publication, the following notice is applicable:
//
//            (C) COPYRIGHT 2006 - 2015  CHIPS&MEDIA INC.
//                      ALL RIGHTS RESERVED
//
//       The entire notice above must be reproduced on all authorized
//       copies.
//
//--=========================================================================--

#ifndef __VPU_DRV_H__
#define __VPU_DRV_H__

#include <linux/fs.h>
#include <linux/types.h>
//#include <asm/semaphore.h>


#define VDI_IOCTL_MAGIC  'V'
#define VDI_IOCTL_ALLOCATE_PHYSICAL_MEMORY	_IO(VDI_IOCTL_MAGIC, 0)
#define VDI_IOCTL_FREE_PHYSICALMEMORY		_IO(VDI_IOCTL_MAGIC, 1)
#define VDI_IOCTL_WAIT_INTERRUPT			_IO(VDI_IOCTL_MAGIC, 2)
#define VDI_IOCTL_SET_CLOCK_GATE			_IO(VDI_IOCTL_MAGIC, 3)
#define VDI_IOCTL_RESET                     _IO(VDI_IOCTL_MAGIC, 4)
#define VDI_IOCTL_GET_INSTANCE_POOL			_IO(VDI_IOCTL_MAGIC, 5)
#define VDI_IOCTL_GET_COMMON_MEMORY			_IO(VDI_IOCTL_MAGIC, 6)
#define VDI_IOCTL_GET_RESERVED_VIDEO_MEMORY_INFO _IO(VDI_IOCTL_MAGIC, 8)
#define VDI_IOCTL_OPEN_INSTANCE				_IO(VDI_IOCTL_MAGIC, 9)
#define VDI_IOCTL_CLOSE_INSTANCE			_IO(VDI_IOCTL_MAGIC, 10)
#define VDI_IOCTL_GET_INSTANCE_NUM			_IO(VDI_IOCTL_MAGIC, 11)

// BIT_RUN command
enum {
	SEQ_INIT = 1,
	SEQ_END = 2,
	PIC_RUN = 3,
	SET_FRAME_BUF = 4,
	ENCODE_HEADER = 5,
	ENC_PARA_SET = 6,
    DEC_RENEW = 6,
	DEC_PARA_SET = 7,
	DEC_BUF_FLUSH = 8,
	RC_CHANGE_PARAMETER	= 9,
	VPU_SLEEP = 10,
	VPU_WAKE = 11,
	ENC_ROI_INIT = 12,
	FIRMWARE_GET = 0xf
};

typedef struct vpudrv_buffer_t {
	unsigned int size;
	unsigned int phys_addr;
	unsigned long base;	     /*kernel logical address in use kernel*/
	unsigned long virt_addr; /* virtual user space address */
} vpudrv_buffer_t;


typedef struct vpu_bit_firmware_info_t {
	unsigned int size;		/* size of this structure */
	unsigned int core_idx;
	unsigned int reg_base_offset;
	unsigned short bit_code[512];
} vpu_bit_firmware_info_t;

typedef struct vpudrv_inst_info_t {
	unsigned int core_idx;
	unsigned int inst_idx;
	int inst_open_count;   /* for output only */
} vpudrv_inst_info_t;

/*struct vpu_dev {
    unsigned int freq_div;

    struct semaphore vpu_mutex;

    struct clk *vpu_clk;
    struct clk *vpu_parent_clk;
    struct clk *mm_clk;

    unsigned int irq;

    struct vsp_fh *vpu_fp;
    struct device_node *dev_np;
};*/


#define VPU_DEBUG 0


#define vpu_loge(fmt, ...) printk(KERN_ERR     "[ERROR][" LOG_TAG "]" "[F:%s-L:%d]" fmt, __FUNCTION__, __LINE__, ##__VA_ARGS__)
#define vpu_logw(fmt, ...) printk(KERN_WARNING "[WARN ][" LOG_TAG "]" "[F:%s-L:%d]" fmt, __FUNCTION__, __LINE__, ##__VA_ARGS__)
#define vpu_logi(fmt, ...) printk(KERN_WARNING "[INFO ][" LOG_TAG "]" "[F:%s-L:%d]" fmt, __FUNCTION__, __LINE__, ##__VA_ARGS__)

#if VPU_DEBUG
#define vpu_logd(fmt, ...) printk(KERN_WARNING "[DEBUG][" LOG_TAG "]" "[F:%s-L:%d]" fmt, __FUNCTION__, __LINE__, ##__VA_ARGS__)
#else
#define vpu_logd(fmt, ...)
#endif


#endif
