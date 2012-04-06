#include "sys/types.h"
#include "debug.h"
#include "game.h"
#include "sound.h"
#include "input.h"

#include "renderer.h"
#include "font.h"

#include <GL/gl.h>
#include <GL/glext.h>
#include <SDL/SDL.h>
#include <sys/soundcard.h>
#include <inttypes.h>
#include <math.h>
#include <stdarg.h>

#ifndef SYS_BUILD_MASTER
//# define TEST_RENDERER
#   define FONT_EDITOR
#endif

enum
{
    ScreenWidth = 1280,
    ScreenHeight = 720
    //ScreenWidth = 640,
    //ScreenHeight = 360
};

static float2 s_soundBuffer[ SoundChannelCount * SoundBufferSampleCount ];
static uint s_callbackCount=0u;

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

static void updateButtonMaskFromJoystick( uint32* pButtonMask, uint32 button, int16 value, int16 threshold )
{
    uint32 buttonMask = *pButtonMask;
    if( value > threshold )
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
    if( SDL_Init( SDL_INIT_VIDEO | SDL_INIT_AUDIO | SDL_INIT_TIMER | SDL_INIT_JOYSTICK ) < 0 )
    {
        SYS_BREAK( "SDL_Init failed!\n" );
    }

    SDL_SetVideoMode( ScreenWidth, ScreenHeight, 0u, SDL_OPENGL /*| SDL_FULLSCREEN */ );
#ifndef FONT_EDITOR
    SDL_ShowCursor( SDL_DISABLE );
#endif
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
    uint32 joystickButtonMask = 0u; 

#ifndef SYS_BUILD_MASTER
    uint32 lastTimingTime = lastTime;
    uint32 timingFrameCount = 0u;
#endif

    SDL_JoystickEventState( SDL_ENABLE );

    SDL_Joystick* pJoystick0 = SDL_JoystickOpen( 0 );
    SYS_USE_ARGUMENT(pJoystick0);

    int16 joystickAxis0 = 0;
    int16 joystickAxis1 = 0;

    float2 mousePos;
    float2_set(&mousePos, 0.0f, 0.0f);

    int leftMouseDown=0;
    int leftMousePressed=0;

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
            case SDL_JOYAXISMOTION:
                if( event.jaxis.axis == 0u )
                {
                    joystickAxis0 = event.jaxis.value;
                }
                else if( event.jaxis.axis == 1u )
                {
                    joystickAxis1 = event.jaxis.value;
                }
                break;

            case SDL_JOYBUTTONDOWN:
                if( event.jbutton.button == 0 )
                {
                    updateButtonMask( &joystickButtonMask, ButtonMask_PlaceBomb, 1 );
                }
                break;

            case SDL_JOYBUTTONUP:
                if( event.jbutton.button == 0 )
                {
                    updateButtonMask( &joystickButtonMask, ButtonMask_PlaceBomb, 0 );
                }
                break;

            case SDL_MOUSEMOTION:
                float2_set(&mousePos, (float)event.motion.x, (float)event.motion.y);
                break;

            case SDL_MOUSEBUTTONDOWN:
                if( event.button.button == 1 )
                {
                    leftMouseDown = 1u;
                    leftMousePressed = 1u;
                }
                break;

            case SDL_MOUSEBUTTONUP:
                if( event.button.button == 1 )
                {
                    leftMouseDown = 0u;
                }
                break;

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

					case SDLK_c:
						updateButtonMask( &buttonMask, ButtonMask_Client, event.type == SDL_KEYDOWN );
						break;

					case SDLK_s:
						updateButtonMask( &buttonMask, ButtonMask_Server, event.type == SDL_KEYDOWN );
						break;

					case SDLK_l:
						updateButtonMask( &buttonMask, ButtonMask_Leave, event.type == SDL_KEYDOWN );
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
        
        updateButtonMaskFromJoystick( &joystickButtonMask, ButtonMask_Right, joystickAxis0, 16000 );
        updateButtonMaskFromJoystick( &joystickButtonMask, ButtonMask_Left, -joystickAxis0, 16000 );
        updateButtonMaskFromJoystick( &joystickButtonMask, ButtonMask_Down, joystickAxis1, 3200 );
        updateButtonMaskFromJoystick( &joystickButtonMask, ButtonMask_Up, -joystickAxis1, 3200 );

#ifdef FONT_EDITOR
        static uint currentChar = '0';
        static uint dataChar = '\0';
        static float2 charPoints[ 128u ];
        static uint charPointCount = 0u;
        static uint32 lastButtonMask = 0u;
        static int currentPointIndex = -1;
        static float2 oldMousePagePos;

        const uint32 buttonPressMask = buttonMask & ~lastButtonMask;
        lastButtonMask = buttonMask;

        float2 mousePagePos;
        mousePagePos.x=64.0f*mousePos.x/(float)ScreenWidth;
        mousePagePos.y=36.0f+-36.0f*mousePos.y/(float)ScreenHeight;

        if( buttonPressMask & ButtonMask_Right )
        {
            currentChar=font_getNextGlyphCharCode(currentChar,1);
        }
        if( buttonPressMask & ButtonMask_Left )
        {
            currentChar=font_getNextGlyphCharCode(currentChar,-1);
        }

        if( currentChar != dataChar )
        {
            const FontGlyph* pGlyph=font_getGlyph((uint)currentChar);
            if(pGlyph)
            {
                charPointCount = uint_min(SYS_COUNTOF(charPoints),pGlyph->pointCount);
                memcpy(charPoints,pGlyph->pPoints,charPointCount*sizeof(float2));
                dataChar = currentChar;
            }
            else
            {
                charPointCount = 0u;
            }
            currentPointIndex = -1;
        }

        if( renderer_isPageDone() )
        {
            // new page:
            renderer_flipPage();

            renderer_setPen( Pen_Font );

/*            const float2 worldOffset = { 32.0f, 16.0f };
            const float2 position = { 0.0f, 0.0f };*/
            
            renderer_setVariance( 0.0f );
            renderer_setTransform( 0 );

            const float fontSize=3.0f;

            float2 textPos;
            float2_set(&textPos,32.0f,16.0f);

            if( charPointCount > 0u )
            {
                float2x3 transform;
                float2x2_identity(&transform.rot);
                float2x2_scale1f(&transform.rot,&transform.rot,fontSize);
                transform.pos=textPos;

                renderer_setTransform( &transform );
                renderer_addQuadraticStroke(charPoints,charPointCount);
            }

            // draw control points:
            const float cpSize=0.5f;
            for(uint i = 0u; i < charPointCount; ++i)
            {
                float2 pos;
                float2_set(&pos,32.0f,16.0f);
                float2_addScaled1f(&pos,&pos,&charPoints[i],fontSize);
                
                const int inRange = float2_distance(&mousePagePos,&pos)<cpSize;
                if(currentPointIndex==-1&&inRange&&leftMousePressed)
                {
                    currentPointIndex=(int)i;
                }
                else if(currentPointIndex!=-1)
                {
                    if(!leftMouseDown)
                    {
                        currentPointIndex=-1;
                    }
                    else if( currentPointIndex==(int)i )
                    {
                        // move cp to be under the mouse cursor:
                        float2 newCpPos;
                        float2_sub(&newCpPos,&mousePagePos,&textPos);
                        float2_scale1f(&charPoints[i],&newCpPos,1.0f/fontSize);
                    }
                }

                float3 color;
                if((int)i==currentPointIndex)
                {
                    float3_set(&color,0.2f,1.0f,0.2f);
                }
                else if(inRange)
                {
                    float3_set(&color,0.5f,0.5f,0.2f);
                }
                else
                {
                    float3_set(&color,0.8f,0.2f,0.2f);
                }
                renderer_drawCircle(&pos,cpSize,&color);
            }
            oldMousePagePos=mousePagePos;
            leftMousePressed=0;
        }
        renderer_updatePage( timeStep );

        FrameData frame;
        //memset( &frame, 0u, sizeof( frame ) );
        //frame.time = s_game.gameTime;
        //frame.playerPos = s_game.player[ 0u ].position;
        renderer_drawFrame( &frame );
                
#elif TEST_RENDERER
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
        GameInput gameInput;
        memset( &gameInput, 0u, sizeof( gameInput ) );
        gameInput.timeStep = timeStep;
        gameInput.buttonMask = buttonMask | joystickButtonMask;

        game_update( &gameInput );

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

