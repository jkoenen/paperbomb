#include "renderer.h"
#include "types.h"
#include "platform.h"
#include "debug.h"
#include "vector.h"

#include "shader.h"
#include "graphics.h"
#include "rendertarget.h"

#include "paper_glsl.h"
#include "pen_glsl.h"
#include "page_glsl.h"
#include "pageflip_glsl.h"
#include "burnhole_glsl.h"
#include "noise_glsl.h"

#ifndef SYS_BUILD_MASTER
#   include "debugpen_glsl.h"
#endif

#include <math.h>

enum 
{
    MaxPointCount = 1024u,
    MaxCommandCount = 256u,
    MaxBurnHoleCount = 32u
};

typedef struct
{
    uint    dummy;
} Mesh;

typedef struct 
{
    float           progress;           // current position [0..length]
    uint            activeSegment;
    float           segmentProgress;    // 0..length of active segment
    float           length;
} Stroke;

typedef struct 
{
    float           width;
    float           pressure;
    float3          color;
} PenDefinition;

typedef struct
{
    float2  start;
    float2  end;
    float   size;
    float   initialSize;
    float   rot;
} BurnHole;

typedef enum 
{
    StrokeCommandType_Draw,
    StrokeCommandType_Delay,
    StrokeCommandType_Count
} StrokeCommandType;

typedef struct
{
    uint        pointIndex;
    uint        pointCount;
    uint        penId;
    float       variance;
} StrokeDrawCommandData;

typedef struct
{
    float       time;
} StrokeDelayCommandData;

typedef union
{
    StrokeDrawCommandData   draw;
    StrokeDelayCommandData  delay;
} StrokeCommandData;

typedef struct
{
    StrokeCommandType   type;
    StrokeCommandData   data;
} StrokeCommand;

typedef struct
{
    float2          points[ MaxPointCount ];
    float2          pointNormals[ MaxPointCount ];
    uint            pointCount;
    StrokeCommand   commands[ MaxCommandCount ];
    uint            commandCount;
} StrokeBuffer;

typedef struct
{
    RenderTarget    bgTarget;
    RenderTarget    fgTarget;
    RenderTarget    burnTarget;
} Page;

typedef enum
{
    PageState_BeforeDraw,
    PageState_Draw,
    PageState_AfterDraw,
    PageState_Done,
    PageState_Count
} PageState;

typedef struct 
{
    Shader          paperShader;
    Shader          penShader;
    Shader          pageShader;
    Shader          pageFlipShader;
    Shader          burnHoleShader;
    Shader          noiseShader;
#ifndef SYS_BUILD_MASTER
    Shader          debugPenShader;
#endif

    RenderTarget    noiseTarget;
    
    Page            pages[ 2u ];    
    uint            currentPage;
    uint            lastPage;

    PageState       pageState;
    float           stateTime;

    float           flipTime;

    uint            currentCommand;
    float           currentCommandTime;
    Stroke          currentStroke;
    StrokeBuffer    strokeBuffer;

    PenDefinition   pens[ Pen_Count ];
    BurnHole        burnHoles[ MaxBurnHoleCount ];

    Mesh2d          pageFlipMesh;

    float           strokeDrawSpeed;
    float           delayAfterFlip;
    float           delayAfterDraw;
    float           flipDuration;

    Pen             currentPen;
    float           currentVariance;
    float2x3        currentTransform;
} Renderer;

static Renderer s_renderer;

static void setPageState( PageState newState )
{
    if( s_renderer.pageState != newState )
    {
        //SYS_TRACE_DEBUG( "page state switched from %i to %i!\n", s_renderer.pageState, newState );
        s_renderer.pageState = newState;
        s_renderer.stateTime = 0.0f;
    }
}

static void page_create( Page* pPage, int width, int height )
{
    SYS_VERIFY( rendertarget_create( &pPage->bgTarget, width, height, PixelFormat_R8G8B8A8 ) );
    SYS_VERIFY( rendertarget_create( &pPage->fgTarget, width, height, PixelFormat_R8G8B8A8 ) );
    SYS_VERIFY( rendertarget_create( &pPage->burnTarget, width, height, PixelFormat_R8G8B8A8 ) );

    // render into render target:
    graphics_setRenderTarget( &pPage->bgTarget );   
    graphics_setBlendMode( BlendMode_Disabled );

    // render paper:
    graphics_setShader( &s_renderer.paperShader );    
    graphics_setFp4f( 0u, float_rand(), float_rand(), 2.0f / width, 2.0f / height );
    graphics_setVp4f( 0u, 32.0f, 18.0f, 0.4f, 0.3f );    
    graphics_setFsTexture(0,s_renderer.noiseTarget.id,SamplerState_MirrorU_MirrorV_Bilinear);
    graphics_drawFullscreenQuad();
   
    // clear fg+burn target:
    graphics_setShader( 0 );

    graphics_setRenderTarget( &pPage->fgTarget );
    graphics_clear( 0.0f, 0.0f, 0.0f, 0.0f );

    graphics_setRenderTarget( &pPage->burnTarget );
    graphics_clear( 0.0f, 0.0f, 0.0f, 0.0f );
}

static void clearStrokeBuffer( StrokeBuffer* pBuffer )
{
    pBuffer->pointCount = 0u;
    pBuffer->commandCount = 0u;
}

static void createPen( PenDefinition* pPen, float width, float pressure, const float3* pColor )
{
    pPen->width = width;
    pPen->pressure = pressure;
    pPen->color = *pColor;
}

