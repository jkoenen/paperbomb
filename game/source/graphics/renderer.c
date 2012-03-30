#include "graphics/renderer.h"
#include "sys/types.h"
#include "sys/sys.h"
#include "sys/debug.h"
#include "graphics/opengl.h"

#include <math.h>

typedef struct 
{
    uint         shaderId;
} renderer_t;

static renderer_t s_renderer;

/*
typedef struct
{
    GLuint  framebufferId;
    GLuint  textureId;
} rendertarget_t;

static int rendertarget_create( rendertarget_t* pTarget, int width, int height, int format )
{
    //    glGenFramebuffers( 1, &pTarget->framebufferId );
    glBindFramebufferEXT( GL_FRAMEBUFFER, pTarget->framebufferId );
       
    glGenTextures( 1, &pTarget->textureId );
    glBindTexture( GL_TEXTURE_2D, pTarget->textureId );
    glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE );
    glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE );
    glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST );
    glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST );
    glTexImage2D( GL_TEXTURE_2D, 0, format, width, height, 0, GL_RGBA, GL_FLOAT, 0 );
                                        
    // todo: optional depth buffer:
    glFramebufferTexture( GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, pTarget->textureId, 0 );

    GLenum drawBuffers[ 1u ] = { GL_COLOR_ATTACHMENT0 };
    glDrawBuffers( 1, drawBuffers );
                                                        
    if( glCheckFramebufferStatus( GL_FRAMEBUFFER ) != GL_FRAMEBUFFER_COMPLETE )
    {
        SYS_TRACE_ERROR( "Could not create framebuffer!\n" );
        return 0;
    }
                                                                                                
    glBindFramebuffer( GL_FRAMEBUFFER, 0 );
    glBindTexture( GL_TEXTURE_2D, 0 );
                                                                                                        
    return 1;
}
*/

static uint shader_create( const char* pVertexShaderCode, const char* pFragmentShaderCode )
{
    uint shaderId = glCreateProgram();                           
    const uint vsId = glCreateShader( GL_VERTEX_SHADER );
    const uint fsId = glCreateShader( GL_FRAGMENT_SHADER );
    glShaderSource( vsId, 1, &pVertexShaderCode, 0 );
    glShaderSource( fsId, 1, &pFragmentShaderCode, 0 );
    glCompileShader( vsId );
    glCompileShader( fsId );
    glAttachShader( shaderId, fsId );
    glAttachShader( shaderId, vsId );
    glLinkProgram( shaderId );

#ifdef SYS_BUILD_DEBUG
    int result;
    char info[1536];
    glGetObjectParameterivARB( vsId,   GL_OBJECT_COMPILE_STATUS_ARB, &result ); 
    if( !result ) 
    {
        glGetInfoLogARB( vsId, 1024, NULL, (char *)info ); 
        SYS_TRACE_ERROR( "Could not build vertex shader! info:\n%s\n", info );
        return 0;
    }
    glGetObjectParameterivARB( fsId, GL_OBJECT_COMPILE_STATUS_ARB, &result ); 
    if( !result ) 
    {
        glGetInfoLogARB( fsId, 1024, NULL, (char *)info ); 
        SYS_TRACE_ERROR( "Could not build fragment shader! info:\n%s\n", info );
        return 0;
    }
    glGetObjectParameterivARB( shaderId, GL_OBJECT_LINK_STATUS_ARB, &result ); 
    if( !result ) 
    {
        glGetInfoLogARB( shaderId, 1024, NULL, (char *)info ); 
        SYS_TRACE_ERROR( "Could not link shader! info:\n%s\n", info );
        return 0;
    }
#endif
    return 1;
}

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
    s_renderer.shaderId = shader_create( s_pVertexShaderCode, s_pFragmentShaderCode );
}

void renderer_done()
{
}

void renderer_drawFrame( const FrameData* pFrame )
{
    float4 params;
    float4_set( &params, 1.0f, 1.0f, 1.0f, 1.0f );

    glClearColor( 1.0f, 0.0f, 1.0f, 1.0f );
    glClear( GL_COLOR_BUFFER_BIT );
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
}

