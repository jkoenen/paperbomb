#ifndef GRAPHICS_RENDERTARGET_H_INCLUDED
#define GRAPHICS_RENDERTARGET_H_INCLUDED

#include "types.h"

typedef enum 
{
    PixelFormat_R8G8B8A8,
    PixelFormat_Count
} PixelFormat;

typedef struct
{
    uint    id;
    uint	colorBuffer0;
    int     width;
    int     height;
    int     format;
} RenderTarget;

int rendertarget_create( RenderTarget* pTarget, int width, int height, PixelFormat format );
void rendertarget_activate( const RenderTarget* pTarget );

#endif

