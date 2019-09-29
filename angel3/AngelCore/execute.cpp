/*
��һ������������߼����㣬������ǿ����������µ����ۼ��裺������һ�����ֻ���ڴ����͸�ֵ���������������������ǿ�����������Զ���ɶ�����Ҫָ���ָʾ��Ҳ�Ͳ���Ҫ�Ĵ����Ĵ��ڡ�
�ڶ��������ȡ��Ԫ�ز��������������ȥ�б����Ԫ�غͶ���ĳ�Ա�����������ֲ�����������ͬһʱ��ֻ��һ���м��������Կ�������һ���������ַ�Ĵ�����
������Ҫ������ִ���ֽ����
����ת��ʹ�ó����װ���ƣ����г���洢���ڴ������У����޳��ȴ�С�����������׵�ַҪ���ĸ��ֽ�����ʾ


ע��������ִ�����棬���е���������ܶ�����������������ڣ�Ҫ�����ܵļ��ٴ�����������ʱ�俪���������ô������Ϳռ�����ȡ����˵�����ٷ�֧���Ĳ�����Ҫ��switch-case����ʽȡ����
Ҫ��Ƶ���Ĳ�����Ϊ�ַ��滻���Ǻ������õ���ʽ��




������ڼ��㻺���Ż�����Ҫ����������Ҫ������Ǽ�����һ��Ĭ���͵�����Ĵ������У�Ȼ������һ����ǰҪ������Ĵ����е����ݷŵ�
�������Ĵ����С����ܺ�����Ҫ���Ż�����Ĺ�����������̿��ܱ��ָ�Ϊ�������輴����ʹ�����Ĭ�ϴ������ڸ��ټĴ����У����������������߾�Ϊ����ʱ��


ע���ַ����Ǵ���ֵ���б��Ǵ�������,�������ԭ�еĻ���֮��������ģ��ָ���ʱ���ĳЩ��Ҫ������ʱ����ȷ�����������͵�����¡�
ָ���Ż�һ�����ǹ�������������������������������ģ���һ���ǹ����ַ������б�����㣬��������������ٿռ�ķ�ʽ��Ϊ�͵�ִ�С�
�������ͻ���ʹ��ֵ���ݣ��������ô��ݣ��Ժ����ȡ����count��һ����ǲ��������ֶΡ�

����Ҫע���б���ʱ��������������Ҫ���帴�ơ�

�ڶ��߳���Ҫ�������Щ��ȫ����Щ�Ǿֲ��ġ�
*/
#define EXECUTE_MODULE
#ifdef WIN32
#include <Windows.h>
#endif

#include <stdlib.h>
#include "data.h"
#include "execute.h"
#include "lib.h"
#include "amem.h"
#include <time.h>
#include "hash.h"
#include "shell.h"
#include "util.h"
#include "aenv.h"
#ifndef COMMON_LIB
#define COMMON_LIB
#endif
#include "../Extension/thread/_thread.h"


extern object_list global_value_list,obj_list,dynamic_name;  //����ȫ�ֱ������б����￼�ǽ�ȫ�ֱ�����ջ����ʽ����, ����������б�����Ķ���ַ��ƫ�����Ĺ�ϵ
extern funlist global_function;  //����ָ�������Ԥ�����
extern object_set global_value_map,global_function_map;
extern bytecode main_byte; //�����Ƕ�����һ���洢�ֽ�����ڴ�ռ� 
extern classlist clist;
extern object_string charset[cellnum];
extern object_bytes byteset[cellnum];
extern switchlist _global_sw_list;
extern int thread_cmd;//��ֻ��Ժ�̨�߳���Ч��
extern linkcollection global_scope;
extern temp_alloc_info angel_temp;
extern angel_buildin_func angel_build_in_def[];
linkcollection angel_stack_list;
int is_sys_lock;


void init_stack_list()
{
	angel_stack_list = initlink();
	angel_thread_count = 0;
}
fun select_func(funlist fl,uint16_t offset)
{
	fun p;
	if(!fl)
		return NULL;
	for(int i=0; i<fl->len; i++)
	{
		p = fl->fun_item[i];
		if(offset<=p->paracount && offset>=p->paracount-p->default_paracount)
			return p;
	}
	char errorinfo[errorsize];
	sprintf(errorinfo,"����%s���ò�����Ŀ���ԣ�",fl->fun_item[0]->name);
	return NULL;
}


unsigned long switchcase(switchtable swt,object o)
{
	int i,j;
	i=0; j=swt->len-1;
	unsigned long hash=globalhash(o);
	while(i<j)
	{
		int mid=(i+j)/2;
		if(swt->sw_item[mid]->hash>hash)
			j=mid-1;
		else if(swt->sw_item[mid]->hash<hash)
			i=mid+1;
		else
		{
			if(swt->sw_item[mid]->type==o->type)
				return swt->sw_item[mid]->offset;
		}
	}
	if(i==j && swt->sw_item[i]->type==o->type && swt->sw_item[i]->hash==hash)
		return swt->sw_item[i]->offset;
	return swt->sw_item[swt->len]->offset;
}

inline object system_call(object *bp,angel_buildin_func sys_fun)//���￼�ǽ�Ĭ�ϲ�����ֵΪNULL����ȻҪ�����������
{
	switch(sys_fun.argcount)
	{
	case 0:
		return ((object(*)())(sys_fun.handle))();
	case 1:
		return ((object(*)(object))(sys_fun.handle))(bp[0]);
	case 2:
		return ((object(*)(object,object))(sys_fun.handle))(bp[0],bp[1]);
	case 3:
		return ((object(*)(object,object,object))(sys_fun.handle))(bp[0],bp[1],bp[2]);
	case 4:
		return ((object(*)(object,object,object,object))(sys_fun.handle))(bp[0],bp[1],bp[2],bp[3]);
	}
}

/*
�ֽ���ִ���ں�
*/
inline object collection_mult(object left,object right)
{
	object o;
	int angel_int1;
	if(ISSTR(left) || ISSTR(right))  //�ַ��������㷨,�Ĵ�����Ķ����ݴ�����
	{
		//�����ԭ������reg1Ϊ�������reg1->count==0�����Ż�����ֱ�Ӹ���
		switch(left->type)
		{
		case INT:  //˵���ڶ�����������Ϊ�ַ���
			angel_int1 = GETINT(left);
			left = right;
			break ;
		case STR:
			if(ISINT(right))
				angel_int1 = GETINT(right);
			else
			{
				angel_error("�˷��ұߵ����Ͳ�����Ҫ��");
				return NULL;
			}
			break ;
		default:
			angel_error("�˷���ߵ����Ͳ�����Ҫ��");
			return NULL;
		}
		o = (object)strrepeat(GETSTR(left),angel_int1);
	}
	else if(ISLIST(left) || ISLIST(right))
	{
		switch(left->type)
		{
		case INT:  //˵���ڶ�����������Ϊ�ַ���
			left = right;
			angel_int1 = GETINT(left);
			break ;
		case LIST:
			;
			if(ISINT(right))
				angel_int1 = GETINT(right);
			else
			{
				angel_error("�˷��ұߵ����Ͳ�����Ҫ��");
				return NULL;
			}
			break ;
		default:
			angel_error("�˷���ߵ����Ͳ�����Ҫ��");
			return NULL;
		}
		o = (object)listrepeat(GETLIST(left),angel_int1);
	}
	else if(ISBYTES(left) || ISBYTES(right))
	{
		switch(left->type)
		{
		case INT:  //˵���ڶ�����������Ϊ�ַ���
			left = right;
			angel_int1 = GETINT(left);
			break ;
		case BYTES:
			;
			if(ISINT(right))
				angel_int1 = GETINT(right);
			else
			{
				angel_error("�˷��ұߵ����Ͳ�����Ҫ��");
				return NULL;
			}
			break ;
		default:
			angel_error("�˷���ߵ����Ͳ�����Ҫ��");
			return NULL;
		}
		o = (object)bytesrepeat(GETBYTES(left),angel_int1);
	}
	else
	{
		angel_error("�˷���ߵ����Ͳ�����Ҫ��");
		return NULL;
	}
	return o;
}
inline object collection_add(object left,object right)
{
	object o;
	if(ISSTR(left) || ISSTR(right))  //�ַ��������㷨,�Ĵ�����Ķ����ݴ�����
	{
		//�����ԭ������reg1Ϊ�������reg1->count==0�����Ż�����ֱ�Ӹ���
		object temp = NULL;
		switch(left->type)
		{
		case INT:  //˵���ڶ�����������Ϊ�ַ���
			temp = (object)initstring(tointchar(GETINT(left)));
			left = temp;
			//left->refcount++;
			break ;
		case FLOAT:
			temp = (object)initstring(tointchar(GETINT(left)));
			left = temp;
			break ;
		case STR:
			if(ISINT(right))
			{
				temp = (object)initstring(tointchar(GETINT(right)));
				right = temp;
				//right->refcount++;
			}
			else if(ISFLOAT(right))
			{
				temp = (object)initstring(tointchar((int64_t)GETFLOAT(right)));
				right = temp;
			}
			else if(!ISSTR(right))
			{
				angel_error("�ӷ��ұߵ����Ͳ�����Ҫ��");
				return NULL;
			}
			break ;
		default:
			angel_error("�ӷ���ߵ����Ͳ�����Ҫ��");
			return NULL;
		}
		o = (object)concatstr(GETSTR(left),GETSTR(right));
		DECREF(temp);
	}
	else if(ISLIST(left) && ISLIST(right))
	{
		o = (object)concatlist(GETLIST(left),GETLIST(right));
	}
	else if(ISSET(left) && ISSET(right))
	{
		o = (object)concatset(GETSET(left),GETSET(right));
	}
	else
	{
		angel_error("�ӷ����ߵ����Ͳ�����Ҫ��");
		return NULL;
	}
	return o;
}



/*

�߳���ͬ��
*/
#define SECURE //fast_lock();
#define EXIT_SECURE  if(ISNORMAL(right)) {_DECREF(right)}//fast_unlock();
#define STACKHEAP_EXIT_SECURE if(ISHEAPED(right)) {right->flag = 0; _DECREF(right)}


#ifdef WIN32
CRITICAL_SECTION g_critical;
#else
#endif



