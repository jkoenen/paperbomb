#include "graphics.h"
#include "mesh.h"
#include "debug.h"
#include "opengl.h"

typedef struct 
{
    VertexFormat    vertexFormat;
    BlendMode       blendMode;
    RenderTarget*   pRenderTarget;
    Shader*         pShader;
} GraphicsState;

static GraphicsState s_graphics;

void graphics_init()
{
    graphics_resetState();
}

void graphics_done()
{
}

void graphics_resetState()
{
    s_graphics.vertexFormat = VertexFormat_Count;
    s_graphics.blendMode = BlendMode_Count;
    s_graphics.pRenderTarget = 0;
    s_graphics.pShader = 0;

    graphics_setVertexFormat( VertexFormat_None );
    graphics_setBlendMode( BlendMode_Disabled );
    
    glDisable( GL_DEPTH_TEST );
    glDisable( GL_TEXTURE_2D );
}

void graphics_clear( float r, float g, float b, float a )
{
    glClearColor( r, g, b, a );
    glClear( GL_COLOR_BUFFER_BIT );
}

void graphics_drawFullscreenQuad()
{
    graphics_setVertexFormat( VertexFormat_None );
    glBegin( GL_QUADS );
        glTexCoord2f( 0.0, 0.0 );   glVertex2i( -1,  1 );
        glTexCoord2f( 1.0, 0.0 );   glVertex2i(  1,  1 );
        glTexCoord2f( 1.0, 1.0 );   glVertex2i(  1, -1 );
        glTexCoord2f( 0.0, 1.0 );   glVertex2i( -1, -1 );
    glEnd();
}

void graphics_drawQuad( const float2* pVertices, float u0, float v0, float u1, float v1 )
{
    graphics_setVertexFormat( VertexFormat_None );
    glBegin( GL_TRIANGLES );
        glTexCoord2f( u0, v0 );   glVertex2f( pVertices[ 0u ].x, pVertices[ 0u ].y );
        glTexCoord2f( u0, v1 );   glVertex2f( pVertices[ 1u ].x, pVertices[ 1u ].y );
        glTexCoord2f( u1, v1 );   glVertex2f( pVertices[ 2u ].x, pVertices[ 2u ].y );

        glTexCoord2f( u0, v0 );   glVertex2f( pVertices[ 0u ].x, pVertices[ 0u ].y );
        glTexCoord2f( u1, v1 );   glVertex2f( pVertices[ 2u ].x, pVertices[ 2u ].y );
        glTexCoord2f( u1, v0 );   glVertex2f( pVertices[ 3u ].x, pVertices[ 3u ].y );
    glEnd();
}

void graphics_setFsTexture( uint index, uint textureId )
{
    glEnable( GL_TEXTURE_2D );    
    glActiveTexture( GL_TEXTURE0 + index );
    glBindTexture( GL_TEXTURE_2D, textureId );
}

void graphics_setRenderTarget( RenderTarget* pTarget )
{
    if( s_graphics.pRenderTarget == pTarget )
    {
        return;
    }
    rendertarget_activate( pTarget );
    s_graphics.pRenderTarget = pTarget;
}

void graphics_setShader( Shader* pShader )
{
    if( s_graphics.pShader == pShader )
    {
        return;
    }

    shader_activate( pShader );
    s_graphics.pShader = pShader;
}

void graphics_setVp4f( uint index, float x, float y, float z, float w )
{
    SYS_ASSERT( s_graphics.pShader );
    shader_setVp4f( s_graphics.pShader, index, x, y, z, w );
}

void graphics_setFp4f( uint index, float x, float y, float z, float w )
{
    SYS_ASSERT( s_graphics.pShader );
    shader_setFp4f( s_graphics.pShader, index, x, y, z, w );
}

void graphics_setVertexFormat( VertexFormat format )
{
    if( format == s_graphics.vertexFormat )
    {
        return;
    }
    
    switch( format )
    {
    case VertexFormat_None:
        glDisableClientState( GL_VERTEX_ARRAY );
        glClientActiveTexture( GL_TEXTURE0 );
        glDisableClientState( GL_TEXTURE_COORD_ARRAY );
        break;

    case VertexFormat_2d:
        glEnableClientState( GL_VERTEX_ARRAY );
        glVertexPointer( 2, GL_FLOAT, sizeof( Vertex2d ), ( void* ) SYS_MEMBEROFFSET( Vertex2d, pos ) );
        glClientActiveTexture( GL_TEXTURE0 );
        glEnableClientState( GL_TEXTURE_COORD_ARRAY );
        glTexCoordPointer( 3, GL_FLOAT, sizeof( Vertex2d ), ( void* )SYS_MEMBEROFFSET( Vertex2d, texCoord ) );
        break;

    default:
        SYS_BREAK( "Invalid vertex format!\n" );
        break;
    }

    s_graphics.vertexFormat = format;
}

