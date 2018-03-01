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

#ifndef SOFT_AVC_SPRD_H_
#define SOFT_AVC_SPRD_H_

#include "SprdSimpleOMXComponent.h"
#include <utils/KeyedVector.h>

#include "avc_dec_api.h"
//#include "basetype.h"

#define H264_DECODER_INTERNAL_BUFFER_SIZE (2000*1024)
#define H264_DECODER_STREAM_BUFFER_SIZE (3*1024*1024)

namespace android {

struct SoftSPRDAVC : public SprdSimpleOMXComponent {
    SoftSPRDAVC(const char *name,
                const OMX_CALLBACKTYPE *callbacks,
                OMX_PTR appData,
                OMX_COMPONENTTYPE **component);

protected:
    virtual ~SoftSPRDAVC();

    virtual OMX_ERRORTYPE internalGetParameter(
        OMX_INDEXTYPE index, OMX_PTR params);

    virtual OMX_ERRORTYPE internalSetParameter(
        OMX_INDEXTYPE index, const OMX_PTR params);

    virtual OMX_ERRORTYPE internalUseBuffer(
        OMX_BUFFERHEADERTYPE **buffer,
        OMX_U32 portIndex,
        OMX_PTR appPrivate,
        OMX_U32 size,
        OMX_U8 *ptr,
        BufferPrivateStruct* bufferPrivate=NULL);

    virtual OMX_ERRORTYPE getConfig(OMX_INDEXTYPE index, OMX_PTR params);

    virtual void onQueueFilled(OMX_U32 portIndex);
    virtual void onPortFlushCompleted(OMX_U32 portIndex);
    virtual void onPortEnableCompleted(OMX_U32 portIndex, bool enabled);
    virtual void onPortFlushPrepare(OMX_U32 portIndex);
    virtual OMX_ERRORTYPE getExtensionIndex(const char *name, OMX_INDEXTYPE *index);

private:
    enum {
        kInputPortIndex   = 0,
        kOutputPortIndex  = 1,
        kNumInputBuffers  = 2,
        kNumOutputBuffers = 2,
    };

    enum EOSStatus {
        INPUT_DATA_AVAILABLE,
        INPUT_EOS_SEEN,
        OUTPUT_FRAMES_FLUSHED,
    };

    enum OutputPortSettingChange {
        NONE,
        AWAITING_DISABLED,
        AWAITING_ENABLED
    };

    tagAVCHandle *mHandle;

    size_t mInputBufferCount;
    int32_t mPicId;
    int mSetFreqCount;

    uint32_t mWidth, mHeight, mPictureSize;
    uint32_t mCropLeft, mCropTop;
    uint32_t mCropWidth, mCropHeight;

    EOSStatus mEOSStatus;
    OutputPortSettingChange mOutputPortSettingsChange;

    void* mLibHandle;
    bool mHeadersDecoded;
    bool mSignalledError;
    bool mNeedIVOP;
    bool mStopDecode;

    uint8_t *mStreamBuffer;
    uint8_t *mCodecInterBuffer;
    uint8_t *mCodecExtraBuffer;


    FT_H264DecInit mH264DecInit;    
    FT_H264DecGetInfo mH264DecGetInfo;
    FT_H264DecDecode mH264DecDecode;
    FT_H264DecRelease mH264DecRelease;
    FT_H264Dec_SetCurRecPic  mH264Dec_SetCurRecPic;
    FT_H264Dec_GetLastDspFrm  mH264Dec_GetLastDspFrm;
    FT_H264Dec_ReleaseRefBuffers  mH264Dec_ReleaseRefBuffers;
    FT_H264DecMemInit mH264DecMemInit;
    FT_H264GetCodecCapability mH264GetCodecCapability;
    FT_H264DecGetNALType mH264DecGetNALType;
    FT_H264DecSetparam mH264DecSetparam;

    OMX_BOOL iUseAndroidNativeBuffer[2];
    MMDecCapability mCapability;

    void initPorts();
    status_t initDecoder();
    bool openDecoder(const char* libName);
    void releaseDecoder();
    void updatePortDefinitions();
    bool drainAllOutputBuffers();
    void drainOneOutputBuffer(int32_t picId, void* pBufferHeader);
    bool handleCropRectEvent(const CropParams* crop);
    bool handlePortSettingChangeEvent(const H264SwDecInfo *info);

    static int32_t ExtMemAllocWrapper(void *aUserData, unsigned int size_extra) ;
    static int32_t BindFrameWrapper(void *aUserData, void *pHeader);
    static int32_t UnbindFrameWrapper(void *aUserData, void *pHeader);

    int VSP_malloc_cb(unsigned int size_extra);
    int VSP_bind_cb(void *pHeader);
    int VSP_unbind_cb(void *pHeader);

    DISALLOW_EVIL_CONSTRUCTORS(SoftSPRDAVC);
};

}  // namespace android

#endif  // SOFT_AVC_H_

