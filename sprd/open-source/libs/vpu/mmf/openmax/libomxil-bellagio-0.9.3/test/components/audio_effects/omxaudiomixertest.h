/**
  test/components/audio_effects/omxaudiomixertest.h

  This simple test application take one or more input stream/s. passes
  these streams to an audio mixer component and stores the mixed output in another
  output file.

  Copyright (C) 2008-2009  STMicroelectronics
  Copyright (C) 2008-2009 Nokia Corporation and/or its subsidiary(-ies).

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

*/

#ifndef __OMXAUDIOMIXERTEST_H__
#define __OMXAUDIOMIXERTEST_H__

#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <string.h>
#include <pthread.h>
#include <unistd.h>
#include <sys/stat.h>

#include <OMX_Core.h>
#include <OMX_Component.h>
#include <OMX_Types.h>
#include <OMX_Audio.h>

#include <bellagio/tsemaphore.h>
#include <user_debug_levels.h>

/** Specification version*/
#define VERSIONMAJOR    1
#define VERSIONMINOR    1
#define VERSIONREVISION 0
#define VERSIONSTEP     0

/* Application's private data */
typedef struct appPrivateType{
  pthread_cond_t condition;
  pthread_mutex_t mutex;
  void* input_data;
  OMX_BUFFERHEADERTYPE* currentInputBuffer;
  tsem_t* eventSem;
  tsem_t* eofSem;
  OMX_HANDLETYPE handle;
}appPrivateType;

/* Size of the buffers requested to the component */
#define BUFFER_IN_SIZE 16384 * 2

/* Callback prototypes */
OMX_ERRORTYPE audiomixerEventHandler(
  OMX_HANDLETYPE hComponent,
  OMX_PTR pAppData,
  OMX_EVENTTYPE eEvent,
  OMX_U32 Data1,
  OMX_U32 Data2,
  OMX_PTR pEventData);

OMX_ERRORTYPE audiomixerEmptyBufferDone(
  OMX_HANDLETYPE hComponent,
  OMX_PTR pAppData,
  OMX_BUFFERHEADERTYPE* pBuffer);

OMX_ERRORTYPE audiomixerFillBufferDone(
  OMX_HANDLETYPE hComponent,
  OMX_PTR pAppData,
  OMX_BUFFERHEADERTYPE* pBuffer);

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

/** Helper functions */
static int getFileSize(int fd);

#endif
