#ifndef GRAPHICS_RENDERER_H_INCLUDED
#define GRAPHICS_RENDERER_H_INCLUDED

#include "types.h"
#include "matrix.h"

typedef struct
{
    uint    dummy;
} FrameData;

typedef struct
{
    const float2*   pPoints;
    uint            pointCount;
} StrokeDefinition;

typedef enum
{
    Pen_Default,
    Pen_Font,
    Pen_Count
} Pen;

void renderer_init();
void renderer_done();

void renderer_setDrawSpeed( float speed );      // 0.0 -> slowest allowed speed (seconds for a page), 1.0f -> one full page per frame

void renderer_flipPage();
void renderer_addStroke( const StrokeDefinition* pDefinition, Pen pen, const float2x3* pTransform, float variance );

void renderer_updatePage( float timeStep );
int renderer_isPageDone();

void renderer_drawFrame( const FrameData* pFrame );

#endif

