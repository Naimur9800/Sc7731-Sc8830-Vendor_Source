//------------------------------------------------------------------------------
// File: vpureport.c
//
// Copyright (c) 2006, Chips & Media.  All rights reserved.
//------------------------------------------------------------------------------
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#include "regdefine.h"
#include "vpuapi.h"
#include "vpuapifunc.h"
#include "vpuio.h"
#include "vpuhelper.h"


#ifdef SUPPORT_FFMPEG_DEMUX
#else
// include in the ffmpeg header
enum CodecID {	// each number mustn't be changed.
    CODEC_ID_H264 = 28,
    CODEC_ID_H263 = 5,
    CODEC_ID_MPEG4 = 13,
    CODEC_ID_MSMPEG4V3 = 17,
    CODEC_ID_WMV1 =  18,
    CODEC_ID_WMV2 = 19,
    CODEC_ID_WMV3 = 74,	
    CODEC_ID_MPEG1VIDEO = 1,
    CODEC_ID_MPEG2VIDEO = 2,
    CODEC_ID_MJPEG = 8,
    CODEC_ID_VC1 = 73,	
    CODEC_ID_RV10 = 6,
    CODEC_ID_RV20 = 7,	
    CODEC_ID_RV30 = 71,
    CODEC_ID_RV40 = 72,
    CODEC_ID_AVS = 85,
    CODEC_ID_CAVS = 90,
    CODEC_ID_VP3 = 30,
    CODEC_ID_THEORA = 31,
    CODEC_ID_VP6 = 94,
    CODEC_ID_VP6F = 95,
    CODEC_ID_VP8 = 146
};
#endif

typedef struct {
    CodStd codStd;
    int mp4Class;
    int codec_id;
    unsigned int fourcc;
} CodStdTab;

#ifndef MKTAG
#define MKTAG(a,b,c,d) (a | (b << 8) | (c << 16) | (d << 24))
#endif
#ifdef SUPPORT_FFMPEG_DEMUX
const CodStdTab codstd_tab[] = {
    { STD_AVC,		0, CODEC_ID_H264,		MKTAG('H', '2', '6', '4') },
    { STD_AVC,		0, CODEC_ID_H264,		MKTAG('X', '2', '6', '4') },
    { STD_AVC,		0, CODEC_ID_H264,		MKTAG('A', 'V', 'C', '1') },
    { STD_AVC,		0, CODEC_ID_H264,		MKTAG('V', 'S', 'S', 'H') },
    { STD_H263,		0, CODEC_ID_H263,		MKTAG('H', '2', '6', '3') },
    { STD_H263,		0, CODEC_ID_H263,		MKTAG('X', '2', '6', '3') },
    { STD_H263,		0, CODEC_ID_H263,		MKTAG('T', '2', '6', '3') },
    { STD_H263,		0, CODEC_ID_H263,		MKTAG('L', '2', '6', '3') },
    { STD_H263,		0, CODEC_ID_H263,		MKTAG('V', 'X', '1', 'K') },
    { STD_H263,		0, CODEC_ID_H263,		MKTAG('Z', 'y', 'G', 'o') },
    { STD_H263,		0, CODEC_ID_H263,		MKTAG('H', '2', '6', '3') },
    { STD_H263,		0, CODEC_ID_H263,		MKTAG('I', '2', '6', '3') },	/* intel h263 */
    { STD_H263,		0, CODEC_ID_H263,		MKTAG('H', '2', '6', '1') },
    { STD_H263,		0, CODEC_ID_H263,		MKTAG('U', '2', '6', '3') },
    { STD_H263,		0, CODEC_ID_H263,		MKTAG('V', 'I', 'V', '1') },
    { STD_MPEG4,	0, CODEC_ID_MPEG4,		MKTAG('F', 'M', 'P', '4') },
    { STD_MPEG4,	5, CODEC_ID_MPEG4,		MKTAG('D', 'I', 'V', 'X') },	// DivX 4
    { STD_MPEG4,	1, CODEC_ID_MPEG4,		MKTAG('D', 'X', '5', '0') },
    { STD_MPEG4,	2, CODEC_ID_MPEG4,		MKTAG('X', 'V', 'I', 'D') },
    { STD_MPEG4,	0, CODEC_ID_MPEG4,		MKTAG('M', 'P', '4', 'S') },
    { STD_MPEG4,	0, CODEC_ID_MPEG4,		MKTAG('M', '4', 'S', '2') },	//MPEG-4 version 2 simple profile
    { STD_MPEG4,	0, CODEC_ID_MPEG4,		MKTAG( 4 ,  0 ,  0 ,  0 ) },	/* some broken avi use this */
    { STD_MPEG4,	0, CODEC_ID_MPEG4,		MKTAG('D', 'I', 'V', '1') },
    { STD_MPEG4,	0, CODEC_ID_MPEG4,		MKTAG('B', 'L', 'Z', '0') },
    { STD_MPEG4,	0, CODEC_ID_MPEG4,		MKTAG('M', 'P', '4', 'V') },
    { STD_MPEG4,	0, CODEC_ID_MPEG4,		MKTAG('U', 'M', 'P', '4') },
    { STD_MPEG4,	0, CODEC_ID_MPEG4,		MKTAG('W', 'V', '1', 'F') },
    { STD_MPEG4,	0, CODEC_ID_MPEG4,		MKTAG('S', 'E', 'D', 'G') },
    { STD_MPEG4,	0, CODEC_ID_MPEG4,		MKTAG('R', 'M', 'P', '4') },
    { STD_MPEG4,	0, CODEC_ID_MPEG4,		MKTAG('3', 'I', 'V', '2') },
    { STD_MPEG4,	0, CODEC_ID_MPEG4,		MKTAG('F', 'F', 'D', 'S') },
    { STD_MPEG4,	0, CODEC_ID_MPEG4,		MKTAG('F', 'V', 'F', 'W') },
    { STD_MPEG4,	0, CODEC_ID_MPEG4,		MKTAG('D', 'C', 'O', 'D') },
    { STD_MPEG4,	0, CODEC_ID_MPEG4,		MKTAG('M', 'V', 'X', 'M') },
    { STD_MPEG4,	0, CODEC_ID_MPEG4,		MKTAG('P', 'M', '4', 'V') },
    { STD_MPEG4,	0, CODEC_ID_MPEG4,		MKTAG('S', 'M', 'P', '4') },
    { STD_MPEG4,	0, CODEC_ID_MPEG4,		MKTAG('D', 'X', 'G', 'M') },
    { STD_MPEG4,	0, CODEC_ID_MPEG4,		MKTAG('V', 'I', 'D', 'M') },
    { STD_MPEG4,	0, CODEC_ID_MPEG4,		MKTAG('M', '4', 'T', '3') },
    { STD_MPEG4,	0, CODEC_ID_MPEG4,		MKTAG('G', 'E', 'O', 'X') },
    { STD_MPEG4,	0, CODEC_ID_MPEG4,		MKTAG('H', 'D', 'X', '4') }, /* flipped video */
    { STD_MPEG4,	0, CODEC_ID_MPEG4,		MKTAG('D', 'M', 'K', '2') },
    { STD_MPEG4,	0, CODEC_ID_MPEG4,		MKTAG('D', 'I', 'G', 'I') },
    { STD_MPEG4,	0, CODEC_ID_MPEG4,		MKTAG('I', 'N', 'M', 'C') },
    { STD_MPEG4,	0, CODEC_ID_MPEG4,		MKTAG('E', 'P', 'H', 'V') }, /* Ephv MPEG-4 */
    { STD_MPEG4,	0, CODEC_ID_MPEG4,		MKTAG('E', 'M', '4', 'A') },
    { STD_MPEG4,	0, CODEC_ID_MPEG4,		MKTAG('M', '4', 'C', 'C') }, /* Divio MPEG-4 */
    { STD_MPEG4,	0, CODEC_ID_MPEG4,		MKTAG('S', 'N', '4', '0') },
    { STD_MPEG4,	0, CODEC_ID_MPEG4,		MKTAG('V', 'S', 'P', 'X') },
    { STD_MPEG4,	0, CODEC_ID_MPEG4,		MKTAG('U', 'L', 'D', 'X') },
    { STD_MPEG4,	0, CODEC_ID_MPEG4,		MKTAG('G', 'E', 'O', 'V') },
    { STD_MPEG4,	0, CODEC_ID_MPEG4,		MKTAG('S', 'I', 'P', 'P') }, /* Samsung SHR-6040 */
    { STD_DIV3,		0, CODEC_ID_MSMPEG4V3,	MKTAG('D', 'I', 'V', '3') }, /* default signature when using MSMPEG4 */
    { STD_DIV3,		0, CODEC_ID_MSMPEG4V3,	MKTAG('M', 'P', '4', '3') },
    { STD_DIV3,		0, CODEC_ID_MSMPEG4V3,	MKTAG('M', 'P', 'G', '3') },
    { STD_MPEG4,	1, CODEC_ID_MSMPEG4V3,	MKTAG('D', 'I', 'V', '5') },
    { STD_MPEG4,	1, CODEC_ID_MSMPEG4V3,	MKTAG('D', 'I', 'V', '6') },
    { STD_MPEG4,	5, CODEC_ID_MSMPEG4V3,	MKTAG('D', 'I', 'V', '4') },
    { STD_DIV3,		0, CODEC_ID_MSMPEG4V3,	MKTAG('D', 'V', 'X', '3') },
    { STD_DIV3,		0, CODEC_ID_MSMPEG4V3,	MKTAG('A', 'P', '4', '1') },	//Another hacked version of Microsoft's MP43 codec. 
    { STD_MPEG4,	0, CODEC_ID_MSMPEG4V3,	MKTAG('C', 'O', 'L', '1') },
    { STD_MPEG4,	0, CODEC_ID_MSMPEG4V3,	MKTAG('C', 'O', 'L', '0') },	// not support ms mpeg4 v1, 2    
    { STD_MPEG4,  256, CODEC_ID_FLV1,		MKTAG('F', 'L', 'V', '1') }, /* Sorenson spark */
    { STD_VC1,		0, CODEC_ID_WMV1,		MKTAG('W', 'M', 'V', '1') },
    { STD_VC1,		0, CODEC_ID_WMV2,		MKTAG('W', 'M', 'V', '2') },
    { STD_MPEG2,	0, CODEC_ID_MPEG1VIDEO,	MKTAG('M', 'P', 'G', '1') },
    { STD_MPEG2,	0, CODEC_ID_MPEG1VIDEO,	MKTAG('M', 'P', 'G', '2') },
    { STD_MPEG2,	0, CODEC_ID_MPEG2VIDEO,	MKTAG('M', 'P', 'G', '2') },
    { STD_MPEG2,	0, CODEC_ID_MPEG2VIDEO,	MKTAG('M', 'P', 'E', 'G') },
    { STD_MPEG2,	0, CODEC_ID_MPEG1VIDEO,	MKTAG('M', 'P', '2', 'V') },
    { STD_MPEG2,	0, CODEC_ID_MPEG1VIDEO,	MKTAG('P', 'I', 'M', '1') },
    { STD_MPEG2,	0, CODEC_ID_MPEG2VIDEO,	MKTAG('P', 'I', 'M', '2') },
    { STD_MPEG2,	0, CODEC_ID_MPEG1VIDEO,	MKTAG('V', 'C', 'R', '2') },
    { STD_MPEG2,	0, CODEC_ID_MPEG1VIDEO,	MKTAG( 1 ,  0 ,  0 ,  16) },
    { STD_MPEG2,	0, CODEC_ID_MPEG2VIDEO,	MKTAG( 2 ,  0 ,  0 ,  16) },
    { STD_MPEG4,	0, CODEC_ID_MPEG4,		MKTAG( 4 ,  0 ,  0 ,  16) },
    { STD_MPEG2,	0, CODEC_ID_MPEG2VIDEO,	MKTAG('D', 'V', 'R', ' ') },
    { STD_MPEG2,	0, CODEC_ID_MPEG2VIDEO,	MKTAG('M', 'M', 'E', 'S') },
    { STD_MPEG2,	0, CODEC_ID_MPEG2VIDEO,	MKTAG('L', 'M', 'P', '2') }, /* Lead MPEG2 in avi */
    { STD_MPEG2,	0, CODEC_ID_MPEG2VIDEO,	MKTAG('S', 'L', 'I', 'F') },
    { STD_MPEG2,	0, CODEC_ID_MPEG2VIDEO,	MKTAG('E', 'M', '2', 'V') },
    { STD_VC1,		0, CODEC_ID_WMV3,		MKTAG('W', 'M', 'V', '3') },
    { STD_VC1,		0, CODEC_ID_VC1,		MKTAG('W', 'V', 'C', '1') },
    { STD_VC1,		0, CODEC_ID_VC1,		MKTAG('W', 'M', 'V', 'A') },


    { STD_RV,		0, CODEC_ID_RV30,		MKTAG('R','V','3','0') },
    { STD_RV,		0, CODEC_ID_RV40,		MKTAG('R','V','4','0') },

    { STD_AVS,		0, CODEC_ID_CAVS,		MKTAG('C','A','V','S') },
    { STD_AVS,		0, CODEC_ID_AVS,		MKTAG('A','V','S','2') },
    { STD_VP3,		0, CODEC_ID_VP3,		MKTAG('V', 'P', '3', '0') },
    { STD_VP3,		0, CODEC_ID_VP3,		MKTAG('V', 'P', '3', '1') },
    { STD_THO,		0, CODEC_ID_THEORA,		MKTAG('T', 'H', 'E', 'O') },
    { STD_VP8,		0, CODEC_ID_VP8,		MKTAG('V', 'P', '8', '0') }
    // 	{ STD_VP6,		0, CODEC_ID_VP6,		MKTAG('V', 'P', '6', '0') },
    // 	{ STD_VP6,		0, CODEC_ID_VP6,		MKTAG('V', 'P', '6', '1') },
    // 	{ STD_VP6,		0, CODEC_ID_VP6,		MKTAG('V', 'P', '6', '2') },
    // 	{ STD_VP6,		0, CODEC_ID_VP6F,		MKTAG('V', 'P', '6', 'F') },
    // 	{ STD_VP6,		0, CODEC_ID_VP6F,		MKTAG('F', 'L', 'V', '4') },

};
#endif
/******************************************************************************
Report Information
******************************************************************************/

// Report Information
const char *strProfile[12][6] =
{
    //STD_AVC
    { "Base Profile", "Main Profile", "Extended Profile", "High Profile", "Reserved Profile", "Reserved Profile"},
    //STD_VC1
    {"Simple Profile", "Main Profile", "Advance Profile", "Reserved Profile", "Reserved Profile", "Reserved Profile"},
    //STD_MPEG2
    {"High Profile", "Spatial Scalable Profile", "SNR Scalable Profile", "Main Profile", "Simple Profile", "Reserved Profile"},
    //STD_MPEG4
    {"Simple Profile", "Advanced Simple Profile", "Advanced Code Efficiency Profile", "Reserved Profile", "Reserved Profile", "Reserved Profile"},
    //STD_H263
    {" ", " ", " ", " ", " ", " "},
    //STD_DIV3
    {" ", " ", " ", " ", " ", " "},
    //STD_RV
    {"Real video Version 8", "Real video Version 9", "Real video Version 10", " ", " ", " "},
    //STD_AVS
    {"Jizhun Profile", " ", " ", " ", " ", " "},
    //STD_MJPG
    {" ", " ", " ", " ", " ", " "},		
    //STD_THO
    {" ", " ", " ", " ", " ", " "},
    //STD_VP3
    {" ", " ", " ", " ", " ", " "},
    //STD_VP8
    {" ", " ", " ", " ", " ", " "}  
};

// Report Information
const char *strLevel[12][8] =
{
    //STD_AVC
    {"Level_1B", "Level_", "Reserved Level", "Reserved Level", "Reserved Level", "Reserved Level", "Reserved Level", "Reserved Level"},
    //STD_VC1
    {"Level_L0(LOW)", "Level_L1", "Level_L2(MEDIUM)", "Level_L3", "Level_L4(HIGH)", "Reserved Level", "Reserved Level", "Reserved Level"},
    //STD_MPEG2
    {"High Level", "High 1440 Level", "Main Level", "Low Level", "Reserved Level", "Reserved Level", "Reserved Level", "Reserved Level"},
    //STD_MPEG4
    {"Level_L0", "Level_L1", "Level_L2", "Level_L3", "Level_L4", "Level_L5", "Level_L6", "Reserved Level"},
    //STD_H263
    {"", "", "", "", "", "", "", ""},
    //STD_DIV3
    {"", "", "", "", "", "", "", ""},
    //STD_RV
    {"", "", "", "", "", "", "", ""},
    //STD_AVS
    {"2.0 Level", "4.0 Level", "4.2 Level", "6.0 Level", "6.2 Level", "", "", ""},
    //STD_MJPG
    {"", "", "", "", "", "", "", ""},
    //STD_THO
    {"", "", "", "", "", "", "", ""},
    //STD_VP3
    {"", "", "", "", "", "", "", ""},
    //STD_VP8
    {"", "", "", "", "", "", "", ""}
};

typedef struct 
{
    osal_file_t fpPicDispInfoLogfile;
    osal_file_t fpPicTypeLogfile;
    osal_file_t fpSeqDispInfoLogfile;
    osal_file_t fpUserDataLogfile;
    osal_file_t fpSeqUserDataLogfile;


    // encoder report file
    osal_file_t fpEncSliceBndInfo;
    osal_file_t fpEncQpInfo;
    osal_file_t fpEncmvInfo;
    osal_file_t fpEncsliceInfo;

    // Report Information
    int reportOpened;
    int decIndex;
    vpu_buffer_t vb_rpt;
    int userDataEnable;
    int userDataReportMode;

    int profile;
    int level;

} vpu_rpt_info_t;

static vpu_rpt_info_t s_rpt_info[MAX_VPU_CORE_NUM];


// VC1 specific
enum {
    BDU_SEQUENCE_END               = 0x0A,
    BDU_SLICE                      = 0x0B,
    BDU_FIELD                      = 0x0C,
    BDU_FRAME                      = 0x0D,
    BDU_ENTRYPOINT_HEADER          = 0x0E,
    BDU_SEQUENCE_HEADER            = 0x0F,
    BDU_SLICE_LEVEL_USER_DATA      = 0x1B,
    BDU_FIELD_LEVEL_USER_DATA      = 0x1C,
    BDU_FRAME_LEVEL_USER_DATA      = 0x1D,
    BDU_ENTRYPOINT_LEVEL_USER_DATA = 0x1E,
    BDU_SEQUENCE_LEVEL_USER_DATA   = 0x1F	
};

// AVC specific - SEI
enum {
    SEI_REGISTERED_ITUTT35_USERDATA	= 0x04,
    SEI_UNREGISTERED_USERDATA		= 0x05
};




/******************************************************************************
Decoder Report Information
******************************************************************************/



