#ifndef GRAPHICS_RENDERTARGET_H_INCLUDED
#define GRAPHICS_RENDERTARGET_H_INCLUDED

#include "types.h"
#include "opengl.h"

typedef struct
{
    GLuint  id;
    GLuint		colorBuffer0;
    int     width;
    int     height;
    int     format;
} RenderTarget;

int rendertarget_create( RenderTarget* pTarget, int width, int height, int format );
void rendertarget_activate( const RenderTarget* pTarget );

#endif

