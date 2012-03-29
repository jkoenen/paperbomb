#ifndef GRAPHICS_RENDERTARGET_H_INCLUDED
#define GRAPHICS_RENDERTARGET_H_INCLUDED

#include <GL/gl.h>
#include <GL/glext.h>

typedef struct
{
    GLuint  id;
    GLuint  colorBuffer0;
    int     width;
    int     height;
    int     format;
} RenderTarget;

int rendertarget_create( RenderTarget* pTarget, int width, int height, int format );
void rendertarget_set( const RenderTarget* pTarget );

#endif

