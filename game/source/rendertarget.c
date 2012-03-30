#include "rendertarget.h"
#include "debug.h"
#include "platform.h"

int rendertarget_create( RenderTarget* pTarget, int width, int height, int format )
{
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
    glTexImage2D( GL_TEXTURE_2D, 0, format, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, NULL );
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
        return 0;
    }
                                                                                                
    glBindFramebuffer( GL_FRAMEBUFFER, 0 );
    glBindTexture( GL_TEXTURE_2D, 0 );

    return 1;
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
