#include "renderer.h"
#include "types.h"
#include "platform.h"
#include "debug.h"
#include "opengl.h"
#include "vector.h"

#include "shader.h"
#include "rendertarget.h"

#include "paper_glsl.h"
#include "pen_glsl.h"
#include "page_glsl.h"

#include <math.h>

enum 
{
    MaxPointCount = 1024u,
    MaxCommandCount = 256u
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
    RenderTarget    bgTarget;       // :TODO: share backgrounds?? we probably don't need that many
    RenderTarget    fgTarget;
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
    SYS_VERIFY( rendertarget_create( &pPage->bgTarget, width, height, GL_RGBA8 ) );
    SYS_VERIFY( rendertarget_create( &pPage->fgTarget, width, height, GL_RGBA8 ) );

    // render into render target:
    rendertarget_activate( &pPage->bgTarget );
    
    glDisable( GL_DEPTH_TEST );
    glDisable( GL_TEXTURE_2D );
    glDisable( GL_BLEND );

    // render paper:
    shader_activate( &s_renderer.paperShader );
    
    int paramId = glGetUniformLocationARB( s_renderer.paperShader.id, "params0" );    
    SYS_ASSERT( paramId >= 0 );

    int paperSizeId = glGetUniformLocationARB( s_renderer.paperShader.id, "paperSize" );
    SYS_ASSERT( paperSizeId >= 0 );

    float4 params;
    float4_set( &params, float_rand(), float_rand(), 2.0f / width, 2.0f / height );
    glUniform4fv( paramId, 1u, &params.x );

    float4_set( &params, 32.0f, 18.0f, 0.4f, 0.3f );
    glUniform4fv( paperSizeId, 1u, &params.x );
    
    glRectf( -1.0f, -1.0f, 1.0f, 1.0f );
    
    rendertarget_activate( &pPage->fgTarget );
    shader_activate( 0 );
    
    glDisable( GL_DEPTH_TEST );
    glDisable( GL_TEXTURE_2D );
    glDisable( GL_BLEND );

