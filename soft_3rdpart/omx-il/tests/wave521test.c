// SPDX-License-Identifier: GPL-2.0
/*
 * Copyright (C) 2021 StarFive Technology Co., Ltd.
 */
#include <stdio.h>
#include <signal.h>
#include <OMX_Core.h>
#include <OMX_Component.h>
#include <OMX_Video.h>

#define OMX_INIT_STRUCTURE(a)         \
    memset(&(a), 0, sizeof(a));       \
    (a).nSize = sizeof(a);            \
    (a).nVersion.nVersion = 1;        \
    (a).nVersion.s.nVersionMajor = 1; \
    (a).nVersion.s.nVersionMinor = 1; \
    (a).nVersion.s.nRevision = 1;     \
    (a).nVersion.s.nStep = 1

static OMX_ERRORTYPE event_handler(
    OMX_HANDLETYPE hComponent,
    OMX_PTR pAppData,
    OMX_EVENTTYPE eEvent,
    OMX_U32 nData1,
    OMX_U32 nData2,
    OMX_PTR pEventData)
{

    return OMX_ErrorNone;
}

static OMX_ERRORTYPE fill_output_buffer_done_handler(
    OMX_HANDLETYPE hComponent,
    OMX_PTR pAppData,
    OMX_BUFFERHEADERTYPE *pBuffer)
{

    return OMX_ErrorNone;
}

static void block_until_state_changed(OMX_HANDLETYPE hComponent, OMX_STATETYPE wanted_eState)
{
    OMX_STATETYPE eState;
    while (eState != wanted_eState)
    {
        OMX_GetState(hComponent, &eState);
        if (eState != wanted_eState)
        {
            printf("state = %d\r\n", eState);
            usleep(1000 * 1000);
        }
    }
}

static int g_is_run  = 0;
static void signal_handle(int sig)
{ 
    printf("[%s,%d]: receive sig=%d \n", __FUNCTION__, __LINE__, sig);
    if (g_is_run) {
        OMX_Deinit();
        g_is_run = 0;
    }
}

//TODO: callback, parameter, buffer
int main(void)
{
    printf("=============================\r\n");
    OMX_HANDLETYPE hComponentEncoder;
    OMX_HANDLETYPE hComponentFeeder;
    OMX_HANDLETYPE hComponentReader;
    OMX_CALLBACKTYPE callbacks;
    int ret = OMX_ErrorNone;

    signal(SIGINT, signal_handle);

    ret = OMX_Init();
    if (ret != OMX_ErrorNone) {
        printf("[%s,%d]: run OMX_Init failed. ret is %d \n", __FUNCTION__, __LINE__, ret);
        return 1;
    }
    g_is_run = 1;

    callbacks.EventHandler = event_handler;
    callbacks.FillBufferDone = fill_output_buffer_done_handler;
    OMX_GetHandle(&hComponentEncoder, "sf.enc.encoder", NULL, &callbacks);
    OMX_GetHandle(&hComponentFeeder, "sf.enc.feeder", NULL, &callbacks);
    OMX_GetHandle(&hComponentReader, "sf.enc.reader", NULL, &callbacks);
    if (hComponentEncoder == NULL || hComponentFeeder == NULL || hComponentReader == NULL)
    {
        printf("could not get handle\r\n");
        return 0;
    }

    OMX_SetParameter(hComponentFeeder, OMX_IndexConfigCommonInputCrop, "./yuv/mix_1920x1080_8b_9frm.yuv");

    OMX_SetParameter(hComponentReader, OMX_IndexConfigCommonOutputCrop, "./output/inter_8b_11.cfg.265");

    OMX_SetParameter(hComponentEncoder, OMX_IndexParamOtherPortFormat, "./cfg/hevc_fhd_inter_8b_11.cfg");
    OMX_SetParameter(hComponentFeeder, OMX_IndexParamOtherPortFormat, "./cfg/hevc_fhd_inter_8b_11.cfg");
    OMX_SetParameter(hComponentReader, OMX_IndexParamOtherPortFormat, "./cfg/hevc_fhd_inter_8b_11.cfg");

    OMX_SendCommand(hComponentEncoder, OMX_CommandStateSet, OMX_StateIdle, NULL);
    OMX_SendCommand(hComponentFeeder, OMX_CommandStateSet, OMX_StateIdle, NULL);
    OMX_SendCommand(hComponentReader, OMX_CommandStateSet, OMX_StateIdle, NULL);

    OMX_SetupTunnel(hComponentFeeder, 0, hComponentEncoder, 0);
    OMX_SetupTunnel(hComponentEncoder, 0, hComponentReader, 0);

    OMX_SendCommand(hComponentFeeder, OMX_CommandStateSet, OMX_StateExecuting, NULL);
    OMX_SendCommand(hComponentEncoder, OMX_CommandStateSet, OMX_StateExecuting, NULL);
    OMX_SendCommand(hComponentReader, OMX_CommandStateSet, OMX_StateExecuting, NULL);

    block_until_state_changed(hComponentFeeder, OMX_StateIdle);
    block_until_state_changed(hComponentEncoder, OMX_StateIdle);
    block_until_state_changed(hComponentReader, OMX_StateIdle);

    OMX_FreeHandle(hComponentFeeder);
    OMX_FreeHandle(hComponentEncoder);
    OMX_FreeHandle(hComponentReader);
    OMX_Deinit();
    return 0;
}