/**
  test/components/parser/omxparsertest.c

  Test application that uses five OpenMAX components, a parser3gp, an audio decoder,
  a video deocder, an alsa sink and a video sink. The application reads a 3gp stream
  from a file, parses the audio and video content to the respective audio and video decoders.
  And sends the decoded data to respective audio and video sink components. Only components
  based on ffmpeg library are supported.
  The video formats supported are:
    MPEG4 (ffmpeg)
    H264  (ffmpeg)
  The audio formats supported are:
    mp3 (ffmpeg)
    aac (ffmpeg)

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

#include "omxparsertest.h"
#define SUPPORT_AUDIO
#define MPEG4_TYPE_SEL            1
#define AVC_TYPE_SEL              2
#define VIDEO_COMPONENT_NAME_BASE "OMX.vpu.video_decoder"
#define AUDIO_COMPONENT_NAME_BASE "OMX.st.audio_decoder"
#define VIDEO_BASE_ROLE           "video_decoder.avc"
#define AUDIO_BASE_ROLE           "audio_decoder.mp3"
#define VIDEO_DEC_MPEG4_ROLE      "video_decoder.mpeg4"
#define VIDEO_DEC_H264_ROLE       "video_decoder.avc"
#define VIDEO_DEC_VP8_ROLE       "video_decoder.vp8"
#define VIDEO_DEC_RV_ROLE       "video_decoder.rv"
#define VIDEO_DEC_WMV_ROLE       "video_decoder.wmv"
#define AUDIO_DEC_MP3_ROLE        "audio_decoder.mp3"
#define AUDIO_DEC_AAC_ROLE        "audio_decoder.aac"
#define AUDIO_DEC_OGG_ROLE      "audio_decoder.ogg"
#define AUDIO_DEC_RA_ROLE      "audio_decoder.ra"
#define AUDIO_DEC_WMA_ROLE      "audio_decoder.wma"
#define VIDEO_SINK_FRAME_BUFFER   "OMX.st.fbdev.fbdev_sink"
#define VIDEO_SINK_X_VIDEO        "OMX.st.video.xvideo_sink"
#define COLOR_CONV                "OMX.st.video_colorconv.ffmpeg"
#define AUDIO_SINK                "OMX.st.alsa.alsasink"
#define AUDIO_EFFECT              "OMX.st.volume.component"
#define PARSER_3GP                "OMX.st.parser.3gp"
#define VIDEO_SCHD		  "OMX.st.video.scheduler"
#define CLOCK_SRC                 "OMX.st.clocksrc"
#define COMPONENT_NAME_BASE_LEN   20
#define extradata_size            1024
#define VIDEO_PORT_INDEX          0  /* video port index on clock component */
#define AUDIO_PORT_INDEX          1  /* audio port index on clock component */
#define PARSER_PORT_INDEX         2  /* parser port index on clock component */
#define AUDIO_SINK_CLK_PORT_INDEX   1  /* clock port index on Audio sink component */
#define VID_SCH_CLK_PORT_INDEX  2  /* clock port index on video scheduler component */

OMX_COLOR_FORMATTYPE COLOR_CONV_OUT_RGB_FORMAT = OMX_COLOR_Format24bitRGB888;

OMX_BUFFERHEADERTYPE *outBufferParseVideo[2], *outBufferParseAudio[2];
OMX_BUFFERHEADERTYPE *inBufferVideoDec[2], *outBufferVideoDec[2];
OMX_BUFFERHEADERTYPE *inBufferAudioDec[2], *outBufferAudioDec[2];
/** used with color converter if selected */
OMX_BUFFERHEADERTYPE *inBufferColorconv[2], *outBufferColorconv[2];
OMX_BUFFERHEADERTYPE *inBufferVideoSchd[2], *outBufferVideoSchd[2];
OMX_BUFFERHEADERTYPE *inBufferSinkVideo[2];
/* used with volume control if selected */
OMX_BUFFERHEADERTYPE *inBufferVolume[2], *outBufferVolume[2];
OMX_BUFFERHEADERTYPE *inBufferSinkAudio[2];
int buffer_in_size = BUFFER_IN_SIZE;
int buffer_out_size = BUFFER_OUT_SIZE;
static OMX_BOOL bEOS = OMX_FALSE;

static OMX_PARAM_PORTDEFINITIONTYPE paramPortVideo, paramPortAudio;
OMX_PARAM_PORTDEFINITIONTYPE decparamPortVideo;
OMX_PARAM_PORTDEFINITIONTYPE decparamPortAudio;

OMX_CALLBACKTYPE videodeccallbacks = {
    .EventHandler = videodecEventHandler,
    .EmptyBufferDone = videodecEmptyBufferDone,
    .FillBufferDone = videodecFillBufferDone
};

OMX_CALLBACKTYPE colorconv_callbacks = {
    .EventHandler = colorconvEventHandler,
    .EmptyBufferDone = colorconvEmptyBufferDone,
    .FillBufferDone = colorconvFillBufferDone
};

OMX_CALLBACKTYPE videoschd_callbacks = {
    .EventHandler = videoschdEventHandler,
    .EmptyBufferDone = videoschdEmptyBufferDone,
    .FillBufferDone = videoschdFillBufferDone
};

OMX_CALLBACKTYPE fbdev_sink_callbacks = {
    .EventHandler = fb_sinkEventHandler,
    .EmptyBufferDone = fb_sinkEmptyBufferDone,
    .FillBufferDone = NULL
};

OMX_CALLBACKTYPE parser3gpcallbacks = {
    .EventHandler = parser3gpEventHandler,
    .EmptyBufferDone = NULL,
    .FillBufferDone = parser3gpFillBufferDone
};

OMX_CALLBACKTYPE clocksrccallbacks = {
    .EventHandler = clocksrcEventHandler,
    .EmptyBufferDone = NULL,
    .FillBufferDone = clocksrcFillBufferDone
};

OMX_CALLBACKTYPE audiodeccallbacks = {
    .EventHandler = audiodecEventHandler,
    .EmptyBufferDone = audiodecEmptyBufferDone,
    .FillBufferDone = audiodecFillBufferDone
};

OMX_CALLBACKTYPE audiosinkcallbacks = {
    .EventHandler = audiosinkEventHandler,
    .EmptyBufferDone = audiosinkEmptyBufferDone,
    .FillBufferDone = NULL
};

OMX_CALLBACKTYPE volumecallbacks = {
    .EventHandler = volumeEventHandler,
    .EmptyBufferDone = volumeEmptyBufferDone,
    .FillBufferDone = volumeFillBufferDone
};

OMX_U32 out_width = 0, new_out_width = 0;
OMX_U32 out_height = 0, new_out_height = 0;

appPrivateType* appPriv;

char *input_file, *output_file_audio, *output_file_video;

FILE *outfileAudio, *outfileVideo;

int flagIsAudioOutputFileExpected; /* to write the audio output to a file */
int flagIsVideoOutputFileExpected; /* to write the video output to a file */
int flagDecodedOutputReceived;
int flagInputReceived;
int flagIsDisplayRequested; /* If Display is ON - volume, color & video scheduler components are chosen by default */
int flagSetupTunnel;
int flagAVsync; /* to select the AVsync option 1 = AV sync ON, clock component selected, 0 = no clock component selected*/
int flagIsXVideoSinkRequested; /* requested X-video sink*/

static void setHeader(OMX_PTR header, OMX_U32 size)
{
    OMX_VERSIONTYPE* ver = (OMX_VERSIONTYPE*) (header + sizeof (OMX_U32));
    *((OMX_U32*) header) = size;

    ver->s.nVersionMajor = VERSIONMAJOR;
    ver->s.nVersionMinor = VERSIONMINOR;
    ver->s.nRevision = VERSIONREVISION;
    ver->s.nStep = VERSIONSTEP;
}

/** this function sets the audio deocder, volume and audio sink port characteristics
 * based on the parser3gp output port settings
 */
int SetPortParametersAudio()
{
    OMX_ERRORTYPE err = OMX_ErrorNone;

    // getting port parameters from parser3gp component //
    paramPortAudio.nPortIndex = AUDIO_PORT_INDEX; //audio port of the parser3gp
    setHeader(&paramPortAudio, sizeof (OMX_PARAM_PORTDEFINITIONTYPE));
    err = OMX_GetParameter(appPriv->parser3gphandle, OMX_IndexParamPortDefinition, &paramPortAudio);
    if (err != OMX_ErrorNone)
    {
        DEBUG(DEB_LEV_ERR, "\n error in parser3gp port settings get\n");
        exit(1);
    }

    // setting the port parameters of audio decoder
    // input port settings
    decparamPortAudio.nPortIndex = 0; /* input port of the audio decoder */
    setHeader(&decparamPortAudio, sizeof (OMX_PARAM_PORTDEFINITIONTYPE));
    err = OMX_GetParameter(appPriv->audiodechandle, OMX_IndexParamPortDefinition, &decparamPortAudio);
    if (err != OMX_ErrorNone)
    {
        DEBUG(DEB_LEV_ERR, "\n error in audiodechandle settings get\n");
        exit(1);
    }

    decparamPortAudio.format.audio.eEncoding = paramPortAudio.format.audio.eEncoding;
    err = OMX_SetParameter(appPriv->audiodechandle, OMX_IndexParamPortDefinition, &decparamPortAudio);
    if (err != OMX_ErrorNone)
    {
        DEBUG(DEB_LEV_ERR, "Error %08x setting audiodec input port param --- \n", err);
        exit(1);
    }

    // output port settings
    decparamPortAudio.nPortIndex = 1;
    setHeader(&decparamPortAudio, sizeof (OMX_PARAM_PORTDEFINITIONTYPE));
    err = OMX_GetParameter(appPriv->audiodechandle, OMX_IndexParamPortDefinition, &decparamPortAudio);
    if (err != OMX_ErrorNone)
    {
        DEBUG(DEB_LEV_ERR, "\n error in audiodechandle settings get\n");
        exit(1);
    }

    decparamPortAudio.format.audio.eEncoding = paramPortAudio.format.audio.eEncoding;
    err = OMX_SetParameter(appPriv->audiodechandle, OMX_IndexParamPortDefinition, &decparamPortAudio);
    if (err != OMX_ErrorNone)
    {
        DEBUG(DEB_LEV_ERR, "Error %08x setting audiodec output port param --- \n", err);
        exit(1);
    }

    return err;
}

/** this function sets the video deocder, color converter and video sink port characteristics
 * based on the parser3gp output port settings
 */
int SetPortParametersVideo()
{
    OMX_ERRORTYPE err = OMX_ErrorNone;
    OMX_PARAM_PORTDEFINITIONTYPE omx_colorconvPortDefinition;

    // getting port parameters from parser3gp component //
    paramPortVideo.nPortIndex = VIDEO_PORT_INDEX; //video port of the parser3gp
    setHeader(&paramPortVideo, sizeof (OMX_PARAM_PORTDEFINITIONTYPE));
    err = OMX_GetParameter(appPriv->parser3gphandle, OMX_IndexParamPortDefinition, &paramPortVideo);
    if (err != OMX_ErrorNone)
    {
        DEBUG(DEB_LEV_ERR, "\n error in parser3gp port settings get\n");
        exit(1);
    }

    // setting the port parameters of video decoder
    // input port settings
    decparamPortVideo.nPortIndex = 0; /* input port of the video decoder */
    setHeader(&decparamPortVideo, sizeof (OMX_PARAM_PORTDEFINITIONTYPE));
    err = OMX_GetParameter(appPriv->videodechandle, OMX_IndexParamPortDefinition, &decparamPortVideo);
    if (err != OMX_ErrorNone)
    {
        DEBUG(DEB_LEV_ERR, "\n error in videodechandle settings get\n");
        exit(1);
    }

    decparamPortVideo.format.video.eCompressionFormat = paramPortVideo.format.video.eCompressionFormat;
    decparamPortVideo.format.video.nFrameWidth = paramPortVideo.format.video.nFrameWidth;
    decparamPortVideo.format.video.nFrameHeight = paramPortVideo.format.video.nFrameHeight;
    err = OMX_SetParameter(appPriv->videodechandle, OMX_IndexParamPortDefinition, &decparamPortVideo);
    if (err != OMX_ErrorNone)
    {
        DEBUG(DEB_LEV_ERR, "Error %08x setting videodec input port param --- \n", err);
        exit(1);
    }

    // output port settings
    decparamPortVideo.nPortIndex = 1;
    setHeader(&decparamPortVideo, sizeof (OMX_PARAM_PORTDEFINITIONTYPE));
    err = OMX_GetParameter(appPriv->videodechandle, OMX_IndexParamPortDefinition, &decparamPortVideo);
    if (err != OMX_ErrorNone)
    {
        DEBUG(DEB_LEV_ERR, "\n error in videodechandle settings get\n");
        exit(1);
    }

    decparamPortVideo.format.video.eCompressionFormat = paramPortVideo.format.video.eCompressionFormat;
    decparamPortVideo.format.video.nFrameWidth = paramPortVideo.format.video.nFrameWidth;
    decparamPortVideo.format.video.nFrameHeight = paramPortVideo.format.video.nFrameHeight;
    err = OMX_SetParameter(appPriv->videodechandle, OMX_IndexParamPortDefinition, &decparamPortVideo);
    if (err != OMX_ErrorNone)
    {
        DEBUG(DEB_LEV_ERR, "Error %08x setting videodec output port param --- \n", err);
        exit(1);
    }

    new_out_width = paramPortVideo.format.video.nFrameWidth;
    new_out_height = paramPortVideo.format.video.nFrameHeight;
    DEBUG(DEB_LEV_SIMPLE_SEQ, "input picture width : %d height : %d \n", (int) new_out_width, (int) new_out_height);

    /** setting the color converter and sink component chararacteristics, if selected -
     * both in tunneled as well as non tunneled case
     */
    if (flagIsDisplayRequested == 1)
    {
        /** setting the color conv input port width, height
         * it will be same as the video decoder output port width, height
         */
        omx_colorconvPortDefinition.nPortIndex = 0;
        setHeader(&omx_colorconvPortDefinition, sizeof (OMX_PARAM_PORTDEFINITIONTYPE));
        err = OMX_GetParameter(appPriv->colorconv_handle, OMX_IndexParamPortDefinition, &omx_colorconvPortDefinition);
        omx_colorconvPortDefinition.format.video.nFrameWidth = new_out_width;
        omx_colorconvPortDefinition.format.video.nFrameHeight = new_out_height;
        err = OMX_SetParameter(appPriv->colorconv_handle, OMX_IndexParamPortDefinition, &omx_colorconvPortDefinition);
        if (err != OMX_ErrorNone)
        {
            DEBUG(DEB_LEV_ERR, "In %s Setting Input Port Definition Error=%x\n", __func__, err);
            return err;
        }
        /** setting the color converter output width height
         * it will be same as input dimensions
         */
        omx_colorconvPortDefinition.nPortIndex = 1;
        err = OMX_GetParameter(appPriv->colorconv_handle, OMX_IndexParamPortDefinition, &omx_colorconvPortDefinition);
        omx_colorconvPortDefinition.format.video.nFrameWidth = new_out_width;
        omx_colorconvPortDefinition.format.video.nFrameHeight = new_out_height;
        err = OMX_SetParameter(appPriv->colorconv_handle, OMX_IndexParamPortDefinition, &omx_colorconvPortDefinition);
        if (err != OMX_ErrorNone)
        {
            DEBUG(DEB_LEV_ERR, "In %s Setting Output Port Definition Error=%x\n", __func__, err);
            return err;
        }
        /** setting the input color format of color converter component
         * it will be same as output yuv color format of the decoder component
         */
        paramPortVideo.nPortIndex = 1;
        setHeader(&paramPortVideo, sizeof (OMX_PARAM_PORTDEFINITIONTYPE));
        err = OMX_GetParameter(appPriv->videodechandle, OMX_IndexParamPortDefinition, &paramPortVideo);
        omx_colorconvPortDefinition.nPortIndex = 0;
        setHeader(&omx_colorconvPortDefinition, sizeof (OMX_PARAM_PORTDEFINITIONTYPE));
        err = OMX_GetParameter(appPriv->colorconv_handle, OMX_IndexParamPortDefinition, &omx_colorconvPortDefinition);
        omx_colorconvPortDefinition.format.video.eColorFormat = paramPortVideo.format.video.eColorFormat;
        err = OMX_SetParameter(appPriv->colorconv_handle, OMX_IndexParamPortDefinition, &omx_colorconvPortDefinition);
        if (err == OMX_ErrorBadParameter)
        {
            DEBUG(DEB_LEV_ERR, "\n bad parameter of input color format - exiting\n");
            exit(1);
        }
        /** setting output color format of the color converter component */
        /* if Xvideo sink called then output color format is same as the video decoder color format i.e. yuv */
        /* if framebuffer sink called then output color format is rgb */
        omx_colorconvPortDefinition.nPortIndex = 1;
        err = OMX_GetParameter(appPriv->colorconv_handle, OMX_IndexParamPortDefinition, &omx_colorconvPortDefinition);
        if (flagIsXVideoSinkRequested)
        {
            omx_colorconvPortDefinition.format.video.eColorFormat = OMX_COLOR_FormatYUV420Planar;
            /*paramPortVideo.format.video.eColorFormat*/;
        }
        else
            omx_colorconvPortDefinition.format.video.eColorFormat = COLOR_CONV_OUT_RGB_FORMAT;
        err = OMX_SetParameter(appPriv->colorconv_handle, OMX_IndexParamPortDefinition, &omx_colorconvPortDefinition);
        if (err == OMX_ErrorBadParameter)
        {
            DEBUG(DEB_LEV_ERR, "\n bad parameter of output color format setting- exiting\n");
            exit(1);
        }

        /** set the input and output port settings of the video scheduler according to the
         * output port settings of the color converter component if AVsync is used
         */
        if (flagAVsync)
        {
            omx_colorconvPortDefinition.nPortIndex = 1; //color converter output port index
            err = OMX_GetParameter(appPriv->colorconv_handle, OMX_IndexParamPortDefinition, &omx_colorconvPortDefinition);
            omx_colorconvPortDefinition.nPortIndex = 0; //video scheduler input port index
            err = OMX_SetParameter(appPriv->videoschd_handle, OMX_IndexParamPortDefinition, &omx_colorconvPortDefinition);
            if (err != OMX_ErrorNone)
            {
                DEBUG(DEB_LEV_ERR, "\n error in setting the input port param of the video scheduler component- exiting\n");
                exit(1);
            }

            omx_colorconvPortDefinition.nPortIndex = 1; //color converter output port index
            err = OMX_GetParameter(appPriv->colorconv_handle, OMX_IndexParamPortDefinition, &omx_colorconvPortDefinition);
            omx_colorconvPortDefinition.nPortIndex = 1; //video scheduler output port index
            err = OMX_SetParameter(appPriv->videoschd_handle, OMX_IndexParamPortDefinition, &omx_colorconvPortDefinition);
            if (err != OMX_ErrorNone)
            {
                DEBUG(DEB_LEV_ERR, "\n error in setting the output port param of the video scheduler component- exiting\n");
                exit(1);
            }
        }

        /** if video sink component is selected then set its input port settings
         *  according to the output port settings of the color converter component
         */
        omx_colorconvPortDefinition.nPortIndex = 1; //color converter output port index
        err = OMX_GetParameter(appPriv->colorconv_handle, OMX_IndexParamPortDefinition, &omx_colorconvPortDefinition);
        omx_colorconvPortDefinition.nPortIndex = 0; //sink input port index
        err = OMX_SetParameter(appPriv->videosinkhandle, OMX_IndexParamPortDefinition, &omx_colorconvPortDefinition);
        if (err != OMX_ErrorNone)
        {
            DEBUG(DEB_LEV_ERR, "\n error in setting the inputport param of the sink component- exiting\n");
            exit(1);
        }
    }
    return err;
}

void display_help()
{
    printf("\n");
    printf("Usage: omxparsertest -vo outfileVideo.yuv -ao outfileAudio.pcm  [-t]  [-h] [-d] [-c] [-x] input_filename\n");
    printf("\n");
    printf("       -ao outfileAudio.pcm \n");
    printf("       -vo outfileVideo.yuv \n");
    printf("                   If this option is specified, the output is written to user specified outfiles\n");
    printf("                   N.B : This option is not needed if you use the sink component\n");
    printf("\n");
    printf("       -h: Displays this help\n");
    printf("\n");
    printf("       -c: clock component selected AVsync ON\n");
    printf("\n");
    printf("       -x: invokes x-video sink as the video sink component\n");
    printf("\n");
    printf("       -d: Uses the video and alsa sink component to display the video and play the audio output \n");
    printf("           Default video sink is frame buffer unless '-x' option given\n");
    printf("       input_filename is the user specified input file name\n");
    printf("\n");
    printf("       -t: Tunneling option - if this option is selected then by default the color converter, \n");
    printf("           video sink, volume control and alsa sink components are selected even if these options \n");
    printf("           are not specified - the components are tunneled between themselves\n");
    printf("\n");
    exit(1);
}

OMX_ERRORTYPE test_OMX_ComponentNameEnum()
{
    char * name;
    int index;

    OMX_ERRORTYPE err = OMX_ErrorNone;

    DEBUG(DEFAULT_MESSAGES, "GENERAL TEST %s\n", __func__);
    name = malloc(OMX_MAX_STRINGNAME_SIZE);
    index = 0;
    while (1)
    {
        err = OMX_ComponentNameEnum(name, OMX_MAX_STRINGNAME_SIZE, index);
        if ((name != NULL) && (err == OMX_ErrorNone))
        {
            DEBUG(DEFAULT_MESSAGES, "component %i is %s\n", index, name);
        }
        else break;
        if (err != OMX_ErrorNone) break;
        index++;
    }
    free(name);
    name = NULL;
    DEBUG(DEFAULT_MESSAGES, "GENERAL TEST %s result %i\n", __func__, err);
    return err;
}