static void createPageFlipMesh( Mesh2d* pMesh, uint meshWidth, uint meshHeight )
{
    const uint meshVertexCount=(meshWidth+1u)*(meshHeight+1u);
    const uint meshIndexCount=2u*3u*meshWidth*meshHeight;
    graphics_createMesh2d( pMesh, meshVertexCount, meshIndexCount );
    Mesh2dLock meshLock;
    SYS_VERIFY( graphics_lockMesh2d( &meshLock, pMesh ) );

    // fill vertices:
    const float xScale = 1.0f/(float)(meshWidth);
    const float yScale = 1.0f/(float)(meshHeight);
    Vertex2d* pVertex = meshLock.pVertexData;
    for( uint y = 0u; y < meshHeight + 1u; ++y )
    {
        for( uint x = 0u; x < meshWidth + 1u; ++x )
        {
            const float u = (float)x*xScale;
            const float v = (float)y*yScale;

            float2_set( &pVertex->pos, 2.0f * u - 1.0f, -2.0f * v + 1.0f );
            float2_set( &pVertex->texCoord, u, v );
            pVertex++;
        }
    }

    // fill indices:
    Index* pIndex = meshLock.pIndexData;
    for( uint y = 0u; y < meshHeight; ++y )
    {
        for( uint x = 0u; x < meshWidth; ++x )
        {
            const Index i0 = (Index)(y*(meshWidth+1u)+x);
            const Index i1 = i0 + 1u;
            const Index i2 = (Index)(i0+(meshWidth+1u));
            const Index i3 = i2 + 1u;
            *pIndex++ = i0;
            *pIndex++ = i2;
            *pIndex++ = i1;
            *pIndex++ = i2;
            *pIndex++ = i3;
            *pIndex++ = i1;
        }
    }

    graphics_unlockMesh2d( pMesh );    
}

void renderer_init()
{
    SYS_VERIFY( shader_create( &s_renderer.paperShader, &s_shader_paper, 1u, 1u, 0u ) );
    SYS_VERIFY( shader_create( &s_renderer.penShader, &s_shader_pen, 0u, 2u, 0u ) );
    SYS_VERIFY( shader_create( &s_renderer.pageShader, &s_shader_page, 0u, 0u, 3u ) );
    SYS_VERIFY( shader_create( &s_renderer.pageFlipShader, &s_shader_pageflip, 1u, 0u, 2u ) );
    SYS_VERIFY( shader_create( &s_renderer.burnHoleShader, &s_shader_burnhole, 0u, 2u, 1u ) );
    SYS_VERIFY( shader_create( &s_renderer.noiseShader, &s_shader_noise, 0u, 0u, 0u ) );
#ifndef SYS_BUILD_MASTER
    SYS_VERIFY( shader_create( &s_renderer.debugPenShader, &s_shader_debugpen, 1u, 0u, 0u ) );
#endif

    SYS_VERIFY( rendertarget_create( &s_renderer.noiseTarget, 512u, 512u, PixelFormat_R8G8B8A8 ) );
    
    graphics_setBlendMode( BlendMode_Disabled );

    // fill noise map:
    graphics_setRenderTarget( &s_renderer.noiseTarget );
    graphics_setShader( &s_renderer.noiseShader );
    graphics_drawFullscreenQuad();
    
    const int width = sys_getScreenWidth();
    const int height = sys_getScreenHeight();

    // create page:
    for( uint i = 0u; i < SYS_COUNTOF( s_renderer.pages ); ++i )
    {
        page_create( &s_renderer.pages[ i ], width, height );
    }

    s_renderer.currentPage = 0u;
    s_renderer.lastPage = 1u;
    
    s_renderer.currentCommand = 0u;

    s_renderer.pageState = PageState_Done;
    s_renderer.stateTime = 0.0f;

    clearStrokeBuffer( &s_renderer.strokeBuffer );

    float3 color;
    color.x = 0.2f;
    color.y = 0.2f;
    color.z = 0.2f;
    createPen( &s_renderer.pens[ Pen_Default ], 2.0f, 1.0f, &color );
    createPen( &s_renderer.pens[ Pen_Font ], 5.0f, 1.0f, &color );
    createPen( &s_renderer.pens[ Pen_Fat ], 20.0f, 1.0f, &color );
    createPen( &s_renderer.pens[ Pen_DebugRed ], 2.0f, 1.0f, float3_set( &color, 1.0f, 0.0f, 0.0f ) );
    createPen( &s_renderer.pens[ Pen_DebugGreen ], 2.0f, 1.0f, float3_set( &color, 0.0f, 1.0f, 0.0f ) );

    // create page flip mesh:
    createPageFlipMesh( &s_renderer.pageFlipMesh, 64u, 36u );

    s_renderer.flipTime = -1.0f;

    for( uint i = 0u; i < SYS_COUNTOF(s_renderer.burnHoles); ++i )
    {
        s_renderer.burnHoles[i].size = -1.0f;
    }

    renderer_setDrawSpeed( 0.5f );
    renderer_setPen( Pen_Default );
    renderer_setTransform( 0 );
}

void renderer_done()
{
    // :TODO:
}

static float getDelayValue( const float2* pKeys, float maxValue, float x )
{
    const float x0 = pKeys->x;
    const float x1 = pKeys->y;
    SYS_ASSERT( x0 < x1 );
    const float m = -1.0f / ( x1 - x0 );
    const float y0 = 1.0f - m * x0;

    return maxValue * float_saturate( y0 + m * x );
}

void renderer_setDrawSpeed( float speed )
{
    speed = float_saturate( speed );

    const float maxStrokeDrawSpeed = 10000.0f;    // units per second..
    const float maxDelayAfterFlip = 1.0f; 
    const float maxDelayAfterDraw = 3.0f;
    const float maxFlipDuration = 2.5f;

    const float2 strokeDrawSpeedKeys = { 0.2f, 0.8f };
    const float2 afterFlipDelayKeys = { 0.2f, 0.8f };
    const float2 afterDrawDelayKeys = { 0.4f, 0.9f };
    const float2 flipDurationKeys = { 0.5f, 0.95f };
   
    if( speed >= 0.99f )
    {
        // don't wait:
        s_renderer.strokeDrawSpeed = 0.0f; 
        s_renderer.delayAfterFlip = 0.0f;
        s_renderer.delayAfterDraw = 0.0f;
        s_renderer.flipDuration = 0.0f;
    }
    else
    {
        s_renderer.strokeDrawSpeed = getDelayValue( &strokeDrawSpeedKeys, maxStrokeDrawSpeed, speed );
        s_renderer.delayAfterFlip = getDelayValue( &afterFlipDelayKeys, maxDelayAfterFlip, speed );
        s_renderer.delayAfterDraw = getDelayValue( &afterDrawDelayKeys, maxDelayAfterDraw, speed );
        s_renderer.flipDuration = getDelayValue( &flipDurationKeys, maxFlipDuration, speed );
    } 

    SYS_TRACE_DEBUG( "speed=%f delays=(%f,%f,%f)\n", 
        s_renderer.strokeDrawSpeed,
        s_renderer.delayAfterFlip,
        s_renderer.delayAfterDraw,
        s_renderer.flipDuration );
}

