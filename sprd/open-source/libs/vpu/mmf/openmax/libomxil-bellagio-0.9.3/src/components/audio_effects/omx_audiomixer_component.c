/**
  src/components/audio_effects/omx_audiomixer_component.c

  OpenMAX audio mixer control component. This component implements a mixer that
  mixes multiple audio PCM streams and produces a single output stream.

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
#include <omx_base_audio_port.h>
#include <omx_audiomixer_component.h>
#include <OMX_Audio.h>



/* Gain value */
#define GAIN_VALUE 100.0f

OMX_ERRORTYPE omx_audio_mixer_component_Constructor(OMX_COMPONENTTYPE *openmaxStandComp, OMX_STRING cComponentName) {
  OMX_ERRORTYPE err = OMX_ErrorNone;
  omx_audio_mixer_component_PrivateType* omx_audio_mixer_component_Private;
  omx_audio_mixer_component_PortType *pPort;//,*inPort1, *outPort;
  OMX_U32 i;

  RM_RegisterComponent(MIXER_COMP_NAME, MAX_MIXER_COMPONENTS);
  if (!openmaxStandComp->pComponentPrivate) {
    DEBUG(DEB_LEV_FUNCTION_NAME, "In %s, allocating component\n",__func__);
    //openmaxStandComp->pComponentPrivate = calloc(1, sizeof(omx_audio_mixer_component_PrivateType));
    openmaxStandComp->pComponentPrivate = malloc(sizeof(omx_audio_mixer_component_PrivateType));
    if(openmaxStandComp->pComponentPrivate == NULL) {
      return OMX_ErrorInsufficientResources;
    }
    memset(openmaxStandComp->pComponentPrivate, 0x00, sizeof(omx_audio_mixer_component_PrivateType));
  } else {
    DEBUG(DEB_LEV_FUNCTION_NAME, "In %s, Error Component %p Already Allocated\n", __func__, openmaxStandComp->pComponentPrivate);
  }

  omx_audio_mixer_component_Private = openmaxStandComp->pComponentPrivate;
  omx_audio_mixer_component_Private->ports = NULL;

  /** Calling base filter constructor */
  err = omx_base_filter_Constructor(openmaxStandComp, cComponentName);

  /*Assuming 4 input and 1 output ports*/
  omx_audio_mixer_component_Private->sPortTypesParam[OMX_PortDomainAudio].nStartPortNumber = 0;
  omx_audio_mixer_component_Private->sPortTypesParam[OMX_PortDomainAudio].nPorts = MAX_PORTS;

  /** Allocate Ports and call port constructor. */
  if (omx_audio_mixer_component_Private->sPortTypesParam[OMX_PortDomainAudio].nPorts && !omx_audio_mixer_component_Private->ports) {
    //omx_audio_mixer_component_Private->ports = calloc(omx_audio_mixer_component_Private->sPortTypesParam[OMX_PortDomainAudio].nPorts, sizeof(omx_base_PortType *));
    omx_audio_mixer_component_Private->ports = malloc(omx_audio_mixer_component_Private->sPortTypesParam[OMX_PortDomainAudio].nPorts*sizeof(omx_base_PortType *));
    if (!omx_audio_mixer_component_Private->ports) {
      return OMX_ErrorInsufficientResources;
    }
    
    memset(omx_audio_mixer_component_Private->ports, 0x00, omx_audio_mixer_component_Private->sPortTypesParam[OMX_PortDomainAudio].nPorts*sizeof(omx_base_PortType *));
    
    for (i=0; i < omx_audio_mixer_component_Private->sPortTypesParam[OMX_PortDomainAudio].nPorts; i++) {
      //omx_audio_mixer_component_Private->ports[i] = calloc(1, sizeof(omx_audio_mixer_component_PortType));
      omx_audio_mixer_component_Private->ports[i] = malloc(sizeof(omx_audio_mixer_component_PortType));
      if (!omx_audio_mixer_component_Private->ports[i]) {
        return OMX_ErrorInsufficientResources;
      }
      memset(omx_audio_mixer_component_Private->ports[i], 0x00, sizeof(omx_audio_mixer_component_PortType));
    }
  }

  /* construct all input ports */
  for(i=0;i<omx_audio_mixer_component_Private->sPortTypesParam[OMX_PortDomainAudio].nPorts-1;i++) {
    base_audio_port_Constructor(openmaxStandComp, &omx_audio_mixer_component_Private->ports[i], i, OMX_TRUE);
  }

  /* construct one output port */
  base_audio_port_Constructor(openmaxStandComp, &omx_audio_mixer_component_Private->ports[omx_audio_mixer_component_Private->sPortTypesParam[OMX_PortDomainAudio].nPorts-1], omx_audio_mixer_component_Private->sPortTypesParam[OMX_PortDomainAudio].nPorts-1, OMX_FALSE);

  /** Domain specific section for the ports. */
  for(i=0;i<omx_audio_mixer_component_Private->sPortTypesParam[OMX_PortDomainAudio].nPorts;i++) {
    pPort = (omx_audio_mixer_component_PortType *) omx_audio_mixer_component_Private->ports[i];

    pPort->sPortParam.nBufferSize = DEFAULT_OUT_BUFFER_SIZE;
    pPort->gain = GAIN_VALUE; //100.0f; // default gain

    setHeader(&pPort->pAudioPcmMode,sizeof(OMX_AUDIO_PARAM_PCMMODETYPE));
    pPort->pAudioPcmMode.nPortIndex = i;
    pPort->pAudioPcmMode.nChannels = 2;
    pPort->pAudioPcmMode.eNumData = OMX_NumericalDataSigned;
    pPort->pAudioPcmMode.eEndian = OMX_EndianBig;
    pPort->pAudioPcmMode.bInterleaved = OMX_TRUE;
    pPort->pAudioPcmMode.nBitPerSample = 16;
    pPort->pAudioPcmMode.nSamplingRate = 44100;
    pPort->pAudioPcmMode.ePCMMode = OMX_AUDIO_PCMModeLinear;

    setHeader(&pPort->sVolume,sizeof(OMX_AUDIO_CONFIG_VOLUMETYPE));
    pPort->sVolume.nPortIndex = i;
    pPort->sVolume.bLinear = OMX_TRUE;           /**< Is the volume to be set in linear (0.100)
                                     or logarithmic scale (mB) */
    pPort->sVolume.sVolume.nValue = (OMX_S32)GAIN_VALUE;
    pPort->sVolume.sVolume.nMin = 0;   /**< minimum for value (i.e. nValue >= nMin) */
    pPort->sVolume.sVolume.nMax = (OMX_S32)GAIN_VALUE;
  }

  omx_audio_mixer_component_Private->destructor = omx_audio_mixer_component_Destructor;
  openmaxStandComp->SetParameter = omx_audio_mixer_component_SetParameter;
  openmaxStandComp->GetParameter = omx_audio_mixer_component_GetParameter;
  openmaxStandComp->GetConfig = omx_audio_mixer_component_GetConfig;
  openmaxStandComp->SetConfig = omx_audio_mixer_component_SetConfig;
  omx_audio_mixer_component_Private->BufferMgmtCallback = omx_audio_mixer_component_BufferMgmtCallback;
  omx_audio_mixer_component_Private->BufferMgmtFunction = omx_audio_mixer_BufferMgmtFunction;

  /* resource management special section */
  omx_audio_mixer_component_Private->nqualitylevels = MIXER_QUALITY_LEVELS;
  omx_audio_mixer_component_Private->currentQualityLevel = 1;
  omx_audio_mixer_component_Private->multiResourceLevel = malloc(sizeof(multiResourceDescriptor *) * MIXER_QUALITY_LEVELS);
  for (i = 0; i<MIXER_QUALITY_LEVELS; i++) {
	  omx_audio_mixer_component_Private->multiResourceLevel[i] = malloc(sizeof(multiResourceDescriptor));
	  omx_audio_mixer_component_Private->multiResourceLevel[i]->CPUResourceRequested = mixerQualityLevels[i * 2];
	  omx_audio_mixer_component_Private->multiResourceLevel[i]->MemoryResourceRequested = mixerQualityLevels[i * 2 + 1];
  }

  return err;
}


