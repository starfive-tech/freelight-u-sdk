// SPDX-License-Identifier: GPL-2.0
/*
 * Copyright (C) 2021 StarFive Technology Co., Ltd.
 */
#include "SF_OMX_mjpeg_common.h"

static void sf_get_component_functions(SF_CODAJ12_FUNCTIONS *funcs, OMX_PTR *sohandle);

OMX_ERRORTYPE GetStateMjpegCommon(OMX_IN OMX_HANDLETYPE hComponent, OMX_OUT OMX_STATETYPE *pState)
{
    OMX_ERRORTYPE ret = OMX_ErrorNone;
    OMX_COMPONENTTYPE *pOMXComponent = NULL;
    SF_OMX_COMPONENT *pSfOMXComponent = NULL;
    OMX_STATETYPE nextState;

    FunctionIn();

EXIT:
    FunctionOut();
    return ret;
}

OMX_ERRORTYPE InitMjpegStructorCommon(SF_OMX_COMPONENT *pSfOMXComponent)
{
    OMX_ERRORTYPE ret = OMX_ErrorNone;
    char *strDebugLevel = NULL;
    int debugLevel = 0;
    SF_CODAJ12_IMPLEMEMT *pSfCodaj12Implement = NULL;

    FunctionIn();
    if (pSfOMXComponent == NULL)
    {
        ret = OMX_ErrorBadParameter;
        goto EXIT;
    }

    pSfOMXComponent->pOMXComponent = malloc(sizeof(OMX_COMPONENTTYPE));
    if (pSfOMXComponent->pOMXComponent == NULL)
    {
        ret = OMX_ErrorInsufficientResources;
        LOG(SF_LOG_ERR, "malloc fail\r\n");
        goto ERROR;
    }
    memset(pSfOMXComponent->pOMXComponent, 0, sizeof(OMX_COMPONENTTYPE));

    pSfOMXComponent->soHandle = dlopen(pSfOMXComponent->libName, RTLD_NOW);
    if (pSfOMXComponent->soHandle == NULL)
    {
        ret = OMX_ErrorInsufficientResources;
        LOG(SF_LOG_ERR, "could not open %s, error: %s\r\n", pSfOMXComponent->libName, dlerror());
        goto ERROR;
    }

    pSfOMXComponent->componentImpl = malloc(sizeof(SF_CODAJ12_IMPLEMEMT));
    if (pSfOMXComponent->componentImpl == NULL)
    {
        ret = OMX_ErrorInsufficientResources;
        LOG(SF_LOG_ERR, "malloc fail\r\n");
        goto ERROR;
    }
    memset(pSfOMXComponent->componentImpl, 0, sizeof(SF_CODAJ12_IMPLEMEMT));

    pSfCodaj12Implement = (SF_CODAJ12_IMPLEMEMT *)pSfOMXComponent->componentImpl;
    pSfCodaj12Implement->functions = malloc(sizeof(SF_CODAJ12_FUNCTIONS));
    pSfCodaj12Implement->currentState = OMX_StateLoaded;
    pSfCodaj12Implement->frameFormat = DEFAULT_FRAME_FORMAT;
    if (pSfCodaj12Implement->functions == NULL)
    {
        ret = OMX_ErrorInsufficientResources;
        LOG(SF_LOG_ERR, "malloc fail\r\n");
        goto ERROR;
    }
    memset(pSfCodaj12Implement->functions, 0, sizeof(SF_CODAJ12_FUNCTIONS));
    sf_get_component_functions(pSfCodaj12Implement->functions, pSfOMXComponent->soHandle);

    // Init JPU log
    if (pSfCodaj12Implement->functions->SetMaxLogLevel)
    {
        strDebugLevel = getenv("JPU_DEBUG");
        if (strDebugLevel)
        {
            debugLevel = atoi(strDebugLevel);
            if (debugLevel >=0)
            {
                pSfCodaj12Implement->functions->SetMaxLogLevel(debugLevel);
            }
        }
    }

    if (strstr(pSfOMXComponent->componentName, "sf.dec") != NULL)
    {
        pSfCodaj12Implement->config = malloc(sizeof(DecConfigParam));
        if (pSfCodaj12Implement->config == NULL)
        {
            ret = OMX_ErrorInsufficientResources;
            LOG(SF_LOG_ERR, "malloc fail\r\n");
            goto ERROR;
        }
        memset(pSfCodaj12Implement->config, 0, sizeof(DecConfigParam));
    }
    else
    {
        ret = OMX_ErrorBadParameter;
        LOG(SF_LOG_ERR, "unknown component!\r\n");
        goto ERROR;
    }

    pSfOMXComponent->pOMXComponent->pComponentPrivate = pSfOMXComponent;

    pSfCodaj12Implement->sInputMessageQueue = msgget(IPC_PRIVATE, 0666 | IPC_CREAT);
    if (pSfCodaj12Implement->sInputMessageQueue < 0)
    {
        LOG(SF_LOG_ERR, "get ipc_id error");
        return;
    }
    pSfCodaj12Implement->sOutputMessageQueue = msgget(IPC_PRIVATE, 0666 | IPC_CREAT);
    if (pSfCodaj12Implement->sOutputMessageQueue < 0)
    {
        LOG(SF_LOG_ERR, "get ipc_id error");
        return;
    }

    for (int i = 0; i < 2; i++)
    {
        OMX_PARAM_PORTDEFINITIONTYPE *pPortDefinition = &pSfOMXComponent->portDefinition[i];

        // OMX_IMAGE_PARAM_PORTFORMATTYPE * imagePortFormat = &pSfOMXComponent->MJPEGComponent[i];
        // memset(&imagePortFormat, 0, sizeof(OMX_IMAGE_PARAM_PORTFORMATTYPE));
        // INIT_SET_SIZE_VERSION(imagePortFormat, OMX_PARAM_PORTDEFINITIONTYPE);
        // imagePortFormat->nPortIndex = i;
        // imagePortFormat->nIndex = 0; //TODO
        // imagePortFormat->eCompressionFormat = OMX_IMAGE_CodingJPEG;
        // imagePortFormat->eColorFormat = OMX_COLOR_FormatUnused;  //TODO

        INIT_SET_SIZE_VERSION(pPortDefinition, OMX_PARAM_PORTDEFINITIONTYPE);
        // INIT_SET_SIZE_VERSION(pAVCComponent, OMX_VIDEO_PARAM_AVCTYPE);
        // OMX_VIDEO_PARAM_AVCTYPE *pAVCComponent = &pSfOMXComponent->AVCComponent[i];
        pPortDefinition->nPortIndex = i;
        // pPortDefinition->nBufferCountActual = VPU_OUTPUT_BUF_NUMBER;
        pPortDefinition->nBufferCountActual = (i == 0 ? CODAJ12_INPUT_BUF_NUMBER : CODAJ12_OUTPUT_BUF_NUMBER);
        pPortDefinition->nBufferCountMin = (i == 0 ? CODAJ12_INPUT_BUF_NUMBER : CODAJ12_OUTPUT_BUF_NUMBER);
        pPortDefinition->nBufferSize = (i == 0 ? DEFAULT_MJPEG_INPUT_BUFFER_SIZE : DEFAULT_MJPEG_OUTPUT_BUFFER_SIZE);
        pPortDefinition->eDomain = OMX_PortDomainVideo;
        // TODO happer
        // pPortDefinition->format.image.pNativeWindow = 0x0;
        pPortDefinition->format.image.nFrameWidth = DEFAULT_FRAME_WIDTH;
        pPortDefinition->format.image.nFrameHeight = DEFAULT_FRAME_HEIGHT;
        pPortDefinition->format.image.nStride = DEFAULT_FRAME_WIDTH;
        pPortDefinition->format.image.nSliceHeight = DEFAULT_FRAME_HEIGHT;
        pPortDefinition->format.image.eCompressionFormat = OMX_IMAGE_CodingUnused;
        pPortDefinition->format.image.cMIMEType = malloc(OMX_MAX_STRINGNAME_SIZE);

        if (pPortDefinition->format.image.cMIMEType == NULL)
        {
            ret = OMX_ErrorInsufficientResources;
            free(pPortDefinition->format.image.cMIMEType);
            pPortDefinition->format.image.cMIMEType = NULL;
            LOG(SF_LOG_ERR, "malloc fail\r\n");
            goto ERROR;
        }
        memset(pPortDefinition->format.image.cMIMEType, 0, OMX_MAX_STRINGNAME_SIZE);
        pPortDefinition->format.image.pNativeRender = 0;
        pPortDefinition->format.image.bFlagErrorConcealment = OMX_FALSE;
        pPortDefinition->format.image.eColorFormat = OMX_COLOR_FormatUnused;
        pPortDefinition->bEnabled = OMX_TRUE;

        pPortDefinition->eDir = (i == 0 ? OMX_DirInput : OMX_DirOutput);
    }

    strcpy(pSfOMXComponent->portDefinition[1].format.image.cMIMEType, "JPEG");
    pSfOMXComponent->portDefinition[1].format.image.eColorFormat = OMX_COLOR_FormatYUV420SemiPlanar;

    memset(pSfOMXComponent->pBufferArray, 0, sizeof(pSfOMXComponent->pBufferArray));
    pSfOMXComponent->memory_optimization = OMX_TRUE;

    FunctionOut();
EXIT:
    return ret;
ERROR:
    if (pSfOMXComponent->pOMXComponent)
    {
        free(pSfOMXComponent->pOMXComponent);
        pSfOMXComponent->pOMXComponent = NULL;
    }

    return ret;
}

