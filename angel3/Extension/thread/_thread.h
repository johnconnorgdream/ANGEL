#ifndef thread_def
#define thread_def

#ifdef _cplusplus
extern "c"{
#endif

#define DAEMON_TYPE 1
#define NON_DAEMON_TYPE 2

#ifndef COMMON_LIB

typedef struct object_threadnode{
	EXT_HEAD;
#ifdef WIN32
	HANDLE handler;
#else
#endif
}*object_thread;

#endif

#define GETTHREAD(o) ((object_thread)o)
#define ISTHREAD(o) (GETEXT(o)->_type == thread_type)
#define THREADNAME "resource<thread>"

object sysstartthread(object func,object argc,object ttype,object mode);
void threaddealloc(object o);

#ifdef _cplusplus
}
#endif

#endif