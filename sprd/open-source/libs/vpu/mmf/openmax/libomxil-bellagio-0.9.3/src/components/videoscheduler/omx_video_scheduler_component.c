/**
  src/components/videoscheduler/omx_video_scheduler_component.c

  This component implements a video scheduler

  Copyright (C) 2008-2009 STMicroelectronics
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

#include <omxcore.h>
#include <omx_video_scheduler_component.h>

#define DEFAULT_WIDTH   352
#define DEFAULT_HEIGHT  288
#define CLOCKPORT_INDEX 2

/** @@ Modified code
  * added deinterlace case
**/

/** define the max input buffer size */
#define DEFAULT_VIDEO_INPUT_BUF_SIZE DEFAULT_WIDTH*DEFAULT_HEIGHT*3/2

/** The Constructor
  * @param openmaxStandComp the component handle to be constructed
  * @param cComponentName is the name of the constructed component
  */
OMX_ERRORTYPE omx_video_scheduler_component_Constructor(OMX_COMPONENTTYPE *openmaxStandComp, OMX_STRING cComponentName) {
  OMX_ERRORTYPE                                err = OMX_ErrorNone;
  omx_video_scheduler_component_PrivateType*   omx_video_scheduler_component_Private;
  omx_base_video_PortType       *inPort,*outPort;
  OMX_U32                                      i;


	RM_RegisterComponent(VIDEO_SCHEDULER_COMP_NAME, MAX_VIDEOSCHED_COMPONENTS);
  if (!openmaxStandComp->pComponentPrivate) {
    DEBUG(DEB_LEV_FUNCTION_NAME, "In %s, allocating component\n", __func__);
    //openmaxStandComp->pComponentPrivate = calloc(1, sizeof(omx_video_scheduler_component_PrivateType));
    openmaxStandComp->pComponentPrivate = malloc(sizeof(omx_video_scheduler_component_PrivateType));
    if(openmaxStandComp->pComponentPrivate == NULL) {
      return OMX_ErrorInsufficientResources;
    }
    memset(openmaxStandComp->pComponentPrivate, 0x00,sizeof(omx_video_scheduler_component_PrivateType));
  } else {
    DEBUG(DEB_LEV_FUNCTION_NAME, "In %s, Error Component %p Already Allocated\n", __func__, openmaxStandComp->pComponentPrivate);
  }

  omx_video_scheduler_component_Private        = openmaxStandComp->pComponentPrivate;
  omx_video_scheduler_component_Private->ports = NULL;

  /** we could create our own port structures here
    * fixme maybe the base class could use a "port factory" function pointer?
    */
  err = omx_base_filter_Constructor(openmaxStandComp, cComponentName);

  omx_video_scheduler_component_Private->sPortTypesParam[OMX_PortDomainVideo].nStartPortNumber = 0;
  omx_video_scheduler_component_Private->sPortTypesParam[OMX_PortDomainVideo].nPorts = 2;

  omx_video_scheduler_component_Private->sPortTypesParam[OMX_PortDomainOther].nStartPortNumber = CLOCKPORT_INDEX;
  omx_video_scheduler_component_Private->sPortTypesParam[OMX_PortDomainOther].nPorts = 1;

  /** Allocate Ports and call port constructor. */
 if ((omx_video_scheduler_component_Private->sPortTypesParam[OMX_PortDomainVideo].nPorts
       + omx_video_scheduler_component_Private->sPortTypesParam[OMX_PortDomainOther].nPorts)
       && !omx_video_scheduler_component_Private->ports) {
    /*omx_video_scheduler_component_Private->ports = calloc((omx_video_scheduler_component_Private->sPortTypesParam[OMX_PortDomainVideo].nPorts
                                                           + omx_video_scheduler_component_Private->sPortTypesParam[OMX_PortDomainOther].nPorts),
                                                           sizeof(omx_base_PortType *));*/
                                                           
   	omx_video_scheduler_component_Private->ports = malloc(((omx_video_scheduler_component_Private->sPortTypesParam[OMX_PortDomainVideo].nPorts
                                                           + omx_video_scheduler_component_Private->sPortTypesParam[OMX_PortDomainOther].nPorts))*
                                                           sizeof(omx_base_PortType *));
                                                           
    if (!omx_video_scheduler_component_Private->ports) {
      return OMX_ErrorInsufficientResources;
    }
    
    memset(omx_video_scheduler_component_Private->ports, 0x00, ((omx_video_scheduler_component_Private->sPortTypesParam[OMX_PortDomainVideo].nPorts
                                                           + omx_video_scheduler_component_Private->sPortTypesParam[OMX_PortDomainOther].nPorts))*
                                                           sizeof(omx_base_PortType *));
    
    for (i=0; i < omx_video_scheduler_component_Private->sPortTypesParam[OMX_PortDomainVideo].nPorts; i++) {
      //omx_video_scheduler_component_Private->ports[i] = calloc(1, sizeof(omx_base_video_PortType));
      omx_video_scheduler_component_Private->ports[i] = malloc(sizeof(omx_base_video_PortType));
      if (!omx_video_scheduler_component_Private->ports[i]) {
        return OMX_ErrorInsufficientResources;
      }
      memset(omx_video_scheduler_component_Private->ports[i], 0x00, sizeof(omx_base_video_PortType));
    }
    base_video_port_Constructor(openmaxStandComp, &omx_video_scheduler_component_Private->ports[OMX_BASE_FILTER_INPUTPORT_INDEX], 0, OMX_TRUE);
    base_video_port_Constructor(openmaxStandComp, &omx_video_scheduler_component_Private->ports[OMX_BASE_FILTER_OUTPUTPORT_INDEX], 1, OMX_FALSE);

    //omx_video_scheduler_component_Private->ports[CLOCKPORT_INDEX] = calloc(1, sizeof(omx_base_clock_PortType));
    omx_video_scheduler_component_Private->ports[CLOCKPORT_INDEX] = malloc(sizeof(omx_base_clock_PortType));
    if (!omx_video_scheduler_component_Private->ports[CLOCKPORT_INDEX]) {
      return OMX_ErrorInsufficientResources;
    }
    memset(omx_video_scheduler_component_Private->ports[CLOCKPORT_INDEX],0x00, sizeof(omx_base_clock_PortType));
    base_clock_port_Constructor(openmaxStandComp, &omx_video_scheduler_component_Private->ports[CLOCKPORT_INDEX], 2, OMX_TRUE);
    omx_video_scheduler_component_Private->ports[CLOCKPORT_INDEX]->sPortParam.bEnabled = OMX_TRUE;
  }

  inPort = (omx_base_video_PortType *) omx_video_scheduler_component_Private->ports[OMX_BASE_FILTER_INPUTPORT_INDEX];
  outPort= (omx_base_video_PortType *) omx_video_scheduler_component_Private->ports[OMX_BASE_FILTER_OUTPUTPORT_INDEX];

  /** Domain specific section for the ports. */

  //input port parameter settings
  inPort->sVideoParam.eColorFormat             = OMX_COLOR_Format24bitRGB888;
  inPort->sPortParam.format.video.nFrameWidth  = DEFAULT_WIDTH;
  inPort->sPortParam.format.video.nFrameHeight = DEFAULT_HEIGHT;
  inPort->sPortParam.nBufferSize               = DEFAULT_VIDEO_INPUT_BUF_SIZE * 2 ;
  inPort->sPortParam.format.video.eColorFormat = OMX_COLOR_Format24bitRGB888;

  outPort->sVideoParam.eColorFormat             = OMX_COLOR_Format24bitRGB888;
  outPort->sPortParam.format.video.nFrameWidth  = DEFAULT_WIDTH;
  outPort->sPortParam.format.video.nFrameHeight = DEFAULT_HEIGHT;
  outPort->sPortParam.nBufferSize               = DEFAULT_VIDEO_INPUT_BUF_SIZE * 2;
  outPort->sPortParam.format.video.eColorFormat = OMX_COLOR_Format24bitRGB888;

  omx_video_scheduler_component_Private->destructor         = omx_video_scheduler_component_Destructor;
  omx_video_scheduler_component_Private->BufferMgmtCallback = omx_video_scheduler_component_BufferMgmtCallback;

  inPort->FlushProcessingBuffers  = omx_video_scheduler_component_port_FlushProcessingBuffers;
  openmaxStandComp->SetParameter  = omx_video_scheduler_component_SetParameter;
  openmaxStandComp->GetParameter  = omx_video_scheduler_component_GetParameter;

  /* resource management special section */
  omx_video_scheduler_component_Private->nqualitylevels = VIDEOSCHED_QUALITY_LEVELS;
  omx_video_scheduler_component_Private->currentQualityLevel = 1;
  omx_video_scheduler_component_Private->multiResourceLevel = malloc(sizeof(multiResourceDescriptor *) * VIDEOSCHED_QUALITY_LEVELS);
  for (i = 0; i<VIDEOSCHED_QUALITY_LEVELS; i++) {
	  omx_video_scheduler_component_Private->multiResourceLevel[i] = malloc(sizeof(multiResourceDescriptor));
	  omx_video_scheduler_component_Private->multiResourceLevel[i]->CPUResourceRequested = videoSchedQualityLevels[i * 2];
	  omx_video_scheduler_component_Private->multiResourceLevel[i]->MemoryResourceRequested = videoSchedQualityLevels[i * 2 + 1];
  }

  return err;
}

