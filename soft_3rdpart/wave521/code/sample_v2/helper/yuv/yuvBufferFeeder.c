#include <string.h>
#include <stdarg.h>
#include "main_helper.h"

typedef struct {
    Uint8*       pYuv;
    Uint8*       address;
    Uint32       size;
} yuvBufferContext;

BOOL yuvBufferFeeder_Create(
    YuvFeederImpl *impl,
    const char*   path,
    Uint32        packed,
    Uint32        fbStride,
    Uint32        fbHeight)
{
    yuvBufferContext*   ctx;
    Uint8*        pYuv;

    if ( packed == 1 )
        pYuv = osal_malloc(fbStride*fbHeight*3*2*2);//packed, unsigned short
    else
        pYuv = osal_malloc(fbStride*fbHeight*3*2);//unsigned short

    if ((ctx=(yuvBufferContext*)osal_malloc(sizeof(yuvBufferContext))) == NULL) {
        osal_free(pYuv);
        return FALSE;
    }

    osal_memset(ctx, 0, sizeof(yuvBufferContext));

    ctx->pYuv     = pYuv;
    impl->context = ctx;

    return TRUE;
}

BOOL yuvBufferFeeder_Destory(
    YuvFeederImpl *impl
    )
{
    yuvBufferContext*    ctx = (yuvBufferContext*)impl->context;

    osal_free(ctx->pYuv);
    osal_free(ctx);
    return TRUE;
}

BOOL yuvBufferFeeder_Feed(
    YuvFeederImpl*  impl,
    Int32           coreIdx,
    FrameBuffer*    fb,
    size_t          picWidth,
    size_t          picHeight,
    Uint32          srcFbIndex,
    void*           arg
    )
{
    yuvBufferContext* ctx = (yuvBufferContext*)impl->context;
    Uint8*      pYuv = ctx->pYuv;
    size_t      frameSize;
    size_t      frameSizeY;
    size_t      frameSizeC;
    Int32       bitdepth=0;
    Int32       yuv3p4b=0;
    Int32       packedFormat=0;
    Uint32      outWidth=0;
    Uint32      outHeight=0;
    CalcYuvSize(fb->format, picWidth, picHeight, fb->cbcrInterleave, &frameSizeY, &frameSizeC, &frameSize, &bitdepth, &packedFormat, &yuv3p4b);

    // Load source one picture image to encode to SDRAM frame buffer.
    osal_memcpy(pYuv, ctx->address, ctx->size);

    if (fb->mapType == LINEAR_FRAME_MAP ) {
        outWidth  = (yuv3p4b&&packedFormat==0) ? ((picWidth+31)/32)*32  : picWidth;
        outHeight = (yuv3p4b) ? ((picHeight+7)/8)*8 : picHeight;

        if ( yuv3p4b  && packedFormat) {
            outWidth = ((picWidth*2)+2)/3*4;
        }
        else if(packedFormat) {
            outWidth *= 2;           // 8bit packed mode(YUYV) only. (need to add 10bit cases later)
            if (bitdepth != 0)      // 10bit packed
                outWidth *= 2;
        }
        LoadYuvImageByYCbCrLine(impl->handle, coreIdx, pYuv, outWidth, outHeight, fb, srcFbIndex);
        //LoadYuvImageBurstFormat(coreIdx, pYuv, outWidth, outHeight, fb, ctx->srcPlanar);
    }
    else {
        TiledMapConfig  mapConfig;

        osal_memset((void*)&mapConfig, 0x00, sizeof(TiledMapConfig));
        if (arg != NULL) {
            osal_memcpy((void*)&mapConfig, arg, sizeof(TiledMapConfig));
        }

        LoadTiledImageYuvBurst(impl->handle, coreIdx, pYuv, picWidth, picHeight, fb, mapConfig);
    }

    return TRUE;
}

BOOL yuvBufferFeeder_Configure(
    YuvFeederImpl* impl,
    Uint32         cmd,
    YuvInfo        yuv
    )
{
    return TRUE;
}

void yuvBufferFeeder_SetData(YuvFeederImpl*  impl, Uint8* address, Uint32 size)
{
    yuvBufferContext* ctx = (yuvBufferContext*)impl->context;
    ctx->address = address;
    ctx->size = size;
}

YuvFeederImpl yuvBufferFeederImpl = {
    NULL,
    yuvBufferFeeder_Create,
    yuvBufferFeeder_Feed,
    yuvBufferFeeder_Destory,
    yuvBufferFeeder_Configure
};