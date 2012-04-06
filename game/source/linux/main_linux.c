#include "sys/types.h"
#include "debug.h"
#include "game.h"
#include "sound.h"
#include "input.h"

#include "renderer.h"

#include <GL/gl.h>
#include <GL/glext.h>
#include <SDL/SDL.h>
#include <sys/soundcard.h>
#include <inttypes.h>
#include <math.h>
#include <stdarg.h>

#define TEST_RENDERER

#ifdef TEST_RENDERER
#   include "font.h"
#endif

enum
{
    ScreenWidth = 1280,
    ScreenHeight = 720
    //ScreenWidth = 640,
    //ScreenHeight = 360
};

static float2 s_soundBuffer[ SoundChannelCount * SoundBufferSampleCount ];

static uint s_callbackCount = 0u;

static void soundCallback( void* pUserData, Uint8* pStream, int size )
{
    SYS_USE_ARGUMENT( pUserData );
s_callbackCount++;
    SYS_ASSERT( size >= 0 );

    const size_t sampleCount = ( size_t )size / ( SoundChannelCount * sizeof( int16 ) );
    SYS_ASSERT( sampleCount * SoundChannelCount * sizeof( int16 ) == ( size_t ) size );

    SYS_ASSERT( sampleCount <= SYS_COUNTOF( s_soundBuffer ) );
    sound_fillBuffer( s_soundBuffer, ( uint )sampleCount );

    int16* pTarget = ( int16* )(void*)pStream;
    const float2* pSource = &s_soundBuffer[ 0u ];

    for( size_t i = 0u; i < sampleCount; ++i )
    {
        *pTarget++ = ( int16 )( 32767.0f * pSource->x );
        *pTarget++ = ( int16 )( 32767.0f * pSource->y );
        pSource++;
    }
}

void sys_trace( const char* pFormat, ... )
{
    va_list arg_list;
    va_start( arg_list, pFormat );
    vprintf( pFormat, arg_list );
    va_end( arg_list );
}

void sys_exit( int exitcode ) 
{
    exit( exitcode );
}

int sys_getScreenWidth()
{
    return ScreenWidth;
}

int sys_getScreenHeight()
{
    return ScreenHeight;
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
    if( SDL_Init( SDL_INIT_VIDEO | SDL_INIT_AUDIO | SDL_INIT_TIMER ) < 0 )
    {
        SYS_BREAK( "SDL_Init failed!\n" );
    }

    SDL_SetVideoMode( ScreenWidth, ScreenHeight, 0u, SDL_OPENGL /*| SDL_FULLSCREEN */ );
    SDL_ShowCursor( SDL_DISABLE );
    SDL_GL_SetAttribute( SDL_GL_SWAP_CONTROL, 1 );

    SDL_AudioSpec audioSpec;
    audioSpec.freq      = SoundSampleRate;
    audioSpec.format    = AUDIO_S16SYS;
    audioSpec.channels  = SoundChannelCount;
    audioSpec.silence   = 0;
    audioSpec.samples   = SoundBufferSampleCount;
    audioSpec.size      = 0;
    audioSpec.callback  = soundCallback;
    audioSpec.userdata  = s_soundBuffer;

    SDL_OpenAudio( &audioSpec, NULL );

    game_init();

    SDL_PauseAudio( 0 );

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

            char windowTitle[ 100u ];
            sprintf( windowTitle, "fps=%f cb=%i", currentFps, s_callbackCount );
            SDL_WM_SetCaption( windowTitle, 0 );
    
            lastTimingTime = currentTime;
            timingFrameCount = 0u;
        }
#endif

        // get local user input:
        SDL_Event event;
        while( SDL_PollEvent( &event ) )
        {
            switch( event.type )
            {
            case SDL_KEYDOWN:
            case SDL_KEYUP:
				{
					const int ctrlPressed = ( event.key.keysym.mod & KMOD_CTRL ) != 0;

					switch( event.key.keysym.sym )
					{
					case SDLK_ESCAPE:
						quit = 1;
						break;

					case SDLK_LEFT:
						updateButtonMask( &buttonMask, ctrlPressed ? ButtonMask_CtrlLeft : ButtonMask_Left, event.type == SDL_KEYDOWN );
						break;

					case SDLK_RIGHT:
						updateButtonMask( &buttonMask, ctrlPressed ? ButtonMask_CtrlRight : ButtonMask_Right, event.type == SDL_KEYDOWN );
						break;

					case SDLK_UP:
						updateButtonMask( &buttonMask, ctrlPressed ? ButtonMask_CtrlUp : ButtonMask_Up, event.type == SDL_KEYDOWN );
						break;

					case SDLK_DOWN:
						updateButtonMask( &buttonMask, ctrlPressed ? ButtonMask_CtrlDown : ButtonMask_Down, event.type == SDL_KEYDOWN );
						break;

					case SDLK_SPACE:
						updateButtonMask( &buttonMask, ButtonMask_PlaceBomb, event.type == SDL_KEYDOWN );
						break;

					default:
						break;
					}
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

#ifdef TEST_RENDERER
        if( renderer_isPageDone() )
        {
            // new page:
            renderer_flipPage();

            /*float2 points[] =
            { 
                { -4.0f,  0.0f },
                { -4.0f, -4.0f },
                {  0.0f, -4.0f },
                {  4.0f, -4.0f },
                {  4.0f,  0.0f },
                {  4.0f,  4.0f },
                {  0.0f,  4.0f },
                { -4.0f,  4.0f },
                { -4.0f,  0.0f }
                { -0.2f,  1.0f }
            };*/

            renderer_setPen( Pen_DebugGreen );

            const float2 worldOffset = { 32.0f, 16.0f };
            const float2 position = { 0.0f, 0.0f };

            float2x3 bombTransform;
            float2x2_rotationY( &bombTransform.rot, 0.0f );
            float2x2_scale1f( &bombTransform.rot, &bombTransform.rot, 1.0f );
            float2_add( &bombTransform.pos, &position, &worldOffset );

            renderer_setTransform( &bombTransform );

            //renderer_addQuadraticStroke(&points[0u],&points[1u],&points[2u]);
            //renderer_addQuadraticStroke(points,SYS_COUNTOF(points));

            float2 textPos;
            float2_set(&textPos,5.0f,4.0f);
            font_drawText(&textPos,2.0f,0.0f,"0123456789A" );
        }
        renderer_updatePage( timeStep );

        FrameData frame;
        //memset( &frame, 0u, sizeof( frame ) );
        //frame.time = s_game.gameTime;
        //frame.playerPos = s_game.player[ 0u ].position;
        renderer_drawFrame( &frame );
#else
        game_render();
#endif
        SDL_GL_SwapBuffers();

#ifndef SYS_BUILD_MASTER
        timingFrameCount++;
#endif
    }
    while( !quit );

    // :TODO: nicer fade out..
    SDL_PauseAudio( 1 );

    game_done();
}