    // 
    glClearColor( 0.0f, 0.0f, 0.0f, 0.0f );
    glClear( GL_COLOR_BUFFER_BIT );
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

void renderer_init()
{
    SYS_VERIFY( shader_create( &s_renderer.paperShader, &s_shader_paper ) );
    SYS_VERIFY( shader_create( &s_renderer.penShader, &s_shader_pen ) );
    SYS_VERIFY( shader_create( &s_renderer.pageShader, &s_shader_page ) );

    const int width = sys_getScreenWidth();
    const int height = sys_getScreenHeight();

    // create page:
    for( uint i = 0u; i < SYS_COUNTOF( s_renderer.pages ); ++i )
    {
        page_create( &s_renderer.pages[ i ], width, height );
    }

    s_renderer.currentPage = 0u;
    s_renderer.lastPage = 1u;
    
//    s_renderer.currentStroke.pDefinition = 0;
    s_renderer.currentCommand = 0u;

    s_renderer.pageState = PageState_Done;
    s_renderer.stateTime = 0.0f;

    clearStrokeBuffer( &s_renderer.strokeBuffer );

    float3 color;
    color.x = 0.2f;
    color.y = 0.2f;
    color.z = 0.2f;
    createPen( &s_renderer.pens[ Pen_Default ], 2.0f, 1.0f, &color );
    createPen( &s_renderer.pens[ Pen_Font ], 2.0f, 1.0f, &color );
    createPen( &s_renderer.pens[ Pen_Fat ], 20.0f, 1.0f, &color );

    // :TODO: create page flip mesh:

    s_renderer.flipTime = -1.0f;

    renderer_setDrawSpeed( 0.5f );
    renderer_setPen( Pen_Default );
    renderer_setVariance( 0.2f );
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
    const float maxDelayAfterDraw = 1.0f;
    const float maxFlipDuration = 0.5f;

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
        s_renderer.strokeDrawSpeed = float_max( speed, 0.001f ) * maxStrokeDrawSpeed;
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
    rendertarget_activate( &pPage->fgTarget );
    
    glDisable( GL_DEPTH_TEST );
    glDisable( GL_TEXTURE_2D );
    glDisable( GL_BLEND );

    // 
    glClearColor( 0.0f, 0.0f, 0.0f, 0.0f );
    glClear( GL_COLOR_BUFFER_BIT );    

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

void renderer_addStroke( const float2* pStrokePoints, uint pointCount )
{
    SYS_ASSERT( pStrokePoints && pointCount >= 2 );
    SYS_ASSERT( s_renderer.pageState == PageState_BeforeDraw );

    // allocate space in output buffer
    if( ( s_renderer.strokeBuffer.pointCount + pointCount > MaxPointCount ) ||
        ( s_renderer.strokeBuffer.commandCount + 1u > MaxCommandCount ) )
    {
        SYS_TRACE_WARNING( "not enough space in command buffer!\n" );
        return;
    }

    StrokeCommand* pCommand = &s_renderer.strokeBuffer.commands[ s_renderer.strokeBuffer.commandCount ];
    s_renderer.strokeBuffer.commandCount += 1u;

    pCommand->type = StrokeCommandType_Draw;
    pCommand->data.draw.pointIndex = s_renderer.strokeBuffer.pointCount;
    pCommand->data.draw.pointCount = pointCount;
    pCommand->data.draw.penId = s_renderer.currentPen;

    float2* pPoints = &s_renderer.strokeBuffer.points[ s_renderer.strokeBuffer.pointCount ];
    float2* pPointNormals = &s_renderer.strokeBuffer.pointNormals[ s_renderer.strokeBuffer.pointCount ];
    s_renderer.strokeBuffer.pointCount += pointCount;

    const float variance = s_renderer.currentVariance;
    const float2x3* pTransform = &s_renderer.currentTransform;

    // transform, randomize and copy positions
    for( uint i = 0u; i < pointCount; ++i )
    {
        const float2 strokePoint = pStrokePoints[ i ];

        float2 offset;
        float2_rand_normal( &offset, 0.0f, i == 0 ?variance / 4.0f : variance );  // reduced variance in the beginning

        float2 point;
        float2x3_transform( &point, pTransform, &strokePoint );
        float2_add( &point, &point, &offset );

        *pPoints++ = point;
    }

    // compute segment normals:
    for( uint i = 1u; i < pointCount; ++i )
    {
        const float2 lastPoint = pStrokePoints[ i - 1u ];
        const float2 currentPoint = pStrokePoints[ i ];

        float2 dir;
        float2_sub( &dir, &currentPoint, &lastPoint );
        float2_normalize( &dir );
        float2_perpendicular( &pPointNormals[ i ], &dir );
    }
    pPointNormals[ 0u ] = pPointNormals[ 1u ];

    // compute averaged normals:
    for( uint i = 1u; i < pointCount - 1u; ++i )
    {
        const float2 lastNormal = pPointNormals[ i ];
        const float2 nextNormal = pPointNormals[ i + 1 ];

        float2 averageNormal;
        float2_add( &averageNormal, &lastNormal, &nextNormal );
        float2_normalize( &averageNormal );

        pPointNormals[ i ] = averageNormal;
    }

/*    for( uint i = 0u; i < pointCount; ++i )
    {
        const float2 normal = pPointNormals[ i ];
        SYS_TRACE_DEBUG( "normal[%i]=(%f,%f)\n", i, normal.x, normal.y );
    }*/

    //SYS_TRACE_DEBUG( "added stroke with %i points (pos=%i)\n", pointCount, s_renderer.strokeBuffer.commandCount - 1u );
}

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
        const float2 segmentEnd = s_renderer.strokeBuffer.points[ i ];
        float segmentLength = float2_distance( &segmentStart, &segmentEnd );

        strokeLength += segmentLength;
        segmentStart = segmentEnd;
    }
    pStroke->length = strokeLength;

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
        return 0;
    }
    
    const StrokeDrawCommandData* pDrawCommand = &pCommand->data.draw;

    rendertarget_activate( &pPage->fgTarget );
    shader_activate( &s_renderer.penShader );

    glEnable( GL_BLEND );
    glBlendEquationSeparate( GL_FUNC_ADD, GL_FUNC_ADD );
    glBlendFuncSeparate( GL_ONE, GL_ONE, GL_ONE, GL_ONE ); 
    glBlendFuncSeparate( GL_ONE, GL_ONE_MINUS_SRC_ALPHA, GL_ONE, GL_ONE_MINUS_SRC_ALPHA ); 

    const PenDefinition* pPen = &s_renderer.pens[ pDrawCommand->penId ];
    const float variance = pDrawCommand->variance;

    // set constant shader parameters:
    float4 params;
    float4_set( &params, 0.5f * pPen->width, variance, variance, 0.0f );
    int paramId = glGetUniformLocationARB( s_renderer.paperShader.id, "params0" );
    SYS_ASSERT( paramId >= 0 );
    glUniform4fv( paramId, 1u, &params.x );

    float currentProgress = pStroke->progress;

    float newProgress;
    if( s_renderer.strokeDrawSpeed <= 0.0f )
    {
        // always finish the whole stroke:
        newProgress = pStroke->length;

        // and we don't need any time:
        *pRemainingTime = timeStep;
    }
    else
    {
        // how long until we reach the end of this stroke?:
        const float remainingStrokeTime = ( pStroke->length - pStroke->progress ) / s_renderer.strokeDrawSpeed;        
        const float usedTime = float_min( timeStep, remainingStrokeTime );

        newProgress = pStroke->progress + usedTime * s_renderer.strokeDrawSpeed;
        *pRemainingTime = timeStep - usedTime;
    }