void SaveUserDataINT(int core_idx, BYTE *userDataBuf, int size, int intIssued, int decIdx, CodStd bitstreamFormat) {

    vpu_rpt_info_t *rpt = &s_rpt_info[core_idx];
    int i;
    int UserDataType = 0;
    int UserDataSize = 0;
    int userDataNum = 0;
    int TotalSize;
    BYTE *tmpBuf;
    static int backupSize;
    static BYTE *backupBuf;
    BYTE *backupBufTmp;

    if (!rpt->reportOpened)
        return ;

    if(rpt->fpUserDataLogfile == NULL) {
        rpt->fpUserDataLogfile = osal_fopen(FN_USER_DATA, "w+");
    }

    backupBufTmp = (BYTE *)osal_malloc(backupSize + size);

    if (backupBufTmp == 0) {
        VLOG( ERR, "Can't mem allock\n");
        return;
    }

    for (i=0; i<backupSize; i++) {
        backupBufTmp[i] = backupBuf[i];
    }
    if (backupBuf)
        osal_free(backupBuf);
    backupBuf = backupBufTmp;

    tmpBuf = userDataBuf + USER_DATA_INFO_OFFSET;
    size -= USER_DATA_INFO_OFFSET;

    for(i=0; i<size; i++) {
        backupBuf[backupSize + i] = tmpBuf[i];
    }

    backupSize += size;

    if (intIssued)
        return;


    tmpBuf = userDataBuf;
    userDataNum = (short)((tmpBuf[0]<<8) | (tmpBuf[1]<<0));
    if(!userDataNum)
        return; 

    tmpBuf = userDataBuf + 8;
    UserDataSize = (short)((tmpBuf[2]<<8) | (tmpBuf[3]<<0));

    UserDataSize = (UserDataSize+7)/8*8;
    osal_fprintf(rpt->fpUserDataLogfile, "FRAME [%1d]\n", decIdx);

    for(i=0; i<backupSize; i++) {
        osal_fprintf(rpt->fpUserDataLogfile, "%02x", backupBuf[i]);
        if ((i&7) == 7)
            osal_fprintf(rpt->fpUserDataLogfile, "\n");

        if( (i%8==7) && (i==UserDataSize-1) && (UserDataSize != backupSize)) {
            osal_fprintf(rpt->fpUserDataLogfile, "\n");
            tmpBuf+=8;
            UserDataSize += (short)((tmpBuf[2]<<8) | (tmpBuf[3]<<0));
            UserDataSize = (UserDataSize+7)/8*8;
        }
    }
    if (backupSize)
        osal_fprintf(rpt->fpUserDataLogfile, "\n");

    tmpBuf = userDataBuf;
    userDataNum = (short)((tmpBuf[0]<<8) | (tmpBuf[1]<<0));
    TotalSize = (short)((tmpBuf[2]<<8) | (tmpBuf[3]<<0));

    osal_fprintf(rpt->fpUserDataLogfile, "User Data Num: [%d]\n", userDataNum);
    osal_fprintf(rpt->fpUserDataLogfile, "User Data Total Size: [%d]\n", TotalSize);

    tmpBuf = userDataBuf + 8;
    for(i=0; i<userDataNum; i++) {

        UserDataType = (short)((tmpBuf[0]<<8) | (tmpBuf[1]<<0));
        UserDataSize = (short)((tmpBuf[2]<<8) | (tmpBuf[3]<<0));

        if(bitstreamFormat == STD_VC1) {
            switch (UserDataType) {

            case BDU_SLICE_LEVEL_USER_DATA:
                osal_fprintf(rpt->fpUserDataLogfile, "User Data Type: [%d:%s]\n", i, "BDU_SLICE_LEVEL_USER_DATA");
                osal_fprintf(rpt->fpUserDataLogfile, "User Data Size: [%d]\n", UserDataSize);
                break;
            case BDU_FIELD_LEVEL_USER_DATA:
                osal_fprintf(rpt->fpUserDataLogfile, "User Data Type: [%d:%s]\n", i, "BDU_FIELD_LEVEL_USER_DATA");
                osal_fprintf(rpt->fpUserDataLogfile, "User Data Size: [%d]\n", UserDataSize);
                break;
            case BDU_FRAME_LEVEL_USER_DATA:
                osal_fprintf(rpt->fpUserDataLogfile, "User Data Type: [%d:%s]\n", i, "BDU_FRAME_LEVEL_USER_DATA");
                osal_fprintf(rpt->fpUserDataLogfile, "User Data Size: [%d]\n", UserDataSize);
                break;
            case BDU_ENTRYPOINT_LEVEL_USER_DATA:
                osal_fprintf(rpt->fpUserDataLogfile, "User Data Type: [%d:%s]\n", i, "BDU_ENTRYPOINT_LEVEL_USER_DATA");
                osal_fprintf(rpt->fpUserDataLogfile, "User Data Size: [%d]\n", UserDataSize);
                break;
            case BDU_SEQUENCE_LEVEL_USER_DATA:
                osal_fprintf(rpt->fpUserDataLogfile, "User Data Type: [%d:%s]\n", i, "BDU_SEQUENCE_LEVEL_USER_DATA");
                osal_fprintf(rpt->fpUserDataLogfile, "User Data Size: [%d]\n", UserDataSize);
                break;
            }
        }
        else if(bitstreamFormat == STD_AVC) {
            switch (UserDataType) {
            case SEI_REGISTERED_ITUTT35_USERDATA:
                osal_fprintf(rpt->fpUserDataLogfile, "User Data Type: [%d:%s]\n", i, "registered_itu_t_t35");
                osal_fprintf(rpt->fpUserDataLogfile, "User Data Size: [%d]\n", UserDataSize);
                break;
            case SEI_UNREGISTERED_USERDATA:
                osal_fprintf(rpt->fpUserDataLogfile, "User Data Type: [%d:%s]\n", i, "unregistered");
                osal_fprintf(rpt->fpUserDataLogfile, "User Data Size: [%d]\n", UserDataSize);
                break;
            }
        }
        else if(bitstreamFormat == STD_MPEG2) {

            switch (UserDataType) {
            case 0:
                osal_fprintf(rpt->fpUserDataLogfile, "User Data Type: [%d:Seq]\n", i);
                break;
            case 1:
                osal_fprintf(rpt->fpUserDataLogfile, "User Data Type: [%d:Gop]\n", i);
                break;
            case 2:
                osal_fprintf(rpt->fpUserDataLogfile, "User Data Type: [%d:Pic]\n", i);
                break;
            default:
                osal_fprintf(rpt->fpUserDataLogfile, "User Data Type: [%d:Error]\n", i);
                break;
            }
            osal_fprintf(rpt->fpUserDataLogfile, "User Data Size: [%d]\n", UserDataSize);
        } 
        else if(bitstreamFormat == STD_AVS) {
            osal_fprintf(rpt->fpUserDataLogfile, "User Data Type: [%d:%s]\n", i, "User Data");
            osal_fprintf(rpt->fpUserDataLogfile, "User Data Size: [%d]\n", UserDataSize);
        }
        else {
            switch (UserDataType) {
            case 0:
                osal_fprintf(rpt->fpUserDataLogfile, "User Data Type: [%d:Vos]\n", i);
                break;
            case 1:
                osal_fprintf(rpt->fpUserDataLogfile, "User Data Type: [%d:Vis]\n", i);
                break;
            case 2:
                osal_fprintf(rpt->fpUserDataLogfile, "User Data Type: [%d:Vol]\n", i);
                break;
            case 3:
                osal_fprintf(rpt->fpUserDataLogfile, "User Data Type: [%d:Gov]\n", i);
                break;
            default:
                osal_fprintf(rpt->fpUserDataLogfile, "User Data Type: [%d:Error]\n", i);
                break;
            }
            osal_fprintf(rpt->fpUserDataLogfile, "User Data Size: [%d]\n", UserDataSize);
        }

        tmpBuf += 8;
    }
    osal_fprintf(rpt->fpUserDataLogfile, "\n");
    osal_fflush(rpt->fpUserDataLogfile);

    backupSize = 0;
    if (backupBuf)
        osal_free(backupBuf);
    backupBuf = 0;
}

void SaveUserData(int core_idx, BYTE *userDataBuf) 
{

    vpu_rpt_info_t *rpt = &s_rpt_info[core_idx];
    int i;
    int UserDataType;
    int UserDataSize;
    int userDataNum;
    int TotalSize;
    BYTE *tmpBuf;

    if (!rpt->reportOpened)
        return ;

    if(rpt->fpUserDataLogfile == 0) {
        rpt->fpUserDataLogfile = osal_fopen(FN_USER_DATA, "w+");
    }

    tmpBuf = userDataBuf;
    userDataNum = (short)((tmpBuf[0]<<8) | (tmpBuf[1]<<0));
    TotalSize = (short)((tmpBuf[2]<<8) | (tmpBuf[3]<<0));

    tmpBuf = userDataBuf + 8;

    for(i=0; i<userDataNum; i++) {

        UserDataType = (short)((tmpBuf[0]<<8) | (tmpBuf[1]<<0));
        UserDataSize = (short)((tmpBuf[2]<<8) | (tmpBuf[3]<<0));

        osal_fprintf(rpt->fpUserDataLogfile, "\n[Idx Type Size] : [%4d %4d %4d]",i, UserDataType, UserDataSize);

        tmpBuf += 8;
    }
    osal_fprintf(rpt->fpUserDataLogfile, "\n");

    tmpBuf = userDataBuf + USER_DATA_INFO_OFFSET;

    for(i=0; i<TotalSize; i++) {
        osal_fprintf(rpt->fpUserDataLogfile, "%02x", tmpBuf[i]);
        if ((i&7) == 7)
            osal_fprintf(rpt->fpUserDataLogfile, "\n");
    }
    osal_fprintf(rpt->fpUserDataLogfile, "\n");

    osal_fflush(rpt->fpUserDataLogfile);

}

void SaveSeqUserData(int core_idx, BYTE *userDataBuf, CodStd bitstreamFormat) 
{
    vpu_rpt_info_t *rpt = &s_rpt_info[core_idx];
    int i;
    int UserDataType;
    int UserDataSize;
    int userDataNum;
    int TotalSize;
    BYTE *tmpBuf;

    if (!rpt->reportOpened)
        return ;

    if(rpt->fpSeqUserDataLogfile == 0) {
        rpt->fpSeqUserDataLogfile = osal_fopen(FN_SEQ_USER_DATA, "w+");
    }

    tmpBuf = userDataBuf;
    userDataNum = (short)((tmpBuf[0]<<8) | (tmpBuf[1]<<0));
    TotalSize = (short)((tmpBuf[2]<<8) | (tmpBuf[3]<<0));

    tmpBuf = userDataBuf + 8;

    for(i=0; i<userDataNum; i++) {

        UserDataType = (short)((tmpBuf[0]<<8) | (tmpBuf[1]<<0));
        UserDataSize = (short)((tmpBuf[2]<<8) | (tmpBuf[3]<<0));

        osal_fprintf(rpt->fpSeqUserDataLogfile, "\n[Idx Type Size] : [%4d %4d %4d]",i, UserDataType, UserDataSize);

        if(bitstreamFormat == STD_AVC) {
            switch (UserDataType) {
            case SEI_REGISTERED_ITUTT35_USERDATA:
                osal_fprintf(rpt->fpSeqUserDataLogfile, "User Data Type: [%d:%s]\n", i, "registered_itu_t_t35");
                osal_fprintf(rpt->fpSeqUserDataLogfile, "User Data Size: [%d]\n", UserDataSize);
                break;
            case SEI_UNREGISTERED_USERDATA:
                osal_fprintf(rpt->fpSeqUserDataLogfile, "User Data Type: [%d:%s]\n", i, "unregistered");
                osal_fprintf(rpt->fpSeqUserDataLogfile, "User Data Size: [%d]\n", UserDataSize);
                break;
            }
        }
        else if(bitstreamFormat == STD_MPEG2) {

            switch (UserDataType) {
            case 0:
                osal_fprintf(rpt->fpSeqUserDataLogfile, "User Data Type: [%d:Seq]\n", i);
                break;
            case 1:
                osal_fprintf(rpt->fpSeqUserDataLogfile, "User Data Type: [%d:Gop]\n", i);
                break;
            case 2:
                osal_fprintf(rpt->fpSeqUserDataLogfile, "User Data Type: [%d:Pic]\n", i);
                break;
            default:
                osal_fprintf(rpt->fpSeqUserDataLogfile, "User Data Type: [%d:Error]\n", i);
                break;
            }
            osal_fprintf(rpt->fpSeqUserDataLogfile, "User Data Size: [%d]\n", UserDataSize);
        } 

        tmpBuf += 8;
    }
    osal_fprintf(rpt->fpSeqUserDataLogfile, "\n");

    tmpBuf = userDataBuf + USER_DATA_INFO_OFFSET;

    for(i=0; i<TotalSize; i++) {
        osal_fprintf(rpt->fpSeqUserDataLogfile, "%02x", tmpBuf[i]);
        if ((i&7) == 7)
            osal_fprintf(rpt->fpSeqUserDataLogfile, "\n");
    }
    osal_fprintf(rpt->fpSeqUserDataLogfile, "\n");

    osal_fflush(rpt->fpSeqUserDataLogfile);

}


void ConfigSeqReport(Uint32 core_idx, DecHandle handle, CodStd bitstreamFormat)
{
    vpu_rpt_info_t *rpt = &s_rpt_info[core_idx];

    if (!rpt->reportOpened)
        return ;

    rpt->decIndex = 0;
    // MPEG2 - To support SEQ_INIT user data reporting
    if (bitstreamFormat == STD_MPEG2) {	
        rpt->vb_rpt.size     = SIZE_REPORT_BUF;
        if (vdi_allocate_dma_memory(core_idx, &rpt->vb_rpt) < 0)
        {
            VLOG(ERR, "fail to allocate report  buffer\n" );
            return;
        }

        VPU_DecGiveCommand( handle, SET_ADDR_REP_USERDATA, &rpt->vb_rpt.phys_addr );
        VPU_DecGiveCommand( handle, SET_SIZE_REP_USERDATA, &rpt->vb_rpt.size );
        VPU_DecGiveCommand( handle, SET_USERDATA_REPORT_MODE, &rpt->userDataReportMode );
        VPU_DecGiveCommand( handle, SET_VIRT_ADDR_REP_USERDATA, &rpt->vb_rpt.virt_addr);// if HOST wants to set the other virtual address to be saved userdata.

        if (rpt->userDataEnable == 1) {
            VPU_DecGiveCommand( handle, ENABLE_REP_USERDATA, 0 );
        } else {
            VPU_DecGiveCommand( handle, DISABLE_REP_USERDATA, 0 );
        }
    }		
}