/** The destructor
  */
OMX_ERRORTYPE omx_audio_mixer_component_Destructor(OMX_COMPONENTTYPE *openmaxStandComp) {

  omx_audio_mixer_component_PrivateType* omx_audio_mixer_component_Private = openmaxStandComp->pComponentPrivate;
  OMX_U32 i;

  DEBUG(DEB_LEV_FUNCTION_NAME, "In %s\n", __func__);
  /* frees port/s */
  if (omx_audio_mixer_component_Private->ports) {
    for (i=0; i < omx_audio_mixer_component_Private->sPortTypesParam[OMX_PortDomainAudio].nPorts; i++) {
      if(omx_audio_mixer_component_Private->ports[i])
        omx_audio_mixer_component_Private->ports[i]->PortDestructor(omx_audio_mixer_component_Private->ports[i]);
    }
    free(omx_audio_mixer_component_Private->ports);
    omx_audio_mixer_component_Private->ports=NULL;
  }

  omx_base_filter_Destructor(openmaxStandComp);

  DEBUG(DEB_LEV_FUNCTION_NAME, "Out of %s\n", __func__);

  return OMX_ErrorNone;
}

/** This function is used to process the input buffer and provide one output buffer
  */
void omx_audio_mixer_component_BufferMgmtCallback(OMX_COMPONENTTYPE *openmaxStandComp, OMX_BUFFERHEADERTYPE* pInBuffer, OMX_BUFFERHEADERTYPE* pOutBuffer) {
  OMX_S32 denominator=0;
  OMX_U32 i,sampleCount = pInBuffer->nFilledLen / 2; // signed 16 bit samples assumed
  omx_audio_mixer_component_PrivateType* omx_audio_mixer_component_Private = openmaxStandComp->pComponentPrivate;
  omx_audio_mixer_component_PortType* pPort;

  for(i=0;i<omx_audio_mixer_component_Private->sPortTypesParam[OMX_PortDomainAudio].nPorts-1;i++) {
    pPort = (omx_audio_mixer_component_PortType*)omx_audio_mixer_component_Private->ports[i];
    if(PORT_IS_ENABLED(pPort)){
      denominator+=pPort->sVolume.sVolume.nValue;
    }
  }

  pPort = (omx_audio_mixer_component_PortType*)omx_audio_mixer_component_Private->ports[pInBuffer->nInputPortIndex];

  /*Copy the first buffer with appropriate gain*/
  if(pOutBuffer->nFilledLen == 0) {
    memset(pOutBuffer->pBuffer,0,pInBuffer->nFilledLen);

    for (i = 0; i < sampleCount; i++) {
      ((OMX_S16*) pOutBuffer->pBuffer)[i] = (OMX_S16)
              ((((OMX_S16*) pInBuffer->pBuffer)[i] * pPort->sVolume.sVolume.nValue ) / denominator);
    }
  } else { // For the second buffer add with the first buffer with gain
    for (i = 0; i < sampleCount; i++) {
      ((OMX_S16*) pOutBuffer->pBuffer)[i] += (OMX_S16)
                ((((OMX_S16*) pInBuffer->pBuffer)[i] * pPort->sVolume.sVolume.nValue ) / denominator);
    }
  }

  pOutBuffer->nFilledLen = pInBuffer->nFilledLen;
  pInBuffer->nFilledLen=0;
}

