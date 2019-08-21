#include "data.h"
#include "runtime.h"
#include "amem.h"
#include "shell.h"
#include "execute.h"

extern object_set global_function_map,class_map;
extern funlist global_function;
object_fun init_function(void *fp,int flag)
{
	object_fun f = (object_fun)angel_alloc_block(APPLYSIZE(sizeof(object_funnode)));
	f->type=FUNP;
	f->extra_flag = flag;
	switch(flag)
	{
	case UNCERTAIN_USER:
	case CERTAIN_USER:
		f->funinfo.user = (fun)fp;
		break ;
	case UNCERTAIN_LIB:
		f->funinfo.sys = (angel_buildin_func *)fp;
		break ;
	}
	return f;
}
object_fun dynamic_get_function(char *name,int16_t count,object obase)  
{
	//count用于给未来需要精确参数个数的函数添加
	int flag = UNCERTAIN_USER;
	void *fpr;
	if(obase == NULL)
	{
		fpr = getfun(global_function,global_function_map,name,count);
		if(!fpr)
		{
			flag = UNCERTAIN_LIB;
			fpr = getglobalsyscall(name,count);			
		}
	}
	else
	{
		fpr = getmemfun(obase,name,count);
		if(!fpr)
		{
			flag = UNCERTAIN_LIB;
			fpr = getsysmembercall(obase,name,count);
		}
	}
	if(fpr)
	{
		return init_function(fpr,flag);
	}
	else
	{
		angel_error("没有合乎要求的函数定义，动态获取函数失败！");
		return NULL;
	}
}
void *dynamic_call(object_fun f,int16_t count)
{
	fun fp;
	angel_buildin_func *lib;
	
	switch(f->extra_flag)
	{
	case UNCERTAIN_USER:
		fp = f->funinfo.user;
		for(fun p = fp; p; p = p->overload)
		{
			if(isparamvalid(p->paracount,p->default_paracount,count))
				return p;
		}
		return NULL;
	case CERTAIN_USER:
		fp = f->funinfo.user;
		if(!isparamvalid(fp->paracount,fp->default_paracount,count))
			return NULL;
		return fp;
	case UNCERTAIN_LIB:
		lib = f->funinfo.sys;
		if(!isparamvalid(lib->argcount,lib->argdefaultcount,count))
			return NULL;
		return lib;
	}
}