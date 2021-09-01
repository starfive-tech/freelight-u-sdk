// SPDX-License-Identifier: GPL-2.0
/*
 * Copyright (C) 2021 StarFive Technology Co., Ltd.
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "SF_OMX_video_common.h"
#include "SF_OMX_Core.h"

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

EXIT:
    FunctionOut();

    return ret;
}

static OMX_ERRORTYPE SF_OMX_ComponentClear(SF_OMX_COMPONENT *hComponent)
{
    OMX_ERRORTYPE ret = OMX_ErrorNone;
    FunctionIn();

EXIT:
    FunctionOut();

    return ret;  
}

SF_OMX_COMPONENT sf_enc_encoder = {
    .componentName = "sf.enc.encoder",
    .libName = "libsfenc.so",
    .pOMXComponent = NULL,
    .SF_OMX_ComponentConstructor = SF_OMX_ComponentConstructor,
    .SF_OMX_ComponentClear = SF_OMX_ComponentClear,
    .functions = NULL,
};