/** setting configurations */
OMX_ERRORTYPE omx_audio_mixer_component_SetConfig(
  OMX_HANDLETYPE hComponent,
  OMX_INDEXTYPE nIndex,
  OMX_PTR pComponentConfigStructure) {

  OMX_AUDIO_CONFIG_VOLUMETYPE* pVolume;
  OMX_COMPONENTTYPE *openmaxStandComp = (OMX_COMPONENTTYPE *)hComponent;
  omx_audio_mixer_component_PrivateType* omx_audio_mixer_component_Private = openmaxStandComp->pComponentPrivate;
  omx_audio_mixer_component_PortType * pPort;
  OMX_ERRORTYPE err = OMX_ErrorNone;

  switch (nIndex) {
    case OMX_IndexConfigAudioVolume :
      pVolume = (OMX_AUDIO_CONFIG_VOLUMETYPE*) pComponentConfigStructure;
      if(pVolume->sVolume.nValue > 100) {
        err =  OMX_ErrorBadParameter;
        break;
      }

      if (pVolume->nPortIndex <= omx_audio_mixer_component_Private->sPortTypesParam[OMX_PortDomainAudio].nPorts) {
        pPort= (omx_audio_mixer_component_PortType *)omx_audio_mixer_component_Private->ports[pVolume->nPortIndex];
        DEBUG(DEB_LEV_SIMPLE_SEQ, "Port %i Gain=%d\n",(int)pVolume->nPortIndex,(int)pVolume->sVolume.nValue);
        memcpy(&pPort->sVolume, pVolume, sizeof(OMX_AUDIO_CONFIG_VOLUMETYPE));
      } else {
        err = OMX_ErrorBadPortIndex;
      }
      break;
    default: // delegate to superclass
      err = omx_base_component_SetConfig(hComponent, nIndex, pComponentConfigStructure);
  }
  return err;
}

