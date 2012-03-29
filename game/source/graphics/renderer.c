#include "graphics/renderer.h"
#include "sys/types.h"
#include "sys/sys.h"
#include "sys/debug.h"

#include "shader.h"
#include "rendertarget.h"

#include <math.h>
#include <GL/gl.h>
#include <GL/glext.h>

typedef struct 
{
    RenderTarget    testTarget;
    int             shaderId;
} Renderer;

static Renderer s_renderer;

static const char* s_pVertexShaderCode = 
    "uniform vec4 p0[4];"
    "void main()"
    "{"
        "gl_Position=gl_Vertex;"
//        "vec3 d=normalize(p0[1].xyz-p0[0].xyz);"
//        "vec3 r=normalize(cross(d,vec3(0.0,1.0,0.0)));"
//        "vec3 u=cross(r,d);"
//        "vec3 e=vec3(gl_Vertex.x*1.333,gl_Vertex.y,0.75);"
//        "gl_TexCoord[0].xyz=mat3(r,u,d)*e;"
//        "gl_TexCoord[1]=vec4(0.5)+gl_Vertex*0.5;"
    "}";

static const char* s_pFragmentShaderCode = 
    "uniform vec4 c0;"
    "void main()"
    "{"
        "gl_FragColor = vec4(c0.xyz,1);;"
    "}";


void renderer_init()
{
    SYS_VERIFY( rendertarget_create( &s_renderer.testTarget, 1280, 720, GL_RGBA8 ) );
    s_renderer.shaderId = shader_create( s_pVertexShaderCode, s_pFragmentShaderCode );
}

void renderer_done()
{
}

void renderer_drawFrame( const FrameData* pFrame )
{
    // render into render target:
    rendertarget_set( &s_renderer.testTarget );
    
    float4 params;
    float4_set( &params, 1.0f, 1.0f, 1.0f, 1.0f );

    glDisable( GL_DEPTH_TEST );

    glUseProgram( s_renderer.shaderId );
    glUniform4fv( glGetUniformLocationARB( s_renderer.shaderId, "c0" ), 1u, &params.x );
    glRectf( -1.0f, -1.0f, 1.0f, 1.0f );

    float4_set( &params, 1.0f, 0.2f, 0.2f, 1.0f );
    glUniform4fv( glGetUniformLocationARB( s_renderer.shaderId, "c0" ), 1u, &params.x );

    const float playerWidth = 0.075f;
    const float playerHeight = 0.1f;
    glRectf( 
        pFrame->playerPos.x - playerWidth * 0.5f, 
        pFrame->playerPos.y - playerHeight * 0.5f,
        pFrame->playerPos.x + playerWidth * 0.5f,
        pFrame->playerPos.y + playerHeight * 0.5f );

    // now render the final screen 
    rendertarget_set( 0 );
    
    glClearColor( 1.0f, 0.0f, 1.0f, 1.0f );
    glClear( GL_COLOR_BUFFER_BIT );
    glUseProgram( 0 );
    
    glBindTexture( GL_TEXTURE_2D, s_renderer.testTarget.colorBuffer0 );
    //glTexEnvf( GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE );
    glEnable( GL_TEXTURE_2D );

    glColor3f( 1.0f, 1.0f, 1.0f );
    glBegin( GL_QUADS );
        glTexCoord2f( 0.0, 0.0 );   glVertex3f( -1.0, -1.0, 0.0 );
        glTexCoord2f( 1.0, 0.0 );   glVertex3f(  1.0, -1.0, 0.0 );
        glTexCoord2f( 1.0, 1.0 );   glVertex3f(  1.0,  1.0, 0.0 );
        glTexCoord2f( 0.0, 1.0 );   glVertex3f( -1.0,  1.0, 0.0 );
    glEnd();
}

