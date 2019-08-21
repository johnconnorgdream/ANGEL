#ifndef execute_def
#define execute_def
#ifdef _cplusplus
extern "c"{
#endif


#ifndef AMEM_MOUDLE
extern unsigned int current_thread;
#else
unsigned int current_thread;
#endif
#ifndef EXECUTE_MODULE
extern int angel_thread_count;
#else
int angel_thread_count;
#endif


#include "aobject.h"


//angel解释器内部枷锁的地方所需要的运行时间不长，运行时间长的可以用angel_lock
inline void fast_unlock()
{
	__asm{
		mov current_thread,0x00;
	}
}
inline void fast_lock()
{
	__asm{
		mov ecx,0x01;
aquire:
		mov eax,0x00;
		lock cmpxchg current_thread,ecx;
		jnz aquire;
	}
}
//系统锁
void critical_init();
void critical_enter();
void critical_leave();


/*
提供给angel程序使用的
*/
void angel_lock();
void angel_unlock();




typedef struct runablenode{
	object_ext pthread;
	object_fun func;
	object_list argc;
	runtime_stack _stack;
	int thread_type;
}*runable;


void init_stack_list();
void waitforthread();
linkcollection alloc_thread();
void free_runable(linkcollection controll);


void exec(linkcollection thread=NULL);
void stopworld();
void goahead();





inline void decref(object o)
{
	(o)->refcount--;
}
#define _DECREF(o) decref((object)o);
#define DECREF(o) if(o) {_DECREF(o)}//if(o->refcount == FLAG_FLAGCLEAN) o->refcount = 0;
#define ASCREF(o) if(o) (o)->refcount++;
__forceinline int getmax(int a,int b)
{
	return a > b ? a : b;
}
__forceinline int getmin(int a,int b)
{
	return a < b ? a : b;
}
__forceinline int isparamvalid(int param_count,int default_count,int input_count)
{
	if(input_count >= param_count - default_count && input_count <= param_count)
		return 1;
	return 0;
}
#define RANGETOSLICE(range,len,s,b,e,n) \
	do{ \
		s = range->step; \
		if(s > 0) {\
			b = getmax(range->begin, 0); \
			e = getmin(range->end, len); \
		}else{ \
			b = getmin(range->begin,len); \
			e = getmax(range->end, 0); \
		} \
		n = (e - b)/s + 1; \
	}while(0);
__forceinline void _assign_execute(object *angel_inplace_addr_reg,object value)
{
	*angel_inplace_addr_reg = value;
}
__forceinline void _assign_execute_ref(object *angel_inplace_addr_reg,object value)
{
	register object temp = *angel_inplace_addr_reg;
	DECREF(temp);
	value->refcount++;
	*angel_inplace_addr_reg = value;
}
__forceinline void assign_execute(object *base,int offset,object value)
{
	register object *angel_inplace_addr_reg = &base[offset];
	register object temp = *angel_inplace_addr_reg;
	DECREF(temp);
	value->refcount++;
	*angel_inplace_addr_reg = value;
}


#ifdef _cplusplus
}
#endif
#endif