OMX_ERRORTYPE omx_audio_mixer_component_GetConfig(
  OMX_HANDLETYPE hComponent,
  OMX_INDEXTYPE nIndex,
  OMX_PTR pComponentConfigStructure) {
  OMX_AUDIO_CONFIG_VOLUMETYPE           *pVolume;
  OMX_COMPONENTTYPE *openmaxStandComp = (OMX_COMPONENTTYPE *)hComponent;
  omx_audio_mixer_component_PrivateType *omx_audio_mixer_component_Private = openmaxStandComp->pComponentPrivate;
  omx_audio_mixer_component_PortType    *pPort;
  OMX_ERRORTYPE err = OMX_ErrorNone;

  switch (nIndex) {
    case OMX_IndexConfigAudioVolume :
      pVolume = (OMX_AUDIO_CONFIG_VOLUMETYPE*) pComponentConfigStructure;
      if (pVolume->nPortIndex <= omx_audio_mixer_component_Private->sPortTypesParam[OMX_PortDomainAudio].nPorts) {
        pPort= (omx_audio_mixer_component_PortType *)omx_audio_mixer_component_Private->ports[pVolume->nPortIndex];
        memcpy(pVolume,&pPort->sVolume,sizeof(OMX_AUDIO_CONFIG_VOLUMETYPE));
      } else {
        err = OMX_ErrorBadPortIndex;
      }
      break;
    default :
      err = omx_base_component_GetConfig(hComponent, nIndex, pComponentConfigStructure);
  }
  return err;
}

OMX_ERRORTYPE omx_audio_mixer_component_SetParameter(
  OMX_HANDLETYPE hComponent,
  OMX_INDEXTYPE nParamIndex,
  OMX_PTR ComponentParameterStructure) {

  OMX_ERRORTYPE                   err = OMX_ErrorNone;
  OMX_AUDIO_PARAM_PORTFORMATTYPE  *pAudioPortFormat;
  OMX_PARAM_COMPONENTROLETYPE     *pComponentRole;
  OMX_U32                         portIndex;
  omx_audio_mixer_component_PortType *port;

  /* Check which structure we are being fed and make control its header */
  OMX_COMPONENTTYPE *openmaxStandComp = (OMX_COMPONENTTYPE *)hComponent;
  omx_audio_mixer_component_PrivateType* omx_audio_mixer_component_Private = openmaxStandComp->pComponentPrivate;
  if (ComponentParameterStructure == NULL) {
    return OMX_ErrorBadParameter;
  }

  DEBUG(DEB_LEV_SIMPLE_SEQ, "   Setting parameter %i\n", nParamIndex);
  switch(nParamIndex) {
    case OMX_IndexParamAudioPortFormat:
      pAudioPortFormat = (OMX_AUDIO_PARAM_PORTFORMATTYPE*)ComponentParameterStructure;
      portIndex = pAudioPortFormat->nPortIndex;
      err = omx_base_component_ParameterSanityCheck(hComponent, portIndex, pAudioPortFormat, sizeof(OMX_AUDIO_PARAM_PORTFORMATTYPE));
      if(err!=OMX_ErrorNone) {
        DEBUG(DEB_LEV_ERR, "In %s Parameter Check Error=%x\n",__func__,err);
        break;
      }
      if (portIndex <= omx_audio_mixer_component_Private->sPortTypesParam[OMX_PortDomainAudio].nPorts) {
        port= (omx_audio_mixer_component_PortType *)omx_audio_mixer_component_Private->ports[portIndex];
        memcpy(&port->sAudioParam, pAudioPortFormat, sizeof(OMX_AUDIO_PARAM_PORTFORMATTYPE));
      } else {
        err = OMX_ErrorBadPortIndex;
      }
      break;
    case OMX_IndexParamStandardComponentRole:
      pComponentRole = (OMX_PARAM_COMPONENTROLETYPE*)ComponentParameterStructure;

      if (omx_audio_mixer_component_Private->state != OMX_StateLoaded && omx_audio_mixer_component_Private->state != OMX_StateWaitForResources) {
        DEBUG(DEB_LEV_ERR, "In %s Incorrect State=%x lineno=%d\n",__func__,omx_audio_mixer_component_Private->state,__LINE__);
        return OMX_ErrorIncorrectStateOperation;
      }

      if ((err = checkHeader(ComponentParameterStructure, sizeof(OMX_PARAM_COMPONENTROLETYPE))) != OMX_ErrorNone) {
        break;
      }

      if (strcmp( (char*) pComponentRole->cRole, MIXER_COMP_ROLE)) {
        return OMX_ErrorBadParameter;
      }
      break;
    default:
      err = omx_base_component_SetParameter(hComponent, nParamIndex, ComponentParameterStructure);
  }
  return err;
}