OMX_ERRORTYPE test_OMX_RoleEnum(OMX_STRING component_name)
{
    OMX_U32 no_of_roles;
    OMX_U8 **string_of_roles;
    OMX_ERRORTYPE err = OMX_ErrorNone;
    int index;

    DEBUG(DEFAULT_MESSAGES, "GENERAL TEST %s\n", __func__);
    DEBUG(DEB_LEV_SIMPLE_SEQ, "Getting roles of %s. Passing Null first...\n", component_name);
    err = OMX_GetRolesOfComponent(component_name, &no_of_roles, NULL);
    if (err != OMX_ErrorNone)
    {
        DEBUG(DEB_LEV_ERR, "Not able to retrieve the number of roles of the given component\n");
        DEBUG(DEFAULT_MESSAGES, "GENERAL TEST %s result %i\n", __func__, err);
        return err;
    }
    DEBUG(DEFAULT_MESSAGES, "The number of roles for the component %s is: %i\n", component_name, (int) no_of_roles);

    if (no_of_roles == 0)
    {
        DEBUG(DEB_LEV_ERR, "The Number or roles is 0.\nThe component selected is not correct for the purpose of this test.\nExiting...\n");
        err = OMX_ErrorInvalidComponentName;
    }
    else
    {
        string_of_roles = (OMX_U8**) malloc(no_of_roles * sizeof (OMX_STRING));
        for (index = 0; index < no_of_roles; index++)
        {
            *(string_of_roles + index) = (OMX_U8 *) malloc(no_of_roles * OMX_MAX_STRINGNAME_SIZE);
        }
        DEBUG(DEB_LEV_SIMPLE_SEQ, "...then buffers\n");

        err = OMX_GetRolesOfComponent(component_name, &no_of_roles, string_of_roles);
        if (err != OMX_ErrorNone)
        {
            DEBUG(DEB_LEV_ERR, "Not able to retrieve the roles of the given component\n");
        }
        else if (string_of_roles != NULL)
        {
            for (index = 0; index < no_of_roles; index++)
            {
                DEBUG(DEFAULT_MESSAGES, "The role %i for the component:  %s \n", (index + 1), *(string_of_roles + index));
            }
        }
        else
        {
            DEBUG(DEB_LEV_ERR, "role string is NULL!!! Exiting...\n");
            err = OMX_ErrorInvalidComponentName;
        }
        for (index = 0; index < no_of_roles; index++)
        {
            free(*(string_of_roles + index));
        }
        free(string_of_roles);
    }
    DEBUG(DEFAULT_MESSAGES, "GENERAL TEST %s result %i\n", __func__, err);
    return err;
}

OMX_ERRORTYPE test_OMX_ComponentEnumByRole(OMX_STRING role_name)
{
    OMX_U32 no_of_comp_per_role;
    OMX_U8 **string_of_comp_per_role;
    OMX_ERRORTYPE err;
    int index;

    DEBUG(DEFAULT_MESSAGES, "GENERAL TEST %s\n", __func__);

    DEBUG(DEFAULT_MESSAGES, "Getting number of components per role for %s\n", role_name);

    err = OMX_GetComponentsOfRole(role_name, &no_of_comp_per_role, NULL);
    if (err != OMX_ErrorNone)
    {
        DEBUG(DEB_LEV_ERR, "Not able to retrieve the number of components of a given role\n");
        DEBUG(DEFAULT_MESSAGES, "GENERAL TEST %s result %i\n", __func__, err);
        return err;
    }
    DEBUG(DEFAULT_MESSAGES, "Number of components per role for %s is %i\n", role_name, (int) no_of_comp_per_role);

    string_of_comp_per_role = (OMX_U8**) malloc(no_of_comp_per_role * sizeof (OMX_STRING));
    for (index = 0; index < no_of_comp_per_role; index++)
    {
        string_of_comp_per_role[index] = malloc(OMX_MAX_STRINGNAME_SIZE);
    }

    err = OMX_GetComponentsOfRole(role_name, &no_of_comp_per_role, string_of_comp_per_role);
    if (err != OMX_ErrorNone)
    {
        DEBUG(DEB_LEV_ERR, "Not able to retrieve the components of a given role\n");
        DEBUG(DEFAULT_MESSAGES, "GENERAL TEST %s result %i\n", __func__, err);
        for (index = 0; index < no_of_comp_per_role; index++)
        {
            if (string_of_comp_per_role[index])
            {
                free(string_of_comp_per_role[index]);
                string_of_comp_per_role[index] = NULL;
            }
        }

        if (string_of_comp_per_role)
        {
            free(string_of_comp_per_role);
            string_of_comp_per_role = NULL;
        }
        return err;
    }

    DEBUG(DEFAULT_MESSAGES, " The components are:\n");
    for (index = 0; index < no_of_comp_per_role; index++)
    {
        DEBUG(DEFAULT_MESSAGES, "%s\n", string_of_comp_per_role[index]);
    }
    for (index = 0; index < no_of_comp_per_role; index++)
    {
        if (string_of_comp_per_role[index])
        {
            free(string_of_comp_per_role[index]);
            string_of_comp_per_role[index] = NULL;
        }
    }

    if (string_of_comp_per_role)
    {
        free(string_of_comp_per_role);
        string_of_comp_per_role = NULL;
    }
    DEBUG(DEFAULT_MESSAGES, "GENERAL TEST %s result OMX_ErrorNone\n", __func__);
    return OMX_ErrorNone;
}

OMX_ERRORTYPE test_OpenClose(OMX_STRING component_name)
{
    OMX_ERRORTYPE err = OMX_ErrorNone;

    DEBUG(DEFAULT_MESSAGES, "GENERAL TEST %s\n", __func__);
    if (strcmp(component_name, AUDIO_COMPONENT_NAME_BASE) == 0)
    {
        err = OMX_GetHandle(&appPriv->audiodechandle, component_name, NULL, &audiodeccallbacks);
        if (err != OMX_ErrorNone)
        {
            DEBUG(DEB_LEV_ERR, "No component found\n");
        }
        else
        {
            err = OMX_FreeHandle(appPriv->audiodechandle);
            if (err != OMX_ErrorNone)
            {
                DEBUG(DEB_LEV_ERR, "In %s err %08x in Free Handle\n", __func__, err);
            }
        }
    }
    else if (strcmp(component_name, VIDEO_COMPONENT_NAME_BASE) == 0)
    {
        err = OMX_GetHandle(&appPriv->videodechandle, component_name, NULL, &videodeccallbacks);
        if (err != OMX_ErrorNone)
        {
            DEBUG(DEB_LEV_ERR, "No component found\n");
        }
        else
        {
            err = OMX_FreeHandle(appPriv->videodechandle);
            if (err != OMX_ErrorNone)
            {
                DEBUG(DEB_LEV_ERR, "In %s err %08x in Free Handle\n", __func__, err);
            }
        }

    }

    DEBUG(DEFAULT_MESSAGES, "GENERAL TEST %s result %i\n", __func__, err);
    return err;
}

