#ifndef afunction_def
#define afunction_def
#ifdef _cplusplus
extern "c"{
#endif


#include "lib.h"

#define UNCERTAIN_USER (1<<4)
#define CERTAIN_USER (2<<4)
#define UNCERTAIN_LIB (3<<4)




#define ISUSERFUN(fp) (fp->extra_flag <= CERTAIN_USER)
typedef funnode _fun;
typedef struct angel_buildin_func _buildin;
typedef struct object_funnode{
	BASEDEF;
	union{
		_buildin *sys;
		_fun *user;
	}funinfo;
}*object_fun;

object_fun init_function(void *fp,int flag);
object_fun dynamic_get_function(char *name,int16_t count,object obase);
void *dynamic_call(object_fun f,int16_t count);



#ifdef _cplusplus
}
#endif
#endif