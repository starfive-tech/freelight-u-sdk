// SPDX-License-Identifier: GPL-2.0
/*
 * Copyright (C) 2021 StarFive Technology Co., Ltd.
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "SF_OMX_video_common.h"
#include "SF_OMX_Core.h"

#define WAVE521_CONFIG_FILE "/lib/firmware/encoder_defconfig.cfg"
extern OMX_TICKS gInitTimeStamp;

static void OnEventArrived(Component com, unsigned long event, void *data, void *context)
{
    FunctionIn();
    SF_OMX_COMPONENT *pSfOMXComponent = GetSFOMXComponrntByComponent(com);
    ComponentImpl *pSFComponent = (ComponentImpl *)com;
    PortContainerExternal *pPortContainerExternal = (PortContainerExternal *)data;
    OMX_BUFFERHEADERTYPE *pOMXBuffer;

    LOG(SF_LOG_INFO, "event=%lX\r\n", event);
    switch (event)
    {
    case COMPONENT_EVENT_ENC_EMPTY_BUFFER_DONE:
        pOMXBuffer = GetOMXBufferByAddr(pSfOMXComponent, (OMX_U8 *)pPortContainerExternal->pBuffer);
        pSfOMXComponent->callbacks->EmptyBufferDone(pSfOMXComponent->pOMXComponent, pSfOMXComponent->pAppData, pOMXBuffer);
        LOG(SF_LOG_PERF, "OMX empty one buffer, address = %p, size = %d, nTimeStamp = %d, nFlags = %X\r\n",
        pOMXBuffer->pBuffer, pOMXBuffer->nFilledLen, pOMXBuffer->nTimeStamp, pOMXBuffer->nFlags);
        sem_post(pSfOMXComponent->inputSemaphore);
        break;
    case COMPONENT_EVENT_ENC_FILL_BUFFER_DONE:
    {
        struct timeval tv;
        pOMXBuffer = GetOMXBufferByAddr(pSfOMXComponent, (OMX_U8 *)pPortContainerExternal->pBuffer);
        gettimeofday(&tv, NULL);
        pOMXBuffer->nFilledLen = pPortContainerExternal->nFilledLen;
        pOMXBuffer->nTimeStamp = tv.tv_sec * 1000000 + tv.tv_usec - gInitTimeStamp;
        pOMXBuffer->nFlags = pPortContainerExternal->nFlags;
#if 0
            {
                FILE *fb = fopen("./out.bcp", "ab+");
                LOG(SF_LOG_ERR, "%d %d\r\n", pOMXBuffer->nFilledLen, pOMXBuffer->nOffset);
                fwrite(pOMXBuffer->pBuffer, 1, pOMXBuffer->nFilledLen, fb);
                fclose(fb);
            }
#endif
        LOG(SF_LOG_PERF, "OMX finish one buffer, address = %p, size = %d, nTimeStamp = %d, nFlags = %X\r\n", pOMXBuffer->pBuffer, pOMXBuffer->nFilledLen, pOMXBuffer->nTimeStamp, pOMXBuffer->nFlags);
        pSfOMXComponent->callbacks->FillBufferDone(pSfOMXComponent->pOMXComponent, pSfOMXComponent->pAppData, pOMXBuffer);
    }
    break;
    case COMPONENT_EVENT_ENC_REGISTER_FB:
    {
        struct timeval tv;
        gettimeofday(&tv, NULL);
        gInitTimeStamp = tv.tv_sec * 1000000 + tv.tv_usec;
        pSfOMXComponent->callbacks->EventHandler(pSfOMXComponent->pOMXComponent, pSfOMXComponent->pAppData, OMX_EventPortSettingsChanged,
                                                 1, OMX_IndexParamPortDefinition, NULL);
    }
    break;
    case COMPONENT_EVENT_ENC_ENCODED_ALL:
        pSfOMXComponent->callbacks->EventHandler(pSfOMXComponent->pOMXComponent, pSfOMXComponent->pAppData, OMX_EventBufferFlag,
                                                 1, 1, NULL);
    default:
        break;
    }

    FunctionOut();
}

static OMX_ERRORTYPE SF_OMX_EmptyThisBuffer(
    OMX_IN OMX_HANDLETYPE hComponent,
    OMX_IN OMX_BUFFERHEADERTYPE *pBuffer)
{
    OMX_ERRORTYPE ret = OMX_ErrorNone;
    FunctionIn();

    if (hComponent == NULL)
    {
        ret = OMX_ErrorBadParameter;
        goto EXIT;
    }

    OMX_COMPONENTTYPE *pOMXComponent = (OMX_COMPONENTTYPE *)hComponent;

    SF_OMX_COMPONENT *pSfOMXComponent = pOMXComponent->pComponentPrivate;

    ComponentImpl *pFeederComponent = pSfOMXComponent->hSFComponentFeeder;

    sem_wait(pSfOMXComponent->inputSemaphore);
    PortContainerExternal *pPortContainerExternal = malloc(sizeof(PortContainerExternal));
    if (pPortContainerExternal == NULL)
    {
        LOG(SF_LOG_ERR, "malloc fail\r\n");
        return OMX_ErrorInsufficientResources;
    }
    memset(pPortContainerExternal, 0, sizeof(PortContainerExternal));
    pPortContainerExternal->pBuffer = pBuffer->pBuffer;
    pPortContainerExternal->nFilledLen = pBuffer->nFilledLen;
    pPortContainerExternal->nFlags = pBuffer->nFlags;
    LOG(SF_LOG_PERF, "Buffer flag = %X\r\n", pBuffer->nFlags);
    if (pSfOMXComponent->functions->Queue_Enqueue(pFeederComponent->srcPort.inputQ, (void *)pPortContainerExternal) != OMX_TRUE)
    {
        LOG(SF_LOG_ERR, "%p:%p FAIL\r\n", pFeederComponent->srcPort.inputQ, pPortContainerExternal);
        free(pPortContainerExternal);
        return OMX_ErrorInsufficientResources;
    }
    LOG(SF_LOG_PERF, "input queue count=%d/%d\r\n", pSfOMXComponent->functions->Queue_Get_Cnt(pFeederComponent->srcPort.inputQ), 
                                                    pSfOMXComponent->portDefinition[0].nBufferCountActual);
    free(pPortContainerExternal);
    pFeederComponent->pause = OMX_FALSE;

EXIT:
    FunctionOut();

    return ret;
}
static OMX_ERRORTYPE SF_OMX_FillThisBuffer(
    OMX_IN OMX_HANDLETYPE hComponent,
    OMX_IN OMX_BUFFERHEADERTYPE *pBuffer)
{
    OMX_ERRORTYPE ret = OMX_ErrorNone;
    FunctionIn();

    if (hComponent == NULL)
    {
        ret = OMX_ErrorBadParameter;
        goto EXIT;
    }

    OMX_COMPONENTTYPE *pOMXComponent = (OMX_COMPONENTTYPE *)hComponent;
    SF_OMX_COMPONENT *pSfOMXComponent = (SF_OMX_COMPONENT *)pOMXComponent->pComponentPrivate;
    ComponentImpl *pRendererComponent = pSfOMXComponent->hSFComponentRender;
    PortContainerExternal *pPortContainerExternal = malloc(sizeof(PortContainerExternal));
    if (pPortContainerExternal == NULL)
    {
        LOG(SF_LOG_ERR, "malloc fail\r\n");
        return OMX_ErrorInsufficientResources;
    }
    memset(pPortContainerExternal, 0, sizeof(PortContainerExternal));
    pPortContainerExternal->pBuffer = pBuffer->pBuffer;
    pPortContainerExternal->nFilledLen = pBuffer->nAllocLen;
    if (pSfOMXComponent->functions->Queue_Enqueue(pRendererComponent->sinkPort.inputQ, (void *)pPortContainerExternal) == FALSE)
    {
        LOG(SF_LOG_ERR, "%p:%p FAIL\r\n", pRendererComponent->sinkPort.inputQ, pPortContainerExternal);
        free(pPortContainerExternal);
        return OMX_ErrorInsufficientResources;
    }
    LOG(SF_LOG_PERF, "output queue count=%d/%d\r\n", pSfOMXComponent->functions->Queue_Get_Cnt(pRendererComponent->sinkPort.inputQ),
                                                        pSfOMXComponent->portDefinition[1].nBufferCountActual);
    free(pPortContainerExternal);
    pRendererComponent->pause = OMX_FALSE;
EXIT:
    FunctionOut();

    return ret;
}

static OMX_ERRORTYPE SF_OMX_UseBuffer(
    OMX_IN OMX_HANDLETYPE hComponent,
    OMX_INOUT OMX_BUFFERHEADERTYPE **ppBufferHdr,
    OMX_IN OMX_U32 nPortIndex,
    OMX_IN OMX_PTR pAppPrivate,
    OMX_IN OMX_U32 nSizeBytes,
    OMX_IN OMX_U8 *pBuffer)
{
    OMX_ERRORTYPE ret = OMX_ErrorNone;
    OMX_COMPONENTTYPE *pOMXComponent = (OMX_COMPONENTTYPE *)hComponent;
    SF_OMX_COMPONENT *pSfOMXComponent = pOMXComponent->pComponentPrivate;

    FunctionIn();

    if (hComponent == NULL)
    {
        ret = OMX_ErrorBadParameter;
        goto EXIT;
    }

    OMX_BUFFERHEADERTYPE *temp_bufferHeader = (OMX_BUFFERHEADERTYPE *)malloc(sizeof(OMX_BUFFERHEADERTYPE));
    if (temp_bufferHeader == NULL)
    {
        LOG(SF_LOG_ERR, "malloc fail\r\n");
        return OMX_ErrorInsufficientResources;
    }
    memset(temp_bufferHeader, 0, sizeof(OMX_BUFFERHEADERTYPE));
    temp_bufferHeader->nAllocLen = nSizeBytes;
    temp_bufferHeader->pAppPrivate = pAppPrivate;
    temp_bufferHeader->pBuffer = pBuffer;
    *ppBufferHdr = temp_bufferHeader;

    ret = StoreOMXBuffer(pSfOMXComponent, temp_bufferHeader);
EXIT:
    FunctionOut();

    return ret;
}

static OMX_ERRORTYPE SF_OMX_AllocateBuffer(
    OMX_IN OMX_HANDLETYPE hComponent,
    OMX_INOUT OMX_BUFFERHEADERTYPE **ppBuffer,
    OMX_IN OMX_U32 nPortIndex,
    OMX_IN OMX_PTR pAppPrivate,
    OMX_IN OMX_U32 nSizeBytes)
{
    OMX_ERRORTYPE ret = OMX_ErrorNone;
    OMX_COMPONENTTYPE *pOMXComponent = (OMX_COMPONENTTYPE *)hComponent;
    SF_OMX_COMPONENT *pSfOMXComponent = pOMXComponent->pComponentPrivate;
    OMX_U32 i = 0;

    OMX_BUFFERHEADERTYPE *temp_bufferHeader = (OMX_BUFFERHEADERTYPE *)malloc(sizeof(OMX_BUFFERHEADERTYPE));
    if (temp_bufferHeader == NULL)
    {
        LOG(SF_LOG_ERR, "malloc fail\r\n");
        return OMX_ErrorInsufficientResources;
    }
    memset(temp_bufferHeader, 0, sizeof(OMX_BUFFERHEADERTYPE));
    temp_bufferHeader->pBuffer = malloc(nSizeBytes);
    if (temp_bufferHeader->pBuffer == NULL)
    {
        free(temp_bufferHeader);
        LOG(SF_LOG_ERR, "malloc fail\r\n");
        return OMX_ErrorInsufficientResources;
    }
    memset(temp_bufferHeader->pBuffer, 0, nSizeBytes);
    temp_bufferHeader->nAllocLen = nSizeBytes;
    temp_bufferHeader->pAppPrivate = pAppPrivate;
    temp_bufferHeader->nOutputPortIndex = temp_bufferHeader->nInputPortIndex = -1;
    if (nPortIndex == 1) {
        temp_bufferHeader->nOutputPortIndex = 1;
    } 
    else if (nPortIndex == 0)
    {
        temp_bufferHeader->nInputPortIndex = 0;
    }
    *ppBuffer = temp_bufferHeader;

    ret = StoreOMXBuffer(pSfOMXComponent, temp_bufferHeader);
    LOG(SF_LOG_PERF, "buffer count = %d\r\n", GetOMXBufferCount(pSfOMXComponent));
EXIT:
    FunctionOut();

    return ret;
}

static OMX_ERRORTYPE SF_OMX_GetParameter(
    OMX_IN OMX_HANDLETYPE hComponent,
    OMX_IN OMX_INDEXTYPE nParamIndex,
    OMX_INOUT OMX_PTR ComponentParameterStructure)
{
    OMX_ERRORTYPE ret = OMX_ErrorNone;
    OMX_COMPONENTTYPE *pOMXComponent = (OMX_COMPONENTTYPE *)hComponent;
    SF_OMX_COMPONENT *pSfOMXComponent = pOMXComponent->pComponentPrivate;
    OMX_PARAM_PORTDEFINITIONTYPE *pInputPort = &pSfOMXComponent->portDefinition[0];
    OMX_PARAM_PORTDEFINITIONTYPE *pOutputPort = &pSfOMXComponent->portDefinition[1];

    FunctionIn();

    if (hComponent == NULL)
    {
        ret = OMX_ErrorBadParameter;
        goto EXIT;
    }
    LOG(SF_LOG_INFO, "Get parameter on index %X\r\n", nParamIndex);
    switch ((OMX_U32)nParamIndex)
    {
    case OMX_IndexParamVideoInit:
    {
        OMX_PORT_PARAM_TYPE *portParam = (OMX_PORT_PARAM_TYPE *)ComponentParameterStructure;
        portParam->nPorts           = 2;
        portParam->nStartPortNumber = 0;
    }
    break;
    case OMX_IndexParamVideoPortFormat:
    {
        OMX_VIDEO_PARAM_PORTFORMATTYPE *portFormat = (OMX_VIDEO_PARAM_PORTFORMATTYPE *)ComponentParameterStructure;
        OMX_U32 index = portFormat->nIndex;
        LOG(SF_LOG_INFO, "Get video port format at index %d\r\n", index);
        switch (index)
        {
        case 0:
            portFormat->eCompressionFormat = OMX_VIDEO_CodingUnused;
            portFormat->eColorFormat = OMX_COLOR_FormatYUV420SemiPlanar;
            portFormat->xFramerate = 30;
            break;
        case 1:
            portFormat->eCompressionFormat = OMX_VIDEO_CodingUnused;
            portFormat->eColorFormat = OMX_COLOR_FormatYUV420Planar;
            portFormat->xFramerate = 30;
            break;
        default:
            if (index > 0)
            {
                ret = OMX_ErrorNoMore;
            }
            break;
        }
    }

    break;
    case OMX_IndexParamVideoBitrate:
    {
        OMX_VIDEO_PARAM_BITRATETYPE     *videoRateControl = (OMX_VIDEO_PARAM_BITRATETYPE *)ComponentParameterStructure;
        OMX_U32                          portIndex = videoRateControl->nPortIndex;
        if ((portIndex != 1)) {
            ret = OMX_ErrorBadPortIndex;
            goto EXIT;
        }
        else
        {
            videoRateControl->nTargetBitrate = pOutputPort->format.video.nBitrate;
        }
        LOG(SF_LOG_INFO, "Get nTargetBitrate = %u on port %d\r\n",videoRateControl->nTargetBitrate, videoRateControl->nPortIndex);
    }
        break;
    case OMX_IndexParamVideoQuantization:

        break;
    case OMX_IndexParamPortDefinition:
    {
        OMX_PARAM_PORTDEFINITIONTYPE *pPortDefinition = (OMX_PARAM_PORTDEFINITIONTYPE *)ComponentParameterStructure;
        OMX_U32 portIndex = pPortDefinition->nPortIndex;
        memcpy(pPortDefinition, &pSfOMXComponent->portDefinition[portIndex], pPortDefinition->nSize);
        LOG(SF_LOG_INFO, "Get portDefinition on port %d\r\n", portIndex);
        LOG(SF_LOG_INFO, "Got width = %d, height = %d\r\n", pPortDefinition->format.video.nFrameWidth, pPortDefinition->format.video.nFrameHeight);
    }

    break;
    case OMX_IndexParamVideoIntraRefresh:

        break;

    case OMX_IndexParamStandardComponentRole:

        break;
    case OMX_IndexParamVideoAvc:
    {
        OMX_VIDEO_PARAM_AVCTYPE *pDstAVCComponent = (OMX_VIDEO_PARAM_AVCTYPE *)ComponentParameterStructure;
        OMX_U32 nPortIndex = pDstAVCComponent->nPortIndex;
        OMX_VIDEO_PARAM_AVCTYPE *pSrcAVCComponent = &pSfOMXComponent->AVCComponent[nPortIndex];

        LOG(SF_LOG_INFO, "Get nPFrames = %d on port %d\r\n", pSrcAVCComponent->nPFrames, nPortIndex);
        memcpy(pDstAVCComponent, pSrcAVCComponent, sizeof(OMX_VIDEO_PARAM_AVCTYPE));
    }
    break;
    case OMX_IndexParamVideoHevc:
    {
        OMX_VIDEO_PARAM_HEVCTYPE *pDstHEVCComponent = (OMX_VIDEO_PARAM_HEVCTYPE *)ComponentParameterStructure;
        OMX_U32 nPortIndex = pDstHEVCComponent->nPortIndex;
        OMX_VIDEO_PARAM_HEVCTYPE *pSrcHEVCComponent = &pSfOMXComponent->HEVCComponent[nPortIndex];

        LOG(SF_LOG_INFO, "Get nKeyFrameInterval = %d on port %d\r\n", pSrcHEVCComponent->nKeyFrameInterval, nPortIndex);
        memcpy(pDstHEVCComponent, pSrcHEVCComponent, sizeof(OMX_VIDEO_PARAM_HEVCTYPE));
    }
    break;
    case OMX_IndexParamVideoProfileLevelQuerySupported:

        break;
    case OMX_IndexParamVideoProfileLevelCurrent:
    {
        OMX_VIDEO_PARAM_PROFILELEVELTYPE *pParam = (OMX_VIDEO_PARAM_PROFILELEVELTYPE *)ComponentParameterStructure;
        LOG(SF_LOG_INFO, "Get ProfileLevel on port %d\r\n", pParam->nPortIndex);
        if (pParam->nPortIndex == 1)
        {
            pParam->eProfile = OMX_VIDEO_HEVCProfileMain;
            pParam->eLevel = OMX_VIDEO_HEVCMainTierLevel5;
            LOG(SF_LOG_INFO, "eProfile = OMX_VIDEO_HEVCProfileMain\r\n");
        }
    }
        // ret = OMX_ErrorNotImplemented;
        break;
    default:

        break;
    }

EXIT:
    FunctionOut();

    return ret;
}

static OMX_ERRORTYPE SF_OMX_SetParameter(
    OMX_IN OMX_HANDLETYPE hComponent,
    OMX_IN OMX_INDEXTYPE nIndex,
    OMX_IN OMX_PTR ComponentParameterStructure)
{
    OMX_ERRORTYPE ret = OMX_ErrorNone;

    FunctionIn();
    if (hComponent == NULL)
    {
        ret = OMX_ErrorBadParameter;
        goto EXIT;
    }

    OMX_COMPONENTTYPE *pOMXComponent = (OMX_COMPONENTTYPE *)hComponent;
    SF_OMX_COMPONENT *pSfOMXComponent = pOMXComponent->pComponentPrivate;
    OMX_PARAM_PORTDEFINITIONTYPE *pInputPort = &pSfOMXComponent->portDefinition[0];
    OMX_PARAM_PORTDEFINITIONTYPE *pOutputPort = &pSfOMXComponent->portDefinition[1];

    if (pSfOMXComponent == NULL)
    {
        ret = OMX_ErrorBadParameter;
        goto EXIT;
    }
    LOG(SF_LOG_INFO, "Set parameter on index %X\r\n", nIndex);
    switch ((OMX_U32)nIndex)
    {
    case OMX_IndexParamPortDefinition:
    {
        OMX_PARAM_PORTDEFINITIONTYPE *pPortDefinition = (OMX_PARAM_PORTDEFINITIONTYPE *)ComponentParameterStructure;
        OMX_U32 portIndex = pPortDefinition->nPortIndex;
        OMX_U32 width = pPortDefinition->format.video.nFrameWidth;
        OMX_U32 height = pPortDefinition->format.video.nFrameHeight;
        OMX_U32 nBitrate = pPortDefinition->format.video.nBitrate;
        OMX_U32 xFramerate = pPortDefinition->format.video.xFramerate;
        LOG(SF_LOG_INFO, "Set width = %d, height = %d xFramerate = %d, nBitrate = %d on port %d\r\n",
            width, height, xFramerate, nBitrate, pPortDefinition->nPortIndex);
        memcpy(&pSfOMXComponent->portDefinition[portIndex], pPortDefinition, pPortDefinition->nSize);
        OMX_COLOR_FORMATTYPE eColorFormat = pPortDefinition->format.video.eColorFormat;
        TestEncConfig *pTestEncConfig = (TestEncConfig *)pSfOMXComponent->testConfig;
        CNMComponentConfig *config = pSfOMXComponent->config;
        EncOpenParam *encOpenParam = &config->encOpenParam;

        if (portIndex == 0)
        {
            pInputPort->format.video.nStride = width;
            pInputPort->format.video.nSliceHeight = height;
            pInputPort->nBufferSize = width * height * 2;
            LOG(SF_LOG_INFO, "Set eColorFormat to %d\r\n", eColorFormat);
            switch (eColorFormat)
            {
            case OMX_COLOR_FormatYUV420Planar:
                pTestEncConfig->cbcrInterleave = FALSE;
                pTestEncConfig->nv21 = FALSE;
                break;
            case OMX_COLOR_FormatYUV420SemiPlanar:
                pTestEncConfig->cbcrInterleave = OMX_TRUE;
                pTestEncConfig->nv21 = OMX_FALSE;
                break;
            default:
                break;
            }
        }
        else if (portIndex == 1)
        {
            width = pInputPort->format.video.nFrameWidth;
            height = pInputPort->format.video.nFrameHeight;
            pOutputPort->format.video.nFrameWidth = width;
            pOutputPort->format.video.nFrameHeight = height;
            pOutputPort->format.video.nStride = width;
            pOutputPort->format.video.nSliceHeight = height;
            switch (eColorFormat)
            {
            case OMX_COLOR_FormatYUV420Planar:
            case OMX_COLOR_FormatYUV420SemiPlanar:
                if (width && height)
                    pOutputPort->nBufferSize = (width * height * 3) / 2;
                break;
            default:
                if (width && height)
                    pOutputPort->nBufferSize = width * height * 2;
                break;
            }
        }
    }
    break;
    case OMX_IndexParamVideoPortFormat:
        {
            OMX_VIDEO_PARAM_PORTFORMATTYPE *portFormat = (OMX_VIDEO_PARAM_PORTFORMATTYPE *)ComponentParameterStructure;
            TestEncConfig *pTestEncConfig = (TestEncConfig *)pSfOMXComponent->testConfig;
            OMX_COLOR_FORMATTYPE eColorFormat = portFormat->eColorFormat;
            LOG(SF_LOG_INFO, "Set eColorFormat to %d\r\n", eColorFormat);
            switch (eColorFormat)
            {
            case OMX_COLOR_FormatYUV420Planar:
                pTestEncConfig->cbcrInterleave = OMX_FALSE;
                pTestEncConfig->nv21 = OMX_FALSE;
                break;
            case OMX_COLOR_FormatYUV420SemiPlanar:
                pTestEncConfig->cbcrInterleave = OMX_TRUE;
                pTestEncConfig->nv21 = OMX_FALSE;
                break;
            default:
                ret = OMX_ErrorBadParameter;
                break;
            }
        }
    break;
    case OMX_IndexParamVideoBitrate:
    {
        OMX_VIDEO_PARAM_BITRATETYPE     *videoRateControl = (OMX_VIDEO_PARAM_BITRATETYPE *)ComponentParameterStructure;
        OMX_U32                          portIndex = videoRateControl->nPortIndex;

        LOG(SF_LOG_INFO, "Set nTargetBitrate = %u on port %d\r\n",videoRateControl->nTargetBitrate, portIndex);

        if ((portIndex != 1)) {
            ret = OMX_ErrorBadPortIndex;
            goto EXIT;
        }
        else
        {
            pOutputPort->format.video.nBitrate = videoRateControl->nTargetBitrate;
        }
        ret = OMX_ErrorNone;
    }
    break;
    case OMX_IndexConfigVideoAVCIntraPeriod:
    {
        OMX_VIDEO_CONFIG_AVCINTRAPERIOD *pAVCIntraPeriod = (OMX_VIDEO_CONFIG_AVCINTRAPERIOD *)ComponentParameterStructure;
        OMX_U32           portIndex = pAVCIntraPeriod->nPortIndex;

        LOG(SF_LOG_INFO, "Set nIDRPeriod = %d nPFrames = %d on port %d\r\n",pAVCIntraPeriod->nIDRPeriod, pAVCIntraPeriod->nPFrames, portIndex);
        if ((portIndex != 1)) {
            ret = OMX_ErrorBadPortIndex;
            goto EXIT;
        }
        else
        {
            pSfOMXComponent->AVCComponent[1].nPFrames = pAVCIntraPeriod->nPFrames;
        }
    }
    break;
    case OMX_IndexParamVideoAvc:
    {
        OMX_VIDEO_PARAM_AVCTYPE *pSrcAVCComponent= (OMX_VIDEO_PARAM_AVCTYPE *)ComponentParameterStructure;
        OMX_U32 nPortIndex = pSrcAVCComponent->nPortIndex;
        OMX_VIDEO_PARAM_AVCTYPE *pDstAVCComponent = &pSfOMXComponent->AVCComponent[nPortIndex];

        LOG(SF_LOG_INFO, "Set nPFrames = %d on port %d\r\n", pSrcAVCComponent->nRefFrames, nPortIndex);
        memcpy(pDstAVCComponent, pSrcAVCComponent, sizeof(OMX_VIDEO_PARAM_AVCTYPE));
    }
    break;
    case OMX_IndexParamVideoHevc:
    {
        OMX_VIDEO_PARAM_HEVCTYPE *pSrcHEVCComponent= (OMX_VIDEO_PARAM_HEVCTYPE *)ComponentParameterStructure;
        OMX_U32 nPortIndex = pSrcHEVCComponent->nPortIndex;
        OMX_VIDEO_PARAM_HEVCTYPE *pDstHEVCComponent = &pSfOMXComponent->HEVCComponent[nPortIndex];

        LOG(SF_LOG_INFO, "Set nKeyFrameInterval = %d on port %d\r\n", pSrcHEVCComponent->nKeyFrameInterval, nPortIndex);
        memcpy(pDstHEVCComponent, pSrcHEVCComponent, sizeof(OMX_VIDEO_PARAM_HEVCTYPE));
    }
    break;
    case OMX_IndexParamVideoQuantization:
    case OMX_IndexParamVideoIntraRefresh:
    default:

        break;
    }

EXIT:
    FunctionOut();

    return ret;
}
static OMX_ERRORTYPE SF_OMX_GetConfig(
    OMX_HANDLETYPE hComponent,
    OMX_INDEXTYPE nIndex,
    OMX_PTR pComponentConfigStructure)
{
    OMX_ERRORTYPE ret = OMX_ErrorNone;

    FunctionIn();

    if (hComponent == NULL)
    {
        ret = OMX_ErrorBadParameter;
        goto EXIT;
    }

    switch (nIndex)
    {
    case OMX_IndexConfigVideoAVCIntraPeriod:

        break;
    case OMX_IndexConfigVideoBitrate:

        break;
    case OMX_IndexConfigVideoFramerate:

        break;

    default:
        break;
    }

EXIT:
    FunctionOut();

    return ret;
}

static OMX_ERRORTYPE SF_OMX_SetConfig(
    OMX_HANDLETYPE hComponent,
    OMX_INDEXTYPE nIndex,
    OMX_PTR pComponentConfigStructure)
{
    OMX_ERRORTYPE ret = OMX_ErrorNone;

    FunctionIn();

    if (hComponent == NULL)
    {
        ret = OMX_ErrorBadParameter;
        goto EXIT;
    }

    switch ((OMX_U32)nIndex)
    {
    case OMX_IndexConfigVideoAVCIntraPeriod:
        break;
    case OMX_IndexConfigVideoBitrate:
        break;
    case OMX_IndexConfigVideoFramerate:
        break;
    case OMX_IndexConfigVideoIntraVOPRefresh:
        break;
    default:

        break;
    }

EXIT:
    FunctionOut();

    return ret;
}

static OMX_ERRORTYPE SF_OMX_GetExtensionIndex(
    OMX_IN OMX_HANDLETYPE hComponent,
    OMX_IN OMX_STRING cParameterName,
    OMX_OUT OMX_INDEXTYPE *pIndexType)
{
    OMX_ERRORTYPE ret = OMX_ErrorNone;

    FunctionIn();

EXIT:
    FunctionOut();
    return ret;
}

static OMX_ERRORTYPE SF_OMX_GetComponentVersion(
    OMX_IN OMX_HANDLETYPE hComponent,
    OMX_OUT OMX_STRING pComponentName,
    OMX_OUT OMX_VERSIONTYPE *pComponentVersion,
    OMX_OUT OMX_VERSIONTYPE *pSpecVersion,
    OMX_OUT OMX_UUIDTYPE *pComponentUUID)
{
    OMX_ERRORTYPE ret = OMX_ErrorNone;

    FunctionIn();

EXIT:
    FunctionOut();
    return ret;
}
static OMX_ERRORTYPE FlushBuffer(SF_OMX_COMPONENT *pSfOMXComponent)
{
    ComponentImpl *pRendererComponent = pSfOMXComponent->hSFComponentRender;
    ComponentImpl *pFeederComponent = pSfOMXComponent->hSFComponentFeeder;
    OMX_U32 inputQueueCount = pSfOMXComponent->functions->Queue_Get_Cnt(pFeederComponent->srcPort.inputQ);
    OMX_U32 OutputQueueCount = pSfOMXComponent->functions->Queue_Get_Cnt(pRendererComponent->sinkPort.inputQ);

    FunctionIn();
    LOG(SF_LOG_PERF, "Flush %d-%d buffers on inputPort-outputPort\r\n", inputQueueCount, OutputQueueCount);
    if (inputQueueCount > 0)
    {
        PortContainerExternal *input = NULL;
        while ((input = (PortContainerExternal*)pSfOMXComponent->functions->ComponentPortGetData(&pFeederComponent->srcPort)) != NULL)
        {
            pSfOMXComponent->functions->ComponentNotifyListeners(pFeederComponent, COMPONENT_EVENT_ENC_EMPTY_BUFFER_DONE, (void *)input);
        }
    }
    if (OutputQueueCount > 0)
    {
        PortContainerExternal *output = NULL;
        while ((output = (PortContainerExternal*)pSfOMXComponent->functions->ComponentPortGetData(&pRendererComponent->sinkPort)) != NULL)
        {
            pSfOMXComponent->functions->ComponentNotifyListeners(pRendererComponent, COMPONENT_EVENT_ENC_FILL_BUFFER_DONE, (void *)output);
        }
    }
    FunctionOut();
}
static OMX_ERRORTYPE InitEncoder(SF_OMX_COMPONENT *pSfOMXComponent)
{
    TestEncConfig *testConfig = NULL;
    CNMComponentConfig *config = NULL;
    Uint32 sizeInWord;
    char *fwPath = NULL;

    if (pSfOMXComponent->hSFComponentExecoder != NULL)
    {
        return OMX_ErrorNone;
    }

    FunctionIn();

    testConfig = (TestEncConfig *)pSfOMXComponent->testConfig;

    testConfig->productId = (ProductId)pSfOMXComponent->functions->VPU_GetProductId(testConfig->coreIdx);
    if (CheckEncTestConfig(testConfig) == FALSE)
    {
        LOG(SF_LOG_ERR, "fail to CheckTestConfig()\n");
        return OMX_ErrorBadParameter;
    }

    switch (testConfig->productId)
    {
    case PRODUCT_ID_521:
        fwPath = pSfOMXComponent->fwPath;
        break;
    case PRODUCT_ID_511:
        fwPath = pSfOMXComponent->fwPath;
        break;
    case PRODUCT_ID_517:
        fwPath = CORE_7_BIT_CODE_FILE_PATH;
        break;
    default:
        LOG(SF_LOG_ERR, "Unknown product id: %d, whether kernel module loaded?\n", testConfig->productId);
        return OMX_ErrorBadParameter;
    }
    LOG(SF_LOG_INFO, "FW PATH = %s\n", fwPath);
    if (pSfOMXComponent->functions->LoadFirmware(testConfig->productId, (Uint8 **)&(pSfOMXComponent->pusBitCode), &sizeInWord, fwPath) < 0)
    {
        LOG(SF_LOG_ERR, "Failed to load firmware: %s\n", fwPath);
        return OMX_ErrorInsufficientResources;
    }

    config = pSfOMXComponent->config;
    memcpy(&(config->testEncConfig), testConfig, sizeof(TestEncConfig));
    config->bitcode = (Uint8 *)pSfOMXComponent->pusBitCode;
    config->sizeOfBitcode = sizeInWord;
    LOG(SF_LOG_INFO, "cbcrInterleave= %d, nv21 = %d\r\n", config->testEncConfig.cbcrInterleave, config->testEncConfig.nv21);
    memcpy(config->testEncConfig.cfgFileName, WAVE521_CONFIG_FILE, sizeof(WAVE521_CONFIG_FILE));
    LOG(SF_LOG_INFO, "Get width = %d, height = %d \r\n", config->encOpenParam.picWidth, config->encOpenParam.picHeight);
    if (pSfOMXComponent->functions->SetupEncoderOpenParam(&config->encOpenParam, &config->testEncConfig, NULL) == OMX_FALSE)
    {
        LOG(SF_LOG_ERR, "SetupEncoderOpenParam error\n");
        return OMX_ErrorBadParameter;
    }
    CNMComponentConfig *pCNMComponentConfig = (CNMComponentConfig*)pSfOMXComponent->config;
    config->encOpenParam.picWidth = pSfOMXComponent->portDefinition[0].format.video.nFrameWidth;
    config->encOpenParam.picHeight = pSfOMXComponent->portDefinition[0].format.video.nFrameHeight;
    config->encOpenParam.frameRateInfo = pSfOMXComponent->portDefinition[0].format.video.xFramerate;
    config->encOpenParam.bitRate = pSfOMXComponent->portDefinition[1].format.video.nBitrate;
    if (pSfOMXComponent->bitFormat == STD_AVC)
    {
        config->encOpenParam.EncStdParam.waveParam.intraPeriod = pSfOMXComponent->AVCComponent[1].nPFrames;
    }
    else if (pSfOMXComponent->bitFormat == STD_HEVC)
    {
        config->encOpenParam.EncStdParam.waveParam.intraPeriod = pSfOMXComponent->HEVCComponent[1].nKeyFrameInterval;
    }
    LOG(SF_LOG_INFO, "Get width = %d, height = %d frameRateInfo = %d intraPeriod = %d \r\n",
        config->encOpenParam.picWidth, config->encOpenParam.picHeight, config->encOpenParam.frameRateInfo,
        config->encOpenParam.EncStdParam.waveParam.intraPeriod);

    pSfOMXComponent->hSFComponentExecoder = pSfOMXComponent->functions->ComponentCreate("wave_encoder", config);
    pSfOMXComponent->hSFComponentFeeder = pSfOMXComponent->functions->ComponentCreate("yuvfeeder", config);
    pSfOMXComponent->hSFComponentRender = pSfOMXComponent->functions->ComponentCreate("reader", config);

    ComponentImpl *pFeederComponent = (ComponentImpl *)pSfOMXComponent->hSFComponentFeeder;
    pSfOMXComponent->functions->ComponentPortCreate(&pFeederComponent->srcPort, pSfOMXComponent->hSFComponentFeeder, 10, sizeof(PortContainerExternal));

    ComponentImpl *pRenderComponent = (ComponentImpl *)pSfOMXComponent->hSFComponentRender;
    pSfOMXComponent->functions->ComponentPortDestroy(&pRenderComponent->sinkPort);
    pSfOMXComponent->functions->ComponentPortCreate(&pRenderComponent->sinkPort, pSfOMXComponent->hSFComponentRender, 10, sizeof(PortContainerExternal));

    if (pSfOMXComponent->functions->SetupEncListenerContext(pSfOMXComponent->lsnCtx, config) == TRUE)
    {
        pSfOMXComponent->functions->ComponentRegisterListener(pSfOMXComponent->hSFComponentExecoder,
                                                              COMPONENT_EVENT_ENC_ALL, pSfOMXComponent->functions->EncoderListener, (void *)pSfOMXComponent->lsnCtx);
        pSfOMXComponent->functions->ComponentRegisterListener(pSfOMXComponent->hSFComponentExecoder,
                                                              COMPONENT_EVENT_ENC_REGISTER_FB, OnEventArrived, (void *)pSfOMXComponent->lsnCtx);
        pSfOMXComponent->functions->ComponentRegisterListener(pSfOMXComponent->hSFComponentExecoder,
                                                              COMPONENT_EVENT_ENC_ENCODED_ALL, OnEventArrived, (void *)pSfOMXComponent->lsnCtx);
        pSfOMXComponent->functions->ComponentRegisterListener(pSfOMXComponent->hSFComponentFeeder,
                                                              COMPONENT_EVENT_ENC_ALL, OnEventArrived, (void *)pSfOMXComponent->lsnCtx);
        pSfOMXComponent->functions->ComponentRegisterListener(pSfOMXComponent->hSFComponentRender,
                                                              COMPONENT_EVENT_ENC_ALL, OnEventArrived, (void *)pSfOMXComponent->lsnCtx);
    }
    else
    {
        LOG(SF_LOG_ERR, "ComponentRegisterListener fail\r\n");
        return OMX_ErrorBadParameter;
    }
    pSfOMXComponent->functions->ComponentSetupTunnel(pSfOMXComponent->hSFComponentFeeder, pSfOMXComponent->hSFComponentExecoder);
    pSfOMXComponent->functions->ComponentSetupTunnel(pSfOMXComponent->hSFComponentExecoder, pSfOMXComponent->hSFComponentRender);
    FunctionOut();
    return OMX_ErrorNone;
}

static OMX_ERRORTYPE SF_OMX_SendCommand(
    OMX_IN OMX_HANDLETYPE hComponent,
    OMX_IN OMX_COMMANDTYPE Cmd,
    OMX_IN OMX_U32 nParam,
    OMX_IN OMX_PTR pCmdData)
{
    OMX_ERRORTYPE ret = OMX_ErrorNone;
    OMX_COMPONENTTYPE *pOMXComponent = (OMX_COMPONENTTYPE *)hComponent;
    SF_OMX_COMPONENT *pSfOMXComponent = pOMXComponent->pComponentPrivate;
    ComponentImpl *pSFComponentEncoder = NULL;
    ComponentImpl *pSFComponentFeeder = NULL;
    ComponentImpl *pSFComponentRender = NULL;
    ComponentState currentState;

    FunctionIn();
    if (hComponent == NULL || pSfOMXComponent == NULL)
    {
        ret = OMX_ErrorBadParameter;
        goto EXIT;
    }


    LOG(SF_LOG_INFO, "cmd = %X, nParam = %X\r\n", Cmd, nParam);
    switch (Cmd)
    {
    case OMX_CommandStateSet:
        pSfOMXComponent->nextState = nParam;
        LOG(SF_LOG_INFO, "OMX dest state = %X\r\n", nParam);
        switch (nParam)
        {
        case OMX_StateLoaded:
            if (pSfOMXComponent->hSFComponentExecoder == NULL)
            {
                break;
            }
            pSFComponentEncoder = (ComponentImpl *)pSfOMXComponent->hSFComponentExecoder;
            pSFComponentFeeder = (ComponentImpl *)pSfOMXComponent->hSFComponentFeeder;
            pSFComponentRender = (ComponentImpl *)pSfOMXComponent->hSFComponentRender;
            currentState = pSfOMXComponent->functions->ComponentGetState(pSfOMXComponent->hSFComponentExecoder);
            LOG(SF_LOG_INFO, "VPU Current state = %X\r\n", currentState);
            switch(currentState)
            {
                case COMPONENT_STATE_CREATED:
                case COMPONENT_STATE_PREPARED:
                case COMPONENT_STATE_EXECUTED:
                case COMPONENT_STATE_TERMINATED:
                if (pSfOMXComponent->functions->Queue_Get_Cnt(pSFComponentRender->sinkPort.inputQ) > 0 ||
                    pSfOMXComponent->functions->Queue_Get_Cnt(pSFComponentFeeder->srcPort.inputQ) > 0)
                {
                    LOG(SF_LOG_ERR, "Buffer not flush!\r\n")
                    // ret = OMX_ErrorIncorrectStateTransition;
                    break;
                }
                pSFComponentEncoder->terminate = OMX_TRUE;
                pSFComponentFeeder->terminate = OMX_TRUE;
                pSFComponentRender->terminate = OMX_TRUE;
                case COMPONENT_STATE_NONE:
                default:
                    ret = OMX_ErrorIncorrectStateTransition;
                    break;
            }
            break;
        case OMX_StateIdle:
            if (pSfOMXComponent->hSFComponentExecoder == NULL)
            {
                ret = InitEncoder(pSfOMXComponent);
                if (ret != OMX_ErrorNone)
                {
                    goto EXIT;
                }
            }

            pSFComponentEncoder = (ComponentImpl *)pSfOMXComponent->hSFComponentExecoder;
            pSFComponentFeeder = (ComponentImpl *)pSfOMXComponent->hSFComponentFeeder;
            pSFComponentRender = (ComponentImpl *)pSfOMXComponent->hSFComponentRender;
            currentState = pSfOMXComponent->functions->ComponentGetState(pSfOMXComponent->hSFComponentExecoder);
            LOG(SF_LOG_INFO, "VPU Current state = %X\r\n", currentState);
            switch(currentState)
            {
            case COMPONENT_STATE_NONE:
                ret = OMX_ErrorIncorrectStateTransition;
                break;
            case COMPONENT_STATE_CREATED:
                break;
            case COMPONENT_STATE_PREPARED:
            case COMPONENT_STATE_EXECUTED:
            case COMPONENT_STATE_TERMINATED:
                {
                    PortContainerExternal *pPortContainerExternal = NULL;
                    OMX_BUFFERHEADERTYPE *pOMXBuffer;
                    pSFComponentEncoder->pause = OMX_TRUE;
                    pSFComponentFeeder->pause = OMX_TRUE;
                    pSFComponentRender->pause = OMX_TRUE;
                    FlushBuffer(pSfOMXComponent);
                }
                break;
            }
            break;

        case OMX_StateExecuting:
            pSFComponentEncoder = (ComponentImpl *)pSfOMXComponent->hSFComponentExecoder;
            pSFComponentFeeder = (ComponentImpl *)pSfOMXComponent->hSFComponentFeeder;
            pSFComponentRender = (ComponentImpl *)pSfOMXComponent->hSFComponentRender;


            currentState = pSfOMXComponent->functions->ComponentGetState(pSfOMXComponent->hSFComponentExecoder);
            LOG(SF_LOG_INFO, "VPU Current state = %X\r\n", currentState);
            switch(currentState)
            {
            case COMPONENT_STATE_NONE:
            case COMPONENT_STATE_TERMINATED:
                ret = OMX_ErrorIncorrectStateTransition;
                break;
            case COMPONENT_STATE_CREATED:
                if (pSFComponentEncoder->thread == NULL)
                {
                    LOG(SF_LOG_INFO, "execute component %s\r\n", pSFComponentEncoder->name);
                    currentState = pSfOMXComponent->functions->ComponentExecute(pSFComponentEncoder);
                    LOG(SF_LOG_INFO, "ret = %d\r\n", currentState);
                }
                if (pSFComponentFeeder->thread == NULL)
                {
                    LOG(SF_LOG_INFO, "execute component %s\r\n", pSFComponentFeeder->name);
                    currentState = pSfOMXComponent->functions->ComponentExecute(pSFComponentFeeder);
                    LOG(SF_LOG_INFO, "ret = %d\r\n", currentState);
                }
                if (pSFComponentRender->thread == NULL)
                {
                    LOG(SF_LOG_INFO, "execute component %s\r\n", pSFComponentRender->name);
                    currentState = pSfOMXComponent->functions->ComponentExecute(pSFComponentRender);
                    LOG(SF_LOG_INFO, "ret = %d\r\n", currentState);
                }
                break;
            case COMPONENT_STATE_PREPARED:
            case COMPONENT_STATE_EXECUTED:
                pSFComponentEncoder->pause = OMX_FALSE;
                pSFComponentFeeder->pause = OMX_FALSE;
                pSFComponentRender->pause = OMX_FALSE;
                break;
            }
            break;
        default:
            ret = OMX_ErrorIncorrectStateTransition;
            break;
        }
        pSfOMXComponent->callbacks->EventHandler(pSfOMXComponent->pOMXComponent, pSfOMXComponent->pAppData,
                                                 OMX_EventCmdComplete, OMX_CommandStateSet, nParam, NULL);
        break;
    case OMX_CommandFlush:
        pSfOMXComponent->callbacks->EventHandler(pSfOMXComponent->pOMXComponent, pSfOMXComponent->pAppData,
                                                 OMX_EventCmdComplete, OMX_CommandFlush, nParam, NULL);
        break;
    case OMX_CommandPortDisable:
        pSfOMXComponent->callbacks->EventHandler(pSfOMXComponent->pOMXComponent, pSfOMXComponent->pAppData,
                                                 OMX_EventCmdComplete, OMX_CommandPortDisable, nParam, NULL);
        break;
    case OMX_CommandPortEnable:
        pSfOMXComponent->callbacks->EventHandler(pSfOMXComponent->pOMXComponent, pSfOMXComponent->pAppData,
                                                 OMX_EventCmdComplete, OMX_CommandPortEnable, nParam, NULL);
        break;
    case OMX_CommandMarkBuffer:

        break;
    default:
        break;
    }

EXIT:
    FunctionOut();
    return ret;
}

static OMX_ERRORTYPE SF_OMX_GetState(
    OMX_IN OMX_HANDLETYPE hComponent,
    OMX_OUT OMX_STATETYPE *pState)
{
    OMX_ERRORTYPE ret = OMX_ErrorNone;
    FunctionIn();
    GetStateCommon(hComponent, pState);

EXIT:
    FunctionOut();
    return ret;
}

static OMX_ERRORTYPE SF_OMX_FreeBuffer(
    OMX_IN OMX_HANDLETYPE hComponent,
    OMX_IN OMX_U32 nPortIndex,
    OMX_IN OMX_BUFFERHEADERTYPE *pBufferHdr)
{
    OMX_ERRORTYPE ret = OMX_ErrorNone;
    OMX_COMPONENTTYPE *pOMXComponent = (OMX_COMPONENTTYPE *)hComponent;
    SF_OMX_COMPONENT *pSfOMXComponent = pOMXComponent->pComponentPrivate;
    OMX_U32 i = 0;
    FunctionIn();
    ClearOMXBuffer(pSfOMXComponent, pBufferHdr);
    LOG(SF_LOG_PERF, "buffer count = %d\r\n", GetOMXBufferCount(pSfOMXComponent));
    free(pBufferHdr);

EXIT:
    FunctionOut();
    return ret;
}

static OMX_ERRORTYPE SF_OMX_UseEGLImage(
    OMX_IN OMX_HANDLETYPE hComponent,
    OMX_INOUT OMX_BUFFERHEADERTYPE **ppBufferHdr,
    OMX_IN OMX_U32 nPortIndex,
    OMX_IN OMX_PTR pAppPrivate,
    OMX_IN void *eglImage)
{
    (void)hComponent;
    (void)ppBufferHdr;
    (void)nPortIndex;
    (void)pAppPrivate;
    (void)eglImage;
    return OMX_ErrorNotImplemented;
}

static OMX_ERRORTYPE SF_OMX_ComponentConstructor(SF_OMX_COMPONENT *hComponent)
{
    OMX_ERRORTYPE ret = OMX_ErrorNone;
    FunctionIn();

    ret = InitComponentStructorCommon(hComponent);
    if (ret != OMX_ErrorNone)
    {
        goto EXIT;
    }
    TestEncConfig *pTestEncConfig = (TestEncConfig *)hComponent->testConfig;
    pTestEncConfig->stdMode = hComponent->bitFormat;
    pTestEncConfig->frame_endian = VPU_FRAME_ENDIAN;
    pTestEncConfig->stream_endian = VPU_STREAM_ENDIAN;
    pTestEncConfig->source_endian = VPU_SOURCE_ENDIAN;
    pTestEncConfig->mapType = COMPRESSED_FRAME_MAP;
    pTestEncConfig->lineBufIntEn = TRUE;
    pTestEncConfig->ringBufferEnable = FALSE;
    pTestEncConfig->ringBufferWrapEnable = FALSE;
    pTestEncConfig->yuv_mode = SOURCE_YUV_WITH_BUFFER;
    /*
     if cbcrInterleave is FALSE and nv21 is FALSE, the default enc format is I420
     if cbcrInterleave is TRUE and nv21 is FALSE, then the enc format is NV12
     if cbcrInterleave is TRUE and nv21 is TRUE, then the enc format is NV21
    */
    pTestEncConfig->cbcrInterleave = TRUE;
    pTestEncConfig->nv21 = FALSE;

    hComponent->pOMXComponent->UseBuffer = &SF_OMX_UseBuffer;
    hComponent->pOMXComponent->AllocateBuffer = &SF_OMX_AllocateBuffer;
    hComponent->pOMXComponent->EmptyThisBuffer = &SF_OMX_EmptyThisBuffer;
    hComponent->pOMXComponent->FillThisBuffer = &SF_OMX_FillThisBuffer;
    hComponent->pOMXComponent->FreeBuffer = &SF_OMX_FreeBuffer;
    // hComponent->pOMXComponent->ComponentTunnelRequest = &SF_OMX_ComponentTunnelRequest;
    hComponent->pOMXComponent->GetParameter = &SF_OMX_GetParameter;
    hComponent->pOMXComponent->SetParameter = &SF_OMX_SetParameter;
    hComponent->pOMXComponent->GetConfig = &SF_OMX_GetConfig;
    hComponent->pOMXComponent->SetConfig = &SF_OMX_SetConfig;
    hComponent->pOMXComponent->SendCommand = &SF_OMX_SendCommand;
    hComponent->pOMXComponent->GetState = &SF_OMX_GetState;
    // hComponent->pOMXComponent->GetExtensionIndex = &SF_OMX_GetExtensionIndex;
    // hComponent->pOMXComponent->ComponentRoleEnum = &SF_OMX_ComponentRoleEnum;
    // hComponent->pOMXComponent->ComponentDeInit = &SF_OMX_ComponentDeInit;
    hComponent->inputSemaphore = (sem_t *)malloc(sizeof(sem_t));
    if (hComponent->inputSemaphore == NULL)
    {
        LOG(SF_LOG_ERR, "malloc fail\r\n");
        ret = OMX_ErrorInsufficientResources;
        goto EXIT;
    }
    sem_init(hComponent->inputSemaphore, 0, 1);
