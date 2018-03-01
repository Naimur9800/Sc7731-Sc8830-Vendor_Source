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

#ifndef SOFT_SPRD_MPEG4_H_
#define SOFT_SPRD_MPEG4_H_

#include "SprdSimpleOMXComponent.h"
#include "m4v_h263_dec_api.h"

#define MP4DEC_INTERNAL_BUFFER_SIZE 10*1024

struct tagMP4Handle;

namespace android {

struct SoftSPRDMPEG4 : public SprdSimpleOMXComponent {
    SoftSPRDMPEG4(const char *name,
                  const OMX_CALLBACKTYPE *callbacks,
                  OMX_PTR appData,
                  OMX_COMPONENTTYPE **component);

protected:
    virtual ~SoftSPRDMPEG4();

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
        kNumInputBuffers  = 4,
        kNumOutputBuffers = 2,
    };

    enum {
        MODE_MPEG4,
        MODE_H263,
        MODE_FLV,

    } mMode;

    enum EOSStatus {
        INPUT_DATA_AVAILABLE,
        INPUT_EOS_SEEN,
        OUTPUT_FRAMES_FLUSHED,
    };

    enum OutputPortSettingChange{
        NONE,
        AWAITING_DISABLED,
        AWAITING_ENABLED
    };
    

    tagMP4Handle *mHandle;

    size_t mInputBufferCount;

    int32_t mWidth, mHeight;
    int32_t mCropLeft, mCropTop, mCropRight, mCropBottom;

    EOSStatus mEOSStatus;
    OutputPortSettingChange mOutputPortSettingsChange;

    void* mLibHandle;
    bool mHeadersDecoded;
    bool mSignalledError;
    bool mNeedIVOP;
    bool mInitialized;
    bool mFramesConfigured;
    bool mStopDecode;

    uint8_t *mCodecInterBuffer;
    uint8_t *mCodecExtraBuffer;

    FT_MP4DecSetCurRecPic mMP4DecSetCurRecPic;
    FT_MP4DecMemCacheInit mMP4DecMemCacheInit;
    FT_MP4DecInit mMP4DecInit;
    FT_MP4DecVolHeader mMP4DecVolHeader;
    FT_MP4DecMemInit mMP4DecMemInit;
    FT_MP4DecDecode mMP4DecDecode;
    FT_MP4DecRelease mMP4DecRelease;
    FT_Mp4GetVideoDimensions mMp4GetVideoDimensions;
    FT_Mp4GetBufferDimensions mMp4GetBufferDimensions;
    FT_MP4DecReleaseRefBuffers mMP4DecReleaseRefBuffers;
    FT_MP4DecSetReferenceYUV mMP4DecSetReferenceYUV;
    FT_MP4DecGetLastDspFrm mMP4DecGetLastDspFrm;
    
    OMX_BOOL iUseAndroidNativeBuffer[2];

    static int32_t extMemoryAllocWrapper(void *userData, unsigned int extra_mem_size);
    static int32_t BindFrameWrapper(void *aUserData, void *pHeader, int flag);
    static int32_t UnbindFrameWrapper(void *aUserData, void *pHeader, int flag);

    int extMemoryAlloc(unsigned int extra_mem_size) ;
    int VSP_bind_cb(void *pHeader,int flag);
    int VSP_unbind_cb(void *pHeader,int flag);

    void initPorts();
    status_t initDecoder();
    void releaseDecoder();
    void updatePortDefinitions();
    bool drainAllOutputBuffers();
    bool portSettingsChanged();
    bool openDecoder(const char* libName);

    DISALLOW_EVIL_CONSTRUCTORS(SoftSPRDMPEG4);
};

}  // namespace android

#endif  // SOFT_SPRD_MPEG4_H_


