/**
  src/components/audio_effects/omx_volume_component.c

  OpenMAX volume control component. This component implements a filter that
  controls the volume level of the audio PCM stream.

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

*/

#include <omxcore.h>
#include <omx_base_audio_port.h>
#include <omx_volume_component.h>
#include<OMX_Audio.h>

/* gain value */
#define GAIN_VALUE 100.0f

OMX_ERRORTYPE omx_volume_component_Constructor(OMX_COMPONENTTYPE *openmaxStandComp, OMX_STRING cComponentName) {
	OMX_ERRORTYPE err;
	omx_volume_component_PrivateType* omx_volume_component_Private;
	OMX_U32 i;

	DEBUG(DEB_LEV_FUNCTION_NAME, "In %s\n",__func__);

	RM_RegisterComponent(VOLUME_COMP_NAME, MAX_VOLUME_COMPONENTS);
	if (!openmaxStandComp->pComponentPrivate) {
		//openmaxStandComp->pComponentPrivate = calloc(1, sizeof(omx_volume_component_PrivateType));
		openmaxStandComp->pComponentPrivate = malloc(sizeof(omx_volume_component_PrivateType));
		DEBUG(DEB_LEV_FUNCTION_NAME, "In %s allocated private structure %p for std component %p\n",
				__func__, openmaxStandComp->pComponentPrivate, openmaxStandComp);
		if(openmaxStandComp->pComponentPrivate == NULL) {
			return OMX_ErrorInsufficientResources;
		}
		memset(openmaxStandComp->pComponentPrivate, 0x00, sizeof(omx_volume_component_PrivateType));
	} else {
		DEBUG(DEB_LEV_ERR, "In %s, Error Component %p Already Allocated\n", __func__, openmaxStandComp->pComponentPrivate);
		return OMX_ErrorUndefined;
	}

	omx_volume_component_Private = openmaxStandComp->pComponentPrivate;
	omx_volume_component_Private->ports = NULL;

	/** Calling base filter constructor */
	err = omx_base_filter_Constructor(openmaxStandComp, cComponentName);
	if (err != OMX_ErrorNone) {
			DEBUG(DEB_LEV_ERR, "In %s failed base class constructor\n", __func__);
			return err;
	}

	omx_volume_component_Private->sPortTypesParam[OMX_PortDomainAudio].nStartPortNumber = 0;
	omx_volume_component_Private->sPortTypesParam[OMX_PortDomainAudio].nPorts = 2;

	/** Allocate Ports and call port constructor. */
	if (omx_volume_component_Private->sPortTypesParam[OMX_PortDomainAudio].nPorts && !omx_volume_component_Private->ports) {
		//omx_volume_component_Private->ports = calloc(omx_volume_component_Private->sPortTypesParam[OMX_PortDomainAudio].nPorts, sizeof(omx_base_PortType *));
		omx_volume_component_Private->ports = malloc(omx_volume_component_Private->sPortTypesParam[OMX_PortDomainAudio].nPorts*sizeof(omx_base_PortType *));
		
		if (!omx_volume_component_Private->ports) {
			return OMX_ErrorInsufficientResources;
		}
		
		memset(omx_volume_component_Private->ports, 0x00, omx_volume_component_Private->sPortTypesParam[OMX_PortDomainAudio].nPorts*sizeof(omx_base_PortType *));
		
		for (i=0; i < omx_volume_component_Private->sPortTypesParam[OMX_PortDomainAudio].nPorts; i++) {
			//omx_volume_component_Private->ports[i] = calloc(1, sizeof(omx_base_audio_PortType));
			omx_volume_component_Private->ports[i] = malloc(sizeof(omx_base_audio_PortType));
			if (!omx_volume_component_Private->ports[i]) {
				return OMX_ErrorInsufficientResources;
			}
			memset(omx_volume_component_Private->ports[i], 0x00, sizeof(omx_base_audio_PortType));
		}
	}

	err = base_audio_port_Constructor(openmaxStandComp, &omx_volume_component_Private->ports[0], 0, OMX_TRUE);
	if (err != OMX_ErrorNone) {
		return OMX_ErrorInsufficientResources;
	}
	err = base_audio_port_Constructor(openmaxStandComp, &omx_volume_component_Private->ports[1], 1, OMX_FALSE);
	if (err != OMX_ErrorNone) {
		return OMX_ErrorInsufficientResources;
	}

	/** Domain specific section for the ports. */
	omx_volume_component_Private->ports[OMX_BASE_FILTER_INPUTPORT_INDEX]->sPortParam.nBufferSize = DEFAULT_OUT_BUFFER_SIZE;
	omx_volume_component_Private->ports[OMX_BASE_FILTER_OUTPUTPORT_INDEX]->sPortParam.nBufferSize = DEFAULT_OUT_BUFFER_SIZE;

	omx_volume_component_Private->gain = GAIN_VALUE; //100.0f; // default gain
	omx_volume_component_Private->destructor = omx_volume_component_Destructor;
	openmaxStandComp->SetParameter = omx_volume_component_SetParameter;
	openmaxStandComp->GetParameter = omx_volume_component_GetParameter;
	openmaxStandComp->GetConfig = omx_volume_component_GetConfig;
	openmaxStandComp->SetConfig = omx_volume_component_SetConfig;
	omx_volume_component_Private->BufferMgmtCallback = omx_volume_component_BufferMgmtCallback;

  /* resource management special section */
  omx_volume_component_Private->nqualitylevels = VOLUME_QUALITY_LEVELS;
  omx_volume_component_Private->currentQualityLevel = 1;
  omx_volume_component_Private->multiResourceLevel = malloc(sizeof(multiResourceDescriptor *) * VOLUME_QUALITY_LEVELS);
  for (i = 0; i<VOLUME_QUALITY_LEVELS; i++) {
	  omx_volume_component_Private->multiResourceLevel[i] = malloc(sizeof(multiResourceDescriptor));
	  omx_volume_component_Private->multiResourceLevel[i]->CPUResourceRequested = volumeQualityLevels[i * 2];
	  omx_volume_component_Private->multiResourceLevel[i]->MemoryResourceRequested = volumeQualityLevels[i * 2 + 1];
  }

	DEBUG(DEB_LEV_FUNCTION_NAME, "Out of %s for component %p\n", __func__, openmaxStandComp);
	return OMX_ErrorNone;
}


