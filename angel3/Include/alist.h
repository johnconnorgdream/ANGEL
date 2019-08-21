#ifndef list_def
#define list_def

#ifdef _cplusplus
extern "c"{
#endif


#include "aexpimp.h"
#include "arange.h"


typedef struct object_listnode{
	BASEDEF
	object *item;
	unsigned long len,alloc_size;  //尽可能增加大的列表数目。
}*object_list;



void expendlistsize(object_list ol,int resizelen);



object_list initarray(int count = list_base_size);
int _addlist(object_list l,object item);
int addlist(object_list l,object item);
void clearlist(object_list l);
int appendlist(object_list l,object_list m);
object_list concatlist(object_list l,object_list m);
int insertlist(object_list l,object_list m,int pos);
object_list copylist(object_list l);
object_list slicelist(object_list l,object_range range);
void storeslicelist(object_list l,object_range range,object_list m);
int storeslicelist_asslice(object_list l,object_range targetrange,object_slice slice);
object_list listrepeat(object_list l,int count);

/*
数组库
*/
object syssize_list(object o);
object sysadd_list(object item,object o);
object sysextend_list(object extend,object o);
object syspop_list(object o);


#ifdef _cplusplus
}
#endif
#endif