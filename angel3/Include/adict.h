#ifndef dict_def
#define dict_def

#ifdef _cplusplus
extern "c"{
#endif



#include "aexpimp.h"


typedef struct object_entrynode{
	BASEDEF
	object key,value;
}*object_entry;
typedef struct object_dictnode{
	BASEDEF
	object_entry *hashtable;
	unsigned long len,alloc_size;
}*object_dict;
object_dict initdictionary(int count = list_base_size);
object_dict copydict(object_dict d);
int _adddict(object_dict dict,object_entry entry);
int adddict(object_dict dict,object_entry entry);
int adddict(object_dict dict,object key,object value);
int _adddict(object_dict dict,object key,object value);
long getdictindex(object_dict dict,object key);
void storehash(object_dict dict,object_entry entry);





void resizedict(object_dict od);


/*
×Öµä¿â
*/
object syssize_dict(object o);
object sysiskey_dict(object key,object o);
object syskeys_dict(object o);

#ifdef _cplusplus
}
#endif
#endif