int main(int argc, char** argv)
{
    int argn_dec;
    int gain = -1;
    OMX_ERRORTYPE err;
    OMX_INDEXTYPE eIndexParamFilename;
    OMX_PARAM_COMPONENTROLETYPE sComponentRole;
    OMX_AUDIO_CONFIG_VOLUMETYPE sVolume;
    OMX_TIME_CONFIG_CLOCKSTATETYPE sClockState;
    OMX_TIME_CONFIG_SCALETYPE sConfigScale;
    OMX_TIME_CONFIG_ACTIVEREFCLOCKTYPE sRefClock;
    char keyin;
    OMX_S32 newscale = 0;


    if (argc < 2)
    {
        display_help();
    }
    else
    {
        flagIsVideoOutputFileExpected = 0;
        flagIsAudioOutputFileExpected = 0;
        flagDecodedOutputReceived = 0;
        flagInputReceived = 0;
        flagSetupTunnel = 0;
        flagIsDisplayRequested = 0;
        flagAVsync = 0;
        flagIsXVideoSinkRequested = 0;

        argn_dec = 1;
        while (argn_dec < argc)
        {
            if (*(argv[argn_dec]) == '-')
            {
                if (flagIsVideoOutputFileExpected || flagIsAudioOutputFileExpected)
                {
                    display_help();
                }
                switch (*(argv[argn_dec] + 1))
                {
                case 'h':
                    display_help();
                    break;
                case 't':
                    flagSetupTunnel = 1;
                    flagIsDisplayRequested = 1;
                    break;
                case 'd':
                    flagIsDisplayRequested = 1;
                    break;
                case 'c':
                    flagAVsync = 1;
                    break;
                case 'x':
                    flagIsXVideoSinkRequested = 1;
                    break;
                case 'a':
                    if (*(argv[argn_dec] + 2) == 'o')
                    {
                        flagIsAudioOutputFileExpected = 1;
                    }
                    break;
                case 'v':
                    if (*(argv[argn_dec] + 2) == 'o')
                    {
                        flagIsVideoOutputFileExpected = 1;
                    }
                    break;
                default:
                    display_help();
                }
            }
            else
            {
                if (flagIsVideoOutputFileExpected)
                {
                    if (strstr(argv[argn_dec], ".yuv") != NULL)
                    {
                        output_file_video = malloc(strlen(argv[argn_dec]) * sizeof (char) + 1);
                        strcpy(output_file_video, argv[argn_dec]);
                    }
                    flagIsVideoOutputFileExpected = 0;
                    flagDecodedOutputReceived = 1;
                }
                else if (flagIsAudioOutputFileExpected)
                {
                    if (strstr(argv[argn_dec], ".pcm") != NULL)
                    {
                        output_file_audio = malloc(strlen(argv[argn_dec]) * sizeof (char) + 1);
                        strcpy(output_file_audio, argv[argn_dec]);
                    }
                    flagIsAudioOutputFileExpected = 0;
                    flagDecodedOutputReceived = 1;
                }
                else
                {
                    input_file = malloc(strlen(argv[argn_dec]) * sizeof (char) + 1);
                    strcpy(input_file, argv[argn_dec]);
                    flagInputReceived = 1;
                }
            }
            argn_dec++;
        }

        /** input file name check */
        //if ((!flagInputReceived) || ((strstr(input_file, ".3gp") == NULL))) {
        if (0)
        {
            DEBUG(DEB_LEV_ERR, "\n you must specify appropriate input file of .3gp format\n");
            display_help();
        }

        /** output file name check */
        //case 1 - user did not specify output file names
        if (flagIsAudioOutputFileExpected || flagIsVideoOutputFileExpected)
        {
            DEBUG(DEB_LEV_ERR, "\n you must specify appropriate output audio and video file names \n");
            display_help();
        }

        if (flagDecodedOutputReceived)
        {
            if (flagIsDisplayRequested || flagSetupTunnel)
            {
                flagDecodedOutputReceived = 0;
                DEBUG(DEB_LEV_ERR, "Display Requested or Components are tunneled. No FILE Output will be produced\n");
            }
        }

        DEBUG(DEFAULT_MESSAGES, "Options selected:\n");
        if (flagDecodedOutputReceived)
        {
            DEBUG(DEFAULT_MESSAGES, "Decode file %s to produce audio file %s and video file %s\n", input_file, output_file_audio, output_file_video);
        }
        else
        {
            DEBUG(DEFAULT_MESSAGES, "As audio and video sink components are chosen, no output file is generated even if specified\n");
        }
        if (flagIsDisplayRequested)
        {
            DEBUG(DEFAULT_MESSAGES, "See the movie being played....\n");
        }
        if (flagSetupTunnel)
        {
            DEBUG(DEFAULT_MESSAGES, "The components are tunneled between themselves\n");
        }
    }

    if (!flagIsDisplayRequested)
    {
        if (output_file_audio == NULL || output_file_video == NULL)
        {
            DEBUG(DEB_LEV_ERR, "\n Error in specifying audio/video output filename ....,\n");
            if (output_file_audio == NULL) DEBUG(DEB_LEV_ERR, "  For audio output specify a filename with .pcm extension\n");
            if (output_file_video == NULL) DEBUG(DEB_LEV_ERR, "  For video output specify a filename with .yuv extension\n");
            display_help();
            exit(1);
        }
        outfileAudio = fopen(output_file_audio, "wb");
        if (outfileAudio == NULL)
        {
            DEBUG(DEB_LEV_ERR, "Error in opening output file %s\n", output_file_audio);
            exit(1);
        }
        outfileVideo = fopen(output_file_video, "wb");
        if (outfileVideo == NULL)
        {
            DEBUG(DEB_LEV_ERR, "Error in opening output file %s\n", output_file_video);
            exit(1);
        }
    }

    /** setting input picture width to a default value (vga format) for allocation of video decoder buffers */
    out_width = 1920;
    out_height = 1080;

    /** initializing appPriv structure */
    appPriv = malloc(sizeof (appPrivateType));
    appPriv->parser3gpEventSem = malloc(sizeof (tsem_t));
    appPriv->videoDecoderEventSem = malloc(sizeof (tsem_t));
    appPriv->audioDecoderEventSem = malloc(sizeof (tsem_t));
    if (flagIsDisplayRequested == 1)
    {
        appPriv->fbdevSinkEventSem = malloc(sizeof (tsem_t));
        appPriv->colorconvEventSem = malloc(sizeof (tsem_t));
        appPriv->audioSinkEventSem = malloc(sizeof (tsem_t));
        appPriv->volumeEventSem = malloc(sizeof (tsem_t));
        if (flagAVsync)
        {
            appPriv->videoschdEventSem = malloc(sizeof (tsem_t));
        }
    }
    appPriv->eofSem = malloc(sizeof (tsem_t));
    if (flagAVsync)
    {
        appPriv->clockEventSem = malloc(sizeof (tsem_t));
    }

    tsem_init(appPriv->parser3gpEventSem, 0);
    tsem_init(appPriv->videoDecoderEventSem, 0);
    tsem_init(appPriv->audioDecoderEventSem, 0);
    if (flagIsDisplayRequested == 1)
    {
        tsem_init(appPriv->fbdevSinkEventSem, 0);
        tsem_init(appPriv->colorconvEventSem, 0);
        tsem_init(appPriv->audioSinkEventSem, 0);
        tsem_init(appPriv->volumeEventSem, 0);
        if (flagAVsync)
        {
            tsem_init(appPriv->videoschdEventSem, 0);
        }
    }
    tsem_init(appPriv->eofSem, 0);
    if (flagAVsync)
    {
        tsem_init(appPriv->clockEventSem, 0);
    }

    /** initialising openmax */
    err = OMX_Init();
    if (err != OMX_ErrorNone)
    {
        DEBUG(DEB_LEV_ERR, "The OpenMAX core can not be initialized. Exiting...\n");
        exit(1);
    }
    else
    {
        DEBUG(DEB_LEV_SIMPLE_SEQ, "Omx core is initialized \n");
    }

    DEBUG(DEFAULT_MESSAGES, "------------------------------------\n");
    test_OMX_ComponentNameEnum();
    DEBUG(DEFAULT_MESSAGES, "------------------------------------\n");
    test_OMX_RoleEnum(VIDEO_COMPONENT_NAME_BASE);
    DEBUG(DEFAULT_MESSAGES, "------------------------------------\n");
    test_OMX_ComponentEnumByRole(VIDEO_BASE_ROLE);
    DEBUG(DEFAULT_MESSAGES, "------------------------------------\n");
    test_OMX_RoleEnum(AUDIO_COMPONENT_NAME_BASE);
    DEBUG(DEFAULT_MESSAGES, "------------------------------------\n");
    test_OMX_ComponentEnumByRole(AUDIO_BASE_ROLE);
    DEBUG(DEFAULT_MESSAGES, "------------------------------------\n");
    test_OpenClose(VIDEO_COMPONENT_NAME_BASE);
    DEBUG(DEFAULT_MESSAGES, "------------------------------------\n");
    test_OpenClose(AUDIO_COMPONENT_NAME_BASE);
    DEBUG(DEFAULT_MESSAGES, "------------------------------------\n");

    DEBUG(DEFAULT_MESSAGES, "The component selected for video decoding is %s\n Role is decided by the decoder\n", VIDEO_COMPONENT_NAME_BASE);
    DEBUG(DEFAULT_MESSAGES, "The component selected for audio decoding is %s\n Role is decided by the decoder\n", AUDIO_COMPONENT_NAME_BASE);

    DEBUG(DEB_LEV_SIMPLE_SEQ, "Using Parser3gp \n");
    /** parser3gp component --  gethandle*/
    err = OMX_GetHandle(&appPriv->parser3gphandle, PARSER_3GP, NULL /*appPriv */, &parser3gpcallbacks);
    if (err != OMX_ErrorNone)
    {
        DEBUG(DEB_LEV_ERR, "Parser3gp Component Not Found\n");
        exit(1);
    }
    else
    {
        DEBUG(DEFAULT_MESSAGES, "Parser3gp Component Found\n");
    }

    /* getting the handle to the clock src component */
    if (flagAVsync)
    {
        err = OMX_GetHandle(&appPriv->clocksrchandle, CLOCK_SRC, NULL, &clocksrccallbacks);
        if (err != OMX_ErrorNone)
        {
            DEBUG(DEB_LEV_ERR, "Clocksrc Component Not Found\n");
            exit(1);
        }
        else
        {
            DEBUG(DEFAULT_MESSAGES, "Clocksrc Component Found\n");
        }
    }

    /* getting the handles for the components in video  and audio piepline*/
    /** getting video decoder handle */
    err = OMX_GetHandle(&appPriv->videodechandle, VIDEO_COMPONENT_NAME_BASE, NULL, &videodeccallbacks);
    if (err != OMX_ErrorNone)
    {
        DEBUG(DEB_LEV_ERR, "No video decoder component found. Exiting...\n");
        exit(1);
    }
    else
    {
        DEBUG(DEFAULT_MESSAGES, "The component for video decoding is %s\n", VIDEO_COMPONENT_NAME_BASE);
    }

#ifdef SUPPORT_AUDIO
    /** getting audio decoder handle */
    DEBUG(DEB_LEV_SIMPLE_SEQ, "Getting Audio %s Decoder Handle\n", AUDIO_COMPONENT_NAME_BASE);
    err = OMX_GetHandle(&appPriv->audiodechandle, AUDIO_COMPONENT_NAME_BASE, NULL, &audiodeccallbacks);
    if (err != OMX_ErrorNone)
    {
        DEBUG(DEB_LEV_ERR, "Audio Decoder Component Not Found\n");
        exit(1);
    }
    DEBUG(DEFAULT_MESSAGES, "The Component for audio deoding is %s \n", AUDIO_COMPONENT_NAME_BASE);
#endif
    /** getting handle of other components in audio and video pipeline, if specified */
    if (flagIsDisplayRequested)
    {

        /** getting color converter component handle */
        err = OMX_GetHandle(&appPriv->colorconv_handle, COLOR_CONV, NULL, &colorconv_callbacks);
        if (err != OMX_ErrorNone)
        {
            DEBUG(DEB_LEV_ERR, "No color converter component found. Exiting...\n");
            exit(1);
        }
        else
        {
            DEBUG(DEFAULT_MESSAGES, "Found The component for color converter \n");
        }

        if (flagAVsync)
        {
            /** getting video scheduler component handle */
            err = OMX_GetHandle(&appPriv->videoschd_handle, VIDEO_SCHD, NULL, &videoschd_callbacks);
            if (err != OMX_ErrorNone)
            {
                DEBUG(DEB_LEV_ERR, "No video scheduler component found. Exiting...\n");
                exit(1);
            }
            else
            {
                DEBUG(DEFAULT_MESSAGES, "Found the video scheduler component \n");
            }
        }

        /** getting video sink component handle */
        if (flagIsXVideoSinkRequested)
            err = OMX_GetHandle(&appPriv->videosinkhandle, VIDEO_SINK_X_VIDEO, NULL, &fbdev_sink_callbacks);
        else
            err = OMX_GetHandle(&appPriv->videosinkhandle, VIDEO_SINK_FRAME_BUFFER, NULL, &fbdev_sink_callbacks);

        if (err != OMX_ErrorNone)
        {
            DEBUG(DEB_LEV_ERR, "No video sink component component found. Exiting...\n");
            exit(1);
        }
        else
        {
            DEBUG(DEFAULT_MESSAGES, "Found The video sink component for color converter \n");
        }
#ifdef SUPPORT_AUDIO
        /** getting audio sink component handle */
        err = OMX_GetHandle(&appPriv->audiosinkhandle, AUDIO_SINK, NULL, &audiosinkcallbacks);
        if (err != OMX_ErrorNone)
        {
            DEBUG(DEB_LEV_ERR, "No audio sink found. Exiting...\n");
            exit(1);
        }
        else
        {
            DEBUG(DEFAULT_MESSAGES, "Found The alsa sink component for audio \n");
        }

        /** getting volume component handle */
        DEBUG(DEFAULT_MESSAGES, "Getting Handle for Component %s\n", AUDIO_EFFECT);
        err = OMX_GetHandle(&appPriv->volumehandle, AUDIO_EFFECT, NULL, &volumecallbacks);
        if (err != OMX_ErrorNone)
        {
            DEBUG(DEB_LEV_ERR, "No volume component found. Exiting...\n");
            exit(1);
        }
        else
        {
            DEBUG(DEFAULT_MESSAGES, "Found The volume component for audio \n");
        }

        if ((gain >= 0) && (gain < 100))
        {
            err = OMX_GetConfig(appPriv->volumehandle, OMX_IndexConfigAudioVolume, &sVolume);
            if (err != OMX_ErrorNone)
            {
                DEBUG(DEB_LEV_ERR, "Error %08x In OMX_GetConfig 0 \n", err);
            }
            sVolume.sVolume.nValue = gain;
            DEBUG(DEFAULT_MESSAGES, "Setting Gain %d \n", gain);
            err = OMX_SetConfig(appPriv->volumehandle, OMX_IndexConfigAudioVolume, &sVolume);
            if (err != OMX_ErrorNone)
            {
                DEBUG(DEB_LEV_ERR, "Error %08x In OMX_SetConfig 0 \n", err);
            }
        }
#endif
    }

    /* disable the clock ports of the clients (audio, video and parser), if AVsync not enabled*/
    if (flagAVsync)
    {
        err = OMX_SendCommand(appPriv->parser3gphandle, OMX_CommandPortEnable, 2, NULL);
        if (err != OMX_ErrorNone)
        {
            DEBUG(DEB_LEV_ERR, "parser clock port Enable failed\n");
            exit(1);
        }
        tsem_down(appPriv->parser3gpEventSem); /* parser clock port Enabled */
        DEBUG(DEB_LEV_SIMPLE_SEQ, "In %s parser Clock Port Enabled\n", __func__);

        if (flagIsDisplayRequested)
        {
#ifdef SUPPORT_AUDIO
            err = OMX_SendCommand(appPriv->audiosinkhandle, OMX_CommandPortEnable, 1, NULL);
            if (err != OMX_ErrorNone)
            {
                DEBUG(DEB_LEV_ERR, "audiosink clock port Enable failed\n");
                exit(1);
            }
            tsem_down(appPriv->audioSinkEventSem); /* audio sink clock port Enabled */
            DEBUG(DEB_LEV_SIMPLE_SEQ, "In %s Audio Sink Clock Port Enabled\n", __func__);
#endif
            err = OMX_SendCommand(appPriv->videoschd_handle, OMX_CommandPortEnable, 2, NULL);
            if (err != OMX_ErrorNone)
            {
                DEBUG(DEB_LEV_ERR, "video scheduler clock port Enable failed\n");
                exit(1);
            }
            tsem_down(appPriv->videoschdEventSem); /* video scheduler clock port Enabled */
            DEBUG(DEB_LEV_SIMPLE_SEQ, "In %s video scheduler Clock Port Enabled\n", __func__);
        }
    }

    /** setting the input format in parser3gp */
    err = OMX_GetExtensionIndex(appPriv->parser3gphandle, "OMX.ST.index.param.inputfilename", &eIndexParamFilename);
    if (err != OMX_ErrorNone)
    {
        DEBUG(DEB_LEV_ERR, "\n error in get extension index\n");
        exit(1);
    }
    else
    {
        char *temp = malloc(25);
        DEBUG(DEB_LEV_SIMPLE_SEQ, "FileName Param index : %x \n", eIndexParamFilename);
        OMX_GetParameter(appPriv->parser3gphandle, eIndexParamFilename, temp);
        err = OMX_SetParameter(appPriv->parser3gphandle, eIndexParamFilename, input_file);
        if (err != OMX_ErrorNone)
        {
            DEBUG(DEB_LEV_ERR, "\n error in input format - exiting\n");
            exit(1);
        }
        free(temp);
    }

    /* Set up the tunnel between the clock component ports and the client clock ports and disable the rest of the ports */
    if (flagAVsync)
    {
#ifdef SUPPORT_AUDIO
        err = OMX_SetupTunnel(appPriv->clocksrchandle, AUDIO_PORT_INDEX, appPriv->audiosinkhandle, AUDIO_SINK_CLK_PORT_INDEX);
        if (err != OMX_ErrorNone)
        {
            DEBUG(DEB_LEV_ERR, "Set up Tunnel btwn clock and audio sink Failed Error=%x\n", err);
            exit(1);
        }
        else
        {
            DEBUG(DEB_LEV_ERR, "Setup Tunnel between clock and audio sink successful\n");
        }
#endif
        err = OMX_SetupTunnel(appPriv->clocksrchandle, VIDEO_PORT_INDEX, appPriv->videoschd_handle, VID_SCH_CLK_PORT_INDEX);
        if (err != OMX_ErrorNone)
        {
            DEBUG(DEB_LEV_ERR, "Set up Tunnel btwn clock and video scheduler Failed\n");
            exit(1);
        }
        else
        {
            DEBUG(DEB_LEV_ERR, "Setup Tunnel between clock and video scheduler successful\n");
        }

        err = OMX_SetupTunnel(appPriv->clocksrchandle, PARSER_PORT_INDEX, appPriv->parser3gphandle, PARSER_PORT_INDEX);
        if (err != OMX_ErrorNone)
        {
            DEBUG(DEB_LEV_ERR, "Set up Tunnel btwn clock and parser3gp Failed\n");
            exit(1);
        }
        else
        {
            DEBUG(DEB_LEV_ERR, "Setup Tunnel between clock and parser3gp successful\n");
        }

        if (!flagSetupTunnel)
        {
            /* disable the clock port on parser and the clock port on the clock component tunneled to the parser, till clock is put in Idle state*/
            err = OMX_SendCommand(appPriv->parser3gphandle, OMX_CommandPortDisable, 2, NULL);
            if (err != OMX_ErrorNone)
            {
                DEBUG(DEB_LEV_ERR, "parser clock port disable failed\n");
                exit(1);
            }
            tsem_down(appPriv->parser3gpEventSem); /* parser clock port disabled */
            DEBUG(DEB_LEV_SIMPLE_SEQ, "In %s parser Clock Port Disabled\n", __func__);

            err = OMX_SendCommand(appPriv->clocksrchandle, OMX_CommandPortDisable, 2, NULL);
            if (err != OMX_ErrorNone)
            {
                DEBUG(DEB_LEV_ERR, "clocksrc component's clock  port (tunneled to parser's clock port) disable failed\n");
                exit(1);
            }
            tsem_down(appPriv->clockEventSem); /* clocksrc clock port disabled */
            DEBUG(DEB_LEV_SIMPLE_SEQ, "In %s clocksrc  Clock Port (connected to parser) Disabled\n", __func__);
        }
    }

    if (flagSetupTunnel)
    {
        DEBUG(DEB_LEV_SIMPLE_SEQ, "Setting up Tunnel\n");
        err = OMX_SetupTunnel(appPriv->parser3gphandle, VIDEO_PORT_INDEX, appPriv->videodechandle, 0);
        if (err != OMX_ErrorNone)
        {
            DEBUG(DEB_LEV_ERR, "Set up Tunnel parser3gp to video decoder Failed\n");
            exit(1);
        }
#ifdef SUPPORT_AUDIO
        err = OMX_SetupTunnel(appPriv->parser3gphandle, AUDIO_PORT_INDEX, appPriv->audiodechandle, 0);
        if (err != OMX_ErrorNone)
        {
            DEBUG(DEB_LEV_ERR, "Set up Tunnel parser3gp to audio decoder Failed\n");
            exit(1);
        }
#endif
        err = OMX_SetupTunnel(appPriv->videodechandle, 1, appPriv->colorconv_handle, 0);
        if (err != OMX_ErrorNone)
        {
            DEBUG(DEB_LEV_ERR, "Set up Tunnel video decoder to color converter Failed\n");
            exit(1);
        }
        if (flagAVsync)
        { /* if AVsync is ON setup the tunnel as colorconv->videoscheduler->videosink else colorconv->videosink */
            err = OMX_SetupTunnel(appPriv->colorconv_handle, 1, appPriv->videoschd_handle, 0);
            if (err != OMX_ErrorNone)
            {
                DEBUG(DEB_LEV_ERR, "Set up Tunnel color converter to video scheduler Failed\n");
                exit(1);
            }
            err = OMX_SetupTunnel(appPriv->videoschd_handle, 1, appPriv->videosinkhandle, 0);
            if (err != OMX_ErrorNone)
            {
                DEBUG(DEB_LEV_ERR, "Set up Tunnel video scheduler to video sink Failed\n");
                exit(1);
            }
        }
        else
        {
            err = OMX_SetupTunnel(appPriv->colorconv_handle, 1, appPriv->videosinkhandle, 0);
            if (err != OMX_ErrorNone)
            {
                DEBUG(DEB_LEV_ERR, "Set up Tunnel color converter to video sink Failed\n");
                exit(1);
            }
        }
#ifdef SUPPORT_AUDIO
        err = OMX_SetupTunnel(appPriv->audiodechandle, 1, appPriv->volumehandle, 0);
        if (err != OMX_ErrorNone)
        {
            DEBUG(DEB_LEV_ERR, "Set up Tunnel audiodecoder to volume Failed\n");
            exit(1);
        }

        err = OMX_SetupTunnel(appPriv->volumehandle, 1, appPriv->audiosinkhandle, 0);
        if (err != OMX_ErrorNone)
        {
            DEBUG(DEB_LEV_ERR, "Set up Tunnel volume to audio sink Failed\n");
            exit(1);
        }
#endif
        DEBUG(DEB_LEV_SIMPLE_SEQ, "Set up Tunnel Completed\n");

        /* The decoder is the buffer supplier and parser is the consumer,
         not the other way round. This ensures that the first buffer is available when the decoder is in idle state.
         This prevents the loss of the first frame buffer */
        OMX_PARAM_BUFFERSUPPLIERTYPE sBufferSupplier;
        sBufferSupplier.nPortIndex = 0;
        sBufferSupplier.eBufferSupplier = OMX_BufferSupplyInput;
        setHeader(&sBufferSupplier, sizeof (OMX_PARAM_BUFFERSUPPLIERTYPE));

        err = OMX_SetParameter(appPriv->videodechandle, OMX_IndexParamCompBufferSupplier, &sBufferSupplier);
        if (err != OMX_ErrorNone)
        {
            DEBUG(DEB_LEV_ERR, "In %s Setting Video Decoder Input Port Buffer Supplier Error=%x\n", __func__, err);
            return err;
        }
#ifdef SUPPORT_AUDIO
        err = OMX_SetParameter(appPriv->audiodechandle, OMX_IndexParamCompBufferSupplier, &sBufferSupplier);
        if (err != OMX_ErrorNone)
        {
            DEBUG(DEB_LEV_ERR, "In %s Setting Audio Decoder Input Port Buffer Supplier Error=%x\n", __func__, err);
            return err;
        }
#endif
    }


    /** set the parser3gp component to idle state */
    OMX_SendCommand(appPriv->parser3gphandle, OMX_CommandStateSet, OMX_StateIdle, NULL);

    if (flagSetupTunnel)
    {
        DEBUG(DEB_LEV_SIMPLE_SEQ, "Send Command Idle to Video Dec\n");
        /*Send State Change Idle command to Video and Audio Decoder*/
        err = OMX_SendCommand(appPriv->videodechandle, OMX_CommandStateSet, OMX_StateIdle, NULL);
#ifdef SUPPORT_AUDIO
        err = OMX_SendCommand(appPriv->audiodechandle, OMX_CommandStateSet, OMX_StateIdle, NULL);
#endif
        if (flagAVsync)
        {
            err = OMX_SendCommand(appPriv->clocksrchandle, OMX_CommandStateSet, OMX_StateIdle, NULL);
        }
        if (flagIsDisplayRequested)
        {
            DEBUG(DEB_LEV_SIMPLE_SEQ, "Send Command Idle to Audio and Video Sink\n");
            err = OMX_SendCommand(appPriv->colorconv_handle, OMX_CommandStateSet, OMX_StateIdle, NULL);
            if (flagAVsync)
            {
                err = OMX_SendCommand(appPriv->videoschd_handle, OMX_CommandStateSet, OMX_StateIdle, NULL);
            }
            err = OMX_SendCommand(appPriv->videosinkhandle, OMX_CommandStateSet, OMX_StateIdle, NULL); /* put in the idle state after the clock is put in idle state */
#ifdef SUPPORT_AUDIO
            err = OMX_SendCommand(appPriv->volumehandle, OMX_CommandStateSet, OMX_StateIdle, NULL);
            err = OMX_SendCommand(appPriv->audiosinkhandle, OMX_CommandStateSet, OMX_StateIdle, NULL);
#endif
        }
    }

    /* In case tunnel is not set up then allocate the output buffers of the parser
       two buffers for video port and two buffers for audio port */
    if (!flagSetupTunnel)
    {
        outBufferParseVideo[0] = outBufferParseVideo[1] = NULL;
        outBufferParseAudio[0] = outBufferParseAudio[1] = NULL;
        /** allocation of parser3gp component's output buffers for video decoder component */
        err = OMX_AllocateBuffer(appPriv->parser3gphandle, &outBufferParseVideo[0], VIDEO_PORT_INDEX, NULL, buffer_out_size);
        if (err != OMX_ErrorNone)
        {
            DEBUG(DEB_LEV_ERR, "Unable to allocate Video buffer 1 in parser3gp \n");
            exit(1);
        }
        err = OMX_AllocateBuffer(appPriv->parser3gphandle, &outBufferParseVideo[1], VIDEO_PORT_INDEX, NULL, buffer_out_size);
        if (err != OMX_ErrorNone)
        {
            DEBUG(DEB_LEV_ERR, "Unable to allocate Video buffer 2 in parser3gp \n");
            exit(1);
        }

        /** allocation of parser3gp component's output buffers for audio decoder component */
        err = OMX_AllocateBuffer(appPriv->parser3gphandle, &outBufferParseAudio[0], AUDIO_PORT_INDEX, NULL, buffer_out_size);
        if (err != OMX_ErrorNone)
        {
            DEBUG(DEB_LEV_ERR, "Unable to allocate Audio buffer 1 in parser3gp \n");
            exit(1);
        }
        err = OMX_AllocateBuffer(appPriv->parser3gphandle, &outBufferParseAudio[1], AUDIO_PORT_INDEX, NULL, buffer_out_size);
        if (err != OMX_ErrorNone)
        {
            DEBUG(DEB_LEV_ERR, "Unable to allocate Audio buffer 2 in parser3gp \n");
            exit(1);
        }
    }

    /*Wait for Parser3gp  and clocksrc state change to Idle*/
    tsem_down(appPriv->parser3gpEventSem);
    DEBUG(DEFAULT_MESSAGES, "Parser3gp in idle state \n");

    /* Wait for all components to change to idle state*/
    if (flagSetupTunnel)
    {
        tsem_down(appPriv->videoDecoderEventSem);
#ifdef SUPPORT_AUDIO
        tsem_down(appPriv->audioDecoderEventSem);
#endif
        if (flagIsDisplayRequested)
        {
            tsem_down(appPriv->colorconvEventSem);
            if (flagAVsync)
            {
                tsem_down(appPriv->videoschdEventSem);
            }
            tsem_down(appPriv->fbdevSinkEventSem);
#ifdef SUPPORT_AUDIO
            tsem_down(appPriv->volumeEventSem);
            tsem_down(appPriv->audioSinkEventSem);
#endif
        }
        if (flagAVsync)
        {
            tsem_down(appPriv->clockEventSem);
        }
        DEBUG(DEFAULT_MESSAGES, "All tunneled components in idle state \n");
    }

    err = OMX_SendCommand(appPriv->parser3gphandle, OMX_CommandStateSet, OMX_StateExecuting, NULL);
    if (err != OMX_ErrorNone)
    {
        DEBUG(DEB_LEV_ERR, "Parser 3gp state executing failed\n");
        exit(1);
    }
    /*Wait for Parser3gp state change to executing*/
    tsem_down(appPriv->parser3gpEventSem);
    DEBUG(DEFAULT_MESSAGES, "Parser3gp in executing state \n");

    /*Wait for Parser3gp Ports Setting Changed Event. Since Parser3gp Always detect the stream
    Always ports setting change event will be received*/
    tsem_down(appPriv->parser3gpEventSem);
    tsem_down(appPriv->parser3gpEventSem);
    DEBUG(DEFAULT_MESSAGES, "Parser3gp Port Settings Changed event \n");


    /* for the tunneled case wait for all ports to be disabled */
    if (flagSetupTunnel)
    {
        tsem_down(appPriv->parser3gpEventSem); /* parser3gp Video o/p port disabled */
        tsem_down(appPriv->parser3gpEventSem); /* parser3gp Audio o/p port disabled */
        DEBUG(DEB_LEV_SIMPLE_SEQ, "In %s Parser 3gp Component Ports Disabled\n", __func__);
        tsem_down(appPriv->videoDecoderEventSem); /* Video decoder i/p port disabled */
        tsem_down(appPriv->videoDecoderEventSem); /* Video decoder o/p port disabled */
        DEBUG(DEB_LEV_SIMPLE_SEQ, "In %s Video decoder Component Port Disabled\n", __func__);
#ifdef SUPPORT_AUDIO
        tsem_down(appPriv->audioDecoderEventSem); /* Audio decoder i/p port disabled */
        tsem_down(appPriv->audioDecoderEventSem); /* Audio decoder o/p port disabled */
        DEBUG(DEB_LEV_SIMPLE_SEQ, "In %s Audio decoder Component Port Disabled\n", __func__);
#endif
        if (flagIsDisplayRequested)
        {
            tsem_down(appPriv->colorconvEventSem); /* color conv  i/p port disabled */
            tsem_down(appPriv->colorconvEventSem); /* color conv  o/p port disabled */
            DEBUG(DEB_LEV_SIMPLE_SEQ, "In %s Color Converter Component Port Disabled\n", __func__);
            if (flagAVsync)
            {
                tsem_down(appPriv->videoschdEventSem); /* video scheduler i/p port disabled */
                tsem_down(appPriv->videoschdEventSem); /* video scheduler o/p port disabled */
                DEBUG(DEB_LEV_SIMPLE_SEQ, "In %s Video Scheduler Component Port Disabled\n", __func__);
            }
            tsem_down(appPriv->fbdevSinkEventSem); /* sink i/p port disabled */
            DEBUG(DEB_LEV_SIMPLE_SEQ, "In %s Video Sink Port Disabled\n", __func__);
#ifdef SUPPORT_AUDIO
            tsem_down(appPriv->volumeEventSem); /* volume  i/p port disabled */
            tsem_down(appPriv->volumeEventSem); /* volume  o/p port disabled */
            DEBUG(DEB_LEV_SIMPLE_SEQ, "In %s Volume Component Port Disabled\n", __func__);
            tsem_down(appPriv->audioSinkEventSem); /* sink i/p port disabled */
            DEBUG(DEB_LEV_SIMPLE_SEQ, "In %s Audio Sink Port Disabled\n", __func__);
#endif
        }

        /* Put the audio and video decoder back to Loaded state to set their role*/
        err = OMX_SendCommand(appPriv->videodechandle, OMX_CommandStateSet, OMX_StateLoaded, NULL);
#ifdef SUPPORT_AUDIO
        err = OMX_SendCommand(appPriv->audiodechandle, OMX_CommandStateSet, OMX_StateLoaded, NULL);
#endif

        tsem_down(appPriv->videoDecoderEventSem);
#ifdef SUPPORT_AUDIO
        tsem_down(appPriv->audioDecoderEventSem);
#endif
    }

    /*  setting the port parameters and component role */
    SetPortParametersVideo();
    setHeader(&sComponentRole, sizeof (OMX_PARAM_COMPONENTROLETYPE));
    err = OMX_GetParameter(appPriv->videodechandle, OMX_IndexParamStandardComponentRole, &sComponentRole);
    switch (paramPortVideo.format.video.eCompressionFormat)
    {
    case OMX_VIDEO_CodingMPEG4:
        strcpy((char*) &sComponentRole.cRole[0], VIDEO_DEC_MPEG4_ROLE);
        break;
    case OMX_VIDEO_CodingAVC:
        strcpy((char*) &sComponentRole.cRole[0], VIDEO_DEC_H264_ROLE);
        break;
    case OMX_VIDEO_CodingVP8:
        strcpy((char*) &sComponentRole.cRole[0], VIDEO_DEC_VP8_ROLE);
        break;
    case OMX_VIDEO_CodingRV:
        strcpy((char*) &sComponentRole.cRole[0], VIDEO_DEC_RV_ROLE);
        break;
    case OMX_VIDEO_CodingWMV:
        strcpy((char*) &sComponentRole.cRole[0], VIDEO_DEC_WMV_ROLE);
        break;
    default:
        DEBUG(DEB_LEV_ERR, "Error only MPEG4 and AVC(h.264) are supported %08x\n", paramPortVideo.format.video.eCompressionFormat);
    }
    err = OMX_SetParameter(appPriv->videodechandle, OMX_IndexParamStandardComponentRole, &sComponentRole);
    if (err != OMX_ErrorNone)
    {
        DEBUG(DEB_LEV_ERR, "\n error in input video format - exiting\n");
        exit(1);
    }

    // C&M add to set exact profile level to decoder component
    switch (paramPortVideo.format.video.eCompressionFormat)
    {
    case OMX_VIDEO_CodingWMV:
        { 
            OMX_VIDEO_PARAM_WMVTYPE wmvParam;
            err = OMX_GetParameter(appPriv->parser3gphandle, OMX_IndexParamVideoWmv, &wmvParam);
            err = OMX_SetParameter(appPriv->videodechandle, OMX_IndexParamVideoWmv, &wmvParam);
            if (err != OMX_ErrorNone)
            {
                DEBUG(DEB_LEV_ERR, "\n error in set video profile - exiting\n");
                exit(1);
            }
            break;
        }
   case OMX_VIDEO_CodingRV:
        { 
            OMX_VIDEO_PARAM_RVTYPE rvParam;
            err = OMX_GetParameter(appPriv->parser3gphandle, OMX_IndexParamVideoRv, &rvParam);
            err = OMX_SetParameter(appPriv->videodechandle, OMX_IndexParamVideoRv, &rvParam);
            if (err != OMX_ErrorNone)
            {
                DEBUG(DEB_LEV_ERR, "\n error in set video profile - exiting\n");
                exit(1);
            }
            break;
        }

    }
    

#ifdef SUPPORT_AUDIO
    /* setting the component role for the audio component */
    SetPortParametersAudio();
    setHeader(&sComponentRole, sizeof (OMX_PARAM_COMPONENTROLETYPE));
    err = OMX_GetParameter(appPriv->audiodechandle, OMX_IndexParamStandardComponentRole, &sComponentRole);
    switch (paramPortAudio.format.audio.eEncoding)
    {
    case OMX_AUDIO_CodingMP3:
        strcpy((char*) &sComponentRole.cRole[0], AUDIO_DEC_MP3_ROLE);
        break;
    case OMX_AUDIO_CodingAAC:
        strcpy((char*) &sComponentRole.cRole[0], AUDIO_DEC_AAC_ROLE);
        break;
    case OMX_AUDIO_CodingVORBIS:
        strcpy((char*) &sComponentRole.cRole[0], AUDIO_DEC_OGG_ROLE);
        break;
    case OMX_AUDIO_CodingRA:
        strcpy((char*) &sComponentRole.cRole[0], AUDIO_DEC_RA_ROLE);
        break;
    case OMX_AUDIO_CodingWMA:
        strcpy((char*) &sComponentRole.cRole[0], AUDIO_DEC_WMA_ROLE);
        break;
    default:
        DEBUG(DEB_LEV_ERR, "Error only MP3 and AAC role are supported %08x\n", paramPortAudio.format.audio.eEncoding);
    }
    err = OMX_SetParameter(appPriv->audiodechandle, OMX_IndexParamStandardComponentRole, &sComponentRole);
    if (err != OMX_ErrorNone)
    {
        DEBUG(DEB_LEV_ERR, "\n error in input audio format - exiting\n");
        exit(1);
    }
#endif


    if (flagSetupTunnel)
    {
        /* bring back the audio and video decoder to Idle state*/

        err = OMX_SendCommand(appPriv->videodechandle, OMX_CommandStateSet, OMX_StateIdle, NULL);
#ifdef SUPPORT_AUDIO
        err = OMX_SendCommand(appPriv->audiodechandle, OMX_CommandStateSet, OMX_StateIdle, NULL);
#endif
        tsem_down(appPriv->videoDecoderEventSem);
#ifdef SUPPORT_AUDIO
        tsem_down(appPriv->audioDecoderEventSem);
#endif
        /* Enabling the ports after having set the parameters */
        err = OMX_SendCommand(appPriv->parser3gphandle, OMX_CommandPortEnable, 0, NULL);
        err = OMX_SendCommand(appPriv->parser3gphandle, OMX_CommandPortEnable, 1, NULL);
        err = OMX_SendCommand(appPriv->videodechandle, OMX_CommandPortEnable, -1, NULL);
#ifdef SUPPORT_AUDIO
        err = OMX_SendCommand(appPriv->audiodechandle, OMX_CommandPortEnable, -1, NULL);
#endif
        err = OMX_SendCommand(appPriv->colorconv_handle, OMX_CommandPortEnable, 0, NULL);
        err = OMX_SendCommand(appPriv->colorconv_handle, OMX_CommandPortEnable, 1, NULL);
        if (flagAVsync)
        {
            err = OMX_SendCommand(appPriv->videoschd_handle, OMX_CommandPortEnable, 0, NULL);
            err = OMX_SendCommand(appPriv->videoschd_handle, OMX_CommandPortEnable, 1, NULL);
        }
#ifdef SUPPORT_AUDIO
        err = OMX_SendCommand(appPriv->volumehandle, OMX_CommandPortEnable, 0, NULL);
        err = OMX_SendCommand(appPriv->volumehandle, OMX_CommandPortEnable, 1, NULL);
#endif
        err = OMX_SendCommand(appPriv->videosinkhandle, OMX_CommandPortEnable, 0, NULL);
#ifdef SUPPORT_AUDIO
        err = OMX_SendCommand(appPriv->audiosinkhandle, OMX_CommandPortEnable, 0, NULL);
#endif
        if (err != OMX_ErrorNone)
        {
            DEBUG(DEB_LEV_ERR, "audio sink port enable failed\n");
            exit(1);
        }
        /*Wait for Ports Enable Events*/
        tsem_down(appPriv->parser3gpEventSem);
        tsem_down(appPriv->parser3gpEventSem);
        tsem_down(appPriv->videoDecoderEventSem);
        tsem_down(appPriv->videoDecoderEventSem);
#ifdef SUPPORT_AUDIO
        tsem_down(appPriv->audioDecoderEventSem);
        tsem_down(appPriv->audioDecoderEventSem);
#endif
        tsem_down(appPriv->colorconvEventSem);
        tsem_down(appPriv->colorconvEventSem);
        if (flagAVsync)
        {
            tsem_down(appPriv->videoschdEventSem);
            tsem_down(appPriv->videoschdEventSem);
        }
#ifdef SUPPORT_AUDIO
        tsem_down(appPriv->volumeEventSem);
        tsem_down(appPriv->volumeEventSem);
#endif
        tsem_down(appPriv->fbdevSinkEventSem);
#ifdef SUPPORT_AUDIO
        tsem_down(appPriv->audioSinkEventSem);
#endif
        DEBUG(DEB_LEV_SIMPLE_SEQ, "All ports enabled\n");
    }

    /*Send State Change Idle command to Video  & Audio Decoder*/
    if (!flagSetupTunnel)
    {
        err = OMX_SendCommand(appPriv->videodechandle, OMX_CommandStateSet, OMX_StateIdle, NULL);

        /** the output buffers of parser 3gp component will be used
         *  in the video decoder component as input buffers
         */
        err = OMX_UseBuffer(appPriv->videodechandle, &inBufferVideoDec[0], 0, NULL, buffer_out_size, outBufferParseVideo[0]->pBuffer);
        if (err != OMX_ErrorNone)
        {
            DEBUG(DEB_LEV_ERR, "Unable to use the parser3gp comp allocate buffer\n");
            exit(1);
        }
        err = OMX_UseBuffer(appPriv->videodechandle, &inBufferVideoDec[1], 0, NULL, buffer_out_size, outBufferParseVideo[1]->pBuffer);
        if (err != OMX_ErrorNone)
        {
            DEBUG(DEB_LEV_ERR, "Unable to use the parser3gp comp allocate buffer\n");
            exit(1);
        }

        /* Allocate the output buffers of the video decoder */
        err = OMX_AllocateBuffer(appPriv->videodechandle, &outBufferVideoDec[0], 1, NULL, buffer_out_size);
        if (err != OMX_ErrorNone)
        {
            DEBUG(DEB_LEV_ERR, "Unable to allocate buffer in video dec\n");
            exit(1);
        }
        err = OMX_AllocateBuffer(appPriv->videodechandle, &outBufferVideoDec[1], 1, NULL, buffer_out_size);
        if (err != OMX_ErrorNone)
        {
            DEBUG(DEB_LEV_ERR, "Unable to allocate buffer in video dec\n");
            exit(1);
        }
#ifdef SUPPORT_AUDIO
        err = OMX_SendCommand(appPriv->audiodechandle, OMX_CommandStateSet, OMX_StateIdle, NULL);

        /** the output buffers of parser3gp component will be used
           in the audio decoder component as input buffers
         */
        err = OMX_UseBuffer(appPriv->audiodechandle, &inBufferAudioDec[0], 0, NULL, buffer_out_size, outBufferParseAudio[0]->pBuffer);
        if (err != OMX_ErrorNone)
        {
            DEBUG(DEB_LEV_ERR, "Unable to use the parser3gp allocate buffer\n");
            exit(1);
        }
        err = OMX_UseBuffer(appPriv->audiodechandle, &inBufferAudioDec[1], 0, NULL, buffer_out_size, outBufferParseAudio[1]->pBuffer);
        if (err != OMX_ErrorNone)
        {
            DEBUG(DEB_LEV_ERR, "Unable to use the parser3gp allocate buffer\n");
            exit(1);
        }

        /* Allocate the output buffers of the audio decoder */
        err = OMX_AllocateBuffer(appPriv->audiodechandle, &outBufferAudioDec[0], 1, NULL, buffer_out_size);
        if (err != OMX_ErrorNone)
        {
            DEBUG(DEB_LEV_ERR, "Unable to allocate buffer in audio dec\n");
            exit(1);
        }
        err = OMX_AllocateBuffer(appPriv->audiodechandle, &outBufferAudioDec[1], 1, NULL, buffer_out_size);
        if (err != OMX_ErrorNone)
        {
            DEBUG(DEB_LEV_ERR, "Unable to allocate buffer in audio dec\n");
            exit(1);
        }
#endif
        /*Wait for audio and video decoder state change to idle*/
        tsem_down(appPriv->videoDecoderEventSem);
#ifdef SUPPORT_AUDIO
        tsem_down(appPriv->audioDecoderEventSem);
#endif
        DEBUG(DEFAULT_MESSAGES, " audio and video decoder transitioned to  idle state \n");
    }

    /*  color conv and sink options */
    if (flagIsDisplayRequested && (!flagSetupTunnel))
    {
        err = OMX_SendCommand(appPriv->colorconv_handle, OMX_CommandStateSet, OMX_StateIdle, NULL);

        err = OMX_UseBuffer(appPriv->colorconv_handle, &inBufferColorconv[0], 0, NULL, buffer_out_size, outBufferVideoDec[0]->pBuffer);
        if (err != OMX_ErrorNone)
        {
            DEBUG(DEB_LEV_ERR, "Unable to use the allocated buffer\n");
            exit(1);
        }
        err = OMX_UseBuffer(appPriv->colorconv_handle, &inBufferColorconv[1], 0, NULL, buffer_out_size, outBufferVideoDec[1]->pBuffer);
        if (err != OMX_ErrorNone)
        {
            DEBUG(DEB_LEV_ERR, "Unable to use the allocated buffer\n");
            exit(1);
        }

        err = OMX_AllocateBuffer(appPriv->colorconv_handle, &outBufferColorconv[0], 1, NULL, buffer_out_size);
        if (err != OMX_ErrorNone)
        {
            DEBUG(DEB_LEV_ERR, "Unable to allocate buffer in Color Converter 1\n");
            exit(1);
        }
        err = OMX_AllocateBuffer(appPriv->colorconv_handle, &outBufferColorconv[1], 1, NULL, buffer_out_size);
        if (err != OMX_ErrorNone)
        {
            DEBUG(DEB_LEV_ERR, "Unable to allocate buffer in Color Converter 2\n");
            exit(1);
        }

        /* wait for color converter to be in Idle state */
        tsem_down(appPriv->colorconvEventSem);
        DEBUG(DEB_LEV_SIMPLE_SEQ, "color converter state idle\n");
#ifdef SUPPORT_AUDIO
        /* volume control options */
        err = OMX_SendCommand(appPriv->volumehandle, OMX_CommandStateSet, OMX_StateIdle, NULL);

        err = OMX_UseBuffer(appPriv->volumehandle, &inBufferVolume[0], 0, NULL, buffer_out_size, outBufferAudioDec[0]->pBuffer);
        if (err != OMX_ErrorNone)
        {
            DEBUG(DEB_LEV_ERR, "Unable to use the allocated buffer\n");
            exit(1);
        }
        err = OMX_UseBuffer(appPriv->volumehandle, &inBufferVolume[1], 0, NULL, buffer_out_size, outBufferAudioDec[1]->pBuffer);
        if (err != OMX_ErrorNone)
        {
            DEBUG(DEB_LEV_ERR, "Unable to use the allocated buffer\n");
            exit(1);
        }

        err = OMX_AllocateBuffer(appPriv->volumehandle, &outBufferVolume[0], 1, NULL, buffer_out_size);
        if (err != OMX_ErrorNone)
        {
            DEBUG(DEB_LEV_ERR, "Unable to allocate buffer in volume 1\n");
            exit(1);
        }
        err = OMX_AllocateBuffer(appPriv->volumehandle, &outBufferVolume[1], 1, NULL, buffer_out_size);
        if (err != OMX_ErrorNone)
        {
            DEBUG(DEB_LEV_ERR, "Unable to allocate buffer in volume 2\n");
            exit(1);
        }

        tsem_down(appPriv->volumeEventSem);
        DEBUG(DEB_LEV_SIMPLE_SEQ, "volume state idle\n");
#endif
        /* audio video sink and clock options */
        if (flagAVsync)
        {
            /* put clock src into Idle state*/
            err = OMX_SendCommand(appPriv->clocksrchandle, OMX_CommandStateSet, OMX_StateIdle, NULL);

            /* video scheduler options */
            err = OMX_SendCommand(appPriv->videoschd_handle, OMX_CommandStateSet, OMX_StateIdle, NULL);
        }

        err = OMX_SendCommand(appPriv->videosinkhandle, OMX_CommandStateSet, OMX_StateIdle, NULL);
#ifdef SUPPORT_AUDIO
        err = OMX_SendCommand(appPriv->audiosinkhandle, OMX_CommandStateSet, OMX_StateIdle, NULL);
#endif
        if (flagAVsync)
        {
            err = OMX_UseBuffer(appPriv->videoschd_handle, &inBufferVideoSchd[0], 0, NULL, buffer_out_size, outBufferColorconv[0]->pBuffer);
            if (err != OMX_ErrorNone)
            {
                DEBUG(DEB_LEV_ERR, "Unable to use the allocated buffer\n");
                exit(1);
            }
            err = OMX_UseBuffer(appPriv->videoschd_handle, &inBufferVideoSchd[1], 0, NULL, buffer_out_size, outBufferColorconv[1]->pBuffer);
            if (err != OMX_ErrorNone)
            {
                DEBUG(DEB_LEV_ERR, "Unable to use the allocated buffer\n");
                exit(1);
            }

            err = OMX_AllocateBuffer(appPriv->videoschd_handle, &outBufferVideoSchd[0], 1, NULL, buffer_out_size);
            if (err != OMX_ErrorNone)
            {
                DEBUG(DEB_LEV_ERR, "Unable to allocate buffer in Video Scheduler 1\n");
                exit(1);
            }
            err = OMX_AllocateBuffer(appPriv->videoschd_handle, &outBufferVideoSchd[1], 1, NULL, buffer_out_size);
            if (err != OMX_ErrorNone)
            {
                DEBUG(DEB_LEV_ERR, "Unable to allocate buffer in Video Scheduler 2\n");
                exit(1);
            }

            err = OMX_UseBuffer(appPriv->videosinkhandle, &inBufferSinkVideo[0], 0, NULL, buffer_out_size, outBufferVideoSchd[0]->pBuffer);
            if (err != OMX_ErrorNone)
            {
                DEBUG(DEB_LEV_ERR, "Unable to use the allocated buffer\n");
                exit(1);
            }

            err = OMX_UseBuffer(appPriv->videosinkhandle, &inBufferSinkVideo[1], 0, NULL, buffer_out_size, outBufferVideoSchd[1]->pBuffer);
            if (err != OMX_ErrorNone)
            {
                DEBUG(DEB_LEV_ERR, "Unable to use the allocated buffer\n");
                exit(1);
            }
        }
        else
        {
            err = OMX_UseBuffer(appPriv->videosinkhandle, &inBufferSinkVideo[0], 0, NULL, buffer_out_size, outBufferColorconv[0]->pBuffer);
            if (err != OMX_ErrorNone)
            {
                DEBUG(DEB_LEV_ERR, "Unable to use the allocated buffer\n");
                exit(1);
            }
            err = OMX_UseBuffer(appPriv->videosinkhandle, &inBufferSinkVideo[1], 0, NULL, buffer_out_size, outBufferColorconv[1]->pBuffer);
            if (err != OMX_ErrorNone)
            {
                DEBUG(DEB_LEV_ERR, "Unable to use the allocated buffer\n");
                exit(1);
            }
        }
#ifdef SUPPORT_AUDIO
        err = OMX_UseBuffer(appPriv->audiosinkhandle, &inBufferSinkAudio[0], 0, NULL, buffer_out_size, outBufferVolume[0]->pBuffer);
        if (err != OMX_ErrorNone)
        {
            DEBUG(DEB_LEV_ERR, "Unable to use the allocated buffer\n");
            exit(1);
        }
        err = OMX_UseBuffer(appPriv->audiosinkhandle, &inBufferSinkAudio[1], 0, NULL, buffer_out_size, outBufferVolume[1]->pBuffer);
        if (err != OMX_ErrorNone)
        {
            DEBUG(DEB_LEV_ERR, "Unable to use the allocated buffer\n");
            exit(1);
        }
#endif
        if (flagAVsync)
        {
            tsem_down(appPriv->videoschdEventSem);
            DEBUG(DEB_LEV_SIMPLE_SEQ, "video scheduler state idle\n");
            DEBUG(DEFAULT_MESSAGES, "video scheduler state idle\n");
            tsem_down(appPriv->clockEventSem);
            DEBUG(DEFAULT_MESSAGES, "clock src  state idle\n");
        }
        tsem_down(appPriv->fbdevSinkEventSem);
        DEBUG(DEFAULT_MESSAGES, "video sink state idle\n");
#ifdef SUPPORT_AUDIO
        tsem_down(appPriv->audioSinkEventSem);
        DEBUG(DEFAULT_MESSAGES, "audio sink state idle\n");
#endif
    }

    if (flagAVsync && !flagSetupTunnel)
    {
        /* enabling the clock port of the clocksrc connected to the parser and parser's clock port */
        err = OMX_SendCommand(appPriv->parser3gphandle, OMX_CommandPortEnable, 2, NULL);
        err = OMX_SendCommand(appPriv->clocksrchandle, OMX_CommandPortEnable, 2, NULL);
        /*Wait for Ports Enable Events*/
        tsem_down(appPriv->parser3gpEventSem);
        tsem_down(appPriv->clockEventSem);
    }

    err = OMX_SendCommand(appPriv->videodechandle, OMX_CommandStateSet, OMX_StateExecuting, NULL);
    if (err != OMX_ErrorNone)
    {
        DEBUG(DEB_LEV_ERR, "video decoder state executing failed\n");
        exit(1);
    }
    tsem_down(appPriv->videoDecoderEventSem);
#ifdef SUPPORT_AUDIO
    err = OMX_SendCommand(appPriv->audiodechandle, OMX_CommandStateSet, OMX_StateExecuting, NULL);
    if (err != OMX_ErrorNone)
    {
        DEBUG(DEB_LEV_ERR, "audio decoder state executing failed\n");
        exit(1);
    }

    /*Wait for decoder state change to executing*/
    tsem_down(appPriv->audioDecoderEventSem);
#endif
    /* putting the clock src in Executing state */
    if (flagAVsync)
    {
        err = OMX_SendCommand(appPriv->clocksrchandle, OMX_CommandStateSet, OMX_StateExecuting, NULL);
        if (err != OMX_ErrorNone)
        {
            DEBUG(DEB_LEV_ERR, "clock src state executing failed\n");
            exit(1);
        }

        DEBUG(DEB_LEV_SIMPLE_SEQ, "waiting for  clock src state executing\n");
        tsem_down(appPriv->clockEventSem);
        DEBUG(DEFAULT_MESSAGES, "clock src in executing state\n");

        /* set the audio as the master */
        setHeader(&sRefClock, sizeof (OMX_TIME_CONFIG_ACTIVEREFCLOCKTYPE));
        sRefClock.eClock = OMX_TIME_RefClockAudio;
        OMX_SetConfig(appPriv->clocksrchandle, OMX_IndexConfigTimeActiveRefClock, &sRefClock);

        /* set the clock state to OMX_TIME_ClockStateWaitingForStartTime */
        setHeader(&sClockState, sizeof (OMX_TIME_CONFIG_CLOCKSTATETYPE));
        err = OMX_GetConfig(appPriv->clocksrchandle, OMX_IndexConfigTimeClockState, &sClockState);
        sClockState.nWaitMask = OMX_CLOCKPORT1 || OMX_CLOCKPORT0; /* wait for audio and video start time */
        sClockState.eState = OMX_TIME_ClockStateWaitingForStartTime;
        err = OMX_SetConfig(appPriv->clocksrchandle, OMX_IndexConfigTimeClockState, &sClockState);
        if (err != OMX_ErrorNone)
        {
            DEBUG(DEB_LEV_ERR, "Error %08x In OMX_SetConfig \n", err);
            exit(1);
        }
        err = OMX_GetConfig(appPriv->clocksrchandle, OMX_IndexConfigTimeClockState, &sClockState);
    }

    if (flagIsDisplayRequested)
    {
        err = OMX_SendCommand(appPriv->colorconv_handle, OMX_CommandStateSet, OMX_StateExecuting, NULL);
        if (err != OMX_ErrorNone)
        {
            DEBUG(DEB_LEV_ERR, "color converter state executing failed\n");
            exit(1);
        }
        DEBUG(DEB_LEV_SIMPLE_SEQ, "waiting for  color converter state executing\n");
        tsem_down(appPriv->colorconvEventSem);

        if (flagAVsync)
        {
            err = OMX_SendCommand(appPriv->videoschd_handle, OMX_CommandStateSet, OMX_StateExecuting, NULL);
            if (err != OMX_ErrorNone)
            {
                DEBUG(DEB_LEV_ERR, "video scheduler state executing failed\n");
                exit(1);
            }
            DEBUG(DEB_LEV_SIMPLE_SEQ, "waiting for  video scheduler state executing\n");
            tsem_down(appPriv->videoschdEventSem);
        }

        DEBUG(DEB_LEV_SIMPLE_SEQ, "sending video sink state to executing\n");
        err = OMX_SendCommand(appPriv->videosinkhandle, OMX_CommandStateSet, OMX_StateExecuting, NULL);
        if (err != OMX_ErrorNone)
        {
            DEBUG(DEB_LEV_ERR, "video sink state executing failed\n");
            exit(1);
        }
        DEBUG(DEB_LEV_SIMPLE_SEQ, "waiting for  video sink state executing\n");
        tsem_down(appPriv->fbdevSinkEventSem);
        DEBUG(DEB_LEV_SIMPLE_SEQ, "video sink state executing successful\n");
#ifdef SUPPORT_AUDIO
        err = OMX_SendCommand(appPriv->volumehandle, OMX_CommandStateSet, OMX_StateExecuting, NULL);
        if (err != OMX_ErrorNone)
        {
            DEBUG(DEB_LEV_ERR, "volume state executing failed\n");
            exit(1);
        }
        DEBUG(DEB_LEV_SIMPLE_SEQ, "waiting for  volume state executing\n");
        tsem_down(appPriv->volumeEventSem);

        DEBUG(DEB_LEV_SIMPLE_SEQ, "sending audio sink state executing\n");
        err = OMX_SendCommand(appPriv->audiosinkhandle, OMX_CommandStateSet, OMX_StateExecuting, NULL);
        if (err != OMX_ErrorNone)
        {
            DEBUG(DEB_LEV_ERR, "audio sink state executing failed\n");
            exit(1);
        }
        DEBUG(DEB_LEV_SIMPLE_SEQ, "waiting for  audio sink state executing\n");
        tsem_down(appPriv->audioSinkEventSem);
        DEBUG(DEB_LEV_SIMPLE_SEQ, "audio sink state executing successful\n");
#endif
    }

    DEBUG(DEFAULT_MESSAGES, "All Component state changed to Executing\n");

    if (!flagSetupTunnel)
    {
        err = OMX_FillThisBuffer(appPriv->parser3gphandle, outBufferParseVideo[0]);
        if (err != OMX_ErrorNone)
        {
            DEBUG(DEB_LEV_ERR, "In %s Error %08x Calling FillThisBuffer Parser3gp \n", __func__, err);
            exit(1);
        }
        DEBUG(DEB_LEV_PARAMS, "Fill parser second buffer %x\n", (int) outBufferParseVideo[1]);
        err = OMX_FillThisBuffer(appPriv->parser3gphandle, outBufferParseVideo[1]);
        if (err != OMX_ErrorNone)
        {
            DEBUG(DEB_LEV_ERR, "In %s Error %08x Calling FillThisBuffer Parser3gp\n", __func__, err);
            exit(1);
        }

        err = OMX_FillThisBuffer(appPriv->parser3gphandle, outBufferParseAudio[0]);
        if (err != OMX_ErrorNone)
        {
            DEBUG(DEB_LEV_ERR, "In %s Error %08x Calling FillThisBuffer Parser3gp \n", __func__, err);
            exit(1);
        }
        DEBUG(DEB_LEV_PARAMS, "Fill parser second buffer %x\n", (int) outBufferParseAudio[1]);
        err = OMX_FillThisBuffer(appPriv->parser3gphandle, outBufferParseAudio[1]);
        if (err != OMX_ErrorNone)
        {
            DEBUG(DEB_LEV_ERR, "In %s Error %08x Calling FillThisBuffer Parser3gp\n", __func__, err);
            exit(1);
        }

        err = OMX_FillThisBuffer(appPriv->videodechandle, outBufferVideoDec[0]);
        if (err != OMX_ErrorNone)
        {
            DEBUG(DEB_LEV_ERR, "In %s Error %08x Calling FillThisBuffer Video Dec\n", __func__, err);
            exit(1);
        }
        DEBUG(DEB_LEV_PARAMS, "Fill decoder second buffer %x\n", (int) outBufferVideoDec[1]);
        err = OMX_FillThisBuffer(appPriv->videodechandle, outBufferVideoDec[1]);
        if (err != OMX_ErrorNone)
        {
            DEBUG(DEB_LEV_ERR, "In %s Error %08x Calling FillThisBuffer Video Dec\n", __func__, err);
            exit(1);
        }
#ifdef SUPPORT_AUDIO
        err = OMX_FillThisBuffer(appPriv->audiodechandle, outBufferAudioDec[0]);
        if (err != OMX_ErrorNone)
        {
            DEBUG(DEB_LEV_ERR, "In %s Error %08x Calling FillThisBuffer Audio Dec\n", __func__, err);
            exit(1);
        }
        DEBUG(DEB_LEV_PARAMS, "Fill decoder second buffer %x\n", (int) outBufferAudioDec[1]);
        err = OMX_FillThisBuffer(appPriv->audiodechandle, outBufferAudioDec[1]);
        if (err != OMX_ErrorNone)
        {
            DEBUG(DEB_LEV_ERR, "In %s Error %08x Calling FillThisBuffer Audio Dec\n", __func__, err);
            exit(1);
        }
#endif
        if (flagIsDisplayRequested)
        {
            err = OMX_FillThisBuffer(appPriv->colorconv_handle, outBufferColorconv[0]);
            if (err != OMX_ErrorNone)
            {
                DEBUG(DEB_LEV_ERR, "In %s Error %08x Calling FillThisBuffer Color converter\n", __func__, err);
                exit(1);
            }
            DEBUG(DEB_LEV_PARAMS, "Fill decoder second buffer %x\n", (int) outBufferColorconv[1]);
            err = OMX_FillThisBuffer(appPriv->colorconv_handle, outBufferColorconv[1]);
            if (err != OMX_ErrorNone)
            {
                DEBUG(DEB_LEV_ERR, "In %s Error %08x Calling FillThisBuffer Color Converter\n", __func__, err);
                exit(1);
            }

            if (flagAVsync)
            {
                err = OMX_FillThisBuffer(appPriv->videoschd_handle, outBufferVideoSchd[0]);
                if (err != OMX_ErrorNone)
                {
                    DEBUG(DEB_LEV_ERR, "In %s Error %08x Calling FillThisBuffer Video Scheduler\n", __func__, err);
                    exit(1);
                }
                DEBUG(DEB_LEV_PARAMS, "Fill video scheduler second buffer %x\n", (int) outBufferVideoSchd[1]);
                err = OMX_FillThisBuffer(appPriv->videoschd_handle, outBufferVideoSchd[1]);
                if (err != OMX_ErrorNone)
                {
                    DEBUG(DEB_LEV_ERR, "In %s Error %08x Calling FillThisBuffer Video Scheduler\n", __func__, err);
                    exit(1);
                }
            }
#ifdef SUPPORT_AUDIO
            err = OMX_FillThisBuffer(appPriv->volumehandle, outBufferVolume[0]);
            if (err != OMX_ErrorNone)
            {
                DEBUG(DEB_LEV_ERR, "In %s Error %08x Calling FillThisBuffer Audio Dec\n", __func__, err);
                exit(1);
            }
            DEBUG(DEB_LEV_PARAMS, "Fill decoder second buffer %x\n", (int) outBufferVolume[1]);
            err = OMX_FillThisBuffer(appPriv->volumehandle, outBufferVolume[1]);
            if (err != OMX_ErrorNone)
            {
                DEBUG(DEB_LEV_ERR, "In %s Error %08x Calling FillThisBuffer Audio Dec\n", __func__, err);
                exit(1);
            }
#endif
        }
    }

    setHeader(&sConfigScale, sizeof (OMX_TIME_CONFIG_SCALETYPE));
    if (flagAVsync)
    {
        DEBUG(DEFAULT_MESSAGES, "--------------------------\n");
        DEBUG(DEFAULT_MESSAGES, "Enter F : fastforward \n");
        DEBUG(DEFAULT_MESSAGES, "Enter R : Rewind      \n");
        DEBUG(DEFAULT_MESSAGES, "Enter P : Pause      \n");
        DEBUG(DEFAULT_MESSAGES, "Enter N : Normal Play      \n");
        DEBUG(DEFAULT_MESSAGES, "Enter Q : Quit       \n");
        DEBUG(DEFAULT_MESSAGES, "--------------------------\n");
        while (1)
        {
            keyin = toupper(getchar());
            if (keyin == 'Q' || keyin == 'q')
            {
                DEBUG(DEFAULT_MESSAGES, "Quitting \n");
                bEOS = OMX_TRUE;
                break;
            }
            else
            {
                switch (keyin)
                {
                case 'F':
                case 'f':
                    newscale = 2 << 16; // Q16 format is used
                    DEBUG(DEFAULT_MESSAGES, "Fast forwarding .......\n");
                    break;
                case 'R':
                case 'r':
                    newscale = -2 << 16; // Q16 format is used
                    DEBUG(DEFAULT_MESSAGES, "Rewinding ........\n");
                    break;
                case 'P':
                case 'p':
                    newscale = 0;
                    DEBUG(DEFAULT_MESSAGES, "Pause ......\n");
                    break;
                case 'N':
                case 'n':
                    newscale = 1 << 16; // Q16 format is used
                    DEBUG(DEFAULT_MESSAGES, "Normal Play ......\n");
                    break;
                default:
                    break;
                }
                sConfigScale.xScale = newscale;
                /* send the scale change notification to the clock */
                err = OMX_SetConfig(appPriv->clocksrchandle, OMX_IndexConfigTimeScale, &sConfigScale);
            }
        }
    }

    DEBUG(DEFAULT_MESSAGES, "Waiting for  EOS = %d\n", appPriv->eofSem->semval);

    if (bEOS == OMX_FALSE)
    {
        ////tsem_up(appPriv->eofSem);
        tsem_down(appPriv->eofSem);
    }

    DEBUG(DEFAULT_MESSAGES, "Received EOS \n");

    /*Send Idle Command to all components*/
    if (flagAVsync)
    {
        err = OMX_SendCommand(appPriv->clocksrchandle, OMX_CommandStateSet, OMX_StateIdle, NULL);
    }


    DEBUG(DEFAULT_MESSAGES, "The execution of the decoding process is terminated\n");
    err = OMX_SendCommand(appPriv->parser3gphandle, OMX_CommandStateSet, OMX_StateIdle, NULL);
    err = OMX_SendCommand(appPriv->videodechandle, OMX_CommandStateSet, OMX_StateIdle, NULL);
#ifdef SUPPORT_AUDIO
    err = OMX_SendCommand(appPriv->audiodechandle, OMX_CommandStateSet, OMX_StateIdle, NULL);
#endif
    if (flagIsDisplayRequested)
    {
        err = OMX_SendCommand(appPriv->colorconv_handle, OMX_CommandStateSet, OMX_StateIdle, NULL);
        if (flagAVsync)
        {
            //      err = OMX_SendCommand(appPriv->clocksrchandle, OMX_CommandStateSet, OMX_StateIdle, NULL);
            err = OMX_SendCommand(appPriv->videoschd_handle, OMX_CommandStateSet, OMX_StateIdle, NULL);
        }
        err = OMX_SendCommand(appPriv->videosinkhandle, OMX_CommandStateSet, OMX_StateIdle, NULL);
#ifdef SUPPORT_AUDIO
        err = OMX_SendCommand(appPriv->volumehandle, OMX_CommandStateSet, OMX_StateIdle, NULL);
        err = OMX_SendCommand(appPriv->audiosinkhandle, OMX_CommandStateSet, OMX_StateIdle, NULL);
#endif
    }

    tsem_down(appPriv->parser3gpEventSem);
    DEBUG(DEFAULT_MESSAGES, "Parser3gp idle state \n");
    tsem_down(appPriv->videoDecoderEventSem);
#ifdef SUPPORT_AUDIO
    tsem_down(appPriv->audioDecoderEventSem);
#endif
    DEBUG(DEFAULT_MESSAGES, "Audio & Video decoder in idle state \n");

    if (flagIsDisplayRequested)
    {
        tsem_down(appPriv->colorconvEventSem);
        DEBUG(DEFAULT_MESSAGES, "color converter in idle state \n");
        if (flagAVsync)
        {
            tsem_down(appPriv->videoschdEventSem);
            DEBUG(DEFAULT_MESSAGES, "video scheduler in idle state \n");
            //      tsem_down(appPriv->clockEventSem);
        }
#ifdef SUPPORT_AUDIO
        tsem_down(appPriv->volumeEventSem);
#endif
        tsem_down(appPriv->fbdevSinkEventSem);
#ifdef SUPPORT_AUDIO
        tsem_down(appPriv->audioSinkEventSem);
#endif
    }

    if (flagAVsync)
    {
        tsem_down(appPriv->clockEventSem);
    }

    DEBUG(DEFAULT_MESSAGES, "All component Transitioned to Idle\n");
    /*Send Loaded Command to all components*/
    err = OMX_SendCommand(appPriv->parser3gphandle, OMX_CommandStateSet, OMX_StateLoaded, NULL);
    err = OMX_SendCommand(appPriv->videodechandle, OMX_CommandStateSet, OMX_StateLoaded, NULL);
#ifdef SUPPORT_AUDIO
    err = OMX_SendCommand(appPriv->audiodechandle, OMX_CommandStateSet, OMX_StateLoaded, NULL);
#endif
    if (flagIsDisplayRequested)
    {
        err = OMX_SendCommand(appPriv->colorconv_handle, OMX_CommandStateSet, OMX_StateLoaded, NULL);
        if (flagAVsync)
        {
            err = OMX_SendCommand(appPriv->videoschd_handle, OMX_CommandStateSet, OMX_StateLoaded, NULL);
        }
        err = OMX_SendCommand(appPriv->videosinkhandle, OMX_CommandStateSet, OMX_StateLoaded, NULL);
#ifdef SUPPORT_AUDIO
        err = OMX_SendCommand(appPriv->volumehandle, OMX_CommandStateSet, OMX_StateLoaded, NULL);
        err = OMX_SendCommand(appPriv->audiosinkhandle, OMX_CommandStateSet, OMX_StateLoaded, NULL);
#endif
    }

    if (flagAVsync)
    {
        err = OMX_SendCommand(appPriv->clocksrchandle, OMX_CommandStateSet, OMX_StateLoaded, NULL);
    }

    DEBUG(DEFAULT_MESSAGES, "All components to loaded\n");

    /*Free input buffers if components are not tunnelled*/
    if (!flagSetupTunnel)
    {
        err = OMX_FreeBuffer(appPriv->videodechandle, 0, inBufferVideoDec[0]);
        err = OMX_FreeBuffer(appPriv->videodechandle, 0, inBufferVideoDec[1]);
#ifdef SUPPORT_AUDIO
        err = OMX_FreeBuffer(appPriv->audiodechandle, 0, inBufferAudioDec[0]);
        err = OMX_FreeBuffer(appPriv->audiodechandle, 0, inBufferAudioDec[1]);
#endif
        DEBUG(DEB_LEV_PARAMS, "Freeing decoder output ports\n");
        err = OMX_FreeBuffer(appPriv->videodechandle, 1, outBufferVideoDec[0]);
        err = OMX_FreeBuffer(appPriv->videodechandle, 1, outBufferVideoDec[1]);
#ifdef SUPPORT_AUDIO
        err = OMX_FreeBuffer(appPriv->audiodechandle, 1, outBufferAudioDec[0]);
        err = OMX_FreeBuffer(appPriv->audiodechandle, 1, outBufferAudioDec[1]);
#endif
        err = OMX_FreeBuffer(appPriv->parser3gphandle, VIDEO_PORT_INDEX, outBufferParseVideo[0]);
        err = OMX_FreeBuffer(appPriv->parser3gphandle, VIDEO_PORT_INDEX, outBufferParseVideo[1]);
        err = OMX_FreeBuffer(appPriv->parser3gphandle, AUDIO_PORT_INDEX, outBufferParseAudio[0]);
        err = OMX_FreeBuffer(appPriv->parser3gphandle, AUDIO_PORT_INDEX, outBufferParseAudio[1]);
    }

    if (flagIsDisplayRequested && (!flagSetupTunnel))
    {
        err = OMX_FreeBuffer(appPriv->colorconv_handle, 0, inBufferColorconv[0]);
        err = OMX_FreeBuffer(appPriv->colorconv_handle, 0, inBufferColorconv[1]);
        err = OMX_FreeBuffer(appPriv->colorconv_handle, 1, outBufferColorconv[0]);
        err = OMX_FreeBuffer(appPriv->colorconv_handle, 1, outBufferColorconv[1]);

        if (flagAVsync)
        {
            err = OMX_FreeBuffer(appPriv->videoschd_handle, 0, inBufferVideoSchd[0]);
            err = OMX_FreeBuffer(appPriv->videoschd_handle, 0, inBufferVideoSchd[1]);
            err = OMX_FreeBuffer(appPriv->videoschd_handle, 1, outBufferVideoSchd[0]);
            err = OMX_FreeBuffer(appPriv->videoschd_handle, 1, outBufferVideoSchd[1]);
        }
#ifdef SUPPORT_AUDIO
        err = OMX_FreeBuffer(appPriv->volumehandle, 0, inBufferVolume[0]);
        err = OMX_FreeBuffer(appPriv->volumehandle, 0, inBufferVolume[1]);
        err = OMX_FreeBuffer(appPriv->volumehandle, 1, outBufferVolume[0]);
        err = OMX_FreeBuffer(appPriv->volumehandle, 1, outBufferVolume[1]);
#endif
        err = OMX_FreeBuffer(appPriv->videosinkhandle, 0, inBufferSinkVideo[0]);
        err = OMX_FreeBuffer(appPriv->videosinkhandle, 0, inBufferSinkVideo[1]);
#ifdef SUPPORT_AUDIO
        err = OMX_FreeBuffer(appPriv->audiosinkhandle, 0, inBufferSinkAudio[0]);
        err = OMX_FreeBuffer(appPriv->audiosinkhandle, 0, inBufferSinkAudio[1]);
#endif
    }

    tsem_down(appPriv->parser3gpEventSem);
    DEBUG(DEFAULT_MESSAGES, "Parser3gp loaded state \n");
    tsem_down(appPriv->videoDecoderEventSem);
#ifdef SUPPORT_AUDIO
    tsem_down(appPriv->audioDecoderEventSem);
#endif

    if (flagIsDisplayRequested)
    {
        tsem_down(appPriv->colorconvEventSem);
        if (flagAVsync)
        {
            tsem_down(appPriv->videoschdEventSem);
        }
        tsem_down(appPriv->fbdevSinkEventSem);
#ifdef SUPPORT_AUDIO
        tsem_down(appPriv->volumeEventSem);
        tsem_down(appPriv->audioSinkEventSem);
#endif
    }

    if (flagAVsync)
    {
        tsem_down(appPriv->clockEventSem);
    }

    DEBUG(DEFAULT_MESSAGES, "All components released\n");

    /** freeing all handles and deinit omx */
    OMX_FreeHandle(appPriv->videodechandle);
    DEBUG(DEB_LEV_SIMPLE_SEQ, "videodec freed\n");
#ifdef SUPPORT_AUDIO
    OMX_FreeHandle(appPriv->audiodechandle);
    DEBUG(DEB_LEV_SIMPLE_SEQ, "audiodec dec freed\n");
#endif
    OMX_FreeHandle(appPriv->parser3gphandle);
    DEBUG(DEB_LEV_SIMPLE_SEQ, "parser3gp freed\n");

    if (flagIsDisplayRequested)
    {
        OMX_FreeHandle(appPriv->colorconv_handle);
        DEBUG(DEB_LEV_SIMPLE_SEQ, "color converter component freed\n");
        if (flagAVsync)
        {
            OMX_FreeHandle(appPriv->videoschd_handle);
            DEBUG(DEB_LEV_SIMPLE_SEQ, "video scheduler component freed\n");
        }
        OMX_FreeHandle(appPriv->videosinkhandle);
        
        DEBUG(DEB_LEV_SIMPLE_SEQ, "videosink freed\n");
#ifdef SUPPORT_AUDIO
        OMX_FreeHandle(appPriv->volumehandle);
        DEBUG(DEB_LEV_SIMPLE_SEQ, "volume component freed\n");
        OMX_FreeHandle(appPriv->audiosinkhandle);
        DEBUG(DEB_LEV_SIMPLE_SEQ, "audiosink freed\n");
#endif
    }

    if (flagAVsync)
    {
        OMX_FreeHandle(appPriv->clocksrchandle);
        DEBUG(DEB_LEV_SIMPLE_SEQ, "clock src freed\n");
    }

    OMX_Deinit();

    DEBUG(DEB_LEV_SIMPLE_SEQ, "All components freed. Closing...\n");

    free(appPriv->parser3gpEventSem);
    appPriv->parser3gpEventSem = NULL;

    free(appPriv->videoDecoderEventSem);
    appPriv->videoDecoderEventSem = NULL;
#ifdef SUPPORT_AUDIO
    free(appPriv->audioDecoderEventSem);
    appPriv->audioDecoderEventSem = NULL;
#endif
    if (flagIsDisplayRequested)
    {
        free(appPriv->colorconvEventSem);
        appPriv->colorconvEventSem = NULL;

        if (flagAVsync)
        {
            free(appPriv->videoschdEventSem);
            appPriv->videoschdEventSem = NULL;
        }

        free(appPriv->fbdevSinkEventSem);
        appPriv->fbdevSinkEventSem = NULL;
#ifdef SUPPORT_AUDIO
        free(appPriv->volumeEventSem);
        appPriv->volumeEventSem = NULL;

        free(appPriv->audioSinkEventSem);
        appPriv->audioSinkEventSem = NULL;
#endif
    }

    if (flagAVsync)
    {
        free(appPriv->clockEventSem);
    }

    free(appPriv->eofSem);
    appPriv->eofSem = NULL;
    free(appPriv);
    appPriv = NULL;

    if (!flagIsDisplayRequested)
    {

        fclose(outfileAudio);
        fclose(outfileVideo);
    }

    free(input_file);
    free(output_file_audio);
    free(output_file_video);

    return 0;
}

