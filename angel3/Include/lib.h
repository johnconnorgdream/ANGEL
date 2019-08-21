#ifndef lib_def
#define lib_def
#ifdef _cplusplus
extern "c"{
#endif


#include <stdio.h>
#include "aenv.h"
#include "runtime.h"



#define ISDEFAULT(o) ISNOTYPE(o)
//ÓÃ#Á¬½Ó
#define ARG_CHECK(arg,type,funname,index) \
	do{ \
		if(!IS##type(arg)) {\
			char errorinfo[errorsize]; \
			sprintf(errorinfo,"function[%s] argument %d must be %s",funname,index,type##NAME); \
			angel_error(errorinfo); \
			return GETNULL; \
		} \
	}while(0);
#define MEM_ARG_CHECK(o,arg,type,funname,index) \
	do{ \
		if(!IS##type(arg)) {\
			char errorinfo[errorsize]; \
			sprintf(errorinfo,"[%s].[%s] argument %d must be %s",gettypedesc(o),funname,index,type##NAME); \
			angel_error(errorinfo); \
			return GETNULL; \
		} \
	}while(0);

int checkrangeparam(object range,int size,int *res);


typedef struct angel_buildin_func{
	char *name;
	void *handle;
	uint16_t argcount,argdefaultcount,_doc_;
};
#define read_base_size 1000
void init_lib_func_map();

inline void lock_func_return(void *o)
{
	((object)o)->refcount++;
}
inline void release_func_return(void *o)
{
	((object)o)->refcount--;
}
int issysfunbyname(char *funname);
int issysfun(fun f);
int isglobalsyscall(char *funname,int count);
angel_buildin_func *getsysmembercall(object o,char *funname,int count);
angel_buildin_func *getglobalsyscall(char *funname,int count);



#ifdef _cplusplus
}
#endif
#endif