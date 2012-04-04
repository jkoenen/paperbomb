#include "types.h"
#include "vector.h"
#include "shader.h"
#include "debug.h"
#include "opengl.h"

static const char* s_pVSUniformNames[ MaxVSUniformCount ] =
{
    "vp0", "vp1", "vp2", "vp3"
};

static const char* s_pFSUniformNames[ MaxFSUniformCount ] =
{
    "fp0", "fp1", "fp2", "fp3"
};

static const char* s_pFSTextureNames[ MaxFSTextureCount ] =
{
    "ft0", "ft1", "ft2", "ft3"
};

uint shader_create( Shader* pShader, const GlslShaderDefinition* pDefinition, uint vsUniformCount, uint fsUniformCount, uint fsTextureCount )
{
    SYS_ASSERT( vsUniformCount <= MaxVSUniformCount );
    SYS_ASSERT( fsUniformCount <= MaxFSUniformCount );
    SYS_ASSERT( fsTextureCount <= MaxFSTextureCount );

    const uint shaderId = glCreateProgram();                           
    const uint vsId = glCreateShader( GL_VERTEX_SHADER );
    const uint fsId = glCreateShader( GL_FRAGMENT_SHADER );
    const char* pVsCode = pDefinition->pVsCode;
    const char* pFsCode = pDefinition->pFsCode;
    glShaderSource( vsId, 1, &pVsCode, 0 );
    glShaderSource( fsId, 1, &pFsCode, 0 );
    glCompileShader( vsId );
    glCompileShader( fsId );
    glAttachShader( shaderId, fsId );
    glAttachShader( shaderId, vsId );
    glLinkProgram( shaderId );

    // :TODO: get uniform locations..
    pShader->id = shaderId;

#ifdef SYS_BUILD_DEBUG
    int result;
    char info[1536];
    glGetObjectParameterivARB( vsId, GL_OBJECT_COMPILE_STATUS_ARB, &result ); 
    if( !result ) 
    {
        glGetInfoLogARB( vsId, 1024, NULL, (char *)info ); 
        SYS_TRACE_ERROR( "Could not build vertex shader! info:\n%s\n", info );
        SYS_TRACE_ERROR( "shader code = \n%s\n", pVsCode );
        return 0;
    }
    glGetObjectParameterivARB( fsId, GL_OBJECT_COMPILE_STATUS_ARB, &result ); 
    if( !result ) 
    {
        glGetInfoLogARB( fsId, 1024, NULL, (char *)info ); 
        SYS_TRACE_ERROR( "Could not build fragment shader! info:\n%s\n", info );
        SYS_TRACE_ERROR( "shader code = \n%s\n", pFsCode );
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

    for( uint i = 0u; i < vsUniformCount; ++i )
    {
        pShader->vp[ i ] = glGetUniformLocationARB( shaderId, s_pVSUniformNames[ i ] );
        if( pShader->vp[ i ] < 0 )
        {
            SYS_TRACE_DEBUG( "Could not find vs uniform parameter %s\n", s_pVSUniformNames[ i ] );
        }
    }
    for( uint i = 0u; i < fsUniformCount; ++i )
    {
        pShader->fp[ i ] = glGetUniformLocationARB( shaderId, s_pFSUniformNames[ i ] );
        if( pShader->fp[ i ] < 0 )
        {
            SYS_TRACE_DEBUG( "Could not find fs uniform parameter %s\n", s_pFSUniformNames[ i ] );
        }
    }
    
    for( uint i = 0u; i < fsTextureCount; ++i )
    {
        pShader->ft[ i ] = glGetUniformLocationARB( shaderId, s_pFSTextureNames[ i ] );
        if( pShader->ft[ i ] < 0 )
        {
            SYS_TRACE_DEBUG( "Could not find fs texture parameter %s\n", s_pFSTextureNames[ i ] );
        }
    }

    return shaderId;
}

void shader_activate( const Shader* pShader )
{
    if( pShader )
    {
        glUseProgram( pShader->id );
    }
    else
    {
        glUseProgram( 0 );
    }
}

void shader_setVp4f( const Shader* pShader, uint index, float x, float y, float z, float w )
{
    float4 params;
    float4_set( &params, x, y, z, w );
    glUniform4fv( pShader->vp[ index ], 1u, &params.x );
}

void shader_setFp4f( const Shader* pShader, uint index, float x, float y, float z, float w )
{
    float4 params;
    float4_set( &params, x, y, z, w );
    glUniform4fv( pShader->fp[ index ], 1u, &params.x );
}