OMX_ERRORTYPE omx_audio_mixer_component_GetParameter(
  OMX_HANDLETYPE hComponent,
  OMX_INDEXTYPE nParamIndex,
  OMX_PTR ComponentParameterStructure) {

  OMX_AUDIO_PARAM_PORTFORMATTYPE  *pAudioPortFormat;
  OMX_AUDIO_PARAM_PCMMODETYPE     *pAudioPcmMode;
  OMX_PARAM_COMPONENTROLETYPE     *pComponentRole;
  OMX_ERRORTYPE                   err = OMX_ErrorNone;
  omx_audio_mixer_component_PortType    *port;
  OMX_COMPONENTTYPE                     *openmaxStandComp = (OMX_COMPONENTTYPE *)hComponent;
  omx_audio_mixer_component_PrivateType *omx_audio_mixer_component_Private = openmaxStandComp->pComponentPrivate;
  if (ComponentParameterStructure == NULL) {
    return OMX_ErrorBadParameter;
  }
  DEBUG(DEB_LEV_SIMPLE_SEQ, "   Getting parameter %i\n", nParamIndex);
  /* Check which structure we are being fed and fill its header */
  switch(nParamIndex) {
    case OMX_IndexParamAudioInit:
      if ((err = checkHeader(ComponentParameterStructure, sizeof(OMX_PORT_PARAM_TYPE))) != OMX_ErrorNone) {
        break;
      }
      memcpy(ComponentParameterStructure, &omx_audio_mixer_component_Private->sPortTypesParam[OMX_PortDomainAudio], sizeof(OMX_PORT_PARAM_TYPE));
      break;
    case OMX_IndexParamAudioPortFormat:
      pAudioPortFormat = (OMX_AUDIO_PARAM_PORTFORMATTYPE*)ComponentParameterStructure;
      if ((err = checkHeader(ComponentParameterStructure, sizeof(OMX_AUDIO_PARAM_PORTFORMATTYPE))) != OMX_ErrorNone) {
        break;
      }
      if (pAudioPortFormat->nPortIndex <= omx_audio_mixer_component_Private->sPortTypesParam[OMX_PortDomainAudio].nPorts) {
        port= (omx_audio_mixer_component_PortType *)omx_audio_mixer_component_Private->ports[pAudioPortFormat->nPortIndex];
        memcpy(pAudioPortFormat, &port->sAudioParam, sizeof(OMX_AUDIO_PARAM_PORTFORMATTYPE));
      } else {
        err = OMX_ErrorBadPortIndex;
      }
      break;
    case OMX_IndexParamAudioPcm:
      pAudioPcmMode = (OMX_AUDIO_PARAM_PCMMODETYPE*)ComponentParameterStructure;
      if ((err = checkHeader(ComponentParameterStructure, sizeof(OMX_AUDIO_PARAM_PCMMODETYPE))) != OMX_ErrorNone) {
        break;
      }

      if (pAudioPcmMode->nPortIndex <= omx_audio_mixer_component_Private->sPortTypesParam[OMX_PortDomainAudio].nPorts) {
        port= (omx_audio_mixer_component_PortType *)omx_audio_mixer_component_Private->ports[pAudioPcmMode->nPortIndex];
        memcpy(pAudioPcmMode, &port->pAudioPcmMode, sizeof(OMX_AUDIO_PARAM_PCMMODETYPE));
      } else {
        err = OMX_ErrorBadPortIndex;
      }
      break;
    case OMX_IndexParamStandardComponentRole:
      pComponentRole = (OMX_PARAM_COMPONENTROLETYPE*)ComponentParameterStructure;
      if ((err = checkHeader(ComponentParameterStructure, sizeof(OMX_PARAM_COMPONENTROLETYPE))) != OMX_ErrorNone) {
        break;
      }
      strcpy( (char*) pComponentRole->cRole, MIXER_COMP_ROLE);
      break;
    default:
      err = omx_base_component_GetParameter(hComponent, nParamIndex, ComponentParameterStructure);
  }
  return err;
}

