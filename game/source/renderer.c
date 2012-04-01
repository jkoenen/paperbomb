#include "renderer.h"
#include "types.h"
#include "platform.h"
#include "debug.h"
#include "opengl.h"

#include "shader.h"
#include "rendertarget.h"

#include "paper_glsl.h"
#include "pen_glsl.h"
#include "page_glsl.h"

#include <math.h>

enum 
{
    // at the same time:
    MaxPageCount = 4,
    MaxStrokeCount = 4
};


typedef struct
{
} Mesh;

typedef struct 
{
    float2                  translation;        // todo: add complete matrix
    float                   progress;           // current position [0..length]
    uint                    activeSegment;
    float2                  startOffset;
    float2                  endOffset;
    float                   segmentProgress;    // 0..length of active segment
    float                   width;
    float                   speed;          
    float                   length;
    float                   variance;
    const StrokeDefinition* pDefinition;        // 0 for inactive strokes
} Stroke;

typedef struct
{
    RenderTarget    bgTarget;       // :TODO: share backgrounds?? we probably don't need that many
    RenderTarget    fgTarget;
} Page;

typedef struct 
{
    Shader          paperShader;
    Shader          penShader;
    Shader          pageShader;
    Page            pages[ 2u ];
    uint            currentPage;
    uint            lastPage;
    float           flipProgress;
    float           flipSpeed;
    Stroke          currentStroke;
} Renderer;

static Renderer s_renderer;

static void page_create( Page* pPage, uint width, uint height )
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

    float4 params;
    float4_set( &params, float_rand(), float_rand(), 1.0f, 1.0f );
    int paramId = glGetUniformLocationARB( s_renderer.paperShader.id, "params0" );
    SYS_TRACE_DEBUG( "%i->%f,%f\n", paramId, params.x, params.y );
    SYS_ASSERT( paramId >= 0 );
    glUniform4fv( paramId, 1u, &params.x );
    
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

void renderer_init()
{
    SYS_VERIFY( shader_create( &s_renderer.paperShader, &s_shader_paper ) );
    SYS_VERIFY( shader_create( &s_renderer.penShader, &s_shader_pen ) );
    SYS_VERIFY( shader_create( &s_renderer.pageShader, &s_shader_page ) );

    const uint width = sys_getScreenWidth();
    const uint height = sys_getScreenHeight();

    // create page:
    for( uint i = 0u; i < SYS_COUNTOF( s_renderer.pages ); ++i )
    {
        page_create( &s_renderer.pages[ i ], width, height );
    }

    s_renderer.currentPage = 0u;
    s_renderer.lastPage = 1u;
    
    s_renderer.currentStroke.pDefinition = 0;

    // :TODO: create page flip mesh:

    s_renderer.flipProgress = 0.0f;
    s_renderer.flipSpeed = -1.0f;
}

void renderer_done()
{
    // :TODO:
}

void renderer_startPageFlip( float duration )
{
    // 
    if( s_renderer.flipSpeed > 0.0f )
    {
        SYS_TRACE_WARNING( "starting page flip while another flip is still active!\n" ); 
    }
    
    s_renderer.lastPage = s_renderer.currentPage;
    s_renderer.currentPage = 1 - s_renderer.currentPage;

    s_renderer.currentStroke.pDefinition = 0;

    Page* pPage = &s_renderer.pages[ s_renderer.currentPage ];
    
    // clear current page:
    rendertarget_activate( &pPage->fgTarget );
    
    glDisable( GL_DEPTH_TEST );
    glDisable( GL_TEXTURE_2D );
    glDisable( GL_BLEND );

    // 
    glClearColor( 0.0f, 0.0f, 0.0f, 0.0f );
    glClear( GL_COLOR_BUFFER_BIT );    

    if( duration > 0.0f )
    {
        s_renderer.flipProgress = 0.0f;
        s_renderer.flipSpeed = 1.0f / duration;
    }
    else
    {
        // flip instantly:
        s_renderer.flipProgress = 1.0f;
        s_renderer.flipSpeed = -1.0f;
    }
}

int renderer_advancePageFlip( float timeStep )
{
    if( s_renderer.flipSpeed <= 0.0f )
    {
        return 0;
    }

    s_renderer.flipProgress += timeStep * s_renderer.flipSpeed;

    if( s_renderer.flipProgress >= 1.0f )
    {
        s_renderer.flipProgress = 1.0f;
        s_renderer.flipSpeed = -1.0f;
    }

    return s_renderer.flipSpeed > 0.0f;
}

void renderer_flipPage()
{
    renderer_startPageFlip( 0.0f );
}