/* Callbacks implementation */
OMX_ERRORTYPE parser3gpEventHandler(
                                    OMX_HANDLETYPE hComponent,
                                    OMX_PTR pAppData,
                                    OMX_EVENTTYPE eEvent,
                                    OMX_U32 Data1,
                                    OMX_U32 Data2,
                                    OMX_PTR pEventData)
{
    OMX_ERRORTYPE err;
    DEBUG(DEB_LEV_SIMPLE_SEQ, "Hi there, I am in the %s callback\n", __func__);

    if (eEvent == OMX_EventCmdComplete)
    {
        if (Data1 == OMX_CommandStateSet)
        {
            DEBUG(DEB_LEV_SIMPLE_SEQ, "Parser 3gp State changed in ");
            switch ((int) Data2)
            {
            case OMX_StateInvalid:
                DEBUG(DEB_LEV_SIMPLE_SEQ, "OMX_StateInvalid\n");
                break;
            case OMX_StateLoaded:
                DEBUG(DEB_LEV_SIMPLE_SEQ, "OMX_StateLoaded\n");
                break;
            case OMX_StateIdle:
                DEBUG(DEB_LEV_SIMPLE_SEQ, "OMX_StateIdle\n");
                break;
            case OMX_StateExecuting:
                DEBUG(DEB_LEV_SIMPLE_SEQ, "OMX_StateExecuting\n");
                break;
            case OMX_StatePause:
                DEBUG(DEB_LEV_SIMPLE_SEQ, "OMX_StatePause\n");
                break;
            case OMX_StateWaitForResources:
                DEBUG(DEB_LEV_SIMPLE_SEQ, "OMX_StateWaitForResources\n");
                break;
            }
            tsem_up(appPriv->parser3gpEventSem);
        }
        else if (Data1 == OMX_CommandPortEnable)
        {
            DEBUG(DEB_LEV_SIMPLE_SEQ, "In %s Received Port Enable  Event\n", __func__);
            tsem_up(appPriv->parser3gpEventSem);
        }
        else if (Data1 == OMX_CommandPortDisable)
        {
            DEBUG(DEB_LEV_SIMPLE_SEQ, "In %s Received Port Disable Event\n", __func__);
            tsem_up(appPriv->parser3gpEventSem);
        }
        else if (Data1 == OMX_CommandFlush)
        {
            DEBUG(DEB_LEV_SIMPLE_SEQ, "In %s Received Port Flush Event\n", __func__);
            tsem_up(appPriv->parser3gpEventSem);
        }
        else
        {
            DEBUG(DEB_LEV_SIMPLE_SEQ, "In %s Received Event Event=%d Data1=%d,Data2=%d\n", __func__, eEvent, (int) Data1, (int) Data2);
        }
    }
    else if (eEvent == OMX_EventPortSettingsChanged)
    {
        DEBUG(DEB_LEV_SIMPLE_SEQ, "Parser3gp Port Setting Changed event\n");

        /* In tunneled case disabling the ports of all the tunneled components */
        if (flagSetupTunnel)
        {
            if (Data2 == 0)
            { /* video port*/
                DEBUG(DEB_LEV_SIMPLE_SEQ, "In %s parser3gp  Component Port Disabling\n", __func__);
                /*Sent Port Disable command. to disable both the output ports of the parser3gp */
                err = OMX_SendCommand(appPriv->parser3gphandle, OMX_CommandPortDisable, VIDEO_PORT_INDEX, NULL);
                if (err != OMX_ErrorNone)
                {
                    DEBUG(DEB_LEV_ERR, "parser3gp video port disable failed\n");
                    exit(1);
                }
                DEBUG(DEB_LEV_SIMPLE_SEQ, "In %s Video decoder Component Port Disabling\n", __func__);
                /*Sent Port Disable command. to disable dec, colorconv, sink ports before setting port their parameters */
                err = OMX_SendCommand(appPriv->videodechandle, OMX_CommandPortDisable, OMX_ALL, NULL);
                if (err != OMX_ErrorNone)
                {
                    DEBUG(DEB_LEV_ERR, "Video decoder port disable failed err=%08x\n", err);
                    exit(1);
                }

                if (flagIsDisplayRequested)
                {
                    DEBUG(DEB_LEV_SIMPLE_SEQ, "In %s Color Converter Component Port Disabling\n", __func__);
                    err = OMX_SendCommand(appPriv->colorconv_handle, OMX_CommandPortDisable, OMX_ALL, NULL);
                    if (err != OMX_ErrorNone)
                    {
                        DEBUG(DEB_LEV_ERR, "Color Converter Component port disable failed\n");
                        exit(1);
                    }

                    if (flagAVsync)
                    {
                        DEBUG(DEB_LEV_SIMPLE_SEQ, "In %s Video Scheduler Component Port Disabling\n", __func__);
                        err = OMX_SendCommand(appPriv->videoschd_handle, OMX_CommandPortDisable, 0, NULL);
                        if (err != OMX_ErrorNone)
                        {
                            DEBUG(DEB_LEV_ERR, "Video Scheduler Component input port disable failed\n");
                            exit(1);
                        }
                        err = OMX_SendCommand(appPriv->videoschd_handle, OMX_CommandPortDisable, 1, NULL);
                        if (err != OMX_ErrorNone)
                        {
                            DEBUG(DEB_LEV_ERR, "Video Scheduler Component output port disable failed\n");
                            exit(1);
                        }
                    }

                    DEBUG(DEB_LEV_SIMPLE_SEQ, "In %s Video Sink Port Disabling\n", __func__);
                    err = OMX_SendCommand(appPriv->videosinkhandle, OMX_CommandPortDisable, 0, NULL);
                    if (err != OMX_ErrorNone)
                    {
                        DEBUG(DEB_LEV_ERR, "video sink port disable failed\n");
                        exit(1);
                    }
                }
            }
            else if (Data2 == 1)
            { /* audio port */
#ifdef SUPPORT_AUDIO
                DEBUG(DEB_LEV_SIMPLE_SEQ, "In %s parser3gp  Component Port Disabling\n", __func__);
                /*Sent Port Disable command. to disable both the output ports of the parser3gp */
                err = OMX_SendCommand(appPriv->parser3gphandle, OMX_CommandPortDisable, AUDIO_PORT_INDEX, NULL);
                if (err != OMX_ErrorNone)
                {
                    DEBUG(DEB_LEV_ERR, "parser3gp audio port disable failed\n");
                    exit(1);
                }

                DEBUG(DEB_LEV_SIMPLE_SEQ, "In %s Audio decoder Component Port Disabling\n", __func__);
                /*Sent Port Disable command. to disable audio dec, volume and audio sink ports before setting port their parameters */
                err = OMX_SendCommand(appPriv->audiodechandle, OMX_CommandPortDisable, OMX_ALL, NULL);
                if (err != OMX_ErrorNone)
                {
                    DEBUG(DEB_LEV_ERR, "Audio decoder port disable failed\n");
                    exit(1);
                }

                if (flagIsDisplayRequested)
                {
                    DEBUG(DEB_LEV_SIMPLE_SEQ, "In %s Volume Component Port Disabling\n", __func__);
                    err = OMX_SendCommand(appPriv->volumehandle, OMX_CommandPortDisable, OMX_ALL, NULL);
                    if (err != OMX_ErrorNone)
                    {
                        DEBUG(DEB_LEV_ERR, "Volume Component port disable failed\n");
                        exit(1);
                    }

                    DEBUG(DEB_LEV_SIMPLE_SEQ, "In %s Audio Sink Port Disabling\n", __func__);
                    err = OMX_SendCommand(appPriv->audiosinkhandle, OMX_CommandPortDisable, 0, NULL);
                    if (err != OMX_ErrorNone)
                    {
                        DEBUG(DEB_LEV_ERR, "Audio sink port disable failed\n");
                        exit(1);
                    }
                }
#endif
            }
        }
        /*Signal Port Setting Changed*/
        tsem_up(appPriv->parser3gpEventSem);
    }
    else if (eEvent == OMX_EventPortFormatDetected)
    {
        DEBUG(DEB_LEV_SIMPLE_SEQ, "In %s Port Format Detected %x\n", __func__, (int) Data1);
    }
    else if (eEvent == OMX_EventError)
    {
        DEBUG(DEB_LEV_SIMPLE_SEQ, "In %s Error in %x Detection for port %d\n", __func__, (int) Data1, (int) Data2);
    }
    else if (eEvent == OMX_EventBufferFlag)
    {
        DEBUG(DEB_LEV_SIMPLE_SEQ, "In %s OMX_BUFFERFLAG_EOS\n", __func__);
        if ((int) Data2 == OMX_BUFFERFLAG_EOS)
        {
        }
    }
    else
    {
        DEBUG(DEB_LEV_SIMPLE_SEQ, "Param1 is %i\n", (int) Data1);
        DEBUG(DEB_LEV_SIMPLE_SEQ, "Param2 is %i\n", (int) Data2);
    }
    return OMX_ErrorNone;
}

