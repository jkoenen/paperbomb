#ifndef DEBUG_H_INCLUDED
#define DEBUG_H_INCLUDED

#ifdef PB_TRACE_ENABLED

enum
{
    TraceLevel_Emergency    = 1,
    TraceLevel_Error        = 10,
    TraceLevel_Warning      = 20,
    TraceLevel_Info         = 30,
    TraceLevel_Debug        = 100
};

void debug_trace( int level, const char* pFormat, ... );

#   define PB_TRACE_EMERGENCY(format,args...)  debug_trace( TraceLevel_Emergency, format, ## args)
#   define PB_TRACE_ERROR(format,args...)      debug_trace( TraceLevel_Error, format, ## args)
#   define PB_TRACE_WARNING(format,args...)    debug_trace( TraceLevel_Warning, format, ## args)
#   define PB_TRACE_INFO(format,args...)       debug_trace( TraceLevel_Infro, format, ## args)
#   define PB_TRACE_DEBUG(format,args...)      debug_trace( TraceLevel_Debug, format, ## args)
#else
#   define PB_TRACE_EMERGENCY(format,...)
#   define PB_TRACE_ERROR(format,...)
#   define PB_TRACE_WARNING(format,...)
#   define PB_TRACE_INFO(format,...)
#   define PB_TRACE_DEBUG(format,...)
#endif

#ifdef PB_ASSERT_ENABLED

#   define PB_ASSERT(Expression)

#else

#   define PB_ASSERT(Expression)

#endif

#endif

