
#include "types.h"
#include "debug.h"
#include "game.h"
#include "input.h"

#include <stdio.h>
#include <stdarg.h>

#define WIN32_LEAN_AND_MEAN
#define WIN32_EXTRA_LEAN
#include <Windows.h>
#include <mmsystem.h>
#include <GL/glew.h>

static uint s_width = 800;
static uint s_height = 600;

static uint32 s_currentButtonMask = 0u;

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
			}
		}
		break;
    }

    return DefWindowProc( hWnd, uMsg, wParam, lParam );
}
// 
// static void DrawTime( WININFO *info, float t )
// {
//     static int      frame=0;
//     static float    to=0.0;
//     static int      fps=0;
//     char            str[64];
//     int             s, m, c;
// 
//     if( t<0.0f) return;
//     if( info->full ) return;
// 
//     frame++;
//     if( (t-to)>1.0f )
//     {
//         fps = frame;
//         to = t;
//         frame = 0;
//     }
// 
//     if( !(frame&3) )
//     {
//         m = msys_ifloorf( t/60.0f );
//         s = msys_ifloorf( t-60.0f*(float)m );
//         c = msys_ifloorf( t*100.0f ) % 100;
//         sprintf( str, "%02d:%02d:%02d  [%d fps]", m, s, c, fps );
//         SetWindowText( info->hWnd, str );
//     }
// }


int WINAPI WinMain( HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow )
{
	SYS_USE_ARGUMENT( hInstance );
	SYS_USE_ARGUMENT( hPrevInstance );
	SYS_USE_ARGUMENT( lpCmdLine );
	SYS_USE_ARGUMENT( nCmdShow );

	const uint width = 800u;
	const uint height = 600u;
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
        memset( &devmode,0,sizeof(DEVMODE) );
        devmode.dmSize       = sizeof(DEVMODE);
        devmode.dmFields     = DM_BITSPERPEL|DM_PELSWIDTH|DM_PELSHEIGHT;
        devmode.dmBitsPerPel = 32;
        devmode.dmPelsWidth  = 800;
        devmode.dmPelsHeight = 600;

        SYS_VERIFY( ChangeDisplaySettings( &devmode,CDS_FULLSCREEN ) == DISP_CHANGE_SUCCESSFUL );

		dwExStyle = WS_EX_APPWINDOW;
        dwStyle   = WS_VISIBLE | WS_POPUP | WS_CLIPSIBLINGS | WS_CLIPCHILDREN;

		while( ShowCursor( 0 )>=0 );	// hide cursor
    }
    else
    {
        dwExStyle = WS_EX_APPWINDOW | WS_EX_WINDOWEDGE;
        dwStyle   = WS_VISIBLE | WS_CAPTION | WS_CLIPSIBLINGS | WS_CLIPCHILDREN | WS_SYSMENU;
    }

    
    RECT rect;

    rect.left   = 0;
    rect.top    = 0;
    rect.right  = width;
    rect.bottom = height;

    AdjustWindowRect( &rect, dwStyle, 0 );

    HWND hWnd = CreateWindowEx( dwExStyle, wc.lpszClassName, wc.lpszClassName, dwStyle,
                               (GetSystemMetrics(SM_CXSCREEN)-rect.right+rect.left)>>1,
                               (GetSystemMetrics(SM_CYSCREEN)-rect.bottom+rect.top)>>1,
                               rect.right-rect.left, rect.bottom-rect.top, 0, 0, hInstance, 0 );

    SYS_VERIFY( hWnd );

	HDC hDC = GetDC( hWnd );
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

            SYS_TRACE_DEBUG( "fps=%f\n", currentFps );

            char windowTitle[ 100u ];
            sprintf_s( windowTitle, sizeof( windowTitle ), "fps=%f", currentFps );
			SetWindowText( hWnd, windowTitle );            
    
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
        gameInput.buttonMask = s_currentButtonMask;

        game_update( &gameInput );
        game_render();

		SwapBuffers( hDC );       

#ifndef SYS_BUILD_MASTER
        timingFrameCount++;
#endif
    }
    while( !quit );
    game_done();

    return( 0 );
}