void critical_init()
{
#ifdef WIN32
	InitializeCriticalSection(&g_critical);
#else
#endif
}
void critical_enter()
{
#ifdef WIN32
	EnterCriticalSection(&g_critical);
#else
#endif
}
void critical_leave()
{
#ifdef WIN32
	LeaveCriticalSection(&g_critical);
#else
#endif
}

void angel_lock()
{
	if(angel_thread_count == 0) ; 
	if(angel_thread_count < 9){ 
		fast_lock(); 
	} else { 
		fast_lock(); 
		critical_enter(); 
		is_sys_lock = 1; 
	} 
}
void angel_unlock()
{
	if(!is_sys_lock)
	{ 
		fast_unlock(); 
	}
	else
	{ 
		is_sys_lock = 0; 
		critical_leave(); 
		fast_unlock(); /*ע�����unlock��leave��˳��Ҫ��fast_lock()���෴*/
	} 
}



linkcollection alloc_thread()
{
	fast_lock();
	linkcollection thread_controll = initlink();
	runable run = (runable)calloc(1,sizeof(runablenode));
	thread_controll->data = run;
	run->thread_type = DAEMON_TYPE;
	addlink(angel_stack_list,thread_controll);
	angel_thread_count++;
	fast_unlock();
	return thread_controll;
}
void free_runable(linkcollection controll)
{
	runable thread = (runable)controll->data;
	if(!thread) return ;
	/*
	���̵߳ȴ�
	if(!thread->func){
		waitforthread();
	}
	*/
	fast_lock();
	object_list thread_param = thread->argc;
	object_fun func = thread->func;
	object_ext pthread = thread->pthread;
	DECREF(thread_param);
	DECREF(func);
	DECREF(pthread);


	angel_thread_count--;
	deletelink(controll);
	
	//ɾ��thread���ƿ�

	
	fast_unlock();
}
#pragma optimize( "", off )
void waitforthread()
{
wait:
	for(linkcollection p = angel_stack_list->next; p != angel_stack_list; p = p->next)
	{
		runable r = (runable)p->data;
		if(r->thread_type == NON_DAEMON_TYPE)
		{
			goto wait;
		}
	}
}


#pragma optimize( "", on )

void exec(linkcollection thread_controll)
{
	//���¶�����ټĴ���
	/*
	�Ĵ�����Ϊ���¼��֣�һ��������Ĵ������ݴ��������Ͳ�����
	����Ϊ�������ֵ������ٶȣ������׼��һЩ����ز�
	һ�����м����Ĵ�������������̵�
	һ����ջ�����ں������õġ�
	*/
	register bytecode exec_byte;
	register int64_t angel_int1;
	register long angel_int2;
	register double angel_float1,angel_float2;


	//���ú������û���
	env env_reg=initenv();


	runtime_stack angel_runtime=initruntime();


	register char *pc;
	//�����������ںͳ�����Ҫ�üĴ�����
	unsigned long longoffset;  //�����ڲ�ָ�룬angel_pc��ȫ��ָ��
	char * res;
	register object *exec_environment,*sys_call_bp;  //����ʱ��������Ĭ��Ϊȫ�ֻ���,�ͻ�ַ�Ĵ���base_addr_reg,���Ǵ������������ġ�
	uint16_t index;
	pclass class_reg=NULL;
	exec_byte=main_byte;
	pc = exec_byte->code;
	
	
	char errorinfo[errorsize];
	object_string memname;
	object *angel_const = global_value_list->item;
	register object *angel_addr=NULL,*angel_shared = &global_value_list->item[global_value_list->alloc_size],
		*angel_inplace_addr_reg=NULL;
	void *integer_heap_in_stack = angel_runtime->stack_heap_base,*tempval;
	//free_perpetual((object)obj_list);
	object_string *angel_name = (object_string *)dynamic_name->item;
	//�Ƚ���ȫ�ֵ�ִ�л���
	exec_environment=angel_runtime->data;

	uint16_t offset,offset1,offset2;
	register object left,right;
	object o,otemp;
	char angel_byte;

	int bool1,bool2;
	fun memfun,exec_fun=NULL;

	object tranversalobj=NULL;
	env_ele e;
	object_iterator angel_iter;
	object_fun function;
	object_entry entry;
	angel_buildin_func *angel_sys_function;

	//�߳̿�
	runable thread = (runable)thread_controll->data;
	thread->_stack = angel_runtime;


#define PARAM(no)  *(int16_t *)(pc+no)
#define ADDOP(op) pc += op;
#define NEXTOP(op) ADDOP(op) goto next;
#define AJUSTSTACKHEAPPTR(i) integer_heap_in_stack = ((char *)integer_heap_in_stack+(i*stack_heap_size))

#define STACKTOP exec_environment[exec_fun->localcount-1]
#define THIS exec_environment[exec_fun->paracount]
#define BINARY_SHARED_LOCAL_W left = angel_shared[PARAM(3)]; right = exec_environment[PARAM(5)];
#define BINARY_SHARED_LOCAL_R left = angel_shared[PARAM(1)]; right = exec_environment[PARAM(3)];
#define BINARY_SHARED_SHARED_W left = angel_shared[PARAM(3)]; right = angel_shared[PARAM(5)];
#define BINARY_SHARED_SHARED_R left = angel_shared[PARAM(1)]; right = angel_shared[PARAM(3)];
#define BINARY_LOCAL_LOCAL_W left = exec_environment[PARAM(3)]; right = exec_environment[PARAM(5)];
#define BINARY_LOCAL_LOCAL_R left = exec_environment[PARAM(1)]; right = exec_environment[PARAM(3)];
#define BINARY_LOCAL_SHARED_W left = exec_environment[PARAM(3)]; right = angel_shared[PARAM(5)];
#define BINARY_LOCAL_SHARED_R left = exec_environment[PARAM(1)]; right = angel_shared[PARAM(3)];

#define UNARY_LOCAL_W right = exec_environment[PARAM(3)];
#define UNARY_LOCAL_R right = exec_environment[PARAM(1)];
#define UNARY_SHARED_W right = angel_shared[PARAM(3)];
#define UNARY_SHARED_R right = angel_shared[PARAM(1)];


#define INPLACE_PREPARE_LOCAL(env) if(angel_inplace_addr_reg) { \
									left = *angel_inplace_addr_reg; UNARY_LOCAL_R; pc += 3;\
								  }else{\
									angel_inplace_addr_reg = &env[PARAM(1)];left = *angel_inplace_addr_reg; \
									UNARY_LOCAL_W; pc += 5; \
								  }
#define INPLACE_PREPARE_SHARED(env) if(angel_inplace_addr_reg) { \
									left = *angel_inplace_addr_reg; UNARY_SHARED_R; pc += 3;\
								  }else{\
									angel_inplace_addr_reg = &env[PARAM(1)];left = *angel_inplace_addr_reg; \
									UNARY_SHARED_W; pc += 5; \
								  }

#define BINARY_SHARED_LOCAL_INPLACE INPLACE_PREPARE_LOCAL(angel_shared);
#define BINARY_SHARED_SHARED_INPLACE INPLACE_PREPARE_SHARED(angel_shared);
#define BINARY_LOCAL_LOCAL_INPLACE INPLACE_PREPARE_LOCAL(exec_environment);
#define BINARY_LOCAL_SHARED_INPLACE INPLACE_PREPARE_SHARED(exec_environment);


#define ADDCLASSMEMBER(_class,key,value) adddict(_class->static_value,(object)key,value);
#define ADDOBJMEMBER(o,key,value) adddict(o->mem_value,(object)key,value);
#define STORE(offset,o) exec_environment[offset] = o;
#define TEMP(o) STORE(PARAM(1),o);
#define STOREASTEMP_I(offset,lval) angel_int1 = lval; right = (object)GETSTACKHEAPASINT(integer_heap_in_stack,offset); right->type = INT; GETINT(right) = angel_int1; STORE(offset,right);
#define STOREASTEMP_F(offset,fval) angel_float1 = fval; right = (object)GETSTACKHEAPASINT(integer_heap_in_stack,offset); right->type = FLOAT; GETFLOAT(right) = angel_float1; STORE(offset,right);
#define STOREASTEMP_C(offset,cval) right = (object)GETSTACKHEAPASINT(integer_heap_in_stack,offset); right->type = STR; res = (char *)&GETSTR(right)->refcount; *(int16_t *)res = cval; GETSTR(right)->s = res; GETSTR(right)->len = 2; STORE(offset,right);
#define TEMPINT(lval) offset = PARAM(1); STOREASTEMP_I(offset,lval);
#define TEMPFLOAT(lval) offset = PARAM(1); STOREASTEMP_F(offset,lval);
#define TEMPCHAR(cval) offset = PARAM(1); STOREASTEMP_C(offset,cval);
#define INPLACE_TEMP(o) if(flag == 0)_assign_execute(angel_inplace_addr_reg,(object)o);else _assign_execute_ref(angel_inplace_addr_reg,(object)o); angel_inplace_addr_reg=NULL;

#define TRANSBOOL(o) if(ISBOOLEAN(o));else if(ISINT(o)) o = GETINT(o)?GETTRUE : GETFALSE; \
				  else o=ISNU(o)?GETFALSE : GETTRUE;
#define TRANSBOOL_N(o) if(o == GETTRUE) o = GETFALSE; else if(o == GETFALSE) o = GETTRUE; \
					   else if(ISINT(o)) o = GETINT(o)? GETFALSE : GETTRUE; \
				       else o=ISNU(o)? GETTRUE : GETFALSE;
//�����Ƕ�ϵͳ�������õ�ռ�ռ�Ŀ��٣���������������callָ���alloc_stackָ�������£�֮����������û��Ĭ�ϲ����Ĵ���
#define ALLOC_SYSCALL_STACK sys_call_bp = angel_runtime->data+angel_runtime->top; angel_runtime->top += offset; 
#define SYSRET_WITH(o) exec_environment[offset1] = o; goto commonret;
#define SYSRET SYSRET_WITH(angel_uninitial);


#define STATICNOTFOUND if(!entry){ \
							sprintf(errorinfo,"��%s�����ھ�̬����%s",class_reg->name,GETSTR(memname)->s); \
							angel_error(errorinfo); \
							goto exit; \
						}