/** The destructor
 */
OMX_ERRORTYPE omx_video_scheduler_component_Destructor(OMX_COMPONENTTYPE *openmaxStandComp) {
  omx_video_scheduler_component_PrivateType*   omx_video_scheduler_component_Private = openmaxStandComp->pComponentPrivate;
  OMX_U32                                      i;

  DEBUG(DEB_LEV_FUNCTION_NAME, "In %s\n", __func__);

  /* frees port/s */
  if (omx_video_scheduler_component_Private->ports) {
  for(i=0; i < (omx_video_scheduler_component_Private->sPortTypesParam[OMX_PortDomainVideo].nPorts +
                   omx_video_scheduler_component_Private->sPortTypesParam[OMX_PortDomainOther].nPorts); i++) {
    if(omx_video_scheduler_component_Private->ports[i])
       omx_video_scheduler_component_Private->ports[i]->PortDestructor(omx_video_scheduler_component_Private->ports[i]);
    }
    free(omx_video_scheduler_component_Private->ports);
    omx_video_scheduler_component_Private->ports=NULL;
  }

  omx_base_filter_Destructor(openmaxStandComp);
  DEBUG(DEB_LEV_FUNCTION_NAME, "Out of %s\n", __func__);

  return OMX_ErrorNone;
}


/** @brief the entry point for sending buffers to the video scheduler ports
 *
 * This function can be called by the EmptyThisBuffer or FillThisBuffer. It depends on
 * the nature of the port, that can be an input or output port.
 */
