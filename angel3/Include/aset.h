#ifndef set_def
#define set_def
#ifdef _cplusplus
extern "c"{
#endif




#include "aexpimp.h"
typedef struct object_setnode{
	BASEDEF
	object *element;
	unsigned long len,alloc_size;  //尽可能增加大的列表数目。
}*object_set;

object_set initset(int count = list_base_size);
long getsetindex(object_set s,object o);
void _addset(object_set s,object o);
void addset(object_set s,object o);
int unionset(object_set s,object_set s1);
object_set concatset(object_set s,object_set s1);
object_set copyset(object_set os);




void resizeset(object_set os);


/*
集合库
*/
object syssize_set(object o);
object sysadd_set(object item,object o);
object sysisexist_set(object item,object o);
object sysremove_set(object item,object o);
void translisttoset(object_set set,object_list l);
void transrangetoset(object_set set,object_range r);



#ifdef _cplusplus
}
#endif
#endif