static void sf_get_component_functions(SF_CODAJ12_FUNCTIONS *funcs, OMX_PTR *sohandle)
{
    FunctionIn();
    funcs->JPU_Init = dlsym(sohandle, "JPU_Init");
    funcs->JPU_DecOpen = dlsym(sohandle, "JPU_DecOpen");
    funcs->JPU_DecGetInitialInfo = dlsym(sohandle, "JPU_DecGetInitialInfo");
    funcs->JPU_DecRegisterFrameBuffer2 = dlsym(sohandle, "JPU_DecRegisterFrameBuffer2");
    funcs->JPU_DecRegisterFrameBuffer = dlsym(sohandle, "JPU_DecRegisterFrameBuffer");
    funcs->JPU_DecGiveCommand = dlsym(sohandle, "JPU_DecGiveCommand");
    funcs->JPU_DecStartOneFrame = dlsym(sohandle, "JPU_DecStartOneFrame");
    funcs->JPU_DecGetOutputInfo = dlsym(sohandle, "JPU_DecGetOutputInfo");
    funcs->JPU_SWReset = dlsym(sohandle, "JPU_SWReset");
    funcs->JPU_DecSetRdPtrEx = dlsym(sohandle, "JPU_DecSetRdPtrEx");
    funcs->JPU_DecClose = dlsym(sohandle, "JPU_DecClose");
    funcs->JPU_WaitInterrupt = dlsym(sohandle, "JPU_WaitInterrupt");
    funcs->JPU_GetFrameBufPool = dlsym(sohandle, "JPU_GetFrameBufPool");
    funcs->JPU_ClrStatus = dlsym(sohandle, "JPU_ClrStatus");
    funcs->JPU_DeInit = dlsym(sohandle, "JPU_DeInit");
    funcs->BSFeederBuffer_SetData = dlsym(sohandle, "BSFeederBuffer_SetData");
    funcs->BitstreamFeeder_SetData = dlsym(sohandle, "BitstreamFeeder_SetData");
    funcs->BitstreamFeeder_Create = dlsym(sohandle, "BitstreamFeeder_Create");
    funcs->BitstreamFeeder_Act = dlsym(sohandle, "BitstreamFeeder_Act");
    funcs->BitstreamFeeder_Destroy = dlsym(sohandle, "BitstreamFeeder_Destroy");
    funcs->jdi_allocate_dma_memory = dlsym(sohandle, "jdi_allocate_dma_memory");
    funcs->jdi_free_dma_memory = dlsym(sohandle, "jdi_free_dma_memory");
    funcs->AllocateOneFrameBuffer = dlsym(sohandle, "AllocateOneFrameBuffer");
    funcs->FreeFrameBuffer = dlsym(sohandle, "FreeFrameBuffer");
    funcs->GetFrameBuffer = dlsym(sohandle, "GetFrameBuffer");
    funcs->GetFrameBufferCount = dlsym(sohandle, "GetFrameBufferCount");
    funcs->UpdateFrameBuffers = dlsym(sohandle, "UpdateFrameBuffers");
    funcs->SetMaxLogLevel = dlsym(sohandle, "SetMaxLogLevel");
    FunctionOut();
}