void renderer_setPen( Pen pen )
{
    SYS_ASSERT( pen < Pen_Count );
    s_renderer.currentPen = pen;
}

void renderer_setVariance( float variance )
{
    s_renderer.currentVariance = variance;
}

void renderer_setTransform( const float2x3* pTransform )
{
    if( pTransform )
    {
        s_renderer.currentTransform = *pTransform;
    }
    else
    {
        float2x2_identity( &s_renderer.currentTransform.rot );
        float2_set( &s_renderer.currentTransform.pos, 0.0f, 0.0f );
    }
}

static void drawBurnHole( const BurnHole* pBurnHole )
{
    const float size=pBurnHole->size;
    if( size <= 0.0f )
    {
        return;
    }
    
    Page* pPage = &s_renderer.pages[ s_renderer.currentPage ];
    graphics_setRenderTarget( &pPage->burnTarget );
    graphics_setShader( &s_renderer.burnHoleShader );
    graphics_setBlendMode( BlendMode_Over );
    graphics_setFsTexture( 0, s_renderer.noiseTarget.id, SamplerState_MirrorU_MirrorV_Bilinear );

    const float initialSize=pBurnHole->initialSize;
    const float rot=pBurnHole->rot;
    const float2 start=pBurnHole->start;
    const float2 end=pBurnHole->end;

    const float len=float2_distance(&start,&end);
    const float s=initialSize;
    const float us=(len+2.0f*s)/10.0f;
    const float vs=2.0f*s/10.0f;
    
    graphics_setFp4f(0u,start.x,start.y,end.x,end.y);
    graphics_setFp4f(1u,size,0.0f,0.0f,0.0f);

    float2 dir;
    float2_normalize(float2_sub(&dir, &end, &start));

    float2 normal;
    float2_perpendicular(&normal, &dir);
    
    float2 v[4u];
    float2_addScaled1f(&v[0u], &start, &normal,  s);
    float2_addScaled1f(&v[1u], &start, &normal, -s);
    float2_addScaled1f(&v[2u], &end, &normal, -s);
    float2_addScaled1f(&v[3u], &end, &normal,  s);

    float2_addScaled1f(&v[0u], &v[0u], &dir, -s);
    float2_addScaled1f(&v[1u], &v[1u], &dir, -s);
    float2_addScaled1f(&v[2u], &v[2u], &dir, s);
    float2_addScaled1f(&v[3u], &v[3u], &dir, s);

    float2 uv0, uv1;
    float2_set(&uv0,0.0f,0.0f);
    float2_set(&uv1,us,vs);
    float2x2 rotM;
    float2x2_rotationY(&rotM,rot);
/*    float2x2_transform(&uv0,&rotM,&uv0);
    float2x2_transform(&uv1,&rotM,&uv1);*/
    graphics_drawQuad(v,uv0.x,uv0.y,uv1.x,uv1.y);
}

static void renderer_updatePageFlip( float timeStep )
{
    if( s_renderer.flipTime < 0.0f || s_renderer.flipDuration <= 0.0f )
    {
        return;
    }

    s_renderer.flipTime += timeStep;
    
    const float flipProgress = float_saturate( s_renderer.flipTime / s_renderer.flipDuration );
    if( flipProgress >= 1.0f )
    {
        s_renderer.flipTime = -1.0f;
    }
}

void renderer_flipPage()
{
    // 
    if( s_renderer.flipTime >= 0.0f )
    {
        //SYS_TRACE_WARNING( "starting page flip while another flip is still active!\n" ); 
    }
    
    s_renderer.lastPage = s_renderer.currentPage;
    s_renderer.currentPage = 1 - s_renderer.currentPage;

    //s_renderer.currentStroke.pDefinition = 0;
    s_renderer.currentCommand = 0u;
    clearStrokeBuffer( &s_renderer.strokeBuffer );

    setPageState( PageState_BeforeDraw );

    Page* pPage = &s_renderer.pages[ s_renderer.currentPage ];
    
    // clear current page:
    graphics_setRenderTarget( &pPage->fgTarget );
    graphics_setBlendMode( BlendMode_Disabled );
    graphics_setShader( 0 );

    // 
    graphics_clear( 0.0f, 0.0f, 0.0f, 0.0f );

    graphics_setRenderTarget( &pPage->burnTarget );
    graphics_clear( 0.0f, 0.0f, 0.0f, 0.0f );

    for( uint i = 0u; i < SYS_COUNTOF(s_renderer.burnHoles); ++i )
    {
        if( s_renderer.burnHoles[i].size>=0.0f)
        {
//            SYS_TRACE_DEBUG("bh[%i]=%f\n", i, s_renderer.burnHoles[i].size);
            drawBurnHole( &s_renderer.burnHoles[i] );
            s_renderer.burnHoles[i].size -= 0.1f;
        }
    }

    if( s_renderer.flipDuration > 0.0f )
    {
        s_renderer.flipTime = 0.0f;
    }
    else
    {
        // flip instantly:
        s_renderer.flipTime = -1.0f;
    }
}