void graphics_setBlendMode( BlendMode mode )
{
    if( s_graphics.blendMode == mode )
    {
        return;
    }

    switch( mode )
    {
    case BlendMode_Disabled:
        glDisable( GL_BLEND );
        break;

    case BlendMode_Over:
        glEnable( GL_BLEND );
        glBlendEquationSeparate( GL_ADD, GL_ADD );
        glBlendFuncSeparate( GL_ONE, GL_ONE_MINUS_SRC_ALPHA, GL_ONE, GL_ONE_MINUS_SRC_ALPHA ); 
        break;

    default:
        SYS_BREAK( "Invalid blend mode!\n" );
        break;
    }

    s_graphics.blendMode = mode;
}

void graphics_createMesh2d( Mesh2d* pMesh, uint vertexCount, uint indexCount )
{
    SYS_ASSERT( pMesh );
    glGenBuffersARB( 1, &pMesh->vertexBufferId );
    glBindBufferARB( GL_ARRAY_BUFFER_ARB, pMesh->vertexBufferId );
    glBufferDataARB( GL_ARRAY_BUFFER_ARB, sizeof( Vertex2d ) * vertexCount, 0, GL_STATIC_DRAW_ARB );

    glGenBuffersARB( 1, &pMesh->indexBufferId );
    glBindBufferARB( GL_ELEMENT_ARRAY_BUFFER_ARB, pMesh->indexBufferId );
    glBufferDataARB( GL_ELEMENT_ARRAY_BUFFER_ARB, sizeof( Index ) * indexCount, 0, GL_STATIC_DRAW_ARB );

    pMesh->vertexCount = vertexCount;
    pMesh->indexCount = indexCount;
    
    glBindBufferARB( GL_ARRAY_BUFFER_ARB, 0 );
    glBindBufferARB( GL_ELEMENT_ARRAY_BUFFER_ARB, 0 );
}

int graphics_lockMesh2d( Mesh2dLock* pLock, Mesh2d* pMesh )
{
    SYS_ASSERT( pLock );
    SYS_ASSERT( pMesh );
    glBindBufferARB( GL_ARRAY_BUFFER_ARB, pMesh->vertexBufferId );
    glBindBufferARB( GL_ELEMENT_ARRAY_BUFFER_ARB, pMesh->indexBufferId );
    void* pVertexData = glMapBufferARB( GL_ARRAY_BUFFER_ARB, GL_WRITE_ONLY_ARB );
    void* pIndexData = glMapBufferARB( GL_ELEMENT_ARRAY_BUFFER_ARB, GL_WRITE_ONLY_ARB );

    if( !pVertexData || !pIndexData )
    {
        return 0;
    }
    pLock->pVertexData = ( Vertex2d* )pVertexData;
    pLock->pIndexData = ( Index* )pIndexData;
    return 1;
}

void graphics_unlockMesh2d( Mesh2d* pMesh )
{
    glBindBufferARB( GL_ARRAY_BUFFER_ARB, pMesh->vertexBufferId );
    glBindBufferARB( GL_ELEMENT_ARRAY_BUFFER_ARB, pMesh->indexBufferId );
    glUnmapBufferARB( GL_ARRAY_BUFFER_ARB );
    glUnmapBufferARB( GL_ELEMENT_ARRAY_BUFFER_ARB );
    glBindBufferARB( GL_ARRAY_BUFFER_ARB, 0 );
    glBindBufferARB( GL_ELEMENT_ARRAY_BUFFER_ARB, 0 );
}

void graphics_drawMesh2d( const Mesh2d* pMesh )
{
    glBindBufferARB( GL_ARRAY_BUFFER_ARB, pMesh->vertexBufferId );
    glBindBufferARB( GL_ELEMENT_ARRAY_BUFFER_ARB, pMesh->indexBufferId );

    graphics_setVertexFormat( VertexFormat_2d );

    glDrawElements( GL_TRIANGLES, ( int )pMesh->indexCount, GL_UNSIGNED_SHORT, 0 );

    glBindBufferARB( GL_ARRAY_BUFFER_ARB, 0 );
    glBindBufferARB( GL_ELEMENT_ARRAY_BUFFER_ARB, 0 );
}