void SaveSeqReport(Uint32 core_idx, DecHandle handle, DecInitialInfo *pSeqInfo, CodStd bitstreamFormat)
{
    vpu_rpt_info_t *rpt = &s_rpt_info[core_idx];
    if (!rpt->reportOpened)
        return ;

    // Report Information
    if (rpt->fpSeqDispInfoLogfile == NULL) rpt->fpSeqDispInfoLogfile = osal_fopen(FN_SEQ_INFO, "w+");

    if (bitstreamFormat == STD_MPEG4) {
        osal_fprintf(rpt->fpSeqDispInfoLogfile, "Data Partition Enable Flag [%1d]\n", pSeqInfo->mp4DataPartitionEnable);
        osal_fprintf(rpt->fpSeqDispInfoLogfile, "Reversible VLC Enable Flag [%1d]\n", pSeqInfo->mp4ReversibleVlcEnable);
        if (pSeqInfo->mp4ShortVideoHeader) {			
            osal_fprintf(rpt->fpSeqDispInfoLogfile, "Short Video Header\n");
            osal_fprintf(rpt->fpSeqDispInfoLogfile, "AnnexJ Enable Flag [%1d]\n", pSeqInfo->h263AnnexJEnable);
        } else
            osal_fprintf(rpt->fpSeqDispInfoLogfile, "Not Short Video\n");		
    }

    switch(bitstreamFormat) {
    case STD_MPEG2:
        rpt->profile = (pSeqInfo->profile==0 || pSeqInfo->profile>5) ? 5 : (pSeqInfo->profile-1);
        rpt->level = pSeqInfo->level==4 ? 0 : pSeqInfo->level==6 ? 1 : pSeqInfo->level==8 ? 2 : pSeqInfo->level==10 ? 3 : 4;
        break;
    case STD_MPEG4:
        if (pSeqInfo->level & 0x80) {// if VOS Header
            pSeqInfo->level &= 0x7F;
            if (pSeqInfo->level == 8 && pSeqInfo->profile == 0) {
                rpt->level = 0; rpt->profile = 0; // Simple, Level_L0
            } else {
                switch(pSeqInfo->profile) {
                case 0xB:	rpt->profile = 2; break;
                case 0xF:	if ((pSeqInfo->level&8) == 0) 
                                rpt->profile = 1; 
                            else
                                rpt->profile = 5;
                    break;
                case 0x0:	rpt->profile = 0; break;
                default :	rpt->profile = 5; break;
                }
                rpt->level = pSeqInfo->level;
            }
        } else { // Vol Header Only
            rpt->level = 7; // reserved level
            switch(pSeqInfo->profile) {
            case  0x1: rpt->profile = 0; break; // simple object
            case  0xC: rpt->profile = 2; break; // advanced coding efficiency object
            case 0x11: rpt->profile = 1; break; // advanced simple object
            default  : rpt->profile = 5; break; // reserved
            }
        }
        break;
    case STD_VC1:
        rpt->profile = pSeqInfo->profile;
        rpt->level = pSeqInfo->level;
        break;
    case STD_AVC:
        rpt->profile = (pSeqInfo->profile==66) ? 0 : (pSeqInfo->profile==77) ? 1 : (pSeqInfo->profile==88) ? 2 : (pSeqInfo->profile==100) ? 3 : 4;
        if(rpt->profile<3) {
            // BP, MP, EP
            rpt->level = (pSeqInfo->level==11 && pSeqInfo->constraint_set_flag[3] == 1) ? 0 /*1b*/ 
                : (pSeqInfo->level>=10 && pSeqInfo->level <= 51) ? 1 : 2;
        } else {
            // HP
            rpt->level = (pSeqInfo->level==9) ? 0 : (pSeqInfo->level>=10 && pSeqInfo->level <= 51) ? 1 : 2;
        }

        break;
    case STD_RV:
        rpt->profile = pSeqInfo->profile - 8;
        rpt->level = pSeqInfo->level;
        break;
    case STD_H263:
        rpt->profile = pSeqInfo->profile;
        rpt->level = pSeqInfo->level;
        break;

    case STD_AVS:
        rpt->profile = pSeqInfo->profile - 0x20;
        switch(pSeqInfo->level) {
        case 0x10 : rpt->level = 0; break;
        case 0x20 : rpt->level = 1; break;
        case 0x22 : rpt->level = 2; break;
        case 0x40 : rpt->level = 3; break;
        case 0x42 : rpt->level = 4; break;
        }
        break;

    default: 
        rpt->profile = 0;
        rpt->level = 0;
    }


    if (!rpt->fpSeqDispInfoLogfile)
        return;

    osal_fprintf(rpt->fpSeqDispInfoLogfile, "%s\n", strProfile[bitstreamFormat][rpt->profile]);
    if (bitstreamFormat != STD_RV) { // No level information in Rv.
        if (bitstreamFormat == STD_AVC && rpt->level != 0 && rpt->level != 2)
            osal_fprintf(rpt->fpSeqDispInfoLogfile, "%s%.1f\n", strLevel[bitstreamFormat][rpt->level], (float)pSeqInfo->level/10);
        else
            osal_fprintf(rpt->fpSeqDispInfoLogfile, "%s\n", strLevel[bitstreamFormat][rpt->level]);
    }

    if(bitstreamFormat == STD_AVC) {
        osal_fprintf(rpt->fpSeqDispInfoLogfile, "frame_mbs_only_flag : %d\n", pSeqInfo->interlace);
    } else if (bitstreamFormat == STD_AVS) {
        if (pSeqInfo->interlace)
            osal_fprintf(rpt->fpSeqDispInfoLogfile, "%s\n", "Progressive Sequence");
        else
            osal_fprintf(rpt->fpSeqDispInfoLogfile, "%s\n", "Interlaced Sequence");
    } else if (bitstreamFormat != STD_RV) {// No interlace information in Rv.
        if (pSeqInfo->interlace)
            osal_fprintf(rpt->fpSeqDispInfoLogfile, "%s\n", "Interlaced Sequence");
        else
            osal_fprintf(rpt->fpSeqDispInfoLogfile, "%s\n", "Progressive Sequence");
    }

    if (bitstreamFormat == STD_VC1) {
        if (pSeqInfo->vc1Psf)
            osal_fprintf(rpt->fpSeqDispInfoLogfile, "%s\n", "VC1 - Progressive Segmented Frame");
        else
            osal_fprintf(rpt->fpSeqDispInfoLogfile, "%s\n", "VC1 - Not Progressive Segmented Frame");
    }

    osal_fprintf(rpt->fpSeqDispInfoLogfile, "Aspect Ratio [%1d]\n\n", pSeqInfo->aspectRateInfo);

    osal_fflush(rpt->fpSeqDispInfoLogfile);


    switch (bitstreamFormat) {
    case STD_AVC:
        VLOG(INFO, "H.264 Profile:%s\n Level:%s\n FrameMbsOnlyFlag:%d\n \n\n",
            strProfile[bitstreamFormat][rpt->profile], strLevel[bitstreamFormat][rpt->level], pSeqInfo->interlace);
        if(pSeqInfo->aspectRateInfo != -1) {
            int aspect_ratio_idc;
            int sar_width, sar_height;

            if( (pSeqInfo->avcIsExtSAR)==0 ) {
                aspect_ratio_idc = (pSeqInfo->aspectRateInfo & 0xFF);
                VLOG(INFO, "aspect_ratio_idc :%d\n", aspect_ratio_idc);
            }
            else {
                sar_width  = (pSeqInfo->aspectRateInfo >> 16);
                sar_height  = (pSeqInfo->aspectRateInfo & 0xFFFF);
                VLOG(INFO, "sar_width  : %d\nsar_height : %d", sar_width, sar_height);				
            }
        } else {
            VLOG(INFO, "Aspect Ratio is not present\n");
        }

        if(pSeqInfo->bitRate == -1) {
            VLOG(INFO, "bit_rate is not present\n");
        }
        else {
            VLOG(INFO, "bit_rate is dummy(0x%x)\n", pSeqInfo->bitRate);
        }


        break;
    case STD_VC1:
        if(rpt->profile == 0) {
            VLOG(INFO, "* VC1 Profile: Simple\n");
        }
        else if(rpt->profile == 1) {
            VLOG(INFO, "* VC1 Profile: Main\n");
        }
        else if(rpt->profile == 2) {
            VLOG(INFO, "* VC1 Profile: Advanced\n");
        }
        else  {
            VLOG(INFO, "* VC1 Profile: Not support\n");
        }

        VLOG(INFO, "* Level: %s Interlace: %d PSF: %d\n", 
            strLevel[bitstreamFormat][rpt->level], pSeqInfo->interlace, pSeqInfo->vc1Psf);

        if(pSeqInfo->aspectRateInfo)
            VLOG(INFO, "* Aspect Ratio [X, Y]:[%3d, %3d]\n", (pSeqInfo->aspectRateInfo>>8)&0xff,
            (pSeqInfo->aspectRateInfo)&0xff);
        else
            VLOG(INFO, "* Aspect Ratio is not present\n");

        if(pSeqInfo->bitRate)
            VLOG(INFO, "* HRD_RATE : %d\n", pSeqInfo->bitRate);

        break;
    case STD_MPEG2:
        VLOG(INFO, "Mpeg2 Profile:%s Level:%s Progressive Sequence Flag:%d\n",
            strProfile[bitstreamFormat][rpt->profile], strLevel[bitstreamFormat][rpt->level], !pSeqInfo->interlace);
        // Profile: 3'b101: Simple, 3'b100: Main, 3'b011: SNR Scalable, 
        // 3'b10: Spatially Scalable, 3'b001: High
        // Level: 4'b1010: Low, 4'b1000: Main, 4'b0110: High 1440, 4'b0100: High
        if(pSeqInfo->aspectRateInfo)
            VLOG(INFO, "Aspect Ratio Table index :%d\n", pSeqInfo->aspectRateInfo);
        else
            VLOG(INFO, "Aspect Ratio is not present");

        if(pSeqInfo->bitRate == -1)
            VLOG(INFO, "bit_rate is not present\n");
        else 
            VLOG(INFO, "bit_rate : 0x%x\n\n",pSeqInfo->bitRate );
        break;
    case STD_MPEG4:
        VLOG(INFO, "Mpeg4 Profile: %s Level: %s Interlaced: %d\n",
            strProfile[bitstreamFormat][rpt->profile], strLevel[bitstreamFormat][rpt->level], pSeqInfo->interlace);
        // Profile: 8'b00000000: SP, 8'b00010001: ASP
        // Level: 4'b0000: L0, 4'b0001: L1, 4'b0010: L2, 4'b0011: L3, ...
        // SP: 1/2/3/4a/5/6, ASP: 0/1/2/3/4/5

        if(pSeqInfo->aspectRateInfo)
        {
            VLOG(INFO, "Aspect Ratio ParCode :%d\n", pSeqInfo->aspectRateInfo&0x0f);
            if ( (pSeqInfo->aspectRateInfo&0x0f) == 0x0f ) // 0xF: extended PAR
            {
                VLOG(INFO, "Aspect Ratio extended PAR\n");
                VLOG(INFO, "Aspect Ratio PAR Height :%d\n", (pSeqInfo->aspectRateInfo>>12)&0xff);
                VLOG(INFO, "Aspect Ratio PAR Width :%d\n", (pSeqInfo->aspectRateInfo>>4)&0xff);
            }
        }
        else
            VLOG(INFO, "Aspect Ratio is not present");

        if(pSeqInfo->bitRate == -1)
            VLOG(INFO, "bit_rate is not present\n");
        else 
            VLOG(INFO, "bit_rate : 0x%x\n\n",pSeqInfo->bitRate );
        break;

    case STD_RV:
        VLOG(INFO, "Real Video Version %s\n", strProfile[bitstreamFormat][rpt->profile]);

        if(pSeqInfo->bitRate == -1)
            VLOG(INFO, "bit_rate is not present\n");

        if(pSeqInfo->aspectRateInfo)
            VLOG(INFO, "Aspect Ratio :%d\n", pSeqInfo->aspectRateInfo);

        break;
    case STD_AVS:
        VLOG(INFO, "AVS Profile: %s Level: %s Progressive Sequence Flag: %d\n", 
            strProfile[bitstreamFormat][rpt->profile], strLevel[bitstreamFormat][rpt->level], !pSeqInfo->interlace);

        if(pSeqInfo->bitRate == -1)
            VLOG(INFO, "bit_rate is not present\n");
        else 
            VLOG(INFO, "bit_rate : 0x%x\n\n",pSeqInfo->bitRate );

        if(pSeqInfo->aspectRateInfo)
            VLOG(INFO, "Aspect Ratio Info (Table 7-5):%d\n", pSeqInfo->aspectRateInfo);

        break;
    default:
        break;

    }

    // Save Userdata report if enable user data option.
    if ((bitstreamFormat == STD_MPEG2 || bitstreamFormat == STD_AVC) && 
        rpt->userDataEnable && pSeqInfo->userDataSize) 
    {
        int size;
        BYTE *userDataBuf;

        if (pSeqInfo->userDataBufFull)
            VLOG(ERR, "User Data Buffer is Full\n");

        size = (pSeqInfo->userDataSize+7)/8*8 + USER_DATA_INFO_OFFSET;
        userDataBuf = osal_malloc(size);
        osal_memset(userDataBuf, 0, size);

        vdi_read_memory(core_idx, rpt->vb_rpt.phys_addr, userDataBuf, size, VDI_BIG_ENDIAN);

        SaveSeqUserData(core_idx, userDataBuf, bitstreamFormat);
        osal_free(userDataBuf);		
    }

    if (bitstreamFormat == STD_RV || bitstreamFormat == STD_DIV3) // RV & DIV3 has no user data
        rpt->userDataEnable = 0;
}


void ConfigDecReport(Uint32 core_idx, DecHandle handle, CodStd bitstreamFormat)
{
    vpu_rpt_info_t *rpt = &s_rpt_info[core_idx];

    if (!rpt->reportOpened)
        return ;

    // Report Information
    if (!rpt->vb_rpt.base)
    {
        rpt->vb_rpt.size     = SIZE_REPORT_BUF;
        if (vdi_allocate_dma_memory(core_idx, &rpt->vb_rpt) < 0)
        {
            VLOG(ERR, "fail to allocate report  buffer\n" );
            return;
        }
    }

    VPU_DecGiveCommand( handle, SET_ADDR_REP_USERDATA, &rpt->vb_rpt.phys_addr );
    VPU_DecGiveCommand( handle, SET_SIZE_REP_USERDATA, &rpt->vb_rpt.size );
    VPU_DecGiveCommand( handle, SET_USERDATA_REPORT_MODE, &rpt->userDataReportMode );

    if (rpt->userDataEnable == 1) {
        VPU_DecGiveCommand( handle, ENABLE_REP_USERDATA, 0 );
    } else {
        VPU_DecGiveCommand( handle, DISABLE_REP_USERDATA, 0 );
    }
}

void SaveDecReport(Uint32 core_idx, DecHandle handle, DecOutputInfo *pDecInfo, CodStd bitstreamFormat, int mbNumX)
{
    vpu_rpt_info_t *rpt = &s_rpt_info[core_idx];

    if (!rpt->reportOpened)
        return ;

    // Report Information	

    // User Data
    if ((pDecInfo->indexFrameDecoded >= 0 || (bitstreamFormat == STD_VC1)) // Vc1 Frame user data follow picture. After last frame decoding, user data should be reported.
        && rpt->userDataEnable && pDecInfo->decOutputExtData.userDataSize) {
            int size;
            BYTE *userDataBuf;

            if (pDecInfo->decOutputExtData.userDataBufFull)
                VLOG(ERR, "User Data Buffer is Full\n");

            size = (pDecInfo->decOutputExtData.userDataSize+7)/8*8 + USER_DATA_INFO_OFFSET;
            userDataBuf = osal_malloc(size);
            osal_memset(userDataBuf, 0, size);

            vdi_read_memory(core_idx, rpt->vb_rpt.phys_addr, userDataBuf, size,  VDI_BIG_ENDIAN);
            if (pDecInfo->indexFrameDecoded >= 0)
                SaveUserData(core_idx, userDataBuf);
            osal_free(userDataBuf);
    }

    if (((pDecInfo->indexFrameDecoded >= 0 || (bitstreamFormat == STD_VC1 )) && rpt->userDataEnable) || // Vc1 Frame user data follow picture. After last frame decoding, user data should be reported.
        (pDecInfo->indexFrameDisplay >= 0 && rpt->userDataEnable) ) {
            int size;
            BYTE *userDataBuf;
            int dataSize;

            if (pDecInfo->decOutputExtData.userDataBufFull)
                VLOG(ERR, "User Data Buffer is Full\n");

            dataSize = pDecInfo->decOutputExtData.userDataSize % rpt->vb_rpt.size;

            if (dataSize == 0 && pDecInfo->decOutputExtData.userDataSize != 0)	{
                dataSize = rpt->vb_rpt.size;
            }

            size = (dataSize+7)/8*8 + USER_DATA_INFO_OFFSET;
            userDataBuf = osal_malloc(size);
            osal_memset(userDataBuf, 0, size);
            vdi_read_memory(core_idx, rpt->vb_rpt.phys_addr, userDataBuf, size, VDI_BIG_ENDIAN);
            if (pDecInfo->indexFrameDecoded >= 0 || (bitstreamFormat == STD_VC1))
                SaveUserDataINT(core_idx, userDataBuf, size, 0, rpt->decIndex, bitstreamFormat);		
            osal_free(userDataBuf);
    }

    if (pDecInfo->indexFrameDecoded >= 0) {
        if (rpt->fpPicTypeLogfile == NULL) rpt->fpPicTypeLogfile = osal_fopen(FN_PIC_TYPE, "w+");
        osal_fprintf(rpt->fpPicTypeLogfile, "FRAME [%1d]\n", rpt->decIndex);

        switch (bitstreamFormat) {
        case STD_AVC:
            if(pDecInfo->pictureStructure == 3) {	// FIELD_INTERLACED
                osal_fprintf(rpt->fpPicTypeLogfile, "Top Field Type: [%s]\n", pDecInfo->picTypeFirst == 0 ? "I_TYPE" : 
                    (pDecInfo->picTypeFirst) == 1 ? "P_TYPE" :
                    (pDecInfo->picTypeFirst) == 2 ? "BI_TYPE" :
                    (pDecInfo->picTypeFirst) == 3 ? "B_TYPE" :
                    (pDecInfo->picTypeFirst) == 4 ? "SKIP_TYPE" :
                    (pDecInfo->picTypeFirst) == 5 ? "IDR_TYPE" :
                    "FORBIDDEN"); 

            osal_fprintf(rpt->fpPicTypeLogfile, "Bottom Field Type: [%s]\n", pDecInfo->picType == 0 ? "I_TYPE" : 
                (pDecInfo->picType) == 1 ? "P_TYPE" :
                (pDecInfo->picType) == 2 ? "BI_TYPE" :
                (pDecInfo->picType) == 3 ? "B_TYPE" :
                (pDecInfo->picType) == 4 ? "SKIP_TYPE" :
                (pDecInfo->picType) == 5 ? "IDR_TYPE" :
                "FORBIDDEN"); 
            }
            else {
                osal_fprintf(rpt->fpPicTypeLogfile, "Picture Type: [%s]\n", pDecInfo->picType == 0 ? "I_TYPE" : 
                    (pDecInfo->picTypeFirst) == 1 ? "P_TYPE" :
                    (pDecInfo->picTypeFirst) == 2 ? "BI_TYPE" :
                    (pDecInfo->picTypeFirst) == 3 ? "B_TYPE" :
                    (pDecInfo->picTypeFirst) == 4 ? "SKIP_TYPE" :
                    (pDecInfo->picTypeFirst) == 5 ? "IDR_TYPE" :
                    "FORBIDDEN"); 
            }
            break;
        case STD_MPEG2 :
            osal_fprintf(rpt->fpPicTypeLogfile, "Picture Type: [%s]\n", pDecInfo->picType == 0 ? "I_TYPE" : 
                pDecInfo->picType == 1 ? "P_TYPE" :
                pDecInfo->picType == 2 ? "B_TYPE" :
                "D_TYPE"); break;
        case STD_MPEG4 :
            osal_fprintf(rpt->fpPicTypeLogfile, "Picture Type: [%s]\n", pDecInfo->picType == 0 ? "I_TYPE" : 
                pDecInfo->picType == 1 ? "P_TYPE" :
                pDecInfo->picType == 2 ? "B_TYPE" :
                "S_TYPE"); break;
        case STD_VC1:
            if(pDecInfo->pictureStructure == 3) {	// FIELD_INTERLACED
                osal_fprintf(rpt->fpPicTypeLogfile, "Top Field Type: [%s]\n", pDecInfo->picTypeFirst == 0 ? "I_TYPE" : 
                    (pDecInfo->picTypeFirst) == 1 ? "P_TYPE" :
                    (pDecInfo->picTypeFirst) == 2 ? "BI_TYPE" :
                    (pDecInfo->picTypeFirst) == 3 ? "B_TYPE" :
                    (pDecInfo->picTypeFirst) == 4 ? "SKIP_TYPE" :
                    "FORBIDDEN"); 

            osal_fprintf(rpt->fpPicTypeLogfile, "Bottom Field Type: [%s]\n", pDecInfo->picType == 0 ? "I_TYPE" : 
                (pDecInfo->picType) == 1 ? "P_TYPE" :
                (pDecInfo->picType) == 2 ? "BI_TYPE" :
                (pDecInfo->picType) == 3 ? "B_TYPE" :
                (pDecInfo->picType) == 4 ? "SKIP_TYPE" :
                "FORBIDDEN"); 
            }
            else {
                osal_fprintf(rpt->fpPicTypeLogfile, "Picture Type: [%s]\n", pDecInfo->picType == 0 ? "I_TYPE" : 
                    (pDecInfo->picTypeFirst) == 1 ? "P_TYPE" :
                    (pDecInfo->picTypeFirst) == 2 ? "BI_TYPE" :
                    (pDecInfo->picTypeFirst) == 3 ? "B_TYPE" :
                    (pDecInfo->picTypeFirst) == 4 ? "SKIP_TYPE" :
                    "FORBIDDEN"); 
            }
            break;
        default:
            osal_fprintf(rpt->fpPicTypeLogfile, "Picture Type: [%s]\n", pDecInfo->picType == 0 ? "I_TYPE" : 
                pDecInfo->picType == 1 ? "P_TYPE" :
                "B_TYPE");
        }
    }

    if (pDecInfo->indexFrameDecoded >= 0) {
        if (rpt->fpPicDispInfoLogfile == NULL) rpt->fpPicDispInfoLogfile = osal_fopen(FN_PIC_INFO, "w+");
        osal_fprintf(rpt->fpPicDispInfoLogfile, "FRAME [%1d]\n", rpt->decIndex);

        switch (bitstreamFormat) {
        case STD_MPEG2:
            osal_fprintf(rpt->fpPicDispInfoLogfile, "%s\n", 
                pDecInfo->picType == 0 ? "I_TYPE" : 
                pDecInfo->picType == 1 ? "P_TYPE" :
                pDecInfo->picType == 2 ? "B_TYPE" :
                "D_TYPE"); break;
        case STD_MPEG4:
            osal_fprintf(rpt->fpPicDispInfoLogfile, "%s\n", 
                pDecInfo->picType == 0 ? "I_TYPE" : 
                pDecInfo->picType == 1 ? "P_TYPE" :
                pDecInfo->picType == 2 ? "B_TYPE" :
                "S_TYPE"); break;
        case STD_VC1 :
            if(pDecInfo->pictureStructure == 3) {	// FIELD_INTERLACED
                osal_fprintf(rpt->fpPicDispInfoLogfile, "Top : %s\n", (pDecInfo->picType>>3) == 0 ? "I_TYPE" : 
                    (pDecInfo->picType>>3) == 1 ? "P_TYPE" :
                    (pDecInfo->picType>>3) == 2 ? "BI_TYPE" :
                    (pDecInfo->picType>>3) == 3 ? "B_TYPE" :
                    (pDecInfo->picType>>3) == 4 ? "SKIP_TYPE" :
                    "FORBIDDEN"); 

            osal_fprintf(rpt->fpPicDispInfoLogfile, "Bottom : %s\n", (pDecInfo->picType&0x7) == 0 ? "I_TYPE" : 
                (pDecInfo->picType&0x7) == 1 ? "P_TYPE" :
                (pDecInfo->picType&0x7) == 2 ? "BI_TYPE" :
                (pDecInfo->picType&0x7) == 3 ? "B_TYPE" :
                (pDecInfo->picType&0x7) == 4 ? "SKIP_TYPE" :
                "FORBIDDEN"); 

            osal_fprintf(rpt->fpPicDispInfoLogfile, "%s\n", "Interlaced Picture");
            }
            else {
                osal_fprintf(rpt->fpPicDispInfoLogfile, "%s\n", (pDecInfo->picType>>3) == 0 ? "I_TYPE" : 
                    (pDecInfo->picType>>3) == 1 ? "P_TYPE" :
                    (pDecInfo->picType>>3) == 2 ? "BI_TYPE" :
                    (pDecInfo->picType>>3) == 3 ? "B_TYPE" :
                    (pDecInfo->picType>>3) == 4 ? "SKIP_TYPE" :
                    "FORBIDDEN"); 

            osal_fprintf(rpt->fpPicDispInfoLogfile, "%s\n", "Frame Picture");
            }
            break;
        default:
            osal_fprintf(rpt->fpPicDispInfoLogfile, "%s\n", 
                pDecInfo->picType == 0 ? "I_TYPE" : 
                pDecInfo->picType == 1 ? "P_TYPE" :
                "B_TYPE"); break;
        }

        if(bitstreamFormat != STD_VC1) {
            if (pDecInfo->interlacedFrame) {
                if(bitstreamFormat == STD_AVS)
                    osal_fprintf(rpt->fpPicDispInfoLogfile, "%s\n", "Frame Picture");
                else
                    osal_fprintf(rpt->fpPicDispInfoLogfile, "%s\n", "Interlaced Picture");
            }
            else {
                if(bitstreamFormat == STD_AVS)
                    osal_fprintf(rpt->fpPicDispInfoLogfile, "%s\n", "Interlaced Picture");
                else
                    osal_fprintf(rpt->fpPicDispInfoLogfile, "%s\n", "Frame Picture");
            }
        }

        if (bitstreamFormat != STD_RV) {
            if(bitstreamFormat == STD_VC1)
            {
                switch(pDecInfo->pictureStructure) {
                case 0:	osal_fprintf(rpt->fpPicDispInfoLogfile, "%s\n", "PROGRESSIVE");	break;
                case 2:	osal_fprintf(rpt->fpPicDispInfoLogfile, "%s\n", "FRAME_INTERLACE"); break;
                case 3:	osal_fprintf(rpt->fpPicDispInfoLogfile, "%s\n", "FIELD_INTERLACE");	break;
                default: osal_fprintf(rpt->fpPicDispInfoLogfile, "%s\n", "FORBIDDEN"); break;				
                }
            }
            else if(bitstreamFormat == STD_AVC)
            {
                if(!pDecInfo->interlacedFrame)
                    osal_fprintf(rpt->fpPicDispInfoLogfile, "%s\n", "FRAME_PICTURE");
                else {
                    if(pDecInfo->topFieldFirst)
                        osal_fprintf(rpt->fpPicDispInfoLogfile, "%s\n", "Top Field First");
                    else
                        osal_fprintf(rpt->fpPicDispInfoLogfile, "%s\n", "Bottom Field First");

                }
            }
            else if (bitstreamFormat != STD_MPEG4 && bitstreamFormat != STD_AVS) {
                switch(pDecInfo->pictureStructure) {
                case 1:	osal_fprintf(rpt->fpPicDispInfoLogfile, "%s\n", "TOP_FIELD");	break;
                case 2:	osal_fprintf(rpt->fpPicDispInfoLogfile, "%s\n", "BOTTOM_FIELD"); break;
                case 3:	osal_fprintf(rpt->fpPicDispInfoLogfile, "%s\n", "FRAME_PICTURE");	break;
                default: osal_fprintf(rpt->fpPicDispInfoLogfile, "%s\n", "FORBIDDEN"); break;				
                }
            }

            if(bitstreamFormat != STD_AVC)
            {
                if (pDecInfo->topFieldFirst)
                    osal_fprintf(rpt->fpPicDispInfoLogfile, "%s\n", "Top Field First");
                else
                    osal_fprintf(rpt->fpPicDispInfoLogfile, "%s\n", "Bottom Field First");

                if (bitstreamFormat != STD_MPEG4) {
                    if (pDecInfo->repeatFirstField)
                        osal_fprintf(rpt->fpPicDispInfoLogfile, "%s\n", "Repeat First Field");
                    else
                        osal_fprintf(rpt->fpPicDispInfoLogfile, "%s\n", "Not Repeat First Field");

                    if (bitstreamFormat == STD_VC1)
                        osal_fprintf(rpt->fpPicDispInfoLogfile, "VC1 RPTFRM [%1d]\n", pDecInfo->progressiveFrame);
                    else if (pDecInfo->progressiveFrame)
                        osal_fprintf(rpt->fpPicDispInfoLogfile, "%s\n", "Progressive Frame");
                    else
                        osal_fprintf(rpt->fpPicDispInfoLogfile, "%s\n", "Interlaced Frame");
                }
            }
        }

        if (bitstreamFormat == STD_MPEG2)
            osal_fprintf(rpt->fpPicDispInfoLogfile, "Field Sequence [%d]\n\n", pDecInfo->fieldSequence);
        else
            osal_fprintf(rpt->fpPicDispInfoLogfile, "\n");

        osal_fflush(rpt->fpPicDispInfoLogfile);
    }

    if(pDecInfo->indexFrameDecoded >= 0)
        rpt->decIndex ++;

}