static void transformPoint( float2* pTarget, const float2* pSource, float variance )
{
    const float2x3* pTransform = &s_renderer.currentTransform;

    float2 offset;
    float2_rand_normal( &offset, 0.0f, variance ); 

    float2 point;
    float2x3_transform( &point, pTransform, pSource );
    float2_add( pTarget, &point, &offset );    
}

void renderer_addBurnHole( const float2* pStart, const float2* pEnd, float size )
{
    for( uint i = 0u; i < SYS_COUNTOF(s_renderer.burnHoles); ++i )
    {
        if( s_renderer.burnHoles[i].size <= 0.0f )
        {
            s_renderer.burnHoles[i].size=size;
            s_renderer.burnHoles[i].initialSize=size;
            transformPoint( &s_renderer.burnHoles[i].start, pStart, 0.0f );
            transformPoint( &s_renderer.burnHoles[i].end, pEnd, 0.0f );
            s_renderer.burnHoles[i].rot=float_rand_range(0.0f, 2.0f*PI);
            drawBurnHole( &s_renderer.burnHoles[i] );
            return;
        }
    }

    SYS_TRACE_WARNING( "no burn hole slot found!\n" );
}

static int pushStrokeCommand( const StrokeCommand* pCommand )
{
    if( s_renderer.strokeBuffer.commandCount >= MaxCommandCount )
    {
        SYS_TRACE_DEBUG( "stroke command buffer is full!\n" );
        return FALSE;
    }
    if( pCommand->data.draw.pointCount < 2u ) 
    {
        return FALSE;
    }

    s_renderer.strokeBuffer.commands[ s_renderer.strokeBuffer.commandCount ] = *pCommand;
    s_renderer.strokeBuffer.commandCount++;
    return TRUE;
}

static int pushStrokePointWithNormal( const float2* pPoint, const float2* pNormal )
{
    if( s_renderer.strokeBuffer.pointCount >= MaxPointCount )
    {
        SYS_TRACE_WARNING( "stroke point buffer is full!\n" );
        return FALSE;
    }
    s_renderer.strokeBuffer.points[ s_renderer.strokeBuffer.pointCount ] = *pPoint;
    s_renderer.strokeBuffer.pointNormals[ s_renderer.strokeBuffer.pointCount ] = *pNormal;
    s_renderer.strokeBuffer.pointCount++;
    return TRUE;
}

static int pushStrokePoint( const float2* pPoint )
{
    if( s_renderer.strokeBuffer.pointCount >= MaxPointCount )
    {
        SYS_TRACE_WARNING( "stroke point buffer is full!\n" );
        return FALSE;
    }
    s_renderer.strokeBuffer.points[ s_renderer.strokeBuffer.pointCount ] = *pPoint;
    float2_set( &s_renderer.strokeBuffer.pointNormals[ s_renderer.strokeBuffer.pointCount ], 1.0f, 0.0f );
    s_renderer.strokeBuffer.pointCount++;
    return TRUE;
}

static void createDrawCommand( StrokeCommand* pCommand )
{
    pCommand->type = StrokeCommandType_Draw;
    pCommand->data.draw.pointIndex = s_renderer.strokeBuffer.pointCount;
    pCommand->data.draw.pointCount = 0u;
    pCommand->data.draw.penId = s_renderer.currentPen;
}

static void computeAverageNormal( float2* pAverageNormal, const float2* pNormal0, const float2* pNormal1 )
{
    float2 averageNormal;
    float2_add( &averageNormal, pNormal0, pNormal1 );
    float2_normalize( &averageNormal );

    // extend the normal 
    const float dp = fabsf( float2_dot( pNormal0, pNormal1 ) );
    const float disc = ( 1.0f - dp ) * 0.5f;
    if( disc > 0.1f )
    {
        const float scale = 1.0f / sqrtf( disc );
        float2_scale1f( &averageNormal, &averageNormal, scale );
    }

    *pAverageNormal = averageNormal;
}

static void computeStrokeNormals( StrokeCommand* pCommand, int isCycle )
{
    SYS_ASSERT( pCommand && pCommand->type == StrokeCommandType_Draw );
    const uint pointCount = pCommand->data.draw.pointCount;

    const uint firstPointIndex = pCommand->data.draw.pointIndex;
    const float2* pPoints = &s_renderer.strokeBuffer.points[ firstPointIndex ];
    float2* pPointNormals = &s_renderer.strokeBuffer.pointNormals[ firstPointIndex ];

    // compute segment normals:
    for( uint i = 1u; i < pointCount; ++i )
    {
        const float2 lastPoint = pPoints[ i - 1u ];
        const float2 currentPoint = pPoints[ i ];

        float2 dir;
        float2_sub( &dir, &currentPoint, &lastPoint );
        float2_normalize( &dir );
        float2_perpendicular( &pPointNormals[ i ], &dir );
    }
    pPointNormals[ 0u ] = pPointNormals[ 1u ];

    for( uint i = 1u; i < pCommand->data.draw.pointCount - 1u; ++i )
    {
        const float2 lastNormal = pPointNormals[ i ];
        const float2 nextNormal = pPointNormals[ i + 1 ];
        computeAverageNormal( &pPointNormals[ i ], &lastNormal, &nextNormal );
    }
    
    // check if this stroke is a cycle:
    const uint pointIndex0 = 0u;
    const uint pointIndexN = pointIndex0 + pCommand->data.draw.pointCount - 1u;
    
    if( isCycle )
    {
        const float2 normal0 = pPointNormals[ pointIndex0 ];
        const float2 normalN = pPointNormals[ pointIndexN ];
        
        float2 averageNormal;
        computeAverageNormal( &averageNormal, &normal0, &normalN );
        pPointNormals[ pointIndex0 ] = averageNormal;
        pPointNormals[ pointIndexN ] = averageNormal;
    }

/*    for( uint i = 0u; i < pointCount; ++i )
    {
        const float2 normal = pPointNormals[ i ];
        SYS_TRACE_DEBUG( "normal[%i]=(%f,%f)\n", i, normal.x, normal.y );
    }*/    
}

