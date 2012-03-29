#include "shader.h"
#include "sys/debug.h"
#include <GL/gl.h>
#include <GL/glext.h>

int shader_create( const char* pVertexShaderCode, const char* pFragmentShaderCode )
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
    return shaderId;
}