OMX_ERRORTYPE omx_video_scheduler_component_port_SendBufferFunction(omx_base_PortType *openmaxStandPort, OMX_BUFFERHEADERTYPE* pBuffer) {

  OMX_ERRORTYPE                   err;
  int                             errQue;
  OMX_U32                         portIndex;
  OMX_COMPONENTTYPE*              omxComponent = openmaxStandPort->standCompContainer;
  omx_base_component_PrivateType* omx_base_component_Private = (omx_base_component_PrivateType*)omxComponent->pComponentPrivate;
  OMX_BOOL                        SendFrame;
  omx_base_clock_PortType*        pClockPort;
#if NO_GST_OMX_PATCH
  unsigned int i;
#endif

  portIndex = (openmaxStandPort->sPortParam.eDir == OMX_DirInput)?pBuffer->nInputPortIndex:pBuffer->nOutputPortIndex;
  DEBUG(DEB_LEV_FUNCTION_NAME, "In %s portIndex %lu\n", __func__, portIndex);

  if (portIndex != openmaxStandPort->sPortParam.nPortIndex) {
    DEBUG(DEB_LEV_ERR, "In %s: wrong port for this operation portIndex=%d port->portIndex=%d\n",
           __func__, (int)portIndex, (int)openmaxStandPort->sPortParam.nPortIndex);
    return OMX_ErrorBadPortIndex;
  }

  if(omx_base_component_Private->state == OMX_StateInvalid) {
    DEBUG(DEB_LEV_ERR, "In %s: we are in OMX_StateInvalid\n", __func__);
    return OMX_ErrorInvalidState;
  }

  if(omx_base_component_Private->state != OMX_StateExecuting &&
    omx_base_component_Private->state != OMX_StatePause &&
    omx_base_component_Private->state != OMX_StateIdle) {
    DEBUG(DEB_LEV_ERR, "In %s: we are not in executing/paused/idle state, but in %d\n", __func__, omx_base_component_Private->state);
    return OMX_ErrorIncorrectStateOperation;
  }
  if (!PORT_IS_ENABLED(openmaxStandPort) || (PORT_IS_BEING_DISABLED(openmaxStandPort) && !PORT_IS_TUNNELED_N_BUFFER_SUPPLIER(openmaxStandPort)) ||
      (omx_base_component_Private->transientState == OMX_TransStateExecutingToIdle &&
      (PORT_IS_TUNNELED(openmaxStandPort) && !PORT_IS_BUFFER_SUPPLIER(openmaxStandPort)))) {
    DEBUG(DEB_LEV_ERR, "In %s: Port %d is disabled comp = %s \n", __func__, (int)portIndex,omx_base_component_Private->name);
    return OMX_ErrorIncorrectStateOperation;
  }

  /* Temporarily disable this check for gst-openmax */
#if NO_GST_OMX_PATCH
  {
  OMX_BOOL foundBuffer = OMX_FALSE;
  if(pBuffer!=NULL && pBuffer->pBuffer!=NULL) {
    for(i=0; i < openmaxStandPort->sPortParam.nBufferCountActual; i++){
    if (pBuffer->pBuffer == openmaxStandPort->pInternalBufferStorage[i]->pBuffer) {
      foundBuffer = OMX_TRUE;
      break;
    }
    }
  }
  if (!foundBuffer) {
    return OMX_ErrorBadParameter;
  }
  }
#endif

  if ((err = checkHeader(pBuffer, sizeof(OMX_BUFFERHEADERTYPE))) != OMX_ErrorNone) {
    DEBUG(DEB_LEV_ERR, "In %s: received wrong buffer header on input port\n", __func__);
    return err;
  }

  pClockPort  = (omx_base_clock_PortType*)omx_base_component_Private->ports[CLOCKPORT_INDEX];
  if(PORT_IS_TUNNELED(pClockPort) && !PORT_IS_BEING_FLUSHED(openmaxStandPort) &&
      (omx_base_component_Private->transientState != OMX_TransStateExecutingToIdle) &&
      ((pBuffer->nFlags & OMX_BUFFERFLAG_EOS) != OMX_BUFFERFLAG_EOS)){
    SendFrame = omx_video_scheduler_component_ClockPortHandleFunction((omx_video_scheduler_component_PrivateType*)omx_base_component_Private, pBuffer);
    if(!SendFrame) pBuffer->nFilledLen=0;
  }

  /* And notify the buffer management thread we have a fresh new buffer to manage */
  if(!PORT_IS_BEING_FLUSHED(openmaxStandPort) && !(PORT_IS_BEING_DISABLED(openmaxStandPort) && PORT_IS_TUNNELED_N_BUFFER_SUPPLIER(openmaxStandPort))
      && omx_base_component_Private->transientState != OMX_TransStateExecutingToIdle){
      errQue = queue(openmaxStandPort->pBufferQueue, pBuffer);
      if (errQue) {
    	  /* /TODO the queue is full. This can be handled in a fine way with
    	   * some retrials, or other checking. For the moment this is a critical error
    	   * and simply causes the failure of this call
    	   */
    	  return OMX_ErrorInsufficientResources;
      }
   	  tsem_up(openmaxStandPort->pBufferSem);
   	  DEBUG(DEB_LEV_FULL_SEQ, "In %s Signalling bMgmtSem Port Index=%d\n",__func__, (int)portIndex);
   	  tsem_up(omx_base_component_Private->bMgmtSem);
  }else if(PORT_IS_BUFFER_SUPPLIER(openmaxStandPort)){
      DEBUG(DEB_LEV_FULL_SEQ, "In %s: Comp %s received io:%d buffer\n", __func__,omx_base_component_Private->name,(int)openmaxStandPort->sPortParam.nPortIndex);
      errQue = queue(openmaxStandPort->pBufferQueue, pBuffer);
      if (errQue) {
    	  /* /TODO the queue is full. This can be handled in a fine way with
    	   * some retrials, or other checking. For the moment this is a critical error
    	   * and simply causes the failure of this call
    	   */
    	  return OMX_ErrorInsufficientResources;
      }
      tsem_up(openmaxStandPort->pBufferSem);
  } else { // If port being flushed and not tunneled then return error
    DEBUG(DEB_LEV_FULL_SEQ, "In %s \n", __func__);
    return OMX_ErrorIncorrectStateOperation;
  }
  return OMX_ErrorNone;
}