static void renderer_addSingleStroke( const float2* pStrokePoints, uint strokePointCount )
{
    SYS_ASSERT( s_renderer.pageState == PageState_BeforeDraw );
    if( !pStrokePoints || strokePointCount < 2u )
    {
        return;
    }
    
    const int isCycle=float2_isEqual(&pStrokePoints[0u],&pStrokePoints[strokePointCount-1u]);
    
    StrokeCommand command;
    createDrawCommand( &command );

    const float variance = s_renderer.currentVariance;

    // transform, randomize and copy positions
    for( uint i = 0u; i < strokePointCount; ++i )
    {
        const float2 strokePoint = pStrokePoints[ i ];

        // reduced variance in the beginning
        const int isFirstVertexInStroke = command.data.draw.pointCount == 0u;

        float2 point;
        transformPoint( &point, &strokePoint, isFirstVertexInStroke ? variance / 4.0f : variance );
        if( pushStrokePoint( &point ) )
        {
            command.data.draw.pointCount++;
        }
    }
   
    computeStrokeNormals( &command, isCycle );
    pushStrokeCommand( &command );
}

void renderer_addLinearStroke( const float2* pStrokePoints, uint strokePointCount )
{
    uint pointIndex = 0u;
    uint pointCount = 0u;

    float2 lastStrokePoint;
    for( uint i = 0u; i < strokePointCount; ++i )
    {
        const float2 strokePoint = pStrokePoints[ i ];

        // check if we have to start a new stroke:
        if( i > 0u && float2_isEqual( &lastStrokePoint, &strokePoint ) )
        {
            if( pointCount >= 2u )
            {
                renderer_addSingleStroke( &pStrokePoints[ pointIndex ], pointCount );
            }
            pointIndex = i + 1u;
            pointCount = 0u;
        }
        else
        {
            pointCount++;
        }

        lastStrokePoint = strokePoint;
    }
    if( pointCount >= 2u )
    {
        renderer_addSingleStroke( &pStrokePoints[ pointIndex ], pointCount );
    }
}

static inline void evaluateQuadraticCurve( float2* pResult, const float2* pPoints, float x )
{
    const float x0=pPoints[0u].x;
    const float y0=pPoints[0u].y;
    const float x1=pPoints[1u].x;
    const float y1=pPoints[1u].y;
    const float x2=pPoints[2u].x;
    const float y2=pPoints[2u].y;
    
    const float w0=((1.0f-x)*(1.0f-x));
    const float w1=2.0f*x*(1.0f-x);
    const float w2=x*x;

    pResult->x = w0*x0 + w1*x1 + w2*x2;
    pResult->y = w0*y0 + w1*y1 + w2*y2;
}

static void evaluateQuadraticCurveTangent( float2* pResult, const float2* pPoints, float x )
{
    const float x0=pPoints[0u].x;
    const float y0=pPoints[0u].y;
    const float x1=pPoints[1u].x;
    const float y1=pPoints[1u].y;
    const float x2=pPoints[2u].x;
    const float y2=pPoints[2u].y;
    
    const float t0x=x1-x0;
    const float t0y=y1-y0;
    const float t1x=x2-x1;
    const float t1y=y2-y1;

    pResult->x = 2.0f*(1.0f-x)*t0x + 2.0f*x*t1x;
    pResult->y = 2.0f*(1.0f-x)*t0y + 2.0f*x*t1y;
}

static uint addQuadraticCurvePoints(const float2* pPoints,uint stepCount,int addLastPoint)
{
    float x = 0.0f;
    float dx = 1.0f / (float)(stepCount-1u);

    uint addedPointCount = 0u;
    
    float2 point;
    if( !addLastPoint )
    {
        stepCount--;
    }
    const float distanceThreshold=0.1f;
    float2 lastPoint;
    float2 lastNormal;
    for( uint i = 0u; i < stepCount; ++i )
    {
        evaluateQuadraticCurve(&point,pPoints,x);
        if( i == 0 || i == stepCount - 1 || float2_distance(&point,&lastPoint)>distanceThreshold)
        {
            float2 tangent;
            evaluateQuadraticCurveTangent(&tangent,pPoints,x);
            float2 normal;
            float2_perpendicular(&normal,&tangent);
            float2_normalize(&normal);

            if( i > 0 && float2_dot( &normal, &lastNormal ) < 0.0f )
            {
                float2_scale1f( &normal, &normal, -1.0f );
            }

            if( pushStrokePointWithNormal( &point, &normal ) )
            {
                addedPointCount++;
            }
            lastPoint=point;
            lastNormal=normal;
        }

        x += dx;
    }
    return addedPointCount;
}

/*void renderer_addQuadraticStroke( const float2* p0, const float2* p1, const float2* p2 )
{
    const float variance = s_renderer.currentVariance;
    float2 cps[3u];
    transformPoint(&cps[0u], p0, variance/4.0f);
    transformPoint(&cps[1u], p1, variance);
    transformPoint(&cps[2u], p2, variance);

    const uint stepCount = 10u; // :TODO: adaptive detail 
    const int isCycle = 0;

    StrokeCommand command;
    createDrawCommand( &command );
    command.data.draw.pointCount += addQuadraticCurvePoints(cps,stepCount,1);
    computeStrokeNormals( &command, isCycle );
    pushStrokeCommand( &command );
}*/

/*static void renderer_addSingleQuadraticStroke( const float2* pPoints, uint pointCount )
{
    SYS_ASSERT( pPoints );
    SYS_ASSERT( pointCount >= 3u );

    // 

//    const uint 
    
}*/