void renderer_startStroke( const StrokeDefinition* pDefinition, const float2* pPositionOnPage, float speed, float width, float variance )
{
    SYS_ASSERT( pDefinition && pDefinition->pPoints && pDefinition->pointCount >= 2 );
    SYS_ASSERT( pPositionOnPage );

    Stroke* pStroke = &s_renderer.currentStroke;

    pStroke->pDefinition        = pDefinition;
    pStroke->translation        = *pPositionOnPage;
    pStroke->progress           = 0.0f;
    pStroke->activeSegment      = 0u;
    pStroke->segmentProgress    = 0.0f;
    pStroke->speed              = speed;
    pStroke->width              = width;
    pStroke->variance           = variance;

    float2_rand_normal( &pStroke->startOffset, 0.0f, variance / 4.0f );
    float2_rand_normal( &pStroke->endOffset, 0.0f, variance );

    // compute total length of this stroke:
    float strokeLength = 0.0f;

    float2 segmentStart = pDefinition->pPoints[ 0u ];
    for( uint i = 1u; i < pDefinition->pointCount; ++i )
    {
        const float2 segmentEnd = pDefinition->pPoints[ i ];
        float segmentLength = float2_distance( &segmentStart, &segmentEnd );

        SYS_ASSERT( segmentLength > 0.0f );

        strokeLength += segmentLength;
        segmentStart = segmentEnd;
    }
    pStroke->length = strokeLength;

    //SYS_TRACE_DEBUG( "Starting new stroke (length=%.2f #segments=%i width=%.2f speed=%.2f)\n", strokeLength, pDefinition->pointCount - 1u, width, speed );
}

int renderer_advanceStroke( float timeStep )
{
    // draw stroke..
    Page* pPage = &s_renderer.pages[ s_renderer.currentPage ];
    Stroke* pStroke = &s_renderer.currentStroke;

    if( !pStroke->pDefinition )
    {
        SYS_TRACE_ERROR( "no active stroke!\n" );
        return 0;
    }
    rendertarget_activate( &pPage->fgTarget );
    shader_activate( &s_renderer.penShader );

    glEnable( GL_BLEND );
    glBlendEquationSeparate( GL_FUNC_ADD, GL_FUNC_ADD );
    glBlendFuncSeparate( GL_ONE, GL_ONE, GL_ONE, GL_ONE ); 
    glBlendFuncSeparate( GL_ONE, GL_ONE_MINUS_SRC_ALPHA, GL_ONE, GL_ONE_MINUS_SRC_ALPHA ); 

    // set constant shader parameters:
    float4 params;
    float4_set( &params, pStroke->width, pStroke->variance, 0.3f, 0.0f );
    int paramId = glGetUniformLocationARB( s_renderer.paperShader.id, "params0" );
    SYS_ASSERT( paramId >= 0 );
    glUniform4fv( paramId, 1u, &params.x );

    float currentProgress = pStroke->progress;
    const float newProgress = float_min( currentProgress + pStroke->speed * timeStep, pStroke->length );

    //SYS_TRACE_DEBUG( "advancing stroke from %f to %f! length=%f\n", currentProgress, newProgress );

    const uint segmentCount = pStroke->pDefinition->pointCount - 1u;
    float remainingLength = newProgress - currentProgress;
    
    while( remainingLength > 0.0f )
    {
        if( pStroke->activeSegment >= segmentCount )
        {
            pStroke->pDefinition = 0u;
            break;
        }
        // get remaining length in current segment:
        float2 segmentStart;
        float2_add( &segmentStart, &pStroke->pDefinition->pPoints[ pStroke->activeSegment ], &pStroke->startOffset );
        float2 segmentEnd;
        float2_add( &segmentEnd, &pStroke->pDefinition->pPoints[ pStroke->activeSegment + 1u ], &pStroke->endOffset );

        float activeSegmentLength = float2_distance( &segmentStart, &segmentEnd );
        float remainingSegmentLength = activeSegmentLength - pStroke->segmentProgress;

        // draw segment part:
        float partStart = pStroke->segmentProgress;
        float partEnd;

        const float segmentAdvance = float_min( remainingSegmentLength, remainingLength );

        // stay in this segment:
        pStroke->segmentProgress += segmentAdvance;
        partEnd = pStroke->segmentProgress;
        remainingLength -= segmentAdvance;

        // draw the active segment between partStart and partEnd:
        //SYS_TRACE_DEBUG( "drawing segment part from %f to %f!\n", partStart / activeSegmentLength, partEnd / activeSegmentLength );
            
        // compute part 

        // :todo: draw start and end parts if this is the start/end of the segment

        // compute coordinates of quad enclosing the segment part:
        float2 segmentDir;
        float2_normalize( float2_sub( &segmentDir, &segmentEnd, &segmentStart ) );

        float2 segmentUp;
        float2_perpendicular( &segmentUp, &segmentDir );

        float2 startPos, endPos;
        float2_addScaled1f( &startPos, &segmentStart, &segmentDir, partStart );
        float2_addScaled1f( &endPos, &segmentStart, &segmentDir, partEnd );

        const float ws = 1.0f;

        float2 vertices[ 4u ];
        float2_addScaled1f( &vertices[ 0u ], &startPos, &segmentUp,  ws * pStroke->width );
        float2_addScaled1f( &vertices[ 1u ], &startPos, &segmentUp, -ws * pStroke->width );
        float2_addScaled1f( &vertices[ 2u ], &endPos, &segmentUp, -ws * pStroke->width );
        float2_addScaled1f( &vertices[ 3u ], &endPos, &segmentUp,  ws * pStroke->width );

        const float u0 = partStart / activeSegmentLength;
        const float u1 = partEnd / activeSegmentLength;

        currentProgress += segmentAdvance;
        
        glBegin( GL_TRIANGLES );
            glTexCoord2f( u0, 0.0 );   glVertex2f( vertices[ 0u ].x, vertices[ 0u ].y );
            glTexCoord2f( u0, 1.0 );   glVertex2f( vertices[ 1u ].x, vertices[ 1u ].y );
            glTexCoord2f( u1, 1.0 );   glVertex2f( vertices[ 2u ].x, vertices[ 2u ].y );

            glTexCoord2f( u0, 0.0 );   glVertex2f( vertices[ 0u ].x, vertices[ 0u ].y );
            glTexCoord2f( u1, 1.0 );   glVertex2f( vertices[ 2u ].x, vertices[ 2u ].y );
            glTexCoord2f( u1, 0.0 );   glVertex2f( vertices[ 3u ].x, vertices[ 3u ].y );
        glEnd();
        
        if( segmentAdvance >= remainingSegmentLength )
        {
            // next segment:
            pStroke->activeSegment++;
            pStroke->segmentProgress = 0.0f;
            pStroke->startOffset = pStroke->endOffset;

            float2_rand_normal( &pStroke->endOffset, 0.0f, pStroke->variance );
        }
    }

    if( newProgress < pStroke->length )
    {
        pStroke->progress = newProgress;  
    }
    else
    {
        pStroke->pDefinition = 0u;
    }

    return pStroke->pDefinition != 0u;
}

