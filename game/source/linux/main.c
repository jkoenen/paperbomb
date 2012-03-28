#include <GL/gl.h>
#include <GL/glext.h>
#include <SDL/SDL.h>
#include <sys/soundcard.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <unistd.h>
#include <inttypes.h>
#include <math.h>

#include "sys_types.h"
#include "sys_math.h"
#include "sys_debug.h"
#include "game_main.h"

static void die( int exitcode )
{
    // syscall to exit the process:
    asm (   
        "movl %0, %%eax\n"
        "xor %%ebx, %%ebx\n"
        "int $128\n"
        : /* no output */
        : "r"( exitcode ));
}

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
        PB_TRACE_ERROR( "Could not create framebuffer!\n" );
        return 0;
    }
                                                                                                
    glBindFramebuffer( GL_FRAMEBUFFER, 0 );
    glBindTexture( GL_TEXTURE_2D, 0 );
                                                                                                        
    return 1;
}
*/

static int shader_create( const char* pVertexShaderCode, const char* pFragmentShaderCode )
{
    int shaderId = glCreateProgram();                           
    const int vsId = glCreateShader( GL_VERTEX_SHADER );
    const int fsId = glCreateShader( GL_FRAGMENT_SHADER );
    glShaderSource( vsId, 1, &pVertexShaderCode, 0 );
    glShaderSource( fsId, 1, &pFragmentShaderCode, 0 );
    glCompileShader( vsId );
    glCompileShader( fsId );
    glAttachShader( shaderId, fsId );
    glAttachShader( shaderId, vsId );
    glLinkProgram( shaderId );

#ifdef MOTOR3D_DEBUG
    int result;
    char info[1536];
    glGetObjectParameterivARB( vsId,   GL_OBJECT_COMPILE_STATUS_ARB, &result ); 
    if( !result ) 
    {
        glGetInfoLogARB( vsId, 1024, NULL, (char *)info ); 
        PB_TRACE_ERROR( "Could not build vertex shader! info:\n%s\n", info );
        return 0;
    }
    glGetObjectParameterivARB( fsId, GL_OBJECT_COMPILE_STATUS_ARB, &result ); 
    if( !result ) 
    {
        glGetInfoLogARB( fsId, 1024, NULL, (char *)info ); 
        PB_TRACE_ERROR( "Could not build fragment shader! info:\n%s\n", info );
        return 0;
    }
    glGetObjectParameterivARB( shaderId, GL_OBJECT_LINK_STATUS_ARB, &result ); 
    if( !result ) 
    {
        glGetInfoLogARB( shaderId, 1024, NULL, (char *)info ); 
        PB_TRACE_ERROR( "Could not link shader! info:\n%s\n", info );
        return 0;
    }
#endif
    PB_TRACE_DEBUG( "blub=%i\n", shaderId );
    return shaderId;
}

static const char* s_pVertexShaderCode = 
    "uniform vec4 p0[4];"
    "void main()"
    "{"
        "gl_Position=gl_Vertex;"
        "vec3 d=normalize(p0[1].xyz-p0[0].xyz);"
        "vec3 r=normalize(cross(d,vec3(0.0,1.0,0.0)));"
        "vec3 u=cross(r,d);"
        "vec3 e=vec3(gl_Vertex.x*1.333,gl_Vertex.y,0.75);"
        "gl_TexCoord[0].xyz=mat3(r,u,d)*e;"
        "gl_TexCoord[1]=vec4(0.5)+gl_Vertex*0.5;"
    "}";

static const char* s_pFragmentShaderCode = 
    "uniform vec4 c0;"
    "void main()"
    "{"
        "gl_FragColor = vec4(c0.xyz,1);;"
    "}";


//#ifdef MOTOR3D_BUILD_MASTER
//void _start()
//#else
int main()
//#endif
{
    // init a lot of stuff.. sound+graphics+threads
    if( SDL_Init( SDL_INIT_VIDEO ) < 0 )
    {
        die( 1 );
    }

    SDL_SetVideoMode( 640, 480, 0, SDL_OPENGL /*| SDL_FULLSCREEN */ );
    SDL_ShowCursor( SDL_DISABLE );
    SDL_GL_SetAttribute( SDL_GL_SWAP_CONTROL, 1 );

    int audio_fd,i;
    audio_fd = open( "/dev/dsp", O_WRONLY, 0 );
    i = AFMT_S16_LE;
    ioctl( audio_fd, SNDCTL_DSP_SETFMT, &i );
    i = 1;
    ioctl( audio_fd, SNDCTL_DSP_CHANNELS, &i );
    i = 11024;
    ioctl( audio_fd, SNDCTL_DSP_SPEED, &i );

    uint16_t audio_buffer[ 4096u ];
    for( size_t i = 0u; i < 4096u; ++i )
    {
        audio_buffer[ i ] = i << 2u;
    }

    int shaderId = shader_create( s_pVertexShaderCode, s_pFragmentShaderCode );
   
    pb_game_t* pGame = pb_game_init();
    
    float time = 0.0f;

    SDL_Event event;
    
    uint32_t lastTime = SDL_GetTicks();

    do
    {
        uint32_t currentTime = SDL_GetTicks();

        const float timeStep = ( float )( currentTime - lastTime ) / 1000.0f;
        lastTime = currentTime;

        ioctl( audio_fd, SNDCTL_DSP_SYNC );
        write( audio_fd, audio_buffer, 8192 );
    
        pb_game_update( pGame, timeStep );
        pb_game_render( pGame );

        time += timeStep;

        float4_t params;
        params = float4_create( 1.0f, 0.0f, cosf( time ), 1.0f );

        glClearColor( 0.0f, 1.0f, 0.0f, 1.0f );
        glClear( GL_COLOR_BUFFER_BIT );

        glUseProgram( shaderId );
        glUniform4fv( glGetUniformLocationARB( shaderId, "c0" ), 1u, &params.x );
        glRects( -1.0f, -1.0f, 1.0f, 1.0f );

        SDL_GL_SwapBuffers();

        SDL_PollEvent( &event );
        SDL_Delay( 10 );
    }
    while( event.type != SDL_KEYDOWN );
    pb_game_done( pGame );

    // syscall to exit the process:
    die( 0 );
}

