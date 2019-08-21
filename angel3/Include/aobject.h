#ifndef object_def
#define object_def
#ifdef _cplusplus
extern "c"{
#endif

typedef struct object_dictnode _object_dict;
typedef struct object_setnode _object_set;
typedef struct tokennode _token;
typedef struct funnode _fun;
typedef struct object_entrynode entry;
typedef struct object_stringnode _string;

typedef struct classnode{
	//类的继承信息全部再这里记录
	char *name;
	_token *entry_token;
	_object_dict* static_value; //这里是静态成员变量的列表
	struct funlistnode *static_f,*mem_f;
	_object_set* static_f_map,*mem_f_map;
}*pclass;
typedef struct objectnode{  
	//堆暂时就游离在操作系统的堆内存里。对象的个数操作16位所能表示的数字是才报系统错误
	BASEDEF
	pclass c;
	_object_dict *mem_value,*pri_mem_value;  //成员变量用列表的方法实现,之所以不用静态数组是因为经常要创建自定义对象这样会，如何在程序运行中确定上限
	//pri_mem_value是私有成员变量
}*object;
typedef struct classlistnode{
	pclass c[runtime_max_size];
	uint16_t len;
}*classlist;

object initobject();
object stacktoheap(object i);

//查找成员函数
_fun * getobjmemfun(object o,char *name,int count = -1);
_fun * getclassfun(pclass c,char *name,uint16_t count);
_fun * getobjmemfun(object o,char *name,int count);
_fun * getmemfun(object o,char *name,uint16_t count);
entry * getobjmember(object o,_string * name);
entry * getclassmember(pclass c,_string * name);
entry * getmember(object o,_string * name);

#ifdef _cplusplus
}
#endif
#endif