OMX_BOOL omx_video_scheduler_component_ClockPortHandleFunction(
  omx_video_scheduler_component_PrivateType* omx_video_scheduler_component_Private,
  OMX_BUFFERHEADERTYPE* pInputBuffer){
  omx_base_clock_PortType               *pClockPort;
  OMX_HANDLETYPE                        hclkComponent;
  OMX_BUFFERHEADERTYPE*                 clockBuffer;
  OMX_TIME_MEDIATIMETYPE*               pMediaTime;
  OMX_TIME_CONFIG_TIMESTAMPTYPE         sClientTimeStamp;
  OMX_ERRORTYPE                         err;
  OMX_BOOL                              SendFrame;
  omx_base_video_PortType               *pInputPort;

  pClockPort    = (omx_base_clock_PortType*) omx_video_scheduler_component_Private->ports[CLOCKPORT_INDEX];
  pInputPort    = (omx_base_video_PortType *) omx_video_scheduler_component_Private->ports[0];
  hclkComponent = pClockPort->hTunneledComponent;

  SendFrame = OMX_TRUE;

  DEBUG(DEB_LEV_FULL_SEQ, "In %s Clock Port is Tunneled. Sending Request\n", __func__);
  /* if first time stamp is received then notify the clock component */
  if((pInputBuffer->nFlags & OMX_BUFFERFLAG_STARTTIME) == OMX_BUFFERFLAG_STARTTIME) {
    DEBUG(DEB_LEV_FULL_SEQ," In %s  first time stamp = %llx \n", __func__,pInputBuffer->nTimeStamp);
    pInputBuffer->nFlags = 0;
    hclkComponent = pClockPort->hTunneledComponent;
    setHeader(&sClientTimeStamp, sizeof(OMX_TIME_CONFIG_TIMESTAMPTYPE));
    sClientTimeStamp.nPortIndex = pClockPort->nTunneledPort;
    sClientTimeStamp.nTimestamp = pInputBuffer->nTimeStamp;
    err = OMX_SetConfig(hclkComponent, OMX_IndexConfigTimeClientStartTime, &sClientTimeStamp);
    if(err!=OMX_ErrorNone) {
     DEBUG(DEB_LEV_ERR,"Error %08x In OMX_SetConfig in func=%s \n",err,__func__);
    }
    tsem_down(pClockPort->pBufferSem); /* wait for state change notification */

    /* update the clock state and clock scale info into the fbdev private data */
    if(pClockPort->pBufferQueue->nelem > 0) {
      clockBuffer=dequeue(pClockPort->pBufferQueue);
      pMediaTime  = (OMX_TIME_MEDIATIMETYPE*)clockBuffer->pBuffer;
      omx_video_scheduler_component_Private->eState      = pMediaTime->eState;
      omx_video_scheduler_component_Private->xScale      = pMediaTime->xScale;
      pClockPort->ReturnBufferFunction((omx_base_PortType*)pClockPort,clockBuffer);
    }
  }

  /* do not send the data to sink and return back, if the clock is not running*/
  if(!omx_video_scheduler_component_Private->eState==OMX_TIME_ClockStateRunning){
    pInputBuffer->nFilledLen=0;
    SendFrame = OMX_FALSE;
    return SendFrame;
  }

  /* check for any scale change information from the clock component */
  if(pClockPort->pBufferSem->semval>0) {
    tsem_down(pClockPort->pBufferSem);
    if(pClockPort->pBufferQueue->nelem > 0) {
      clockBuffer = dequeue(pClockPort->pBufferQueue);
      pMediaTime  = (OMX_TIME_MEDIATIMETYPE*)clockBuffer->pBuffer;
      if(pMediaTime->eUpdateType==OMX_TIME_UpdateScaleChanged) {
        /* On scale change update the media time base */
        sClientTimeStamp.nPortIndex = pClockPort->nTunneledPort;
        sClientTimeStamp.nTimestamp = pInputBuffer->nTimeStamp;
        err = OMX_SetConfig(hclkComponent, OMX_IndexConfigTimeCurrentVideoReference, &sClientTimeStamp);
        if(err!=OMX_ErrorNone) {
          DEBUG(DEB_LEV_ERR,"Error %08x In OMX_SetConfig in func=%s \n",err,__func__);
        }
        omx_video_scheduler_component_Private->frameDropFlag = OMX_TRUE;
        omx_video_scheduler_component_Private->dropFrameCount = 0;
        omx_video_scheduler_component_Private->xScale = pMediaTime->xScale;
      }
      pClockPort->ReturnBufferFunction((omx_base_PortType*)pClockPort,clockBuffer);
    }
  }

  /* drop next seven frames on scale change
     and rebase the clock time base */
  if(omx_video_scheduler_component_Private->frameDropFlag && omx_video_scheduler_component_Private->dropFrameCount<7) { //TODO - second check cond can be removed verify
     omx_video_scheduler_component_Private->dropFrameCount ++;
     if(omx_video_scheduler_component_Private->dropFrameCount==7) {
        /* rebase the clock time base */
        setHeader(&sClientTimeStamp, sizeof(OMX_TIME_CONFIG_TIMESTAMPTYPE));
        sClientTimeStamp.nPortIndex = pClockPort->nTunneledPort;
        sClientTimeStamp.nTimestamp = pInputBuffer->nTimeStamp;
        err = OMX_SetConfig(hclkComponent, OMX_IndexConfigTimeCurrentVideoReference, &sClientTimeStamp);
        if(err!=OMX_ErrorNone) {
          DEBUG(DEB_LEV_ERR,"Error %08x In OMX_SetConfig in func=%s \n",err,__func__);
        }

        omx_video_scheduler_component_Private->frameDropFlag  = OMX_FALSE;
        omx_video_scheduler_component_Private->dropFrameCount = 0;
        SendFrame = OMX_TRUE;
     }
     SendFrame = OMX_FALSE;
  }

  /* frame is not to be dropped so send the request for the timestamp for the data delivery */
  if(SendFrame){
    if(!PORT_IS_BEING_FLUSHED(pInputPort) && !PORT_IS_BEING_FLUSHED(pClockPort) &&
        omx_video_scheduler_component_Private->transientState != OMX_TransStateExecutingToIdle) {
      setHeader(&pClockPort->sMediaTimeRequest, sizeof(OMX_TIME_CONFIG_MEDIATIMEREQUESTTYPE));
      pClockPort->sMediaTimeRequest.nMediaTimestamp = pInputBuffer->nTimeStamp;
      pClockPort->sMediaTimeRequest.nOffset         = 100; /*set the requested offset */
      pClockPort->sMediaTimeRequest.nPortIndex      = pClockPort->nTunneledPort;
      pClockPort->sMediaTimeRequest.pClientPrivate  = NULL; /* fill the appropriate value */
      err = OMX_SetConfig(hclkComponent, OMX_IndexConfigTimeMediaTimeRequest, &pClockPort->sMediaTimeRequest);
      if(err!=OMX_ErrorNone) {
        DEBUG(DEB_LEV_ERR,"Error %08x In OMX_SetConfig in func=%s \n",err,__func__);
      }
      if(!PORT_IS_BEING_FLUSHED(pInputPort) && !PORT_IS_BEING_FLUSHED(pClockPort) &&
          omx_video_scheduler_component_Private->transientState != OMX_TransStateExecutingToIdle) {
        tsem_down(pClockPort->pBufferSem); /* wait for the request fullfillment */
        if(pClockPort->pBufferQueue->nelem > 0) {
          clockBuffer = dequeue(pClockPort->pBufferQueue);
          pMediaTime  = (OMX_TIME_MEDIATIMETYPE*)clockBuffer->pBuffer;
          if(pMediaTime->eUpdateType==OMX_TIME_UpdateScaleChanged) {
           /* update the media time base */
            setHeader(&sClientTimeStamp, sizeof(OMX_TIME_CONFIG_TIMESTAMPTYPE)); // do not need to setHeader again do once at the top
            sClientTimeStamp.nPortIndex = pClockPort->nTunneledPort;
            sClientTimeStamp.nTimestamp = pInputBuffer->nTimeStamp;
            err = OMX_SetConfig(hclkComponent, OMX_IndexConfigTimeCurrentVideoReference, &sClientTimeStamp);
            if(err!=OMX_ErrorNone) {
              DEBUG(DEB_LEV_ERR,"Error %08x In OMX_SetConfig in func=%s \n",err,__func__);
            }
            omx_video_scheduler_component_Private->frameDropFlag  = OMX_TRUE;
            omx_video_scheduler_component_Private->dropFrameCount = 0;
            omx_video_scheduler_component_Private->xScale = pMediaTime->xScale;
          }
          if(pMediaTime->eUpdateType==OMX_TIME_UpdateRequestFulfillment){
            if((pMediaTime->nOffset)>0) {
              SendFrame=OMX_TRUE;
            }else {
              SendFrame = OMX_FALSE;
            }
          }
          pClockPort->ReturnBufferFunction((omx_base_PortType *)pClockPort,clockBuffer);
        }
      }
    }
  }
  return(SendFrame);
}