void renderer_addQuadraticStroke( const float2* pPoints, uint pointCount )
{
    SYS_ASSERT( pPoints );
    SYS_ASSERT( pointCount >= 3u );

    const int isCycle=float2_isEqual(&pPoints[0u],&pPoints[pointCount-1u]);
    
    const uint segmentCount = ( pointCount - 3u ) / 2u + 1u;

    const float variance = s_renderer.currentVariance;
    float2 cps[3u];

    StrokeCommand command;
    createDrawCommand( &command );
    const float2* pSegmentPoints = pPoints;
    for(uint i=0u;i<segmentCount;++i)
    {
        if(i==0u)
        {
            transformPoint(&cps[0u], &pSegmentPoints[0u], variance/4.0f);
            transformPoint(&cps[1u], &pSegmentPoints[1u], variance);
            transformPoint(&cps[2u], &pSegmentPoints[2u], variance);
            pSegmentPoints += 3u;
        }
        else
        {
            // copy the last point over:
            float2 lastDir;
            float2_sub(&lastDir,&cps[2u],&cps[1u]);
            
            cps[0u]=cps[2u];
            transformPoint(&cps[1u], &pSegmentPoints[0u], variance);
            transformPoint(&cps[2u], &pSegmentPoints[1u], variance);
            
            // make sure that the cp1 is colinear with the last curve segment:
            float2 newDir;
            float2_sub(&newDir,&cps[1u],&cps[0u]);
            if(float2_squareLength(&lastDir)>0.0001f&&float2_squareLength(&newDir)>0.0001f)
            {
                float2_normalize(&lastDir);
                
                float2 newCp1;
                float2_scale1f(&newDir,&lastDir,float2_dot(&lastDir,&newDir));
                float2_add(&newCp1, &cps[0u], &newDir);
                cps[1u]=newCp1;
            }

            pSegmentPoints += 2u;
        }
        const uint stepCount = 10u; // :TODO: adaptive detail 
        command.data.draw.pointCount += addQuadraticCurvePoints(cps,stepCount,i==segmentCount-1u);
    }

    SYS_USE_ARGUMENT(isCycle);

//    computeStrokeNormals( &command, isCycle );
    pushStrokeCommand( &command );
}

/*void renderer_addQuadraticStroke( const float2* pPoints, uint pointCount )
{
    SYS_ASSERT( pPoints );
    SYS_ASSERT( pointCount >= 3 );
    uint pointIndex = 0u;
    uint pointCount = 0u;

    float2 lastStrokePoint;
    for( uint i = 0u; i < strokePointCount; ++i )
    {
        const float2 strokePoint = pStrokePoints[ i ];

        // check if we have to start a new stroke:
        if( i > 0u && float2_isEqual( &lastStrokePoint, &strokePoint ) )
        {
            if( pointCount >= 3u )
            {
                renderer_addSingleLinearStroke( &pStrokePoints[ pointIndex ], pointCount );
            }
            pointIndex = i + 1u;
            pointCount = 0u;
        }
        else
        {
            pointCount++;
        }

        lastStrokePoint = strokePoint;
    }
    if( pointCount >= 3u )
    {
        renderer_addSingleQuadraticStroke( &pStrokePoints[ pointIndex ], pointCount );
    }
}*/

static void startStroke( uint pointIndex, uint pointCount )
{
    SYS_ASSERT( pointCount >= 2u );

    Stroke* pStroke = &s_renderer.currentStroke;

    pStroke->progress           = 0.0f;
    pStroke->activeSegment      = 0u;
    pStroke->segmentProgress    = 0.0f;

    // compute total length of this stroke:
    float strokeLength = 0.0f;

    float2 segmentStart = s_renderer.strokeBuffer.points[ pointIndex ];
    for( uint i = 1u; i < pointCount; ++i )
    {
        const float2 segmentEnd = s_renderer.strokeBuffer.points[ pointIndex + i ];
        float segmentLength = float2_distance( &segmentStart, &segmentEnd );

        strokeLength += segmentLength;
        segmentStart = segmentEnd;
    }
    pStroke->length = strokeLength;

	SYS_ASSERT( strokeLength < 100000.0f );

    //SYS_TRACE_DEBUG( "Starting new stroke (length=%.2f #segments=%i)\n", strokeLength, pointCount - 1u );
}