int checkAnyPortBeingFlushed(omx_audio_mixer_component_PrivateType* omx_audio_mixer_component_Private) {
  omx_base_PortType *pPort;
  int ret = OMX_FALSE,i;

  if(omx_audio_mixer_component_Private->state == OMX_StateLoaded ||
     omx_audio_mixer_component_Private->state == OMX_StateInvalid) {
    return 0;
  }

  pthread_mutex_lock(&omx_audio_mixer_component_Private->flush_mutex);
  for (i=0; i < omx_audio_mixer_component_Private->sPortTypesParam[OMX_PortDomainAudio].nPorts; i++) {
    pPort = omx_audio_mixer_component_Private->ports[i];
    if(PORT_IS_BEING_FLUSHED(pPort)) {
      ret = OMX_TRUE;
      break;
    }
  }
  pthread_mutex_unlock(&omx_audio_mixer_component_Private->flush_mutex);

  return ret;
}

/** This is the central function for component processing,overridden for audio mixer. It
  * is executed in a separate thread, is synchronized with
  * semaphores at each port, those are released each time a new buffer
  * is available on the given port.
  */
void* omx_audio_mixer_BufferMgmtFunction (void* param) {
  OMX_COMPONENTTYPE* openmaxStandComp = (OMX_COMPONENTTYPE*)param;
  omx_audio_mixer_component_PrivateType* omx_audio_mixer_component_Private = (omx_audio_mixer_component_PrivateType*)openmaxStandComp->pComponentPrivate;

  omx_base_PortType *pPort[MAX_PORTS];
  tsem_t* pSem[MAX_PORTS];
  queue_t* pQueue[MAX_PORTS];
  OMX_BUFFERHEADERTYPE* pBuffer[MAX_PORTS];
  OMX_BOOL isBufferNeeded[MAX_PORTS];
  OMX_COMPONENTTYPE* target_component;
  OMX_U32 nOutputPortIndex,i;

  for(i=0;i<omx_audio_mixer_component_Private->sPortTypesParam[OMX_PortDomainAudio].nPorts;i++){
    pPort[i] = omx_audio_mixer_component_Private->ports[i];
    pSem[i] = pPort[i]->pBufferSem;
    pQueue[i] = pPort[i]->pBufferQueue;
    pBuffer[i] = NULL;
    isBufferNeeded[i] = OMX_TRUE;
  }

  nOutputPortIndex = omx_audio_mixer_component_Private->sPortTypesParam[OMX_PortDomainAudio].nPorts - 1;


  DEBUG(DEB_LEV_FUNCTION_NAME, "In %s\n", __func__);
  while(omx_audio_mixer_component_Private->state == OMX_StateIdle || omx_audio_mixer_component_Private->state == OMX_StateExecuting ||  omx_audio_mixer_component_Private->state == OMX_StatePause ||
    omx_audio_mixer_component_Private->transientState == OMX_TransStateLoadedToIdle) {

    /*Wait till the ports are being flushed*/
    while( checkAnyPortBeingFlushed(omx_audio_mixer_component_Private) ) {

      DEBUG(DEB_LEV_FULL_SEQ, "In %s 1 signalling flush all cond iF=%d,oF=%d iSemVal=%d,oSemval=%d\n",
        __func__,isBufferNeeded[0],isBufferNeeded[nOutputPortIndex],pSem[0]->semval,pSem[nOutputPortIndex]->semval);

      for(i=0;i<omx_audio_mixer_component_Private->sPortTypesParam[OMX_PortDomainAudio].nPorts;i++){
        if(isBufferNeeded[i]==OMX_FALSE && PORT_IS_BEING_FLUSHED(pPort[i])) {
          pPort[i]->ReturnBufferFunction(pPort[i],pBuffer[i]);
          pBuffer[i]=NULL;
          isBufferNeeded[i]=OMX_TRUE;
          DEBUG(DEB_LEV_FULL_SEQ, "Ports are flushing,so returning buffer %i\n",(int)i);
        }
      }

      DEBUG(DEB_LEV_FULL_SEQ, "In %s 2 signalling flush all cond iF=%d,oF=%d iSemVal=%d,oSemval=%d\n",
        __func__,isBufferNeeded[0],isBufferNeeded[nOutputPortIndex],pSem[0]->semval,pSem[nOutputPortIndex]->semval);

      tsem_up(omx_audio_mixer_component_Private->flush_all_condition);
      tsem_down(omx_audio_mixer_component_Private->flush_condition);
    }

    if(omx_audio_mixer_component_Private->state == OMX_StateLoaded || omx_audio_mixer_component_Private->state == OMX_StateInvalid) {
      DEBUG(DEB_LEV_SIMPLE_SEQ, "In %s Buffer Management Thread is exiting\n",__func__);
      break;
    }

    /*No buffer to process. So wait here*/
    for(i=0;i<omx_audio_mixer_component_Private->sPortTypesParam[OMX_PortDomainAudio].nPorts;i++){
      if((isBufferNeeded[i]==OMX_TRUE && pSem[i]->semval==0) &&
        (omx_audio_mixer_component_Private->state != OMX_StateLoaded && omx_audio_mixer_component_Private->state != OMX_StateInvalid) &&
        PORT_IS_ENABLED(pPort[i]) && !PORT_IS_BEING_FLUSHED(pPort[i])) {
        //Signalled from EmptyThisBuffer or FillThisBuffer or some thing else
        DEBUG(DEB_LEV_FULL_SEQ, "Waiting for next input/output buffer\n");
        tsem_down(omx_audio_mixer_component_Private->bMgmtSem);

      }
      /*Don't wait for buffers, if any port is flushing*/
      if(checkAnyPortBeingFlushed(omx_audio_mixer_component_Private)) {
        break;
      }
      if(omx_audio_mixer_component_Private->state == OMX_StateLoaded || omx_audio_mixer_component_Private->state == OMX_StateInvalid) {
        DEBUG(DEB_LEV_SIMPLE_SEQ, "In %s Buffer Management Thread is exiting\n",__func__);
        break;
      }
    }

    for(i=0;i<omx_audio_mixer_component_Private->sPortTypesParam[OMX_PortDomainAudio].nPorts;i++){
      DEBUG(DEB_LEV_SIMPLE_SEQ, "Waiting for buffer %i semval=%d \n",(int)i,pSem[i]->semval);
      if(pSem[i]->semval>0 && isBufferNeeded[i]==OMX_TRUE && PORT_IS_ENABLED(pPort[i])) {
        tsem_down(pSem[i]);
        if(pQueue[i]->nelem>0){
          isBufferNeeded[i]=OMX_FALSE;
          pBuffer[i] = dequeue(pQueue[i]);
          if(pBuffer[i] == NULL){
            DEBUG(DEB_LEV_ERR, "Had NULL input buffer!!\n");
            break;
          }
        }
      }
    }

    if(isBufferNeeded[nOutputPortIndex]==OMX_FALSE) {

      if(omx_audio_mixer_component_Private->pMark.hMarkTargetComponent != NULL){
        pBuffer[nOutputPortIndex]->hMarkTargetComponent = omx_audio_mixer_component_Private->pMark.hMarkTargetComponent;
        pBuffer[nOutputPortIndex]->pMarkData            = omx_audio_mixer_component_Private->pMark.pMarkData;
        omx_audio_mixer_component_Private->pMark.hMarkTargetComponent = NULL;
        omx_audio_mixer_component_Private->pMark.pMarkData            = NULL;
      }
      for(i=0;i<omx_audio_mixer_component_Private->sPortTypesParam[OMX_PortDomainAudio].nPorts-1;i++){
        if(isBufferNeeded[i]==OMX_FALSE && PORT_IS_ENABLED(pPort[i])) {

          if(isBufferNeeded[i]==OMX_FALSE) {
            target_component=(OMX_COMPONENTTYPE*)pBuffer[i]->hMarkTargetComponent;
            if(target_component==(OMX_COMPONENTTYPE *)openmaxStandComp) {
              /*Clear the mark and generate an event*/
              (*(omx_audio_mixer_component_Private->callbacks->EventHandler))
                (openmaxStandComp,
                omx_audio_mixer_component_Private->callbackData,
                OMX_EventMark, /* The command was completed */
                1, /* The commands was a OMX_CommandStateSet */
                0, /* The state has been changed in message->messageParam2 */
                pBuffer[i]->pMarkData);
            } else if(pBuffer[i]->hMarkTargetComponent!=NULL){
              /*If this is not the target component then pass the mark*/
              pBuffer[nOutputPortIndex]->hMarkTargetComponent  = pBuffer[i]->hMarkTargetComponent;
              pBuffer[nOutputPortIndex]->pMarkData = pBuffer[i]->pMarkData;
              pBuffer[i]->pMarkData=NULL;
            }
            pBuffer[nOutputPortIndex]->nTimeStamp = pBuffer[i]->nTimeStamp;
          }

          if((pBuffer[i]->nFlags & OMX_BUFFERFLAG_EOS) == OMX_BUFFERFLAG_EOS && pBuffer[i]->nFilledLen==0) {
            DEBUG(DEB_LEV_FULL_SEQ, "Detected EOS flags in input buffer %p of %i filled len=%d\n", pBuffer[i], (int)i, (int)pBuffer[i]->nFilledLen);
            pBuffer[nOutputPortIndex]->nFlags = pBuffer[i]->nFlags;
            pBuffer[i]->nFlags=0;
            (*(omx_audio_mixer_component_Private->callbacks->EventHandler))
              (openmaxStandComp,
              omx_audio_mixer_component_Private->callbackData,
              OMX_EventBufferFlag, /* The command was completed */
              nOutputPortIndex, /* The commands was a OMX_CommandStateSet */
              pBuffer[nOutputPortIndex]->nFlags, /* The state has been changed in message->messageParam2 */
              NULL);
          }

          //TBD: To be verified
          if(omx_audio_mixer_component_Private->state == OMX_StateExecuting)  {
            if (omx_audio_mixer_component_Private->BufferMgmtCallback && pBuffer[i]->nFilledLen != 0) {
              (*(omx_audio_mixer_component_Private->BufferMgmtCallback))(openmaxStandComp, pBuffer[i], pBuffer[nOutputPortIndex]);
            } else {
              /*It no buffer management call back the explicitly consume input buffer*/
              pBuffer[i]->nFilledLen = 0;
            }
          } else {
            DEBUG(DEB_LEV_ERR, "In %s Received Buffer in non-Executing State(%x)\n", __func__, (int)omx_audio_mixer_component_Private->state);
            if(OMX_TransStateExecutingToIdle == omx_audio_mixer_component_Private->transientState ||
			   OMX_TransStatePauseToIdle == omx_audio_mixer_component_Private->transientState) {
              pBuffer[i]->nFilledLen = 0;
            }
          }

          /*Input Buffer has been completely consumed. So, get new input buffer*/
          if(pBuffer[i]->nFilledLen==0) {
            isBufferNeeded[i] = OMX_TRUE;
          }
        }
      }

      if(omx_audio_mixer_component_Private->state==OMX_StatePause &&
        !(checkAnyPortBeingFlushed(omx_audio_mixer_component_Private))) {
        /*Waiting at paused state*/
        tsem_wait(omx_audio_mixer_component_Private->bStateSem);
      }

      /*If EOS and Input buffer Filled Len Zero then Return output buffer immediately*/
      if(pBuffer[nOutputPortIndex]->nFilledLen!=0 || (pBuffer[nOutputPortIndex]->nFlags & OMX_BUFFERFLAG_EOS) == OMX_BUFFERFLAG_EOS){
        DEBUG(DEB_LEV_SIMPLE_SEQ, "Returning output buffer \n");
        pPort[nOutputPortIndex]->ReturnBufferFunction(pPort[nOutputPortIndex],pBuffer[nOutputPortIndex]);
        pBuffer[nOutputPortIndex]=NULL;
        isBufferNeeded[nOutputPortIndex]=OMX_TRUE;
      }
    }

    DEBUG(DEB_LEV_FULL_SEQ, "Input buffer arrived\n");

    if(omx_audio_mixer_component_Private->state==OMX_StatePause &&
      !(checkAnyPortBeingFlushed(omx_audio_mixer_component_Private))) {
      /*Waiting at paused state*/
      tsem_wait(omx_audio_mixer_component_Private->bStateSem);
    }

    /*Input Buffer has been completely consumed. So, return input buffer*/
    for(i=0;i<omx_audio_mixer_component_Private->sPortTypesParam[OMX_PortDomainAudio].nPorts-1;i++){
      if(isBufferNeeded[i] == OMX_TRUE && pBuffer[i]!=NULL && PORT_IS_ENABLED(pPort[i])) {
        pPort[i]->ReturnBufferFunction(pPort[i],pBuffer[i]);
        pBuffer[i]=NULL;
      }
    }
  }
  DEBUG(DEB_LEV_SIMPLE_SEQ,"Exiting Buffer Management Thread\n");
  return NULL;
}

