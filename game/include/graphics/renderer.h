#ifndef GRAPHICS_RENDERER_H_INCLUDED
#define GRAPHICS_RENDERER_H_INCLUDED

#include "sys/types.h"

typedef struct
{
    float       time;
    float2      playerPos;
	float		playerDir;
} FrameData;

void renderer_init();
void renderer_done();

void renderer_drawFrame( const FrameData* pFrame );

#endif