OMX_ERRORTYPE parser3gpFillBufferDone(
                                      OMX_HANDLETYPE hComponent,
                                      OMX_PTR pAppData,
                                      OMX_BUFFERHEADERTYPE* pBuffer)
{
    OMX_ERRORTYPE err;
    /* Output data to video & audio decoder */

    if (pBuffer != NULL)
    {
        switch (pBuffer->nOutputPortIndex)
        {
        case VIDEO_PORT_INDEX:
            if (!bEOS)
            {
                if (inBufferVideoDec[0]->pBuffer == pBuffer->pBuffer)
                {
                    inBufferVideoDec[0]->nFilledLen = pBuffer->nFilledLen;
                    inBufferVideoDec[0]->nTimeStamp = pBuffer->nTimeStamp;
                    inBufferVideoDec[0]->nFlags = pBuffer->nFlags;
                    err = OMX_EmptyThisBuffer(appPriv->videodechandle, inBufferVideoDec[0]);
                }
                else
                {
                    inBufferVideoDec[1]->nFilledLen = pBuffer->nFilledLen;
                    inBufferVideoDec[1]->nTimeStamp = pBuffer->nTimeStamp;
                    inBufferVideoDec[1]->nFlags = pBuffer->nFlags;
                    err = OMX_EmptyThisBuffer(appPriv->videodechandle, inBufferVideoDec[1]);
                }
                if (err != OMX_ErrorNone)
                {
                    DEBUG(DEB_LEV_ERR, "In %s Error %08x Calling FillThisBuffer\n", __func__, err);
                }
                if ((pBuffer->nFlags & OMX_BUFFERFLAG_EOS) == OMX_BUFFERFLAG_EOS)
                {
                    DEBUG(DEB_LEV_SIMPLE_SEQ, "In %s: eos=%x Calling Empty This Buffer\n", __func__, (int) pBuffer->nFlags);
                    bEOS = OMX_TRUE;
                }
                pBuffer->nFlags = 0;
            }
            else
            {
                DEBUG(DEB_LEV_SIMPLE_SEQ, "In %s: eos=%x Dropping Empty This Buffer\n", __func__, (int) pBuffer->nFlags);
            }
            break;
#ifdef SUPPORT_AUDIO
        case AUDIO_PORT_INDEX:
            if (!bEOS)
            {
                if (inBufferAudioDec[0]->pBuffer == pBuffer->pBuffer)
                {
                    inBufferAudioDec[0]->nFilledLen = pBuffer->nFilledLen;
                    inBufferAudioDec[0]->nTimeStamp = pBuffer->nTimeStamp;
                    inBufferAudioDec[0]->nFlags = pBuffer->nFlags;
                    err = OMX_EmptyThisBuffer(appPriv->audiodechandle, inBufferAudioDec[0]);
                }
                else
                {
                    inBufferAudioDec[1]->nFilledLen = pBuffer->nFilledLen;
                    inBufferAudioDec[1]->nTimeStamp = pBuffer->nTimeStamp;
                    inBufferAudioDec[1]->nFlags = pBuffer->nFlags;
                    err = OMX_EmptyThisBuffer(appPriv->audiodechandle, inBufferAudioDec[1]);
                }
                if (err != OMX_ErrorNone)
                {
                    DEBUG(DEB_LEV_ERR, "In %s Error %08x Calling FillThisBuffer\n", __func__, err);
                }
                if ((pBuffer->nFlags & OMX_BUFFERFLAG_EOS) == OMX_BUFFERFLAG_EOS)
                {
                    DEBUG(DEB_LEV_SIMPLE_SEQ, "In %s: eos=%x Calling Empty This Buffer\n", __func__, (int) pBuffer->nFlags);
                    bEOS = OMX_TRUE;
                }
                pBuffer->nFlags = 0;
            }
            else
            {
                DEBUG(DEB_LEV_SIMPLE_SEQ, "In %s: eos=%x Dropping Empty This Buffer\n", __func__, (int) pBuffer->nFlags);
            }
            break;
#endif
        }
    }
    else
    {
        DEBUG(DEB_LEV_ERR, "Ouch! In %s: had NULL buffer to output...\n", __func__);
    }
    return OMX_ErrorNone;
}

