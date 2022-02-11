// SPDX-License-Identifier: GPL-2.0
/*
 * Copyright (C) 2021 StarFive Technology Co., Ltd.
 */
#ifndef OMX__CORE
#define OMX__CORE

#include "OMX_Types.h"
#include "OMX_Core.h"
#include "OMX_Component.h"
#include "OMX_VideoExt.h"
#include "component.h"
#include "encoder_listener.h"
#include "decoder_listener.h"
#include <sys/time.h>
#include <stdarg.h>
#include <dlfcn.h>
#include <string.h>

#define ENABLE_DEBUG
#ifdef ENABLE_DEBUG
enum
{
    SF_LOG_ERR = 0,
    SF_LOG_WARN,
    SF_LOG_PERF,
    SF_LOG_INFO,
    SF_LOG_DEBUG,
    SF_LOG_ALL,
};
void SF_LogMsg(int level, const char *function, int line, const char *format, ...);
#define LOG(level, ...) SF_LogMsg(level, __FUNCTION__, __LINE__, __VA_ARGS__);
#define FunctionIn() SF_LogMsg(SF_LOG_DEBUG, __FUNCTION__, __LINE__, "FUN IN\r\n");
#define FunctionOut() SF_LogMsg(SF_LOG_DEBUG, __FUNCTION__, __LINE__, "FUN OUT\r\n");
#else
#define FunctionIn()
#define FunctionOut()
#endif
#define VPU_OUTPUT_BUF_NUMBER 10
#define VPU_INPUT_BUF_NUMBER 5

typedef struct _SF_COMPONENT_FUNCTIONS
{
    Component (*ComponentCreate)(const char *componentName, CNMComponentConfig *componentParam);
    BOOL(*ComponentSetupTunnel)(Component fromComponent, Component toComponent);
    ComponentState (*ComponentExecute)(Component component);
    Int32 (*ComponentWait)(Component component);
    void (*ComponentStop)(Component component);
    void (*ComponentRelease)(Component component);
    BOOL(*ComponentChangeState)(Component component, Uint32 state);
    BOOL(*ComponentDestroy)(Component component, BOOL *ret);
    CNMComponentParamRet (*ComponentGetParameter)(Component from, Component to, GetParameterCMD commandType, void *data);
    CNMComponentParamRet (*ComponentSetParameter)(Component from, Component to, SetParameterCMD commandType, void *data);
    void (*ComponentNotifyListeners)(Component component, Uint64 event, void *data);
    BOOL(*ComponentRegisterListener)(Component component, Uint64 events, ComponentListenerFunc func, void *context);
    void (*ComponentPortCreate)(Port *port, Component owner, Uint32 depth, Uint32 size);
    void (*ComponentPortSetData)(Port *port, PortContainer *portData);
    PortContainer *(*ComponentPortPeekData)(Port *port);
    PortContainer *(*ComponentPortGetData)(Port *port);
    void *(*WaitBeforeComponentPortGetData)(Port *port);
    void (*ComponentPortWaitReadyStatus)(Port *port);
    void (*ComponentPortDestroy)(Port *port);
    BOOL(*ComponentPortHasInputData)(Port *port);
    Uint32 (*ComponentPortGetSize)(Port *port);
    void (*ComponentPortFlush)(Component component);
    ComponentState (*ComponentGetState)(Component component);
    BOOL(*ComponentParamReturnTest)(CNMComponentParamRet ret, BOOL *retry);
    // Listener
    BOOL(*SetupDecListenerContext)(DecListenerContext *ctx, CNMComponentConfig *config, Component renderer);
    BOOL(*SetupEncListenerContext)(EncListenerContext *ctx, CNMComponentConfig *config);
    void (*ClearDecListenerContext)(DecListenerContext *ctx);
    void (*HandleDecCompleteSeqEvent)(Component com, CNMComListenerDecCompleteSeq *param, DecListenerContext *ctx);
    void (*HandleDecRegisterFbEvent)(Component com, CNMComListenerDecRegisterFb *param, DecListenerContext *ctx);
    void (*HandleDecGetOutputEvent)(Component com, CNMComListenerDecDone *param, DecListenerContext *ctx);
    void (*HandleDecCloseEvent)(Component com, CNMComListenerDecClose *param, DecListenerContext *ctx);
    void (*ClearEncListenerContext)(EncListenerContext *ctx);
    void (*HandleEncFullEvent)(Component com, CNMComListenerEncFull *param, EncListenerContext *ctx);
    void (*HandleEncGetOutputEvent)(Component com, CNMComListenerEncDone *param, EncListenerContext *ctx);
    void (*HandleEncCompleteSeqEvent)(Component com, CNMComListenerEncCompleteSeq *param, EncListenerContext *ctx);
    void (*HandleEncGetEncCloseEvent)(Component com, CNMComListenerEncClose *param, EncListenerContext *ctx);
    void (*EncoderListener)(Component com, Uint64 event, void *data, void *context);
    void (*DecoderListener)(Component com, Uint64 event, void *data, void *context);
    // Helper
    void (*SetDefaultEncTestConfig)(TestEncConfig *testConfig);
    void (*SetDefaultDecTestConfig)(TestDecConfig *testConfig);
    Int32 (*LoadFirmware)(Int32 productId, Uint8 **retFirmware, Uint32 *retSizeInWord, const char *path);
    BOOL(*SetupEncoderOpenParam)(EncOpenParam *param, TestEncConfig *config, ENC_CFG *encCfg);
    RetCode (*SetUpDecoderOpenParam)(DecOpenParam *param, TestDecConfig *config);
    // VPU
    int (*VPU_GetProductId)(int coreIdx);
    BOOL(*Queue_Enqueue)(Queue *queue, void *data);
    Uint32 (*Queue_Get_Cnt)(Queue *queue);
    RetCode (*VPU_DecClrDispFlag)(DecHandle handle, int index);
    RetCode (*VPU_DecGetFrameBuffer)(DecHandle handle, int frameIdx, FrameBuffer* frameBuf);
    void (*Render_DecClrDispFlag)(void *context, int index);
    // VPU Log
    int (*InitLog)(void);
    void (*DeInitLog)(void);
    void (*SetMaxLogLevel)(int level);
    int (*GetMaxLogLevel)(void);
    // FrameBuffer
    void* (*AllocateFrameBuffer2)(ComponentImpl* com, Uint32 size);
    BOOL (*AttachDMABuffer)(ComponentImpl* com, Uint64 virtAddress, Uint32 size);
    void (*SetRenderTotalBufferNumber)(ComponentImpl* com, Uint32 number);
    void (*WaitForExecoderReady)(ComponentImpl *com)
} SF_COMPONENT_FUNCTIONS;