VpuReportContext_t *OpenDecReport(Uint32 core_idx, VpuReportConfig_t *cfg)
{
    vpu_rpt_info_t *rpt = &s_rpt_info[core_idx];
    // To be replaced with VpuReportContext_t
    rpt->fpPicDispInfoLogfile = NULL;
    rpt->fpPicTypeLogfile = NULL;
    rpt->fpSeqDispInfoLogfile = NULL;
    rpt->fpUserDataLogfile = NULL;
    rpt->fpSeqUserDataLogfile = NULL;


    rpt->decIndex = 0;
    rpt->userDataEnable = VPU_REPORT_USERDATA;
    rpt->userDataReportMode = 0;		// User Data report mode (0 : interrupt mode, 1 interrupt disable mode)
    rpt->userDataEnable = cfg->userDataEnable;
    rpt->userDataReportMode = cfg->userDataReportMode;

    rpt->reportOpened = 1;
    return (VpuReportContext_t *)NULL;
}

void CloseDecReport(Uint32 core_idx)
{
    vpu_rpt_info_t *rpt = &s_rpt_info[core_idx];

    if (!rpt->reportOpened)
        return ;

    if (rpt->fpPicTypeLogfile) {
        osal_fclose(rpt->fpPicTypeLogfile);
        rpt->fpPicTypeLogfile = NULL;
    }
    if (rpt->fpSeqDispInfoLogfile) {
        osal_fclose(rpt->fpSeqDispInfoLogfile);
        rpt->fpSeqDispInfoLogfile = NULL;
    }


    if (rpt->fpUserDataLogfile) {
        osal_fclose(rpt->fpUserDataLogfile);
        rpt->fpUserDataLogfile= NULL;
    }

    if (rpt->fpSeqUserDataLogfile) {
        osal_fclose(rpt->fpSeqUserDataLogfile);
        rpt->fpSeqUserDataLogfile = NULL;
    }

    if (rpt->vb_rpt.base)
    {
        vdi_free_dma_memory(core_idx, &rpt->vb_rpt);
    }
    rpt->decIndex = 0;
}


void CheckUserDataInterrupt(Uint32 core_idx, DecHandle handle, int decodeIdx, CodStd bitstreamFormat, int int_reason)
{
    vpu_rpt_info_t *rpt = &s_rpt_info[core_idx];

    if (int_reason & (1<<INT_BIT_USERDATA)) 
    {
        // USER DATA INTERRUPT Issued
        // User Data save
        if (rpt->userDataEnable) {
            int size;
            BYTE *userDataBuf;
            size = rpt->vb_rpt.size + USER_DATA_INFO_OFFSET;
            userDataBuf = osal_malloc(size);
            osal_memset(userDataBuf, 0, size);

            vdi_read_memory(core_idx, rpt->vb_rpt.phys_addr, userDataBuf, size, VDI_BIG_ENDIAN);
            if (decodeIdx >= 0)
                SaveUserDataINT(core_idx, userDataBuf, size, 1, rpt->decIndex, bitstreamFormat);
            osal_free(userDataBuf);
        } else {
            VLOG(ERR, "Unexpected Interrupt issued");
        }					

    }
}


/******************************************************************************
Read Version
******************************************************************************/


void CheckVersion(Uint32 core_idx)
{
    Uint32 version;
    Uint32 revision;
    Uint32 productId;


    VPU_GetVersionInfo(core_idx, &version, &revision, &productId);	

    VLOG(INFO, "VPU coreNum : [%d]\n", core_idx);
    VLOG(INFO, "Firmware Version => projectId : %x | version : %04d.%04d.%08d | revision : r%d\n", 
        (Uint32)(version>>16), (Uint32)((version>>(12))&0x0f), (Uint32)((version>>(8))&0x0f), (Uint32)((version)&0xff), revision);
    VLOG(INFO, "Hardware Version => %04x\n", productId);
    VLOG(INFO, "API Version => %04x\n\n", API_VERSION);
}




/******************************************************************************
Submenu to test runtime options
******************************************************************************/

int FrameMenu(Uint32 option[])
{
    int val;
    osal_close_keyboard();
    printf("------------------------------------\n");
    printf("--  function for decoding frame   --\n");
    printf("------------------------------------\n");

    printf("1.Random Access\n");
    printf("2.I-frame Search\n");
    printf("3.Skip Frame\n");
    printf("4.Display output info according to display index\n");
    printf("5.Continue\n");
    //---- VPU_SLEEP/WAKE
    printf("10.VPU Sleep\n");
    printf("11.VPU Wake\n");
    printf("12.Re-Init\n");
    printf("15.VPU Reset\n");
    printf("Enter Option : ");

    scanf("%d", &val );	
    if( val == 1 )
    {
        printf("Enter random access range(%%) 0 ~ 100 : ");
        scanf("%d", &option[0] );	
    }
    else if( val == 2 )
    {
        option[0] = 1;
    }
    else if( val == 3 )
    {
        printf("frame skip mode?(	0: disable");
        printf("\n			1: skip except I(IDR) picture");
        printf("\n			2: skip B picture");
        scanf("%d", &option[0] );	
        option[1] = 1;
    }
    else if ( val == 4 )
    {
        printf("Enter display frame index : ");
        scanf("%d", &option[0] );
    }
    osal_init_keyboard();
    return val;
}


/**
* UI_GetUserCommand
* IN
* OUT
*    return UI_CMD_ERROR if error
*    return UI_CMD_KEEP_GOING if application should keep going
*    return UI_CMD_SKIP_FRAME_LIFECYCLE if application should skip to next frame lifecycle
*    return UI_CMD_RANDOM_ACCESS if application need random access 
*/
int UI_GetUserCommand(int core_idx, DecHandle handle, DecParam *decParam, int *randomAccessPos)
{
    int menu = 0;

    Uint32 option[5] = {0};
//#define TEST_POWER_DOWN
#ifdef  TEST_POWER_DOWN
    static BYTE* pDRAM = NULL;
#endif
    menu = FrameMenu( option );
    if( menu == 1 )
    {
        if(randomAccessPos == NULL)
            return UI_CMD_KEEP_GOING;
        decParam->iframeSearchEnable = 1;
        *randomAccessPos = (option[0] % 100);
        return UI_CMD_RANDOM_ACCESS;
    }
    else if( menu == 2 )
    {
        decParam->iframeSearchEnable = option[0];
    }
    else if( menu == 3 )
    {
        decParam->skipframeMode = option[0];
        if( decParam->skipframeMode != 0 )
        {
            decParam->iframeSearchEnable = 0;
        }
    }
    else if ( menu == 4 )
    {
        DecOutputInfo decOutInfo;
        
        decOutInfo.indexFrameDisplay =  option[0];
        VPU_DecGiveCommand(handle, DEC_GET_DISPLAY_OUTPUT_INFO, &decOutInfo);
        // print some information
        VLOG(INFO, "\n PIC Type : %d ", decOutInfo.picType);
        VLOG(INFO, "\n rdptr : 0x%08x, wrptr : 0x%08x ", decOutInfo.rdPtr, decOutInfo.wrPtr);
        VLOG(INFO, "\n Enter any key..");
        osal_getch();
        osal_flush_ch();
    }
    //---- VPU_SLEEP
    else if( menu == 10 ) 
    {
        VPU_SleepWake(core_idx, 1);
#ifdef  TEST_POWER_DOWN
        if(pDRAM==NULL) {
            pDRAM = osal_malloc(1024*1024*10);
			vdi_read_memory(core_idx, 0x80000000, pDRAM, 1024*1024*10, VDI_BIG_ENDIAN)   ;      
        }
#endif
        return UI_CMD_SKIP_FRAME_LIFECYCLE;
    }
    //---- VPU_WAKE
    else if( menu == 11 ) 
    {		
#ifdef  TEST_POWER_DOWN
#ifdef CNM_FPGA_PLATFORM
        SetDefaultClock(core_idx);
        vdi_hw_reset(core_idx);
#if defined(_MSC_VER)
		Sleep(1000);
#else
		sleep(1);
#endif
		vdi_set_timing_opt(core_idx);
#endif	
        if(pDRAM) {
			vdi_write_memory(core_idx, 0x80000000, pDRAM, 1024*1024*10, VDI_BIG_ENDIAN);
            osal_free(pDRAM);
            pDRAM = NULL;
        }
#endif
        VPU_SleepWake(core_idx, 0);
#ifdef  TEST_POWER_DOWN
#ifdef CNM_FPGA_PLATFORM
		vdi_write_register(core_idx, 0x1000000 + 0x0000, 0x05000000);
		vdi_write_register(core_idx, 0x1000000 + 0x044, 0);
#endif
#endif
        return UI_CMD_SKIP_FRAME_LIFECYCLE;
    }
    else if( menu == 15 ) 
    {
        VPU_SWReset(core_idx, SW_RESET_SAFETY, NULL);
        return UI_CMD_SKIP_FRAME_LIFECYCLE;
    }

    return UI_CMD_KEEP_GOING;
}

/************************************************************************/
/* Utility functions													*/
/************************************************************************/

int HasStartCode( int bitstreamFormat, int profile )
{
    if (bitstreamFormat == STD_RV ||
        bitstreamFormat == STD_DIV3 )
        return 0;

    if (bitstreamFormat == STD_VC1)
    {
        if (profile != 2)	// if the profile of stream isn't advance profile;
            return 0;
    }

    return 1;
}



void PrintSEI(DecOutputInfo *out)
{
    printf("frame packing arrangement SEI - framePackingArrangementId: %d\n",                out->avcFpaSei.framePackingArrangementId);
    printf("frame packing arrangement SEI - frame_packing_arrangement_cancel_flag: %d\n",       out->avcFpaSei.framePackingArrangementCancelFlag);
    printf("frame packing arrangement SEI - quincinx_sampling_flag: %d\n",                      out->avcFpaSei.quincunxSamplingFlag);
    printf("frame packing arrangement SEI - spatial_flipping_flag: %d\n",                       out->avcFpaSei.spatialFlippingFlag);
    printf("frame packing arrangement SEI - frame0_flipped_flag: %d\n",                         out->avcFpaSei.frame0FlippedFlag);
    printf("frame packing arrangement SEI - fieldViewsFlag: %d\n",                            out->avcFpaSei.fieldViewsFlag);
    printf("frame packing arrangement SEI - currentFrameIsFrame0Flag: %d\n",                out->avcFpaSei.currentFrameIsFrame0Flag);
    printf("frame packing arrangement SEI - frame0SelfContainedFlag: %d\n",                  out->avcFpaSei.frame0SelfContainedFlag);
    printf("frame packing arrangement SEI - frame1SelfContainedFlag: %d\n",                  out->avcFpaSei.frame1SelfContainedFlag);
    printf("frame packing arrangement SEI - framePackingArrangementExtensionFlag: %d\n",    out->avcFpaSei.framePackingArrangementExtensionFlag);
    printf("frame packing arrangement SEI - framePackingArrangementType: %d\n",              out->avcFpaSei.framePackingArrangementType);
    printf("frame packing arrangement SEI - contentInterpretationType: %d\n",                 out->avcFpaSei.contentInterpretationType);
    printf("frame packing arrangement SEI - frame0GridPositionX: %d\n",                      out->avcFpaSei.frame0GridPositionX);
    printf("frame packing arrangement SEI - frame0GridPositionY: %d\n",                      out->avcFpaSei.frame0GridPositionY);
    printf("frame packing arrangement SEI - frame1GridPositionX: %d\n",                      out->avcFpaSei.frame1GridPositionX);
    printf("frame packing arrangement SEI - frame1GridPositionY: %d\n",                      out->avcFpaSei.frame1GridPositionY);
    printf("frame packing arrangement SEI - framePackingArrangementRepetitionPeriod: %d\n", out->avcFpaSei.framePackingArrangementRepetitionPeriod);
}



/******************************************************************************
Bitstream Buffer Control
******************************************************************************/

/**
* Bitstream Write for decoders
*/
RetCode WriteBsBufHelper(Uint32 core_idx, 
    DecHandle handle,
    BufInfo *pBufInfo,
    vpu_buffer_t *pVbStream,
    int defaultsize, 
    int checkeos,
    int *pstreameos,
    int endian)
{
    RetCode ret = RETCODE_SUCCESS;

    int size = 0;
    int fillSize = 0;
    PhysicalAddress paRdPtr, paWrPtr;

    ret = VPU_DecGetBitstreamBuffer(handle, &paRdPtr, &paWrPtr, &size);
    if( ret != RETCODE_SUCCESS )
    {
        VLOG(ERR, "VPU_DecGetBitstreamBuffer failed Error code is 0x%x \n", ret );
        goto FILL_BS_ERROR;				
    }

    if( size <= 0 )
        return RETCODE_SUCCESS;

    if( defaultsize > 0 )
    {

        if( size < defaultsize )
            fillSize = size;
        else
            fillSize = defaultsize;
    }
    else
    {
        fillSize = defaultsize;
    }

    if (fillSize == 0)
        return RETCODE_SUCCESS;

    fillSize = FillSdramBurst(core_idx, pBufInfo, paWrPtr, pVbStream, fillSize, checkeos, pstreameos, endian);

    //VLOG(ERR, "WriteBsBufHelper finish FillSdramBurst filesize = %d, streameos = %d\n", fillSize, *pstreameos);

    if (fillSize == 0)
        return RETCODE_SUCCESS;

    if( *pstreameos == 0 )
    {
        ret = VPU_DecUpdateBitstreamBuffer(handle, fillSize);
        if( ret != RETCODE_SUCCESS )
        {
            VLOG(ERR, "VPU_DecUpdateBitstreamBuffer failed Error code is 0x%x \n", ret );
            goto FILL_BS_ERROR;					
        }		

        if ((pBufInfo->size - pBufInfo->point) <= 0 && !pBufInfo->fillendbs)
        {
#define STREAM_END_SIZE			0
            ret = VPU_DecUpdateBitstreamBuffer(handle, STREAM_END_SIZE) ;
            if( ret != RETCODE_SUCCESS )
            {
                VLOG(ERR, "VPU_DecUpdateBitstreamBuffer failed Error code is 0x%x \n", ret );
                goto FILL_BS_ERROR;                 
            }

            pBufInfo->fillendbs = 1;
        }
    }
    else
    {
        if (!pBufInfo->fillendbs)
        {
#define STREAM_END_SIZE			0
            ret = VPU_DecUpdateBitstreamBuffer(handle, STREAM_END_SIZE) ;
            if( ret != RETCODE_SUCCESS )
            {
                VLOG(ERR, "VPU_DecUpdateBitstreamBuffer failed Error code is 0x%x \n", ret );
                goto FILL_BS_ERROR;					
            }	
            pBufInfo->fillendbs = 1;
        }					
    }

FILL_BS_ERROR:

    return ret;
}

