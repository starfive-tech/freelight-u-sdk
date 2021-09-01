// SPDX-License-Identifier: GPL-2.0
/*
 * Copyright (C) 2021 StarFive Technology Co., Ltd.
 */
#include <stdio.h>
#include <signal.h>
#include <OMX_Core.h>
#include <OMX_Component.h>
#include <OMX_Video.h>
#include <OMX_IndexExt.h>
#include <sys/time.h>

#define OMX_INIT_STRUCTURE(a)         \
    memset(&(a), 0, sizeof(a));       \
    (a).nSize = sizeof(a);            \
    (a).nVersion.nVersion = 1;        \
    (a).nVersion.s.nVersionMajor = 1; \
    (a).nVersion.s.nVersionMinor = 1; \
    (a).nVersion.s.nRevision = 1;     \
    (a).nVersion.s.nStep = 1

OMX_BUFFERHEADERTYPE *pInputBuffer, *pOutputBuffer;
static int g_is_run  = 0;
static int numBuffer = 0;
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
    struct timeval tv;
    gettimeofday(&tv,NULL);
    printf("[%ld], frame = %d\n",tv.tv_sec*1000000 + tv.tv_usec, numBuffer);  //微秒
    numBuffer ++;
    //write to file
    FILE *fb = fopen("./out.dat", "ab+");
    fwrite(pBuffer->pBuffer, 1, pBuffer->nFilledLen, fb);
    fclose(fb);
    OMX_FillThisBuffer(hComponent, pOutputBuffer);
    return OMX_ErrorNone;
}

static OMX_ERRORTYPE empty_buffer_done_handler(
    OMX_HANDLETYPE hComponent,
    OMX_PTR pAppData,
    OMX_BUFFERHEADERTYPE *pBuffer)
{
    printf("%s:%d:!!!!!!!!!!!!!!!!!!\r\n",__FUNCTION__, __LINE__);
    //write to file
    return OMX_ErrorNone;
}

static void block_until_state_changed(OMX_HANDLETYPE hComponent, OMX_STATETYPE wanted_eState)
{
    OMX_STATETYPE eState;
    while (eState != wanted_eState && g_is_run)
    {
        OMX_GetState(hComponent, &eState);
        if (eState != wanted_eState)
        {
            printf("state = %d\r\n", eState);
            usleep(10000 * 1000);
        }
    }
}

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
    OMX_HANDLETYPE hComponentDecoder;

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
    callbacks.EmptyBufferDone = empty_buffer_done_handler;
    OMX_GetHandle(&hComponentDecoder, "sf.dec.decoder", NULL, &callbacks);

    if (hComponentDecoder == NULL)
    {
        printf("could not get handle\r\n");
        return 0;
    }

    OMX_SetParameter(hComponentDecoder, OMX_IndexParamVideoHevc, NULL);

    OMX_SendCommand(hComponentDecoder, OMX_CommandStateSet, OMX_StateIdle, NULL);

    FILE *fb = fopen("./stream/hevc/akiyo.cfg_ramain_tv0.cfg.265", "r");
    if (fb == NULL)
    {
        printf("%s:%d can not open file\r\n", __FUNCTION__, __LINE__);
    }
    int curpos = ftell(fb);
    fseek(fb,0L,SEEK_END);
    int size = ftell(fb);
    fseek(fb,curpos,SEEK_SET);
    printf("%s:%d size = %d\r\n", __FUNCTION__, __LINE__, size);
    
    OMX_AllocateBuffer(hComponentDecoder, &pInputBuffer, 0, NULL, size);
    OMX_AllocateBuffer(hComponentDecoder, &pOutputBuffer, 0, NULL, 0x20000);

    int len = fread(pInputBuffer->pBuffer, 1, size, fb);
    printf("%s:%d:size = %d\r\n",__FUNCTION__, __LINE__, len);
    
    OMX_SendCommand(hComponentDecoder, OMX_CommandStateSet, OMX_StateExecuting, NULL);
    OMX_EmptyThisBuffer(hComponentDecoder, pInputBuffer);
    OMX_FillThisBuffer(hComponentDecoder, pOutputBuffer);
    // OMX_FillThisBuffer(hComponentDecoder, pOutputBuffer);

    block_until_state_changed(hComponentDecoder, OMX_StateIdle);

    OMX_FreeHandle(hComponentDecoder);
    return 0;
}