OMX_U8 *AllocateOutputBuffer(SF_OMX_COMPONENT *pSfOMXComponent, OMX_U32 nSizeBytes)
{
    SF_CODAJ12_IMPLEMEMT *pSfCodaj12Implement = pSfOMXComponent->componentImpl;
    Int32 instIdx = pSfCodaj12Implement->instIdx;
    JpgDecOpenParam *decOP = &pSfCodaj12Implement->decOP;
    DecConfigParam *decConfig = pSfCodaj12Implement->config;
    JpgDecInitialInfo *initialInfo = &pSfCodaj12Implement->initialInfo;
    FrameBuffer *pFrameBuf = &pSfCodaj12Implement->frameBuf;
    JpgDecHandle handle = pSfCodaj12Implement->handle;

    Uint32 framebufWidth = 0, framebufHeight = 0, framebufStride = 0;
    Uint32 decodingWidth, decodingHeight;
    BOOL scalerOn = FALSE;
    Uint32 bitDepth = 0;
    FrameFormat subsample;
    Uint32 temp;
    OMX_U8 *virtAddr;
    Uint32 bufferIndex;
    JpgRet jpgret;

    FunctionIn();
    if (initialInfo->sourceFormat == FORMAT_420 || initialInfo->sourceFormat == FORMAT_422)
        framebufWidth = JPU_CEIL(16, initialInfo->picWidth);
    else
        framebufWidth = JPU_CEIL(8, initialInfo->picWidth);

    if (initialInfo->sourceFormat == FORMAT_420 || initialInfo->sourceFormat == FORMAT_440)
        framebufHeight = JPU_CEIL(16, initialInfo->picHeight);
    else
        framebufHeight = JPU_CEIL(8, initialInfo->picHeight);

    if (framebufWidth == 0 || framebufHeight == 0)
    {
        LOG(SF_LOG_WARN, "width or height == 0, use port parameters\r\n");
        for (int i = 0; i < OMX_PORT_MAX; i ++)
        {
            OMX_PARAM_PORTDEFINITIONTYPE *pPort = &pSfOMXComponent->portDefinition[i];
            framebufWidth = pPort->format.video.nFrameWidth;
            framebufHeight = pPort->format.video.nFrameHeight;
            if (framebufWidth > 0 && framebufHeight > 0)
            {
                LOG(SF_LOG_INFO, "Use port: %d\r\n", i);
                break;
            }
        }
        if (framebufWidth == 0 || framebufHeight == 0)
        {
            LOG(SF_LOG_ERR, "Can not get frame size\r\n");
            return NULL;
        }
    }
    LOG(SF_LOG_DEBUG, "framebufWidth: %d, framebufHeight: %d\r\n", framebufWidth, framebufHeight);

    decodingWidth = framebufWidth >> decConfig->iHorScaleMode;
    decodingHeight = framebufHeight >> decConfig->iVerScaleMode;
    if (decOP->packedFormat != PACKED_FORMAT_NONE && decOP->packedFormat != PACKED_FORMAT_444)
    {
        // When packed format, scale-down resolution should be multiple of 2.
        decodingWidth = JPU_CEIL(2, decodingWidth);
    }

    subsample = pSfCodaj12Implement->frameFormat;
    temp = decodingWidth;
    decodingWidth = (decConfig->rotation == 90 || decConfig->rotation == 270) ? decodingHeight : decodingWidth;
    decodingHeight = (decConfig->rotation == 90 || decConfig->rotation == 270) ? temp : decodingHeight;
    if (decConfig->roiEnable == TRUE)
    {
        decodingWidth = framebufWidth = initialInfo->roiFrameWidth;
        decodingHeight = framebufHeight = initialInfo->roiFrameHeight;
    }

    LOG(SF_LOG_DEBUG, "decodingWidth: %d, decodingHeight: %d\n", decodingWidth, decodingHeight);

    if (decOP->rotation != 0 || decOP->mirror != MIRDIR_NONE)
    {
        if (decOP->outputFormat != FORMAT_MAX && decOP->outputFormat != initialInfo->sourceFormat)
        {
            LOG(SF_LOG_ERR, "The rotator cannot work with the format converter together.\n");
            return NULL;
        }
    }

    LOG(SF_LOG_INFO, "<INSTANCE %d>\n", instIdx);
    LOG(SF_LOG_INFO, "SOURCE PICTURE SIZE : W(%d) H(%d)\n", initialInfo->picWidth, initialInfo->picHeight);
    LOG(SF_LOG_INFO, "SUBSAMPLE           : %d\n", subsample);

    bitDepth = initialInfo->bitDepth;
    scalerOn = (BOOL)(decConfig->iHorScaleMode || decConfig->iVerScaleMode);
    // may be handle == NULL on ffmpeg case
    if (bitDepth == 0)
    {
        bitDepth = 8;
    }
    LOG(SF_LOG_DEBUG, "AllocateOneFrameBuffer\r\n");
    LOG_APPEND(SF_LOG_DEBUG, "instIdx = %d subsample = %d chromaInterleave = %d packedFormat = %d rotation = %d\r\n",
                instIdx, subsample, decOP->chromaInterleave, decOP->packedFormat, decConfig->rotation);
    LOG_APPEND(SF_LOG_DEBUG, "scalerOn = %d decodingWidth = %d decodingHeight = %d bitDepth = %d\r\n",
                scalerOn, decodingWidth, decodingHeight, bitDepth);
    virtAddr = (OMX_U8 *)pSfCodaj12Implement->functions->AllocateOneFrameBuffer
        (instIdx, subsample, decOP->chromaInterleave, decOP->packedFormat, decConfig->rotation,
        scalerOn, decodingWidth, decodingHeight, bitDepth, &bufferIndex);
    if (virtAddr == NULL)
    {
        LOG(SF_LOG_ERR, "Failed to AllocateOneFrameBuffer()\n");
        return NULL;
    }
    LOG(SF_LOG_INFO, "Allocate frame buffer %p, index = %d\r\n", virtAddr, bufferIndex);

    //Register frame buffer
    FRAME_BUF *pFrame = pSfCodaj12Implement->functions->GetFrameBuffer(instIdx, bufferIndex);
    memcpy(&pSfCodaj12Implement->frame[bufferIndex], pFrame, sizeof(FRAME_BUF));
    pFrameBuf[bufferIndex].bufY = pFrame->vbY.phys_addr;
    pFrameBuf[bufferIndex].bufCb = pFrame->vbCb.phys_addr;
    if (decOP->chromaInterleave == CBCR_SEPARATED)
        pFrameBuf[bufferIndex].bufCr = pFrame->vbCr.phys_addr;
    pFrameBuf[bufferIndex].stride = pFrame->strideY;
    pFrameBuf[bufferIndex].strideC = pFrame->strideC;
    pFrameBuf[bufferIndex].endian = decOP->frameEndian;
    pFrameBuf[bufferIndex].format = (FrameFormat)pFrame->Format;
    framebufStride = pFrameBuf[bufferIndex].stride;

    // may be handle == NULL on ffmpeg case
    jpgret = pSfCodaj12Implement->functions->JPU_DecRegisterFrameBuffer2(handle, &pFrameBuf[bufferIndex], framebufStride);
    LOG(SF_LOG_DEBUG, "JPU_DecRegisterFrameBuffer2 ret = %d\r\n", jpgret);
    FunctionOut();
    return virtAddr;
}