int GetBitstreamBufferRoom(DecHandle handle)
{
    RetCode ret = RETCODE_SUCCESS;
    int room;
    PhysicalAddress paRdPtr, paWrPtr;

    ret = VPU_DecGetBitstreamBuffer(handle, &paRdPtr, &paWrPtr, &room);
    if( ret != RETCODE_SUCCESS )
    {
        VLOG(ERR, "VPU_DecGetBitstreamBuffer failed Error code is 0x%x \n", ret );
        return 0;				
    }

    return room;
}

int WriteBsBufFromBufHelper(Uint32 core_idx,
    DecHandle handle,
    vpu_buffer_t *pVbStream,
    BYTE *pChunk, 
    int chunkSize,
    int endian)
{
    RetCode ret = RETCODE_SUCCESS;
    int size = 0;
    int room = 0;
    PhysicalAddress paRdPtr, paWrPtr, targetAddr;
    
    
    if (chunkSize < 1)
        return 0;

    if (chunkSize > (int)pVbStream->size)
        return -1;	

    ret = VPU_DecGetBitstreamBuffer(handle, &paRdPtr, &paWrPtr, &size);
    if( ret != RETCODE_SUCCESS )
    {
        VLOG(ERR, "VPU_DecGetBitstreamBuffer failed Error code is 0x%x \n", ret );
        return -1;			
    }

    if(size < chunkSize)
        return 0; // no room for feeding bitstream. it will take a change to fill stream

    targetAddr = paWrPtr;
    if ((targetAddr+chunkSize) >  (pVbStream->phys_addr+pVbStream->size))
    {
        room =  (pVbStream->phys_addr+pVbStream->size) - targetAddr;

        //write to physical address
        vdi_write_memory(core_idx, targetAddr, pChunk, room, endian);
        vdi_write_memory(core_idx, pVbStream->phys_addr, pChunk+room, chunkSize-room, endian);

    }
    else
    {
        //write to physical address
        vdi_write_memory(core_idx, targetAddr, pChunk, chunkSize, endian);
    }

    ret = VPU_DecUpdateBitstreamBuffer(handle, chunkSize);
    if( ret != RETCODE_SUCCESS )
    {
        VLOG(ERR, "VPU_DecUpdateBitstreamBuffer failed Error code is 0x%x \n", ret );
        return -1;					
    }

    return chunkSize;
}




/**
* Bitstream Buffer Reset for encoders
*/

int FillBsResetBufHelper(Uint32 core_idx,
    BYTE *buf,
    PhysicalAddress paBsBufAddr,
    int bsBufsize,
    int endian)
{	
    if( !bsBufsize )
        return -1;
    VpuReadMem(core_idx, paBsBufAddr, buf, bsBufsize, endian);
    return bsBufsize;
}

RetCode ReadBsRingBufHelper(Uint32 core_idx,
    EncHandle handle,
    osal_file_t bsFp,
    PhysicalAddress bitstreamBuffer,
    Uint32 bitstreamBufferSize,
    int defaultsize,
    int endian)
{
    RetCode ret = RETCODE_SUCCESS;
    int loadSize = 0;
    PhysicalAddress paRdPtr, paWrPtr;
    int size = 0;
    PhysicalAddress paBsBufStart = bitstreamBuffer;
    PhysicalAddress paBsBufEnd   = bitstreamBuffer+bitstreamBufferSize;

    ret = VPU_EncGetBitstreamBuffer(handle, &paRdPtr, &paWrPtr, &size);
    if( ret != RETCODE_SUCCESS )
    {
        VLOG(ERR, "VPU_EncGetBitstreamBuffer failed Error code is 0x%x \n", ret );
        goto LOAD_BS_ERROR;
    }

    if( size > 0 )
    {
        if( defaultsize > 0 )
        {
            if( size < defaultsize )
                loadSize = ( ( size >> 9 ) << 9 ); 
            else
                loadSize = defaultsize;
        }
        else
        {
            loadSize = size;
        }

        if( loadSize > 0 )
        {
            ProcessEncodedBitstreamBurst(core_idx, bsFp, paRdPtr, paBsBufStart, paBsBufEnd, loadSize, endian );
            ret = VPU_EncUpdateBitstreamBuffer(handle, loadSize);
            if( ret != RETCODE_SUCCESS )
            {
                VLOG(ERR, "VPU_EncUpdateBitstreamBuffer failed Error code is 0x%x \n", ret );
                goto LOAD_BS_ERROR;
            }
        }

    }

LOAD_BS_ERROR:

    return ret;
}

int FillBsRingBufHelper(Uint32 core_idx,
    EncHandle handle,
    unsigned char *buf,
    PhysicalAddress paBsBufStart,
    PhysicalAddress paBsBufEnd,
    int defaultsize,
    int endian )
{
    RetCode ret = RETCODE_SUCCESS;
    int loadSize = 0;
    PhysicalAddress paRdPtr, paWrPtr;
    int size = 0;
    int room;

    ret = VPU_EncGetBitstreamBuffer(handle, &paRdPtr, &paWrPtr, &size);
    if( ret != RETCODE_SUCCESS )
    {
        VLOG(ERR, "VPU_EncGetBitstreamBuffer failed Error code is 0x%x \n", ret );
        goto LOAD_BS_ERROR;
    }

    if( size > 0 )
    {
        if( defaultsize > 0 )
        {
            if( size < defaultsize )
                loadSize = ( ( size >> 9 ) << 9 ); 
            else
                loadSize = defaultsize;
        }
        else
        {
            loadSize = size;
        }

        if( loadSize > 0 )
        {
			int addrStart = (int)(paRdPtr + loadSize);
	        if (addrStart > (int)paBsBufEnd)
            {
                room = paBsBufEnd - paRdPtr;
                VpuReadMem(core_idx, paRdPtr, buf, room,  endian);
                VpuReadMem(core_idx, paBsBufStart, buf+room, (loadSize-room), endian);
            }
            else
            {
                VpuReadMem(core_idx, paRdPtr, buf, loadSize, endian);
            }	

            ret = VPU_EncUpdateBitstreamBuffer(handle, loadSize);
            if( ret != RETCODE_SUCCESS )
            {
                VLOG(ERR, "VPU_EncUpdateBitstreamBuffer failed Error code is 0x%x \n", ret );
                goto LOAD_BS_ERROR;
            }
        }

    }

LOAD_BS_ERROR:

    return loadSize;
}

/******************************************************************************
DPB Image Data Control
******************************************************************************/
int LoadYuvImageHelperFormat(Uint32 core_idx,
    osal_file_t yuvFp,
    Uint8 *pYuv,
    FrameBuffer *fb,
    TiledMapConfig mapCfg,
    int picWidth,
    int picHeight,
    int stride,
    int interleave,
    int format,
    int endian)
{
    int frameSize;

    switch (format)
    {
    case FORMAT_420:
        frameSize = picWidth * picHeight * 3 / 2;
        break;
    case FORMAT_224:
        frameSize = picWidth * picHeight * 4 / 2;
        break;
    case FORMAT_422:
        frameSize = picWidth * picHeight * 4 / 2;
        break;
    case FORMAT_444:
        frameSize = picWidth * picHeight * 6 / 2;
        break;
    case FORMAT_400:
        frameSize = picWidth * picHeight;
        break;
	default:
		frameSize = picWidth * picHeight * 3 / 2;
		break;
    }

    // Load source one picture image to encode to SDRAM frame buffer.
    if( !osal_fread(pYuv, 1, frameSize, yuvFp) ) 
    {
        if( !osal_feof( yuvFp ) )
            VLOG(ERR, "Yuv Data osal_fread failed file handle is %p \n", yuvFp );			
        return 0;
    }

	LoadYuvImageBurstFormat(core_idx, pYuv, picWidth, picHeight, fb, stride, interleave,	format,	endian);
        

    return 1;
}


int SaveYuvImageHelperFormat(Uint32 core_idx,
    osal_file_t yuvFp,
    FrameBuffer *fbSrc,
    TiledMapConfig mapCfg,
    Uint8 *pYuv, 
    Rect rect,
    int interLeave,
    int format,
    int endian)
{
    int frameSize;
    int picWidth = fbSrc->stride;
    int picHeight = fbSrc->height;

    VLOG(ERR, "SaveYuvImageHelperFormat \n");

    switch (format)
    {
    case FORMAT_420:
        frameSize = picWidth * ((picHeight+1)/2*2) * 3/2;
        break;
    case FORMAT_224:
        frameSize = picWidth * ((picHeight+1)/2*2) * 4/2;
        break;
    case FORMAT_422:
        frameSize = picWidth * picHeight * 4/2;
        break;
    case FORMAT_444:
        frameSize = picWidth * picHeight * 6/2; 
        break;
    case FORMAT_400:
        frameSize = picWidth * picHeight;
        break;
	default:
		 frameSize = picWidth * ((picHeight+1)/2*2) * 3/2;
		 break;
    }

	StoreYuvImageBurstLinear(core_idx, 
		fbSrc, pYuv, rect,
		interLeave,
		format,
		endian);
    if (yuvFp)
    {
        if( !osal_fwrite(pYuv, sizeof(Uint8), frameSize , yuvFp) )
        {
            VLOG(ERR, "Frame Data osal_fwrite failed file handle is %p \n", yuvFp );
            return 0;
        }
    }
    return 1;
} 

int ReadBsResetBufHelper(Uint32 core_idx,
    osal_file_t streamFp,
    PhysicalAddress bitstream,
    int size,
    int endian)
{
    unsigned char *buf = osal_malloc(size);

    if (!buf)
    {
        VLOG(ERR, "fail to allocate bitstream buffer\n" );
        return 0;
    }

    vdi_read_memory(core_idx, bitstream, buf, size, endian);
    osal_fwrite((void *)buf, sizeof(Uint8), size, streamFp);
    osal_fflush(streamFp);
    osal_free(buf);
    return 1;
}

#ifdef SUPPORT_FFMPEG_DEMUX
int fourCCToMp4Class(unsigned int fourcc)
{
    int i;
    int mp4Class = -1;
    char str[5];

    str[0] = toupper((char)fourcc);
    str[1] = toupper((char)(fourcc>>8));
    str[2] = toupper((char)(fourcc>>16));
    str[3] = toupper((char)(fourcc>>24));
    str[4] = '\0';

    for(i=0; i<sizeof(codstd_tab)/sizeof(codstd_tab[0]); i++)
    {
        if (codstd_tab[i].fourcc == (unsigned int)MKTAG(str[0], str[1], str[2], str[3]) )
        {
            mp4Class = codstd_tab[i].mp4Class;
            break;
        }
    }

    return mp4Class;
}

int fourCCToCodStd(unsigned int fourcc)
{
    int codStd = -1;
    int i;

    char str[5];

    str[0] = toupper((char)fourcc);
    str[1] = toupper((char)(fourcc>>8));
    str[2] = toupper((char)(fourcc>>16));
    str[3] = toupper((char)(fourcc>>24));
    str[4] = '\0';

    for(i=0; i<sizeof(codstd_tab)/sizeof(codstd_tab[0]); i++)
    {
        if (codstd_tab[i].fourcc == (unsigned int)MKTAG(str[0], str[1], str[2], str[3]))
        {
            codStd = codstd_tab[i].codStd;
            break;
        }
    }

    return codStd;
}

int codecIdToMp4Class(int codec_id)
{
    int mp4Class = -1;
    int i;

    for(i=0; i<sizeof(codstd_tab)/sizeof(codstd_tab[0]); i++)
    {
        if (codstd_tab[i].codec_id == codec_id)
        {
            mp4Class = codstd_tab[i].mp4Class;
            break;
        }
    }

    return mp4Class;

}
int codecIdToCodStd(int codec_id)
{
    int codStd = -1;
    int i;

    for(i=0; i<sizeof(codstd_tab)/sizeof(codstd_tab[0]); i++)
    {
        if (codstd_tab[i].codec_id == codec_id)
        {
            codStd = codstd_tab[i].codStd;
            break;
        }
    }
    return codStd;
}

int codecIdToFourcc(int codec_id)
{
    int fourcc = 0;
    int i;

    for(i=0; i<sizeof(codstd_tab)/sizeof(codstd_tab[0]); i++)
    {
        if (codstd_tab[i].codec_id == codec_id)
        {
            fourcc = codstd_tab[i].fourcc;
            break;
        }
    }
    return fourcc;
}
#endif


// Local definition
#define	RCV_V2
#ifdef RCV_V2
#define VC1_SEQ_LAYER_METADATA_BYTESIZE 36 // Annex L.2 SequenceLayer(9*4byte)
#define VC1_FRM_LAYER_METADATA_BYTESIZE 8  // Annex L.3 FrameLayer(2*4byte)
#else // RCV_V2
#define VC1_SEQ_LAYER_METADATA_BYTESIZE 20
#define VC1_FRM_LAYER_METADATA_BYTESIZE 4
#endif // RCV_V2

#define PUT_BYTE(_p, _b) \
    *_p++ = (unsigned char)_b; 

#define PUT_BUFFER(_p, _buf, _len) \
    memcpy(_p, _buf, _len); \
    _p += _len;

#define PUT_LE32(_p, _var) \
    *_p++ = (unsigned char)((_var)>>0);  \
    *_p++ = (unsigned char)((_var)>>8);  \
    *_p++ = (unsigned char)((_var)>>16); \
    *_p++ = (unsigned char)((_var)>>24); 

#define PUT_BE32(_p, _var) \
    *_p++ = (unsigned char)((_var)>>24);  \
    *_p++ = (unsigned char)((_var)>>16);  \
    *_p++ = (unsigned char)((_var)>>8); \
    *_p++ = (unsigned char)((_var)>>0); 


#define PUT_LE16(_p, _var) \
    *_p++ = (unsigned char)((_var)>>0);  \
    *_p++ = (unsigned char)((_var)>>8);  


#define PUT_BE16(_p, _var) \
    *_p++ = (unsigned char)((_var)>>8);  \
    *_p++ = (unsigned char)((_var)>>0);  


#ifdef SUPPORT_FFMPEG_DEMUX
int BuildSeqHeader(BYTE *pbHeader, const CodStd codStd, const AVStream *st)
{
    AVCodecContext *avc = st->codec;
    BYTE *pbMetaData = avc->extradata;
    int nMetaData = avc->extradata_size;
    BYTE* p = pbMetaData;
    BYTE *a = p + 4 - ((long) p & 3);
    BYTE* t = pbHeader;	
    int size; 
    int fourcc;
    int sps, pps, i, nal;
    int frameRate = 0;

    fourcc = avc->codec_tag;
    if (!fourcc)
        fourcc = codecIdToFourcc(avc->codec_id);


    if (st->avg_frame_rate.den && st->avg_frame_rate.num)
        frameRate = (int)((double)st->avg_frame_rate.num/(double)st->avg_frame_rate.den);

    if (!frameRate && st->r_frame_rate.den && st->r_frame_rate.num)
        frameRate = (int)((double)st->r_frame_rate.num/(double)st->r_frame_rate.den);

    size = 0;
    if (codStd == STD_AVC || codStd == STD_AVS)
    {
        if (nMetaData > 1 && pbMetaData && pbMetaData[0] == 0x01)// check mov/mo4 file format stream
        {
            p += 5;
            sps = (*p & 0x1f); // Number of sps
            p++;
            for (i = 0; i < sps; i++)
            {
                nal = (*p << 8) + *(p + 1) + 2;
                PUT_BYTE(t, 0x00);
                PUT_BYTE(t, 0x00);
                PUT_BYTE(t, 0x00);
                PUT_BYTE(t, 0x01);
                PUT_BUFFER(t, p+2, nal-2);
                p += nal;
                size += (nal - 2 + 4); // 4 => length of start code to be inserted
            }

            pps = *(p++); // number of pps
            for (i = 0; i < pps; i++)
            {
                nal = (*p << 8) + *(p + 1) + 2;
                PUT_BYTE(t, 0x00);
                PUT_BYTE(t, 0x00);
                PUT_BYTE(t, 0x00);
                PUT_BYTE(t, 0x01);
                PUT_BUFFER(t, p+2, nal-2);
                p += nal;
                size += (nal - 2 + 4); // 4 => length of start code to be inserted
            }
        }
        else if(nMetaData > 3)
        {
            size = -1;// return to meaning of invalid stream data;		

            for (; p < a; p++)
            {
                if (p[0] == 0 && p[1] == 0 && p[2] == 1) // find startcode
                {
                    size = avc->extradata_size;
                    PUT_BUFFER(pbHeader, pbMetaData, size);
                    break;
                }
            }	
        }
    }
    else if (codStd == STD_VC1)
    {
        if (!fourcc)
            return -1;
        if (fourcc == MKTAG('W', 'V', 'C', '1') || fourcc == MKTAG('W', 'M', 'V', 'A'))	//VC AP
        {
            size = nMetaData;
            PUT_BUFFER(pbHeader, pbMetaData, size);
            //if there is no seq startcode in pbMetatData. VPU will be failed at seq_init stage.
        }
        else
        {	
#ifdef RCV_V2
            PUT_LE32(pbHeader, ((0xC5 << 24)|0));
            size += 4; //version
            PUT_LE32(pbHeader, nMetaData);
            size += 4;
            PUT_BUFFER(pbHeader, pbMetaData, nMetaData);
            size += nMetaData;
            PUT_LE32(pbHeader, avc->height);
            size += 4;
            PUT_LE32(pbHeader, avc->width);
            size += 4;
            PUT_LE32(pbHeader, 12);
            size += 4;
            PUT_LE32(pbHeader, 2 << 29 | 1 << 28 | 0x80 << 24 | 1 << 0);
            size += 4; // STRUCT_B_FRIST (LEVEL:3|CBR:1:RESERVE:4:HRD_BUFFER|24)
            PUT_LE32(pbHeader, avc->bit_rate);
            size += 4; // hrd_rate
            PUT_LE32(pbHeader, frameRate);            
            size += 4; // frameRate
#else	//RCV_V1
            PUT_LE32(pbHeader, (0x85 << 24) | 0x00);
            size += 4; //frames count will be here
            PUT_LE32(pbHeader, nMetaData);
            size += 4;
            PUT_BUFFER(pbHeader, pbMetaData, nMetaData);
            size += nMetaData;
            PUT_LE32(pbHeader, avc->height);
            size += 4;
            PUT_LE32(pbHeader, avc->width);
            size += 4;
#endif
        }
    }
    else if (codStd == STD_RV)
    {
        int st_size =0;

        if (!fourcc)
            return -1;
        if (fourcc != MKTAG('R','V','3','0') && fourcc != MKTAG('R','V','4','0'))
            return -1;

        size = 26 + nMetaData;
        PUT_BE32(pbHeader, size); //Length
        PUT_LE32(pbHeader, MKTAG('V', 'I', 'D', 'O')); //MOFTag
        PUT_LE32(pbHeader, fourcc); //SubMOFTagl
        PUT_BE16(pbHeader, avc->width);
        PUT_BE16(pbHeader, avc->height);
        PUT_BE16(pbHeader, 0x0c); //BitCount;
        PUT_BE16(pbHeader, 0x00); //PadWidth;
        PUT_BE16(pbHeader, 0x00); //PadHeight;

        //PUT_LE32(pbHeader, frameRate);
		PUT_BE32(pbHeader, frameRate<<16);
        PUT_BUFFER(pbHeader, pbMetaData, nMetaData); //OpaqueDatata
        size += st_size; //add for startcode pattern.
    }
    else if (codStd == STD_VP8)
    {
        PUT_LE32(pbHeader, MKTAG('D', 'K', 'I', 'F')); //signature 'DKIF'
        PUT_LE16(pbHeader, 0x00);                      //version
        PUT_LE16(pbHeader, 0x20);                      //length of header in bytes
        PUT_LE32(pbHeader, MKTAG('V', 'P', '8', '0')); //codec FourCC
        PUT_LE16(pbHeader, avc->width);                //width
        PUT_LE16(pbHeader, avc->height);               //height
        PUT_LE32(pbHeader, st->r_frame_rate.num);      //frame rate
        PUT_LE32(pbHeader, st->r_frame_rate.den);      //time scale(?)
        PUT_LE32(pbHeader, st->nb_index_entries);      //number of frames in file
        PUT_LE32(pbHeader, 0); //unused
        size += 32;
    }
    else
    {
        PUT_BUFFER(pbHeader, pbMetaData, nMetaData);
        size = nMetaData;
    }


    return size;
}