#define MEMBERNOTFOUND(o) if(!entry){ \
							sprintf(errorinfo,"����%sû�г�Ա����%s",o->c->name,GETSTR(memname)->s); \
							angel_error(errorinfo); \
							goto exit; \
						}
#define INDEXNEGATIVE(index) if(GETINT(index) < 0) {angel_error("����ֵ���벻��Ϊ������"); goto exit;}
#define _OUTOFBOUNDEXCEPT_L(base,index)  INDEXNEGATIVE(index) if(GETINT(index) >= GETLIST(base)->len){  angel_error("��������Խ�磡"); goto exit;} angel_int2 = GETINT(index);
#define OUTOFBOUNDEXCEPT_L(base,index)  _OUTOFBOUNDEXCEPT_L(base,index);
#define _OUTOFBOUNDEXCEPT_S(base,index) INDEXNEGATIVE(index) if(GETINT(index)*2 >= GETLIST(base)->len){  angel_error("�ַ�������Խ�磡"); goto exit;} angel_int2 = GETINT(index);
#define OUTOFBOUNDEXCEPT_S(base,index)  _OUTOFBOUNDEXCEPT_S(base,index);
#define GETMEMBER(base) entry = getmember(base,memname); MEMBERNOTFOUND(base);
#define GETSTATIC entry = getclassmember(class_reg,memname); STATICNOTFOUND;

#define LOADMEMBER(base) GETMEMBER(base); TEMP(entry->value);
#define LOADSTATIC GETSTATIC; TEMP(entry->value);
#define GETCHAR(o,index) ((uint16_t *)(GETSTR(o)->s))[index]
#define GETBYTE(o,index) ((uchar *)(GETBYTES(o)->bytes))[index]
#define INITCLASSREG if(exec_fun) class_reg = exec_fun->class_context;
#define TEMPSTRITEM(item) offset2 = item; if(offset2 < cellnum){ TEMP((object)charset[offset2]); } \
					else{ TEMPCHAR(offset2); }
#define STACKHEAPCOPY(right,offset2) \
			do{ \
				if(ISHEAPINSTACK(right))  { \
					if(ISINT(right)){ \
						STOREASTEMP_I(offset2,GETINT(right));   \
					}else if(ISFLOAT(right)){ \
						STOREASTEMP_F(offset2,GETFLOAT(right));   \
					}else{\
						STOREASTEMP_C(offset2,*((int16_t *)GETSTR(right)->s)); \
					}\
				}else{ \
					STORE(offset2,right); \
				} \
			}while(0);