OMX_ERRORTYPE videodecEventHandler(
                                   OMX_HANDLETYPE hComponent,
                                   OMX_PTR pAppData,
                                   OMX_EVENTTYPE eEvent,
                                   OMX_U32 Data1,
                                   OMX_U32 Data2,
                                   OMX_PTR pEventData)
{
    OMX_ERRORTYPE err;
    OMX_PARAM_PORTDEFINITIONTYPE param;

    DEBUG(DEB_LEV_FULL_SEQ, "Hi there, I am in the %s callback\n", __func__);
    if (eEvent == OMX_EventCmdComplete)
    {
        if (Data1 == OMX_CommandStateSet)
        {
            DEBUG(DEB_LEV_SIMPLE_SEQ/*SIMPLE_SEQ*/, "Video Decoder State changed in ");
            switch ((int) Data2)
            {
            case OMX_StateInvalid:
                DEBUG(DEB_LEV_SIMPLE_SEQ, "OMX_StateInvalid\n");
                break;
            case OMX_StateLoaded:
                DEBUG(DEB_LEV_SIMPLE_SEQ, "OMX_StateLoaded\n");
                break;
            case OMX_StateIdle:
                DEBUG(DEB_LEV_SIMPLE_SEQ, "OMX_StateIdle\n");
                break;
            case OMX_StateExecuting:
                DEBUG(DEB_LEV_SIMPLE_SEQ, "OMX_StateExecuting\n");
                break;
            case OMX_StatePause:
                DEBUG(DEB_LEV_SIMPLE_SEQ, "OMX_StatePause\n");
                break;
            case OMX_StateWaitForResources:
                DEBUG(DEB_LEV_SIMPLE_SEQ, "OMX_StateWaitForResources\n");
                break;
            }
            tsem_up(appPriv->videoDecoderEventSem);
        }
        else if (Data1 == OMX_CommandPortEnable)
        {
            DEBUG(DEB_LEV_SIMPLE_SEQ, "In %s Received Port Enable  Event\n", __func__);
            tsem_up(appPriv->videoDecoderEventSem);
        }
        else if (Data1 == OMX_CommandPortDisable)
        {
            DEBUG(DEB_LEV_SIMPLE_SEQ, "In %s Received Port Disable Event\n", __func__);
            tsem_up(appPriv->videoDecoderEventSem);
        }
        else if (Data1 == OMX_CommandFlush)
        {
            DEBUG(DEB_LEV_SIMPLE_SEQ, "In %s Received Port Flush Event\n", __func__);
            tsem_up(appPriv->videoDecoderEventSem);
        }
    }
    else if (eEvent == OMX_EventPortSettingsChanged)
    {
        DEBUG(DEB_LEV_SIMPLE_SEQ, "In %s Received Port Settings Changed Event\n", __func__);
        if (Data2 == 1)
        {
            param.nPortIndex = 1;
            setHeader(&param, sizeof (OMX_PARAM_PORTDEFINITIONTYPE));
            err = OMX_GetParameter(appPriv->videodechandle, OMX_IndexParamPortDefinition, &param);
            /*Get Port parameters*/
        }
        else if (Data2 == 0)
        {
            /*Get Port parameters*/
            param.nPortIndex = 0;
            setHeader(&param, sizeof (OMX_PARAM_PORTDEFINITIONTYPE));
            err = OMX_GetParameter(appPriv->videodechandle, OMX_IndexParamPortDefinition, &param);
        }
    }
    else if (eEvent == OMX_EventBufferFlag)
    {
        DEBUG(DEB_LEV_SIMPLE_SEQ, "In %s OMX_BUFFERFLAG_EOS\n", __func__);
        if ((int) Data2 == OMX_BUFFERFLAG_EOS)
        {
        }
    }
    else
    {
        DEBUG(DEB_LEV_SIMPLE_SEQ, "Param1 is %i\n", (int) Data1);
        DEBUG(DEB_LEV_SIMPLE_SEQ, "Param2 is %i\n", (int) Data2);
    }

    return OMX_ErrorNone;
}

OMX_ERRORTYPE videodecEmptyBufferDone(
                                      OMX_HANDLETYPE hComponent,
                                      OMX_PTR pAppData,
                                      OMX_BUFFERHEADERTYPE* pBuffer)
{
    OMX_ERRORTYPE err;
    static int iBufferDropped = 0;
    DEBUG(DEB_LEV_FULL_SEQ, "Hi there, I am in the %s callback.\n", __func__);

    if (pBuffer != NULL)
    {
        if (!bEOS)
        {
            if (outBufferParseVideo[0]->pBuffer == pBuffer->pBuffer)
            {
                outBufferParseVideo[0]->nFilledLen = 0;
                err = OMX_FillThisBuffer(appPriv->parser3gphandle, outBufferParseVideo[0]);
            }
            else
            {
                outBufferParseVideo[1]->nFilledLen = 0;
                err = OMX_FillThisBuffer(appPriv->parser3gphandle, outBufferParseVideo[1]);
            }
            if (err != OMX_ErrorNone)
            {
                DEBUG(DEB_LEV_ERR, "In %s Error %08x Calling FillThisBuffer\n", __func__, err);
            }
        }
        else
        {
            DEBUG(DEB_LEV_SIMPLE_SEQ, "In %s: eos=%x Dropping Fill This Buffer\n", __func__, (int) pBuffer->nFlags);
            iBufferDropped++;
            if (iBufferDropped == 2 && !flagIsDisplayRequested)
            {
                tsem_up(appPriv->eofSem);
            }
        }
    }
    else
    {
        if (!bEOS)
        {
            DEBUG(DEB_LEV_SIMPLE_SEQ, "It is here EOS = %d\n", appPriv->eofSem->semval);
        }
        DEBUG(DEB_LEV_ERR, "Ouch! In %s: had NULL buffer to output...\n", __func__);
    }
    return OMX_ErrorNone;
}

OMX_ERRORTYPE videodecFillBufferDone(
                                     OMX_HANDLETYPE hComponent,
                                     OMX_PTR pAppData,
                                     OMX_BUFFERHEADERTYPE* pBuffer)
{
    OMX_ERRORTYPE err;
    int i;
    DEBUG(DEB_LEV_FUNCTION_NAME, "In %s \n", __func__);
    /* Output data to alsa sink */
    if (pBuffer != NULL)
    {
        if (!bEOS)
        {
            if (pBuffer->nFilledLen == 0)
            {
                DEBUG(DEB_LEV_ERR, "Gadbadax! In %s: had 0 data size in output buffer...\n", __func__);
                return OMX_ErrorNone;
            }
            if ((!flagDecodedOutputReceived) && (!flagIsDisplayRequested))
            {
                for (i = 0; i < pBuffer->nFilledLen; i++)
                {
                    putchar(*(char*) (pBuffer->pBuffer + i));
                }
                pBuffer->nFilledLen = 0;
                err = OMX_FillThisBuffer(hComponent, pBuffer);
                if (err != OMX_ErrorNone)
                {
                    DEBUG(DEB_LEV_ERR, "In %s Error %08x Calling FillThisBuffer\n", __func__, err);
                }
            }
            else if ((flagDecodedOutputReceived) && (!flagIsDisplayRequested))
            {
                if (pBuffer->nFilledLen > 0)
                {
                    fwrite(pBuffer->pBuffer, 1, pBuffer->nFilledLen, outfileVideo);
                }
                pBuffer->nFilledLen = 0;
                err = OMX_FillThisBuffer(hComponent, pBuffer);
                if (err != OMX_ErrorNone)
                {
                    DEBUG(DEB_LEV_ERR, "In %s Error %08x Calling FillThisBuffer\n", __func__, err);
                }
            }
            else if ((!flagSetupTunnel) && (flagIsDisplayRequested))
            { //colorconverter on, redirect to sink, if it is not tunneled
                if (inBufferColorconv[0]->pBuffer == pBuffer->pBuffer)
                {
                    inBufferColorconv[0]->nFilledLen = pBuffer->nFilledLen;
                    inBufferColorconv[0]->nTimeStamp = pBuffer->nTimeStamp;
                    if ((pBuffer->nFlags & OMX_BUFFERFLAG_STARTTIME) == OMX_BUFFERFLAG_STARTTIME)
                    {
                        inBufferColorconv[0]->nFlags = pBuffer->nFlags;
                        pBuffer->nFlags = 0;
                    }
                    err = OMX_EmptyThisBuffer(appPriv->colorconv_handle, inBufferColorconv[0]);
                }
                else
                {
                    inBufferColorconv[1]->nFilledLen = pBuffer->nFilledLen;
                    inBufferColorconv[1]->nTimeStamp = pBuffer->nTimeStamp;
                    if ((pBuffer->nFlags & OMX_BUFFERFLAG_STARTTIME) == OMX_BUFFERFLAG_STARTTIME)
                    {
                        inBufferColorconv[1]->nFlags = pBuffer->nFlags;
                        pBuffer->nFlags = 0;
                    }
                    err = OMX_EmptyThisBuffer(appPriv->colorconv_handle, inBufferColorconv[1]);
                }
                if (err != OMX_ErrorNone)
                {
                    DEBUG(DEB_LEV_ERR, "In %s Error %08x Calling EmptyThisBuffer\n", __func__, err);
                }
            }
        }
        else
        {
            DEBUG(DEB_LEV_ERR, " Buffer Dropping in %s...\n", __func__);
            tsem_up(appPriv->eofSem);
        }
    }
    else
    {
        DEBUG(DEB_LEV_ERR, "Ouch! In %s: had NULL buffer to output...\n", __func__);
    }
    return OMX_ErrorNone;
}

/* Callbacks implementation of the color conv component */
OMX_ERRORTYPE colorconvEventHandler(
                                    OMX_HANDLETYPE hComponent,
                                    OMX_PTR pAppData,
                                    OMX_EVENTTYPE eEvent,
                                    OMX_U32 Data1,
                                    OMX_U32 Data2,
                                    OMX_PTR pEventData)
{

    DEBUG(DEB_LEV_SIMPLE_SEQ, "\nHi there, I am in the %s callback\n", __func__);
    if (eEvent == OMX_EventCmdComplete)
    {
        if (Data1 == OMX_CommandStateSet)
        {
            DEBUG(DEB_LEV_SIMPLE_SEQ, "\nState changed in ");
            switch ((int) Data2)
            {
            case OMX_StateInvalid:
                DEBUG(DEB_LEV_SIMPLE_SEQ, "OMX_StateInvalid\n");
                break;
            case OMX_StateLoaded:
                DEBUG(DEB_LEV_SIMPLE_SEQ, "OMX_StateLoaded\n");
                break;
            case OMX_StateIdle:
                DEBUG(DEB_LEV_SIMPLE_SEQ, "OMX_StateIdle\n");
                break;
            case OMX_StateExecuting:
                DEBUG(DEB_LEV_SIMPLE_SEQ, "OMX_StateExecuting\n");
                break;
            case OMX_StatePause:
                DEBUG(DEB_LEV_SIMPLE_SEQ, "OMX_StatePause\n");
                break;
            case OMX_StateWaitForResources:
                DEBUG(DEB_LEV_SIMPLE_SEQ, "OMX_StateWaitForResources\n");
                break;
            }
            tsem_up(appPriv->colorconvEventSem);
        }
        else if (OMX_CommandPortEnable || OMX_CommandPortDisable)
        {
            DEBUG(DEB_LEV_SIMPLE_SEQ, "In %s Received Port Enable/Disable Event\n", __func__);
            tsem_up(appPriv->colorconvEventSem);
        }
        else if (OMX_CommandFlush)
        {
            DEBUG(DEB_LEV_SIMPLE_SEQ, "In %s Received Port Flush Event\n", __func__);
            tsem_up(appPriv->colorconvEventSem);
        }
    }
    else if (eEvent == OMX_EventBufferFlag)
    {
        DEBUG(DEB_LEV_SIMPLE_SEQ, "In %s OMX_BUFFERFLAG_EOS\n", __func__);
        if ((int) Data2 == OMX_BUFFERFLAG_EOS)
        {
            //      tsem_up(appPriv->eofSem);
        }
    }
    else
    {
        DEBUG(DEB_LEV_SIMPLE_SEQ, "Param1 is %i\n", (int) Data1);
        DEBUG(DEB_LEV_SIMPLE_SEQ, "Param2 is %i\n", (int) Data2);
    }
    return OMX_ErrorNone;
}

OMX_ERRORTYPE colorconvEmptyBufferDone(
                                       OMX_HANDLETYPE hComponent,
                                       OMX_PTR pAppData,
                                       OMX_BUFFERHEADERTYPE* pBuffer)
{

    OMX_ERRORTYPE err;
    static int iBufferDropped = 0;

    if (pBuffer != NULL)
    {
        if (!bEOS)
        {
            if (outBufferVideoDec[0]->pBuffer == pBuffer->pBuffer)
            {
                outBufferVideoDec[0]->nFilledLen = pBuffer->nFilledLen;
                err = OMX_FillThisBuffer(appPriv->videodechandle, outBufferVideoDec[0]);
            }
            else
            {
                outBufferVideoDec[1]->nFilledLen = pBuffer->nFilledLen;
                err = OMX_FillThisBuffer(appPriv->videodechandle, outBufferVideoDec[1]);
            }
            if (err != OMX_ErrorNone)
            {
                DEBUG(DEB_LEV_ERR, "In %s Error %08x Calling FillThisBuffer\n", __func__, err);
            }
        }
        else
        {
            DEBUG(DEB_LEV_ERR, "In %s: eos=%x Dropping Fill This Buffer\n", __func__, (int) pBuffer->nFlags);
            iBufferDropped++;
            if (iBufferDropped == 2)
            {
                tsem_up(appPriv->eofSem);
            }
        }
    }
    else
    {
        if (!bEOS)
        {
            tsem_up(appPriv->eofSem);
        }
        DEBUG(DEB_LEV_ERR, "Ouch! In %s: had NULL buffer to output...\n", __func__);
    }
    return OMX_ErrorNone;
}

