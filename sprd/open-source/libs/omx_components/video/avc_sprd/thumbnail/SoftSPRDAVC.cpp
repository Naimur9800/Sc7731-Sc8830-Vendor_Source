/*
 * Copyright (C) 2011 The Android Open Source Project
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

//#define LOG_NDEBUG 0
#define LOG_TAG "SoftSPRDAVC"
#include <utils/Log.h>

#include "SoftSPRDAVC.h"

#include <media/stagefright/foundation/ADebug.h>
#include <media/stagefright/MediaDefs.h>
#include <media/stagefright/MediaErrors.h>
#include <media/IOMX.h>

#include <dlfcn.h>
#include <media/hardware/HardwareAPI.h>
#include <ui/GraphicBufferMapper.h>
#include "gralloc_priv.h"

#include "avc_dec_api.h"

namespace android {

static const CodecProfileLevel kProfileLevels[] = {
    { OMX_VIDEO_AVCProfileBaseline, OMX_VIDEO_AVCLevel1  },
    { OMX_VIDEO_AVCProfileBaseline, OMX_VIDEO_AVCLevel1b },
    { OMX_VIDEO_AVCProfileBaseline, OMX_VIDEO_AVCLevel11 },
    { OMX_VIDEO_AVCProfileBaseline, OMX_VIDEO_AVCLevel12 },
    { OMX_VIDEO_AVCProfileBaseline, OMX_VIDEO_AVCLevel13 },
    { OMX_VIDEO_AVCProfileBaseline, OMX_VIDEO_AVCLevel2  },
    { OMX_VIDEO_AVCProfileBaseline, OMX_VIDEO_AVCLevel21 },
    { OMX_VIDEO_AVCProfileBaseline, OMX_VIDEO_AVCLevel22 },
    { OMX_VIDEO_AVCProfileBaseline, OMX_VIDEO_AVCLevel3  },
    { OMX_VIDEO_AVCProfileBaseline, OMX_VIDEO_AVCLevel31 },
    { OMX_VIDEO_AVCProfileBaseline, OMX_VIDEO_AVCLevel32 },
    { OMX_VIDEO_AVCProfileBaseline, OMX_VIDEO_AVCLevel4  },
    { OMX_VIDEO_AVCProfileBaseline, OMX_VIDEO_AVCLevel41 },
    { OMX_VIDEO_AVCProfileBaseline, OMX_VIDEO_AVCLevel42 },
    { OMX_VIDEO_AVCProfileBaseline, OMX_VIDEO_AVCLevel5  },
    { OMX_VIDEO_AVCProfileBaseline, OMX_VIDEO_AVCLevel51 },

    { OMX_VIDEO_AVCProfileMain, OMX_VIDEO_AVCLevel1  },
    { OMX_VIDEO_AVCProfileMain, OMX_VIDEO_AVCLevel1b },
    { OMX_VIDEO_AVCProfileMain, OMX_VIDEO_AVCLevel11 },
    { OMX_VIDEO_AVCProfileMain, OMX_VIDEO_AVCLevel12 },
    { OMX_VIDEO_AVCProfileMain, OMX_VIDEO_AVCLevel13 },
    { OMX_VIDEO_AVCProfileMain, OMX_VIDEO_AVCLevel2  },
    { OMX_VIDEO_AVCProfileMain, OMX_VIDEO_AVCLevel21 },
    { OMX_VIDEO_AVCProfileMain, OMX_VIDEO_AVCLevel22 },
    { OMX_VIDEO_AVCProfileMain, OMX_VIDEO_AVCLevel3  },
    { OMX_VIDEO_AVCProfileMain, OMX_VIDEO_AVCLevel31 },
    { OMX_VIDEO_AVCProfileMain, OMX_VIDEO_AVCLevel32 },
    { OMX_VIDEO_AVCProfileMain, OMX_VIDEO_AVCLevel4  },
    { OMX_VIDEO_AVCProfileMain, OMX_VIDEO_AVCLevel41 },
    { OMX_VIDEO_AVCProfileMain, OMX_VIDEO_AVCLevel42 },
    { OMX_VIDEO_AVCProfileMain, OMX_VIDEO_AVCLevel5  },
    { OMX_VIDEO_AVCProfileMain, OMX_VIDEO_AVCLevel51 },

    { OMX_VIDEO_AVCProfileHigh, OMX_VIDEO_AVCLevel1  },
    { OMX_VIDEO_AVCProfileHigh, OMX_VIDEO_AVCLevel1b },
    { OMX_VIDEO_AVCProfileHigh, OMX_VIDEO_AVCLevel11 },
    { OMX_VIDEO_AVCProfileHigh, OMX_VIDEO_AVCLevel12 },
    { OMX_VIDEO_AVCProfileHigh, OMX_VIDEO_AVCLevel13 },
    { OMX_VIDEO_AVCProfileHigh, OMX_VIDEO_AVCLevel2  },
    { OMX_VIDEO_AVCProfileHigh, OMX_VIDEO_AVCLevel21 },
    { OMX_VIDEO_AVCProfileHigh, OMX_VIDEO_AVCLevel22 },
    { OMX_VIDEO_AVCProfileHigh, OMX_VIDEO_AVCLevel3  },
    { OMX_VIDEO_AVCProfileHigh, OMX_VIDEO_AVCLevel31 },
    { OMX_VIDEO_AVCProfileHigh, OMX_VIDEO_AVCLevel32 },
    { OMX_VIDEO_AVCProfileHigh, OMX_VIDEO_AVCLevel4  },
    { OMX_VIDEO_AVCProfileHigh, OMX_VIDEO_AVCLevel41 },
    { OMX_VIDEO_AVCProfileHigh, OMX_VIDEO_AVCLevel42 },
    { OMX_VIDEO_AVCProfileHigh, OMX_VIDEO_AVCLevel5  },
    { OMX_VIDEO_AVCProfileHigh, OMX_VIDEO_AVCLevel51 },
};

template<class T>
static void InitOMXParams(T *params) {
    params->nSize = sizeof(T);
    params->nVersion.s.nVersionMajor = 1;
    params->nVersion.s.nVersionMinor = 0;
    params->nVersion.s.nRevision = 0;
    params->nVersion.s.nStep = 0;
}

typedef struct LevelConversion {
    OMX_U32 omxLevel;
    AVCLevel avcLevel;
} LevelConcersion;

static LevelConversion ConversionTable[] = {
    { OMX_VIDEO_AVCLevel1,  AVC_LEVEL1_B },
    { OMX_VIDEO_AVCLevel1b, AVC_LEVEL1   },
    { OMX_VIDEO_AVCLevel11, AVC_LEVEL1_1 },
    { OMX_VIDEO_AVCLevel12, AVC_LEVEL1_2 },
    { OMX_VIDEO_AVCLevel13, AVC_LEVEL1_3 },
    { OMX_VIDEO_AVCLevel2,  AVC_LEVEL2 },
#if 1
    // encoding speed is very poor if video
    // resolution is higher than CIF
    { OMX_VIDEO_AVCLevel21, AVC_LEVEL2_1 },
    { OMX_VIDEO_AVCLevel22, AVC_LEVEL2_2 },
    { OMX_VIDEO_AVCLevel3,  AVC_LEVEL3   },
    { OMX_VIDEO_AVCLevel31, AVC_LEVEL3_1 },
    { OMX_VIDEO_AVCLevel32, AVC_LEVEL3_2 },
    { OMX_VIDEO_AVCLevel4,  AVC_LEVEL4   },
    { OMX_VIDEO_AVCLevel41, AVC_LEVEL4_1 },
    { OMX_VIDEO_AVCLevel42, AVC_LEVEL4_2 },
    { OMX_VIDEO_AVCLevel5,  AVC_LEVEL5   },
    { OMX_VIDEO_AVCLevel51, AVC_LEVEL5_1 },
#endif
};

SoftSPRDAVC::SoftSPRDAVC(
    const char *name,
    const OMX_CALLBACKTYPE *callbacks,
    OMX_PTR appData,
    OMX_COMPONENTTYPE **component)
    : SprdSimpleOMXComponent(name, callbacks, appData, component),
      mHandle(new tagAVCHandle),
      mInputBufferCount(0),
      mPicId(0),
      mSetFreqCount(0),
      mWidth(320),
      mHeight(240),
      mPictureSize(mWidth * mHeight * 3 / 2),
      mCropLeft(0),
      mCropTop(0),
      mCropWidth(mWidth),
      mCropHeight(mHeight),
      mEOSStatus(INPUT_DATA_AVAILABLE),
      mOutputPortSettingsChange(NONE),
      mLibHandle(NULL),
      mHeadersDecoded(false),
      mSignalledError(false),
      mNeedIVOP(true),
      mStopDecode(false),
      mStreamBuffer(NULL),
      mCodecInterBuffer(NULL),
      mCodecExtraBuffer(NULL),
      mH264DecInit(NULL),
      mH264DecGetInfo(NULL),
      mH264DecDecode(NULL),
      mH264DecRelease(NULL),
      mH264Dec_SetCurRecPic(NULL),
      mH264Dec_GetLastDspFrm(NULL),
      mH264Dec_ReleaseRefBuffers(NULL),
      mH264DecMemInit(NULL),
      mH264DecSetparam(NULL) {
    CHECK_EQ(openDecoder("libomx_avcdec_sw_sprd.so"), true);
    initPorts();

    iUseAndroidNativeBuffer[OMX_DirInput] = OMX_FALSE;
    iUseAndroidNativeBuffer[OMX_DirOutput] = OMX_FALSE;

    CHECK_EQ(initDecoder(), (status_t)OK);
}

SoftSPRDAVC::~SoftSPRDAVC() {
    releaseDecoder();

    delete mHandle;
    mHandle = NULL;

    List<BufferInfo *> &outQueue = getPortQueue(kOutputPortIndex);
    List<BufferInfo *> &inQueue = getPortQueue(kInputPortIndex);
    CHECK(outQueue.empty());
    CHECK(inQueue.empty());
}

void SoftSPRDAVC::initPorts() {
    OMX_PARAM_PORTDEFINITIONTYPE def;
    InitOMXParams(&def);

    def.nPortIndex = kInputPortIndex;
    def.eDir = OMX_DirInput;
    def.nBufferCountMin = kNumInputBuffers;
    def.nBufferCountActual = def.nBufferCountMin;
    def.nBufferSize = 8192;
    def.bEnabled = OMX_TRUE;
    def.bPopulated = OMX_FALSE;
    def.eDomain = OMX_PortDomainVideo;
    def.bBuffersContiguous = OMX_FALSE;
    def.nBufferAlignment = 1;

    def.format.video.cMIMEType = const_cast<char *>(MEDIA_MIMETYPE_VIDEO_AVC);
    def.format.video.pNativeRender = NULL;
    def.format.video.nFrameWidth = mWidth;
    def.format.video.nFrameHeight = mHeight;
    def.format.video.nStride = def.format.video.nFrameWidth;
    def.format.video.nSliceHeight = def.format.video.nFrameHeight;
    def.format.video.nBitrate = 0;
    def.format.video.xFramerate = 0;
    def.format.video.bFlagErrorConcealment = OMX_FALSE;
    def.format.video.eCompressionFormat = OMX_VIDEO_CodingAVC;
    def.format.video.eColorFormat = OMX_COLOR_FormatUnused;
    def.format.video.pNativeWindow = NULL;

    addPort(def);

    def.nPortIndex = kOutputPortIndex;
    def.eDir = OMX_DirOutput;
    def.nBufferCountMin = kNumOutputBuffers;
    def.nBufferCountActual = def.nBufferCountMin;
    def.bEnabled = OMX_TRUE;
    def.bPopulated = OMX_FALSE;
    def.eDomain = OMX_PortDomainVideo;
    def.bBuffersContiguous = OMX_FALSE;
    def.nBufferAlignment = 2;

    def.format.video.cMIMEType = const_cast<char *>(MEDIA_MIMETYPE_VIDEO_RAW);
    def.format.video.pNativeRender = NULL;
    def.format.video.nFrameWidth = mWidth;
    def.format.video.nFrameHeight = mHeight;
    def.format.video.nStride = def.format.video.nFrameWidth;
    def.format.video.nSliceHeight = def.format.video.nFrameHeight;
    def.format.video.nBitrate = 0;
    def.format.video.xFramerate = 0;
    def.format.video.bFlagErrorConcealment = OMX_FALSE;
    def.format.video.eCompressionFormat = OMX_VIDEO_CodingUnused;
    def.format.video.eColorFormat = OMX_COLOR_FormatYUV420Planar;
    def.format.video.pNativeWindow = NULL;

    def.nBufferSize =
        (def.format.video.nFrameWidth * def.format.video.nFrameHeight * 3) / 2;

    addPort(def);
}

status_t SoftSPRDAVC::initDecoder() {
    memset(mHandle, 0, sizeof(tagAVCHandle));

    mHandle->userdata = (void *)this;
    mHandle->VSP_bindCb = BindFrameWrapper;
    mHandle->VSP_unbindCb = UnbindFrameWrapper;
    mHandle->VSP_extMemCb = ExtMemAllocWrapper;

    mStreamBuffer = (uint8_t *)malloc(H264_DECODER_STREAM_BUFFER_SIZE);

    uint32_t size_inter = H264_DECODER_INTERNAL_BUFFER_SIZE;
    mCodecInterBuffer = (uint8_t *)malloc(size_inter);

    MMCodecBuffer codec_buf;
    MMDecVideoFormat video_format;

    codec_buf.common_buffer_ptr = mCodecInterBuffer;
    codec_buf.common_buffer_ptr_phy = 0;
    codec_buf.size = size_inter;
    codec_buf.int_buffer_ptr = NULL;
    codec_buf.int_size = 0;

    video_format.video_std = H264;
    video_format.frame_width = 0;
    video_format.frame_height = 0;
    video_format.p_extra = NULL;
    video_format.p_extra_phy = 0;
    video_format.i_extra = 0;
    video_format.yuv_format = YUV420P_YU12;

    if ((*mH264DecInit)(mHandle, &codec_buf,&video_format) != MMDEC_OK)
    {
        ALOGE("Failed to init AVCDEC");
        return OMX_ErrorUndefined;
    }

    //int32 codec_capabilty;
    if ((*mH264GetCodecCapability)(mHandle, &mCapability) != MMDEC_OK) {
        ALOGE("Failed to mH264GetCodecCapability");
    }

    ALOGI("initDecoder, Capability: profile %d, level %d, max wh=%d %d",
          mCapability.profile, mCapability.level, mCapability.max_width, mCapability.max_height);

    return OMX_ErrorNone;
}

void SoftSPRDAVC::releaseDecoder()
{
    (*mH264DecRelease)(mHandle);

    if (mStreamBuffer != NULL)
    {
        free(mStreamBuffer);
        mStreamBuffer = NULL;
    }

    if (mCodecInterBuffer != NULL)
    {
        free(mCodecInterBuffer);
        mCodecInterBuffer = NULL;
    }

    if (mCodecExtraBuffer != NULL)
    {
        free(mCodecExtraBuffer);
        mCodecExtraBuffer = NULL;
    }

    if(mLibHandle)
    {
        dlclose(mLibHandle);
        mLibHandle = NULL;
    }
}

OMX_ERRORTYPE SoftSPRDAVC::internalGetParameter(
    OMX_INDEXTYPE index, OMX_PTR params) {
    switch (index) {
    case OMX_IndexParamVideoPortFormat:
    {
        OMX_VIDEO_PARAM_PORTFORMATTYPE *formatParams =
            (OMX_VIDEO_PARAM_PORTFORMATTYPE *)params;

        if (formatParams->nPortIndex > kOutputPortIndex) {
            return OMX_ErrorUndefined;
        }

        if (formatParams->nIndex != 0) {
            return OMX_ErrorNoMore;
        }

        if (formatParams->nPortIndex == kInputPortIndex) {
            formatParams->eCompressionFormat = OMX_VIDEO_CodingAVC;
            formatParams->eColorFormat = OMX_COLOR_FormatUnused;
            formatParams->xFramerate = 0;
        } else {
            CHECK(formatParams->nPortIndex == kOutputPortIndex);

            PortInfo *pOutPort = editPortInfo(kOutputPortIndex);
            ALOGI("internalGetParameter, OMX_IndexParamVideoPortFormat, eColorFormat: 0x%x",pOutPort->mDef.format.video.eColorFormat);
            formatParams->eCompressionFormat = OMX_VIDEO_CodingUnused;
            formatParams->eColorFormat = pOutPort->mDef.format.video.eColorFormat;
            formatParams->xFramerate = 0;
        }

        return OMX_ErrorNone;
    }

    case OMX_IndexParamVideoProfileLevelQuerySupported:
    {
        OMX_VIDEO_PARAM_PROFILELEVELTYPE *profileLevel =
            (OMX_VIDEO_PARAM_PROFILELEVELTYPE *) params;

        if (profileLevel->nPortIndex != kInputPortIndex) {
            ALOGE("Invalid port index: %d", profileLevel->nPortIndex);
            return OMX_ErrorUnsupportedIndex;
        }

        size_t index = profileLevel->nProfileIndex;
        size_t nProfileLevels =
            sizeof(kProfileLevels) / sizeof(kProfileLevels[0]);
        if (index >= nProfileLevels) {
            return OMX_ErrorNoMore;
        }

        profileLevel->eProfile = kProfileLevels[index].mProfile;
        profileLevel->eLevel = kProfileLevels[index].mLevel;

        if (profileLevel->eProfile == OMX_VIDEO_AVCProfileHigh) {
            if (mCapability.profile < AVC_HIGH) {
                profileLevel->eProfile = OMX_VIDEO_AVCProfileMain;
            }
        }

        if (profileLevel->eProfile == OMX_VIDEO_AVCProfileMain) {
            if (mCapability.profile < AVC_MAIN) {
                profileLevel->eProfile = OMX_VIDEO_AVCProfileBaseline;
            }
        }

        const size_t size =
            sizeof(ConversionTable) / sizeof(ConversionTable[0]);

        for (index = 0; index < size; index++) {
            if (ConversionTable[index].avcLevel > mCapability.level) {
                index--;
                break;
            }
        }

        if (profileLevel->eLevel > ConversionTable[index].omxLevel) {
            profileLevel->eLevel = ConversionTable[index].omxLevel;
        }

        return OMX_ErrorNone;
    }

    case OMX_IndexParamEnableAndroidBuffers:
    {
        EnableAndroidNativeBuffersParams *peanbp = (EnableAndroidNativeBuffersParams *)params;
        peanbp->enable = iUseAndroidNativeBuffer[OMX_DirOutput];
        ALOGI("internalGetParameter, OMX_IndexParamEnableAndroidBuffers %d",peanbp->enable);
        return OMX_ErrorNone;
    }

    case OMX_IndexParamGetAndroidNativeBuffer:
    {
        GetAndroidNativeBufferUsageParams *pganbp;

        pganbp = (GetAndroidNativeBufferUsageParams *)params;
        pganbp->nUsage = GRALLOC_USAGE_SW_READ_OFTEN |GRALLOC_USAGE_SW_WRITE_OFTEN;
        ALOGI("internalGetParameter, OMX_IndexParamGetAndroidNativeBuffer 0x%x",pganbp->nUsage);
        return OMX_ErrorNone;
    }

    default:
        return SprdSimpleOMXComponent::internalGetParameter(index, params);
    }
}

OMX_ERRORTYPE SoftSPRDAVC::internalSetParameter(
    OMX_INDEXTYPE index, const OMX_PTR params) {
    switch (index) {
    case OMX_IndexParamStandardComponentRole:
    {
        const OMX_PARAM_COMPONENTROLETYPE *roleParams =
            (const OMX_PARAM_COMPONENTROLETYPE *)params;

        if (strncmp((const char *)roleParams->cRole,
                    "video_decoder.avc",
                    OMX_MAX_STRINGNAME_SIZE - 1)) {
            return OMX_ErrorUndefined;
        }

        return OMX_ErrorNone;
    }

    case OMX_IndexParamVideoPortFormat:
    {
        OMX_VIDEO_PARAM_PORTFORMATTYPE *formatParams =
            (OMX_VIDEO_PARAM_PORTFORMATTYPE *)params;

        if (formatParams->nPortIndex > kOutputPortIndex) {
            return OMX_ErrorUndefined;
        }

        if (formatParams->nIndex != 0) {
            return OMX_ErrorNoMore;
        }

        return OMX_ErrorNone;
    }

    case OMX_IndexParamEnableAndroidBuffers:
    {
        EnableAndroidNativeBuffersParams *peanbp = (EnableAndroidNativeBuffersParams *)params;
        PortInfo *pInPort = editPortInfo(kInputPortIndex);
        PortInfo *pOutPort = editPortInfo(kOutputPortIndex);
        if (peanbp->enable == OMX_FALSE) {
            ALOGI("internalSetParameter, disable AndroidNativeBuffer");
            iUseAndroidNativeBuffer[OMX_DirOutput] = OMX_FALSE;

            pOutPort->mDef.format.video.eColorFormat = OMX_COLOR_FormatYUV420Planar;
        } else {
            MMDecVideoFormat video_format;
            ALOGI("internalSetParameter, enable AndroidNativeBuffer");
            iUseAndroidNativeBuffer[OMX_DirOutput] = OMX_TRUE;
            pInPort->mDef.nBufferCountActual = 8;
            pOutPort->mDef.nBufferCountActual = 5;
            pOutPort->mDef.format.video.eColorFormat = OMX_COLOR_FormatYUV420SemiPlanar;
            video_format.yuv_format = YUV420SP_NV12;
            (*mH264DecSetparam)(mHandle, &video_format);
        }
        return OMX_ErrorNone;
    }


    case OMX_IndexParamPortDefinition:
    {
        OMX_PARAM_PORTDEFINITIONTYPE *defParams =
            (OMX_PARAM_PORTDEFINITIONTYPE *)params;

        if (defParams->nPortIndex > 1
                || defParams->nSize
                != sizeof(OMX_PARAM_PORTDEFINITIONTYPE)) {
            return OMX_ErrorUndefined;
        }

        PortInfo *port = editPortInfo(defParams->nPortIndex);

        if (defParams->nBufferSize != port->mDef.nBufferSize) {
            CHECK_GE(defParams->nBufferSize, port->mDef.nBufferSize);
            port->mDef.nBufferSize = defParams->nBufferSize;
        }

        if (defParams->nBufferCountActual
                != port->mDef.nBufferCountActual) {
            CHECK_GE(defParams->nBufferCountActual,
                     port->mDef.nBufferCountMin);

            port->mDef.nBufferCountActual = defParams->nBufferCountActual;
        }

        memcpy(&port->mDef.format.video, &defParams->format.video, sizeof(OMX_VIDEO_PORTDEFINITIONTYPE));
        if(defParams->nPortIndex == kOutputPortIndex) {
            port->mDef.format.video.nStride = port->mDef.format.video.nFrameWidth;
            port->mDef.format.video.nSliceHeight = port->mDef.format.video.nFrameHeight;
            mWidth = port->mDef.format.video.nFrameWidth;
            mHeight = port->mDef.format.video.nFrameHeight;
            mCropWidth = mWidth;
            mCropHeight = mHeight;
            port->mDef.nBufferSize =(((mWidth + 15) & -16)* ((mHeight + 15) & -16) * 3) / 2;
            mPictureSize = port->mDef.nBufferSize;
        }

        if (!((mWidth < 1280 && mHeight < 720) || (mWidth < 720 && mHeight < 1280))) {
            PortInfo *port = editPortInfo(kInputPortIndex);
            if(port->mDef.nBufferSize < 384*1024)
                port->mDef.nBufferSize = 384*1024;
        } else if (!((mWidth < 720 && mHeight < 480) || (mWidth < 480 && mHeight < 720))) {
            PortInfo *port = editPortInfo(kInputPortIndex);
            if(port->mDef.nBufferSize < 256*1024)
                port->mDef.nBufferSize = 256*1024;
        }

        return OMX_ErrorNone;
    }

    default:
        return SprdSimpleOMXComponent::internalSetParameter(index, params);
    }
}

OMX_ERRORTYPE SoftSPRDAVC::internalUseBuffer(
    OMX_BUFFERHEADERTYPE **header,
    OMX_U32 portIndex,
    OMX_PTR appPrivate,
    OMX_U32 size,
    OMX_U8 *ptr,
    BufferPrivateStruct* bufferPrivate) {

    *header = new OMX_BUFFERHEADERTYPE;
    (*header)->nSize = sizeof(OMX_BUFFERHEADERTYPE);
    (*header)->nVersion.s.nVersionMajor = 1;
    (*header)->nVersion.s.nVersionMinor = 0;
    (*header)->nVersion.s.nRevision = 0;
    (*header)->nVersion.s.nStep = 0;
    (*header)->pBuffer = ptr;
    (*header)->nAllocLen = size;
    (*header)->nFilledLen = 0;
    (*header)->nOffset = 0;
    (*header)->pAppPrivate = appPrivate;
    (*header)->pPlatformPrivate = NULL;
    (*header)->pInputPortPrivate = NULL;
    (*header)->pOutputPortPrivate = NULL;
    (*header)->hMarkTargetComponent = NULL;
    (*header)->pMarkData = NULL;
    (*header)->nTickCount = 0;
    (*header)->nTimeStamp = 0;
    (*header)->nFlags = 0;
    (*header)->nOutputPortIndex = portIndex;
    (*header)->nInputPortIndex = portIndex;

    if(portIndex == OMX_DirOutput) {
        (*header)->pOutputPortPrivate = new BufferCtrlStruct;
        CHECK((*header)->pOutputPortPrivate != NULL);
        BufferCtrlStruct* pBufCtrl= (BufferCtrlStruct*)((*header)->pOutputPortPrivate);
        pBufCtrl->iRefCount = 1; //init by1
        pBufCtrl->pMem = NULL;
        pBufCtrl->bufferFd = 0;
        pBufCtrl->phyAddr = 0;
        pBufCtrl->bufferSize = 0;
    }

    PortInfo *port = editPortInfo(portIndex);

    port->mBuffers.push();

    BufferInfo *buffer =
        &port->mBuffers.editItemAt(port->mBuffers.size() - 1);
    ALOGI("internalUseBuffer, header=0x%p, pBuffer=0x%p, size=%d",*header, ptr, size);
    buffer->mHeader = *header;
    buffer->mOwnedByUs = false;

    if (port->mBuffers.size() == port->mDef.nBufferCountActual) {
        port->mDef.bPopulated = OMX_TRUE;
        checkTransitions();
    }

    return OMX_ErrorNone;
}

OMX_ERRORTYPE SoftSPRDAVC::getConfig(
    OMX_INDEXTYPE index, OMX_PTR params) {
    switch (index) {
    case OMX_IndexConfigCommonOutputCrop:
    {
        OMX_CONFIG_RECTTYPE *rectParams = (OMX_CONFIG_RECTTYPE *)params;

        if (rectParams->nPortIndex != 1) {
            return OMX_ErrorUndefined;
        }

        rectParams->nLeft = mCropLeft;
        rectParams->nTop = mCropTop;
        rectParams->nWidth = mCropWidth;
        rectParams->nHeight = mCropHeight;

        return OMX_ErrorNone;
    }

    default:
        return OMX_ErrorUnsupportedIndex;
    }
}

void dump_bs( uint8* pBuffer,int32 aInBufSize) {
    FILE *fp = fopen("/data/video_es.m4v","ab");
    fwrite(pBuffer,1,aInBufSize,fp);
    fclose(fp);
}

void dump_yuv( uint8* pBuffer,int32 aInBufSize) {
    FILE *fp = fopen("/data/video.yuv","ab");
    fwrite(pBuffer,1,aInBufSize,fp);
    fclose(fp);
}

void SoftSPRDAVC::onQueueFilled(OMX_U32 portIndex) {
    if (mSignalledError || mOutputPortSettingsChange != NONE) {
        return;
    }

    if (mEOSStatus == OUTPUT_FRAMES_FLUSHED) {
        return;
    }

    List<BufferInfo *> &inQueue = getPortQueue(kInputPortIndex);
    List<BufferInfo *> &outQueue = getPortQueue(kOutputPortIndex);

    while (!mStopDecode && (mEOSStatus != INPUT_DATA_AVAILABLE || !inQueue.empty())
            && outQueue.size() != 0) {

        if (mEOSStatus == INPUT_EOS_SEEN) {
            drainAllOutputBuffers();
            return;
        }

        BufferInfo *inInfo = *inQueue.begin();
        OMX_BUFFERHEADERTYPE *inHeader = inInfo->mHeader;

        List<BufferInfo *>::iterator itBuffer = outQueue.begin();
        OMX_BUFFERHEADERTYPE *outHeader = NULL;
        BufferCtrlStruct *pBufCtrl = NULL;
        size_t count = 0;
        do {
            if(count >= outQueue.size()) {
                ALOGI("onQueueFilled, get outQueue buffer, return, count=%zd, queue_size=%d",count, outQueue.size());
                return;
            }

            outHeader = (*itBuffer)->mHeader;
            pBufCtrl= (BufferCtrlStruct*)(outHeader->pOutputPortPrivate);
            if(pBufCtrl == NULL) {
                ALOGE("onQueueFilled, pBufCtrl == NULL, fail");
                notify(OMX_EventError, OMX_ErrorUndefined, 0, NULL);
                mSignalledError = true;
                return;
            }

            itBuffer++;
            count++;
        }
        while(pBufCtrl->iRefCount > 0);

        ALOGI("%s, %d, outHeader:0x%p, inHeader: 0x%p, len: %d, nOffset: %d, time: %lld, EOS: %d",
              __FUNCTION__, __LINE__,outHeader,inHeader, inHeader->nFilledLen,inHeader->nOffset, inHeader->nTimeStamp,inHeader->nFlags & OMX_BUFFERFLAG_EOS);


        ++mPicId;
        if (inHeader->nFlags & OMX_BUFFERFLAG_EOS) {
//bug253058 , the last frame size may be not zero, it need to be decoded.
//            inQueue.erase(inQueue.begin());
//           inInfo->mOwnedByUs = false;
//            notifyEmptyBufferDone(inHeader);
            mEOSStatus = INPUT_EOS_SEEN;
//            continue;
        }

        if(inHeader->nFilledLen == 0) {
            inInfo->mOwnedByUs = false;
            inQueue.erase(inQueue.begin());
            inInfo = NULL;
            notifyEmptyBufferDone(inHeader);
            inHeader = NULL;
            continue;
        }

        MMDecInput dec_in;
        MMDecOutput dec_out;

        dec_in.dataLen = inHeader->nFilledLen;
        dec_in.pStream = mStreamBuffer;
        dec_in.pStream_phy = 0;
        dec_in.beLastFrm = 0;
        dec_in.expected_IVOP = mNeedIVOP;
        dec_in.beDisplayed = 1;
        dec_in.err_pkt_num = 0;
        dec_out.frameEffective = 0;

        uint32_t bufferSize = dec_in.dataLen;

        if(iUseAndroidNativeBuffer[OMX_DirOutput]) {
            memcpy(mStreamBuffer, inHeader->pBuffer + inHeader->nOffset, inHeader->nFilledLen);
        } else {
            uint32_t add_startcode_len = 0;

            //       if (!memcmp((uint8 *)(inHeader->pBuffer + inHeader->nOffset), "\x00\x00\x00\x01", 4))
            uint8_t *p = (uint8_t *)(inHeader->pBuffer + inHeader->nOffset);

            if((p[0] != 0x0) || (p[1] != 0x0) || (p[2] != 0x0) || (p[3] != 0x1))
            {
                ALOGI("%s, %d, p[0]: %x, p[1]: %x, p[2]: %x, p[3]: %x", __FUNCTION__, __LINE__, p[0], p[1], p[2], p[3]);

                ((uint8_t *) mStreamBuffer)[0] = 0x0;
                ((uint8_t *) mStreamBuffer)[1] = 0x0;
                ((uint8_t *) mStreamBuffer)[2] = 0x0;
                ((uint8_t *) mStreamBuffer)[3] = 0x1;

                add_startcode_len = 4;
                dec_in.dataLen += add_startcode_len;
            }
            memcpy(mStreamBuffer+add_startcode_len, inHeader->pBuffer + inHeader->nOffset, inHeader->nFilledLen);
        }

        //BufferInfo *outInfo = *outQueue.begin();
        //OMX_BUFFERHEADERTYPE *outHeader = outInfo->mHeader;
        outHeader->nTimeStamp = inHeader->nTimeStamp;
        outHeader->nFlags = inHeader->nFlags;
        //outHeader->nFilledLen = mPictureSize;

        GraphicBufferMapper &mapper = GraphicBufferMapper::get();
        if(iUseAndroidNativeBuffer[OMX_DirOutput]) {
            OMX_PARAM_PORTDEFINITIONTYPE *def = &editPortInfo(kOutputPortIndex)->mDef;
            int width = def->format.video.nStride;
            int height = def->format.video.nSliceHeight;
            Rect bounds(width, height);
            void *vaddr;
            int usage;

            usage = GRALLOC_USAGE_SW_READ_OFTEN|GRALLOC_USAGE_SW_WRITE_OFTEN;

            if(mapper.lock((const native_handle_t*)outHeader->pBuffer, usage, bounds, &vaddr)) {
                ALOGE("onQueueFilled, mapper.lock fail 0x%p",outHeader->pBuffer);
                return ;
            }
            ALOGV("%s, %d, pBuffer: 0x%p, vaddr: 0x%p", __FUNCTION__, __LINE__, outHeader->pBuffer,vaddr);
            uint8_t *yuv = (uint8_t *)((uint8_t *)vaddr + outHeader->nOffset);
            ALOGV("%s, %d, yuv: 0x%p, mPicId: %d, outHeader: 0x%p, outHeader->pBuffer: 0x%p, outHeader->nTimeStamp: %lld",
                  __FUNCTION__, __LINE__, yuv, mPicId,outHeader, outHeader->pBuffer, outHeader->nTimeStamp);
            (*mH264Dec_SetCurRecPic)(mHandle, yuv, NULL, (void *)outHeader, mPicId);
        } else {
            uint8_t *yuv = (uint8_t *)(outHeader->pBuffer + outHeader->nOffset);
            (*mH264Dec_SetCurRecPic)(mHandle, yuv, NULL, (void *)outHeader, mPicId);
        }


#if 0
        dump_bs( dec_in.pStream, dec_in.dataLen);
#endif

        int64_t start_decode = systemTime();
        MMDecRet decRet = (*mH264DecDecode)(mHandle, &dec_in,&dec_out);
        int64_t end_decode = systemTime();
        ALOGI("%s, %d, decRet: %d, %dms, dec_out.frameEffective: %d, needIVOP: %d", __FUNCTION__, __LINE__, decRet, (unsigned int)((end_decode-start_decode) / 1000000L), dec_out.frameEffective, mNeedIVOP);

        if(iUseAndroidNativeBuffer[OMX_DirOutput]) {
            if(mapper.unlock((const native_handle_t*)outHeader->pBuffer)) {
                ALOGE("onQueueFilled, mapper.unlock fail 0x%p",outHeader->pBuffer);
            }
        }

        if( decRet == MMDEC_OK) {
            mNeedIVOP = false;
        } else {
            mNeedIVOP = true;
            if (decRet == MMDEC_MEMORY_ERROR) {
                ALOGE("failed to allocate memory.");
                notify(OMX_EventError, OMX_ErrorInsufficientResources, 0, NULL);
                mSignalledError = true;
                return;
            } else if (decRet == MMDEC_NOT_SUPPORTED) {
                ALOGE("failed to support this format.");
                notify(OMX_EventError, OMX_ErrorFormatNotDetected, 0, NULL);
                mSignalledError = true;
                return;
            } else if (decRet == MMDEC_STREAM_ERROR) {
                ALOGE("failed to decode video frame, stream error");
            } else if (decRet == MMDEC_HW_ERROR) {
                ALOGE("failed to decode video frame, hardware error");
            } else {
                ALOGI("now, we don't take care of the decoder return: %d", decRet);
            }
        }

        H264SwDecInfo decoderInfo;
        MMDecRet ret;
        ret = (*mH264DecGetInfo)(mHandle, &decoderInfo);
        if(ret == MMDEC_OK) {
            if (!((decoderInfo.picWidth<= 1920&& decoderInfo.picHeight<= 1088)
                    || (decoderInfo.picWidth <= 1088 && decoderInfo.picHeight <= 1920))) {
                ALOGE("[%d,%d] is out of range [1920, 1088], failed to support this format.",
                      decoderInfo.picWidth, decoderInfo.picHeight);
                notify(OMX_EventError, OMX_ErrorFormatNotDetected, 0, NULL);
                mSignalledError = true;
                return;
            }

            if (handlePortSettingChangeEvent(&decoderInfo)) {
                return;
            }

            if (decoderInfo.croppingFlag &&
                    handleCropRectEvent(&decoderInfo.cropParams)) {
                return;
            }
        } else {
            inInfo->mOwnedByUs = false;
            inQueue.erase(inQueue.begin());
            inInfo = NULL;
            notifyEmptyBufferDone(inHeader);
            inHeader = NULL;

            continue;
        }

        CHECK_LE(bufferSize, inHeader->nFilledLen);

        ALOGI("%s, %d, bufferSize: %d, inHeader->nFilledLen: %d", __FUNCTION__, __LINE__, bufferSize, inHeader->nFilledLen);
        inHeader->nOffset += bufferSize;
        inHeader->nFilledLen -= bufferSize;

        if (inHeader->nFilledLen == 0) {
            inHeader->nOffset = 0;
            inInfo->mOwnedByUs = false;
            inQueue.erase(inQueue.begin());
            inInfo = NULL;
            notifyEmptyBufferDone(inHeader);
            inHeader = NULL;
        }

        while (!outQueue.empty() &&
                mHeadersDecoded &&
                dec_out.frameEffective) {
            ALOGI("%s, %d, dec_out.pBufferHeader: 0x%p, dec_out.mPicId: %d", __FUNCTION__, __LINE__, dec_out.pBufferHeader, dec_out.mPicId);
            int32_t picId = dec_out.mPicId;//decodedPicture.picId;
            drainOneOutputBuffer(picId, dec_out.pBufferHeader);
            dec_out.frameEffective = false;
            if(!iUseAndroidNativeBuffer[OMX_DirOutput]) {
                mStopDecode = true;
            }
        }
    }
}

bool SoftSPRDAVC::handlePortSettingChangeEvent(const H264SwDecInfo *info) {
    OMX_PARAM_PORTDEFINITIONTYPE *def = &editPortInfo(kOutputPortIndex)->mDef;
    if ((mWidth != info->picWidth) || (mHeight != info->picHeight) ||
            (info->numRefFrames > def->nBufferCountActual-(2+1+info->has_b_frames))) {
        ALOGI("%s, %d, mWidth: %d, mHeight: %d, info->picWidth: %d, info->picHeight: %d",
              __FUNCTION__, __LINE__,mWidth, mHeight, info->picWidth, info->picHeight);
        mWidth  = info->picWidth;
        mHeight = info->picHeight;
        mPictureSize = mWidth * mHeight * 3 / 2;
        mCropWidth = mWidth;
        mCropHeight = mHeight;

        if (info->numRefFrames > def->nBufferCountActual-(2+1+info->has_b_frames)) {
            ALOGI("%s, %d, info->numRefFrames: %d, info->has_b_frames: %d, def->nBufferCountActual: %d", __FUNCTION__, __LINE__, info->numRefFrames, info->has_b_frames, def->nBufferCountActual);
            def->nBufferCountActual = info->numRefFrames + (2+1+info->has_b_frames);
        }

        updatePortDefinitions();
        (*mH264Dec_ReleaseRefBuffers)(mHandle);
        notify(OMX_EventPortSettingsChanged, 1, 0, NULL);
        mOutputPortSettingsChange = AWAITING_DISABLED;
        return true;
    }

    return false;
}

bool SoftSPRDAVC::handleCropRectEvent(const CropParams *crop) {
    if (mCropLeft != crop->cropLeftOffset ||
            mCropTop != crop->cropTopOffset ||
            mCropWidth != crop->cropOutWidth ||
            mCropHeight != crop->cropOutHeight) {
        mCropLeft = crop->cropLeftOffset;
        mCropTop = crop->cropTopOffset;
        mCropWidth = crop->cropOutWidth;
        mCropHeight = crop->cropOutHeight;

        notify(OMX_EventPortSettingsChanged, 1,
               OMX_IndexConfigCommonOutputCrop, NULL);

        return true;
    }
    return false;
}

void SoftSPRDAVC::drainOneOutputBuffer(int32_t picId, void* pBufferHeader) {

    List<BufferInfo *> &outQueue = getPortQueue(kOutputPortIndex);

    List<BufferInfo *>::iterator it = outQueue.begin();
    while ((*it)->mHeader != (OMX_BUFFERHEADERTYPE*)pBufferHeader) {
        ++it;
    }
    CHECK((*it)->mHeader == (OMX_BUFFERHEADERTYPE*)pBufferHeader);

    BufferInfo *outInfo = *it;
    OMX_BUFFERHEADERTYPE *outHeader = outInfo->mHeader;

    outHeader->nFilledLen = mPictureSize;

    ALOGV("%s, %d, outHeader: 0x%p, outHeader->pBuffer: 0x%p, outHeader->nOffset: %d, outHeader->nFlags: %d, outHeader->nTimeStamp: %lld",
          __FUNCTION__, __LINE__, outHeader , outHeader->pBuffer, outHeader->nOffset, outHeader->nFlags, outHeader->nTimeStamp);

#if 0
    dump_yuv(data, mPictureSize);
#endif
    outInfo->mOwnedByUs = false;
    outQueue.erase(it);
    outInfo = NULL;

    BufferCtrlStruct* pOutBufCtrl= (BufferCtrlStruct*)(outHeader->pOutputPortPrivate);
    pOutBufCtrl->iRefCount++;
    notifyFillBufferDone(outHeader);
}

bool SoftSPRDAVC::drainAllOutputBuffers() {
    ALOGI("%s, %d", __FUNCTION__, __LINE__);

    List<BufferInfo *> &outQueue = getPortQueue(kOutputPortIndex);
    BufferInfo *outInfo;
    OMX_BUFFERHEADERTYPE *outHeader;

    int32_t picId;
    void* pBufferHeader;

    while (!outQueue.empty() && mEOSStatus != OUTPUT_FRAMES_FLUSHED) {

        if (mHeadersDecoded &&
                MMDEC_OK == (*mH264Dec_GetLastDspFrm)(mHandle, &pBufferHeader, &picId) ) {
            List<BufferInfo *>::iterator it = outQueue.begin();
            while ((*it)->mHeader != (OMX_BUFFERHEADERTYPE*)pBufferHeader && it != outQueue.end()) {
                ++it;
            }
            CHECK((*it)->mHeader == (OMX_BUFFERHEADERTYPE*)pBufferHeader);
            outInfo = *it;
            outQueue.erase(it);
            outHeader = outInfo->mHeader;
            outHeader->nFilledLen = mPictureSize;
        } else {
            outInfo = *outQueue.begin();
            outQueue.erase(outQueue.begin());
            outHeader = outInfo->mHeader;
            outHeader->nTimeStamp = 0;
            outHeader->nFilledLen = 0;
            outHeader->nFlags = OMX_BUFFERFLAG_EOS;
            mEOSStatus = OUTPUT_FRAMES_FLUSHED;
        }

        outInfo->mOwnedByUs = false;
        BufferCtrlStruct* pOutBufCtrl= (BufferCtrlStruct*)(outHeader->pOutputPortPrivate);
        pOutBufCtrl->iRefCount++;
        notifyFillBufferDone(outHeader);
    }

    return true;
}

void SoftSPRDAVC::onPortFlushCompleted(OMX_U32 portIndex) {
    if (portIndex == kInputPortIndex) {
        mEOSStatus = INPUT_DATA_AVAILABLE;
        mNeedIVOP = true;
    }
}

void SoftSPRDAVC::onPortEnableCompleted(OMX_U32 portIndex, bool enabled) {
    switch (mOutputPortSettingsChange) {
    case NONE:
        break;

    case AWAITING_DISABLED:
    {
        CHECK(!enabled);
        mOutputPortSettingsChange = AWAITING_ENABLED;
        break;
    }

    default:
    {
        CHECK_EQ((int)mOutputPortSettingsChange, (int)AWAITING_ENABLED);
        CHECK(enabled);
        mOutputPortSettingsChange = NONE;
        break;
    }
    }
}

void SoftSPRDAVC::onPortFlushPrepare(OMX_U32 portIndex) {
    if(portIndex == OMX_DirOutput) {
        (*mH264Dec_ReleaseRefBuffers)(mHandle);
    }
}

void SoftSPRDAVC::updatePortDefinitions() {
    OMX_PARAM_PORTDEFINITIONTYPE *def = &editPortInfo(kInputPortIndex)->mDef;
    def->format.video.nFrameWidth = mWidth;
    def->format.video.nFrameHeight = mHeight;
    def->format.video.nStride = def->format.video.nFrameWidth;
    def->format.video.nSliceHeight = def->format.video.nFrameHeight;

    def = &editPortInfo(kOutputPortIndex)->mDef;
    def->format.video.nFrameWidth = mWidth;
    def->format.video.nFrameHeight = mHeight;
    def->format.video.nStride = def->format.video.nFrameWidth;
    def->format.video.nSliceHeight = def->format.video.nFrameHeight;

    def->nBufferSize =
        (def->format.video.nFrameWidth
         * def->format.video.nFrameHeight * 3) / 2;
}

// static
int32_t SoftSPRDAVC::ExtMemAllocWrapper(
    void *aUserData, unsigned int size_extra) {
    return static_cast<SoftSPRDAVC *>(aUserData)->VSP_malloc_cb(size_extra);
}

// static
int32_t SoftSPRDAVC::BindFrameWrapper(void *aUserData, void *pHeader) {
    return static_cast<SoftSPRDAVC *>(aUserData)->VSP_bind_cb(pHeader);
}

// static
int32_t SoftSPRDAVC::UnbindFrameWrapper(void *aUserData, void *pHeader) {
    return static_cast<SoftSPRDAVC *>(aUserData)->VSP_unbind_cb(pHeader);
}

int SoftSPRDAVC::VSP_malloc_cb(unsigned int size_extra) {
    MMCodecBuffer ExtraBuffer[MAX_MEM_TYPE];

    if (mCodecExtraBuffer != NULL)
    {
        free(mCodecExtraBuffer);
        mCodecExtraBuffer = NULL;
    }
    mCodecExtraBuffer = (uint8_t *)malloc(size_extra);

    ALOGI("%s, %d, mPictureSize: %d, size_extra: %d, mCodecExtraBuffer: 0x%p",
          __FUNCTION__, __LINE__, mPictureSize, size_extra, mCodecExtraBuffer);

    if (mCodecExtraBuffer == NULL)
    {
        return -1;
    }

    ExtraBuffer[SW_CACHABLE].common_buffer_ptr = mCodecExtraBuffer;
    ExtraBuffer[SW_CACHABLE].size = size_extra;

    (*mH264DecMemInit)(((SoftSPRDAVC *)this)->mHandle, ExtraBuffer);

    mHeadersDecoded = true;

    return 0;
}

int SoftSPRDAVC::VSP_bind_cb(void *pHeader) {
    BufferCtrlStruct *pBufCtrl = (BufferCtrlStruct *)(((OMX_BUFFERHEADERTYPE *)pHeader)->pOutputPortPrivate);

    ALOGI("VSP_bind_cb, pBuffer: 0x%p, pHeader: 0x%p; iRefCount=%d",
          ((OMX_BUFFERHEADERTYPE *)pHeader)->pBuffer, pHeader,pBufCtrl->iRefCount);

    pBufCtrl->iRefCount++;
    return 0;
}

int SoftSPRDAVC::VSP_unbind_cb(void *pHeader) {
    BufferCtrlStruct *pBufCtrl = (BufferCtrlStruct *)(((OMX_BUFFERHEADERTYPE *)pHeader)->pOutputPortPrivate);

    ALOGI("VSP_unbind_cb, pBuffer: 0x%p, pHeader: 0x%p; iRefCount=%d",
          ((OMX_BUFFERHEADERTYPE *)pHeader)->pBuffer, pHeader,pBufCtrl->iRefCount);

    if (pBufCtrl->iRefCount  > 0) {
        pBufCtrl->iRefCount--;
    }

    return 0;
}

OMX_ERRORTYPE SoftSPRDAVC::getExtensionIndex(
    const char *name, OMX_INDEXTYPE *index) {

    ALOGI("getExtensionIndex, name: %s",name);
    if(strcmp(name, SPRD_INDEX_PARAM_ENABLE_ANB) == 0) {
        ALOGI("getExtensionIndex:%s",SPRD_INDEX_PARAM_ENABLE_ANB);
        *index = (OMX_INDEXTYPE) OMX_IndexParamEnableAndroidBuffers;
        return OMX_ErrorNone;
    } else if (strcmp(name, SPRD_INDEX_PARAM_GET_ANB) == 0) {
        ALOGI("getExtensionIndex:%s",SPRD_INDEX_PARAM_GET_ANB);
        *index = (OMX_INDEXTYPE) OMX_IndexParamGetAndroidNativeBuffer;
        return OMX_ErrorNone;
    }	else if (strcmp(name, SPRD_INDEX_PARAM_USE_ANB) == 0) {
        ALOGI("getExtensionIndex:%s",SPRD_INDEX_PARAM_USE_ANB);
        *index = OMX_IndexParamUseAndroidNativeBuffer2;
        return OMX_ErrorNone;
    }

    return OMX_ErrorNotImplemented;
}

bool SoftSPRDAVC::openDecoder(const char* libName)
{
    if(mLibHandle) {
        dlclose(mLibHandle);
    }

    ALOGI("openDecoder, lib: %s",libName);

    mLibHandle = dlopen(libName, RTLD_NOW);
    if(mLibHandle == NULL) {
        ALOGE("openDecoder, can't open lib: %s",libName);
        return false;
    }

    mH264DecInit = (FT_H264DecInit)dlsym(mLibHandle, "H264DecInit");
    if(mH264DecInit == NULL) {
        ALOGE("Can't find H264DecInit in %s",libName);
        dlclose(mLibHandle);
        mLibHandle = NULL;
        return false;
    }

    mH264DecGetInfo = (FT_H264DecGetInfo)dlsym(mLibHandle, "H264DecGetInfo");
    if(mH264DecGetInfo == NULL) {
        ALOGE("Can't find H264DecGetInfo in %s",libName);
        dlclose(mLibHandle);
        mLibHandle = NULL;
        return false;
    }

    mH264DecDecode = (FT_H264DecDecode)dlsym(mLibHandle, "H264DecDecode");
    if(mH264DecDecode == NULL) {
        ALOGE("Can't find H264DecDecode in %s",libName);
        dlclose(mLibHandle);
        mLibHandle = NULL;
        return false;
    }

    mH264DecRelease = (FT_H264DecRelease)dlsym(mLibHandle, "H264DecRelease");
    if(mH264DecRelease == NULL) {
        ALOGE("Can't find H264DecRelease in %s",libName);
        dlclose(mLibHandle);
        mLibHandle = NULL;
        return false;
    }

    mH264Dec_SetCurRecPic = (FT_H264Dec_SetCurRecPic)dlsym(mLibHandle, "H264Dec_SetCurRecPic");
    if(mH264Dec_SetCurRecPic == NULL) {
        ALOGE("Can't find H264Dec_SetCurRecPic in %s",libName);
        dlclose(mLibHandle);
        mLibHandle = NULL;
        return false;
    }

    mH264Dec_GetLastDspFrm = (FT_H264Dec_GetLastDspFrm)dlsym(mLibHandle, "H264Dec_GetLastDspFrm");
    if(mH264Dec_GetLastDspFrm == NULL) {
        ALOGE("Can't find H264Dec_GetLastDspFrm in %s",libName);
        dlclose(mLibHandle);
        mLibHandle = NULL;
        return false;
    }

    mH264Dec_ReleaseRefBuffers = (FT_H264Dec_ReleaseRefBuffers)dlsym(mLibHandle, "H264Dec_ReleaseRefBuffers");
    if(mH264Dec_ReleaseRefBuffers == NULL) {
        ALOGE("Can't find H264Dec_ReleaseRefBuffers in %s",libName);
        dlclose(mLibHandle);
        mLibHandle = NULL;
        return false;
    }

    mH264DecMemInit = (FT_H264DecMemInit)dlsym(mLibHandle, "H264DecMemInit");
    if(mH264DecMemInit == NULL) {
        ALOGE("Can't find H264DecMemInit in %s",libName);
        dlclose(mLibHandle);
        mLibHandle = NULL;
        return false;
    }

    mH264GetCodecCapability = (FT_H264GetCodecCapability)dlsym(mLibHandle, "H264GetCodecCapability");
    if(mH264GetCodecCapability == NULL) {
        ALOGE("Can't find H264GetCodecCapability in %s",libName);
        dlclose(mLibHandle);
        mLibHandle = NULL;
        return false;
    }

    mH264DecGetNALType = (FT_H264DecGetNALType)dlsym(mLibHandle, "H264DecGetNALType");
    if(mH264DecGetNALType == NULL) {
        ALOGE("Can't find H264DecGetNALType in %s",libName);
        dlclose(mLibHandle);
        mLibHandle = NULL;
        return false;
    }

    mH264DecSetparam = (FT_H264DecSetparam)dlsym(mLibHandle, "H264DecSetParameter");
    if(mH264DecSetparam == NULL) {
        ALOGE("Can't find H264DecSetParameter in %s",libName);
        dlclose(mLibHandle);
        mLibHandle = NULL;
        return false;
    }

    return true;
}

}  // namespace android

android::SprdOMXComponent *createSprdOMXComponent(
    const char *name, const OMX_CALLBACKTYPE *callbacks,
    OMX_PTR appData, OMX_COMPONENTTYPE **component) {
    return new android::SoftSPRDAVC(name, callbacks, appData, component);
}
