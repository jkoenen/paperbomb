#ifndef RENDERER_H_INCLUDED
#define RENDERER_H_INCLUDED

#include "types.h"
#include "matrix.h"
#include "geometry.h"

typedef struct
{
    uint    dummy;
} FrameData;

typedef enum
{
    Pen_Default,
    Pen_Font,
    Pen_Fat,

    Pen_PageNumber,
    
    Pen_DebugRed,
    Pen_DebugGreen,
    Pen_Count
} Pen;

typedef enum
{
    DrawCommand_Move,
    DrawCommand_Line,
    DrawCommand_Curve,
    DrawCommand_Count
} DrawCommand;


void renderer_init();
void renderer_done();

void renderer_setDrawSpeed( float speed );      // 0.0 -> slowest allowed speed (seconds for a page), 1.0f -> one full page per frame
void renderer_setPen( Pen pen );
void renderer_setVariance( float variance );
void renderer_setTransform( const float2x3* pTransform );

void renderer_addBurnHole( const float2* pStart, const float2* pEnd, float size );

void renderer_flipPage();
void renderer_flush();

void renderer_addLinearStroke( const float2* pPoints, uint pointCount );
void renderer_addQuadraticStroke( const float2* pPoints, uint pointCount );
void renderer_addCircle( const Circle* pCircle );

void renderer_addCommandStroke( const float2* pPoints, const uint8* pCommands, uint commandCount );

void renderer_updatePage( float timeStep );
int renderer_isPageDone();

void renderer_drawFrame( const FrameData* pFrame );

void renderer_drawCircle(const float2* pPos, float radius,const float3* pColor);

#endif

