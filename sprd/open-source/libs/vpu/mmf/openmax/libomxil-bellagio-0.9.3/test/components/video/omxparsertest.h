/**
  test/components/parser/omxparsertest.h
  Test application uses following OpenMAX components,
    parser3gp  -- 3gp file parser ,
    audio decoder pipeline  - audio decoder, volume component and alsa sink,
    video decoder pipeline  - video decoder, color converter and video sink.
  The application receives a 3gp stream as the input, parses it and sends the audio and
  video streams to their respective pipelines.
  The audio formats supported are:
    mp3 (ffmpeg)
    aac
    The video formats supported are:
    H.264
    MPEG4

  Copyright (C) 2007-2009 STMicroelectronics
  Copyright (C) 2007-2009 Nokia Corporation and/or its subsidiary(-ies).

  This library is free software; you can redistribute it and/or modify it under
  the terms of the GNU Lesser General Public License as published by the Free
  Software Foundation; either version 2.1 of the License, or (at your option)
  any later version.

  This library is distributed in the hope that it will be useful, but WITHOUT
  ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
  FOR A PARTICULAR PURPOSE.  See the GNU Lesser General Public License for more
  details.

  You should have received a copy of the GNU Lesser General Public License
  along with this library; if not, write to the Free Software Foundation, Inc.,
  51 Franklin St, Fifth Floor, Boston, MA
  02110-1301  USA

  $Date: 2009-04-08 14:57:09 +0200 (Wed, 08 Apr 2009) $
  Revision $Rev: 806 $
  Author $Author: gsent $
 */

#ifndef __OMXPARSERTEST_H__
#define __OMXPARSERTEST_H__

#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <string.h>
#include <pthread.h>
#include <ctype.h>

#include <OMX_Core.h>
#include <OMX_Component.h>
#include <OMX_Types.h>
#include <OMX_Video.h>
#include <OMX_Audio.h>

#include <tsemaphore.h>
#include <user_debug_levels.h>

#include "../../../src/components/vpu/OMX_VPU_Video.h"
typedef struct appPrivateType
{
    tsem_t* parser3gpEventSem;
    tsem_t* clockEventSem;
    tsem_t* videoDecoderEventSem;
    tsem_t* colorconvEventSem;
    tsem_t* videoschdEventSem;
    tsem_t* fbdevSinkEventSem;
    tsem_t* audioDecoderEventSem;
    tsem_t* audioSinkEventSem;
    tsem_t* volumeEventSem;
    tsem_t* eofSem;
    OMX_HANDLETYPE videodechandle;
    OMX_HANDLETYPE colorconv_handle;
    OMX_HANDLETYPE videosinkhandle;
    OMX_HANDLETYPE audiodechandle;
    OMX_HANDLETYPE audiosinkhandle;
    OMX_HANDLETYPE volumehandle;
    OMX_HANDLETYPE parser3gphandle;
    OMX_HANDLETYPE clocksrchandle;
    OMX_HANDLETYPE videoschd_handle;
} appPrivateType;

#define BUFFER_IN_SIZE 4096
#define BUFFER_OUT_SIZE   1920*1080*3 // 1382400    //921600 -- the output buffer size is chosen to support upto VGA picture: 640*480*3

/** Specification version*/
#define VERSIONMAJOR    1
#define VERSIONMINOR    1
#define VERSIONREVISION 0
#define VERSIONSTEP     0

/* Callback prototypes for video decoder*/
OMX_ERRORTYPE videodecEventHandler(
                                   OMX_HANDLETYPE hComponent,
                                   OMX_PTR pAppData,
                                   OMX_EVENTTYPE eEvent,
                                   OMX_U32 Data1,
                                   OMX_U32 Data2,
                                   OMX_PTR pEventData);

OMX_ERRORTYPE videodecEmptyBufferDone(
                                      OMX_HANDLETYPE hComponent,
                                      OMX_PTR pAppData,
                                      OMX_BUFFERHEADERTYPE* pBuffer);

OMX_ERRORTYPE videodecFillBufferDone(
                                     OMX_HANDLETYPE hComponent,
                                     OMX_PTR pAppData,
                                     OMX_BUFFERHEADERTYPE* pBuffer);

/** callback prototypes for color converter */
OMX_ERRORTYPE colorconvEventHandler(
                                    OMX_HANDLETYPE hComponent,
                                    OMX_PTR pAppData,
                                    OMX_EVENTTYPE eEvent,
                                    OMX_U32 Data1,
                                    OMX_U32 Data2,
                                    OMX_PTR pEventData);

OMX_ERRORTYPE colorconvEmptyBufferDone(
                                       OMX_HANDLETYPE hComponent,
                                       OMX_PTR pAppData,
                                       OMX_BUFFERHEADERTYPE* pBuffer);

OMX_ERRORTYPE colorconvFillBufferDone(
                                      OMX_HANDLETYPE hComponent,
                                      OMX_PTR pAppData,
                                      OMX_BUFFERHEADERTYPE* pBuffer);