/** @brief Releases buffers under processing.
 * This function must be implemented in the derived classes, for the
 * specific processing
 */
OMX_ERRORTYPE  omx_video_scheduler_component_port_FlushProcessingBuffers(omx_base_PortType *openmaxStandPort) {
  omx_base_component_PrivateType*              omx_base_component_Private;
  omx_video_scheduler_component_PrivateType*   omx_video_scheduler_component_Private;
  OMX_BUFFERHEADERTYPE*                        pBuffer;
  omx_base_clock_PortType                      *pClockPort;
  int errQue;

  DEBUG(DEB_LEV_FUNCTION_NAME, "In %s\n", __func__);
  omx_base_component_Private             = (omx_base_component_PrivateType*)openmaxStandPort->standCompContainer->pComponentPrivate;
  omx_video_scheduler_component_Private  = ( omx_video_scheduler_component_PrivateType*) omx_base_component_Private;

  pClockPort    = (omx_base_clock_PortType*) omx_video_scheduler_component_Private->ports[CLOCKPORT_INDEX];

  if(openmaxStandPort->sPortParam.eDomain!=OMX_PortDomainOther) { /* clock buffers not used in the clients buffer managment function */
    pthread_mutex_lock(&omx_base_component_Private->flush_mutex);
    openmaxStandPort->bIsPortFlushed=OMX_TRUE;
    /*Signal the buffer management thread of port flush,if it is waiting for buffers*/
    if(omx_base_component_Private->bMgmtSem->semval==0) {
      tsem_up(omx_base_component_Private->bMgmtSem);
    }

    if(omx_base_component_Private->state != OMX_StateExecuting ) {
      /*signal at non-executing state*/
      tsem_signal(omx_base_component_Private->bStateSem);
    }
    DEBUG(DEB_LEV_FULL_SEQ, "In %s waiting for flush all condition port index =%d\n", __func__,(int)openmaxStandPort->sPortParam.nPortIndex);
    /* Wait until flush is completed */
    pthread_mutex_unlock(&omx_base_component_Private->flush_mutex);

    /*Dummy signal to clock port*/
    if(pClockPort->pBufferSem->semval == 0) {
      tsem_up(pClockPort->pBufferSem);
      tsem_reset(pClockPort->pBufferSem);
    }
    tsem_down(omx_base_component_Private->flush_all_condition);
  }

  tsem_reset(omx_base_component_Private->bMgmtSem);

  /* Flush all the buffers not under processing */
  while (openmaxStandPort->pBufferSem->semval > 0) {
    DEBUG(DEB_LEV_FULL_SEQ, "In %s TFlag=%x Flusing Port=%d,Semval=%d Qelem=%d\n",
    __func__,(int)openmaxStandPort->nTunnelFlags,(int)openmaxStandPort->sPortParam.nPortIndex,
    (int)openmaxStandPort->pBufferSem->semval,(int)openmaxStandPort->pBufferQueue->nelem);

    tsem_down(openmaxStandPort->pBufferSem);
    pBuffer = dequeue(openmaxStandPort->pBufferQueue);
    if (PORT_IS_TUNNELED(openmaxStandPort) && !PORT_IS_BUFFER_SUPPLIER(openmaxStandPort)) {
      DEBUG(DEB_LEV_FULL_SEQ, "In %s: Comp %s is returning io:%d buffer\n",
        __func__,omx_base_component_Private->name,(int)openmaxStandPort->sPortParam.nPortIndex);
      if (openmaxStandPort->sPortParam.eDir == OMX_DirInput) {
        ((OMX_COMPONENTTYPE*)(openmaxStandPort->hTunneledComponent))->FillThisBuffer(openmaxStandPort->hTunneledComponent, pBuffer);
      } else {
        ((OMX_COMPONENTTYPE*)(openmaxStandPort->hTunneledComponent))->EmptyThisBuffer(openmaxStandPort->hTunneledComponent, pBuffer);
      }
    } else if (PORT_IS_TUNNELED_N_BUFFER_SUPPLIER(openmaxStandPort)) {
        errQue = queue(openmaxStandPort->pBufferQueue,pBuffer);
        if (errQue) {
      	  /* /TODO the queue is full. This can be handled in a fine way with
      	   * some retrials, or other checking. For the moment this is a critical error
      	   * and simply causes the failure of this call
      	   */
      	  return OMX_ErrorInsufficientResources;
        }
    } else {
      (*(openmaxStandPort->BufferProcessedCallback))(
        openmaxStandPort->standCompContainer,
        omx_base_component_Private->callbackData,
        pBuffer);
    }
  }
  /*Port is tunneled and supplier and didn't received all it's buffer then wait for the buffers*/
  if (PORT_IS_TUNNELED_N_BUFFER_SUPPLIER(openmaxStandPort)) {
    while(openmaxStandPort->pBufferQueue->nelem!= openmaxStandPort->nNumAssignedBuffers){
      tsem_down(openmaxStandPort->pBufferSem);
      DEBUG(DEB_LEV_PARAMS, "In %s Got a buffer qelem=%d\n",__func__,openmaxStandPort->pBufferQueue->nelem);
    }
    tsem_reset(openmaxStandPort->pBufferSem);
  }

  pthread_mutex_lock(&omx_base_component_Private->flush_mutex);
  openmaxStandPort->bIsPortFlushed=OMX_FALSE;
  pthread_mutex_unlock(&omx_base_component_Private->flush_mutex);

  tsem_up(omx_base_component_Private->flush_condition);
  tsem_up(omx_base_component_Private->deinterlace_flush_condition);

  DEBUG(DEB_LEV_FULL_SEQ, "Out %s Port Index=%d bIsPortFlushed=%d Component %s\n", __func__,
    (int)openmaxStandPort->sPortParam.nPortIndex,(int)openmaxStandPort->bIsPortFlushed,omx_base_component_Private->name);

  DEBUG(DEB_LEV_PARAMS, "In %s TFlag=%x Qelem=%d BSem=%d bMgmtsem=%d component=%s\n", __func__,
    (int)openmaxStandPort->nTunnelFlags,
    (int)openmaxStandPort->pBufferQueue->nelem,
    (int)openmaxStandPort->pBufferSem->semval,
    (int)omx_base_component_Private->bMgmtSem->semval,
    omx_base_component_Private->name);

  DEBUG(DEB_LEV_FUNCTION_NAME, "Out %s Port Index=%d\n", __func__,(int)openmaxStandPort->sPortParam.nPortIndex);

  return OMX_ErrorNone;
}

