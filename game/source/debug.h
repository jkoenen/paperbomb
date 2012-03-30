#ifndef DEBUG_H_INCLUDED
#define DEBUG_H_INCLUDED

#ifdef SYS_TRACE_ENABLED

enum
{
    TraceLevel_Emergency    = 1,
    TraceLevel_Error        = 10,
    TraceLevel_Warning      = 20,
    TraceLevel_Info         = 30,
    TraceLevel_Debug        = 100
};

void sys_trace( int level, const char* pFormat, ... );
void sys_exit( int extCode );

#   define SYS_TRACE_EMERGENCY(format,args...)  sys_trace( TraceLevel_Emergency, format, ## args)
#   define SYS_TRACE_ERROR(format,args...)      sys_trace( TraceLevel_Error, format, ## args)
#   define SYS_TRACE_WARNING(format,args...)    sys_trace( TraceLevel_Warning, format, ## args)
#   define SYS_TRACE_INFO(format,args...)       sys_trace( TraceLevel_Infro, format, ## args)
#   define SYS_TRACE_DEBUG(format,args...)      sys_trace( TraceLevel_Debug, format, ## args)
#else
#   ifdef _MSC_VER
#      define SYS_TRACE_EMERGENCY	__noop
#      define SYS_TRACE_ERROR		__noop
#      define SYS_TRACE_WARNING	__noop
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
#   define SYS_ASSERT(Expression)	if( !( Expression ) ) { SYS_TRACE_ERROR( "Assertion failed: %s\n", # Expression ); sys_exit( 1 ); };
#   define SYS_BREAK(...)			sys_exit( 1 );
#	define SYS_VERIFY(Expression)	SYS_ASSERT(Expression)
#else
#   define SYS_ASSERT(Expression)	(void)(Expression)
#   define SYS_BREAK(...)			sys_exit( 1 );
#	define SYS_VERIFY(Expression)	(Expression)
#endif

#endif