int BuildPicHeader(BYTE *pbHeader, const CodStd codStd, const AVStream *st, const AVPacket *pkt)
{
    AVCodecContext *avc = st->codec;
    BYTE *pbChunk = pkt->data;
    int size = 0;
    int fourcc;
    int cSlice, nSlice;
    int i, val, offset;
    int has_st_code = 0;

    size = 0;
    offset = 0;
    fourcc = avc->codec_tag;
    if (!fourcc)
        fourcc = codecIdToFourcc(avc->codec_id);


    if (codStd == STD_VC1)
    {
        if (!fourcc)
            return -1;

        if (fourcc == MKTAG('W', 'V', 'C', '1') || fourcc == MKTAG('W', 'M', 'V', 'A'))	//VC AP
        {
            if (pbChunk[0] != 0 || pbChunk[1] != 0 || pbChunk[2] != 1) // check start code as prefix (0x00, 0x00, 0x01)
            {
                pbHeader[0] = 0x00;
                pbHeader[1] = 0x00;
                pbHeader[2] = 0x01;
                pbHeader[3] = 0x0D;	// replace to the correct picture header to indicate as frame				

				size += 4;
            }			
        }
        else
        {

            PUT_LE32(pbHeader, pkt->size | ((pkt->flags & AV_PKT_FLAG_KEY) ? 0x80000000 : 0));
            size += 4;
#ifdef RCV_V2
            if (AV_NOPTS_VALUE == pkt->pts)
            {
                PUT_LE32(pbHeader, 0);
            }
            else
            {
                PUT_LE32(pbHeader, (int)((double)(pkt->pts/st->time_base.den))); // milli_sec
            }
            size += 4;
#endif
        }
    }
    else if (codStd == STD_RV)
    {
        int st_size = 0;

        if (!fourcc)
            return -1;
        if (fourcc != MKTAG('R','V','3','0') && fourcc != MKTAG('R','V','4','0')) // RV version 8, 9 , 10
            return -1;


        cSlice = pbChunk[0] + 1;
        nSlice =  pkt->size - 1 - (cSlice * 8);
        size = 20 + (cSlice*8);

        PUT_BE32(pbHeader, nSlice);
        if (AV_NOPTS_VALUE == pkt->pts)
        {
            PUT_LE32(pbHeader, 0);
        }
        else
        {
            PUT_LE32(pbHeader, (int)((double)(pkt->pts/st->time_base.den))); // milli_sec
        }
        PUT_BE16(pbHeader, avc->frame_number);
        PUT_BE16(pbHeader, 0x02); //Flags
        PUT_BE32(pbHeader, 0x00); //LastPacket
        PUT_BE32(pbHeader, cSlice); //NumSegments
        offset = 1;
        for (i = 0; i < (int) cSlice; i++)
        {
            val = (pbChunk[offset+3] << 24) | (pbChunk[offset+2] << 16) | (pbChunk[offset+1] << 8) | pbChunk[offset];
            PUT_BE32(pbHeader, val); //isValid
            offset += 4;
            val = (pbChunk[offset+3] << 24) | (pbChunk[offset+2] << 16) | (pbChunk[offset+1] << 8) | pbChunk[offset];
            PUT_BE32(pbHeader, val); //Offset
            offset += 4;
        }

        size += st_size;
#if 0
        PUT_BUFFER(pbChunk, pkt->data+(1+(cSlice*8)), nSlice);		
        size += nSlice;
#endif		
    }
    else if (codStd == STD_AVC)
    {
        if(pkt->size < 5)
            return 0;

        if (!(avc->extradata_size > 1 && avc->extradata && avc->extradata[0] == 0x01))
        {
            const Uint8 *pbEnd = pbChunk + 4 - ((intptr_t)pbChunk & 3);

            for (; pbChunk < pbEnd ; pbChunk++) 
            {
                if (pbChunk[0] == 0 && pbChunk[1] == 0 && pbChunk[2] == 1)
                {
                    has_st_code = 1;
                    break;
                }
            }
        }

        if ((!has_st_code && avc->extradata[0] == 0x01) || (avc->extradata_size > 1 && avc->extradata && avc->extradata[0] == 0x01)) // check sequence metadata if the stream is mov/mo4 file format.
        {
            pbChunk = pkt->data;

            while (offset < pkt->size)
            {
                nSlice = pbChunk[offset] << 24 | pbChunk[offset+1] << 16 | pbChunk[offset+2] << 8 | pbChunk[offset+3];

                pbChunk[offset] = 0x00;
                pbChunk[offset+1] = 0x00;
                pbChunk[offset+2] = 0x00;
                pbChunk[offset+3] = 0x01;		//replace size to startcode

                offset += 4;

                switch (pbChunk[offset]&0x1f) /* NAL unit */
                {
                case 6: /* SEI */
                case 7: /* SPS */
                case 8: /* PPS */
                case 9: /* AU */
                    /* check next */
                    break;
                }

                offset += nSlice;
            }			
        }
    }
    else if(codStd == STD_AVS)
    {
        const Uint8 *pbEnd;

							
        if(pkt->size < 5)
            return 0;

        pbEnd = pbChunk + 4 - ((intptr_t)pbChunk & 3);

        for (; pbChunk < pbEnd ; pbChunk++) 
        {
            if (pbChunk[0] == 0 && pbChunk[1] == 0 && pbChunk[2] == 1)
            {
                has_st_code = 1;
                break;
            }
        }

        if(has_st_code == 0)
        {
            pbChunk = pkt->data;

            while (offset < pkt->size)
            {
                nSlice = pbChunk[offset] << 24 | pbChunk[offset+1] << 16 | pbChunk[offset+2] << 8 | pbChunk[offset+3];

                pbChunk[offset] = 0x00;
                pbChunk[offset+1] = 0x00;
                pbChunk[offset+2] = 0x00;
                pbChunk[offset+3] = 0x00;		//replace size to startcode
                pbChunk[offset+4] = 0x01;

                offset += 4;

                switch (pbChunk[offset]&0x1f) /* NAL unit */
                {
                case 6: /* SEI */
                case 7: /* SPS */
                case 8: /* PPS */
                case 9: /* AU */
                    /* check next */
                    break;
                }

                offset += nSlice;
            }			
        }
    }
    else if (codStd == STD_VP8)
    {
        PUT_LE32(pbHeader,pkt->size);
        PUT_LE32(pbHeader,0);
        PUT_LE32(pbHeader,0);
        size += 12;

    }

    return size;
}



#endif



/******************************************************************************
Runtime Parameter Update
******************************************************************************/
void changeRcParaTest(Uint32 core_idx, EncHandle handle, 
    osal_file_t bsFp, 
    EncParam *pEncParam, 
    EncHeaderParam *pEncHeaderParam,
    EncConfigParam *pEncConfig,
    EncOpenParam	*pEncOP)
{
    int optionNum;

    while(1)
    {
        printf("\n	0: GOP number change\n");
        printf("	1: Intra Qp change\n");
        printf("	2: Bit Rate change\n");
        printf("	3: Frame Rate change\n");
        printf("	4: Intra Refresh Number change\n");
        printf("	5: Slice Mode change\n");
        if(pEncOP->bitstreamFormat == STD_MPEG4)
        {
            printf("	6: HEC Mode change\n");
        }

        printf("	9: go encoding\n");
        scanf("%d", &optionNum);

        switch(optionNum){
        case 0:
            {
                int newGopNum=0;
                printf("\n	New Gop Number=");
                scanf("%d", &newGopNum);
                VPU_EncGiveCommand(handle, ENC_SET_GOP_NUMBER, &newGopNum);
            }
            break;
        case 1:
            {
                int newIntraQp=0;
                printf("\n	New Intra Qp value=");
                scanf("%d", &newIntraQp);
                VPU_EncGiveCommand(handle, ENC_SET_INTRA_QP, &newIntraQp);
            }
            break;
        case 2:
            {
                int newBitrate=0;
                printf("\n	New Bit Rate=");
                scanf("%d", &newBitrate);
                VPU_EncGiveCommand(handle, ENC_SET_BITRATE, &newBitrate);
            }
            break;
        case 3:
            {
                int newFramerate=0;
                printf("\n	New Frame Rate=");
                scanf("%d", &newFramerate);
                VPU_EncGiveCommand(handle, ENC_SET_FRAME_RATE, &newFramerate);
                if( pEncOP->bitstreamFormat == STD_MPEG4 ) 
                {
                    pEncHeaderParam->headerType = VOL_HEADER;
                    VPU_EncGiveCommand(handle, ENC_PUT_VIDEO_HEADER, pEncHeaderParam); 
#ifdef SUPPORT_FFMPEG_DEMUX
                    if( pEncOP->ringBufferEnable == 0 && !pEncConfig->en_container ) 
#else
                    if( pEncOP->ringBufferEnable == 0 ) 
#endif
                    {
                        if (!ReadBsResetBufHelper(core_idx, bsFp, pEncHeaderParam->buf, pEncHeaderParam->size, pEncOP->streamEndian))
                            break;
                    }
                }
            }
            break;
        case 4:
            {
                int newIntraRefreshNum=0;
                printf("\n	New Intra Refresh Number=");
                scanf("%d", &newIntraRefreshNum);
                VPU_EncGiveCommand(handle, ENC_SET_INTRA_MB_REFRESH_NUMBER, &newIntraRefreshNum);
            }
            break;
        case 5:
            {
                EncSliceMode newSlice;
                printf("\n	New Slice Mode[0:one slice, 1:muliple slice]=");
                scanf("%d", &newSlice.sliceMode);
                if(!newSlice.sliceMode)
                {
                    newSlice.sliceSizeMode	= 0;
                    newSlice.sliceSize		= 0;
                }
                else
                {
                    printf("\n	New Slice Size Mode[0:bit number, 1:mb number]=");
                    scanf("%d", &newSlice.sliceSizeMode);
                        if(!newSlice.sliceSizeMode)
                        {
                            printf("\n	New Slice bit number=");
                        }
                        else
                        {
                            printf("\n	New Slice MB number=");
                        }
                        scanf("%d", &newSlice.sliceSize);
                }

                VPU_EncGiveCommand(handle, ENC_SET_SLICE_INFO, &newSlice);
            }
            break;
        case 6:
            {
                int newHecMode=0;
                printf("\n	New Hec Mode Enable[0:disable, 1:enable]=");
                scanf("%d", &newHecMode);
                if(newHecMode > 0)
                {
                    VPU_EncGiveCommand(handle, ENC_ENABLE_HEC, &newHecMode);
                }
                else
                {
                    VPU_EncGiveCommand(handle, ENC_DISABLE_HEC, &newHecMode);
                }

            }
            break;
        default:
            break;
        }
        if(optionNum == 9)
            break;
    }

}


/******************************************************************************
EncOpenParam Initialization
******************************************************************************/
/**
* To init EncOpenParam by runtime evaluation
* IN
*   EncConfigParam *pEncConfig
* OUT
*   EncOpenParam *pEncOP
*/
int getEncOpenParamDefault(EncOpenParam *pEncOP, EncConfigParam *pEncConfig)
{
    int bitFormat;

    bitFormat = pEncOP->bitstreamFormat;

    pEncOP->picWidth = pEncConfig->picWidth;
    pEncOP->picHeight = pEncConfig->picHeight;
    pEncOP->frameRateInfo = 30;
    pEncOP->bitRate = pEncConfig->kbps;
    pEncOP->initialDelay = 0;
    pEncOP->vbvBufferSize = 0;			// 0 = ignore
    pEncOP->frameSkipDisable = 1;			// for compare with C-model ( C-model = only 1 )
    pEncOP->gopSize = 30;					// only first picture is I
    pEncOP->sliceMode.sliceMode = 1;		// 1 slice per picture
    pEncOP->sliceMode.sliceSizeMode = 1;
    pEncOP->sliceMode.sliceSize = 115;
    pEncOP->intraRefresh = 0;
    pEncOP->rcIntraQp = -1;				// disable == -1
    pEncOP->userQpMax		= 0;
    pEncOP->userGamma		= (Uint32)(0.75*32768);		//  (0*32768 < gamma < 1*32768)
    pEncOP->rcIntervalMode= 1;						// 0:normal, 1:frame_level, 2:slice_level, 3: user defined Mb_level
    pEncOP->mbInterval	= 0;
    pEncConfig->picQpY = 23;

    pEncOP->intraCostWeight = 400;

    // Standard specific
    if( bitFormat == STD_MPEG4 ) {
        pEncOP->EncStdParam.mp4Param.mp4DataPartitionEnable = 0;
        pEncOP->EncStdParam.mp4Param.mp4ReversibleVlcEnable = 0;
        pEncOP->EncStdParam.mp4Param.mp4IntraDcVlcThr = 0;
        pEncOP->EncStdParam.mp4Param.mp4HecEnable	= 0;
        pEncOP->EncStdParam.mp4Param.mp4Verid = 2;		
    }
    else if( bitFormat == STD_H263 ) {
        pEncOP->EncStdParam.h263Param.h263AnnexIEnable = 0;
        pEncOP->EncStdParam.h263Param.h263AnnexJEnable = 0;
        pEncOP->EncStdParam.h263Param.h263AnnexKEnable = 0;
        pEncOP->EncStdParam.h263Param.h263AnnexTEnable = 0;		
    }
    else if( bitFormat == STD_AVC ) {
        pEncOP->EncStdParam.avcParam.constrainedIntraPredFlag = 0;
        pEncOP->EncStdParam.avcParam.disableDeblk = 1;
        pEncOP->EncStdParam.avcParam.deblkFilterOffsetAlpha = 6;
        pEncOP->EncStdParam.avcParam.deblkFilterOffsetBeta = 0;
        pEncOP->EncStdParam.avcParam.chromaQpOffset = 10;
        pEncOP->EncStdParam.avcParam.audEnable = 0;
        pEncOP->EncStdParam.avcParam.frameCroppingFlag = 0;
        pEncOP->EncStdParam.avcParam.frameCropLeft = 0;
        pEncOP->EncStdParam.avcParam.frameCropRight = 0;
        pEncOP->EncStdParam.avcParam.frameCropTop = 0;
        pEncOP->EncStdParam.avcParam.frameCropBottom = 0;
        pEncOP->EncStdParam.avcParam.level = 0;

        // Update cropping information : Usage example for H.264 frame_cropping_flag
/*
        if (pEncOP->picHeight == 1080) 
        {
            // In case of AVC encoder, when we want to use unaligned display width(For example, 1080),
            // frameCroppingFlag parameters should be adjusted to displayable rectangle
            if (pEncConfig->rotAngle != 90 && pEncConfig->rotAngle != 270) // except rotation
            {        
                if (pEncOP->EncStdParam.avcParam.frameCroppingFlag == 0) 
                {
                    pEncOP->EncStdParam.avcParam.frameCroppingFlag = 1;
                    // frameCropBottomOffset = picHeight(MB-aligned) - displayable rectangle height
                    pEncOP->EncStdParam.avcParam.frameCropBottom = 8;
                }
            }
        }*/
    }
    else {
        VLOG(ERR, "Invalid codec standard mode \n" );
        return 0;
    }


    return 1;
}


/**
* To init EncOpenParam by CFG file
* IN
*   EncConfigParam *pEncConfig
* OUT
*   EncOpenParam *pEncOP
*   char *srcYuvFileName
*/
int getEncOpenParam(EncOpenParam *pEncOP, EncConfigParam *pEncConfig, ENC_CFG *pEncCfg)
{
    int bitFormat;
    ENC_CFG encCfgInst;
    ENC_CFG *pCfg;
    char yuvDir[256] = "//52.2.12.200/AllFiles/qctool/img/";
#ifdef ANDROID
	strcpy(yuvDir, "");
#endif

    // Source YUV Image File to load
    if (pEncCfg)
    {
        pCfg = pEncCfg;
    }
    else
    {
        osal_memset( &encCfgInst, 0x00, sizeof(ENC_CFG));
        pCfg = &encCfgInst;
    }
    bitFormat = pEncOP->bitstreamFormat;
    switch(bitFormat) {
    case STD_AVC:
        if (parseAvcCfgFile(pCfg, pEncConfig->cfgFileName) == 0)
            return 0;
        pEncConfig->picQpY = pCfg->PicQpY;	
        if (pEncCfg)
            strcpy(pEncConfig->yuvFileName, pCfg->SrcFileName);			
        else
            sprintf(pEncConfig->yuvFileName,  "%s%s", yuvDir, pCfg->SrcFileName);		
        break;
    case STD_MPEG4:
    case STD_H263:
        if (parseMp4CfgFile(pCfg, pEncConfig->cfgFileName) == 0)
            return 0;
        pEncConfig->picQpY = pCfg->VopQuant;
        if (pEncCfg)
            strcpy(pEncConfig->yuvFileName, pCfg->SrcFileName);			
        else
            sprintf(pEncConfig->yuvFileName,  "%s%s", yuvDir, pCfg->SrcFileName);
        if (pCfg->ShortVideoHeader == 1) 
            bitFormat = STD_H263;
        break;
    default :
        break;
    }




    pEncConfig->outNum = pCfg->NumFrame;
#ifdef TEST_SKIP_PIC
    memcpy(pEncConfig->skip_pic_nums, pCfg->skip_pic_nums, sizeof(pCfg->skip_pic_nums));
#endif
    pEncOP->picWidth = pCfg->PicX;
    pEncOP->picHeight = pCfg->PicY;
    pEncOP->frameRateInfo = pCfg->FrameRate;
    pEncOP->bitRate = pCfg->RcBitRate;	
    pEncOP->initialDelay = pCfg->RcInitDelay;
    pEncOP->vbvBufferSize = pCfg->RcBufSize;
    pEncOP->frameSkipDisable = pCfg->frameSkipDisable;
    pEncOP->gopSize = pCfg->GopPicNum;
    pEncOP->sliceMode.sliceMode = pCfg->SliceMode;
    pEncOP->sliceMode.sliceSizeMode = pCfg->SliceSizeMode;
    pEncOP->sliceMode.sliceSize = pCfg->SliceSizeNum;
    pEncOP->intraRefresh = pCfg->IntraRefreshNum;
	pEncOP->Intra16x16ModeOnly = pCfg->Intra16x16ModeOnly;
    pEncOP->rcIntraQp = pCfg->RCIntraQP;	
    pEncOP->intraCostWeight = pCfg->intraCostWeight;


    pEncOP->rcEnable = pCfg->RcEnable;
    if (!pCfg->RcEnable)
        pEncOP->bitRate = 0;

    if (!pCfg->GammaSetEnable)
        pEncOP->userGamma = -1;
    else
        pEncOP->userGamma = pCfg->Gamma;
    /* It was agreed that the statements below would be used. but Cmodel at r25518 is not changed yet according to the statements below   
    if (bitFormat == STD_MPEG4)
    pEncOP->MEUseZeroPmv = 1;			
    else
    pEncOP->MEUseZeroPmv = 0;		
    */
    // MP4 263 Only
    if (!pCfg->ConstantIntraQPEnable)
        pEncOP->rcIntraQp = -1;

    if (pCfg->MaxQpSetEnable)
        pEncOP->userQpMax       = pCfg->MaxQp;	
    else
        pEncOP->userQpMax       = -1;
    pEncOP->rcIntervalMode = pCfg->rcIntervalMode;		// 0:normal, 1:frame_level, 2:slice_level, 3: user defined Mb_level
    pEncOP->mbInterval = pCfg->RcMBInterval;			// FIXME

    // Standard specific
    if( bitFormat == STD_MPEG4 ) {
        pEncOP->EncStdParam.mp4Param.mp4DataPartitionEnable = pCfg->DataPartEn;
        pEncOP->EncStdParam.mp4Param.mp4ReversibleVlcEnable = pCfg->RevVlcEn;
        pEncOP->EncStdParam.mp4Param.mp4IntraDcVlcThr = pCfg->IntraDcVlcThr;
        pEncOP->EncStdParam.mp4Param.mp4HecEnable	= pCfg->HecEnable;
		pEncOP->EncStdParam.mp4Param.mp4Verid = 2;
    }
    else if( bitFormat == STD_H263 ) {
        pEncOP->EncStdParam.h263Param.h263AnnexIEnable = pCfg->AnnexI;
        pEncOP->EncStdParam.h263Param.h263AnnexJEnable = pCfg->AnnexJ;
        pEncOP->EncStdParam.h263Param.h263AnnexKEnable = pCfg->AnnexK;
        pEncOP->EncStdParam.h263Param.h263AnnexTEnable = pCfg->AnnexT;		
    }
    else if( bitFormat == STD_AVC ) {
        pEncOP->EncStdParam.avcParam.constrainedIntraPredFlag = pCfg->ConstIntraPredFlag;
        pEncOP->EncStdParam.avcParam.disableDeblk = pCfg->DisableDeblk;
        pEncOP->EncStdParam.avcParam.deblkFilterOffsetAlpha = pCfg->DeblkOffsetA;
        pEncOP->EncStdParam.avcParam.deblkFilterOffsetBeta = pCfg->DeblkOffsetB;
        pEncOP->EncStdParam.avcParam.chromaQpOffset = pCfg->ChromaQpOffset;
        pEncOP->EncStdParam.avcParam.audEnable = pCfg->aud_en;
        pEncOP->EncStdParam.avcParam.frameCroppingFlag = 0;
        pEncOP->EncStdParam.avcParam.frameCropLeft = 0;
        pEncOP->EncStdParam.avcParam.frameCropRight = 0;
        pEncOP->EncStdParam.avcParam.frameCropTop = 0;
        pEncOP->EncStdParam.avcParam.frameCropBottom = 0;
        pEncOP->EncStdParam.avcParam.level = pCfg->level;


        // Update cropping information : Usage example for H.264 frame_cropping_flag
/*
        if (pEncOP->picHeight == 1080) 
        {
            // In case of AVC encoder, when we want to use unaligned display width(For example, 1080),
            // frameCroppingFlag parameters should be adjusted to displayable rectangle
            if (pEncConfig->rotAngle != 90 && pEncConfig->rotAngle != 270) // except rotation
            {        
                if (pEncOP->EncStdParam.avcParam.frameCroppingFlag == 0) 
                {
                    pEncOP->EncStdParam.avcParam.frameCroppingFlag = 1;
                    // frameCropBottomOffset = picHeight(MB-aligned) - displayable rectangle height
                    pEncOP->EncStdParam.avcParam.frameCropBottom = 8;
                }
            }
        }*/
    }
    else {
        VLOG(ERR, "Invalid codec standard mode \n" );
        return 0;
    }

    return 1;
}