/** This function is used to process the input buffer and provide one output buffer
  */
void omx_video_scheduler_component_BufferMgmtCallback(OMX_COMPONENTTYPE *openmaxStandComp, OMX_BUFFERHEADERTYPE* pInputBuffer, OMX_BUFFERHEADERTYPE* pOutputBuffer) {

  omx_video_scheduler_component_PrivateType*   omx_video_scheduler_component_Private = openmaxStandComp->pComponentPrivate;
  omx_base_video_PortType       *inPort = (omx_base_video_PortType *)omx_video_scheduler_component_Private->ports[OMX_BASE_FILTER_INPUTPORT_INDEX];
  omx_base_clock_PortType*        pClockPort;
  OMX_BOOL                        SendFrame;

  pClockPort  = (omx_base_clock_PortType*)omx_video_scheduler_component_Private->ports[CLOCKPORT_INDEX];
  if(PORT_IS_TUNNELED(pClockPort) && !PORT_IS_BEING_FLUSHED(inPort) &&
      (omx_video_scheduler_component_Private->transientState != OMX_TransStateExecutingToIdle) &&
      ((pInputBuffer->nFlags & OMX_BUFFERFLAG_EOS) != OMX_BUFFERFLAG_EOS)){
    SendFrame = omx_video_scheduler_component_ClockPortHandleFunction(omx_video_scheduler_component_Private, pInputBuffer);
    if(!SendFrame) pInputBuffer->nFilledLen = 0;
  }

  if((pInputBuffer->pBuffer != pOutputBuffer->pBuffer) && (pInputBuffer->nFilledLen > 0)){
    memcpy(pOutputBuffer->pBuffer,pInputBuffer->pBuffer,pInputBuffer->nFilledLen);
    pOutputBuffer->nOffset = pInputBuffer->nOffset;
  }
  pOutputBuffer->nFilledLen = pInputBuffer->nFilledLen;
  pInputBuffer->nFilledLen=0;
}



