#ifndef OPENGL_H__INCLUDED
#define OPENGL_H__INCLUDED

#ifdef SYS_PLATFORM_WIN32

#include "win32/win32_pre.h"
#include <GL/glew.h>
#include "win32/win32_post.h"

#else

#include <GL/gl.h>
#include <GL/glext.h>

#endif

#endif // KEEN__OPENGL_H__INCLUDED