//    SYS_TRACE_DEBUG( "advancing stroke from %f to %f! length=%f\n", currentProgress, newProgress, pStroke->length );

    if( newProgress <= currentProgress )
    {
        return 1;
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

        const float u0 = partStart / activeSegmentLength;
        const float u1 = partEnd / activeSegmentLength;
        
        // draw the active segment between partStart and partEnd:
        //SYS_TRACE_DEBUG( "drawing segment part from %f to %f!\n", partStart / activeSegmentLength, partEnd / activeSegmentLength );
           
        // compute part 

        // :todo: draw start and end parts if this is the start/end of the segment

        // compute coordinates of quad enclosing the segment part:
        float2 segmentDir;
        float2_normalize( float2_sub( &segmentDir, &segmentEnd, &segmentStart ) );
    
        float2 normalStart, normalEnd;
        float2_lerp( &normalStart, &segmentNormalStart, &segmentNormalEnd, u0 );
        float2_normalize( &normalStart );
        float2_lerp( &normalEnd, &segmentNormalStart, &segmentNormalEnd, u1 );
        float2_normalize( &normalEnd );

        float2 startPos, endPos;
        float2_addScaled1f( &startPos, &segmentStart, &segmentDir, partStart );
        float2_addScaled1f( &endPos, &segmentStart, &segmentDir, partEnd );

        const float ws = 2.0f * ( 64.0f / sys_getScreenWidth() );

        float2 vertices[ 4u ];
        float2_addScaled1f( &vertices[ 0u ], &startPos, &normalStart,  ws * pPen->width );
        float2_addScaled1f( &vertices[ 1u ], &startPos, &normalStart, -ws * pPen->width );
        float2_addScaled1f( &vertices[ 2u ], &endPos, &normalEnd, -ws * pPen->width );
        float2_addScaled1f( &vertices[ 3u ], &endPos, &normalEnd,  ws * pPen->width );

        glBegin( GL_TRIANGLES );
            glTexCoord2f( u0, 0.0 );   glVertex2f( vertices[ 0u ].x, vertices[ 0u ].y );
            glTexCoord2f( u0, 1.0 );   glVertex2f( vertices[ 1u ].x, vertices[ 1u ].y );
            glTexCoord2f( u1, 1.0 );   glVertex2f( vertices[ 2u ].x, vertices[ 2u ].y );

            glTexCoord2f( u0, 0.0 );   glVertex2f( vertices[ 0u ].x, vertices[ 0u ].y );
            glTexCoord2f( u1, 1.0 );   glVertex2f( vertices[ 2u ].x, vertices[ 2u ].y );
            glTexCoord2f( u1, 0.0 );   glVertex2f( vertices[ 3u ].x, vertices[ 3u ].y );
        glEnd();
        
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

    if( newProgress < pStroke->length )
    {
        pStroke->progress = newProgress;  
        return 1u;
    }

    return 0u;
}

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