/** The destructor
  */
OMX_ERRORTYPE omx_volume_component_Destructor(OMX_COMPONENTTYPE *openmaxStandComp) {

	omx_volume_component_PrivateType* omx_volume_component_Private = openmaxStandComp->pComponentPrivate;
	OMX_U32 i;

	/* frees port/s */
	DEBUG(DEB_LEV_FUNCTION_NAME, "In %s for component %p\n", __func__, openmaxStandComp);
	if (omx_volume_component_Private->ports) {
		for (i=0; i < omx_volume_component_Private->sPortTypesParam[OMX_PortDomainAudio].nPorts; i++) {
			if(omx_volume_component_Private->ports[i]) {
				omx_volume_component_Private->ports[i]->PortDestructor(omx_volume_component_Private->ports[i]);
			}
		}
		free(omx_volume_component_Private->ports);
		omx_volume_component_Private->ports=NULL;
	}

	omx_base_filter_Destructor(openmaxStandComp);

	DEBUG(DEB_LEV_FUNCTION_NAME, "Out of %s for component %p\n", __func__, openmaxStandComp);
	return OMX_ErrorNone;
}

/** This function is used to process the input buffer and provide one output buffer
  */
void omx_volume_component_BufferMgmtCallback(OMX_COMPONENTTYPE *openmaxStandComp, OMX_BUFFERHEADERTYPE* pInputBuffer, OMX_BUFFERHEADERTYPE* pOutputBuffer) {
  int i;
  int sampleCount = pInputBuffer->nFilledLen / 2; // signed 16 bit samples assumed
  omx_volume_component_PrivateType* omx_volume_component_Private = openmaxStandComp->pComponentPrivate;

  if(omx_volume_component_Private->gain != GAIN_VALUE) {
    for (i = 0; i < sampleCount; i++) {
      ((OMX_S16*) pOutputBuffer->pBuffer)[i] = (OMX_S16)
              (((OMX_S16*) pInputBuffer->pBuffer)[i] * (omx_volume_component_Private->gain / 100.0f));
    }
  } else {
    memcpy(pOutputBuffer->pBuffer,pInputBuffer->pBuffer,pInputBuffer->nFilledLen);
  }
  pOutputBuffer->nFilledLen = pInputBuffer->nFilledLen;
  pInputBuffer->nFilledLen=0;
}