typedef struct _SF_OMX_COMPONENT
{
    OMX_STRING componentName;
    OMX_STRING libName;
    OMX_COMPONENTTYPE *pOMXComponent;
    OMX_ERRORTYPE (*SF_OMX_ComponentConstructor)(struct _SF_OMX_COMPONENT *hComponent);
    OMX_ERRORTYPE (*SF_OMX_ComponentClear)(struct _SF_OMX_COMPONENT *hComponent);
    SF_COMPONENT_FUNCTIONS *functions;
    Component *hSFComponentExecoder;
    Component *hSFComponentFeeder;
    Component *hSFComponentRender;
    void *testConfig;
    CNMComponentConfig *config;
    void *lsnCtx;
    OMX_PTR soHandle;
    Uint16 *pusBitCode;
    OMX_CALLBACKTYPE *callbacks;
    OMX_PTR pAppData;
    OMX_PARAM_PORTDEFINITIONTYPE portDefinition[2];
    OMX_VIDEO_PARAM_AVCTYPE AVCComponent[2];
    OMX_VIDEO_PARAM_HEVCTYPE HEVCComponent[2];
    OMX_BUFFERHEADERTYPE *pBufferArray[64];
    CodStd bitFormat;
    OMX_STRING fwPath;
    OMX_STRING componentRule;
    OMX_STATETYPE nextState;
    OMX_BOOL memory_optimization;
} SF_OMX_COMPONENT;

typedef struct _SF_PORT_PRIVATE
{
    OMX_U32 nPortnumber;
} SF_PORT_PRIVATE;

#define PRINT_STUCT(a, b)                                   \
    do                                                      \
    {                                                       \
        printf("size = %d\r\n", sizeof(b));                 \
        for (int i = 0; i < sizeof(b); i += sizeof(void *)) \
        {                                                   \
            for (int j = 0; j < sizeof(void *); j++)        \
            {                                               \
                printf("%02X ", *((char *)a + i + j));      \
            }                                               \
            for (int j = 0; j < sizeof(void *); j++)        \
            {                                               \
                printf("%c", *((char *)a + i + j));         \
            }                                               \
            printf("\r\n");                                 \
        }                                                   \
    } while (0)

#ifdef __cplusplus
extern "C"
{
#endif

    void sf_get_component_functions(SF_COMPONENT_FUNCTIONS *funcs, OMX_PTR *sohandle);
    SF_OMX_COMPONENT *GetSFOMXComponrntByComponent(Component *pComponent);
    OMX_BUFFERHEADERTYPE *GetOMXBufferByAddr(SF_OMX_COMPONENT *pSfOMXComponent, OMX_U8 *pAddr);
    OMX_ERRORTYPE StoreOMXBuffer(SF_OMX_COMPONENT *pSfOMXComponent, OMX_BUFFERHEADERTYPE *pBuffer);
    OMX_ERRORTYPE ClearOMXBuffer(SF_OMX_COMPONENT *pSfOMXComponent, OMX_BUFFERHEADERTYPE *pBuffer);
    OMX_U32 GetOMXBufferCount(SF_OMX_COMPONENT *pSfOMXComponent);
#ifdef __cplusplus
}
#endif

#endif