void renderer_drawStroke( const StrokeDefinition* pDefinition, const float2* pPositionOnPage, float width, float variance )
{
    renderer_startStroke( pDefinition, pPositionOnPage, 1.0f, width, variance );
    renderer_advanceStroke( s_renderer.currentStroke.length );
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
/*    glClearColor( 1.0f, 0.0f, 1.0f, 1.0f );
    glClear( GL_COLOR_BUFFER_BIT );

    glEnable( GL_BLEND );
    glBlendEquationSeparate( GL_FUNC_ADD, GL_FUNC_ADD );
    glBlendFuncSeparate( GL_ONE, GL_ONE_MINUS_SRC_ALPHA, GL_ONE, GL_ZERO ); 

    glBindTexture( GL_TEXTURE_2D, s_renderer.pages[ s_renderer.currentPage ].fgTarget.id );
    //glTexEnvf( GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE );
    glEnable( GL_TEXTURE_2D );

    int width = sys_getScreenWidth();
    int height = sys_getScreenHeight();

    glMatrixMode( GL_PROJECTION );
    glLoadIdentity();
    glOrtho( 0, width, height, 0, 0, 1 );
    glMatrixMode( GL_MODELVIEW );
    glLoadIdentity();

    glTranslatef( 0.5f * width, 0.5f * height, 0.0f );
   // glRotatef( 10.0f * pFrame->time, 0.0f, 0.0f, 1.0f );
    glTranslatef( -0.5f * width, -0.5f * height, 0.0f );

    glColor3f( 1.0f, 1.0f, 1.0f );
    glBegin( GL_QUADS );
        glTexCoord2f( 0.0, 0.0 );   glVertex2i( 0, 0 );
        glTexCoord2f( 1.0, 0.0 );   glVertex2i( width, 0 );
        glTexCoord2f( 1.0, 1.0 );   glVertex2i( width, height );
        glTexCoord2f( 0.0, 1.0 );   glVertex2i( 0, height );
    glEnd();*/
}