void renderer_drawFrame( const FrameData* pFrame )
{
    SYS_USE_ARGUMENT( pFrame );

    // now render the final screen 
    rendertarget_activate( 0 );

    glDisable( GL_DEPTH_TEST );
    glDisable( GL_TEXTURE_2D );
    glDisable( GL_BLEND );

    // render paper:
    shader_activate( &s_renderer.pageShader );

    int bgTextureId = glGetUniformLocationARB( s_renderer.pageShader.id, "bgTexture" );
    int fgTextureId = glGetUniformLocationARB( s_renderer.pageShader.id, "fgTexture" );

    glUniform1i( bgTextureId, 0 );
    glUniform1i( fgTextureId, 1 );

    glEnable( GL_TEXTURE_2D );    
    glActiveTexture( GL_TEXTURE0 );
    glBindTexture( GL_TEXTURE_2D, s_renderer.pages[ s_renderer.currentPage ].bgTarget.id );

    glActiveTexture( GL_TEXTURE0 + 1u );
    glBindTexture( GL_TEXTURE_2D, s_renderer.pages[ s_renderer.currentPage ].fgTarget.id );

    glBegin( GL_QUADS );
        glTexCoord2f( 0.0, 0.0 );   glVertex2i( -1,  1 );
        glTexCoord2f( 1.0, 0.0 );   glVertex2i(  1,  1 );
        glTexCoord2f( 1.0, 1.0 );   glVertex2i(  1, -1 );
        glTexCoord2f( 0.0, 1.0 );   glVertex2i( -1, -1 );
    glEnd();

    // render the flipped page on top:
    if( s_renderer.flipTime >= 0.0f )
    {
        const float flipProgress = float_saturate( s_renderer.flipTime / s_renderer.flipDuration );

        glEnable( GL_TEXTURE_2D );    
        glActiveTexture( GL_TEXTURE0 );
        glBindTexture( GL_TEXTURE_2D, s_renderer.pages[ s_renderer.lastPage ].bgTarget.id );

        glActiveTexture( GL_TEXTURE0 + 1u );
        glBindTexture( GL_TEXTURE_2D, s_renderer.pages[ s_renderer.lastPage ].fgTarget.id );
    
        // :TODO: nicer flip effect please:
        const float offset = 2.0f * flipProgress;

        glBegin( GL_QUADS );
            glTexCoord2f( 0.0, 0.0 );   glVertex2f( -1.0f,  1.0f + offset );
            glTexCoord2f( 1.0, 0.0 );   glVertex2f(  1.0f,  1.0f + offset );
            glTexCoord2f( 1.0, 1.0 );   glVertex2f(  1.0f, -1.0f + offset );
            glTexCoord2f( 0.0, 1.0 );   glVertex2f( -1.0f, -1.0f + offset );
        glEnd();
    }
}