static int advanceStroke( float* pRemainingTime, float timeStep )
{
    // draw stroke..
    Page* pPage = &s_renderer.pages[ s_renderer.currentPage ];
    Stroke* pStroke = &s_renderer.currentStroke;
    const StrokeCommand* pCommand = &s_renderer.strokeBuffer.commands[ s_renderer.currentCommand ];

    if( !pCommand || pCommand->type != StrokeCommandType_Draw )
    {
        SYS_TRACE_ERROR( "no active stroke!\n" );
        return FALSE;
    }
    
    const StrokeDrawCommandData* pDrawCommand = &pCommand->data.draw;

    graphics_setRenderTarget( &pPage->fgTarget );
    graphics_setShader( &s_renderer.penShader );
    graphics_setBlendMode( BlendMode_Over );

    const PenDefinition* pPen = &s_renderer.pens[ pDrawCommand->penId ];
    const float variance = pDrawCommand->variance;

    // set constant shader parameters:
    graphics_setFp4f( 0u, 0.5f * pPen->width, variance, 0.05f * variance, 0.0f );
    graphics_setFp4f( 1u, pPen->color.x, pPen->color.y, pPen->color.z, 1.0f );

    float currentProgress = pStroke->progress;
    const float strokeLength = pStroke->length;
    
    float newProgress;
    if( s_renderer.strokeDrawSpeed <= 0.0f )
    {
        // always finish the whole stroke:
        newProgress = strokeLength;

        // and we don't need any time:
        *pRemainingTime = timeStep;
    }
    else
    {
        // how long until we reach the end of this stroke?:
        const float remainingStrokeTime = ( strokeLength - pStroke->progress ) / s_renderer.strokeDrawSpeed;        
        const float usedTime = float_min( timeStep, remainingStrokeTime );

        newProgress = pStroke->progress + usedTime * s_renderer.strokeDrawSpeed;
        *pRemainingTime = timeStep - usedTime;
    }

//    SYS_TRACE_DEBUG( "advancing stroke from %f to %f! length=%f\n", currentProgress, newProgress, strokeLength );

    if( newProgress <= currentProgress )
    {
        return TRUE;
    }

    const uint segmentCount = pDrawCommand->pointCount - 1u;
    float remainingLength = newProgress - currentProgress;

    const float2* pStrokePoints = &s_renderer.strokeBuffer.points[ pDrawCommand->pointIndex ];
    const float2* pStrokeNormals = &s_renderer.strokeBuffer.pointNormals[ pDrawCommand->pointIndex ];
    
    while( pStroke->activeSegment < segmentCount && remainingLength > 0.0f )
    {
        const float2 segmentStart = pStrokePoints[ pStroke->activeSegment ];
        const float2 segmentEnd = pStrokePoints[ pStroke->activeSegment + 1u ];
        const float2 segmentNormalStart = pStrokeNormals[ pStroke->activeSegment ];
        const float2 segmentNormalEnd = pStrokeNormals[ pStroke->activeSegment + 1u ];

        // get remaining length in current segment:
        float activeSegmentLength = float2_distance( &segmentStart, &segmentEnd );

        if( activeSegmentLength <= 0.0f )
        {
            break;
        }

        float remainingSegmentLength = activeSegmentLength - pStroke->segmentProgress;
        
        const float segmentAdvance = float_min( remainingSegmentLength, remainingLength );

        // draw segment part:
        const float partStart = pStroke->segmentProgress;
        const float partEnd = partStart + segmentAdvance;

        const float strokeU0 = currentProgress / strokeLength;
        const float strokeU1 = ( currentProgress + segmentAdvance ) / strokeLength;

        const float segmentU0 = partStart / activeSegmentLength;
        const float segmentU1 = partEnd / activeSegmentLength;
        
        // draw the active segment between partStart and partEnd:
        //SYS_TRACE_DEBUG( "drawing segment part from %f to %f!\n", partStart / activeSegmentLength, partEnd / activeSegmentLength );
           
        // compute part 

        // :todo: draw start and end parts if this is the start/end of the segment

        // compute coordinates of quad enclosing the segment part:
        float2 segmentDir;
        float2_normalize( float2_sub( &segmentDir, &segmentEnd, &segmentStart ) );
    
        float2 normalStart, normalEnd;
        float2_lerp( &normalStart, &segmentNormalStart, &segmentNormalEnd, segmentU0 );
        float2_lerp( &normalEnd, &segmentNormalStart, &segmentNormalEnd, segmentU1 );

        float2 startPos, endPos;
        float2_addScaled1f( &startPos, &segmentStart, &segmentDir, partStart );
        float2_addScaled1f( &endPos, &segmentStart, &segmentDir, partEnd );

        const float ws = 2.0f * ( 64.0f / sys_getScreenWidth() );
        
        float2 vertices[ 4u ];
        float2_addScaled1f( &vertices[ 0u ], &startPos, &normalStart,  ws * pPen->width );
        float2_addScaled1f( &vertices[ 1u ], &startPos, &normalStart, -ws * pPen->width );
        float2_addScaled1f( &vertices[ 2u ], &endPos, &normalEnd, -ws * pPen->width );
        float2_addScaled1f( &vertices[ 3u ], &endPos, &normalEnd,  ws * pPen->width );

        const float u0 = strokeU0;
        const float u1 = strokeU1;

        //SYS_TRACE_DEBUG( "u0=%f u1=%f v0=%f,%f v3=%f,%f\n", u0, u1, vertices[ 0u ].x, vertices[ 0u ].y, vertices[ 3u ].x, vertices[ 3u ].y );

        graphics_drawQuad( vertices, u0, 0.0f, u1, 1.0f );
        
        pStroke->segmentProgress += segmentAdvance;
        remainingLength -= segmentAdvance;
        currentProgress += segmentAdvance;

        if( segmentAdvance >= remainingSegmentLength )
        {
            // next segment:
            pStroke->activeSegment++;
            pStroke->segmentProgress = 0.0f;
        }
    }

    if( newProgress < strokeLength )
    {
        pStroke->progress = newProgress;  
        return TRUE;
    }

    return FALSE;
}

#ifndef SYS_BUILD_MASTER
void renderer_drawCircle(const float2* pPos, float radius,const float3* pColor)
{
    Page* pPage = &s_renderer.pages[ s_renderer.currentPage ];
    graphics_setRenderTarget( &pPage->fgTarget );
    graphics_setShader( &s_renderer.debugPenShader );
    graphics_setBlendMode( BlendMode_Over );

    graphics_setFp4f( 0u, pColor->x, pColor->y, pColor->z, 1.0f );
    graphics_drawCircle(pPos,radius);
}
#endif

static void startDrawCommand()
{
    if( s_renderer.currentCommand >= s_renderer.strokeBuffer.commandCount )
    {
        return;
    }
    const StrokeCommand* pCurrentCommand = &s_renderer.strokeBuffer.commands[ s_renderer.currentCommand ];

    switch( pCurrentCommand->type )
    {
    case StrokeCommandType_Draw:
        startStroke( pCurrentCommand->data.draw.pointIndex, pCurrentCommand->data.draw.pointCount );
        break;

    case StrokeCommandType_Delay:
        
        break;

    default:
        break;
    }
}

static void updateDrawCommands( float timeStep )
{
    while( timeStep > 0.0f && s_renderer.currentCommand < s_renderer.strokeBuffer.commandCount )
    {
        const StrokeCommand* pCurrentCommand = &s_renderer.strokeBuffer.commands[ s_renderer.currentCommand ];
//        SYS_TRACE_DEBUG( "command %i/%i remaining time = %f\n", s_renderer.currentCommand, s_renderer.strokeBuffer.commandCount, timeStep );
        switch( pCurrentCommand->type )
        {
        case StrokeCommandType_Draw:
            if( !advanceStroke( &timeStep, timeStep ) )
            {
                s_renderer.currentCommand++;
                startDrawCommand();
            }
            break;

        case StrokeCommandType_Delay:
            // done..
            s_renderer.currentCommand++;
            startDrawCommand();
            break;

        default:
            break;
        }
    }
}

