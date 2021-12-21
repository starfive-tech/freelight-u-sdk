// SPDX-License-Identifier: GPL-2.0
/*
 * Copyright (C) 2021 StarFive Technology Co., Ltd.
 */
#include "SF_OMX_video_common.h"

OMX_ERRORTYPE GetStateCommon(OMX_IN OMX_HANDLETYPE hComponent, OMX_OUT OMX_STATETYPE *pState)
{
    OMX_ERRORTYPE ret = OMX_ErrorNone;
    OMX_COMPONENTTYPE *pOMXComponent = NULL;
    SF_OMX_COMPONENT *pSfOMXComponent = NULL;
    ComponentState state;
    OMX_STATETYPE nextState;

    FunctionIn();
    if (hComponent == NULL)
    {
        ret = OMX_ErrorBadParameter;
        goto EXIT;
    }
    pOMXComponent = (OMX_COMPONENTTYPE *)hComponent;
    pSfOMXComponent = pOMXComponent->pComponentPrivate;
    nextState = pSfOMXComponent->nextState;
    state = pSfOMXComponent->functions->ComponentGetState(pSfOMXComponent->hSFComponentExecoder);
    LOG(SF_LOG_INFO, "state = %d\r\n", state);

    switch (state)
    {
    case COMPONENT_STATE_CREATED:
        *pState = OMX_StateIdle;
        break;
    case COMPONENT_STATE_NONE:
    case COMPONENT_STATE_TERMINATED:
        *pState = OMX_StateLoaded;
        break;
    case COMPONENT_STATE_PREPARED:
    case COMPONENT_STATE_EXECUTED:
        if (nextState == OMX_StateIdle || nextState == OMX_StateExecuting || nextState == OMX_StatePause)
        {
            *pState = nextState;
        }
        break;
    default:
        LOG(SF_LOG_WARN, "unknown state:%d \r\n", state);
        ret = OMX_ErrorUndefined;
        break;
    }
EXIT:
    FunctionOut();
    return ret;
}

OMX_ERRORTYPE ComponentClearCommon(SF_OMX_COMPONENT *hComponent)
{
    hComponent->functions->ComponentRelease(hComponent->hSFComponentExecoder);
    hComponent->functions->ComponentDestroy(hComponent->hSFComponentExecoder, NULL);
    hComponent->functions->DeInitLog();
    dlclose(hComponent->soHandle);
    free(hComponent->functions);
    free(hComponent->testConfig);
    free(hComponent->config);
    free(hComponent->lsnCtx);
    free(hComponent->pOMXComponent);
    for (int i = 0; i < 2; i++)
    {
        OMX_PARAM_PORTDEFINITIONTYPE *pPortDefinition = &hComponent->portDefinition[i];
        free(pPortDefinition->format.video.cMIMEType);
    }
    return OMX_ErrorNone;
}

BOOL CheckEncTestConfig(TestEncConfig *testConfig)
{
    FunctionIn();
    if ((testConfig->compareType & (1 << MODE_SAVE_ENCODED)) && testConfig->bitstreamFileName[0] == 0)
    {
        testConfig->compareType &= ~(1 << MODE_SAVE_ENCODED);
        LOG(SF_LOG_ERR, "You want to Save bitstream data. Set the path\r\n");
        return FALSE;
    }

    if ((testConfig->compareType & (1 << MODE_COMP_ENCODED)) && testConfig->ref_stream_path[0] == 0)
    {
        testConfig->compareType &= ~(1 << MODE_COMP_ENCODED);
        LOG(SF_LOG_ERR, "You want to Compare bitstream data. Set the path\r\n");
        return FALSE;
    }

    FunctionOut();
    return TRUE;
}

BOOL CheckDecTestConfig(TestDecConfig *testConfig)
{
    BOOL isValidParameters = TRUE;

    /* Check parameters */
    if (testConfig->skipMode < 0 || testConfig->skipMode == 3 || testConfig->skipMode > 4)
    {
        LOG(SF_LOG_WARN, "Invalid skip mode: %d\n", testConfig->skipMode);
        isValidParameters = FALSE;
    }
    if ((FORMAT_422 < testConfig->wtlFormat && testConfig->wtlFormat <= FORMAT_400) ||
        testConfig->wtlFormat < FORMAT_420 || testConfig->wtlFormat >= FORMAT_MAX)
    {
        LOG(SF_LOG_WARN, "Invalid WTL format(%d)\n", testConfig->wtlFormat);
        isValidParameters = FALSE;
    }
    if (isValidParameters == TRUE && (testConfig->scaleDownWidth > 0 || testConfig->scaleDownHeight > 0))
    {
    }
    if (testConfig->renderType < RENDER_DEVICE_NULL || testConfig->renderType >= RENDER_DEVICE_MAX)
    {
        LOG(SF_LOG_WARN, "unknown render device type(%d)\n", testConfig->renderType);
        isValidParameters = FALSE;
    }
    if (testConfig->thumbnailMode == TRUE && testConfig->skipMode != 0)
    {
        LOG(SF_LOG_WARN, "Turn off thumbnail mode or skip mode\n");
        isValidParameters = FALSE;
    }

    return isValidParameters;
}

