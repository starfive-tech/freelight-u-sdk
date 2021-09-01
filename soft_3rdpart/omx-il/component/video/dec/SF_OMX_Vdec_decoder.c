// SPDX-License-Identifier: GPL-2.0
/*
 * Copyright (C) 2021 StarFive Technology Co., Ltd.
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "SF_OMX_video_common.h"
#include "SF_OMX_Core.h"

extern OMX_TICKS gInitTimeStamp;
static void OnEventArrived(Component com, unsigned long event, void *data, void *context)
{
    SF_OMX_COMPONENT *pSfOMXComponent = GetSFOMXComponrntByComponent(com);
    PortContainerExternal *pPortContainerExternal = (PortContainerExternal *)data;
    OMX_BUFFERHEADERTYPE *pOMXBuffer;
    static OMX_U32 dec_cnt = 0;
    static struct timeval tv_old = {0};
    OMX_U32 fps = 0;
    OMX_U64 diff_time = 0; // ms
    FunctionIn();

    LOG(SF_LOG_INFO, "event=%X\r\n", event);
    switch (event)
    {
    case COMPONENT_EVENT_DEC_EMPTY_BUFFER_DONE:
        pOMXBuffer = GetOMXBufferByAddr(pSfOMXComponent, (OMX_U8 *)pPortContainerExternal->pBuffer);
        pSfOMXComponent->callbacks->EmptyBufferDone(pSfOMXComponent->pOMXComponent, pSfOMXComponent->pAppData, pOMXBuffer);
        sem_post(pSfOMXComponent->inputSemaphore);
        break;
    case COMPONENT_EVENT_DEC_FILL_BUFFER_DONE:
    {
        struct timeval tv;
        pOMXBuffer = GetOMXBufferByAddr(pSfOMXComponent, (OMX_U8 *)pPortContainerExternal->pBuffer);
        gettimeofday(&tv, NULL);
        pOMXBuffer->nFilledLen = pPortContainerExternal->nFilledLen;
        pOMXBuffer->nTimeStamp = tv.tv_sec * 1000000 + tv.tv_usec - gInitTimeStamp;
#if 0
        {
            FILE *fb = fopen("./out.bcp", "ab+");
            LOG(SF_LOG_INFO, "%d %d\r\n", pOMXBuffer->nFilledLen, pOMXBuffer->nOffset);
            fwrite(pOMXBuffer->pBuffer, 1, pOMXBuffer->nFilledLen, fb);
            fclose(fb);
        }
#endif

        // Following is to print the decoding fps
        if (dec_cnt == 0) {
            tv_old = tv;
        }
        if (dec_cnt++ >= 50) {
            diff_time = (tv.tv_sec - tv_old.tv_sec) * 1000 + (tv.tv_usec - tv_old.tv_usec) / 1000;
            fps = 1000  * (dec_cnt - 1) / diff_time;
            dec_cnt = 0;
            LOG(SF_LOG_WARN, "Decoding fps: %d \r\n", fps);
        }

        LOG(SF_LOG_PERF, "OMX finish one buffer\r\n");
        pSfOMXComponent->callbacks->FillBufferDone(pSfOMXComponent->pOMXComponent, pSfOMXComponent->pAppData, pOMXBuffer);
    }
    break;
    case COMPONENT_EVENT_DEC_REGISTER_FB:
    {
        struct timeval tv;
        gettimeofday(&tv, NULL);
        gInitTimeStamp = tv.tv_sec * 1000000 + tv.tv_usec;
        pSfOMXComponent->callbacks->EventHandler(pSfOMXComponent->pOMXComponent, pSfOMXComponent->pAppData, OMX_EventPortSettingsChanged,
                                                 1, OMX_IndexParamPortDefinition, NULL);
    }
    break;
    case COMPONENT_EVENT_DEC_DECODED_ALL:
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

    ComponentImpl *pFeederComponent = (ComponentImpl *)pSfOMXComponent->hSFComponentFeeder;

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

    if (pSfOMXComponent->functions->Queue_Enqueue(pFeederComponent->srcPort.inputQ, (void *)pPortContainerExternal) != OMX_TRUE)
    {
        LOG(SF_LOG_ERR, "%p:%p FAIL\r\n", pFeederComponent->srcPort.inputQ, pPortContainerExternal);
        free(pPortContainerExternal);
        return OMX_ErrorInsufficientResources;
    }
    LOG(SF_LOG_PERF, "input queue count=%d\r\n", pSfOMXComponent->functions->Queue_Get_Cnt(pFeederComponent->srcPort.inputQ));
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
    ComponentImpl *pRendererComponent = (ComponentImpl *)pSfOMXComponent->hSFComponentRender;
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
    LOG(SF_LOG_PERF, "output queue count=%d\r\n", pSfOMXComponent->functions->Queue_Get_Cnt(pRendererComponent->sinkPort.inputQ));
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

    *ppBuffer = temp_bufferHeader;

    ret = StoreOMXBuffer(pSfOMXComponent, temp_bufferHeader);
    LOG(SF_LOG_PERF, "buffer count = %d\r\n", GetOMXBufferCount(pSfOMXComponent));

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

    FunctionIn();

    if (hComponent == NULL)
    {
        ret = OMX_ErrorBadParameter;
        goto EXIT;
    }
    switch ((OMX_U32)nParamIndex)
    {
    case OMX_IndexParamVideoInit:

        break;
    case OMX_IndexParamVideoPortFormat:
    {
        OMX_VIDEO_PARAM_PORTFORMATTYPE *portFormat = (OMX_VIDEO_PARAM_PORTFORMATTYPE *)ComponentParameterStructure;
        OMX_U32 index = portFormat->nIndex;
        switch (index)
        {
        case 0:
            portFormat->eCompressionFormat = OMX_VIDEO_CodingUnused;
            portFormat->eColorFormat = OMX_COLOR_FormatYUV420SemiPlanar;
            portFormat->xFramerate = 30;
        default:
            if (index > 0)
            {
                ret = OMX_ErrorNoMore;
            }
        }
    }

    break;
    case OMX_IndexParamVideoBitrate:
        break;
    case OMX_IndexParamVideoQuantization:

        break;
    case OMX_IndexParamPortDefinition:
    {
        OMX_PARAM_PORTDEFINITIONTYPE *pPortDefinition = (OMX_PARAM_PORTDEFINITIONTYPE *)ComponentParameterStructure;
        OMX_U32 portIndex = pPortDefinition->nPortIndex;
        memcpy(pPortDefinition, &pSfOMXComponent->portDefinition[portIndex], pPortDefinition->nSize);
    }

    break;
    case OMX_IndexParamVideoIntraRefresh:

        break;

    case OMX_IndexParamStandardComponentRole:

        break;
    case OMX_IndexParamVideoAvc:
        break;
    case OMX_IndexParamVideoHevc:
        break;
    case OMX_IndexParamVideoProfileLevelQuerySupported:

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

    if (pSfOMXComponent == NULL)
    {
        ret = OMX_ErrorBadParameter;
        goto EXIT;
    }
    TestDecConfig *testConfig = pSfOMXComponent->testConfig;

    switch ((OMX_U32)nIndex)
    {
    case OMX_IndexParamVideoPortFormat:
        break;
    case OMX_IndexParamVideoBitrate:
        break;
    case OMX_IndexParamVideoQuantization:

        break;
    case OMX_IndexParamPortDefinition:
    {
        OMX_PARAM_PORTDEFINITIONTYPE *pPortDefinition = (OMX_PARAM_PORTDEFINITIONTYPE *)ComponentParameterStructure;
        OMX_PARAM_PORTDEFINITIONTYPE *pInputPort = &pSfOMXComponent->portDefinition[0];
        OMX_PARAM_PORTDEFINITIONTYPE *pOutputPort = &pSfOMXComponent->portDefinition[1];
        TestDecConfig *pTestDecConfig = (TestDecConfig *)pSfOMXComponent->testConfig;
        OMX_U32 portIndex = pPortDefinition->nPortIndex;
        OMX_U32 width = pPortDefinition->format.video.nFrameWidth;
        OMX_U32 height = pPortDefinition->format.video.nFrameHeight;
        LOG(SF_LOG_INFO, "Set width = %d, height = %d on port %d\r\n", width, height, pPortDefinition->nPortIndex);

        memcpy(&pSfOMXComponent->portDefinition[portIndex], pPortDefinition, pPortDefinition->nSize);

        if (portIndex == 0)
        {
            pInputPort->format.video.nStride = width;
            pInputPort->format.video.nSliceHeight = height;
            pInputPort->nBufferSize = width * height * 2;
        } else if (portIndex == 1)
        {
            if (width == 1)
            {
                width = pInputPort->format.video.nFrameWidth;
            }
            if (height == 1)
            {
                height = pInputPort->format.video.nFrameHeight;
            }
            int scalew = pInputPort->format.video.nFrameWidth / width;
            int scaleh = pInputPort->format.video.nFrameHeight / height;
            if (scalew > 8 || scaleh > 8 || scalew < 1 || scaleh < 1)
            {
                LOG(SF_LOG_ERR, "Scaling should be 1 to 1/8 (down-scaling only)!\r\n");
                ret = OMX_ErrorBadParameter;
                goto EXIT;
            }
            pOutputPort->format.video.nFrameWidth = width;
            pOutputPort->format.video.nFrameHeight = height;
            pOutputPort->format.video.nStride = width;
            pOutputPort->format.video.nSliceHeight = height;
            pTestDecConfig->scaleDownWidth = VPU_CEIL(width, 2);
            pTestDecConfig->scaleDownHeight = VPU_CEIL(height, 2);
            LOG(SF_LOG_INFO, "Set scale = %d, %d\r\n", pTestDecConfig->scaleDownWidth , pTestDecConfig->scaleDownHeight);
            switch (pOutputPort->format.video.eColorFormat)
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
    case OMX_IndexParamVideoIntraRefresh:

        break;

    case OMX_IndexParamStandardComponentRole:

        break;
    case OMX_IndexParamVideoAvc:
        testConfig->bitFormat = 0;
        break;
    case OMX_IndexParamVideoHevc:
        testConfig->bitFormat = 12;
        break;
    case OMX_IndexConfigCommonInputCrop:
        memcpy(testConfig->inputPath, ComponentParameterStructure, strlen(ComponentParameterStructure));
        break;
    case OMX_IndexConfigCommonOutputCrop:
        memcpy(testConfig->outputPath, ComponentParameterStructure, strlen(ComponentParameterStructure));
        break;
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

    FunctionOut();
    return ret;
}

static OMX_ERRORTYPE InitDecoder(SF_OMX_COMPONENT *pSfOMXComponent)
{
    TestDecConfig *testConfig = NULL;
    CNMComponentConfig *config = NULL;
    Uint32 sizeInWord;
    char *fwPath = NULL;

    if (pSfOMXComponent->hSFComponentExecoder != NULL)
    {
        return OMX_ErrorNone;
    }

    FunctionIn();

    testConfig = (TestDecConfig *)pSfOMXComponent->testConfig;

    testConfig->productId = (ProductId)pSfOMXComponent->functions->VPU_GetProductId(testConfig->coreIdx);
    if (CheckDecTestConfig(testConfig) == FALSE)
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
    memcpy(&(config->testDecConfig), testConfig, sizeof(TestDecConfig));
    config->bitcode = (Uint8 *)pSfOMXComponent->pusBitCode;
    config->sizeOfBitcode = sizeInWord;

    if (pSfOMXComponent->functions->SetUpDecoderOpenParam(&config->decOpenParam, &config->testDecConfig) != RETCODE_SUCCESS)
    {
        LOG(SF_LOG_ERR, "SetupDecoderOpenParam error\n");
        return OMX_ErrorBadParameter;
    }
    pSfOMXComponent->hSFComponentExecoder = pSfOMXComponent->functions->ComponentCreate("wave_decoder", config);
    pSfOMXComponent->hSFComponentFeeder = pSfOMXComponent->functions->ComponentCreate("feeder", config);
    pSfOMXComponent->hSFComponentRender = pSfOMXComponent->functions->ComponentCreate("renderer", config);

    ComponentImpl *pFeederComponent = (ComponentImpl *)pSfOMXComponent->hSFComponentFeeder;
    pSfOMXComponent->functions->ComponentPortCreate(&pFeederComponent->srcPort, pSfOMXComponent->hSFComponentFeeder, 5, sizeof(PortContainerExternal));

    ComponentImpl *pRenderComponent = (ComponentImpl *)pSfOMXComponent->hSFComponentRender;
    pSfOMXComponent->functions->ComponentPortDestroy(&pRenderComponent->sinkPort);
    pSfOMXComponent->functions->ComponentPortCreate(&pRenderComponent->sinkPort, pSfOMXComponent->hSFComponentRender, 5, sizeof(PortContainerExternal));

    if (pSfOMXComponent->functions->SetupDecListenerContext(pSfOMXComponent->lsnCtx, config, NULL) == TRUE)
    {
        pSfOMXComponent->functions->ComponentRegisterListener(pSfOMXComponent->hSFComponentExecoder,
                                                              COMPONENT_EVENT_DEC_ALL, pSfOMXComponent->functions->DecoderListener, (void *)pSfOMXComponent->lsnCtx);
        pSfOMXComponent->functions->ComponentRegisterListener(pSfOMXComponent->hSFComponentExecoder,
                                                              COMPONENT_EVENT_DEC_REGISTER_FB, OnEventArrived, (void *)pSfOMXComponent->lsnCtx);
        pSfOMXComponent->functions->ComponentRegisterListener(pSfOMXComponent->hSFComponentExecoder,
                                                              COMPONENT_EVENT_DEC_DECODED_ALL, OnEventArrived, (void *)pSfOMXComponent->lsnCtx);
        pSfOMXComponent->functions->ComponentRegisterListener(pSfOMXComponent->hSFComponentFeeder,
                                                              COMPONENT_EVENT_DEC_ALL, OnEventArrived, (void *)pSfOMXComponent->lsnCtx);
        pSfOMXComponent->functions->ComponentRegisterListener(pSfOMXComponent->hSFComponentRender,
                                                              COMPONENT_EVENT_DEC_ALL, OnEventArrived, (void *)pSfOMXComponent->lsnCtx);
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
    ComponentImpl *pSFComponentDecoder = NULL;
    ComponentImpl *pSFComponentFeeder = NULL;
    ComponentImpl *pSFComponentRender = NULL;
    ComponentState componentState;

    FunctionIn();
    if (hComponent == NULL || pSfOMXComponent == NULL)
    {
        ret = OMX_ErrorBadParameter;
        goto EXIT;
    }

    switch (Cmd)
    {
    case OMX_CommandStateSet:
        switch (nParam)
        {
        case OMX_StateIdle:
            ret = InitDecoder(pSfOMXComponent);
            if (ret != OMX_ErrorNone)
            {
                goto EXIT;
            }
            pSFComponentDecoder = (ComponentImpl *)pSfOMXComponent->hSFComponentExecoder;
            pSFComponentFeeder = (ComponentImpl *)pSfOMXComponent->hSFComponentFeeder;
            pSFComponentRender = (ComponentImpl *)pSfOMXComponent->hSFComponentRender;

            pSFComponentDecoder->pause = OMX_TRUE;
            pSFComponentFeeder->pause = OMX_TRUE;
            pSFComponentRender->pause = OMX_TRUE;
            pSfOMXComponent->callbacks->EventHandler(pSfOMXComponent->pOMXComponent, pSfOMXComponent->pAppData,
                                                     OMX_EventCmdComplete, OMX_CommandStateSet, nParam, NULL);
            break;

        case OMX_StateExecuting:
            ret = InitDecoder(pSfOMXComponent);
            if (ret != OMX_ErrorNone)
            {
                goto EXIT;
            }
            pSFComponentDecoder = (ComponentImpl *)pSfOMXComponent->hSFComponentExecoder;
            pSFComponentFeeder = (ComponentImpl *)pSfOMXComponent->hSFComponentFeeder;
            pSFComponentRender = (ComponentImpl *)pSfOMXComponent->hSFComponentRender;

            pSFComponentDecoder->pause = OMX_FALSE;
            if (pSFComponentDecoder->thread == NULL)
            {
                LOG(SF_LOG_INFO, "execute component %s\r\n", pSFComponentDecoder->name);
                componentState = pSfOMXComponent->functions->ComponentExecute(pSFComponentDecoder);
                LOG(SF_LOG_INFO, "ret = %d\r\n", componentState);
            }
            if (pSFComponentFeeder->thread == NULL)
            {
                LOG(SF_LOG_INFO, "execute component %s\r\n", pSFComponentFeeder->name);
                componentState = pSfOMXComponent->functions->ComponentExecute(pSFComponentFeeder);
                LOG(SF_LOG_INFO, "ret = %d\r\n", componentState);
            }
            if (pSFComponentRender->thread == NULL)
            {
                LOG(SF_LOG_INFO, "execute component %s\r\n", pSFComponentRender->name);
                componentState = pSfOMXComponent->functions->ComponentExecute(pSFComponentRender);
                LOG(SF_LOG_INFO, "ret = %d\r\n", componentState);
            }
            break;
        default:
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

    FunctionIn();
    ClearOMXBuffer(pSfOMXComponent, pBufferHdr);
    LOG(SF_LOG_PERF, "buffer count = %d\r\n", GetOMXBufferCount(pSfOMXComponent));
    free(pBufferHdr);

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
    TestDecConfig *pTestDecConfig = (TestDecConfig *)hComponent->testConfig;
    pTestDecConfig->feedingMode = FEEDING_METHOD_BUFFER;
    pTestDecConfig->bitstreamMode = BS_MODE_PIC_END;
    pTestDecConfig->bitFormat = hComponent->bitFormat;
    /* 
     if cbcrInterleave is FALSE and nv21 is FALSE, the default dec format is I420
     if cbcrInterleave is TRUE and nv21 is FALSE, then the dec format is NV12
     if cbcrInterleave is TRUE and nv21 is TRUE, then the dec format is NV21
    */
    pTestDecConfig->cbcrInterleave = TRUE;
    pTestDecConfig->nv21 = FALSE;

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
    sem_init(hComponent->inputSemaphore, 0, 5);