OMX_ERRORTYPE colorconvFillBufferDone(
                                      OMX_HANDLETYPE hComponent,
                                      OMX_PTR pAppData,
                                      OMX_BUFFERHEADERTYPE* pBuffer)
{

    OMX_ERRORTYPE err;
    if (pBuffer != NULL)
    {
        if (!bEOS)
        {
            /** if there is no sink component then write buffer content in output file, in non tunneled case
             * else in non tunneled case, call the sink comp handle to process this buffer as its input buffer
             */
            if (flagIsDisplayRequested && (!flagSetupTunnel))
            {
                if (flagAVsync)
                {
                    if (inBufferVideoSchd[0]->pBuffer == pBuffer->pBuffer)
                    {
                        inBufferVideoSchd[0]->nFilledLen = pBuffer->nFilledLen;
                        inBufferVideoSchd[0]->nTimeStamp = pBuffer->nTimeStamp;
                        if ((pBuffer->nFlags & OMX_BUFFERFLAG_STARTTIME) == OMX_BUFFERFLAG_STARTTIME)
                        {
                            inBufferVideoSchd[0]->nFlags = pBuffer->nFlags;
                            pBuffer->nFlags = 0;
                        }
                        err = OMX_EmptyThisBuffer(appPriv->videoschd_handle, inBufferVideoSchd[0]);
                    }
                    else
                    {
                        inBufferVideoSchd[1]->nFilledLen = pBuffer->nFilledLen;
                        inBufferVideoSchd[1]->nTimeStamp = pBuffer->nTimeStamp;
                        if ((pBuffer->nFlags & OMX_BUFFERFLAG_STARTTIME) == OMX_BUFFERFLAG_STARTTIME)
                        {
                            inBufferVideoSchd[1]->nFlags = pBuffer->nFlags;
                            pBuffer->nFlags = 0;
                        }
                        err = OMX_EmptyThisBuffer(appPriv->videoschd_handle, inBufferVideoSchd[1]);
                    }
                    if (err != OMX_ErrorNone)
                    {
                        DEBUG(DEB_LEV_ERR, "In %s Error %08x Calling FillThisBuffer\n", __func__, err);
                    }
                }
                else
                { /* since flagAVsync=0 colorconv connected directly to video sink */
                    if (inBufferSinkVideo[0]->pBuffer == pBuffer->pBuffer)
                    {
                        inBufferSinkVideo[0]->nFilledLen = pBuffer->nFilledLen;
                        inBufferSinkVideo[0]->nTimeStamp = pBuffer->nTimeStamp;
                        if ((pBuffer->nFlags & OMX_BUFFERFLAG_STARTTIME) == OMX_BUFFERFLAG_STARTTIME)
                        {
                            inBufferSinkVideo[0]->nFlags = pBuffer->nFlags;
                            pBuffer->nFlags = 0;
                        }
                        err = OMX_EmptyThisBuffer(appPriv->videosinkhandle, inBufferSinkVideo[0]);
                    }
                    else
                    {
                        inBufferSinkVideo[1]->nFilledLen = pBuffer->nFilledLen;
                        inBufferSinkVideo[1]->nTimeStamp = pBuffer->nTimeStamp;
                        if ((pBuffer->nFlags & OMX_BUFFERFLAG_STARTTIME) == OMX_BUFFERFLAG_STARTTIME)
                        {
                            inBufferSinkVideo[1]->nFlags = pBuffer->nFlags;
                            pBuffer->nFlags = 0;
                        }
                        err = OMX_EmptyThisBuffer(appPriv->videosinkhandle, inBufferSinkVideo[1]);
                    }
                    if (err != OMX_ErrorNone)
                    {
                        DEBUG(DEB_LEV_ERR, "In %s Error %08x Calling FillThisBuffer\n", __func__, err);
                    }
                }
            }
            else if ((pBuffer->nFilledLen > 0) && (!flagSetupTunnel))
            {
                fwrite(pBuffer->pBuffer, sizeof (char), pBuffer->nFilledLen, outfileVideo);
                pBuffer->nFilledLen = 0;
            }
            if ((pBuffer->nFlags & OMX_BUFFERFLAG_EOS) == OMX_BUFFERFLAG_EOS)
            {
                DEBUG(DEB_LEV_ERR, "In %s: eos=%x Calling Empty This Buffer\n", __func__, (int) pBuffer->nFlags);
                bEOS = OMX_TRUE;
            }
            if (!bEOS && !flagIsDisplayRequested && (!flagSetupTunnel))
            {
                err = OMX_FillThisBuffer(hComponent, pBuffer);
            }
        }
        else
        {
            DEBUG(DEB_LEV_ERR, "In %s: eos=%x Dropping Empty This Buffer\n", __func__, (int) pBuffer->nFlags);
        }
    }
    else
    {
        DEBUG(DEB_LEV_ERR, "Ouch! In %s: had NULL buffer to output...\n", __func__);
    }
    return OMX_ErrorNone;
}

/* Callback implementation of the video scheduler component */
OMX_ERRORTYPE videoschdEventHandler(
                                    OMX_HANDLETYPE hComponent,
                                    OMX_PTR pAppData,
                                    OMX_EVENTTYPE eEvent,
                                    OMX_U32 Data1,
                                    OMX_U32 Data2,
                                    OMX_PTR pEventData)
{

    DEBUG(DEB_LEV_SIMPLE_SEQ, "\nHi there, I am in the %s callback\n", __func__);
    if (eEvent == OMX_EventCmdComplete)
    {
        if (Data1 == OMX_CommandStateSet)
        {
            DEBUG(DEB_LEV_SIMPLE_SEQ, "\nState changed in ");
            switch ((int) Data2)
            {
            case OMX_StateInvalid:
                DEBUG(DEB_LEV_SIMPLE_SEQ, "OMX_StateInvalid\n");
                break;
            case OMX_StateLoaded:
                DEBUG(DEB_LEV_SIMPLE_SEQ, "OMX_StateLoaded\n");
                break;
            case OMX_StateIdle:
                DEBUG(DEB_LEV_SIMPLE_SEQ, "OMX_StateIdle\n");
                break;
            case OMX_StateExecuting:
                DEBUG(DEB_LEV_SIMPLE_SEQ, "OMX_StateExecuting\n");
                break;
            case OMX_StatePause:
                DEBUG(DEB_LEV_SIMPLE_SEQ, "OMX_StatePause\n");
                break;
            case OMX_StateWaitForResources:
                DEBUG(DEB_LEV_SIMPLE_SEQ, "OMX_StateWaitForResources\n");
                break;
            }
            tsem_up(appPriv->videoschdEventSem);
        }
        else if (OMX_CommandPortEnable || OMX_CommandPortDisable)
        {
            DEBUG(DEB_LEV_SIMPLE_SEQ, "In %s Received Port Enable/Disable Event\n", __func__);
            tsem_up(appPriv->videoschdEventSem);
        }
        else if (OMX_CommandFlush)
        {
            DEBUG(DEB_LEV_SIMPLE_SEQ, "In %s Received Port Flush Event\n", __func__);
            tsem_up(appPriv->videoschdEventSem);
        }
    }
    else if (eEvent == OMX_EventBufferFlag)
    {
        DEBUG(DEB_LEV_SIMPLE_SEQ, "In %s OMX_BUFFERFLAG_EOS\n", __func__);
        if ((int) Data2 == OMX_BUFFERFLAG_EOS)
        {
            //      tsem_up(appPriv->eofSem);
        }
    }
    else
    {
        DEBUG(DEB_LEV_SIMPLE_SEQ, "Param1 is %i\n", (int) Data1);
        DEBUG(DEB_LEV_SIMPLE_SEQ, "Param2 is %i\n", (int) Data2);
    }
    return OMX_ErrorNone;
}

OMX_ERRORTYPE videoschdEmptyBufferDone(
                                       OMX_HANDLETYPE hComponent,
                                       OMX_PTR pAppData,
                                       OMX_BUFFERHEADERTYPE* pBuffer)
{

    OMX_ERRORTYPE err;
    static int iBufferDropped = 0;

    if (pBuffer != NULL)
    {
        if (!bEOS)
        {
            if (outBufferColorconv[0]->pBuffer == pBuffer->pBuffer)
            {
                outBufferColorconv[0]->nFilledLen = pBuffer->nFilledLen;
                err = OMX_FillThisBuffer(appPriv->colorconv_handle, outBufferColorconv[0]);
            }
            else
            {
                outBufferColorconv[1]->nFilledLen = pBuffer->nFilledLen;
                err = OMX_FillThisBuffer(appPriv->colorconv_handle, outBufferColorconv[1]);
            }
            if (err != OMX_ErrorNone)
            {
                DEBUG(DEB_LEV_ERR, "In %s Error %08x Calling FillThisBuffer\n", __func__, err);
            }
        }
        else
        {
            DEBUG(DEB_LEV_ERR, "In %s: eos=%x Dropping Fill This Buffer\n", __func__, (int) pBuffer->nFlags);
            iBufferDropped++;
            if (iBufferDropped == 2)
            {
                //   tsem_up(appPriv->eofSem);
            }
        }
    }
    else
    {
        if (!bEOS)
        {
            //  tsem_up(appPriv->eofSem);
        }
        DEBUG(DEB_LEV_ERR, "Ouch! In %s: had NULL buffer to output...\n", __func__);
    }
    return OMX_ErrorNone;
}

OMX_ERRORTYPE videoschdFillBufferDone(
                                      OMX_HANDLETYPE hComponent,
                                      OMX_PTR pAppData,
                                      OMX_BUFFERHEADERTYPE* pBuffer)
{

    OMX_ERRORTYPE err;
    if (pBuffer != NULL)
    {
        if (!bEOS)
        {
            /** if there is no sink component then write buffer content in output file, in non tunneled case
             * else in non tunneled case, call the sink comp handle to process this buffer as its input buffer
             */
            if (flagIsDisplayRequested && (!flagSetupTunnel))
            {
                if (inBufferSinkVideo[0]->pBuffer == pBuffer->pBuffer)
                {
                    inBufferSinkVideo[0]->nFilledLen = pBuffer->nFilledLen;
                    inBufferSinkVideo[0]->nTimeStamp = pBuffer->nTimeStamp;
                    if ((pBuffer->nFlags & OMX_BUFFERFLAG_STARTTIME) == OMX_BUFFERFLAG_STARTTIME)
                    {
                        inBufferSinkVideo[0]->nFlags = pBuffer->nFlags;
                        pBuffer->nFlags = 0;
                    }
                    err = OMX_EmptyThisBuffer(appPriv->videosinkhandle, inBufferSinkVideo[0]);
                }
                else
                {
                    inBufferSinkVideo[1]->nFilledLen = pBuffer->nFilledLen;
                    inBufferSinkVideo[1]->nTimeStamp = pBuffer->nTimeStamp;
                    if ((pBuffer->nFlags & OMX_BUFFERFLAG_STARTTIME) == OMX_BUFFERFLAG_STARTTIME)
                    {
                        inBufferSinkVideo[1]->nFlags = pBuffer->nFlags;
                        pBuffer->nFlags = 0;
                    }
                    err = OMX_EmptyThisBuffer(appPriv->videosinkhandle, inBufferSinkVideo[1]);
                }
                if (err != OMX_ErrorNone)
                {
                    DEBUG(DEB_LEV_ERR, "In %s Error %08x Calling FillThisBuffer\n", __func__, err);
                }
            }
            if ((pBuffer->nFlags & OMX_BUFFERFLAG_EOS) == OMX_BUFFERFLAG_EOS)
            {
                DEBUG(DEB_LEV_ERR, "In %s: eos=%x Calling Empty This Buffer\n", __func__, (int) pBuffer->nFlags);
                bEOS = OMX_TRUE;
            }
            if (!bEOS && !flagIsDisplayRequested && (!flagSetupTunnel))
            {
                err = OMX_FillThisBuffer(hComponent, pBuffer);
            }
        }
        else
        {
            DEBUG(DEB_LEV_ERR, "In %s: eos=%x Dropping Empty This Buffer\n", __func__, (int) pBuffer->nFlags);
        }
    }
    else
    {
        DEBUG(DEB_LEV_ERR, "Ouch! In %s: had NULL buffer to output...\n", __func__);
    }
    return OMX_ErrorNone;
}

/** callbacks implementation of video sink component */
OMX_ERRORTYPE fb_sinkEventHandler(
                                  OMX_HANDLETYPE hComponent,
                                  OMX_PTR pAppData,
                                  OMX_EVENTTYPE eEvent,
                                  OMX_U32 Data1,
                                  OMX_U32 Data2,
                                  OMX_PTR pEventData)
{

    DEBUG(DEB_LEV_SIMPLE_SEQ, "Hi there, I am in the %s callback\n", __func__);
    if (eEvent == OMX_EventCmdComplete)
    {
        if (Data1 == OMX_CommandStateSet)
        {
            DEBUG(DEB_LEV_SIMPLE_SEQ, "State changed in ");
            switch ((int) Data2)
            {
            case OMX_StateInvalid:
                DEBUG(DEB_LEV_SIMPLE_SEQ, "OMX_StateInvalid\n");
                break;
            case OMX_StateLoaded:
                DEBUG(DEB_LEV_SIMPLE_SEQ, "OMX_StateLoaded\n");
                break;
            case OMX_StateIdle:
                DEBUG(DEB_LEV_SIMPLE_SEQ, "OMX_StateIdle ---- fbsink\n");
                break;
            case OMX_StateExecuting:
                DEBUG(DEB_LEV_SIMPLE_SEQ, "OMX_StateExecuting\n");
                break;
            case OMX_StatePause:
                DEBUG(DEB_LEV_SIMPLE_SEQ, "OMX_StatePause\n");
                break;
            case OMX_StateWaitForResources:
                DEBUG(DEB_LEV_SIMPLE_SEQ, "OMX_StateWaitForResources\n");
                break;
            }
            tsem_up(appPriv->fbdevSinkEventSem);
        }
        else if (OMX_CommandPortEnable || OMX_CommandPortDisable)
        {
            DEBUG(DEB_LEV_SIMPLE_SEQ, "In %s Received Port Enable/Disable Event\n", __func__);
            tsem_up(appPriv->fbdevSinkEventSem);
        }
        else if (OMX_CommandFlush)
        {
            DEBUG(DEB_LEV_SIMPLE_SEQ, "In %s Received Port Flush Event\n", __func__);
            tsem_up(appPriv->fbdevSinkEventSem);
        }
    }
    else if (eEvent == OMX_EventBufferFlag)
    {
        DEBUG(DEB_LEV_SIMPLE_SEQ, "In %s OMX_BUFFERFLAG_EOS\n", __func__);
        if ((int) Data2 == OMX_BUFFERFLAG_EOS)
        {
            tsem_up(appPriv->eofSem);
        }
    }
    else
    {
        DEBUG(DEB_LEV_SIMPLE_SEQ, "Param1 is %i\n", (int) Data1);
        DEBUG(DEB_LEV_SIMPLE_SEQ, "Param2 is %i\n", (int) Data2);
    }
    return OMX_ErrorNone;
}

OMX_ERRORTYPE fb_sinkEmptyBufferDone(
                                     OMX_HANDLETYPE hComponent,
                                     OMX_PTR pAppData,
                                     OMX_BUFFERHEADERTYPE* pBuffer)
{

    OMX_ERRORTYPE err;
    static int inputBufferDropped = 0;
    if (pBuffer != NULL)
    {
        if (!bEOS)
        {
            if (flagAVsync)
            {
                if (outBufferVideoSchd[0]->pBuffer == pBuffer->pBuffer)
                {
                    outBufferVideoSchd[0]->nFilledLen = pBuffer->nFilledLen;
                    err = OMX_FillThisBuffer(appPriv->videoschd_handle, outBufferVideoSchd[0]);
                }
                else
                {
                    outBufferVideoSchd[1]->nFilledLen = pBuffer->nFilledLen;
                    err = OMX_FillThisBuffer(appPriv->videoschd_handle, outBufferVideoSchd[1]);
                }
                if (err != OMX_ErrorNone)
                {
                    DEBUG(DEB_LEV_ERR, "In %s Error %08x Calling FillThisBuffer\n", __func__, err);
                }
            }
            else
            {
                if (outBufferColorconv[0]->pBuffer == pBuffer->pBuffer)
                {
                    outBufferColorconv[0]->nFilledLen = pBuffer->nFilledLen;
                    err = OMX_FillThisBuffer(appPriv->colorconv_handle, outBufferColorconv[0]);
                }
                else
                {
                    outBufferColorconv[1]->nFilledLen = pBuffer->nFilledLen;
                    err = OMX_FillThisBuffer(appPriv->colorconv_handle, outBufferColorconv[1]);
                }
                if (err != OMX_ErrorNone)
                {
                    DEBUG(DEB_LEV_ERR, "In %s Error %08x Calling FillThisBuffer\n", __func__, err);
                }
            }
        }
        else
        {
            DEBUG(DEB_LEV_ERR, "In %s: eos=%x Dropping Fill This Buffer\n", __func__, (int) pBuffer->nFlags);
            inputBufferDropped++;
            if (inputBufferDropped == 2)
            {
                tsem_up(appPriv->eofSem);
            }
        }
    }
    else
    {
        if (!bEOS)
        {
            tsem_up(appPriv->eofSem);
        }
        DEBUG(DEB_LEV_ERR, "Ouch! In %s: had NULL buffer to output...\n", __func__);
    }
    return OMX_ErrorNone;
}

OMX_ERRORTYPE audiodecEventHandler(
                                   OMX_HANDLETYPE hComponent,
                                   OMX_PTR pAppData,
                                   OMX_EVENTTYPE eEvent,
                                   OMX_U32 Data1,
                                   OMX_U32 Data2,
                                   OMX_PTR pEventData)
{
    OMX_ERRORTYPE err;
    OMX_PARAM_PORTDEFINITIONTYPE param;
    //  OMX_AUDIO_PARAM_PCMMODETYPE pcmParam;

    DEBUG(DEB_LEV_SIMPLE_SEQ, "Hi there, I am in the %s callback\n", __func__);
    if (eEvent == OMX_EventCmdComplete)
    {
        if (Data1 == OMX_CommandStateSet)
        {
            DEBUG(DEB_LEV_SIMPLE_SEQ/*SIMPLE_SEQ*/, "Audio Decoder State changed in ");
            switch ((int) Data2)
            {
            case OMX_StateInvalid:
                DEBUG(DEB_LEV_SIMPLE_SEQ, "OMX_StateInvalid\n");
                break;
            case OMX_StateLoaded:
                DEBUG(DEB_LEV_SIMPLE_SEQ, "OMX_StateLoaded\n");
                break;
            case OMX_StateIdle:
                DEBUG(DEB_LEV_SIMPLE_SEQ, "OMX_StateIdle\n");
                break;
            case OMX_StateExecuting:
                DEBUG(DEB_LEV_SIMPLE_SEQ, "OMX_StateExecuting\n");
                break;
            case OMX_StatePause:
                DEBUG(DEB_LEV_SIMPLE_SEQ, "OMX_StatePause\n");
                break;
            case OMX_StateWaitForResources:
                DEBUG(DEB_LEV_SIMPLE_SEQ, "OMX_StateWaitForResources\n");
                break;
            }
			tsem_up(appPriv->audioDecoderEventSem);
        }
        else if (Data1 == OMX_CommandPortEnable)
        {
            DEBUG(DEB_LEV_SIMPLE_SEQ, "In %s Received Port Enable  Event\n", __func__);
            tsem_up(appPriv->audioDecoderEventSem);
        }
        else if (Data1 == OMX_CommandPortDisable)
        {
            DEBUG(DEB_LEV_SIMPLE_SEQ, "In %s Received Port Disable Event\n", __func__);
            tsem_up(appPriv->audioDecoderEventSem);
        }
        else if (Data1 == OMX_CommandFlush)
        {
            DEBUG(DEB_LEV_SIMPLE_SEQ, "In %s Received Port Flush Event\n", __func__);
            tsem_up(appPriv->audioDecoderEventSem);
        }
    }
    else if (eEvent == OMX_EventPortSettingsChanged)
    {
        DEBUG(DEB_LEV_SIMPLE_SEQ, "In %s Received Port Settings Changed Event\n", __func__);
        if (Data2 == 1)
        {
            param.nPortIndex = 1;
            setHeader(&param, sizeof (OMX_PARAM_PORTDEFINITIONTYPE));
            err = OMX_GetParameter(appPriv->audiodechandle, OMX_IndexParamPortDefinition, &param);
        }
        else if (Data2 == 0)
        {
            /*Get Port parameters*/
            param.nPortIndex = 0;
            setHeader(&param, sizeof (OMX_PARAM_PORTDEFINITIONTYPE));
            err = OMX_GetParameter(appPriv->audiodechandle, OMX_IndexParamPortDefinition, &param);
        }
    }
    else if (eEvent == OMX_EventBufferFlag)
    {
        DEBUG(DEB_LEV_SIMPLE_SEQ, "In %s OMX_BUFFERFLAG_EOS\n", __func__);
        if ((int) Data2 == OMX_BUFFERFLAG_EOS)
        {
            //      tsem_up(appPriv->eofSem);
        }
    }
    else
    {
        DEBUG(DEB_LEV_SIMPLE_SEQ, "Param1 is %i\n", (int) Data1);
        DEBUG(DEB_LEV_SIMPLE_SEQ, "Param2 is %i\n", (int) Data2);
    }

    return OMX_ErrorNone;
}

OMX_ERRORTYPE audiodecEmptyBufferDone(
                                      OMX_HANDLETYPE hComponent,
                                      OMX_PTR pAppData,
                                      OMX_BUFFERHEADERTYPE* pBuffer)
{
    OMX_ERRORTYPE err;
    static int iBufferDropped = 0;
    DEBUG(DEB_LEV_FULL_SEQ, "Hi there, I am in the %s callback.\n", __func__);

    if (pBuffer != NULL)
    {
        if (!bEOS)
        {
            if (outBufferParseAudio[0]->pBuffer == pBuffer->pBuffer)
            {
                outBufferParseAudio[0]->nFilledLen = 0;
                err = OMX_FillThisBuffer(appPriv->parser3gphandle, outBufferParseAudio[0]);
            }
            else
            {
                outBufferParseAudio[1]->nFilledLen = 0;
                err = OMX_FillThisBuffer(appPriv->parser3gphandle, outBufferParseAudio[1]);
            }
            if (err != OMX_ErrorNone)
            {
                DEBUG(DEB_LEV_ERR, "In %s Error %08x Calling FillThisBuffer\n", __func__, err);
            }
        }
        else
        {
            DEBUG(DEB_LEV_SIMPLE_SEQ, "In %s: eos=%x Dropping Fill This Buffer\n", __func__, (int) pBuffer->nFlags);
            iBufferDropped++;
            if (iBufferDropped == 2 && !flagIsDisplayRequested)
            {
                tsem_up(appPriv->eofSem);
            }
        }
    }
    else
    {
        if (!bEOS)
        {
            DEBUG(DEB_LEV_SIMPLE_SEQ, "It is here EOS = %d\n", appPriv->eofSem->semval);
            tsem_up(appPriv->eofSem);
        }
        DEBUG(DEB_LEV_ERR, "Ouch! In %s: had NULL buffer to output...\n", __func__);
    }
    return OMX_ErrorNone;
}

