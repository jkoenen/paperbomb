#include "win32_pre.h"

#include <mmsystem.h>
#include <GL/glew.h>
#include <initguid.h>
#define DIRECTINPUT_VERSION 0x0800
#include <dsound.h>

#include "win32_post.h"

#include "types.h"
#include "debug.h"
#include "game.h"
#include "input.h"
#include "sound.h"

#include <stdio.h>
#include <stdarg.h>
#include <process.h> 

static uint		s_width = 800;
static uint		s_height = 450;
static uint32	s_currentButtonMask = 0u;
static uint32	s_currentJoyStickButtonMask = 0u;

static HWND					s_hWnd = NULL;
static WAVEFORMATEX			s_waveFormat;
static LPDIRECTSOUND		s_pDxSound = NULL;
static LPDIRECTSOUNDBUFFER	s_pDxSoundBuffer = NULL;
static HANDLE				s_soundEvents[ 2u ];
static HANDLE				s_soundThreadHandle = NULL;
static float				s_soundBuffer[ SoundChannelCount * SoundBufferSampleCount ];

int sys_getScreenWidth()
{
	return (int)s_width;
}

int sys_getScreenHeight()
{
	return (int)s_height;
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

void sys_trace( const char* pFormat, ... )
{
	va_list	vargs;
	char buffer[ 2048u ];

	va_start( vargs, pFormat );
	vsnprintf_s( buffer, sizeof( buffer ), sizeof( buffer ) - 1u, pFormat, vargs );
	va_end(vargs);

	OutputDebugString( buffer );
}

void sys_exit( int exitcode )
{
	ExitProcess( ( uint )exitcode );
}

static LRESULT CALLBACK WndProc( HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam )
{
	switch( uMsg )
	{
	case WM_CREATE:
		if( joySetCapture( hWnd, JOYSTICKID1, NULL, FALSE ) ) 
		{ 
			//MessageBox( hWnd, "No fucking Joystick", NULL, MB_OK | MB_ICONEXCLAMATION ); 
		} 
		break; 

	case MM_JOY1ZMOVE:
		{
			//SYS_TRACE_DEBUG( "MOVE %d\n", LOWORD(lParam) );
		}
		break;

	case MM_JOY1MOVE: 
		{
			const int value = 20000;
			const int xPos = ( (int)LOWORD(lParam) - 32768 ); 
			const int yPos = ( (int)HIWORD(lParam) - 32768 ); 
			//SYS_TRACE_DEBUG( "MOVE %d %d\n", xPos, yPos );
			updateButtonMask( &s_currentJoyStickButtonMask, ButtonMask_Left, xPos < -value );
			updateButtonMask( &s_currentJoyStickButtonMask, ButtonMask_Right, xPos > value );
			updateButtonMask( &s_currentJoyStickButtonMask, ButtonMask_Up, yPos < -value );
			updateButtonMask( &s_currentJoyStickButtonMask, ButtonMask_Down, yPos > value );
		}
		break; 

	case MM_JOY1BUTTONDOWN:
		if( (uint)wParam & JOY_BUTTON1 ) 
		{ 
			//SYS_TRACE_DEBUG( "DOWN\n" );
			updateButtonMask( &s_currentJoyStickButtonMask, ButtonMask_PlaceBomb, TRUE );
		} 
		break; 

	case MM_JOY1BUTTONUP:
		if( (uint)wParam & JOY_BUTTON1CHG ) 
		{ 
			//SYS_TRACE_DEBUG( "UP\n" );
			updateButtonMask( &s_currentJoyStickButtonMask, ButtonMask_PlaceBomb, FALSE );
		} 
		break; 

	case WM_SYSCOMMAND:
		if( wParam==SC_SCREENSAVE || wParam==SC_MONITORPOWER )
		{
			return 0;
		}
		break;

	case WM_CLOSE:
	case WM_DESTROY:
		{
			PostQuitMessage( 0 );
			return 0;
		}
		break;

	case WM_KEYDOWN:
	case WM_KEYUP:
		{
			const short ctrlPressed = GetAsyncKeyState( VK_CONTROL );

			switch( wParam )
			{
			case VK_ESCAPE:
				PostQuitMessage( 0 );
				break;

			case VK_LEFT:
				updateButtonMask( &s_currentButtonMask, ctrlPressed ? ButtonMask_CtrlLeft : ButtonMask_Left, uMsg == WM_KEYDOWN );
				break;

			case VK_RIGHT:
				updateButtonMask( &s_currentButtonMask, ctrlPressed ? ButtonMask_CtrlRight : ButtonMask_Right, uMsg == WM_KEYDOWN );
				break;

			case VK_UP:
				updateButtonMask( &s_currentButtonMask, ctrlPressed ? ButtonMask_CtrlUp : ButtonMask_Up, uMsg == WM_KEYDOWN );
				break;

			case VK_DOWN:
				updateButtonMask( &s_currentButtonMask, ctrlPressed ? ButtonMask_CtrlDown : ButtonMask_Down, uMsg == WM_KEYDOWN );
				break;

			case VK_SPACE:
				updateButtonMask( &s_currentButtonMask, ButtonMask_PlaceBomb, uMsg == WM_KEYDOWN );
				break;		

			case 'A':
				updateButtonMask( &s_currentButtonMask, ButtonMask_Player2Left, uMsg == WM_KEYDOWN );
				break;

			case 'D':
				updateButtonMask( &s_currentButtonMask, ButtonMask_Player2Right, uMsg == WM_KEYDOWN );
				break;

			case 'W':
				updateButtonMask( &s_currentButtonMask, ButtonMask_Player2Up, uMsg == WM_KEYDOWN );
				break;

			case 'S':
				updateButtonMask( &s_currentButtonMask, ButtonMask_Player2Down, uMsg == WM_KEYDOWN );
				updateButtonMask( &s_currentButtonMask, ButtonMask_Server, uMsg == WM_KEYDOWN );
				break;

			case 'C':
				updateButtonMask( &s_currentButtonMask, ButtonMask_Client, uMsg == WM_KEYDOWN );
				break;

			case 'L':
				updateButtonMask( &s_currentButtonMask, ButtonMask_Leave, uMsg == WM_KEYDOWN );
				break;

			case VK_TAB:
				updateButtonMask( &s_currentButtonMask, ButtonMask_Player2PlaceBomb, uMsg == WM_KEYDOWN );
				break;		
			}
		}
		break;
    }

    return DefWindowProc( hWnd, uMsg, wParam, lParam );
}

static void	__cdecl soundThreadFunction( void* )
{
	SYS_TRACE_DEBUG( "start thread\n" );

	const uint halfBufferSize = SoundBufferSampleHalfCount * SoundChannelCount * SoundSampleSize;

	for(;;)
	{
		LPVOID lpvAudio1 = NULL;
		LPVOID lpvAudio2 = NULL;
		DWORD dwBytesAudio1 = 0;
		DWORD dwBytesAudio2 = 0;

		const DWORD hr = WaitForMultipleObjects(2, s_soundEvents, FALSE, INFINITE );
		uint bufferIndex;

		if( WAIT_OBJECT_0 == hr ) 
		{
			bufferIndex = 1;
		}
		else if( WAIT_OBJECT_0 + 1 == hr ) 
		{		
			bufferIndex = 0;
		}
		else 
		{
			SYS_TRACE_DEBUG( "exit thread\n" );
			return;
		}

		if( FAILED( s_pDxSoundBuffer->Lock( bufferIndex * halfBufferSize, halfBufferSize, &lpvAudio1, &dwBytesAudio1, &lpvAudio2, &dwBytesAudio2, 0 ) ) ) 
		{
			SYS_TRACE_ERROR( "lock %d failed\n", bufferIndex );
			return;
		}		

		float2 fbuffer[ SoundBufferSampleHalfCount ];
		sound_fillBuffer( fbuffer, SoundBufferSampleHalfCount );

		int16* pBuffer = (int16*)lpvAudio1;
		for( uint i = 0u; i < SoundBufferSampleHalfCount; ++i )
		{
			*pBuffer++ = (int16)( fbuffer[ i ].x * 32768.0f ); 
			*pBuffer++ = (int16)( fbuffer[ i ].y * 32768.0f ); 
		}

		//static float time = 0.0f;
		//int16* pBuffer = (int16*)lpvAudio1;
		//const float freq = 2000.0f;
		//for( uint i = 0u; i < SoundBufferSampleHalfCount; ++i )
		//{
		//	*pBuffer++ = (int16)( cosf( time * freq * 2.0f * 3.14159265f ) * 32000.0f );
		//	*pBuffer++ = (int16)( cosf( time * freq * 2.0f * 3.14159265f ) * 32000.0f );

		//	time += ( 1.0f / 44100.0f );
		//}
		
		SYS_ASSERT( lpvAudio2 == NULL );

		s_pDxSoundBuffer->Unlock( lpvAudio1, dwBytesAudio1, lpvAudio2, dwBytesAudio2 );
	}
}

static void dxsound_init()
{
	s_soundEvents[ 0u ] = CreateEvent( NULL, FALSE, FALSE, "NOTIFY0" );
	s_soundEvents[ 1u ] = CreateEvent( NULL, FALSE, FALSE, "NOTIFY1" );

	if( FAILED( DirectSoundCreate( NULL, &s_pDxSound, NULL ) ) ) 
	{
		SYS_TRACE_ERROR( "dxs create\n" );
		sys_exit( 1 );
	}

	if( FAILED( s_pDxSound->SetCooperativeLevel( s_hWnd, DSSCL_PRIORITY ) ) ) 
	{
		SYS_TRACE_ERROR( "dxs coop\n" );
		sys_exit( 1 );
	}

	DSBUFFERDESC dsbd;
	ZeroMemory( &dsbd, sizeof( dsbd ) );
	dsbd.dwSize = sizeof( DSBUFFERDESC );
	dsbd.dwFlags = DSBCAPS_PRIMARYBUFFER;
	dsbd.dwBufferBytes = 0;
	dsbd.lpwfxFormat = NULL;

	LPDIRECTSOUNDBUFFER primaryBuffer = NULL;
	if( FAILED( s_pDxSound->CreateSoundBuffer( &dsbd, &primaryBuffer, NULL ) ) ) 
	{
		SYS_TRACE_ERROR( "dxs buffer\n" );
		sys_exit( 1 );
	}
	
	s_waveFormat.wFormatTag			= WAVE_FORMAT_PCM; 
	s_waveFormat.nChannels			= SoundChannelCount; 
	s_waveFormat.nSamplesPerSec		= SoundSampleRate; 
	s_waveFormat.nAvgBytesPerSec	= SoundSampleRate * SoundChannelCount * SoundSampleSize;
	s_waveFormat.nBlockAlign		= SoundChannelCount * SoundSampleSize;
	s_waveFormat.wBitsPerSample		= SoundSampleSize * 8u;
	s_waveFormat.cbSize				= 0u; 

	if( FAILED( primaryBuffer->SetFormat( &s_waveFormat ) ) ) 
	{
		SYS_TRACE_ERROR( "dxs format\n" );
		sys_exit( 1 );
	}
	
	dsbd.dwFlags		= DSBCAPS_CTRLPOSITIONNOTIFY | DSBCAPS_GLOBALFOCUS;
	dsbd.dwBufferBytes	= SoundBufferSampleCount * SoundChannelCount * SoundSampleSize;
	dsbd.lpwfxFormat	= &s_waveFormat;

	if( FAILED( s_pDxSound->CreateSoundBuffer( &dsbd, &s_pDxSoundBuffer, NULL ) ) ) 
	{
		SYS_TRACE_ERROR( "dxs buffer2\n" );
		sys_exit( 1 );
	}

	LPDIRECTSOUNDNOTIFY lpDSBNotify;
	if( FAILED( s_pDxSoundBuffer->QueryInterface( IID_IDirectSoundNotify, (LPVOID*)&lpDSBNotify ) ) ) 
	{
		SYS_TRACE_ERROR( "dxs buffer notify\n" );
		sys_exit( 1 );
	}

	s_pDxSoundBuffer->SetVolume( DSBVOLUME_MAX );

	const uint soundBufferSize = SoundBufferSampleCount * SoundChannelCount * SoundSampleSize;

	DSBPOSITIONNOTIFY pPosNotify[ 2u ];
	pPosNotify[ 0u ].dwOffset = ( soundBufferSize / 4u );
	pPosNotify[ 1u ].dwOffset = ( soundBufferSize / 4u ) * 3u;	
	pPosNotify[ 0u ].hEventNotify = s_soundEvents[ 0u ];
	pPosNotify[ 1u ].hEventNotify = s_soundEvents[ 1u ];	

	const int result = lpDSBNotify->SetNotificationPositions( 2u, pPosNotify );
	if( FAILED( result ) ) 
	{ 
		SYS_TRACE_ERROR( "dxs buffer notify pos\n" );
		sys_exit( 1 );
	}

	s_soundThreadHandle = (void*)_beginthread( &soundThreadFunction, 1000000u, NULL );
	SetThreadPriority( s_soundThreadHandle, THREAD_PRIORITY_HIGHEST );

	s_pDxSoundBuffer->Play( 0, 0, DSBPLAY_LOOPING );
}

static void dxsound_done()
{
	s_pDxSoundBuffer->Stop();
}

int WINAPI WinMain( HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow )
{
	SYS_USE_ARGUMENT( hInstance );
	SYS_USE_ARGUMENT( hPrevInstance );
	SYS_USE_ARGUMENT( lpCmdLine );
	SYS_USE_ARGUMENT( nCmdShow );

	const char* pWndClass = "paperbomb_wc";

    WNDCLASS wc;
    memset( &wc, 0, sizeof(WNDCLASS) );

    wc.style         = CS_OWNDC | CS_HREDRAW | CS_VREDRAW;
    wc.lpfnWndProc   = WndProc;
    wc.hInstance     = hInstance;
    wc.lpszClassName = pWndClass;
    //wc.hbrBackground=(HBRUSH)CreateSolidBrush(0x00785838);
	
    SYS_VERIFY( RegisterClass( &wc ) );

	DEVMODE devmode;
	bool fullscreen = false;

	if( nCmdShow == 42 )
	{
		fullscreen = true;
	}

	DWORD dwExStyle, dwStyle;
    if( fullscreen )
    {
        memset( &devmode, 0, sizeof( DEVMODE ) );
        devmode.dmSize       = sizeof( DEVMODE );
        devmode.dmFields     = DM_BITSPERPEL | DM_PELSWIDTH | DM_PELSHEIGHT;
        devmode.dmBitsPerPel = 32;
        devmode.dmPelsWidth  = s_width;
        devmode.dmPelsHeight = s_height;

        SYS_VERIFY( ChangeDisplaySettings( &devmode,CDS_FULLSCREEN ) == DISP_CHANGE_SUCCESSFUL );

		dwExStyle = WS_EX_APPWINDOW;
        dwStyle   = WS_VISIBLE | WS_POPUP | WS_CLIPSIBLINGS | WS_CLIPCHILDREN;

		while( ShowCursor( 0 ) >=0 );	// hide cursor
    }
    else
    {
        dwExStyle = WS_EX_APPWINDOW | WS_EX_WINDOWEDGE;
        dwStyle   = WS_VISIBLE | WS_CAPTION | WS_CLIPSIBLINGS | WS_CLIPCHILDREN | WS_SYSMENU;
    }

    
    RECT rect;

    rect.left   = 0;
    rect.top    = 0;
    rect.right  = (int)s_width;
    rect.bottom = (int)s_height;

    AdjustWindowRect( &rect, dwStyle, 0 );

    s_hWnd = CreateWindowEx( dwExStyle, wc.lpszClassName, wc.lpszClassName, dwStyle,
                               (GetSystemMetrics(SM_CXSCREEN)-rect.right+rect.left)>>1,
                               (GetSystemMetrics(SM_CYSCREEN)-rect.bottom+rect.top)>>1,
                               rect.right-rect.left, rect.bottom-rect.top, 0, 0, hInstance, 0 );

    SYS_VERIFY( s_hWnd );

	HDC hDC = GetDC( s_hWnd );
	SYS_VERIFY( hDC );

	static const PIXELFORMATDESCRIPTOR pfd =
	{
		sizeof(PIXELFORMATDESCRIPTOR),
		1,
		PFD_DRAW_TO_WINDOW | PFD_SUPPORT_OPENGL | PFD_DOUBLEBUFFER,
		PFD_TYPE_RGBA,
		32,
		0, 0, 0, 0, 0, 0, 0, 0,
		0, 0, 0, 0, 0,
		32,             // zbuffer
		0,              // stencil!
		0,
		PFD_MAIN_PLANE,
		0, 0, 0, 0
	};

	int pixelFormat = ChoosePixelFormat( hDC, &pfd );
    SYS_VERIFY( pixelFormat );

    SYS_VERIFY( SetPixelFormat( hDC, pixelFormat, &pfd ) );
	HGLRC hRC = wglCreateContext( hDC );
    SYS_VERIFY( hRC );

    SYS_VERIFY( wglMakeCurrent( hDC, hRC ) );

	SYS_VERIFY( glewInit() == GLEW_OK );

	dxsound_init();
	game_init();
   
    uint32 lastTime = timeGetTime();

#ifndef SYS_BUILD_MASTER
    uint32 lastTimingTime = lastTime;
    uint32 timingFrameCount = 0u;
#endif

    int quit = 0;
    do
    {
        uint32 currentTime = timeGetTime();

        const float timeStep = ( float )( currentTime - lastTime ) / 1000.0f;
        lastTime = currentTime;

#ifndef SYS_BUILD_MASTER
        if( currentTime - lastTimingTime > 500u )
        {
            const float timingTimeSpan = ( float )( currentTime - lastTimingTime ) / 1000.0f;
            const float currentFps = ( float )( timingFrameCount ) / timingTimeSpan;

            //SYS_TRACE_DEBUG( "fps=%f\n", currentFps );

            char windowTitle[ 100u ];
            sprintf_s( windowTitle, sizeof( windowTitle ), "fps=%f", currentFps );
			SetWindowText( s_hWnd, windowTitle );            
    
            lastTimingTime = currentTime;
            timingFrameCount = 0u;
        }
#endif

		MSG msg;
        while( PeekMessage( &msg, 0, 0, 0, PM_REMOVE ) )
        {
            if( msg.message == WM_QUIT )
			{
				quit = 1;
			}
		    TranslateMessage( &msg );
            DispatchMessage( &msg );
        }

        GameInput gameInput;
        memset( &gameInput, 0u, sizeof( gameInput ) );
        gameInput.timeStep = timeStep;
        gameInput.buttonMask = s_currentButtonMask | s_currentJoyStickButtonMask;

        game_update( &gameInput );
        game_render();

		SwapBuffers( hDC );       

#ifndef SYS_BUILD_MASTER
        timingFrameCount++;
#endif
    }
    while( !quit );

    game_done();
	dxsound_done();

    return( 0 );
}