EXIT:
    FunctionOut();

    return ret;
}

static OMX_ERRORTYPE SF_OMX_ComponentClear(SF_OMX_COMPONENT *hComponent)
{
    OMX_ERRORTYPE ret = OMX_ErrorNone;
    ComponentImpl *pSFComponentEncoder = (ComponentImpl *)hComponent->hSFComponentExecoder;
    ComponentImpl *pSFComponentFeeder = (ComponentImpl *)hComponent->hSFComponentFeeder;
    ComponentImpl *pSFComponentRender = (ComponentImpl *)hComponent->hSFComponentRender;

    FunctionIn();
    if (pSFComponentEncoder == NULL || pSFComponentFeeder == NULL || pSFComponentRender == NULL)
    {
        goto EXIT;
    }
    pSFComponentEncoder->terminate = OMX_TRUE;
    pSFComponentFeeder->terminate = OMX_TRUE;
    pSFComponentRender->terminate = OMX_TRUE;
    hComponent->functions->ComponentWait(hComponent->hSFComponentFeeder);
    hComponent->functions->ComponentWait(hComponent->hSFComponentExecoder);
    hComponent->functions->ComponentWait(hComponent->hSFComponentRender);

    free(hComponent->pusBitCode);
    hComponent->functions->ClearEncListenerContext(hComponent->lsnCtx);
    ComponentClearCommon(hComponent);
EXIT:
    FunctionOut();

    return ret;
}

SF_OMX_COMPONENT sf_enc_encoder_h265 = {
    .componentName = "sf.enc.encoder.h265",
    .libName = "libsfenc.so",
    .pOMXComponent = NULL,
    .SF_OMX_ComponentConstructor = SF_OMX_ComponentConstructor,
    .SF_OMX_ComponentClear = SF_OMX_ComponentClear,
    .functions = NULL,
    .bitFormat = STD_HEVC,
    .fwPath = "/lib/firmware/chagall.bin",
    .componentRule = "video_encoder.hevc"};

SF_OMX_COMPONENT sf_enc_encoder_h264 = {
    .componentName = "sf.enc.encoder.h264",
    .libName = "libsfenc.so",
    .pOMXComponent = NULL,
    .SF_OMX_ComponentConstructor = SF_OMX_ComponentConstructor,
    .SF_OMX_ComponentClear = SF_OMX_ComponentClear,
    .functions = NULL,
    .bitFormat = STD_AVC,
    .fwPath = "/lib/firmware/chagall.bin",
    .componentRule = "video_encoder.avc"};