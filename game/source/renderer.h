#ifndef GRAPHICS_RENDERER_H_INCLUDED
#define GRAPHICS_RENDERER_H_INCLUDED

#include "types.h"

typedef struct
{
} FrameData;

typedef struct
{
    const float2*   pPoints;
    uint            pointCount;
} StrokeDefinition;

void renderer_init();
void renderer_done();

void renderer_startPageFlip( float duration );
int renderer_advancePageFlip( float timeStep );
void renderer_flipPage();

void renderer_startStroke( const StrokeDefinition* pDefinition, const float2* pPositionOnPage, float speed, float size, float width, float variance );
int renderer_advanceStroke( float timeStep );
void renderer_drawStroke( const StrokeDefinition* pDefinition, const float2* pPositionOnPage, float size, float width, float variance );

void renderer_drawFrame( const FrameData* pFrame );

#endif