OMX_ERRORTYPE CreateThread(OMX_HANDLETYPE *threadHandle, OMX_PTR function_name, OMX_PTR argument)
{
    FunctionIn();

    int result = 0;
    int detach_ret = 0;
    THREAD_HANDLE_TYPE *thread;
    OMX_ERRORTYPE ret = OMX_ErrorNone;

    thread = malloc(sizeof(THREAD_HANDLE_TYPE));
    memset(thread, 0, sizeof(THREAD_HANDLE_TYPE));

    pthread_attr_init(&thread->attr);
    if (thread->stack_size != 0)
        pthread_attr_setstacksize(&thread->attr, thread->stack_size);

    /* set priority */
    if (thread->schedparam.sched_priority != 0)
        pthread_attr_setschedparam(&thread->attr, &thread->schedparam);

    detach_ret = pthread_attr_setdetachstate(&thread->attr, PTHREAD_CREATE_JOINABLE);
    if (detach_ret != 0)
    {
        free(thread);
        *threadHandle = NULL;
        ret = OMX_ErrorUndefined;
        goto EXIT;
    }

    result = pthread_create(&thread->pthread, &thread->attr, function_name, (void *)argument);
    /* pthread_setschedparam(thread->pthread, SCHED_RR, &thread->schedparam); */

    switch (result)
    {
    case 0:
        *threadHandle = (OMX_HANDLETYPE)thread;
        ret = OMX_ErrorNone;
        break;
    case EAGAIN:
        free(thread);
        *threadHandle = NULL;
        ret = OMX_ErrorInsufficientResources;
        break;
    default:
        free(thread);
        *threadHandle = NULL;
        ret = OMX_ErrorUndefined;
        break;
    }

EXIT:
    FunctionOut();

    return ret;
}