OMX_ERRORTYPE InitComponentStructorCommon(SF_OMX_COMPONENT *hComponent)
{
    OMX_ERRORTYPE ret = OMX_ErrorNone;
    char *strDebugLevel = NULL;
    int debugLevel = 0;

    FunctionIn();
    if (hComponent == NULL)
    {
        ret = OMX_ErrorBadParameter;
        goto EXIT;
    }

    hComponent->pOMXComponent = malloc(sizeof(OMX_COMPONENTTYPE));
    if (hComponent->pOMXComponent == NULL)
    {
        ret = OMX_ErrorInsufficientResources;
        LOG(SF_LOG_ERR, "malloc fail\r\n");
        goto ERROR;
    }
    memset(hComponent->pOMXComponent, 0, sizeof(OMX_COMPONENTTYPE));
    hComponent->soHandle = dlopen(hComponent->libName, RTLD_NOW);
    if (hComponent->soHandle == NULL)
    {
        ret = OMX_ErrorInsufficientResources;
        LOG(SF_LOG_ERR, "could not open %s\r\n", hComponent->libName);
        goto ERROR;
    }

    hComponent->functions = malloc(sizeof(SF_COMPONENT_FUNCTIONS));
    if (hComponent->functions == NULL)
    {
        ret = OMX_ErrorInsufficientResources;
        LOG(SF_LOG_ERR, "malloc fail\r\n");
        goto ERROR;
    }
    memset(hComponent->functions, 0, sizeof(SF_COMPONENT_FUNCTIONS));
    sf_get_component_functions(hComponent->functions, hComponent->soHandle);

    // Init VPU log
    if (hComponent->functions->InitLog && hComponent->functions->SetMaxLogLevel)
    {
        hComponent->functions->InitLog();
        strDebugLevel = getenv("VPU_DEBUG");
        if (strDebugLevel)
        {
            debugLevel = atoi(strDebugLevel);
            if (debugLevel >=0)
            {
                hComponent->functions->SetMaxLogLevel(debugLevel);
            }
        }
    }

    if (strstr(hComponent->componentName, "sf.enc") != NULL)
    {
        hComponent->testConfig = malloc(sizeof(TestEncConfig));
        if (hComponent->testConfig == NULL)
        {
            ret = OMX_ErrorInsufficientResources;
            LOG(SF_LOG_ERR, "malloc fail\r\n");
            goto ERROR;
        }
        hComponent->lsnCtx = malloc(sizeof(EncListenerContext));
        if (hComponent->lsnCtx == NULL)
        {
            ret = OMX_ErrorInsufficientResources;
            LOG(SF_LOG_ERR, "malloc fail\r\n");
            goto ERROR;
        }
        memset(hComponent->testConfig, 0, sizeof(TestEncConfig));
        memset(hComponent->lsnCtx, 0, sizeof(EncListenerContext));
        hComponent->functions->SetDefaultEncTestConfig(hComponent->testConfig);
    }
    else if (strstr(hComponent->componentName, "sf.dec") != NULL)
    {
        hComponent->testConfig = malloc(sizeof(TestDecConfig));
        if (hComponent->testConfig == NULL)
        {
            ret = OMX_ErrorInsufficientResources;
            LOG(SF_LOG_ERR, "malloc fail\r\n");
            goto ERROR;
        }
        hComponent->lsnCtx = malloc(sizeof(DecListenerContext));
        if (hComponent->lsnCtx == NULL)
        {
            ret = OMX_ErrorInsufficientResources;
            LOG(SF_LOG_ERR, "malloc fail\r\n");
            goto ERROR;
        }
        memset(hComponent->testConfig, 0, sizeof(TestDecConfig));
        memset(hComponent->lsnCtx, 0, sizeof(DecListenerContext));
        hComponent->functions->SetDefaultDecTestConfig(hComponent->testConfig);
    }
    else
    {
        ret = OMX_ErrorBadParameter;
        LOG(SF_LOG_ERR, "unknown component!\r\n");
        goto ERROR;
    }

    hComponent->config = malloc(sizeof(CNMComponentConfig));
    if (hComponent->config == NULL)
    {
        ret = OMX_ErrorInsufficientResources;
        LOG(SF_LOG_ERR, "malloc fail\r\n");
        goto ERROR;
    }
    memset(hComponent->config, 0, sizeof(CNMComponentConfig));

    hComponent->pOMXComponent->pComponentPrivate = hComponent;
    for (int i = 0; i < 2; i++)
    {
        OMX_PARAM_PORTDEFINITIONTYPE *pPortDefinition = &hComponent->portDefinition[i];
        OMX_VIDEO_PARAM_AVCTYPE *pAVCComponent = &hComponent->AVCComponent[i];
        OMX_VIDEO_PARAM_HEVCTYPE *pHEVCComponent = &hComponent->HEVCComponent[i];
        INIT_SET_SIZE_VERSION(pPortDefinition, OMX_PARAM_PORTDEFINITIONTYPE);
        INIT_SET_SIZE_VERSION(pAVCComponent, OMX_VIDEO_PARAM_AVCTYPE);
        INIT_SET_SIZE_VERSION(pHEVCComponent, OMX_VIDEO_PARAM_HEVCTYPE);
        pPortDefinition->nPortIndex = i;
        pPortDefinition->nBufferCountActual = VPU_OUTPUT_BUF_NUMBER;
        pPortDefinition->nBufferCountMin = VPU_OUTPUT_BUF_NUMBER;
        pPortDefinition->nBufferSize = 0;
        pPortDefinition->eDomain = OMX_PortDomainVideo;
        pPortDefinition->format.video.nFrameWidth = DEFAULT_FRAME_WIDTH;
        pPortDefinition->format.video.nFrameHeight = DEFAULT_FRAME_HEIGHT;
        pPortDefinition->format.video.nStride = DEFAULT_FRAME_WIDTH;
        pPortDefinition->format.video.nSliceHeight = DEFAULT_FRAME_HEIGHT;
        pPortDefinition->format.video.eCompressionFormat = OMX_VIDEO_CodingUnused;
        pPortDefinition->format.video.cMIMEType = malloc(OMX_MAX_STRINGNAME_SIZE);
        pPortDefinition->format.video.xFramerate = 30;
        if (pPortDefinition->format.video.cMIMEType == NULL)
        {
            ret = OMX_ErrorInsufficientResources;
            free(pPortDefinition->format.video.cMIMEType);
            pPortDefinition->format.video.cMIMEType = NULL;
            LOG(SF_LOG_ERR, "malloc fail\r\n");
            goto ERROR;
        }
        memset(pPortDefinition->format.video.cMIMEType, 0, OMX_MAX_STRINGNAME_SIZE);
        pPortDefinition->format.video.pNativeRender = 0;
        pPortDefinition->format.video.bFlagErrorConcealment = OMX_FALSE;
        pPortDefinition->format.video.eColorFormat = OMX_COLOR_FormatUnused;
        pPortDefinition->bEnabled = OMX_TRUE;

        pAVCComponent->nPortIndex = i;
        pAVCComponent->nPFrames = 30;
        pAVCComponent->eProfile = OMX_VIDEO_AVCProfileHigh;
        pHEVCComponent->nPortIndex = i;
        pHEVCComponent->nKeyFrameInterval = 30;
        pHEVCComponent->eProfile = OMX_VIDEO_HEVCProfileMain;
    }

    hComponent->portDefinition[0].eDir = OMX_DirInput;
    hComponent->portDefinition[0].nBufferSize = DEFAULT_VIDEO_INPUT_BUFFER_SIZE;
    hComponent->portDefinition[0].nBufferCountActual = VPU_INPUT_BUF_NUMBER;
    hComponent->portDefinition[0].nBufferCountMin = VPU_INPUT_BUF_NUMBER;

    strcpy(hComponent->portDefinition[1].format.video.cMIMEType, "raw/video");
    hComponent->portDefinition[1].format.video.eColorFormat = OMX_COLOR_FormatYUV420SemiPlanar;
    hComponent->portDefinition[1].eDir = OMX_DirOutput;
    hComponent->portDefinition[1].nBufferSize = DEFAULT_VIDEO_OUTPUT_BUFFER_SIZE;
    hComponent->portDefinition[1].nBufferCountActual = VPU_OUTPUT_BUF_NUMBER;
    hComponent->portDefinition[1].nBufferCountMin = VPU_OUTPUT_BUF_NUMBER;

    memset(hComponent->pBufferArray, 0, sizeof(hComponent->pBufferArray));
    hComponent->memory_optimization = OMX_TRUE;

    FunctionOut();
EXIT:
    return ret;
ERROR:
    if (hComponent->pOMXComponent)
    {
        free(hComponent->pOMXComponent);
        hComponent->pOMXComponent = NULL;
    }
    if (hComponent->functions)
    {
        free(hComponent->functions);
        hComponent->functions = NULL;
    }
    if (hComponent->testConfig)
    {
        free(hComponent->testConfig);
        hComponent->testConfig = NULL;
    }
    if (hComponent->lsnCtx)
    {
        free(hComponent->lsnCtx);
        hComponent->lsnCtx = NULL;
    }
    if (hComponent->config)
    {
        free(hComponent->config);
        hComponent->config = NULL;
    }
    return ret;
}