//------------------------------------------------------------------------------
// ENCODE PARAMETER PARSE FUNCSIONS
//------------------------------------------------------------------------------
// Parameter parsing helper
static int GetValue(osal_file_t fp, char *para, char *value)
{
    static int LineNum = 1;
    char lineStr[256];
    char paraStr[256];
    osal_fseek(fp, 0, SEEK_SET);

    while (1) {
        if (fgets(lineStr, 256, fp) == NULL)
            return 0;
        sscanf(lineStr, "%s %s", paraStr, value);
        if (paraStr[0] != ';') {
            if (strcmp(para, paraStr) == 0)
                return 1;
        }
        LineNum++;
    }
    return 1;
} 

// Parse "string number number ..." at most "num" numbers
// e.g. SKIP_PIC_NUM 1 3 4 5
static int GetValues(osal_file_t fp, char *para, int *values, int num)
{
    char line[1024];

    osal_fseek(fp, 0, SEEK_SET);

    while (1)
    {
        int  i;
        char *str;

        if (fgets(line, sizeof(line)-1, fp) == NULL)
            return 0;

        // empty line
        if ((str = strtok(line, " ")) == NULL)
            continue;

        if(strcmp(str, para) != 0)
            continue;

        for (i=0; i<num; i++)
        {
            if ((str = strtok(NULL, " ")) == NULL)
                return 1;
            if (!isdigit(str[0]))
                return 1;
            values[i] = atoi(str);
        }
        return 1;
    }
}



int parseMp4CfgFile(ENC_CFG *pEncCfg, char *FileName)
{
    osal_file_t fp;
    char sValue[1024];
    

    fp = osal_fopen(FileName, "rt");
    if (fp == NULL) {
        return 0;
    }

    if (GetValue(fp, "YUV_SRC_IMG", sValue) == 0)
        goto __end_parseMp4CfgFile;
    else
        strcpy(pEncCfg->SrcFileName, sValue);

    if (GetValue(fp, "FRAME_NUMBER_ENCODED", sValue) == 0)
        goto __end_parseMp4CfgFile;
    pEncCfg->NumFrame = atoi(sValue);
    if (GetValue(fp, "PICTURE_WIDTH", sValue) == 0)
        goto __end_parseMp4CfgFile;
    pEncCfg->PicX = atoi(sValue);
    if (GetValue(fp, "PICTURE_HEIGHT", sValue) == 0)
        goto __end_parseMp4CfgFile;
    pEncCfg->PicY = atoi(sValue);
    if (GetValue(fp, "FRAME_RATE", sValue) == 0)
        goto __end_parseMp4CfgFile;
    {
        double frameRate;
        int  timeRes, timeInc;
#ifdef ANDROID
        frameRate = atoi(sValue);
#else
        frameRate = atof(sValue);
#endif
        timeInc = 1;
        while ((int)frameRate != frameRate) {
            timeInc *= 10;
            frameRate *= 10;
        }
        timeRes = (int) frameRate;
        // divide 2 or 5
        if (timeInc%2 == 0 && timeRes%2 == 0)
            timeInc /= 2, timeRes /= 2;
        if (timeInc%5 == 0 && timeRes%5 == 0)
            timeInc /= 5, timeRes /= 5;

        if (timeRes == 2997 && timeInc == 100)
            timeRes = 30000, timeInc = 1001;
        pEncCfg->FrameRate = (timeInc - 1) << 16;
        pEncCfg->FrameRate |= timeRes;
    }
    if (GetValue(fp, "VERSION_ID", sValue) == 0)
        goto __end_parseMp4CfgFile;
    pEncCfg->VerId = atoi(sValue);
    if (GetValue(fp, "DATA_PART_ENABLE", sValue) == 0)
        goto __end_parseMp4CfgFile;
    pEncCfg->DataPartEn = atoi(sValue);
    if (GetValue(fp, "REV_VLC_ENABLE", sValue) == 0)
        goto __end_parseMp4CfgFile;
    pEncCfg->RevVlcEn = atoi(sValue);

    if (GetValue(fp, "INTRA_DC_VLC_THRES", sValue) == 0)
        goto __end_parseMp4CfgFile;
    pEncCfg->IntraDcVlcThr = atoi(sValue);
    if (GetValue(fp, "SHORT_VIDEO", sValue) == 0)
        goto __end_parseMp4CfgFile;
    pEncCfg->ShortVideoHeader = atoi(sValue);
    if (GetValue(fp, "ANNEX_I_ENABLE", sValue) == 0)
        goto __end_parseMp4CfgFile;
    pEncCfg->AnnexI = atoi(sValue);
    if (GetValue(fp, "ANNEX_J_ENABLE", sValue) == 0)
        goto __end_parseMp4CfgFile;
    pEncCfg->AnnexJ = atoi(sValue);
    if (GetValue(fp, "ANNEX_K_ENABLE", sValue) == 0)
        goto __end_parseMp4CfgFile;
    pEncCfg->AnnexK = atoi(sValue);
    if (GetValue(fp, "ANNEX_T_ENABLE", sValue) == 0)
        goto __end_parseMp4CfgFile;
    pEncCfg->AnnexT = atoi(sValue);

    if (GetValue(fp, "VOP_QUANT_SCALE", sValue) == 0)
        goto __end_parseMp4CfgFile;
    pEncCfg->VopQuant = atoi(sValue);
    if (GetValue(fp, "GOP_PIC_NUMBER", sValue) == 0)
        goto __end_parseMp4CfgFile;
    pEncCfg->GopPicNum = atoi(sValue);
    if (GetValue(fp, "SLICE_MODE", sValue) == 0)
        goto __end_parseMp4CfgFile;
    pEncCfg->SliceMode = atoi(sValue);
    if (GetValue(fp, "SLICE_SIZE_MODE", sValue) == 0)
        goto __end_parseMp4CfgFile;
    pEncCfg->SliceSizeMode = atoi(sValue);
    if (GetValue(fp, "SLICE_SIZE_NUMBER", sValue) == 0)
        goto __end_parseMp4CfgFile;
    pEncCfg->SliceSizeNum = atoi(sValue);	
	/** this is workaround for coda7l*/
	if ( pEncCfg->SliceSizeNum < 6 ) pEncCfg->SliceSizeNum = 6;
    if (GetValue(fp, "RATE_CONTROL_ENABLE", sValue) == 0)
        goto __end_parseMp4CfgFile;
    pEncCfg->RcEnable = atoi(sValue);
    if (GetValue(fp, "BIT_RATE_KBPS", sValue) == 0)
        goto __end_parseMp4CfgFile;
    pEncCfg->RcBitRate = atoi(sValue);
    if (GetValue(fp, "DELAY_IN_MS", sValue) == 0)
        goto __end_parseMp4CfgFile;
    pEncCfg->RcInitDelay = atoi(sValue);
    if (GetValue(fp, "VBV_BUFFER_SIZE", sValue) == 0)
        goto __end_parseMp4CfgFile;
    pEncCfg->RcBufSize = atoi(sValue);
    if (GetValue(fp, "INTRA_MB_REFRESH", sValue) == 0)
        goto __end_parseMp4CfgFile;

    pEncCfg->IntraRefreshNum = atoi(sValue);

	/** diable RC */
	goto __end_parseMp4CfgFile;

    pEncCfg->ConscIntraRefreshEnable = 0;
    if (pEncCfg->IntraRefreshNum > 0)
    {
        if (GetValue(fp, "CONSC_INTRA_REFRESH_EN", sValue) == 0)
            pEncCfg->ConscIntraRefreshEnable = 0;
        else
            pEncCfg->ConscIntraRefreshEnable = atoi(sValue);
    }
    if (GetValue(fp, "CONST_INTRA_QP_EN", sValue) == 0)
        pEncCfg->ConstantIntraQPEnable = 0;
    else
        pEncCfg->ConstantIntraQPEnable = atoi(sValue);
    if (GetValue(fp, "CONST_INTRA_QP", sValue) == 0)
        pEncCfg->RCIntraQP = 0;
    else
        pEncCfg->RCIntraQP = atoi(sValue);

    if (GetValue(fp, "HEC_ENABLE", sValue) == 0)
        pEncCfg->HecEnable = 0;
    else
        pEncCfg->HecEnable = atoi(sValue);

    if (GetValue(fp, "SEARCH_RANGE", sValue) == 0)
        pEncCfg->SearchRange = 0;
    else
        pEncCfg->SearchRange = atoi(sValue);
    if (GetValue(fp, "WEIGHT_INTRA_COST", sValue) == 0)
        pEncCfg->intraCostWeight = 0;
    else
        pEncCfg->intraCostWeight = atoi(sValue);

    if (GetValue(fp, "MAX_QP_SET_ENABLE", sValue) == 0)
        pEncCfg->MaxQpSetEnable= 0;
    else
        pEncCfg->MaxQpSetEnable = atoi(sValue);
    if (GetValue(fp, "MAX_QP", sValue) == 0)
        pEncCfg->MaxQp = 0;
    else
        pEncCfg->MaxQp = atoi(sValue);
    if (GetValue(fp, "GAMMA_SET_ENABLE", sValue) == 0)
        pEncCfg->GammaSetEnable = 0;
    else
        pEncCfg->GammaSetEnable = atoi(sValue);
    if (GetValue(fp, "GAMMA", sValue) == 0)
        pEncCfg->Gamma = 0;
    else
        pEncCfg->Gamma = atoi(sValue);

    if (GetValue(fp, "RC_INTERVAL_MODE", sValue) == 0)
        pEncCfg->rcIntervalMode = 0;
    else
        pEncCfg->rcIntervalMode = atoi(sValue);

    if (GetValue(fp, "RC_MB_INTERVAL", sValue) == 0)
        pEncCfg->RcMBInterval = 0;
    else
        pEncCfg->RcMBInterval = atoi(sValue);

    
__end_parseMp4CfgFile:
    osal_fclose(fp);
    return 1;
}

int parseAvcCfgFile(ENC_CFG *pEncCfg, char *FileName)
{
    osal_file_t fp;
    char sValue[1024];
    int  ret = 0;

    fp = osal_fopen(FileName, "rt");
    if (fp == NULL) {
        return 0;
    }

    if (GetValue(fp, "YUV_SRC_IMG", sValue) == 0)
        goto __end_parseAvcCfgFile;
    else
        strcpy(pEncCfg->SrcFileName, sValue);

    if (GetValue(fp, "FRAME_NUMBER_ENCODED", sValue) == 0)
        goto __end_parseAvcCfgFile;
    pEncCfg->NumFrame = atoi(sValue);
    if (GetValue(fp, "PICTURE_WIDTH", sValue) == 0)
        goto __end_parseAvcCfgFile;
    pEncCfg->PicX = atoi(sValue);
    if (GetValue(fp, "PICTURE_HEIGHT", sValue) == 0)
        goto __end_parseAvcCfgFile;
    pEncCfg->PicY = atoi(sValue);
    if (GetValue(fp, "FRAME_RATE", sValue) == 0)
        goto __end_parseAvcCfgFile;
    {
        double frameRate;
        int  timeRes, timeInc;

#ifdef ANDROID
        frameRate = atoi(sValue);
#else
        frameRate = atof(sValue);
#endif

        timeInc = 1;
        while ((int)frameRate != frameRate) {
            timeInc *= 10;
            frameRate *= 10;
        }
        timeRes = (int) frameRate;
        // divide 2 or 5
        if (timeInc%2 == 0 && timeRes%2 == 0)
            timeInc /= 2, timeRes /= 2;
        if (timeInc%5 == 0 && timeRes%5 == 0)
            timeInc /= 5, timeRes /= 5;

        if (timeRes == 2997 && timeInc == 100)
            timeRes = 30000, timeInc = 1001;
        pEncCfg->FrameRate = (timeInc - 1) << 16;
        pEncCfg->FrameRate |= timeRes;
    }
    if (GetValue(fp, "CONSTRAINED_INTRA", sValue) == 0)
        goto __end_parseAvcCfgFile;
    pEncCfg->ConstIntraPredFlag = atoi(sValue);
    if (GetValue(fp, "DISABLE_DEBLK", sValue) == 0)
        goto __end_parseAvcCfgFile;
    pEncCfg->DisableDeblk = atoi(sValue);
    if (GetValue(fp, "DEBLK_ALPHA", sValue) == 0)
        goto __end_parseAvcCfgFile;
    pEncCfg->DeblkOffsetA = atoi(sValue);
    if (GetValue(fp, "DEBLK_BETA", sValue) == 0)
        goto __end_parseAvcCfgFile;
    pEncCfg->DeblkOffsetB = atoi(sValue);
    if (GetValue(fp, "CHROMA_QP_OFFSET", sValue) == 0)
        goto __end_parseAvcCfgFile;
    pEncCfg->ChromaQpOffset = atoi(sValue);

    if (GetValue(fp, "LEVEL", sValue) == 0)
    {
        pEncCfg->level = 0;//note : 0 means auto calculation.
    }
    else
    {
        pEncCfg->level = atoi(sValue);
        if (pEncCfg->level<0 || pEncCfg->level>51)
            goto __end_parseAvcCfgFile;
    }

    if (GetValue(fp, "PIC_QP_Y", sValue) == 0)
        goto __end_parseAvcCfgFile;
    pEncCfg->PicQpY = atoi(sValue);
    if (GetValue(fp, "GOP_PIC_NUMBER", sValue) == 0)
        goto __end_parseAvcCfgFile;
    pEncCfg->GopPicNum = atoi(sValue);
    if (GetValue(fp, "IDR_INTERVAL", sValue) == 0)
        pEncCfg->IDRInterval = 0;
    else
        pEncCfg->IDRInterval = atoi(sValue);
    if (GetValue(fp, "SLICE_MODE", sValue) == 0)
        goto __end_parseAvcCfgFile;
    pEncCfg->SliceMode = atoi(sValue);
    if (GetValue(fp, "SLICE_SIZE_MODE", sValue) == 0)
        goto __end_parseAvcCfgFile;
    pEncCfg->SliceSizeMode = atoi(sValue);
    if (GetValue(fp, "SLICE_SIZE_NUMBER", sValue) == 0)
        goto __end_parseAvcCfgFile;
    pEncCfg->SliceSizeNum = atoi(sValue);	
    if (GetValue(fp, "AUD_ENABLE", sValue) == 0)
        pEncCfg->aud_en = 0;
    else
        pEncCfg->aud_en = atoi(sValue);	


    /**
    * Error Resilience
    */
    // Intra Cost Weight : not mandatory. default zero
    if (GetValue(fp, "WEIGHT_INTRA_COST", sValue) == 0)
        pEncCfg->intraCostWeight = 0;
    else
        pEncCfg->intraCostWeight = atoi(sValue);

    

    if (GetValue(fp, "RATE_CONTROL_ENABLE", sValue) == 0)
        goto __end_parseAvcCfgFile;
    pEncCfg->RcEnable = atoi(sValue);
    if (GetValue(fp, "BIT_RATE_KBPS", sValue) == 0)
        goto __end_parseAvcCfgFile;
    pEncCfg->RcBitRate = atoi(sValue);
    if (GetValue(fp, "DELAY_IN_MS", sValue) == 0)
        goto __end_parseAvcCfgFile;
    pEncCfg->RcInitDelay = atoi(sValue);
    if (GetValue(fp, "VBV_BUFFER_SIZE", sValue) == 0)
        goto __end_parseAvcCfgFile;
    pEncCfg->RcBufSize = atoi(sValue);
    if (GetValue(fp, "INTRA_MB_REFRESH", sValue) == 0)
        goto __end_parseAvcCfgFile;
    pEncCfg->IntraRefreshNum = atoi(sValue);

    pEncCfg->ConscIntraRefreshEnable = 0;
    if (pEncCfg->IntraRefreshNum > 0)
    {
        if (GetValue(fp, "CONSC_INTRA_REFRESH_EN", sValue) == 0)
            pEncCfg->ConscIntraRefreshEnable = 0;
        else
            pEncCfg->ConscIntraRefreshEnable = atoi(sValue);
    }

    if (GetValue(fp, "FRAME_SKIP_DISABLE", sValue) == 0)
        pEncCfg->frameSkipDisable = 0;
    else
        pEncCfg->frameSkipDisable = atoi(sValue);

    

	if (GetValue(fp, "INTRA16_MODE_ONLY", sValue) == 0)
		pEncCfg->Intra16x16ModeOnly = 0;
	else
		pEncCfg->Intra16x16ModeOnly = atoi(sValue);
    if (GetValue(fp, "MIN_QP_SET_ENABLE", sValue) == 0)
        pEncCfg->MinQpSetEnable= 0;
    else
        pEncCfg->MinQpSetEnable = atoi(sValue);
    if (GetValue(fp, "MIN_QP", sValue) == 0)
        pEncCfg->MinQp = 0;
    else
        pEncCfg->MinQp = atoi(sValue);

    if (GetValue(fp, "MAX_DELTA_QP_SET_ENABLE", sValue) == 0)
        pEncCfg->MaxDeltaQpSetEnable= 0;
    else
        pEncCfg->MaxDeltaQpSetEnable = atoi(sValue);
    if (GetValue(fp, "MAX_DELTA_QP", sValue) == 0)
        pEncCfg->MaxDeltaQp = 0;
    else
        pEncCfg->MaxDeltaQp = atoi(sValue);

    if (GetValue(fp, "MIN_DELTA_QP_SET_ENABLE", sValue) == 0)
        pEncCfg->MinDeltaQpSetEnable= 0;
    else
        pEncCfg->MinDeltaQpSetEnable = atoi(sValue);
    if (GetValue(fp, "MIN_DELTA_QP", sValue) == 0)
        pEncCfg->MinDeltaQp = 0;
    else
        pEncCfg->MinDeltaQp = atoi(sValue);
    if (GetValue(fp, "SEARCH_RANGE", sValue) == 0)
        pEncCfg->SearchRange = 0;
    else
        pEncCfg->SearchRange = atoi(sValue);


    osal_memset(pEncCfg->skip_pic_nums, 0, sizeof(pEncCfg->skip_pic_nums));
    GetValues(fp, "SKIP_PIC_NUMS", pEncCfg->skip_pic_nums, sizeof(pEncCfg->skip_pic_nums));    


    /**
    * VUI Parameter
    */
    if (GetValue(fp, "VUI_PARAMETERS_PRESENT_FLAG", sValue) == 0)
        pEncCfg->VuiPresFlag = 0;
    else 
        pEncCfg->VuiPresFlag = atoi(sValue);

    if (pEncCfg->VuiPresFlag == 1) {

        if (GetValue(fp, "VIDEO_SIGNAL_TYPE_PRESENT_FLAG", sValue) == 0)
            pEncCfg->VideoSignalTypePresFlag = 0;
        else
            pEncCfg->VideoSignalTypePresFlag = atoi(sValue);

        if (pEncCfg->VideoSignalTypePresFlag) {
            if (GetValue(fp, "VIDEO_FORMAT", sValue) == 0)
                pEncCfg->VideoFormat = 5;
            else
                pEncCfg->VideoFormat = (char)atoi(sValue);

            if (GetValue(fp, "VIDEO_FULL_RANGE_FLAG", sValue) == 0)
                pEncCfg->VideoFullRangeFlag = 0;
            else 
				pEncCfg->VideoFullRangeFlag = (char) atoi(sValue);

            if (GetValue(fp, "COLOUR_DESCRIPTION_PRESENT_FLAG", sValue) == 0)
                pEncCfg->ColourDescripPresFlag = 1;
            else
                pEncCfg->ColourDescripPresFlag = atoi(sValue);

            if (pEncCfg->ColourDescripPresFlag) {
                if (GetValue(fp, "COLOUR_PRIMARIES", sValue) == 0)
                    pEncCfg->ColourPrimaries = 1;
                else
					pEncCfg->ColourPrimaries = (char) atoi(sValue);

                if (GetValue(fp, "TRANSFER_CHARACTERISTICS", sValue) == 0)
                    pEncCfg->TransferCharacteristics = 2;
                else
					pEncCfg->TransferCharacteristics = (char) atoi(sValue);

                if (GetValue(fp, "MATRIX_COEFFICIENTS", sValue) == 0)
                    pEncCfg->MatrixCoeff = 2;
                else
					pEncCfg->MatrixCoeff = (char) atoi(sValue);
            }
        }
    }


    ret = 1; /* Success */
__end_parseAvcCfgFile:
    osal_fclose(fp);
    return ret;
}









