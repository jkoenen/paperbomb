#ifndef RENDERER_H_INCLUDED
#define RENDERER_H_INCLUDED

#include "types.h"
#include "matrix.h"

typedef struct
{
    uint    dummy;
} FrameData;

typedef enum
{
    Pen_Default,
    Pen_Font,
    Pen_Fat,
    Pen_Count
} Pen;

void renderer_init();
void renderer_done();

void renderer_setDrawSpeed( float speed );      // 0.0 -> slowest allowed speed (seconds for a page), 1.0f -> one full page per frame
void renderer_setPen( Pen pen );
void renderer_setVariance( float variance );
void renderer_setTransform( const float2x3* pTransform );

void renderer_flipPage();

void renderer_addLinearStroke( const float2* pPoints, uint pointCount );

void renderer_addQuadraticStroke( const float2* pPoints, uint pointCount );

void renderer_updatePage( float timeStep );
int renderer_isPageDone();

void renderer_drawFrame( const FrameData* pFrame );

#endif

