#include "renderer.h"
#include "types.h"
#include "platform.h"
#include "debug.h"
#include "opengl.h"

#include "shader.h"
#include "rendertarget.h"

#include "paper_glsl.h"

#include <math.h>

typedef struct 
{
    RenderTarget    testTarget;
    Shader          paperShader;
} Renderer;

static Renderer s_renderer;

void renderer_init()
{
    SYS_VERIFY( rendertarget_create( &s_renderer.testTarget, 1280, 720, GL_RGBA8 ) );
    SYS_VERIFY( shader_create( &s_renderer.paperShader, &s_shader_paper ) );
}

void renderer_done()
{
}

void renderer_drawFrame( const FrameData* pFrame )
{
    // render into render target:
    rendertarget_activate( &s_renderer.testTarget );
    
    float4 params;
    float4_set( &params, 1.0f, 1.0f, 1.0f, 1.0f );

    glDisable( GL_DEPTH_TEST );
    glDisable( GL_TEXTURE_2D );

    // render paper:
    shader_activate( &s_renderer.paperShader );
    glUniform4fv( glGetUniformLocationARB( s_renderer.paperShader.id, "c0" ), 1u, &params.x );
    glRectf( -1.0f, -1.0f, 1.0f, 1.0f );

    // render cars:
    shader_activate( 0 );
    glMatrixMode( GL_PROJECTION );
    glLoadIdentity();
    glOrtho( -1.0f, 1.0f, 1.0f, -1.0f, 0.0f, 1.0f );
    glMatrixMode( GL_MODELVIEW );
    glLoadIdentity();
    glDisable( GL_TEXTURE_2D );
    const float playerWidth = 0.075f;
    const float playerHeight = 0.1f;
    glColor3f( 1.0f, 0.2f, 0.2f );
    glRectf( 
        pFrame->playerPos.x - playerWidth * 0.5f, 
        pFrame->playerPos.y - playerHeight * 0.5f,
        pFrame->playerPos.x + playerWidth * 0.5f,
        pFrame->playerPos.y + playerHeight * 0.5f );

    // now render the final screen 
    rendertarget_activate( 0 );
    shader_activate( 0 );
    
    glClearColor( 1.0f, 0.0f, 1.0f, 1.0f );
    glClear( GL_COLOR_BUFFER_BIT );
    
    glBindTexture( GL_TEXTURE_2D, s_renderer.testTarget.colorBuffer0 );
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
    glRotatef( 10.0f * pFrame->time, 0.0f, 0.0f, 1.0f );
    glTranslatef( -0.5f * width, -0.5f * height, 0.0f );

    glColor3f( 1.0f, 1.0f, 1.0f );
    glBegin( GL_QUADS );
        glTexCoord2f( 0.0, 0.0 );   glVertex2i( 0, 0 );
        glTexCoord2f( 1.0, 0.0 );   glVertex2i( width, 0 );
        glTexCoord2f( 1.0, 1.0 );   glVertex2i( width, height );
        glTexCoord2f( 0.0, 1.0 );   glVertex2i( 0, height );
    glEnd();
}

