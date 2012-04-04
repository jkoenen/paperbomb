#ifndef GRAPHICS_H_INCLUDED
#define GRAPHICS_H_INCLUDED

void graphics_init();
void graphics_done();

typedef enum
{
    BlendMode_Disabled,
    BlendMode_Over,
    BlendMode_Count
} BlendMode;

typedef enum
{
    VertexFormat_None,
    VertexFormat_2d,
    VertexFormat_Count
} VertexFormat;

void graphics_setVertexFormat( VertexFormat format );
void graphics_setBlendMode( BlendMode mode );


#endif