/** setting configurations */
OMX_ERRORTYPE omx_volume_component_SetConfig(
  OMX_HANDLETYPE hComponent,
  OMX_INDEXTYPE nIndex,
  OMX_PTR pComponentConfigStructure) {

  OMX_AUDIO_CONFIG_VOLUMETYPE* pVolume;
  OMX_COMPONENTTYPE *openmaxStandComp = (OMX_COMPONENTTYPE *)hComponent;
  omx_volume_component_PrivateType* omx_volume_component_Private = openmaxStandComp->pComponentPrivate;
  OMX_ERRORTYPE err = OMX_ErrorNone;

  switch (nIndex) {
    case OMX_IndexConfigAudioVolume :
      pVolume = (OMX_AUDIO_CONFIG_VOLUMETYPE*) pComponentConfigStructure;
      if(pVolume->sVolume.nValue > 100) {
        err = OMX_ErrorBadParameter;
        break;
      }
      omx_volume_component_Private->gain = pVolume->sVolume.nValue;
      err = OMX_ErrorNone;
      break;
    default: // delegate to superclass
      err = omx_base_component_SetConfig(hComponent, nIndex, pComponentConfigStructure);
  }

  return err;
}

OMX_ERRORTYPE omx_volume_component_GetConfig(
  OMX_HANDLETYPE hComponent,
  OMX_INDEXTYPE nIndex,
  OMX_PTR pComponentConfigStructure) {
  OMX_AUDIO_CONFIG_VOLUMETYPE* pVolume;
  OMX_COMPONENTTYPE *openmaxStandComp = (OMX_COMPONENTTYPE *)hComponent;
  omx_volume_component_PrivateType* omx_volume_component_Private = openmaxStandComp->pComponentPrivate;
  OMX_ERRORTYPE err = OMX_ErrorNone;

  switch (nIndex) {
    case OMX_IndexConfigAudioVolume :
      pVolume = (OMX_AUDIO_CONFIG_VOLUMETYPE*) pComponentConfigStructure;
      setHeader(pVolume,sizeof(OMX_AUDIO_CONFIG_VOLUMETYPE));
      pVolume->sVolume.nValue = omx_volume_component_Private->gain;
      pVolume->sVolume.nMin = 0;
      pVolume->sVolume.nMax = 100;
      pVolume->bLinear = OMX_TRUE;
      err = OMX_ErrorNone;
      break;
    default :
      err = omx_base_component_GetConfig(hComponent, nIndex, pComponentConfigStructure);
  }
  return err;
}