EXIT:
    FunctionOut();

    return ret;
}

static OMX_ERRORTYPE SF_OMX_ComponentClear(SF_OMX_COMPONENT *hComponent)
{
    OMX_ERRORTYPE ret = OMX_ErrorNone;
    ComponentImpl *pSFComponentDecoder = (ComponentImpl *)hComponent->hSFComponentExecoder;
    ComponentImpl *pSFComponentFeeder = (ComponentImpl *)hComponent->hSFComponentFeeder;
    ComponentImpl *pSFComponentRender = (ComponentImpl *)hComponent->hSFComponentRender;

    FunctionIn();
    if (pSFComponentDecoder == NULL || pSFComponentFeeder == NULL || pSFComponentRender == NULL)
    {
        goto EXIT;
    }
    pSFComponentDecoder->terminate = OMX_TRUE;
    pSFComponentFeeder->terminate = OMX_TRUE;
    pSFComponentRender->terminate = OMX_TRUE;
    hComponent->functions->ComponentWait(hComponent->hSFComponentFeeder);
    hComponent->functions->ComponentWait(hComponent->hSFComponentExecoder);
    hComponent->functions->ComponentWait(hComponent->hSFComponentRender);

    free(hComponent->pusBitCode);
    hComponent->functions->ClearDecListenerContext(hComponent->lsnCtx);
    ComponentClearCommon(hComponent);
EXIT:
    FunctionOut();

    return ret;
}

SF_OMX_COMPONENT sf_dec_decoder_h265 = {
    .componentName = "sf.dec.decoder.h265",
    .libName = "libsfdec.so",
    .pOMXComponent = NULL,
    .SF_OMX_ComponentConstructor = SF_OMX_ComponentConstructor,
    .SF_OMX_ComponentClear = SF_OMX_ComponentClear,
    .functions = NULL,
    .bitFormat = STD_HEVC,
    .fwPath = "/lib/firmware/chagall.bin"};

SF_OMX_COMPONENT sf_dec_decoder_h264 = {
    .componentName = "sf.dec.decoder.h264",
    .libName = "libsfdec.so",
    .pOMXComponent = NULL,
    .SF_OMX_ComponentConstructor = SF_OMX_ComponentConstructor,
    .SF_OMX_ComponentClear = SF_OMX_ComponentClear,
    .functions = NULL,
    .bitFormat = STD_AVC,
    .fwPath = "/lib/firmware/chagall.bin"};