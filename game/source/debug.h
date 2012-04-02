#ifndef DEBUG_H_INCLUDED
#define DEBUG_H_INCLUDED

#include "types.h"

#ifdef SYS_TRACE_ENABLED

enum
{
    TraceLevel_Emergency    = 1,
    TraceLevel_Error        = 10,
    TraceLevel_Warning      = 20,
    TraceLevel_Info         = 30,
    TraceLevel_Debug        = 100
};

void sys_trace( const char* pFormat, ... );
void sys_exit( int extCode ) SYS_NO_RETURN;

#   define SYS_TRACE_EMERGENCY  sys_trace
#   define SYS_TRACE_ERROR		sys_trace
#   define SYS_TRACE_WARNING	sys_trace
#   define SYS_TRACE_INFO		sys_trace
#   define SYS_TRACE_DEBUG		sys_trace
#else
#   ifdef _MSC_VER
#      define SYS_TRACE_EMERGENCY	__noop
#      define SYS_TRACE_ERROR		__noop
#      define SYS_TRACE_WARNING		__noop
#      define SYS_TRACE_INFO		__noop
#       define SYS_TRACE_DEBUG		__noop
#   else
#       define SYS_TRACE_EMERGENCY(format,...)
#       define SYS_TRACE_ERROR(format,...)
#       define SYS_TRACE_WARNING(format,...)
#       define SYS_TRACE_INFO(format,...)
#       define SYS_TRACE_DEBUG(format,...)
#   endif
#endif

#ifdef SYS_ASSERT_ENABLED
#   define SYS_ASSERT(Expression)	        if(!(Expression)){SYS_TRACE_ERROR("Assertion failed: %s\n", # Expression); sys_exit(1);}
#   define SYS_BREAK(...)			        sys_exit(1);
#	define SYS_VERIFY(Expression)	        SYS_ASSERT(Expression)
#else
#   define SYS_ASSERT(Expression)
#   define SYS_BREAK(...)			        sys_exit(1);
#	define SYS_VERIFY(Expression)	        (void)(Expression)
#endif

#endif

