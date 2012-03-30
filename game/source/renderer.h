#ifndef GRAPHICS_RENDERER_H_INCLUDED
#define GRAPHICS_RENDERER_H_INCLUDED

#include "types.h"

typedef struct
{
    float       time;
    float2      playerPos;
} FrameData;

void renderer_init();
void renderer_done();

void renderer_drawFrame( const FrameData* pFrame );

#endif