OMX_ERRORTYPE audiodecFillBufferDone(
                                     OMX_HANDLETYPE hComponent,
                                     OMX_PTR pAppData,
                                     OMX_BUFFERHEADERTYPE* pBuffer)
{
    OMX_ERRORTYPE err;
    int i;
    DEBUG(DEB_LEV_FUNCTION_NAME, "In %s \n", __func__);
    /* Output data to alsa sink */
    if (pBuffer != NULL)
    {
        if (!bEOS)
        {
            if (pBuffer->nFilledLen == 0)
            {
                DEBUG(DEB_LEV_ERR, "Ouch! In %s: had 0 data size in output buffer...\n", __func__);
                return OMX_ErrorNone;
            }
            if ((!flagDecodedOutputReceived) && (!flagIsDisplayRequested)/*(!flagOutputReceived) && (!flagPlaybackOn) && (flagDirect)*/)
            {
                for (i = 0; i < pBuffer->nFilledLen; i++)
                {
                    putchar(*(char*) (pBuffer->pBuffer + i));
                }
                pBuffer->nFilledLen = 0;
                err = OMX_FillThisBuffer(hComponent, pBuffer);
                if (err != OMX_ErrorNone)
                {
                    DEBUG(DEB_LEV_ERR, "In %s Error %08x Calling FillThisBuffer\n", __func__, err);
                }
            }
            else if ((flagDecodedOutputReceived) && (!flagIsDisplayRequested))
            {
                if (pBuffer->nFilledLen > 0)
                {
                    fwrite(pBuffer->pBuffer, 1, pBuffer->nFilledLen, outfileAudio);
                }
                pBuffer->nFilledLen = 0;
                err = OMX_FillThisBuffer(hComponent, pBuffer);
                if (err != OMX_ErrorNone)
                {
                    DEBUG(DEB_LEV_ERR, "In %s Error %08x Calling FillThisBuffer\n", __func__, err);
                }
            }
            else if ((!flagSetupTunnel) && (flagIsDisplayRequested))
            { //playback on, redirect to alsa sink, if it is not tunneled
                if (inBufferVolume[0]->pBuffer == pBuffer->pBuffer)
                {
                    inBufferVolume[0]->nFilledLen = pBuffer->nFilledLen;
                    inBufferVolume[0]->nTimeStamp = pBuffer->nTimeStamp;
                    if ((pBuffer->nFlags & OMX_BUFFERFLAG_STARTTIME) == OMX_BUFFERFLAG_STARTTIME)
                    {
                        inBufferVolume[0]->nFlags = pBuffer->nFlags;
                        pBuffer->nFlags = 0;
                    }
                    err = OMX_EmptyThisBuffer(appPriv->volumehandle, inBufferVolume[0]);
                }
                else
                {
                    inBufferVolume[1]->nFilledLen = pBuffer->nFilledLen;
                    inBufferVolume[1]->nTimeStamp = pBuffer->nTimeStamp;
                    if ((pBuffer->nFlags & OMX_BUFFERFLAG_STARTTIME) == OMX_BUFFERFLAG_STARTTIME)
                    {
                        inBufferVolume[1]->nFlags = pBuffer->nFlags;
                        pBuffer->nFlags = 0;
                    }
                    err = OMX_EmptyThisBuffer(appPriv->volumehandle, inBufferVolume[1]);
                }
                if (err != OMX_ErrorNone)
                {
                    DEBUG(DEB_LEV_ERR, "In %s Error %08x Calling EmptyThisBuffer\n", __func__, err);
                }
            }
        }
        else
        {
            DEBUG(DEB_LEV_ERR, " Buffer Dropping in %s...\n", __func__);
        }
    }
    else
    {
        DEBUG(DEB_LEV_ERR, "Ouch! In %s: had NULL buffer to output...\n", __func__);
    }
    return OMX_ErrorNone;
}

OMX_ERRORTYPE volumeEventHandler(
                                 OMX_HANDLETYPE hComponent,
                                 OMX_PTR pAppData,
                                 OMX_EVENTTYPE eEvent,
                                 OMX_U32 Data1,
                                 OMX_U32 Data2,
                                 OMX_PTR pEventData)
{

    DEBUG(DEB_LEV_SIMPLE_SEQ, "Hi there, I am in the %s callback\n", __func__);
    if (eEvent == OMX_EventCmdComplete)
    {
        if (Data1 == OMX_CommandStateSet)
        {
            DEBUG(DEB_LEV_SIMPLE_SEQ/*SIMPLE_SEQ*/, "Audio Decoder State changed in ");
            switch ((int) Data2)
            {
            case OMX_StateInvalid:
                DEBUG(DEB_LEV_SIMPLE_SEQ, "OMX_StateInvalid\n");
                break;
            case OMX_StateLoaded:
                DEBUG(DEB_LEV_SIMPLE_SEQ, "OMX_StateLoaded\n");
                break;
            case OMX_StateIdle:
                DEBUG(DEB_LEV_SIMPLE_SEQ, "OMX_StateIdle\n");
                break;
            case OMX_StateExecuting:
                DEBUG(DEB_LEV_SIMPLE_SEQ, "OMX_StateExecuting\n");
                break;
            case OMX_StatePause:
                DEBUG(DEB_LEV_SIMPLE_SEQ, "OMX_StatePause\n");
                break;
            case OMX_StateWaitForResources:
                DEBUG(DEB_LEV_SIMPLE_SEQ, "OMX_StateWaitForResources\n");
                break;
            }
            tsem_up(appPriv->volumeEventSem);
        }
        else if (Data1 == OMX_CommandPortEnable)
        {
            DEBUG(DEB_LEV_SIMPLE_SEQ, "In %s Received Port Enable  Event\n", __func__);
            tsem_up(appPriv->volumeEventSem);
        }
        else if (Data1 == OMX_CommandPortDisable)
        {
            DEBUG(DEB_LEV_SIMPLE_SEQ, "In %s Received Port Disable Event\n", __func__);
            tsem_up(appPriv->volumeEventSem);
        }
        else if (Data1 == OMX_CommandFlush)
        {
            DEBUG(DEB_LEV_SIMPLE_SEQ, "In %s Received Port Flush Event\n", __func__);
            tsem_up(appPriv->volumeEventSem);
        }
    }
    else if (eEvent == OMX_EventBufferFlag)
    {
        DEBUG(DEB_LEV_SIMPLE_SEQ, "In %s OMX_BUFFERFLAG_EOS\n", __func__);
        if ((int) Data2 == OMX_BUFFERFLAG_EOS)
        {
            //      tsem_up(appPriv->eofSem);
        }
    }
    else
    {
        DEBUG(DEB_LEV_SIMPLE_SEQ, "Param1 is %i\n", (int) Data1);
        DEBUG(DEB_LEV_SIMPLE_SEQ, "Param2 is %i\n", (int) Data2);
    }

    return OMX_ErrorNone;
}

OMX_ERRORTYPE volumeEmptyBufferDone(
                                    OMX_HANDLETYPE hComponent,
                                    OMX_PTR pAppData,
                                    OMX_BUFFERHEADERTYPE* pBuffer)
{
    OMX_ERRORTYPE err;
    static int iBufferDropped = 0;
    DEBUG(DEB_LEV_FULL_SEQ, "Hi there, I am in the %s callback.\n", __func__);

    if (pBuffer != NULL)
    {
        if (!bEOS)
        {
            if (outBufferAudioDec[0]->pBuffer == pBuffer->pBuffer)
            {
                outBufferAudioDec[0]->nFilledLen = 0;
                err = OMX_FillThisBuffer(appPriv->audiodechandle, outBufferAudioDec[0]);
            }
            else
            {
                outBufferAudioDec[1]->nFilledLen = 0;
                err = OMX_FillThisBuffer(appPriv->audiodechandle, outBufferAudioDec[1]);
            }
            if (err != OMX_ErrorNone)
            {
                DEBUG(DEB_LEV_ERR, "In %s Error %08x Calling FillThisBuffer\n", __func__, err);
            }
        }
        else
        {
            DEBUG(DEB_LEV_SIMPLE_SEQ, "In %s: eos=%x Dropping Fill This Buffer\n", __func__, (int) pBuffer->nFlags);
            iBufferDropped++;
            if (iBufferDropped == 2)
            {
                tsem_up(appPriv->eofSem);
            }
        }
    }
    else
    {
        if (!bEOS)
        {
            DEBUG(DEFAULT_MESSAGES, "It is here EOS = %d\n", appPriv->eofSem->semval);
            tsem_up(appPriv->eofSem);
        }
        DEBUG(DEB_LEV_ERR, "Ouch! In %s: had NULL buffer to output...\n", __func__);
    }
    return OMX_ErrorNone;
}

OMX_ERRORTYPE volumeFillBufferDone(
                                   OMX_HANDLETYPE hComponent,
                                   OMX_PTR pAppData,
                                   OMX_BUFFERHEADERTYPE* pBuffer)
{
    OMX_ERRORTYPE err;
    int i;
    static int volCompBufferDropped = 0;
    DEBUG(DEB_LEV_FUNCTION_NAME, "In %s \n", __func__);
    /* Output data to alsa sink */
    if (pBuffer != NULL)
    {
        if (pBuffer->nFilledLen == 0)
        {
            DEBUG(DEB_LEV_ERR, "Ouch! In %s: had 0 data size in output buffer...\n", __func__);
            return OMX_ErrorNone;
        }
        if ((!flagDecodedOutputReceived) && (!flagIsDisplayRequested))
        {
            for (i = 0; i < pBuffer->nFilledLen; i++)
            {
                putchar(*(char*) (pBuffer->pBuffer + i));
            }
            pBuffer->nFilledLen = 0;
            err = OMX_FillThisBuffer(hComponent, pBuffer);
            if (err != OMX_ErrorNone)
            {
                DEBUG(DEB_LEV_ERR, "In %s Error %08x Calling FillThisBuffer\n", __func__, err);
            }
        }
        else if (flagDecodedOutputReceived && (!flagIsDisplayRequested))
        {
            if (pBuffer->nFilledLen > 0)
            {
                fwrite(pBuffer->pBuffer, 1, pBuffer->nFilledLen, outfileAudio);
            }
            pBuffer->nFilledLen = 0;
            err = OMX_FillThisBuffer(hComponent, pBuffer);
            if (err != OMX_ErrorNone)
            {
                DEBUG(DEB_LEV_ERR, "In %s Error %08x Calling FillThisBuffer\n", __func__, err);
            }
        }
        else if ((!flagSetupTunnel) && (flagIsDisplayRequested))
        { //playback on, redirect to alsa sink, if it is not tunneled
            if (!bEOS)
            {
                if (inBufferSinkAudio[0]->pBuffer == pBuffer->pBuffer)
                {
                    inBufferSinkAudio[0]->nFilledLen = pBuffer->nFilledLen;
                    inBufferSinkAudio[0]->nTimeStamp = pBuffer->nTimeStamp;
                    if ((pBuffer->nFlags & OMX_BUFFERFLAG_STARTTIME) == OMX_BUFFERFLAG_STARTTIME)
                    {
                        inBufferSinkAudio[0]->nFlags = pBuffer->nFlags;
                        pBuffer->nFlags = 0;
                    }
                    err = OMX_EmptyThisBuffer(appPriv->audiosinkhandle, inBufferSinkAudio[0]);
                }
                else
                {
                    inBufferSinkAudio[1]->nFilledLen = pBuffer->nFilledLen;
                    inBufferSinkAudio[1]->nTimeStamp = pBuffer->nTimeStamp;
                    if ((pBuffer->nFlags & OMX_BUFFERFLAG_STARTTIME) == OMX_BUFFERFLAG_STARTTIME)
                    {
                        inBufferSinkAudio[1]->nFlags = pBuffer->nFlags;
                        pBuffer->nFlags = 0;
                    }
                    err = OMX_EmptyThisBuffer(appPriv->audiosinkhandle, inBufferSinkAudio[1]);
                }
                if (err != OMX_ErrorNone)
                {
                    DEBUG(DEB_LEV_ERR, "In %s Error %08x Calling EmptyThisBuffer\n", __func__, err);
                }
            }
            else
            {
                DEBUG(DEFAULT_MESSAGES, "In %s EOS reached\n", __func__);
                volCompBufferDropped++;
                if (volCompBufferDropped == 2)
                {
                    tsem_up(appPriv->eofSem);
                }
            }
        }
    }
    else
    {
        DEBUG(DEB_LEV_ERR, "Ouch! In %s: had NULL buffer to output...\n", __func__);
    }
    return OMX_ErrorNone;
}

OMX_ERRORTYPE audiosinkEventHandler(
                                    OMX_HANDLETYPE hComponent,
                                    OMX_PTR pAppData,
                                    OMX_EVENTTYPE eEvent,
                                    OMX_U32 Data1,
                                    OMX_U32 Data2,
                                    OMX_PTR pEventData)
{
    DEBUG(DEB_LEV_SIMPLE_SEQ, "Hi there, I am in the %s callback\n", __func__);
    if (Data1 == OMX_CommandStateSet)
    {
        DEBUG(DEB_LEV_SIMPLE_SEQ, "Audio Sink State changed in ");
        switch ((int) Data2)
        {
        case OMX_StateInvalid:
            DEBUG(DEB_LEV_SIMPLE_SEQ, "OMX_StateInvalid\n");
            break;
        case OMX_StateLoaded:
            DEBUG(DEB_LEV_SIMPLE_SEQ, "OMX_StateLoaded\n");
            break;
        case OMX_StateIdle:
            DEBUG(DEB_LEV_SIMPLE_SEQ, "OMX_StateIdle\n");
            break;
        case OMX_StateExecuting:
            DEBUG(DEB_LEV_SIMPLE_SEQ, "OMX_StateExecuting\n");
            break;
        case OMX_StatePause:
            DEBUG(DEB_LEV_SIMPLE_SEQ, "OMX_StatePause\n");
            break;
        case OMX_StateWaitForResources:
            DEBUG(DEB_LEV_SIMPLE_SEQ, "OMX_StateWaitForResources\n");
            break;
        }
        tsem_up(appPriv->audioSinkEventSem);
    }
    else if (Data1 == OMX_CommandPortEnable)
    {
        DEBUG(DEB_LEV_SIMPLE_SEQ, "In %s Received Port Enable  Event\n", __func__);
        tsem_up(appPriv->audioSinkEventSem);
    }
    else if (Data1 == OMX_CommandPortDisable)
    {
        DEBUG(DEB_LEV_SIMPLE_SEQ, "In %s Received Port Disable Event\n", __func__);
        tsem_up(appPriv->audioSinkEventSem);
    }
    else if (Data1 == OMX_CommandFlush)
    {
        DEBUG(DEB_LEV_SIMPLE_SEQ, "In %s Received Port Flush Event\n", __func__);
        tsem_up(appPriv->audioSinkEventSem);
    }
    else if (eEvent == OMX_EventBufferFlag)
    {
        DEBUG(DEB_LEV_ERR, "In %s OMX_BUFFERFLAG_EOS\n", __func__);
        if ((int) Data2 == OMX_BUFFERFLAG_EOS)
        {
            tsem_up(appPriv->eofSem);
        }
    }
    else
    {
        DEBUG(DEB_LEV_SIMPLE_SEQ, "Param1 is %i\n", (int) Data1);
        DEBUG(DEB_LEV_SIMPLE_SEQ, "Param2 is %i\n", (int) Data2);
    }

    return OMX_ErrorNone;
}

OMX_ERRORTYPE audiosinkEmptyBufferDone(
                                       OMX_HANDLETYPE hComponent,
                                       OMX_PTR pAppData,
                                       OMX_BUFFERHEADERTYPE* pBuffer)
{
    OMX_ERRORTYPE err;
    static int alsaSinkBufferDropped = 0;
    DEBUG(DEB_LEV_FULL_SEQ, "Hi there, I am in the %s callback.\n", __func__);

    DEBUG(DEB_LEV_PARAMS, "Empty buffer %x\n", (int) pBuffer);
    if (!bEOS)
    {
        if (outBufferVolume[0]->pBuffer == pBuffer->pBuffer)
        {
            outBufferVolume[0]->nFilledLen = 0;
            err = OMX_FillThisBuffer(appPriv->volumehandle, outBufferVolume[0]);
        }
        else
        {
            outBufferVolume[1]->nFilledLen = 0;
            err = OMX_FillThisBuffer(appPriv->volumehandle, outBufferVolume[1]);
        }
        if (err != OMX_ErrorNone)
        {
            DEBUG(DEB_LEV_ERR, "In %s Error %08x Calling FillThisBuffer\n", __func__, err);
        }
    }
    else
    {
        DEBUG(DEFAULT_MESSAGES, "In %s EOS reached\n", __func__);
        alsaSinkBufferDropped++;
        if (alsaSinkBufferDropped == 2)
        {
            tsem_up(appPriv->eofSem);
        }
    }

    return OMX_ErrorNone;
}

OMX_ERRORTYPE clocksrcEventHandler(
                                   OMX_HANDLETYPE hComponent,
                                   OMX_PTR pAppData,
                                   OMX_EVENTTYPE eEvent,
                                   OMX_U32 Data1,
                                   OMX_U32 Data2,
                                   OMX_PTR pEventData)
{
    DEBUG(DEB_LEV_FULL_SEQ, "Hi there, I am in the %s callback\n", __func__);

    if (eEvent == OMX_EventCmdComplete)
    {
        if (Data1 == OMX_CommandStateSet)
        {
            DEBUG(DEB_LEV_SIMPLE_SEQ, "Clock Component State changed in ");
            switch ((int) Data2)
            {
            case OMX_StateInvalid:
                DEBUG(DEB_LEV_SIMPLE_SEQ, "OMX_StateInvalid\n");
                break;
            case OMX_StateLoaded:
                DEBUG(DEB_LEV_SIMPLE_SEQ, "OMX_StateLoaded\n");
                break;
            case OMX_StateIdle:
                DEBUG(DEB_LEV_SIMPLE_SEQ, "OMX_StateIdle\n");
                break;
            case OMX_StateExecuting:
                DEBUG(DEB_LEV_SIMPLE_SEQ, "OMX_StateExecuting\n");
                break;
            case OMX_StatePause:
                DEBUG(DEB_LEV_SIMPLE_SEQ, "OMX_StatePause\n");
                break;
            case OMX_StateWaitForResources:
                DEBUG(DEB_LEV_SIMPLE_SEQ, "OMX_StateWaitForResources\n");
                break;
            }
            tsem_up(appPriv->clockEventSem);
        }
        else if (Data1 == OMX_CommandPortEnable)
        {
            DEBUG(DEB_LEV_SIMPLE_SEQ, "In %s Received Port Enable  Event\n", __func__);
            tsem_up(appPriv->clockEventSem);
        }
        else if (Data1 == OMX_CommandPortDisable)
        {
            DEBUG(DEB_LEV_SIMPLE_SEQ, "In %s Received Port Disable Event\n", __func__);
            tsem_up(appPriv->clockEventSem);
        }
        else
        {
            DEBUG(DEB_LEV_SIMPLE_SEQ, "In %s Received Event Event=%d Data1=%d,Data2=%d\n", __func__, eEvent, (int) Data1, (int) Data2);
        }
    }
    else if (eEvent == OMX_EventPortSettingsChanged)
    {
        DEBUG(DEB_LEV_SIMPLE_SEQ, "Clock src  Port Setting Changed event\n");
        /*Signal Port Setting Changed*/
        tsem_up(appPriv->clockEventSem);
    }
    else if (eEvent == OMX_EventPortFormatDetected)
    {
        DEBUG(DEB_LEV_SIMPLE_SEQ, "In %s Port Format Detected %x\n", __func__, (int) Data1);
    }
    else if (eEvent == OMX_EventBufferFlag)
    {
        DEBUG(DEB_LEV_SIMPLE_SEQ, "In %s OMX_BUFFERFLAG_EOS\n", __func__);
        if ((int) Data2 == OMX_BUFFERFLAG_EOS)
        {
            //      tsem_up(appPriv->eofSem);
        }
    }
    else
    {
        DEBUG(DEB_LEV_SIMPLE_SEQ, "Param1 is %i\n", (int) Data1);
        DEBUG(DEB_LEV_SIMPLE_SEQ, "Param2 is %i\n", (int) Data2);
    }
    return OMX_ErrorNone;
}

OMX_ERRORTYPE clocksrcFillBufferDone(
                                     OMX_HANDLETYPE hComponent,
                                     OMX_PTR pAppData,
                                     OMX_BUFFERHEADERTYPE* pBuffer)
{
    OMX_ERRORTYPE err;
    /* Output data to audio decoder */

    if (pBuffer != NULL)
    {
        if (!bEOS)
        {
            err = OMX_EmptyThisBuffer(appPriv->clocksrchandle, pBuffer);
            if (err != OMX_ErrorNone)
            {
                DEBUG(DEB_LEV_ERR, "In %s Error %08x Calling FillThisBuffer\n", __func__, err);
            }
            if ((pBuffer->nFlags & OMX_BUFFERFLAG_EOS) == OMX_BUFFERFLAG_EOS)
            {
                DEBUG(DEB_LEV_SIMPLE_SEQ, "In %s: eos=%x Calling Empty This Buffer\n", __func__, (int) pBuffer->nFlags);
                bEOS = OMX_TRUE;
            }
        }
        else
        {
            DEBUG(DEB_LEV_SIMPLE_SEQ, "In %s: eos=%x Dropping Empty This Buffer\n", __func__, (int) pBuffer->nFlags);
        }
    }
    else
    {
        DEBUG(DEB_LEV_ERR, "Ouch! In %s: had NULL buffer to output...\n", __func__);
    }
    return OMX_ErrorNone;
}