void renderer_updateState( float timeStep )
{
    float newStateTime = s_renderer.stateTime + timeStep;
    switch( s_renderer.pageState )
    {
    case PageState_BeforeDraw:
        if( newStateTime >= s_renderer.delayAfterFlip )
        {
            setPageState( PageState_Draw );
            startDrawCommand();
            
            newStateTime -= s_renderer.delayAfterFlip;
            renderer_updateState( newStateTime );
        }
        else
        {
            s_renderer.stateTime = newStateTime;
        }
        break;

    case PageState_Draw:
        // until the last stroke is done..
        if( s_renderer.currentCommand >= s_renderer.strokeBuffer.commandCount )
        {
            setPageState( PageState_AfterDraw );
            renderer_updateState( timeStep );
        }
        else
        {
            // update current stroke.. 
            updateDrawCommands( timeStep );
            // don't flip immediately..
        }
        break;

    case PageState_AfterDraw:
        if( newStateTime >= s_renderer.delayAfterDraw )
        {
            setPageState( PageState_Done );
            newStateTime -= s_renderer.delayAfterFlip;
            renderer_updateState( newStateTime );
        }
        else
        {
            s_renderer.stateTime = newStateTime;
        }
        break;

    case PageState_Done:
        break;

    default:
        break;
    }
}

void renderer_updatePage( float timeStep )
{
    renderer_updateState( timeStep );
    renderer_updatePageFlip( timeStep );
}

int renderer_isPageDone()
{
    return s_renderer.pageState == PageState_Done;
}

static void computeFlipParams( float3* pResult, float t )
{
	float angle1 = DEG2RADF( 90.0f );
    float angle2 = DEG2RADF( 8.0f );
    float angle3 = DEG2RADF( 6.0f ); 
    float A1 = -15.0f;
    float A2 = -2.5f;
    float A3 = -3.5f;
    float theta1 = 0.05f;
    float theta2 = 0.5f;
    float theta3 = 10.0f;
    float theta4 = 2.0f;
  
    float rho = t * PI;
    float theta, A;
  
	if( t <= 0.15f )
	{
		float dt = t / 0.15f;
		float f1 = sinf(PI * powf(dt, theta1) / 2.0f);
		float f2 = sinf(PI * powf(dt, theta2) / 2.0f);
        theta = float_lerp( angle1, angle2, f1 );
		A = float_lerp( A1, A2, f2 );
	}
	else if( t <= 0.4f )
	{
		float dt = (t - 0.15f) / 0.25f;
		theta = float_lerp( angle2, angle3, dt );
		A = float_lerp( A2, A3, dt );
	}
	else 
	{
		float dt = (t - 0.4f) / 0.6f;
		float f1 = sinf(PI * powf(dt, theta3) / 2.0f);
		float f2 = sinf(PI * powf(dt, theta4) / 2.0f);
		theta = float_lerp( angle3, angle1, f1 );
		A = float_lerp( A3, A1, f2 );
	}
    float3_set( pResult, A, theta, rho );
}

void renderer_drawFrame( const FrameData* pFrame )
{
    SYS_USE_ARGUMENT( pFrame );

    // now render the final screen 
    graphics_setRenderTarget( 0 );

    graphics_setBlendMode( BlendMode_Disabled );

    // render paper:
    graphics_setShader( &s_renderer.pageShader );

    const Page* pCurrentPage=&s_renderer.pages[s_renderer.currentPage];
    graphics_setFsTexture(0,pCurrentPage->bgTarget.id,SamplerState_ClampU_ClampV_Nearest);
    graphics_setFsTexture(1,pCurrentPage->fgTarget.id,SamplerState_ClampU_ClampV_Nearest);
    graphics_setFsTexture(2,pCurrentPage->burnTarget.id,SamplerState_ClampU_ClampV_Nearest);
    
    graphics_drawFullscreenQuad();

    // render the flipped page on top:
    if( s_renderer.flipTime >= 0.0f )
    {
        const float flipProgress = 0.5f * float_saturate( s_renderer.flipTime / s_renderer.flipDuration );
        //SYS_TRACE_DEBUG( "%f (%f/%f)\n", flipProgress, s_renderer.flipTime, s_renderer.flipDuration );

        graphics_setShader( &s_renderer.pageFlipShader );

        const Page* pLastPage=&s_renderer.pages[s_renderer.lastPage];
        graphics_setFsTexture(0,pLastPage->bgTarget.id,SamplerState_ClampU_ClampV_Nearest);
        graphics_setFsTexture(1,pLastPage->fgTarget.id,SamplerState_ClampU_ClampV_Nearest);
        graphics_setFsTexture(2,pLastPage->burnTarget.id,SamplerState_ClampU_ClampV_Nearest);

        float3 flipParams;
        computeFlipParams( &flipParams, flipProgress );
        graphics_setVp4f( 0u, flipParams.x, flipParams.y, flipParams.z, 0.0f );
        graphics_drawMesh2d( &s_renderer.pageFlipMesh ); 
    }
}

void renderer_addCircle( const Circle* pCircle )
{
	float2 points[] =
	{ 
		{ -1.0f,  0.0f },
		{ -1.0f,  1.0f },
		{  0.0f,  1.0f },
		{  1.0f,  1.0f },
		{  1.0f,  0.0f },
		{  1.0f, -1.0f },
		{  0.0f, -1.0f },
		{ -1.0f, -1.0f },
		{ -1.0f,  0.0f },
	};

	float2 scale;
	float2_set( &scale, pCircle->radius, pCircle->radius );

	for( uint i = 0u; i < SYS_COUNTOF( points ); ++i )
	{
		float2_scale2f( &points[ i ], &points[i], &scale );
		float2_add( &points[ i ], &points[ i ], &pCircle->center );
	}

	renderer_addQuadraticStroke( points, SYS_COUNTOF( points ) );
}
