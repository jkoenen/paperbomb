#include "rendertarget.h"
#include "debug.h"
#include "platform.h"
#include "opengl.h"

int rendertarget_create( RenderTarget* pTarget, int width, int height, PixelFormat format )
{
    GLint glFormat;

    switch( format )
    {
    case PixelFormat_R8G8B8A8:
        glFormat = GL_RGBA8;
        break;

    default:
        SYS_TRACE_ERROR( "Unsupported format %i\n", format );
        return FALSE;
    }

    pTarget->width = width;
    pTarget->height = height;
    pTarget->format = format;

    // create the color buffer:
    glGenTextures( 1, &pTarget->colorBuffer0 );
    glBindTexture( GL_TEXTURE_2D, pTarget->colorBuffer0 );
    glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE );
    glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE );
    glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST );
    glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST );
    glTexImage2D( GL_TEXTURE_2D, 0, glFormat, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, NULL );
    glBindTexture( GL_TEXTURE_2D, 0 );
                                        
    // todo: optional depth buffer..

    glGenFramebuffers( 1, &pTarget->id );
    glBindFramebuffer( GL_FRAMEBUFFER, pTarget->id );
    glFramebufferTexture2D( GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, pTarget->colorBuffer0, 0 );

    GLenum drawBuffers[ 1u ] = { GL_COLOR_ATTACHMENT0 };
    glDrawBuffers( 1, drawBuffers );
                                                        
    if( glCheckFramebufferStatus( GL_FRAMEBUFFER ) != GL_FRAMEBUFFER_COMPLETE )
    {
        SYS_TRACE_ERROR( "Could not create framebuffer!\n" );
        return FALSE;
    }
                                                                                                
    glBindFramebuffer( GL_FRAMEBUFFER, 0 );
    glBindTexture( GL_TEXTURE_2D, 0 );

    return TRUE;
}

void rendertarget_activate( const RenderTarget* pTarget )
{
    if( pTarget )
    {
        glBindFramebuffer( GL_FRAMEBUFFER, pTarget->id );
        glViewport( 0, 0, pTarget->width, pTarget->height );
    }
    else
    {
        glBindFramebuffer( GL_FRAMEBUFFER, 0 );
        glViewport( 0, 0, sys_getScreenWidth(), sys_getScreenHeight() );
    }
}