/** callback prototypes for video scheduler */
OMX_ERRORTYPE videoschdEventHandler(
                                    OMX_HANDLETYPE hComponent,
                                    OMX_PTR pAppData,
                                    OMX_EVENTTYPE eEvent,
                                    OMX_U32 Data1,
                                    OMX_U32 Data2,
                                    OMX_PTR pEventData);

OMX_ERRORTYPE videoschdEmptyBufferDone(
                                       OMX_HANDLETYPE hComponent,
                                       OMX_PTR pAppData,
                                       OMX_BUFFERHEADERTYPE* pBuffer);

OMX_ERRORTYPE videoschdFillBufferDone(
                                      OMX_HANDLETYPE hComponent,
                                      OMX_PTR pAppData,
                                      OMX_BUFFERHEADERTYPE* pBuffer);

/** callback prototypes for video sink */
OMX_ERRORTYPE fb_sinkEventHandler(
                                  OMX_HANDLETYPE hComponent,
                                  OMX_PTR pAppData,
                                  OMX_EVENTTYPE eEvent,
                                  OMX_U32 Data1,
                                  OMX_U32 Data2,
                                  OMX_PTR pEventData);

OMX_ERRORTYPE fb_sinkEmptyBufferDone(
                                     OMX_HANDLETYPE hComponent,
                                     OMX_PTR pAppData,
                                     OMX_BUFFERHEADERTYPE* pBuffer);

/** callback prototypes for parser3gp */
OMX_ERRORTYPE parser3gpEventHandler(
                                    OMX_HANDLETYPE hComponent,
                                    OMX_PTR pAppData,
                                    OMX_EVENTTYPE eEvent,
                                    OMX_U32 Data1,
                                    OMX_U32 Data2,
                                    OMX_PTR pEventData);

OMX_ERRORTYPE parser3gpFillBufferDone(
                                      OMX_HANDLETYPE hComponent,
                                      OMX_PTR pAppData,
                                      OMX_BUFFERHEADERTYPE* pBuffer);

OMX_ERRORTYPE clocksrcEventHandler(
                                   OMX_HANDLETYPE hComponent,
                                   OMX_PTR pAppData,
                                   OMX_EVENTTYPE eEvent,
                                   OMX_U32 Data1,
                                   OMX_U32 Data2,
                                   OMX_PTR pEventData);

OMX_ERRORTYPE clocksrcFillBufferDone(
                                     OMX_HANDLETYPE hComponent,
                                     OMX_PTR pAppData,
                                     OMX_BUFFERHEADERTYPE* pBuffer);


/** callback prototypes for audio Decoder */
OMX_ERRORTYPE audiodecEventHandler(
                                   OMX_HANDLETYPE hComponent,
                                   OMX_PTR pAppData,
                                   OMX_EVENTTYPE eEvent,
                                   OMX_U32 Data1,
                                   OMX_U32 Data2,
                                   OMX_PTR pEventData);

OMX_ERRORTYPE audiodecEmptyBufferDone(
                                      OMX_HANDLETYPE hComponent,
                                      OMX_PTR pAppData,
                                      OMX_BUFFERHEADERTYPE* pBuffer);

OMX_ERRORTYPE audiodecFillBufferDone(
                                     OMX_HANDLETYPE hComponent,
                                     OMX_PTR pAppData,
                                     OMX_BUFFERHEADERTYPE* pBuffer);

/** callback prototypes for volume control */
OMX_ERRORTYPE volumeEventHandler(
                                 OMX_HANDLETYPE hComponent,
                                 OMX_PTR pAppData,
                                 OMX_EVENTTYPE eEvent,
                                 OMX_U32 Data1,
                                 OMX_U32 Data2,
                                 OMX_PTR pEventData);

OMX_ERRORTYPE volumeEmptyBufferDone(
                                    OMX_HANDLETYPE hComponent,
                                    OMX_PTR pAppData,
                                    OMX_BUFFERHEADERTYPE* pBuffer);

OMX_ERRORTYPE volumeFillBufferDone(
                                   OMX_HANDLETYPE hComponent,
                                   OMX_PTR pAppData,
                                   OMX_BUFFERHEADERTYPE* pBuffer);

/** callback prototypes for audio sink */
OMX_ERRORTYPE audiosinkEventHandler(
                                    OMX_HANDLETYPE hComponent,
                                    OMX_PTR pAppData,
                                    OMX_EVENTTYPE eEvent,
                                    OMX_U32 Data1,
                                    OMX_U32 Data2,
                                    OMX_PTR pEventData);

OMX_ERRORTYPE audiosinkEmptyBufferDone(
                                       OMX_HANDLETYPE hComponent,
                                       OMX_PTR pAppData,
                                       OMX_BUFFERHEADERTYPE* pBuffer);

/** display general help  */
void display_help();

/** this function sets the color converter and video sink port characteristics
 * based on the video decoder output port settings
 */
int setPortParameters();

#endif