OMX_ERRORTYPE omx_video_scheduler_component_SetParameter(
  OMX_HANDLETYPE hComponent,
  OMX_INDEXTYPE nParamIndex,
  OMX_PTR ComponentParameterStructure) {

  OMX_ERRORTYPE                     err = OMX_ErrorNone;
  OMX_PARAM_PORTDEFINITIONTYPE      *pPortDef;
  OMX_VIDEO_PARAM_PORTFORMATTYPE    *pVideoPortFormat;
  OMX_OTHER_PARAM_PORTFORMATTYPE    *pOtherPortFormat;
  OMX_U32                           portIndex;
  OMX_PARAM_COMPONENTROLETYPE       *pComponentRole;

  /* Check which structure we are being fed and make control its header */
  OMX_COMPONENTTYPE                           *openmaxStandComp = (OMX_COMPONENTTYPE *)hComponent;
  omx_video_scheduler_component_PrivateType*  omx_video_scheduler_component_Private = openmaxStandComp->pComponentPrivate;
  omx_base_video_PortType                     *pPort;
  omx_base_clock_PortType                     *pClockPort;

  if (ComponentParameterStructure == NULL) {
    return OMX_ErrorBadParameter;
  }

  DEBUG(DEB_LEV_SIMPLE_SEQ, "   Setting parameter %i\n", nParamIndex);
  switch(nParamIndex) {
     case OMX_IndexParamPortDefinition:
      pPortDef = (OMX_PARAM_PORTDEFINITIONTYPE*) ComponentParameterStructure;
      portIndex = pPortDef->nPortIndex;
      err = omx_base_component_ParameterSanityCheck(hComponent, portIndex, pPortDef, sizeof(OMX_PARAM_PORTDEFINITIONTYPE));
      if(err!=OMX_ErrorNone) {
        DEBUG(DEB_LEV_ERR, "In %s Parameter Check Error=%x\n",__func__,err);
        break;
      }

      if(portIndex > (omx_video_scheduler_component_Private->sPortTypesParam[OMX_PortDomainVideo].nPorts +
                      omx_video_scheduler_component_Private->sPortTypesParam[OMX_PortDomainOther].nPorts)) {
        return OMX_ErrorBadPortIndex;
      }

      if(portIndex < CLOCKPORT_INDEX) {
        pPort = (omx_base_video_PortType *) omx_video_scheduler_component_Private->ports[portIndex];
        pPort->sPortParam.nBufferCountActual = pPortDef->nBufferCountActual;
        //  Copy stuff from OMX_VIDEO_PORTDEFINITIONTYPE structure
        if(pPortDef->format.video.cMIMEType != NULL) {
          strcpy(pPort->sPortParam.format.video.cMIMEType , pPortDef->format.video.cMIMEType);
        }
        pPort->sPortParam.format.video.nFrameWidth  = pPortDef->format.video.nFrameWidth;
        pPort->sPortParam.format.video.nFrameHeight = pPortDef->format.video.nFrameHeight;
        pPort->sPortParam.format.video.nBitrate     = pPortDef->format.video.nBitrate;
        pPort->sPortParam.format.video.xFramerate   = pPortDef->format.video.xFramerate;
        pPort->sPortParam.format.video.bFlagErrorConcealment = pPortDef->format.video.bFlagErrorConcealment;

        //  Figure out stride, slice height, min buffer size
        pPort->sPortParam.format.video.nStride      = pPortDef->format.video.nStride;
        pPort->sPortParam.format.video.nSliceHeight = pPort->sPortParam.format.video.nFrameHeight;  //  No support for slices yet
        // Read-only field by spec

        pPort->sPortParam.nBufferSize = (OMX_U32) abs(pPort->sPortParam.format.video.nStride) * pPort->sPortParam.format.video.nSliceHeight;
      } else {
        pClockPort = (omx_base_clock_PortType *) omx_video_scheduler_component_Private->ports[portIndex];

        pClockPort->sPortParam.nBufferCountActual   = pPortDef->nBufferCountActual;
        pClockPort->sPortParam.format.other.eFormat = pPortDef->format.other.eFormat;
      }
      break;

    case OMX_IndexParamVideoPortFormat:
      //  FIXME: How do we handle the nIndex member?
      pVideoPortFormat = (OMX_VIDEO_PARAM_PORTFORMATTYPE*)ComponentParameterStructure;
      portIndex = pVideoPortFormat->nPortIndex;
      err = omx_base_component_ParameterSanityCheck(hComponent, portIndex, pVideoPortFormat, sizeof(OMX_VIDEO_PARAM_PORTFORMATTYPE));
      if(err!=OMX_ErrorNone) {
        DEBUG(DEB_LEV_ERR, "In %s Parameter Check Error=%x\n",__func__,err);
        break;
      }
      pPort = (omx_base_video_PortType *) omx_video_scheduler_component_Private->ports[portIndex];
      if(portIndex > 1) {
        return OMX_ErrorBadPortIndex;
      }
      if (pVideoPortFormat->eCompressionFormat != OMX_VIDEO_CodingUnused)  {
        //  No compression allowed
        return OMX_ErrorUnsupportedSetting;
      }

      pPort->sVideoParam.xFramerate         = pVideoPortFormat->xFramerate;
      pPort->sVideoParam.eCompressionFormat = pVideoPortFormat->eCompressionFormat;
      pPort->sVideoParam.eColorFormat       = pVideoPortFormat->eColorFormat;
      break;
    case  OMX_IndexParamOtherPortFormat:
      pOtherPortFormat = (OMX_OTHER_PARAM_PORTFORMATTYPE*)ComponentParameterStructure;
      portIndex        = pOtherPortFormat->nPortIndex;
      err = omx_base_component_ParameterSanityCheck(hComponent, portIndex, pOtherPortFormat, sizeof(OMX_OTHER_PARAM_PORTFORMATTYPE));
      if(err!=OMX_ErrorNone) {
        DEBUG(DEB_LEV_ERR, "In %s Parameter Check Error=%x\n",__func__,err);
        break;
      }
      if(portIndex != CLOCKPORT_INDEX) {
        return OMX_ErrorBadPortIndex;
      }
      pClockPort = (omx_base_clock_PortType *) omx_video_scheduler_component_Private->ports[portIndex];

      pClockPort->sOtherParam.eFormat = pOtherPortFormat->eFormat;
      break;

    case OMX_IndexParamStandardComponentRole:
      pComponentRole = (OMX_PARAM_COMPONENTROLETYPE*)ComponentParameterStructure;

      if (omx_video_scheduler_component_Private->state != OMX_StateLoaded && omx_video_scheduler_component_Private->state != OMX_StateWaitForResources) {
        DEBUG(DEB_LEV_ERR, "In %s Incorrect State=%x lineno=%d\n",__func__,omx_video_scheduler_component_Private->state,__LINE__);
        return OMX_ErrorIncorrectStateOperation;
      }

      if ((err = checkHeader(ComponentParameterStructure, sizeof(OMX_PARAM_COMPONENTROLETYPE))) != OMX_ErrorNone) {
        break;
      }

      if (strcmp( (char*) pComponentRole->cRole, VIDEO_SCHEDULER_COMP_ROLE)) {
        return OMX_ErrorBadParameter;
      }
      break;

    default: /*Call the base component function*/
      return omx_base_component_SetParameter(hComponent, nParamIndex, ComponentParameterStructure);
  }
  return err;
}

