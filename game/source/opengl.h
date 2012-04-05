#ifndef OPENGL_H__INCLUDED
#define OPENGL_H__INCLUDED

#ifdef SYS_PLATFORM_WIN32

#ifndef near
#	define near
#endif
#ifndef far
#	define far
#endif
#pragma warning(push)
#pragma warning(disable:4668)
#pragma warning(disable:4201)
#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#include <GL/glew.h>
#pragma warning(pop)
#ifdef min
#	undef min
#endif
#ifdef max
#	undef max
#endif

#else

#include <GL/gl.h>
#include <GL/glext.h>

#endif

#endif
