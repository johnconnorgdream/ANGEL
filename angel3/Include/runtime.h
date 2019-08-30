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


//֮��������ͬ��λģʽ����ǣ�����Ϊ���Ա�ǵ������ڶ�����ʱ����Ч�����Ҹ�����Ч���õ��������ڲ��ύ��
#define IS_PRINTED (1)

#define IS_DECED (2)
//������Կɴ��Է����ı�־λ��ע�������־λ�ڽ���recovery֮��ͻỹԭ����gc�����в�����IS_DECED��ͻ
//��Ϊ��gc������ֻҪ����IS_FLAGED�Ͳ��ᱻGC����Ȼ���ò���
#define IS_FLAGED (3)

//��ʱ��ǣ���ѭ��Ӧ�ü��
#define LOOP_CHECK_FLAG (4)

//����Ƿ���ѭ������
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
	uint16_t localcount,default_paracount,paracount,type,index,temp_var_num;  //�����¼������ڲ�����Ĭ�ϲ����ĸ������������ֶ�ֻ���жϺ����Ƿ����ظ����ߵ��óɹ���ʱ���������. 
	  //type�������ʾ���������ͣ��������ǹ��캯������ͨ�������Ա����,��̬��Ա���������ǵı�ʾ�ֱ�Ϊ1,0,2,s3
	object_set local_v_map;  //�����ھֲ�������ӳ���ϵȫ������ڽ�������ʱ���Ƚ���ӳ��ı����������������������ֽ����ʱ��ƫ������д��
	token grammar,default_para;
	linkcollection local_scope,current_scope;
	unsigned long *base_addr;
	funnode *overload;  //���ֻ�����غ�����ÿ����Ҫ��λ����һ��������Ϳ��Զ�����˳��������еĺ�����
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
//����ʵ�ʵ�ַ��ƫ������ӳ�������һ��˳�����ʵ��
typedef struct codemapnode{
	uint16_t length;
	unsigned long head_addr[runtime_max_size];
}*codemap;
typedef struct runtime_stacknode{
	object *data;
	void *stack_heap_base;
	int top,push_pos;
	int stack_size;
} *runtime_stack;  //����ʱջ

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