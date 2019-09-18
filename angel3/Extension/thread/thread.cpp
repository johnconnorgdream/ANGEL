#ifdef WIN32 //WINDOWS
#include <process.h>
#include <Windows.h>
#endif

#include "_common.h"
#include "_thread.h"


extern linkcollection angel_stack_list;


extern ext_type thread_type;


void threaddealloc(object o)
{
#ifdef WIN32
	HANDLE hthread = GETTHREAD(o)->handler;
	CloseHandle(hthread);
#else
#endif
}
void stopworld()
{
#ifdef WIN32
	DWORD cthread = GetCurrentThreadId();
#else
#endif
	for(linkcollection p = angel_stack_list->next; p != angel_stack_list; p = p->next)
	{
		runable	run = (runable)p->data;
		object_thread th = (object_thread)run->pthread;
#ifdef WIN32
		if(cthread != GetThreadId(th->handler))
			SuspendThread(th->handler);
#else
#endif
		
	}
}
void goahead()
{
#ifdef WIN32
	DWORD cthread = GetCurrentThreadId();
#else
#endif
	for(linkcollection p = angel_stack_list->next; p != angel_stack_list; p = p->next)
	{
		runable	run = (runable)p->data;
		object_thread th = (object_thread)run->pthread;
#ifdef WIN32
		if(cthread != GetThreadId(th->handler))
			ResumeThread(th->handler);
#else
#endif
		
	}
}




object_ext getcurrentthread()
{
	object_thread ret = (object_thread)initext(sizeof(object_threadnode));
	ret->_type = thread_type;
	ret->flag = FLAG_CONST;
#ifdef WIN32
	ret->handler = GetCurrentThread();
#else
#endif
	return (object_ext)ret;
}
unsigned _stdcall bootstrap(void *argc)
{
	linkcollection run = (linkcollection)argc;
	exec(run);
	return 0;
}
object sysstartthread(object func,object argc,object ttype,object mode)
{
	object_thread ret = (object_thread)initext(sizeof(object_threadnode));
	ret->_type = thread_type;
	int initflag = 0,typeflag = NON_DAEMON_TYPE;  //默认

	ARG_CHECK(func,FUNP,"startthread",1)
	ARG_CHECK(argc,LIST,"startthread",2)
	if(!ISDEFAULT(ttype)){
		ARG_CHECK(ttype,BOOLEAN,"startthread",3)
		//这里true表示的是守护线程。否则需要阻塞,默认为false
		if(ISTRUE(ttype))
		{
			typeflag = DAEMON_TYPE;
		}
	}
	if(!ISDEFAULT(mode)){
		ARG_CHECK(mode,BOOLEAN,"startthread",4)
		//这里true表示的是需要创建后立即执行。否则需要阻塞,默认为true
		if(ISFALSE(mode))
		{
			initflag = CREATE_SUSPENDED;
		}
	}
	object_list argc_l = GETLIST(argc);

	
	linkcollection thread_controll = alloc_thread();
	runable run = (runable)thread_controll->data;

	ASCREF(argc_l);
	ASCREF(func);
	ASCREF(ret);

	run->argc = argc_l;
	run->func = GETFUN(func);
	run->thread_type = typeflag;
	run->pthread = (object_ext)ret;

#ifdef WIN32
	unsigned int id;
	HANDLE hthread = (HANDLE)_beginthreadex(NULL,0,bootstrap,thread_controll,initflag,&id);
	ret->handler = hthread;
#else
#endif
	return (object)ret;
}
/*
void exit_thread()
{
	_endthreadex(0);
}
object systhreadstate(object_thread thobj)
{
	unsigned long state;
	object ret;
#ifdef WIN32

	if(!GetExitCodeThread(thobj,&state))
		ret = (object)initinteger(-1);
	else
		ret = (object)initinteger(state);
#else
#endif
	return ret;
}
*/