OMX_ERRORTYPE omx_video_scheduler_component_GetParameter(
  OMX_HANDLETYPE hComponent,
  OMX_INDEXTYPE nParamIndex,
  OMX_PTR ComponentParameterStructure) {

  OMX_VIDEO_PARAM_PORTFORMATTYPE             *pVideoPortFormat;
  OMX_OTHER_PARAM_PORTFORMATTYPE             *pOtherPortFormat;
  OMX_ERRORTYPE                              err = OMX_ErrorNone;
  OMX_COMPONENTTYPE                          *openmaxStandComp = (OMX_COMPONENTTYPE *)hComponent;
  omx_video_scheduler_component_PrivateType* omx_video_scheduler_component_Private = openmaxStandComp->pComponentPrivate;
  omx_base_video_PortType                    *pPort;
  omx_base_clock_PortType                    *pClockPort = (omx_base_clock_PortType *) omx_video_scheduler_component_Private->ports[CLOCKPORT_INDEX];
  OMX_PARAM_COMPONENTROLETYPE                *pComponentRole;

  if (ComponentParameterStructure == NULL) {
    return OMX_ErrorBadParameter;
  }
  DEBUG(DEB_LEV_SIMPLE_SEQ, "   Getting parameter %i\n", nParamIndex);
  /* Check which structure we are being fed and fill its header */
  switch(nParamIndex) {
    case OMX_IndexParamVideoInit:
      if ((err = checkHeader(ComponentParameterStructure, sizeof(OMX_PORT_PARAM_TYPE))) != OMX_ErrorNone) {
        break;
      }
      memcpy(ComponentParameterStructure, &omx_video_scheduler_component_Private->sPortTypesParam[OMX_PortDomainVideo], sizeof(OMX_PORT_PARAM_TYPE));
      break;
    case OMX_IndexParamOtherInit:
      if ((err = checkHeader(ComponentParameterStructure, sizeof(OMX_PORT_PARAM_TYPE))) != OMX_ErrorNone) {
        break;
      }
      memcpy(ComponentParameterStructure, &omx_video_scheduler_component_Private->sPortTypesParam[OMX_PortDomainOther], sizeof(OMX_PORT_PARAM_TYPE));
      break;
    case OMX_IndexParamVideoPortFormat:
      pVideoPortFormat = (OMX_VIDEO_PARAM_PORTFORMATTYPE*)ComponentParameterStructure;
      pPort = (omx_base_video_PortType *) omx_video_scheduler_component_Private->ports[pVideoPortFormat->nPortIndex];
      if ((err = checkHeader(ComponentParameterStructure, sizeof(OMX_VIDEO_PARAM_PORTFORMATTYPE))) != OMX_ErrorNone) {
        break;
      }
      if (pVideoPortFormat->nPortIndex < CLOCKPORT_INDEX) {
        memcpy(pVideoPortFormat, &pPort->sVideoParam, sizeof(OMX_VIDEO_PARAM_PORTFORMATTYPE));
      } else {
        return OMX_ErrorBadPortIndex;
      }
      break;
    case  OMX_IndexParamOtherPortFormat:
      pOtherPortFormat = (OMX_OTHER_PARAM_PORTFORMATTYPE*)ComponentParameterStructure;
      if ((err = checkHeader(ComponentParameterStructure, sizeof(OMX_OTHER_PARAM_PORTFORMATTYPE))) != OMX_ErrorNone) {
        break;
      }
      if (pOtherPortFormat->nPortIndex == CLOCKPORT_INDEX) {
        memcpy(pOtherPortFormat, &pClockPort->sOtherParam, sizeof(OMX_OTHER_PARAM_PORTFORMATTYPE));
      } else {
        return OMX_ErrorBadPortIndex;
      }
      break;

    case OMX_IndexParamStandardComponentRole:
      pComponentRole = (OMX_PARAM_COMPONENTROLETYPE*)ComponentParameterStructure;
      if ((err = checkHeader(ComponentParameterStructure, sizeof(OMX_PARAM_COMPONENTROLETYPE))) != OMX_ErrorNone) {
        break;
      }
      strcpy( (char*) pComponentRole->cRole, VIDEO_SCHEDULER_COMP_ROLE);
      break;

    default: /*Call the base component function*/
      return omx_base_component_GetParameter(hComponent, nParamIndex, ComponentParameterStructure);
  }
  return err;
}

