/*************************************************************************
** File Name:      decoder.h                                             *
** Author:         Reed zhang                                            *
** Date:           30/11/2005                                            *
** Copyright:      2001 Spreatrum, Incoporated. All Rights Reserved.     *
** Description:    
**                        Edit History                                   *
** ----------------------------------------------------------------------*
** DATE           NAME             DESCRIPTION                           *
** 30/11/2005     Reed zhang       Create.                               *
**************************************************************************/

#ifndef __DECODER_H__
#define __DECODER_H__

#ifdef __cplusplus
extern "C" {
#endif
//#include "aac_structs.h"

/* library output formats */
#define FAAD_FMT_16BIT  1
#define FAAD_FMT_24BIT  2
#define FAAD_FMT_32BIT  3
#define FAAD_FMT_FLOAT  4
#define FAAD_FMT_FIXED  FAAD_FMT_FLOAT
#define FAAD_FMT_DOUBLE 5

#define LC_DEC_CAP            (1<<0)
#define MAIN_DEC_CAP          (1<<1)
#define LTP_DEC_CAP           (1<<2)
#define LD_DEC_CAP            (1<<3)
#define ERROR_RESILIENCE_CAP  (1<<4)
#define FIXED_POINT_CAP       (1<<5)

#define FRONT_CHANNEL_CENTER (1)
#define FRONT_CHANNEL_LEFT   (2)
#define FRONT_CHANNEL_RIGHT  (3)
#define SIDE_CHANNEL_LEFT    (4)
#define SIDE_CHANNEL_RIGHT   (5)
#define BACK_CHANNEL_LEFT    (6)
#define BACK_CHANNEL_RIGHT   (7)
#define BACK_CHANNEL_CENTER  (8)
#define LFE_CHANNEL          (9)
#define UNKNOWN_CHANNEL      (0)

/* 
   �ú�������AAC�������������ʵ��һ�����룬�ӿڲ������£�
       buffer_ptr��     ����������ֻ����һ����������
       buffer_size��    ��ǰ�������Ĵ�С
       pcm_out_l_ptr��  ���������PCM
       pcm_out_r_ptr��  ���������PCM
       frm_pcm_len_ptr�������ǰ������
       aac_dec_mem_ptr��������Ҫ��memory��ַ
*/
int16_t AAC_FrameDecode(uint8_t           *buffer_ptr,         // the input stream
						uint32_t           buffer_size,    // input stream size
						uint16_t          *pcm_out_l_ptr,      // output left channel pcm
						uint16_t          *pcm_out_r_ptr,      // output right channel pcm
						uint16_t          *frm_pcm_len_ptr,     // frmae length 
						void              *aac_dec_mem_ptr,
						int16_t            aacplus_decode_flag,/*1:decode aac plus part, 0:don't decode aac plus part.*/
						int16_t        *decoded_bytes
						);
/* 
   �ú�������AAC��ʼ���������ӿڲ������£�
       headerstream_ptr�� �������ͷ��Ϣ��������
       head_length��      ͷ��Ϣ��������
       sample_rate��      ������
       sign��             ����������Ч�����������ǲ����ʣ�����1��ʾ��������Ч��������������������Ϣ������
       frm_pcm_len_ptr��  �����ǰ������
       aac_buf_ptr    ��  ������Ҫ��memory��ַ
*/				
uint32_t AAC_RetrieveSampleRate(void     *aac_buf_ptr);
int16_t AAC_DecInit(int8_t  *headerstream_ptr,  // input header info stream
                    int16_t  head_length,       // header stream length
                    int32_t  sample_rate,       // sample rate
                    int16_t  sign,              // 1: input sample rate, other: inout header stream 
					void     *aac_buf_ptr);     // allocate memory
/* 
   �ú������ڹ���demux��Ҫ�Ŀռ����Ϣ
   aac_mem_ptr����Ҫ������DEMUX
   �ɹ�����0��ʧ�ܷ��� 1
*/
int16_t AAC_MemoryAlloc(void ** const aac_mem_ptr);    // memory constrct

/*
    �ú�����������demuxʹ�õĿռ����Ϣ��
    aac_mem_ptr����Ҫ������DEMUX
	�ɹ�����0��ʧ�ܷ��� 1
*/
int16_t AAC_MemoryFree(void ** const aac_mem_ptr);    // memory deconstrct


/*
    �ú�������֪ͨ������seek��Ϣ��
    seek_sign��
        1�� seek ��־
        other�����������
*/
int16_t AAC_DecStreamBufferUpdate(
                    int16_t  update_sign,         // 1: update mode, 
					void     *aac_buf_ptr);     // allocate memory

typedef int16_t (*FT_AAC_MemoryFree)(void ** const aac_mem_ptr);
typedef int16_t (*FT_AAC_MemoryAlloc)(void ** const aac_mem_ptr) ;
typedef int16_t (*FT_AAC_DecInit)(int8_t  *headerstream_ptr,  // input header info stream
                    int16_t  head_length,       // header stream length
                    int32_t  sample_rate,       // sample rate
                    int16_t  sign,              // 1: input sample rate, other: inout header stream 
					void     *aac_buf_ptr);
typedef uint32_t (*FT_AAC_RetrieveSampleRate)(void     *aac_buf_ptr);
typedef int16_t (*FT_AAC_FrameDecode)(uint8_t           *buffer_ptr,         // the input stream
						uint32_t           buffer_size,    // input stream size
						uint16_t          *pcm_out_l_ptr,      // output left channel pcm
						uint16_t          *pcm_out_r_ptr,      // output right channel pcm
						uint16_t          *frm_pcm_len_ptr,     // frmae length 
						void              *aac_dec_mem_ptr,
						int16_t            aacplus_decode_flag,/*1:decode aac plus part, 0:don't decode aac plus part.*/
						int16_t        *decoded_bytes
						);
typedef int16_t (*FT_AAC_DecStreamBufferUpdate)(
                    int16_t  update_sign,         // 1: update mode, 
					void     *aac_buf_ptr);

#ifdef __cplusplus
}
#endif
#endif
