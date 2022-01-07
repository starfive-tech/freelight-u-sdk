// SPDX-License-Identifier: GPL-2.0
/*
 * Copyright (C) 2021 StarFive Technology Co., Ltd.
 */
#ifndef SF_OMX_VIDEO_COMMON
#define SF_OMX_VIDEO_COMMON

#include "OMX_Component.h"
#include "OMX_Video.h"
#include "OMX_VideoExt.h"
#include "OMX_Index.h"
#include "OMX_IndexExt.h"
#include "SF_OMX_Core.h"
#include "component.h"

#define INIT_SET_SIZE_VERSION(_struct_, _structType_) \
    do                                                \
    {                                                 \
        memset((_struct_), 0, sizeof(_structType_));  \
        (_struct_)->nSize = sizeof(_structType_);     \
        (_struct_)->nVersion.s.nVersionMajor = 1;     \
        (_struct_)->nVersion.s.nVersionMinor = 18;    \
        (_struct_)->nVersion.s.nRevision = 1;         \
        (_struct_)->nVersion.s.nStep = 0;             \
    } while (0)

#define DEFAULT_FRAME_WIDTH 0
#define DEFAULT_FRAME_HEIGHT 0
#define DEFAULT_VIDEO_INPUT_BUFFER_SIZE (DEFAULT_FRAME_WIDTH * DEFAULT_FRAME_HEIGHT) / 2
#define DEFAULT_VIDEO_OUTPUT_BUFFER_SIZE (DEFAULT_FRAME_WIDTH * DEFAULT_FRAME_HEIGHT * 3) / 2

#ifdef __cplusplus
extern "C"
{
#endif

    BOOL CheckEncTestConfig(TestEncConfig *testConfig);
    BOOL CheckDecTestConfig(TestDecConfig *testConfig);
    OMX_ERRORTYPE GetStateCommon(OMX_IN OMX_HANDLETYPE hComponent, OMX_OUT OMX_STATETYPE *pState);
    OMX_ERRORTYPE InitComponentStructorCommon(SF_OMX_COMPONENT *hComponent);
    OMX_ERRORTYPE ComponentClearCommon(SF_OMX_COMPONENT *hComponent);
    OMX_ERRORTYPE FlushBuffer(SF_OMX_COMPONENT *pSfOMXComponent, OMX_U32 nPort);

#ifdef __cplusplus
}
#endif

#endif
