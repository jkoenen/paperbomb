#include <GL/gl.h>
#include <GL/glext.h>
#include <SDL/SDL.h>
#include <sys/soundcard.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <unistd.h>
#include <inttypes.h>
#include <math.h>
#include <stdarg.h>

#include "sys/types.h"
#include "sys/debug.h"
#include "game/game.h"

void sys_trace( int level, const char* pFormat, ... )
{
    (void)level;
    va_list arg_list;
    va_start( arg_list, pFormat );
    vprintf( pFormat, arg_list );
    va_end( arg_list );
}

int sys_getScreenWidth()
{
    return 1280;
}

int sys_getScreenHeight()
{
    return 720;
}

static void updateButtonMask( uint32* pButtonMask, uint32 button, int isDown )
{
    uint32 buttonMask = *pButtonMask;
    if( isDown )
    {
        buttonMask |= button;
    }
    else
    {
        buttonMask &= ~button;
    }
    *pButtonMask = buttonMask;
}

int main()
{
    if( SDL_Init( SDL_INIT_VIDEO ) < 0 )
    {
        SYS_BREAK( "SDL_Init failed!\n" );
    }

    SDL_SetVideoMode( 1280u, 720u, 0u, SDL_OPENGL /*| SDL_FULLSCREEN */ );
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

    game_init();
   
    uint32 lastTime = SDL_GetTicks();
    uint32 buttonMask = 0u;

#ifndef SYS_BUILD_MASTER
    uint32 lastTimingTime = lastTime;
    uint32 timingFrameCount = 0u;
#endif

    int quit = 0;
    do
    {
        uint32 currentTime = SDL_GetTicks();

        const float timeStep = ( float )( currentTime - lastTime ) / 1000.0f;
        lastTime = currentTime;

#ifndef SYS_BUILD_MASTER
        if( currentTime - lastTimingTime > 500u )
        {
            const float timingTimeSpan = ( float )( currentTime - lastTimingTime ) / 1000.0f;
            const float currentFps = ( float )( timingFrameCount ) / timingTimeSpan;

            SYS_TRACE_DEBUG( "fps=%f\n", currentFps );

            char windowTitle[ 100u ];
            sprintf( windowTitle, "fps=%f", currentFps );
            SDL_WM_SetCaption( windowTitle, 0 );
    
            lastTimingTime = currentTime;
            timingFrameCount = 0u;
        }
#endif

        ioctl( audio_fd, SNDCTL_DSP_SYNC );
        write( audio_fd, audio_buffer, 8192 );
    
        // get local user input:
        SDL_Event event;
        while( SDL_PollEvent( &event ) )
        {
            switch( event.type )
            {
            case SDL_KEYDOWN:
            case SDL_KEYUP:
                switch( event.key.keysym.sym )
                {
                case SDLK_ESCAPE:
                    quit = 1;
                    break;

                case SDLK_LEFT:
                    updateButtonMask( &buttonMask, ButtonMask_Left, event.type == SDL_KEYDOWN );
                    break;

                case SDLK_RIGHT:
                    updateButtonMask( &buttonMask, ButtonMask_Right, event.type == SDL_KEYDOWN );
                    break;

                case SDLK_UP:
                    updateButtonMask( &buttonMask, ButtonMask_Up, event.type == SDL_KEYDOWN );
                    break;

                case SDLK_DOWN:
                    updateButtonMask( &buttonMask, ButtonMask_Down, event.type == SDL_KEYDOWN );
                    break;

                case SDLK_SPACE:
                    updateButtonMask( &buttonMask, ButtonMask_PlaceBomb, event.type == SDL_KEYDOWN );
                    break;

                default:
                    break;
                }
                break;

            case SDL_QUIT:
                quit = 1;
                break;
            }
        }
        
        GameInput gameInput;
        memset( &gameInput, 0u, sizeof( gameInput ) );
        gameInput.timeStep = timeStep;
        gameInput.buttonMask = buttonMask;

        game_update( &gameInput );
        game_render();
        SDL_GL_SwapBuffers();

#ifndef SYS_BUILD_MASTER
        timingFrameCount++;
#endif
    }
    while( !quit );
    game_done();
}