OMX_ERRORTYPE omx_volume_component_SetParameter(
  OMX_HANDLETYPE hComponent,
  OMX_INDEXTYPE nParamIndex,
  OMX_PTR ComponentParameterStructure) {

  OMX_ERRORTYPE err = OMX_ErrorNone;
  OMX_AUDIO_PARAM_PORTFORMATTYPE *pAudioPortFormat;
  OMX_PARAM_COMPONENTROLETYPE *pComponentRole;
  OMX_U32 portIndex;
  omx_base_audio_PortType *port;

  /* Check which structure we are being fed and make control its header */
  OMX_COMPONENTTYPE *openmaxStandComp = (OMX_COMPONENTTYPE *)hComponent;
  omx_volume_component_PrivateType* omx_volume_component_Private = openmaxStandComp->pComponentPrivate;
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
      if (portIndex <= 1) {
        port= (omx_base_audio_PortType *)omx_volume_component_Private->ports[portIndex];
        memcpy(&port->sAudioParam, pAudioPortFormat, sizeof(OMX_AUDIO_PARAM_PORTFORMATTYPE));
      } else {
        err = OMX_ErrorBadPortIndex;
      }
      break;
    case OMX_IndexParamStandardComponentRole:
      pComponentRole = (OMX_PARAM_COMPONENTROLETYPE*)ComponentParameterStructure;

      if (omx_volume_component_Private->state != OMX_StateLoaded && omx_volume_component_Private->state != OMX_StateWaitForResources) {
        DEBUG(DEB_LEV_ERR, "In %s Incorrect State=%x lineno=%d\n",__func__,omx_volume_component_Private->state,__LINE__);
        return OMX_ErrorIncorrectStateOperation;
      }

      if ((err = checkHeader(ComponentParameterStructure, sizeof(OMX_PARAM_COMPONENTROLETYPE))) != OMX_ErrorNone) {
        break;
      }

      if (strcmp( (char*) pComponentRole->cRole, VOLUME_COMP_ROLE)) {
        return OMX_ErrorBadParameter;
      }
      break;
    default:
      err = omx_base_component_SetParameter(hComponent, nParamIndex, ComponentParameterStructure);
  }
  return err;
}

OMX_ERRORTYPE omx_volume_component_GetParameter(
  OMX_HANDLETYPE hComponent,
  OMX_INDEXTYPE nParamIndex,
  OMX_PTR ComponentParameterStructure) {

  OMX_AUDIO_PARAM_PORTFORMATTYPE *pAudioPortFormat;
  OMX_AUDIO_PARAM_PCMMODETYPE *pAudioPcmMode;
  OMX_PARAM_COMPONENTROLETYPE *pComponentRole;
  OMX_ERRORTYPE err = OMX_ErrorNone;
  omx_base_audio_PortType *port;
  OMX_COMPONENTTYPE *openmaxStandComp = (OMX_COMPONENTTYPE *)hComponent;
  omx_volume_component_PrivateType* omx_volume_component_Private = openmaxStandComp->pComponentPrivate;
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
      memcpy(ComponentParameterStructure, &omx_volume_component_Private->sPortTypesParam[OMX_PortDomainAudio], sizeof(OMX_PORT_PARAM_TYPE));
      break;
    case OMX_IndexParamAudioPortFormat:
      pAudioPortFormat = (OMX_AUDIO_PARAM_PORTFORMATTYPE*)ComponentParameterStructure;
      if ((err = checkHeader(ComponentParameterStructure, sizeof(OMX_AUDIO_PARAM_PORTFORMATTYPE))) != OMX_ErrorNone) {
        break;
      }
      if (pAudioPortFormat->nPortIndex <= 1) {
        port= (omx_base_audio_PortType *)omx_volume_component_Private->ports[pAudioPortFormat->nPortIndex];
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

      if (pAudioPcmMode->nPortIndex > 1) {
        return OMX_ErrorBadPortIndex;
      }
      pAudioPcmMode->nChannels = 2;
      pAudioPcmMode->eNumData = OMX_NumericalDataSigned;
      pAudioPcmMode->eEndian = OMX_EndianBig;
      pAudioPcmMode->bInterleaved = OMX_TRUE;
      pAudioPcmMode->nBitPerSample = 16;
      pAudioPcmMode->nSamplingRate = 0;
      pAudioPcmMode->ePCMMode = OMX_AUDIO_PCMModeLinear;
      break;
    case OMX_IndexParamStandardComponentRole:
      pComponentRole = (OMX_PARAM_COMPONENTROLETYPE*)ComponentParameterStructure;
      if ((err = checkHeader(ComponentParameterStructure, sizeof(OMX_PARAM_COMPONENTROLETYPE))) != OMX_ErrorNone) {
        break;
      }
      strcpy( (char*) pComponentRole->cRole, VOLUME_COMP_ROLE);
      break;
    default:
      err = omx_base_component_GetParameter(hComponent, nParamIndex, ComponentParameterStructure);
  }
  return err;
}
