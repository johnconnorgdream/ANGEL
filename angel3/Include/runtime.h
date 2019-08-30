#ifndef runtime_def
#define runtime_def
#ifdef _cplusplus
extern "c"{
#endif



#include "data.h"

#define FLAG_STACKHEAP 2
#define FLAG_CONST 3
#define FLAG_POOL 4
#define FLAG_HASH_TEST 5
#define FLAG_HEAPED 6


#define ISHEAPINSTACK(o) ((o->flag) == FLAG_STACKHEAP)
#define ISCONST(o) ((o->flag) == FLAG_CONST)
#define ISPOOL(o) ((o->flag) == FLAG_POOL)
#define ISHASHTEST(o) (o->flag == FLAG_HASH_TEST)
#define ISHEAPED(o) (o->flag == FLAG_HEAPED)
#define ISNORMAL(o) (o->flag == 0)


//之所以用相同的位模式来标记，是因为各自标记的作用于都是暂时性有效，而且各自有效作用的生命周期不会交叉
#define IS_PRINTED (1)

#define IS_DECED (2)
//这是针对可达性分析的标志位，注意这个标志位在进行recovery之后就会还原，在gc过程中不会与IS_DECED冲突
//因为在gc过程中只要遇到IS_FLAGED就不会被GC，自然就用不到
#define IS_FLAGED (3)

//暂时标记，做循环应用检测
#define LOOP_CHECK_FLAG (4)

//标记是否是循环引用
#define IS_LOOPED (5)


#define ISFLAGED(o) ((o->extra_flag) == IS_FLAGED)
#define ISDECED(o) ((o->extra_flag) == IS_DECED)
#define ISPRINTED(o) ((((object)o)->extra_flag) == IS_PRINTED)
#define ISCHECKING(o) ((((object)o)->extra_flag) == LOOP_CHECK_FLAG)
#define ISLOOPED(o) ((((object)o)->extra_flag) == IS_LOOPED)


#define STACKTOHEAP(o) o = (object)stacktoheap(o);
#define stack_heap_size NUMSIZE
#define GETSTACKHEAPASINT(base,i) (object_int)((char *)base+(i*stack_heap_size))



typedef struct _switchnode{
	unsigned long hash,offset;
	char type;
}*_switch;
typedef struct switchtablenode{
	_switch *sw_item;
	uint16_t len;
}*switchtable;
typedef struct switchlistnode{
	uint16_t len;
	switchtable st_table[runtime_max_size];
}*switchlist;

typedef struct linkcollectionnode{
	void *data;
	linkcollectionnode *next,*pre;
}*linkcollection;
typedef struct funnode{
	char *name;
	pclass class_context;
	uint16_t localcount,default_paracount,paracount,type,index,temp_var_num;  //这里记录的是入口参数和默认参数的个数。这两个字段只在判断函数是否定义重复或者调用成功的时候才起作用. 
	  //type在这里表示函数的类型，即可能是构造函数，普通函数或成员函数,静态成员函数，他们的表示分别为1,0,2,s3
	object_set local_v_map;  //函数内局部变量的映射关系全在这里，在解析函数时候先将其映射的变量名给填满，再在生成字节码的时候将偏移量给写入
	token grammar,default_para;
	linkcollection local_scope,current_scope;
	unsigned long *base_addr;
	funnode *overload;  //这个只想重载函数，每次需要定位到第一个函数后就可以对链表顺序遍历所有的函数块
}*fun;
typedef struct funlistnode{
	fun *fun_item;
	int alloc_size,len;
}*funlist;
typedef struct indexlistnode{
	uint16_t *item;
	uint16_t len,alloc;
}*indexlist;

typedef struct env_elenode{
	char *pc_reg;
	fun bf;
	object *baseaddr;
	uint16_t ret_temp_num;
}*env_ele;
typedef struct envnode{
	env_ele env_item[runtime_max_size];
	int len;
}*env;
//函数实际地址和偏移量的映射可以用一个顺序表来实现
typedef struct codemapnode{
	uint16_t length;
	unsigned long head_addr[runtime_max_size];
}*codemap;
typedef struct runtime_stacknode{
	object *data;
	void *stack_heap_base;
	int top,push_pos;
	int stack_size;
} *runtime_stack;  //运行时栈

typedef struct regallocnode{
	unsigned char regno[256];
	int top;
}*regalloc;





runtime_stack initruntime(int stacksize = runtime_max_size);
void freeruntime(runtime_stack r);
int stackempty_runtime(runtime_stack s);
object gettop(runtime_stack s);
void push(runtime_stack s,object v);
object pop(runtime_stack s);




indexlist initindexlist();
void addindexlist(indexlist base,uint16_t s);
funlist initfunlist();
void _addfun(funlist fl,fun f);
int addfun(funlist fl,object_set fmap,fun f);
env initenv();
void freeenv(env ev);

object_entry initentry(object key,object value);



void *popcollection(collection c);
collection initcollection(int count = list_base_size);
void addcollection(collection base,void *el);
void clearcollection(collection c);
linkcollection initlink();
void addlink(linkcollection head,linkcollection item);
void clearlink(linkcollection head);
void deletelink(linkcollection node);


pclass initclass(char *name);
int getclassoffset(char *classname);
int getfunoffset(funlist head,object_set fmap,char *funname,int count);
fun getfun(funlist head,object_set fmap,char *funname,int count = -1);
fun getglobalfun(char *funname,int count = -1);


#ifdef _cplusplus
}
#endif
#endif