#define TRANSFLOAT \
				do{ \
					if(ISFLOAT(left)) angel_float1 = GETFLOAT(left); else angel_float1 = (double)GETINT(left); \
					if(ISFLOAT(right)) angel_float2 = GETFLOAT(right); else angel_float2 = (double)GETINT(right); \
				}while(0);
	if(thread->func)
	{
		object_list thread_param = thread->argc;
		function = thread->func;
		for(int i=0; i<thread_param->len; i++)
			angel_runtime->data[++angel_runtime->push_pos] = thread_param->item[i];

		offset = thread_param->len;
		pc = NULL;
		goto callback;
	}
	while(1)
	{
		int flag=0;
		//�������ֱ��ȡ�ĸ��ֽ�����
		//���Ƕ������һ��ָ���Ԥ������ȡ������������angel_reg_ptr��pc����ָ�룬��ԭ��һ��Ҫһ�������������
		//times = *pc;
		//�Ĵ����������Ļ����йأ�����exec_environment�й�
atom:
		switch((uchar)*pc)  //���²�������ԭ�Ӳ���Ҫ���������������ַ�֧
		{
		case _nop:  //�ղ���
			NEXTOP(1);
		case _load_dynamic:
			memname = angel_name[PARAM(3)];
			if(exec_fun->type != 3)
			{
				o = THIS;
				LOADMEMBER(o);
			} 
			else 
			{
				class_reg = exec_fun->class_context;
				LOADSTATIC;
			}
			NEXTOP(5);
		case _load_index_shared_local: BINARY_SHARED_LOCAL_W goto loadindex;
		case _load_index_shared_shared: BINARY_SHARED_SHARED_W goto loadindex;
		case _load_index_local_shared: BINARY_LOCAL_SHARED_W goto loadindex;
		case _load_index_local_local: BINARY_LOCAL_LOCAL_W
loadindex:
			switch(left->type)
			{
			case LIST:
				if(ISINT(right))
				{
					_OUTOFBOUNDEXCEPT_L(left,right);
					TEMP(GETLIST(left)->item[angel_int2]);
				}
				else if(ISRANGE(right))
				{
					uchar testc = *(pc+7);
					if(!testc)
					{
						right = (object)slicelist(GETLIST(left),GETRANGE(right));
					}
					else
					{
						right = (object)initslice(left,GETRANGE(right));
					}
					TEMP(right);
					EXIT_SECURE;
				}
				break ;
			case STR:  //������ַ�����ֱ�����ַ��������256*256��
				if(ISINT(right))
				{
					_OUTOFBOUNDEXCEPT_S(left,right);
					TEMPSTRITEM(GETCHAR(left,angel_int2));
				}
				else if(ISRANGE(right))
				{
					uchar testc = *(pc+7);
					if(!testc)
					{
						right = (object)slicestring(GETSTR(left),GETRANGE(right));
					}
					else
					{
						right = (object)initslice(left,GETRANGE(right));
					}
					TEMP(right);
					EXIT_SECURE;
				}
				break ;
			case BYTES:  //������ַ�����ֱ�����ַ��������256*256��
				if(ISINT(right))
				{
					_OUTOFBOUNDEXCEPT_L(left,right);
					offset2 = GETBYTE(left,angel_int2);
					TEMP((object)byteset[offset2]);
				}
				else
				{
					uchar testc = *(pc+7);
					if(!testc)
					{
						right = (object)slicebytes(GETBYTES(left),GETRANGE(right));
					}
					else
					{
						right = (object)initslice(left,GETRANGE(right));
					}
					TEMP(right);
					EXIT_SECURE;
				}
				break ;
			case DICT:
				angel_int2=getdictindex(GETDICT(left),right);
				if(!GETDICT(left)->hashtable[angel_int2])
				{
					angel_error("�ֵ䲻���ڸü���");
					goto exit;
				}
				TEMP(GETDICT(left)->hashtable[angel_int2]->value);
				break ;
			default:
				angel_error("����������߱���Ϊ�б���ַ�����");  //����Ϊ��ģ������
				goto exit;
			}
			NEXTOP(8)
			goto next;
		case _load_member_local: UNARY_LOCAL_W goto loadmember;
		case _load_member_shared: UNARY_SHARED_W
loadmember:
			//��ʱ�Ĵ������Ƕ���ֵ
			memname = angel_name[PARAM(5)];
			
			LOADMEMBER(right);

			NEXTOP(7);
		case _load_static:  //û�в�����������Ҫ�Լ�����ָ������
			class_reg = clist->c[PARAM(3)];
			memname = angel_name[PARAM(5)];

			LOADSTATIC;
			NEXTOP(7);
		case _load_static_default:
			memname = angel_name[PARAM(3)];
			INITCLASSREG;


			LOADSTATIC;
			NEXTOP(5);
		case _mov_local: UNARY_LOCAL_W; goto tempit;
		case _mov_shared: UNARY_SHARED_W;
tempit:
			TEMP(right);
			NEXTOP(5);
		case _asc_ref:
			left = exec_environment[PARAM(1)];
			ASCREF(left);
			NEXTOP(3);
		case _dec_ref:
			left = exec_environment[PARAM(1)];
			DECREF(left);
			NEXTOP(3);
		case _store_global_local: UNARY_LOCAL_R; goto storeglobal;
		case _store_global_shared: UNARY_SHARED_R;
storeglobal:
			_assign_execute(&angel_shared[PARAM(3)],right);
			NEXTOP(5)
		case _store_global_temp: UNARY_LOCAL_R;
			SECURE
			right = _stacktoheap(right);
			_assign_execute(&angel_shared[PARAM(3)],right);
			STACKHEAP_EXIT_SECURE
			NEXTOP(5)
		case _store_local_local: UNARY_LOCAL_R; goto storelocal;
		case _store_local_shared: UNARY_SHARED_R
storelocal:
			_assign_execute(&exec_environment[PARAM(3)],right);
			NEXTOP(5)
		case _store_local_temp:UNARY_LOCAL_R;
			SECURE
			right = _stacktoheap(right);
			_assign_execute(&exec_environment[PARAM(3)],right);
			STACKHEAP_EXIT_SECURE
			NEXTOP(5)
		case _store_dynamic_local: UNARY_LOCAL_R; goto storedefault;
		case _store_dynamic_shared: UNARY_SHARED_R; goto storedefault;
storedefault:
			memname = angel_name[PARAM(3)];
			if(exec_fun->type != 3)
			{
				o = THIS;
				ADDOBJMEMBER(o,memname,right);
			}
			else 
			{ 
				class_reg = THIS->c;
				ADDCLASSMEMBER(class_reg,memname,right);
			}
			NEXTOP(5)
		case _store_dynamic_temp: UNARY_LOCAL_R;
			STACKTOHEAP(right)
			goto storedefault;
		case _store_index_shared_local: BINARY_SHARED_LOCAL_R; goto storeindex;
		case _store_index_shared_shared: BINARY_SHARED_SHARED_R; goto storeindex;
		case _store_index_local_local: BINARY_LOCAL_LOCAL_R;  goto storeindex;
		case _store_index_local_shared: BINARY_LOCAL_SHARED_R; //leftΪbase��rightΪֵ��oΪ����
storeindex: 
			o = exec_environment[PARAM(5)];
			switch(left->type)
			{
			case LIST:
				if(ISINT(o))
				{
					OUTOFBOUNDEXCEPT_L(left,o);
					assign_execute(GETLIST(left)->item,angel_int2,right);
					STACKHEAP_EXIT_SECURE
				}
				else if(ISRANGE(o))
				{
					if(ISLIST(right))
					{
						storeslicelist(GETLIST(left),GETRANGE(o),GETLIST(right));
					}
					else
					{
						if(!storeslicelist_asslice(GETLIST(left),GETRANGE(o),GETSLICE(right)))
							goto exit;
						DECREF(right);
					}
				}
				else
				{
indexerror:
					angel_error("����ֵ����Ϊ������"); 
					goto exit;
				}
				break ;
			case STR:
fillchar:
				if(ISINT(o)) 
				{
					flag = left->refcount-2;
					OUTOFBOUNDEXCEPT_S(left,o);
					if(right->type != STR)
					{
						angel_error("������ֵ����������ַ�����");
						goto exit;
					}
					if(ISPOOL(left))
					{
						left = (object)copystring(GETSTR(left));
						left->refcount = flag;
						if((uchar)*pc > _store_index_shared_temp)
						{
							TEMP(left);
						}
						else
							angel_shared[PARAM(1)] = left;
					}
					((int16_t *)GETSTR(left)->s)[angel_int2] = *(int16_t *)GETSTR(right)->s;
				}
				else if(ISRANGE(o))
				{

				}
				else
				{
					goto indexerror;
				}
				break ;
			case BYTES:
fillbyte:
				if(ISINT(o)) 
				{
					flag = left->refcount-2;
					OUTOFBOUNDEXCEPT_L(left,o);
					if(ISBYTES(right))
					{
						angel_error("������ֵ����������ֽڻ����֣�");
						goto exit;
					}
					if(ISPOOL(left))
					{
						left = (object)copybytes(GETBYTES(left));
						left->refcount = flag;
						if((uchar)*pc > _store_index_shared_temp)
						{
							TEMP(left);
						}
						else
							angel_shared[PARAM(1)] = left;
					}
					((char *)GETBYTES(left)->bytes)[angel_int2] = angel_byte;
				}
				else if(ISRANGE(o))
				{

				}
				else
				{
					goto indexerror;
				}
				break ;
			case DICT:
				adddict(GETDICT(left),o,right);
				break ;
			default:
				angel_error("��������ַ���Ϸ���");
				goto exit;
			}
			NEXTOP(7)
		case _store_index_shared_temp: BINARY_SHARED_LOCAL_R; goto sit;
		case _store_index_local_temp: BINARY_LOCAL_LOCAL_R;
sit:

			switch(left->type)
			{
				case STR:
					o = exec_environment[PARAM(5)];
					goto fillchar;
				case BYTES: //�������Ǹ��ַ�������������
					o = exec_environment[PARAM(5)];
					angel_byte = *GETBYTES(right)->bytes;
					goto fillbyte;
				default:
					SECURE
					right = _stacktoheap(right);
			}
			goto storeindex;
		case _store_member_shared_local: BINARY_SHARED_LOCAL_R; goto storemember;
		case _store_member_shared_shared: BINARY_SHARED_SHARED_R; goto storemember;
		case _store_member_local_local: BINARY_LOCAL_LOCAL_R; goto storemember;
		case _store_member_local_shared: BINARY_LOCAL_SHARED_R; goto storemember;
storemember:
			memname = angel_name[PARAM(5)];
			ADDOBJMEMBER(left,memname,right);
			STACKHEAP_EXIT_SECURE
			NEXTOP(7)
		case _store_member_shared_temp: BINARY_SHARED_LOCAL_R; goto smt;
		case _store_member_local_temp: BINARY_LOCAL_LOCAL_R;
smt:
			SECURE
			right = _stacktoheap(right);
			goto storemember;
		case _store_static_local: UNARY_LOCAL_R; goto store_static;
		case _store_static_shared: UNARY_SHARED_R;
store_static:
			class_reg = clist->c[PARAM(3)];
			memname = angel_name[PARAM(5)];
			ADDCLASSMEMBER(class_reg,memname,right);
			STACKHEAP_EXIT_SECURE
			NEXTOP(7)
		case _store_static_temp: UNARY_LOCAL_R;
			SECURE
			right = _stacktoheap(right);
			goto store_static;
		case _store_static_default_local: UNARY_LOCAL_R; goto store_static_default;
		case _store_static_default_shared: UNARY_SHARED_R;
store_static_default:
			memname = angel_name[PARAM(3)];
			INITCLASSREG;
			ADDCLASSMEMBER(class_reg,memname,right);
			STACKHEAP_EXIT_SECURE
			NEXTOP(5)
		case _store_static_default_temp: UNARY_LOCAL_R;
			SECURE
			right = _stacktoheap(right);
			goto store_static_default;

		/*
		��������ָ��
		*/
		case _add_shared_local: BINARY_SHARED_LOCAL_W; goto add;
		case _add_shared_shared: BINARY_SHARED_SHARED_W; goto add;
		case _add_local_shared: BINARY_LOCAL_SHARED_W; goto add;
		case _add_local_local: BINARY_LOCAL_LOCAL_W; goto add;
add:
			if(ISINT(left) && ISINT(right))
			{
				TEMPINT(GETINT(left)+GETINT(right));
			}
			else if(ISNUM(left) && ISNUM(right))
			{
				TRANSFLOAT
				TEMPFLOAT(angel_float1+angel_float2);
			}
			else 
			{
				right = collection_add(left,right);
				if(!right)
					goto exit;
				TEMP(right);
				EXIT_SECURE
			}
			NEXTOP(7)
		case _sub_shared_local: BINARY_SHARED_LOCAL_W; goto sub;
		case _sub_shared_shared: BINARY_SHARED_SHARED_W; goto sub;
		case _sub_local_shared: BINARY_LOCAL_SHARED_W; goto sub;
		case _sub_local_local: BINARY_LOCAL_LOCAL_W; 
sub:
			if(ISINT(left) && ISINT(right))
			{
				TEMPINT(GETINT(left)-GETINT(right));
			}
			else if(ISNUM(left) && ISNUM(right))
			{
				TRANSFLOAT;
				TEMPFLOAT(angel_float1-angel_float2);
			}
			else
			{
				angel_error("�����������߱��������α�����");
				goto exit;
			}
			NEXTOP(7)
		case _mult_shared_local: BINARY_SHARED_LOCAL_W; goto mult;
		case _mult_shared_shared: BINARY_SHARED_SHARED_W; goto mult;
		case _mult_local_shared: BINARY_LOCAL_SHARED_W; goto mult;
		case _mult_local_local: BINARY_LOCAL_LOCAL_W; goto mult;
mult:
			if(ISINT(left) && ISINT(right))
			{
				TEMPINT(GETINT(left)*GETINT(right));
			}
			else if(ISNUM(left) && ISNUM(right))
			{
				TRANSFLOAT
				TEMPFLOAT(angel_float1*angel_float2);
			}
			else
			{
				right = collection_mult(left,right);
				if(!right)
					goto exit;
				TEMP(right);
				EXIT_SECURE
			}
			NEXTOP(7)
		case _div_shared_local: BINARY_SHARED_LOCAL_W; goto _div;
		case _div_shared_shared: BINARY_SHARED_SHARED_W; goto _div;
		case _div_local_shared: BINARY_LOCAL_SHARED_W; goto _div;
		case _div_local_local: BINARY_LOCAL_LOCAL_W; goto _div;
_div:
			if(ISINT(left) && ISINT(right))
			{
				if(ISINT(right) == 0)
				{
					angel_error("������");
					goto exit;
				}
				TEMPINT(GETINT(left) / GETINT(right));
				/*
				if(GETINT(left) % GETINT(right) == 0)
				{
					TEMPINT(GETINT(left) / GETINT(right));
				}
				else
				{
					TEMPFLOAT((double)GETINT(left) / (double)GETINT(right));
				}
				*/
			}
			else if(ISNUM(left) && ISNUM(right))
			{
				TRANSFLOAT
				if(angel_float2 == 0)
				{
					angel_error("������");
					goto exit;
				}
				TEMPFLOAT(angel_float1/angel_float2);
			}
			else
			{
				angel_error("�����������߱��������α�����");
				goto exit;
			}
			NEXTOP(7)
			break;
		case _mod_shared_local: BINARY_SHARED_LOCAL_W; goto mod;
		case _mod_shared_shared: BINARY_SHARED_SHARED_W; goto mod;
		case _mod_local_shared: BINARY_LOCAL_SHARED_W; goto mod;
		case _mod_local_local: BINARY_LOCAL_LOCAL_W; goto mod;
mod:
			if(ISINT(left) && ISINT(right))
			{
				TEMPINT(GETINT(left)%GETINT(right));
			}
			else
			{
				if(ISSTR(left) && ISLIST(right))  //�ַ��������㷨,�Ĵ�����Ķ����ݴ�����
				{
					//�����ԭ������reg1Ϊ�������reg1->count==0�����Ż�����ֱ�Ӹ���
					//������Ҫ����һ���µĶ���
					//strfomat(GETSTR(o),GETLIST(right));
					;
				}
				else
				{
					angel_error("ȡģ���ߵ����Ͳ�����Ҫ��");
					goto exit;
				}
				TEMP(o);
			}
			NEXTOP(7)
			goto exit;
		case _lshift_shared_local: BINARY_SHARED_LOCAL_W; goto lshift;
		case _lshift_shared_shared: BINARY_SHARED_SHARED_W; goto lshift;
		case _lshift_local_local: BINARY_LOCAL_LOCAL_W; goto lshift;
		case _lshift_local_shared: BINARY_LOCAL_SHARED_W;
lshift:
			if(ISINT(left) && ISINT(right))
			{
				TEMPINT(GETINT(left) << GETINT(right));
			}
			else
			{
				angel_error("�����������߱��������α�����");
				goto exit;
			}
			NEXTOP(7)
		case _rshift_shared_local: BINARY_SHARED_LOCAL_W; goto rshift;
		case _rshift_shared_shared: BINARY_SHARED_SHARED_W; goto rshift;
		case _rshift_local_local: BINARY_LOCAL_LOCAL_W; goto rshift;
		case _rshift_local_shared: BINARY_LOCAL_SHARED_W; goto rshift;
rshift:
			if(ISINT(left) && ISINT(right))
			{
				TEMPINT(GETINT(left) >> GETINT(right));
			}
			else
			{
				angel_error("�����������߱��������α�����");
				goto exit;
			}
			NEXTOP(7)
		case _and_direct_local: BINARY_LOCAL_LOCAL_W;TRANSBOOL(right);goto _and;
		case _and_direct_shared: BINARY_LOCAL_SHARED_W;TRANSBOOL(right);goto _and;
		case _and_direct_direct: BINARY_LOCAL_LOCAL_W;goto _and;
		case _and_local_direct: BINARY_LOCAL_LOCAL_W;TRANSBOOL(left);goto _and;
		case _and_shared_direct: BINARY_SHARED_LOCAL_W;TRANSBOOL(left);goto _and;
		case _and_shared_local: BINARY_SHARED_LOCAL_W;TRANSBOOL(left);TRANSBOOL(right); goto _and;
		case _and_shared_shared: BINARY_SHARED_SHARED_W;TRANSBOOL(left);TRANSBOOL(right); goto _and;
		case _and_local_shared: BINARY_LOCAL_SHARED_W;TRANSBOOL(left);TRANSBOOL(right); goto _and;
		case _and_local_local: BINARY_LOCAL_LOCAL_W; TRANSBOOL(left);TRANSBOOL(right);
_and:
			if(left == GETFALSE){
				TEMP(GETFALSE);
			}
			else{
				TEMP(right);
			}
			NEXTOP(7);
			goto next;
		case _or_direct_local: BINARY_LOCAL_LOCAL_W;TRANSBOOL(right);goto _or;
		case _or_direct_shared: BINARY_LOCAL_SHARED_W;TRANSBOOL(right);goto _or;
		case _or_direct_direct: BINARY_LOCAL_LOCAL_W; goto _or;
		case _or_local_direct: BINARY_LOCAL_LOCAL_W;TRANSBOOL(left);goto _or;
		case _or_shared_direct: BINARY_SHARED_LOCAL_W;TRANSBOOL(left);goto _or;
		case _or_shared_local: BINARY_SHARED_LOCAL_W;TRANSBOOL(left);TRANSBOOL(right); goto _or;
		case _or_shared_shared: BINARY_SHARED_SHARED_W;TRANSBOOL(left);TRANSBOOL(right); goto _or;
		case _or_local_shared: BINARY_LOCAL_SHARED_W;TRANSBOOL(left);TRANSBOOL(right); goto _or;
		case _or_local_local: BINARY_LOCAL_LOCAL_W; TRANSBOOL(left);TRANSBOOL(right);
_or:
			if(left == GETTRUE){
				TEMP(GETTRUE);
			}
			else{
				TEMP(right);
			}
			NEXTOP(7);
			goto next;
		
#define COMPY(op,desc) if(ISINT(left) && ISINT(right)){ \
				TEMP(GETINT(left)##op##GETINT(right) ? GETTRUE : GETFALSE); \
			  } \
			  else if(ISNUM(left) && ISNUM(right)){ \
				TRANSFLOAT; \
				TEMP(angel_float1##op##angel_float2 ? GETTRUE : GETFALSE); \
			  } \
			  else if(ISSTR(left) && ISSTR(right))  {TEMP(comparestring(GETSTR(left),GETSTR(right))##op##0 ? GETTRUE : GETFALSE); }\
			  else{ \
				 errorinfo[errorsize]; \
				 sprintf(errorinfo,"%s ���������ߵ����Ͳ�����Ҫ��",desc); \
				 angel_error(errorinfo); \
				 goto exit; \
			}
#define EQUAL(op)  if(ISINT(left) && ISINT(right)){ \
				TEMP(GETINT(left)##op##GETINT(right)?GETTRUE : GETFALSE); \
			  } \
			  else if(ISSTR(left) && ISSTR(right))  {TEMP(comparestring(GETSTR(left),GETSTR(right))##op##0 ? GETTRUE : GETFALSE); } \
			  else TEMP(left##op##right ? GETTRUE : GETFALSE);
		
		case _big_shared_local: BINARY_SHARED_LOCAL_W; goto big;
		case _big_shared_shared: BINARY_SHARED_SHARED_W; goto big;
		case _big_local_shared: BINARY_LOCAL_SHARED_W; goto big;
		case _big_local_local: BINARY_LOCAL_LOCAL_W; goto big;
big:
			COMPY(>,">");
			NEXTOP(7)
		case _small_shared_local: BINARY_SHARED_LOCAL_W; goto _small;
		case _small_shared_shared: BINARY_SHARED_SHARED_W; goto _small;
		case _small_local_shared: BINARY_LOCAL_SHARED_W; goto _small;
		case _small_local_local: BINARY_LOCAL_LOCAL_W; goto _small;
_small:
			COMPY(<,"<");
			NEXTOP(7)
		case _equal_shared_local: BINARY_SHARED_LOCAL_W; goto equal;
		case _equal_shared_shared: BINARY_SHARED_SHARED_W; goto equal;
		case _equal_local_shared: BINARY_LOCAL_SHARED_W; goto equal;
		case _equal_local_local: BINARY_LOCAL_LOCAL_W; goto equal;
equal:
			EQUAL(==);
			NEXTOP(7)
			break;
		case _noequal_shared_local: BINARY_SHARED_LOCAL_W; goto noequal;
		case _noequal_shared_shared: BINARY_SHARED_SHARED_W; goto noequal;
		case _noequal_local_shared: BINARY_LOCAL_SHARED_W; goto noequal;
		case _noequal_local_local: BINARY_LOCAL_LOCAL_W; goto noequal;
noequal:
			EQUAL(!=);
			NEXTOP(7)
			break;
		case _small_equal_shared_local: BINARY_SHARED_LOCAL_W; goto small_equal;
		case _small_equal_shared_shared: BINARY_SHARED_SHARED_W; goto small_equal;
		case _small_equal_local_shared: BINARY_LOCAL_SHARED_W; goto small_equal;
		case _small_equal_local_local: BINARY_LOCAL_LOCAL_W; goto small_equal;
small_equal:
			COMPY(<=,"<=");
			NEXTOP(7)
			break;
		case _big_equal_shared_local: BINARY_SHARED_LOCAL_W; goto big_equal;
		case _big_equal_shared_shared: BINARY_SHARED_SHARED_W; goto big_equal;
		case _big_equal_local_shared: BINARY_LOCAL_SHARED_W; goto big_equal;
		case _big_equal_local_local: BINARY_LOCAL_LOCAL_W; goto big_equal;
big_equal:
			COMPY(>=,">=");
			NEXTOP(7)
			break;
		case _not_local: UNARY_LOCAL_W; TRANSBOOL_N(right); goto _not;
		case _not_shared: UNARY_SHARED_W; TRANSBOOL_N(right); goto _not;
		case _not_direct: UNARY_LOCAL_R; if(right == GETTRUE) {TEMP(GETFALSE);}else {TEMP(GETTRUE);} NEXTOP(5);
_not:
			TEMP(right);
			NEXTOP(5)
			break ;

		case _bool_local: UNARY_LOCAL_W; TRANSBOOL(right);goto _bool;
		case _bool_shared: UNARY_SHARED_W; TRANSBOOL(right);goto _bool;
_bool:
			TEMP(right);
			NEXTOP(5)
			break ;


		case _is_item_shared_local: BINARY_SHARED_LOCAL_W; goto is_item;
		case _is_item_shared_shared: BINARY_SHARED_SHARED_W; goto is_item;
		case _is_item_local_shared: BINARY_LOCAL_SHARED_W; goto is_item;
		case _is_item_local_local: BINARY_LOCAL_LOCAL_W; goto is_item;
is_item:
			NEXTOP(7)
			break ;


#define BITWISE_BOOL(op) if(ISINT(right) && ISINT(left)){ TEMPINT(GETINT(left)##op##GETINT(right));} \
			else{angel_error("λ�������Ͳ�����Ҫ�󣡣�"); goto exit;}
		case _bitwise_and_shared_local: BINARY_SHARED_LOCAL_W; goto bitwise_and;
		case _bitwise_and_shared_shared: BINARY_SHARED_SHARED_W; goto bitwise_and;
		case _bitwise_and_local_shared: BINARY_LOCAL_SHARED_W; goto bitwise_and;
		case _bitwise_and_local_local: BINARY_LOCAL_LOCAL_W; goto bitwise_and;
bitwise_and:
			BITWISE_BOOL(&);
			NEXTOP(7)
			break;
		case _bitwise_or_shared_local: BINARY_SHARED_LOCAL_W; goto bitwise_or;
		case _bitwise_or_shared_shared: BINARY_SHARED_SHARED_W; goto bitwise_or;
		case _bitwise_or_local_shared: BINARY_LOCAL_SHARED_W; goto bitwise_or;
		case _bitwise_or_local_local: BINARY_LOCAL_LOCAL_W; goto bitwise_or;
bitwise_or:
			BITWISE_BOOL(|);
			NEXTOP(7)
			break;
		case _bitwise_xor_shared_local: BINARY_SHARED_LOCAL_W; goto bitwise_xor;
		case _bitwise_xor_shared_shared: BINARY_SHARED_SHARED_W; goto bitwise_xor;
		case _bitwise_xor_local_shared: BINARY_LOCAL_SHARED_W; goto bitwise_xor;
		case _bitwise_xor_local_local: BINARY_LOCAL_LOCAL_W; goto bitwise_xor;
bitwise_xor:
			BITWISE_BOOL(^);
			NEXTOP(7)
			break;

		case _jnp_bool_local: UNARY_LOCAL_R; goto jnp_bool;
		case _jnp_bool_shared: UNARY_SHARED_R;
jnp_bool:
			TRANSBOOL(right);
			goto jnp;
		case _jnp: UNARY_LOCAL_R;
jnp:
			if(right == GETFALSE)
				pc=exec_byte->code+*(unsigned long*)(pc+3);
			else
				NEXTOP(7);
			continue ;
		case _jmp:
			pc=exec_byte->code+*(unsigned long*)(pc+1);
			continue ;
		case _switch_case_local: UNARY_LOCAL_R; goto switch_case;
		case _switch_case_shared: UNARY_SHARED_R;
switch_case:
			pc=exec_byte->code+switchcase(_global_sw_list->st_table[PARAM(3)],right);
			goto next;


		/*
		��������ָ��
		*/
		//ֱ�ӷŵ�ջ��
		//ע�����ʱ���Ǽ�������׶λ�û�����ú�����һ��
		case _push_local: UNARY_LOCAL_R; goto push;
		case _push_shared: UNARY_SHARED_R;
push:
			flag = ++angel_runtime->push_pos;
			//STACKTOHEAP(right); //δ����ĳ��ʱ�̻ᷢ������

			offset2 = angel_runtime->data+flag-exec_environment;
			STACKHEAPCOPY(right,offset2);
			
			//angel_runtime->data[flag] = right;
			NEXTOP(3)
		case _call:
			offset1 = PARAM(1);  //��÷���ֵ��д��λ��
			memfun = global_function->fun_item[PARAM(3)];
			if(!memfun)
				goto exit;
			offset = PARAM(5);
			ADDOP(7)
precall:
			longoffset=memfun->base_addr[memfun->paracount-offset];
			
			e=env_reg->env_item[env_reg->len++];
			e->ret_temp_num = offset1;
			e->pc_reg=pc; 
			e->bf=exec_fun; //��exec_fun������ΪNULL������ں�����
			e->baseaddr = exec_environment;


			exec_fun=memfun;
			pc=exec_byte->code+longoffset;
			AJUSTSTACKHEAPPTR(exec_fun->localcount);
			//���亯���ֲ���������Ҫ�Ŀռ�,���õ�ǰ�Ļ��������Ȳ����ã�ע�����o��д��exec_environment
			
			flag = angel_runtime->push_pos-offset;  //flag��ʱ��ʾ��û��push������ʱ���ջ��λ��
			
			//��Ҫԭ������pushΪ���ģ�ͨ��push��ָ�뽨�������ռ�Ļ���ַ
			exec_environment = angel_runtime->data+flag+1;  //-exec_bp-1

			flag += exec_fun->localcount;  //������ܿռ�
			if(flag >= runtime_max_size)
			{
				angel_error("����ջ�����");
				goto exit;
			}
			angel_runtime->push_pos = angel_runtime->top = flag;

			if(exec_fun->type == 0)
				continue ;
			if(exec_fun->type == 3)
				continue ;
			if(exec_fun->type == 1)
			{
				SECURE
				right = initobject();
				right->c = class_reg;
				THIS = right;
				EXIT_SECURE
			}
			else if(exec_fun->type == 2)
			{
putthistotop:
				THIS = right;
			}
			continue ;
		case _sys_call:
			//��������жϾ�������Ǹ�����]
			offset = PARAM(5); //��������
			
			angel_sys_function = &angel_build_in_def[PARAM(3)];
			offset1 = PARAM(1); //����ֵ
			ADDOP(7);

syscallkernel:
			
			goto _syscallkernel;

		//ע��������Ҫ��̬����ָ�ִ��,����ֻ�ǵ�һ���ö�̬����ķ��������������취�����Ĭ�ϲ��������׵�ַ��취�������������ƿ���
		case _call_member_local: UNARY_LOCAL_W; goto call_member;
		case _call_member_shared: UNARY_SHARED_W;
call_member:
			//right�Ƕ���
			offset1 = PARAM(1);  //����ֵ
			memname = angel_name[PARAM(5)];
			offset = PARAM(7);  //��������
			ADDOP(9)

			if(ISNU(right))
			{
				angel_error("�ն���û�г�Ա������");
				goto exit;
			}
			//�ж��ǲ������õĿ⺯��
			if(ISBUILDINTYPE(right))
			{
				//ע������offsetһ������Ϊ�Դ�����������Ȼ�����ܵ�local��
				//angel_runtime->data[angel_runtime->push_pos+1] = right;
				angel_sys_function = getsysmembercall(right,GETSTR(memname)->s,offset+1);  //��1�ǽ�����Ҳ����
				if(!angel_sys_function)
				{
					char errorinfo[errorsize];
					sprintf(errorinfo,"���ö����Ա����%sδ������������������",GETSTR(memname)->s);
					angel_error(errorinfo);
					goto exit;
				}
				angel_runtime->data[angel_runtime->push_pos + angel_sys_function->argcount - offset] = right;
_syscallkernel:
				flag = angel_runtime->push_pos-offset;  //flag��ʱ��ʾ��û��push������ʱ���ջ��λ��
				
				//��Ҫԭ������pushΪ���ģ�ͨ��push��ָ�뽨�������ռ�Ļ���ַ
				sys_call_bp=angel_runtime->data+flag+1;  //-exec_bp-1
				
				offset = angel_sys_function->argcount;  //�ܵ�local��
				flag += offset;  //������ܿռ�
				if(flag >= runtime_max_size)
				{
					angel_error("����ջ�����");
					goto exit;
				}
				angel_runtime->push_pos = angel_runtime->top = flag;

				if(!angel_sys_function)
					goto exit;

				//�����scan������ᵼ���޷���ʱ����
				SECURE
				//fast_lock();
				right = system_call(sys_call_bp,*angel_sys_function); //������
				for(flag = 0; flag < offset; flag++) sys_call_bp[flag] = angel_uninitial;
				exec_environment[offset1] = right;
				EXIT_SECURE
				//fast_unlock();
				goto commonret;
			}

			memfun=getmemfun(right,GETSTR(memname)->s,offset);
			if(!memfun)
			{
				if(!right->c)
					goto exit;
				sprintf(errorinfo,"����%sû�г�Ա����%s����Ĳ�������������",right->c->name,GETSTR(memname)->s);
				angel_error(errorinfo);
				goto exit;
			}
			if(!memfun)
				goto exit;
			goto precall;
		case _call_static:
			offset1 = PARAM(1);
			class_reg = clist->c[PARAM(3)];
			memname = angel_name[PARAM(5)];
			offset = PARAM(7);
			ADDOP(9)
			goto staticmemfun;
		case _call_static_default:
			offset1 = PARAM(1); //����ֵ
			class_reg = THIS->c;
			memname = angel_name[PARAM(3)];
			offset = PARAM(5);
			ADDOP(7);
staticmemfun:
			memfun=getclassfun(class_reg,GETSTR(memname)->s,offset);
			if(!memfun)
			{
				sprintf(errorinfo,"��%sû�о�̬��Ա����%s",class_reg->name,memname);
				angel_error(errorinfo);
				goto exit;
			}
			goto precall;

			
		case _call_default:
			offset1 = PARAM(1);
			memname=angel_name[PARAM(3)];
			offset = PARAM(5);
			ADDOP(7)
			//o=angel_runtime->data[angel_runtime->top];
			if(exec_fun->type != 3) //��������߲��Ǿ�̬����
			{
				//�ҵ���Ա�������ڵĶ���
				right = THIS;
				memfun=getobjmemfun(right,GETSTR(memname)->s,offset);
			}
			else
				memfun=getclassfun(exec_fun->class_context,GETSTR(memname)->s,offset);

			if(!memfun)
			{
				//��ν���һ��prepare��������е�this�ͷŵ�
				if((flag=isglobalsyscall(GETSTR(memname)->s,offset))>=0)
				{
					//lib_fun=funlib[flag]; //��������Ǹ��ƹ��̶����ǻ��ָ��
					//������֤�ڲ�ͬ��ִ�л����¶�����Ӧ�ĸ���
					//������Ĵ����ǲ��ֵ�ǰexec_byte��˭��
dealsyscall:
					angel_sys_function = &angel_build_in_def[flag];
					goto _syscallkernel; 
				}
				memfun=getfun(global_function,global_function_map,GETSTR(memname)->s,offset);  //���Ƿ���ȫ��
				if(!memfun)
					goto exit;
			}
			goto precall;
		case _call_back:
			offset1 = PARAM(1);  //��÷���ֵ��д��λ��
			function = (object_fun)exec_environment[PARAM(3)];
			offset = PARAM(5);
			ADDOP(7);
callback:
			tempval = dynamic_call(function,offset);
			if(!tempval)
			{
				angel_error("��̬���ú�������������Ҫ��");
				goto exit;
			}
			if(ISUSERFUN(function))
			{
				memfun = (fun)tempval;
				goto precall;
			}
			else
			{
				angel_sys_function = (angel_buildin_func *)tempval;
				goto syscallkernel;
			}
			goto exit;
		case _ret:
		case _ret_anyway:
			right = angel_uninitial;
			goto usrfunret;
		case _ret_obj:
			right = THIS;
			goto usrfunret;
		case _ret_with_local: UNARY_LOCAL_R; goto usrfunret;
		case _ret_with_shared: UNARY_SHARED_R;
usrfunret:
			//�����ʾ�û��������ع��̣�������Ҫ�ָ�pc��fun��map����
			offset = exec_fun->localcount;

			//�ͷ�ջ�ռ�
			for(int i = 0; i<offset; i++)
			{
				exec_environment[i] = angel_uninitial;
			}

			e=env_reg->env_item[--env_reg->len];

			AJUSTSTACKHEAPPTR(-exec_fun->localcount);

			exec_fun = e->bf;
			//ȡ�ϵ�
			pc = e->pc_reg;
			//�ָ�����ַ
			exec_environment = e->baseaddr;

			offset2 = e->ret_temp_num;
		
			if(!right)
			{
				STORE(offset2,right);
			}
			else
			{
				STACKHEAPCOPY(right,offset2);
			}
commonret:
			if(!pc)  //��ʾ�߳��������Ѿ�����
			{
				goto exit;
			}
			angel_runtime->top-=offset;  //ע����ϵͳ����ʱ�����offsetʱ�����
			angel_runtime->push_pos = angel_runtime->top;

			continue ;


		/*
		inplaceָ��
		����loadaddrϵ��ָ�����������ܣ�load������+loadaddr
		���loadaddrָ������������ָ����������һ��ԭ�Ӳ���
		*/
		case _loadaddr_index_shared_local: BINARY_SHARED_LOCAL_R; goto loadaddrindex;
		case _loadaddr_index_shared_shared: BINARY_SHARED_SHARED_R; goto loadaddrindex;
		case _loadaddr_index_local_shared: BINARY_LOCAL_SHARED_R; goto loadaddrindex;
		case _loadaddr_index_local_local: BINARY_LOCAL_LOCAL_R;
loadaddrindex:
			flag = 1;
			if(ISLIST(left))
			{
				OUTOFBOUNDEXCEPT_L(left,right);
				angel_inplace_addr_reg = &GETLIST(left)->item[angel_int2];
			}
			else if(ISDICT(left))
			{
				angel_int2=getdictindex(GETDICT(left),right);
				entry = GETDICT(left)->hashtable[angel_int2];
				if(!entry)
				{
					angel_error("�ֵ䲻���ڸü���");
					goto exit;
				}
				angel_inplace_addr_reg = &entry->value;
			}
			else
			{
				angel_error("inplace����ǰ����Ϊ�б���ֵ����ͣ�");
				goto exit;
			}
			ADDOP(5);
			goto atom;
		case _loadaddr_dynamic:
			flag = 1;
			memname = angel_name[PARAM(1)];
			if(exec_fun->type != 3)
			{
				o = THIS;
				GETMEMBER(o);
			} 
			else 
			{ 
				class_reg = THIS->c;
				GETSTATIC;
			}
			angel_inplace_addr_reg = &entry->value;
			
			ADDOP(3);
			goto atom;
		case _loadaddr_member_local: UNARY_LOCAL_R; goto loadaddrdynamic;
		case _loadaddr_member_shared: UNARY_SHARED_R
loadaddrdynamic:
			flag = 1;
			memname = angel_name[PARAM(3)];

			GETMEMBER(right);
			angel_inplace_addr_reg = &entry->value;
			
			ADDOP(5);
			goto atom;
		case _loadaddr_static:
			flag = 1;
			class_reg = clist->c[PARAM(1)];
			memname = angel_name[PARAM(3)];

			GETSTATIC;
			angel_inplace_addr_reg = &entry->value;
			
			ADDOP(5);
			goto atom;
		case _loadaddr_static_default:
			flag = 1;
			memname = angel_name[PARAM(1)];
			class_reg = THIS->c;

			GETSTATIC;
			angel_inplace_addr_reg = &entry->value;
			
			ADDOP(3);
			goto atom;

		
		case _inplace_add_global_local: BINARY_SHARED_LOCAL_INPLACE; goto inplace_add;
		case _inplace_add_global_shared: BINARY_SHARED_SHARED_INPLACE; goto inplace_add;
		case _inplace_add_local_shared: BINARY_LOCAL_SHARED_INPLACE; goto inplace_add;
		case _inplace_add_local_local: BINARY_LOCAL_LOCAL_INPLACE; goto inplace_add;
inplace_add:
			SECURE;
			if(ISINT(left) && ISINT(right))
			{
				right = (object)initinteger(GETINT(left)+GETINT(right));
			}
			else if(ISNUM(left) && ISNUM(right))
			{
				TRANSFLOAT
				right = (object)initfloat(angel_float1+angel_float2);
			}
			else 
			{
				right = collection_add(left,right);
				if(!right)
					goto exit;
			}
			INPLACE_TEMP(right);
			EXIT_SECURE;
			goto next;
		case _inplace_sub_global_local: BINARY_SHARED_LOCAL_INPLACE; goto inplace_sub;
		case _inplace_sub_global_shared: BINARY_SHARED_SHARED_INPLACE; goto inplace_sub;
		case _inplace_sub_local_shared: BINARY_LOCAL_SHARED_INPLACE; goto inplace_sub;
		case _inplace_sub_local_local: BINARY_LOCAL_LOCAL_INPLACE; 
inplace_sub:
			SECURE;
			if(ISINT(left) && ISINT(right))
			{
				right = (object)initinteger(GETINT(left)-GETINT(right));
			}
			else if(ISNUM(left) && ISNUM(right))
			{
				TRANSFLOAT
				right = (object)initfloat(angel_float1-angel_float2);
			}
			else
			{
				angel_error("�����������߱��������α�����");
				goto exit;
			}
			INPLACE_TEMP(right);
			EXIT_SECURE;
			goto next;
		case _inplace_mult_global_local: BINARY_SHARED_LOCAL_INPLACE; goto inplace_mult;
		case _inplace_mult_global_shared: BINARY_SHARED_SHARED_INPLACE; goto inplace_mult;
		case _inplace_mult_local_shared: BINARY_LOCAL_SHARED_INPLACE; goto inplace_mult;
		case _inplace_mult_local_local: BINARY_LOCAL_LOCAL_INPLACE; goto inplace_mult;
inplace_mult:
			SECURE;
			if(ISINT(left) && ISINT(right))
			{
				right = (object)initinteger(GETINT(left)*GETINT(right));
			}
			else if(ISNUM(left) && ISNUM(right))
			{
				TRANSFLOAT
				right = (object)initfloat(angel_float1*angel_float2);
			}
			else
			{
				right = collection_mult(left,right);
				if(!right)
					goto exit;
			}
			INPLACE_TEMP(right);
			EXIT_SECURE;
			goto next;
			break ;
		case _inplace_div_global_local: BINARY_SHARED_LOCAL_INPLACE; goto _inplace_div;
		case _inplace_div_global_shared: BINARY_SHARED_SHARED_INPLACE; goto _inplace_div;
		case _inplace_div_local_shared: BINARY_LOCAL_SHARED_INPLACE; goto _inplace_div;
		case _inplace_div_local_local: BINARY_LOCAL_LOCAL_INPLACE; goto _inplace_div;
_inplace_div:
			SECURE;
			if(ISINT(left) && ISINT(right))
			{
				right = (object)initinteger(GETINT(left)/GETINT(right));
			}
			else if(ISNUM(left) && ISNUM(right))
			{
				TRANSFLOAT
				right = (object)initfloat(angel_float1/angel_float2);
			}
			else
			{
				angel_error("�����������߱��������α�����");
				goto exit;
			}
			INPLACE_TEMP(right);
			EXIT_SECURE;
			goto next;
		case _inplace_mod_global_local: BINARY_SHARED_LOCAL_INPLACE; goto inplace_mod;
		case _inplace_mod_global_shared: BINARY_SHARED_SHARED_INPLACE; goto inplace_mod;
		case _inplace_mod_local_shared: BINARY_LOCAL_SHARED_INPLACE; goto inplace_mod;
		case _inplace_mod_local_local: BINARY_LOCAL_LOCAL_INPLACE; goto inplace_mod;
inplace_mod:
			SECURE;
			if(ISINT(left) && ISINT(right))
			{
				right = (object)initinteger(GETINT(left)%GETINT(right));
			}
			else
			{
				if(ISSTR(left) && ISLIST(right))  //�ַ��������㷨,�Ĵ�����Ķ����ݴ�����
				{
					//�����ԭ������reg1Ϊ�������reg1->count==0�����Ż�����ֱ�Ӹ���
					//������Ҫ����һ���µĶ���
					//INPLACE_TEMP((object)strfomat(GETSTR(o),GETLIST(right)));
					;
				}
				else
				{
					angel_error("ȡģ���ߵ����Ͳ�����Ҫ��");
					goto exit;
				}
			}
			INPLACE_TEMP(right);
			EXIT_SECURE;
			goto next;
		case _inplace_lshift_global_local: BINARY_SHARED_LOCAL_INPLACE; goto inplace_lshift;
		case _inplace_lshift_global_global: BINARY_SHARED_SHARED_INPLACE; goto inplace_lshift;
		case _inplace_lshift_local_local: BINARY_LOCAL_LOCAL_INPLACE; goto inplace_lshift;
		case _inplace_lshift_local_global: BINARY_LOCAL_SHARED_INPLACE;
inplace_lshift:
			SECURE;
			if(ISINT(left) && ISINT(right))
			{
				right = (object)initinteger(GETINT(left) << GETINT(right));
			}
			else
			{
				angel_error("�������ߵ����Ͳ�����Ҫ��");
				goto exit;
			}
			INPLACE_TEMP(right);
			EXIT_SECURE;
			goto next;
		case _inplace_rshift_global_local: BINARY_SHARED_LOCAL_INPLACE; goto inplace_rshift;
		case _inplace_rshift_global_global: BINARY_SHARED_LOCAL_INPLACE; goto inplace_rshift;
		case _inplace_rshift_local_local: BINARY_SHARED_LOCAL_INPLACE; goto inplace_rshift;
		case _inplace_rshift_local_global: BINARY_SHARED_LOCAL_INPLACE; 
inplace_rshift:
			SECURE;
			if(ISINT(left) && ISINT(right))
			{
				right = (object)initinteger(GETINT(left) >> GETINT(right));
			}
			else
			{
				angel_error("�������ߵ����Ͳ�����Ҫ��");
				goto exit;
			}
			INPLACE_TEMP(right);
			EXIT_SECURE;
			goto next;
#define BITWISE_BOOL_INPLACE(op) if(ISINT(right) && ISINT(left)){ \
				SECURE; right = (object)(initinteger(GETINT(left)##op##GETINT(right))); INPLACE_TEMP(right) \
				EXIT_SECURE \
			} \
			else{angel_error("λ�������Ͳ�����Ҫ�󣡣�"); goto exit;}
		case _inplace_bitwise_and_global_local: BINARY_SHARED_LOCAL_INPLACE; goto inplace_inplace_and;
		case _inplace_bitwise_and_global_shared: BINARY_SHARED_SHARED_INPLACE; goto inplace_inplace_and;
		case _inplace_bitwise_and_local_shared: BINARY_LOCAL_SHARED_INPLACE; goto inplace_inplace_and;
		case _inplace_bitwise_and_local_local: BINARY_LOCAL_LOCAL_INPLACE; goto inplace_inplace_and;
inplace_inplace_and:
			BITWISE_BOOL_INPLACE(&);
			goto next;
		case _inplace_bitwise_or_global_local: BINARY_SHARED_LOCAL_INPLACE; goto inplace_inplace_or;
		case _inplace_bitwise_or_global_shared: BINARY_SHARED_SHARED_INPLACE; goto inplace_inplace_or;
		case _inplace_bitwise_or_local_shared: BINARY_LOCAL_SHARED_INPLACE; goto inplace_inplace_or;
		case _inplace_bitwise_or_local_local: BINARY_LOCAL_LOCAL_INPLACE; goto inplace_inplace_or;
inplace_inplace_or:
			BITWISE_BOOL_INPLACE(|);
			goto next;
		case _inplace_bitwise_xor_global_local: BINARY_SHARED_LOCAL_INPLACE; goto inplace_inplace_xor;
		case _inplace_bitwise_xor_global_shared: BINARY_SHARED_SHARED_INPLACE; goto inplace_inplace_xor;
		case _inplace_bitwise_xor_local_shared: BINARY_LOCAL_SHARED_INPLACE; goto inplace_inplace_xor;
		case _inplace_bitwise_xor_local_local: BINARY_LOCAL_LOCAL_INPLACE; goto inplace_inplace_xor;
inplace_inplace_xor:
			BITWISE_BOOL_INPLACE(^);
			goto next;

		/*
		�����Լ�����
		*/
#define SELF_EXCEPTON if(!ISINT(right)) {angel_error("�����Խ������������������ͣ�"); goto exit;}
#define SELF_ADD_L right = *angel_inplace_addr_reg; SELF_EXCEPTON; SECURE right = (object)initinteger(GETINT(right)+1); TEMP(right);  INPLACE_TEMP(right);EXIT_SECURE
#define SELF_SUB_L right = *angel_inplace_addr_reg; SELF_EXCEPTON; SECURE right = (object)initinteger(GETINT(right)-1); TEMP(right);  INPLACE_TEMP(right);EXIT_SECURE
#define SELF_ADD_R right = *angel_inplace_addr_reg; SELF_EXCEPTON; SECURE TEMP(right); right = (object)initinteger(GETINT(right)+1);  INPLACE_TEMP(right);EXIT_SECURE
#define SELF_SUB_R right = *angel_inplace_addr_reg; SELF_EXCEPTON; SECURE TEMP(right); right = (object)initinteger(GETINT(right)-1);  INPLACE_TEMP(right);EXIT_SECURE
		case _self_ladd_local:
			if(angel_inplace_addr_reg)
			{
				SELF_ADD_L;
				NEXTOP(3);
			}
			else 
			{
				angel_inplace_addr_reg = &exec_environment[PARAM(3)];
				SELF_ADD_L;
				NEXTOP(5);
			}
		case _self_ladd_shared:
			if(angel_inplace_addr_reg)
			{
				SELF_ADD_L;
				NEXTOP(3);
			}
			else 
			{
				angel_inplace_addr_reg = &angel_shared[PARAM(3)];
				SELF_ADD_L;
				NEXTOP(5);
			}
		case _self_radd_local:
			if(angel_inplace_addr_reg)
			{
				SELF_ADD_R;
				NEXTOP(3);
			}
			else 
			{
				angel_inplace_addr_reg = &exec_environment[PARAM(3)];
				SELF_ADD_R;
				NEXTOP(5);
			}
		case _self_radd_shared:
			if(angel_inplace_addr_reg)
			{
				SELF_ADD_R;
				NEXTOP(3);
			}
			else 
			{
				angel_inplace_addr_reg = &angel_shared[PARAM(3)];
				SELF_ADD_R;
				NEXTOP(5);
			}
		case _self_lsub_local:
			if(angel_inplace_addr_reg)
			{
				SELF_SUB_L;
				NEXTOP(3);
			}
			else 
			{
				angel_inplace_addr_reg = &exec_environment[PARAM(3)];
				SELF_ADD_L;
				NEXTOP(5);
			}
		case _self_lsub_shared:
			if(angel_inplace_addr_reg)
			{
				SELF_SUB_L;
				NEXTOP(3);
			}
			else 
			{
				angel_inplace_addr_reg = &angel_shared[PARAM(3)];
				SELF_SUB_L;
				NEXTOP(5);
			}
		case _self_rsub_local:
			if(angel_inplace_addr_reg)
			{
				SELF_SUB_R;
				NEXTOP(3);
			}
			else 
			{
				angel_inplace_addr_reg = &exec_environment[PARAM(3)];
				SELF_SUB_R;
				NEXTOP(5);
			}
		case _self_rsub_shared:
			if(angel_inplace_addr_reg)
			{
				SELF_SUB_R;
				NEXTOP(3);
			}
			else 
			{
				angel_inplace_addr_reg = &angel_shared[PARAM(3)];
				SELF_SUB_R;
				NEXTOP(5);
			}



		case _build_list:
			SECURE 
			right = (object)initarray(PARAM(3));
			TEMP(right);
			EXIT_SECURE 
			NEXTOP(5);
		case _append_list_local: UNARY_LOCAL_R; goto append;
		case _append_list_shared: UNARY_SHARED_R;
append:
			//SECURE 
			left = exec_environment[PARAM(3)];
			addlist(GETLIST(left),right);
			//EXIT_SECURE 
			NEXTOP(5);
		case _extend_list_local: UNARY_LOCAL_R; goto extendlist;
		case _extend_list_shared: UNARY_SHARED_R;
extendlist:
			//SECURE
			left = exec_environment[PARAM(3)];
			appendlist(GETLIST(left),GETLIST(right));
			//EXIT_SECURE 
			NEXTOP(5);

		case _build_set:
			SECURE
			right = (object)copyset(GETSET(angel_shared[PARAM(3)]));
			TEMP(right);
			EXIT_SECURE
			NEXTOP(5);
		case _add_set_local: UNARY_LOCAL_R; goto addset;
		case _add_set_shared: UNARY_SHARED_R;
addset:
			//SECURE
			left = exec_environment[PARAM(3)];
			addset(GETSET(left),right);
			//EXIT_SECURE
			NEXTOP(5);
		case _build_dict:
			SECURE 
			right = (object)copydict(GETDICT(angel_shared[PARAM(3)]));
			TEMP(right);
			EXIT_SECURE 
			NEXTOP(5);
		case _init_iter_local: UNARY_LOCAL_W; goto inititeration;
		case _init_iter_shared: UNARY_SHARED_W;
inititeration:
			SECURE 
			right = (object)inititerator(right);
			//�����治��ҪEXIT_SECURE��Ϊ������ʱ������������Զ��ڵ���������ref--
			TEMP(right);
			NEXTOP(5)
		case _iter:
			SECURE 
			angel_iter=(object_iterator)exec_environment[PARAM(3)];
			left = angel_iter->base;
			switch(left->type)//�ݹ���ú���
			{
			case RANGE:
				right = (object)initinteger(GETRANGE(left)->begin + angel_iter->pointer * GETRANGE(left)->step);
				if(angel_iter->pointer >= GETRANGE(left)->n){
					goto iterend_secure;
				}
				else{
					goto iternext_secure;
				}
			case STR:
				right = (object)initstring(2);
				*((uint16_t *)GETSTR(right)->s) = GETCHAR(left,angel_iter->pointer);
				if((angel_iter->pointer*2 >= GETSTR(left)->len)){
iterend_secure:
					EXIT_SECURE
					goto iterend;
				}
				else{
iternext_secure:
					TEMP(right);
					EXIT_SECURE
					goto iternext;
				}
			case LIST:  //����û���ж�Խ�����
				right = GETLIST(left)->item[angel_iter->pointer];
				if(angel_iter->pointer >= GETLIST(left)->len){
iterend:
					_DECREF(angel_iter);
					pc=exec_byte->code+*(unsigned long*)(pc+5);
					continue ;
				}
				else{
					TEMP(right);
iternext:
					angel_iter->pointer++;
					NEXTOP(9);
				}
			case SET:
				while(angel_iter->pointer < GETSET(left)->alloc_size)
				{
					right = GETSET(left)->element[angel_iter->pointer];
					if(right){
						TEMP(right);
						goto iternext;
					}
					else
						angel_iter->pointer++;
				}
				goto iterend;
			case DICT:
				while(angel_iter->pointer < GETDICT(left)->alloc_size)
				{
					entry = GETDICT(left)->hashtable[angel_iter->pointer];
					if(entry)
					{
						TEMP(entry->key);
						goto iternext;
					}
					else
						angel_iter->pointer++;
				}
				goto iterend;
			default:
				angel_error("�������Ķ���������б��ֵ�򼯺ϣ�");
				goto exit;
			}
		case _init_range_shared_local: BINARY_SHARED_LOCAL_W; goto init_range;
		case _init_range_shared_shared: BINARY_SHARED_SHARED_W goto init_range;
		case _init_range_local_local: BINARY_LOCAL_LOCAL_W goto init_range;
		case _init_range_local_shared: BINARY_LOCAL_SHARED_W goto init_range;
init_range:
			SECURE 
			if(!ISINT(left))
			{
				angel_error("range����������������");
				goto exit;
			}
			if(!ISINT(right))
			{
				angel_error("range���յ������������");
				goto exit;
			}
			right = (object)initrange(GETINT(left),GETINT(right));
			TEMP(right);
			EXIT_SECURE
			NEXTOP(7);
		case _range_step:
			left = exec_environment[PARAM(1)];
			right = exec_environment[PARAM(3)];
			if(!ISRANGE(left))
			{
				angel_error("�������ӦΪ���Խṹ��");
				goto exit;
			}
			if(!ISINT(right))
			{
				angel_error("����ӦΪ������");
				goto exit;
			}
			if(GETRANGE(left)->begin > GETRANGE(left)->end)
				GETRANGE(left)->step = -GETINT(right);
			else
				GETRANGE(left)->step = GETINT(right);
			NEXTOP(5);
		case _init_class:
			class_reg = clist->c[PARAM(1)];
			NEXTOP(3);


		case _dynamic_get_function:
			SECURE 
			switch(*(pc+1))
			{
			case 0:
				angel_int1 = -1;
				memname = angel_name[PARAM(4)];
				left = NULL;
				flag = 6;
				break ;
			case 1:
				angel_int1 = -1;
				left = exec_environment[PARAM(4)];
				memname = angel_name[PARAM(6)];
				flag = 8;
				break ;
			/*case 2:
				left = exec_environment[PARAM(4)];
				angel_int1 = GETINT(left);
				left = NULL;
				memname = angel_name[PARAM(6)];
				flag = 8;
				break ;
			case 3:
				left = exec_environment[PARAM(4)];
				memname = angel_name[PARAM(6)];
				right = exec_environment[PARAM(8)];
				angel_int1 = GETINT(right);
				flag = 10;
				break ;*/
			}
			right = (object)dynamic_get_function(GETSTR(memname)->s,angel_int1,left);
			if(!right)
				goto exit;
			TEMP(right);
			EXIT_SECURE 
			NEXTOP(flag);
		case _end:
			goto exit;
		default:
			angel_error("��Чָ�");
			goto exit;
		}

next:
		;
	}
exit:
	freeenv(env_reg);
	free_runable(thread_controll);
	free(angel_runtime->data);
	free(angel_runtime);
//	free(global_value_list->item);  ���潻��Ҫ����
//	free(angel_name);
}