void ThreadExit(void *value_ptr)
{
    pthread_exit(value_ptr);
    return;
}

void CodaJ12FlushBuffer(SF_OMX_COMPONENT *pSfOMXComponent, OMX_U32 nPortNumber)
{
    SF_CODAJ12_IMPLEMEMT *pSfCodaj12Implement = pSfOMXComponent->componentImpl;
    Message data;
    OMX_BUFFERHEADERTYPE *pOMXBuffer = NULL;

    FunctionIn();
    switch (nPortNumber)
    {    
    case OMX_INPUT_PORT_INDEX:
        while (OMX_TRUE)
        {
            if (msgrcv(pSfCodaj12Implement->sInputMessageQueue, (void *)&data, BUFSIZ, 0, IPC_NOWAIT) == -1)
            {
                break;
            }
            else
            {
                pOMXBuffer = data.pBuffer;
                pOMXBuffer->nFilledLen = 0;
                pOMXBuffer->nFlags = OMX_BUFFERFLAG_EOS;
                pSfOMXComponent->callbacks->EmptyBufferDone(pSfOMXComponent->pOMXComponent, pSfOMXComponent->pAppData, pOMXBuffer);
            }
        }
        break;
    case OMX_OUTPUT_PORT_INDEX:
        while (OMX_TRUE)
        {
            if (msgrcv(pSfCodaj12Implement->sOutputMessageQueue, (void *)&data, BUFSIZ, 0, IPC_NOWAIT) == -1)
            {
                break;
            }
            else
            {
                pOMXBuffer = data.pBuffer;
                pOMXBuffer->nFilledLen = 0;
                pOMXBuffer->nFlags = OMX_BUFFERFLAG_EOS;
                pSfOMXComponent->callbacks->FillBufferDone(pSfOMXComponent->pOMXComponent, pSfOMXComponent->pAppData, pOMXBuffer);
            }
        }
        break;
    default:
        break;
    }
    FunctionOut();
}