int IsSupportInterlaceMode(CodStd bitstreamFormat, DecInitialInfo *pSeqInfo)
{
    int bSupport = 0;	
    int profile;


    profile = pSeqInfo->profile;

    switch(bitstreamFormat)
    {
    case STD_AVC:
        profile = (pSeqInfo->profile==66) ? 0 : (pSeqInfo->profile==77) ? 1 : (pSeqInfo->profile==88) ? 2 : (pSeqInfo->profile==100) ? 3 : 4;
        if (profile == 0)	// BP
            bSupport = 0;
        else
            bSupport = 1;
        break;
    case STD_MPEG4:
        if (pSeqInfo->level & 0x80)
        {
            pSeqInfo->level &= 0x7F;
            if (pSeqInfo->level == 8 && pSeqInfo->profile == 0) 
                profile = 0; // Simple
            else 
            {
                switch(pSeqInfo->profile) 
                {
                case 0xB:	profile = 2; break;
                case 0xF:	
                    if ((pSeqInfo->level&8) == 0) 
                        profile = 1; 
                    else
                        profile = 5;
                    break;
                case 0x0:	profile = 0; break;
                default :	profile = 5; break;
                }				
            }
        }
        else // Vol Header Only
        { 
            switch(pSeqInfo->profile) 
            {
            case  0x1: profile = 0; break; // simple object
            case  0xC: profile = 2; break; // advanced coding efficiency object
            case 0x11: profile = 1; break; // advanced simple object
            default  : profile = 5; break; // reserved
            }
        }
        if (profile == 0)	//SP
            bSupport = 0;
        else
            bSupport = 1;
        break;	
    case STD_VC1:
        profile = pSeqInfo->profile;
        if (profile == 0 || profile == 1)	// SP	// MP
            bSupport = 0;
        else
            bSupport = 1;
        break;
    case STD_RV:
        profile = pSeqInfo->profile - 8;
        if (profile == 0)
            bSupport = 0;
        else
            bSupport = 1;
        break;
    case STD_MPEG2:		
    case STD_AVS:
        bSupport = 1;
        break;	
    case STD_H263:
    case STD_DIV3:
        bSupport = 0;
        break;
    case STD_THO:
    case STD_VP3:
    case STD_VP8:
        bSupport = 0;
        break;
    }

    return bSupport;
}

static int VSYNC_SIGNALED;

void init_VSYNC_flag()
{
	VSYNC_SIGNALED = 0;
}
int  check_VSYNC_flag()
{
#ifdef CNM_FPGA_PLATFORM
#ifdef FPGA_LX_330
	int  data;

	data = VpuReadReg(0, MIX_INT);
	return (data & 1);
#else
	return VSYNC_SIGNALED;
#endif
#else
	return VSYNC_SIGNALED;
#endif	
}

void  set_VSYNC_flag()
{
#ifdef CNM_FPGA_PLATFORM
#ifdef FPGA_LX_330
	VpuWriteReg(0, MIX_INT, vale);	
#else
	VSYNC_SIGNALED = 1;
#endif
#else
	VSYNC_SIGNALED = 1;
#endif	
}

void clear_VSYNC_flag()
{
#ifdef CNM_FPGA_PLATFORM
#ifdef FPGA_LX_330
	VpuWriteReg(0, MIX_INT, 0);
#else
	VSYNC_SIGNALED = 0;
#endif
#else
	VSYNC_SIGNALED = 0;
#endif
}

frame_queue_item_t* frame_queue_init(int count)
{
	frame_queue_item_t* queue = NULL; 

	queue = (frame_queue_item_t *)osal_malloc(sizeof(frame_queue_item_t));
	if (!queue)
		return NULL;
	queue->size   = count;
	queue->count  = 0;
	queue->front  = 0;
	queue->rear   = 0;
	queue->buffer = (int*)osal_malloc(count*sizeof(int));
	
	return queue;
}

void frame_queue_deinit(frame_queue_item_t* queue)
{
	if (queue == NULL) 
		return;

	if (queue->buffer)
		osal_free(queue->buffer);
	osal_free(queue);
}

/*
 * Return 0 on success.
 *	   -1 on failure
 */
int frame_queue_enqueue(frame_queue_item_t* queue, int data)
{
	if (queue == NULL) return -1;

	/* Queue is full */
	if (queue->count == queue->size) return -1;

	queue->buffer[queue->rear++] = data;
	queue->rear %= queue->size;
	queue->count++;

	return 0;
}


/*
 * Return 0 on success.
 *	   -1 on failure
 */
int frame_queue_dequeue(frame_queue_item_t* queue, int* data)
{
	if (queue == NULL) return -1;
	/* Queue is empty */
	if (queue->count == 0) return -1;

	*data = queue->buffer[queue->front++];
	queue->front %= queue->size;
	queue->count--;

	return 0;
}

int frame_queue_dequeue_all(frame_queue_item_t* queue)
{
	int ret;
	int data;
	if (queue == NULL) return -1;

	do 
	{
		ret = frame_queue_dequeue(queue, &data);
		if (ret >=0)
			VLOG(INFO, "Empty display Queue for flush display_index=%d\n", data);
	} while (ret >= 0);	
	return 0;
}

int frame_queue_peekqueue(frame_queue_item_t* queue, int* data)
{
	if (queue == NULL) return -1;
	/* Queue is empty */
	if (queue->count == 0) return -1;

	*data = queue->buffer[queue->front];
	return 0;
}


int frame_queue_check_in_queue(frame_queue_item_t* queue, int val)
{
	int data;
	int front;
	int count;
	if (queue == NULL) return -1;

	front = queue->front;
	count = queue->count;
	while(count > 0)
	{
		data = queue->buffer[front++];
		if (data == val)
			return 1;

		count--;
		front %= queue->size;
	}

	return 0;
}

int frame_queue_count(frame_queue_item_t* queue)
{
	if (queue == NULL) return -1;

	return queue->count;
}



/* make container */
#ifdef SUPPORT_FFMPEG_DEMUX

/* add a video output stream */
static AVStream *add_stream(AVFormatContext *oc, enum AVCodecID codec_id, EncConfigParam encConfig, int gopSize)
{
    AVCodecContext *c;
    AVStream *st;

    st = avformat_new_stream(oc, 0);
    if (!st) {
        VLOG(ERR, "Could not alloc stream\n");
        return 0;
    }
    st->id = oc->nb_streams-1;
    c = st->codec;
    c->codec_id = codec_id;
    c->codec_type = AVMEDIA_TYPE_VIDEO;

    /* put sample parameters */
    c->bit_rate = encConfig.kbps * 1000;
    /* resolution must be a multiple of two */
    c->width = (encConfig.rotAngle==90||encConfig.rotAngle ==270)?encConfig.picHeight:encConfig.picWidth;
    c->height = (encConfig.rotAngle==90||encConfig.rotAngle ==270)?encConfig.picWidth:encConfig.picHeight;

    /* time base: this is the fundamental unit of time (in seconds) in terms
    of which frame timestamps are represented. for fixed-fps content,
    timebase should be 1/framerate and timestamp increments should be
    identically 1. */
    c->time_base.den = encConfig.container_frame_rate;
    c->time_base.num = 1;

    c->gop_size = gopSize; /* emit one intra frame every (gop_size)frames at most */

    c->pix_fmt = PIX_FMT_YUV420P;
    if (c->codec_id == CODEC_ID_MPEG2VIDEO) {
        /* just for testing, we also add B frames */
        c->max_b_frames = 2;
    }
    if (c->codec_id == CODEC_ID_MPEG1VIDEO){
        /* Needed to avoid using macroblocks in which some coeffs overflow.
        This does not happen with normal video, it just happens here as
        the motion of the chroma plane does not match the luma plane. */
        c->mb_decision=2;
    }
    // some formats want stream headers to be separate
    if(oc->oformat->flags & AVFMT_GLOBALHEADER)
        c->flags |= CODEC_FLAG_GLOBAL_HEADER;

    return st;
}

static int write_video_frame(AVFormatContext *oc, AVStream *st, unsigned char *addr, int size, int picType)
{
    int ret;
    
    /* if zero size, it means the image was buffered */
    if (size > 0) {
        AVPacket pkt;
        av_init_packet(&pkt);

        if (picType == 0)   //I Frame
            pkt.flags |= AV_PKT_FLAG_KEY;
        pkt.stream_index= st->index;
        pkt.data= addr;
        pkt.size= size;

        /* write the compressed frame in the media file */
        ret = av_interleaved_write_frame(oc, &pkt);
    } 
    else
        ret = 0;

    if (ret != 0) {
        VLOG(ERR, "Error while writing video frame\n");
        return 0;
    }

    return 1;
}

/*
* make a file and write a container's header part
* must set -> en_container, container_frame_rate, gopSize, kbps, rotAngle, picHeight, picWidth,
*             stdMode and bitstreamFileName in struct EncConfigParam
*/
int container_init(EncConfigParam encConfig,
    AVOutputFormat **fmt,
    AVFormatContext **oc,
    AVStream **video_st, 
    int gopSize)
{
    int stdMode = encConfig.stdMode;
    char *filename = encConfig.bitstreamFileName;
    unsigned int i;

    if (encConfig.ringBufferEnable == 1)
    {
        VLOG(ERR, "not supported format\n");
        return 0;
    }

    /* initialize libavcodec, and register all codecs and formats */
    av_register_all();

    /* allocate the output media context */
    avformat_alloc_output_context2(&(*oc), NULL, NULL, filename);
    if (!*oc) {
        printf("Could not deduce output format from file extension: using MPEG.\n");
        avformat_alloc_output_context2(&(*oc), NULL, "mpeg", filename);
    }
    if (!*oc) {
        return 0;
    }
    *fmt = (*oc)->oformat;

    if (stdMode == 0)
        (*fmt)->video_codec = CODEC_ID_MPEG4;
    else if (stdMode == 1)
        (*fmt)->video_codec = CODEC_ID_H263;
    else if (stdMode == 2)
        (*fmt)->video_codec = CODEC_ID_H264;
    else
        VLOG(ERR, "unknown codec type = 0x%x\n", stdMode);

    (*oc)->video_codec_id = (*fmt)->video_codec;
    (*oc)->audio_codec_id = (*fmt)->audio_codec;

    /* add the audio and video streams using the default format codecs
    and initialize the codecs */
    *video_st = add_stream(*oc, (*fmt)->video_codec, encConfig, gopSize);

    av_dump_format(*oc, 0, filename, 1);

    /* open the output file, if needed */
    if (avio_open(&(*oc)->pb, filename, AVIO_FLAG_WRITE) < 0) {
        VLOG(ERR, "Could not open '%s'\n", filename);
        for(i=0; i<(*oc)->nb_streams; i++) {
            av_freep(&(*oc)->streams[i]->codec);
            av_freep(&(*oc)->streams[i]);
        }
        /* osal_free the stream */
        av_free(*oc);
        return 0;
    }

    /* write the stream header, if any */
    if (avformat_write_header(*oc, NULL) < 0) {
        VLOG(ERR, "Could not write header for output file (incorrect codec paramters ?)\n");
        /* osal_free the streams */
        for(i=0; i<(*oc)->nb_streams; i++) {
            av_freep(&(*oc)->streams[i]->codec);
            av_freep(&(*oc)->streams[i]);
        }

        /* close the output file */
        avio_close((*oc)->pb);

        /* osal_free the stream */
        av_free(*oc);
        *oc = NULL;
        return 0;
    }

    return 1;
}



/*
* save the header data to temp_buf(header_buf). (instead of ReadBsResetBufHelper)
*/
int container_copy_header_from_bitstream_buffer(const Uint32 core_idx,
    const PhysicalAddress bitstream,
    const int size,
    const int endian,
    char *buf,
    int *pos)
{
    vdi_read_memory(core_idx, bitstream, (unsigned char *)(buf+(*pos)), size, endian);
    *pos += size;

    //to check memory boundary
    if (*pos >= CONTAINER_HEADER_SIZE)
    {
        VLOG(ERR, "exceed header buffer, header buffer size[%d]\n", *pos);
        return 0;
    }
    return 1;
}

/*
* write the Element stream data with syntax header to the container file
*/
int container_write_es(const Uint32 core_idx,
    const PhysicalAddress bitstream,
    const int size,
    const int endian,
    AVFormatContext *oc, 
    AVStream *st,
    char *header_buffer,
    int *header_pos,
    int format, 
    int picType)
{
    unsigned char *buf = osal_malloc(size);

    if (!buf)
    {
        VLOG(ERR, "fail to allocate bitstream buffer\n");
        return 0;
    }
    vdi_read_memory(core_idx, bitstream, buf, size, endian);

    //video data = write only 1 SPS & PPS data in the file, 
    //JPEG = copy jpeg header to every chunk in the file
    if (*header_pos)//I Frame || MJPEG
    {
        unsigned char *buf2 = osal_malloc(size + *header_pos);

        memcpy(buf2, header_buffer, *header_pos);
        memcpy(buf2 + *header_pos, buf, size);

        write_video_frame(oc, st, buf2,size + *header_pos, picType);

        osal_free(buf2);

        //to write a header to the first chunk just one time in container except JPEG
        if ( format != 8 ) //(format != MJPEG)
            *header_pos = 0;
    }
    else
        write_video_frame(oc, st, buf, size, picType);

    osal_free(buf);

    return 1;
}

int container_deinit(AVFormatContext *oc)
{
    unsigned int i;

    if (!oc || !oc->pb)
        return 0;

    /* write the trailer, if any.  the trailer must be written
    * before you close the CodecContexts open when you wrote the
    * header; otherwise write_trailer may try to use memory that
    * was freed on av_codec_close() */
    av_write_trailer(oc);

    /* osal_free the streams */
    for(i=0; i<oc->nb_streams; i++) {
        av_freep(&oc->streams[i]->codec);
        av_freep(&oc->streams[i]);
    }

    /* close the output file */
    avio_close(oc->pb);

    /* osal_free the stream */
    av_free(oc);

    return 1;
}

#endif /* SUPPORT_FFMPEG_DEMUX */


#ifdef REPORT_FRAMERATE
Uint32 GetFrameRate(Uint32 core_idx, DecInitialInfo * info, CodStd bitstreamFormat)
{

	Uint32 framerate;

	if (info == 0) {
		return RETCODE_INVALID_PARAM;
	}

	framerate = 0;

	if(info->fRateDenominator <=0)
		return 0;
	//if (pVpu->initialInfo.fRateDenominator <= 0)
	//	return 0;

	switch(bitstreamFormat)
	{
	case STD_AVC:
	case STD_AVS:
	case STD_MPEG2:
	case STD_MPEG4:
	case STD_H263:
	case STD_RV:
	case STD_VP8:
		framerate = (Uint32)(info->fRateNumerator/info->fRateDenominator);
		break;
	case STD_VC1:
		if (info->profile == 2) // AP
		{
			const int TBL_FRAME_RATE_NR[8] = {0, 24, 25, 30, 50, 60, 48, 72};

			framerate = (Uint32)(TBL_FRAME_RATE_NR[info->fRateNumerator] * 100)/(info->fRateDenominator + 999);
		}
		else
		{
			framerate = (Uint32)((info->fRateNumerator+1)/info->fRateDenominator);
		}
		break;
	default:
		framerate = 0;
		break;
	}

	return framerate;
}
#endif //#ifdef REPORT_FRAMERATE


