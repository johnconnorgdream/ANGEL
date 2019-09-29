/*
第一类操作是算数逻辑运算，因此我们可以先做如下的理论假设：即关于一般对象只存在创建和赋值操作，而这两个操作都是可以有虚拟机自动完成而不需要指令的指示，也就不需要寄存器的存在。
第二类操作是取子元素操作，这类操作有去列表的子元素和对象的成员变量，单这种操作基本上在同一时刻只有一个中间结果，所以可以设置一个缓冲基地址寄存器。
这里主要是用来执行字节码的
代码转载使用程序段装入制，所有程序存储在内存数组中，不限长度大小，程序代码的首地址要用四个字节来表示


注意这里是执行引擎，所有的虚拟机性能都几种体现在这个环节，要尽可能的减少代码所带来的时间开销，可以用代码两和空间来换取比如说，减少分支语句的产生，要用switch-case的形式取代。
要将频繁的操作变为字符替换而非函数调用的形式。




这里关于计算缓存优化还需要做，这里主要问题就是计算结果一般默认送到结果寄存器组中，然后做下一计算前要将结果寄存器中的数据放到
操作数寄存器中。可能后面需要做优化代码的工作，这个过程可能被分割为两个步骤即计算和存数，默认存数放在高速寄存器中，当遇到操作符两边均为操作时，


注意字符串是传递值而列表是传递引用,虚拟机在原有的基础之上增加了模糊指令，这时这对某些需要在运行时才能确定操作数类型的情况下。
指令优化一方面是关于数据运算的整体走向，这个不方便更改，另一个是关于字符串和列表的运算，将连接运算的另开辟空间的方式变为就地执行。
数字类型还是使用值传递，绝非引用传递，以后会逐步取消其count这一项而是采用其他字段。

这里要注意列表复制时对于整数的子项要整体复制。

在多线程中要搞清楚那些是全局那些是局部的。
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


extern object_list global_value_list,obj_list,dynamic_name;  //创建全局变量的列表，这里考虑将全局变量用栈的形式管理, 建立对象的列表，对象的堆首址与偏移量的关系
extern funlist global_function;  //函数指令产生的预处理块
extern object_set global_value_map,global_function_map;
extern bytecode main_byte; //这里是定义了一个存储字节码的内存空间 
extern classlist clist;
extern object_string charset[cellnum];
extern object_bytes byteset[cellnum];
extern switchlist _global_sw_list;
extern int thread_cmd;//这只会对后台线程有效果
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
	sprintf(errorinfo,"函数%s调用参数数目不对！",fl->fun_item[0]->name);
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

inline object system_call(object *bp,angel_buildin_func sys_fun)//这里考虑将默认参数赋值为NULL，当然要传入参数个数
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
字节码执行内核
*/
inline object collection_mult(object left,object right)
{
	object o;
	int angel_int1;
	if(ISSTR(left) || ISSTR(right))  //字符串连接算法,寄存器存的都是暂存数据
	{
		//这里的原则是以reg1为主，如果reg1->count==0才做优化否则直接复制
		switch(left->type)
		{
		case INT:  //说明第二个参数必须为字符串
			angel_int1 = GETINT(left);
			left = right;
			break ;
		case STR:
			if(ISINT(right))
				angel_int1 = GETINT(right);
			else
			{
				angel_error("乘法右边的类型不符合要求！");
				return NULL;
			}
			break ;
		default:
			angel_error("乘法左边的类型不符合要求！");
			return NULL;
		}
		o = (object)strrepeat(GETSTR(left),angel_int1);
	}
	else if(ISLIST(left) || ISLIST(right))
	{
		switch(left->type)
		{
		case INT:  //说明第二个参数必须为字符串
			left = right;
			angel_int1 = GETINT(left);
			break ;
		case LIST:
			;
			if(ISINT(right))
				angel_int1 = GETINT(right);
			else
			{
				angel_error("乘法右边的类型不符合要求！");
				return NULL;
			}
			break ;
		default:
			angel_error("乘法左边的类型不符合要求！");
			return NULL;
		}
		o = (object)listrepeat(GETLIST(left),angel_int1);
	}
	else if(ISBYTES(left) || ISBYTES(right))
	{
		switch(left->type)
		{
		case INT:  //说明第二个参数必须为字符串
			left = right;
			angel_int1 = GETINT(left);
			break ;
		case BYTES:
			;
			if(ISINT(right))
				angel_int1 = GETINT(right);
			else
			{
				angel_error("乘法右边的类型不符合要求！");
				return NULL;
			}
			break ;
		default:
			angel_error("乘法左边的类型不符合要求！");
			return NULL;
		}
		o = (object)bytesrepeat(GETBYTES(left),angel_int1);
	}
	else
	{
		angel_error("乘法左边的类型不符合要求！");
		return NULL;
	}
	return o;
}
inline object collection_add(object left,object right)
{
	object o;
	if(ISSTR(left) || ISSTR(right))  //字符串连接算法,寄存器存的都是暂存数据
	{
		//这里的原则是以reg1为主，如果reg1->count==0才做优化否则直接复制
		object temp = NULL;
		switch(left->type)
		{
		case INT:  //说明第二个参数必须为字符串
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
				angel_error("加法右边的类型不符合要求！");
				return NULL;
			}
			break ;
		default:
			angel_error("加法左边的类型不符合要求！");
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
		angel_error("加法两边的类型不符合要求！");
		return NULL;
	}
	return o;
}



/*

线程与同步
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
		fast_unlock(); /*注意这个unlock和leave的顺序要与fast_lock()中相反*/
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
	主线程等待
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
	
	//删除thread控制块

	
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
	//以下定义高速寄存器
	/*
	寄存器分为以下几种，一种是运算寄存器，暂存运算结果和操作数
	另外为了提高数值运算的速度，则可以准备一些对象池并
	一种是中间结果寄存器，这运算过程的
	一种是栈，用于函数调用的。
	*/
	register bytecode exec_byte;
	register int64_t angel_int1;
	register long angel_int2;
	register double angel_float1,angel_float2;


	//设置函数调用环境
	env env_reg=initenv();


	runtime_stack angel_runtime=initruntime();


	register char *pc;
	//核心引擎的入口和出口需要用寄存器变
	unsigned long longoffset;  //这是内部指针，angel_pc是全局指针
	char * res;
	register object *exec_environment,*sys_call_bp;  //运行时环境首先默认为全局环境,和基址寄存器base_addr_reg,它是处理批量变量的。
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
	//先建立全局的执行环境
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

	//线程块
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
//下面是对系统函数调用的占空间的开辟，基本上这里做了call指令和alloc_stack指令两件事，之所以这样是没有默认参数的传递
#define ALLOC_SYSCALL_STACK sys_call_bp = angel_runtime->data+angel_runtime->top; angel_runtime->top += offset; 
#define SYSRET_WITH(o) exec_environment[offset1] = o; goto commonret;
#define SYSRET SYSRET_WITH(angel_uninitial);


#define STATICNOTFOUND if(!entry){ \
							sprintf(errorinfo,"类%s不存在静态变量%s",class_reg->name,GETSTR(memname)->s); \
							angel_error(errorinfo); \
							goto exit; \
						}
#define MEMBERNOTFOUND(o) if(!entry){ \
							sprintf(errorinfo,"对象%s没有成员变量%s",o->c->name,GETSTR(memname)->s); \
							angel_error(errorinfo); \
							goto exit; \
						}
#define INDEXNEGATIVE(index) if(GETINT(index) < 0) {angel_error("索引值必须不能为负数！"); goto exit;}
#define _OUTOFBOUNDEXCEPT_L(base,index)  INDEXNEGATIVE(index) if(GETINT(index) >= GETLIST(base)->len){  angel_error("数组索引越界！"); goto exit;} angel_int2 = GETINT(index);
#define OUTOFBOUNDEXCEPT_L(base,index)  _OUTOFBOUNDEXCEPT_L(base,index);
#define _OUTOFBOUNDEXCEPT_S(base,index) INDEXNEGATIVE(index) if(GETINT(index)*2 >= GETLIST(base)->len){  angel_error("字符串索引越界！"); goto exit;} angel_int2 = GETINT(index);
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
		//这里可以直接取四个字节数据
		//这是对特殊的一类指令的预处理，获取操作数并更新angel_reg_ptr和pc两个指针，但原则一定要一次性以提高性能
		//times = *pc;
		//寄存器与上下文环境有关，即与exec_environment有关
atom:
		switch((uchar)*pc)  //以下操作都是原子操作要尽可能甚至不出现分支
		{
		case _nop:  //空操作
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
			case STR:  //如果是字符常量直接用字符池最多有256*256个
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
			case BYTES:  //如果是字符常量直接用字符池最多有256*256个
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
					angel_error("字典不存在该键！");
					goto exit;
				}
				TEMP(GETDICT(left)->hashtable[angel_int2]->value);
				break ;
			default:
				angel_error("索引操作左边必须为列表或字符串！");  //这是为了模糊运算
				goto exit;
			}
			NEXTOP(8)
			goto next;
		case _load_member_local: UNARY_LOCAL_W goto loadmember;
		case _load_member_shared: UNARY_SHARED_W
loadmember:
			//此时寄存器即是对象值
			memname = angel_name[PARAM(5)];
			
			LOADMEMBER(right);

			NEXTOP(7);
		case _load_static:  //没有操作数所以需要自己处理指针问题
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
		case _store_index_local_shared: BINARY_LOCAL_SHARED_R; //left为base，right为值，o为索引
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
					angel_error("索引值必须为整数！"); 
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
						angel_error("索引赋值对象必须是字符串！");
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
						angel_error("索引赋值对象必须是字节或数字！");
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
				angel_error("索引基地址不合法！");
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
				case BYTES: //表明这是给字符串做索引操作
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
		基本运算指令
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
				angel_error("减法运算两边必须是整形变量！");
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
					angel_error("除法错！");
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
					angel_error("除法错！");
					goto exit;
				}
				TEMPFLOAT(angel_float1/angel_float2);
			}
			else
			{
				angel_error("除法运算两边必须是整形变量！");
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
				if(ISSTR(left) && ISLIST(right))  //字符串连接算法,寄存器存的都是暂存数据
				{
					//这里的原则是以reg1为主，如果reg1->count==0才做优化否则直接复制
					//运算结果要创建一个新的对象
					//strfomat(GETSTR(o),GETLIST(right));
					;
				}
				else
				{
					angel_error("取模两边的类型不符合要求！");
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
				angel_error("左移运算两边必须是整形变量！");
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
				angel_error("右移运算两边必须是整形变量！");
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
				 sprintf(errorinfo,"%s 操作符两边的类型不符合要求！",desc); \
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
			else{angel_error("位运算类型不符合要求！！"); goto exit;}
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
		函数调用指令
		*/
		//直接放到栈中
		//注意这个时候还是计算参数阶段还没到调用函数这一步
		case _push_local: UNARY_LOCAL_R; goto push;
		case _push_shared: UNARY_SHARED_R;
push:
			flag = ++angel_runtime->push_pos;
			//STACKTOHEAP(right); //未来的某个时刻会发生错误

			offset2 = angel_runtime->data+flag-exec_environment;
			STACKHEAPCOPY(right,offset2);
			
			//angel_runtime->data[flag] = right;
			NEXTOP(3)
		case _call:
			offset1 = PARAM(1);  //获得返回值的写人位置
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
			e->bf=exec_fun; //该exec_fun可能是为NULL，即入口函数。
			e->baseaddr = exec_environment;


			exec_fun=memfun;
			pc=exec_byte->code+longoffset;
			AJUSTSTACKHEAPPTR(exec_fun->localcount);
			//分配函数局部变量所需要的空间,重置当前的环境这里先不设置，注意读用o，写用exec_environment
			
			flag = angel_runtime->push_pos-offset;  //flag此时表示在没有push参数的时候的栈底位置
			
			//主要原理是以push为核心，通过push的指针建立函数空间的基地址
			exec_environment = angel_runtime->data+flag+1;  //-exec_bp-1

			flag += exec_fun->localcount;  //分配的总空间
			if(flag >= runtime_max_size)
			{
				angel_error("函数栈溢出！");
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
			//根据序号判断具体调用那个函数]
			offset = PARAM(5); //参数个数
			
			angel_sys_function = &angel_build_in_def[PARAM(3)];
			offset1 = PARAM(1); //返回值
			ADDOP(7);

syscallkernel:
			
			goto _syscallkernel;

		//注意这里需要动态编译指令并执行,这里只是第一次用动态编译的方法，后面可以想办法将这个默认参数代码首地址想办法保存起来到控制块中
		case _call_member_local: UNARY_LOCAL_W; goto call_member;
		case _call_member_shared: UNARY_SHARED_W;
call_member:
			//right是对象
			offset1 = PARAM(1);  //返回值
			memname = angel_name[PARAM(5)];
			offset = PARAM(7);  //参数个数
			ADDOP(9)

			if(ISNU(right))
			{
				angel_error("空对象没有成员函数！");
				goto exit;
			}
			//判断是不是内置的库函数
			if(ISBUILDINTYPE(right))
			{
				//注意这里offset一定先作为显传参数个数，然后是总的local数
				//angel_runtime->data[angel_runtime->push_pos+1] = right;
				angel_sys_function = getsysmembercall(right,GETSTR(memname)->s,offset+1);  //加1是将对象也带上
				if(!angel_sys_function)
				{
					char errorinfo[errorsize];
					sprintf(errorinfo,"内置对象成员函数%s未定义或参数不符合条件",GETSTR(memname)->s);
					angel_error(errorinfo);
					goto exit;
				}
				angel_runtime->data[angel_runtime->push_pos + angel_sys_function->argcount - offset] = right;
_syscallkernel:
				flag = angel_runtime->push_pos-offset;  //flag此时表示在没有push参数的时候的栈底位置
				
				//主要原理是以push为核心，通过push的指针建立函数空间的基地址
				sys_call_bp=angel_runtime->data+flag+1;  //-exec_bp-1
				
				offset = angel_sys_function->argcount;  //总的local数
				flag += offset;  //分配的总空间
				if(flag >= runtime_max_size)
				{
					angel_error("函数栈溢出！");
					goto exit;
				}
				angel_runtime->push_pos = angel_runtime->top = flag;

				if(!angel_sys_function)
					goto exit;

				//如果是scan函数则会导致无法及时返回
				SECURE
				//fast_lock();
				right = system_call(sys_call_bp,*angel_sys_function); //函数号
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
				sprintf(errorinfo,"对象%s没有成员函数%s或传入的参数个数不满足",right->c->name,GETSTR(memname)->s);
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
			offset1 = PARAM(1); //返回值
			class_reg = THIS->c;
			memname = angel_name[PARAM(3)];
			offset = PARAM(5);
			ADDOP(7);
staticmemfun:
			memfun=getclassfun(class_reg,GETSTR(memname)->s,offset);
			if(!memfun)
			{
				sprintf(errorinfo,"类%s没有静态成员函数%s",class_reg->name,memname);
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
			if(exec_fun->type != 3) //如果调用者不是静态函数
			{
				//找到成员变量所在的对象
				right = THIS;
				memfun=getobjmemfun(right,GETSTR(memname)->s,offset);
			}
			else
				memfun=getclassfun(exec_fun->class_context,GETSTR(memname)->s,offset);

			if(!memfun)
			{
				//如何将第一次prepare到结果集中的this释放掉
				if((flag=isglobalsyscall(GETSTR(memname)->s,offset))>=0)
				{
					//lib_fun=funlib[flag]; //这个过程是复制过程而不是获得指针
					//这样保证在不同的执行环境下都有相应的副本
					//这里面的处理都是不分当前exec_byte是谁的
dealsyscall:
					angel_sys_function = &angel_build_in_def[flag];
					goto _syscallkernel; 
				}
				memfun=getfun(global_function,global_function_map,GETSTR(memname)->s,offset);  //看是否是全局
				if(!memfun)
					goto exit;
			}
			goto precall;
		case _call_back:
			offset1 = PARAM(1);  //获得返回值的写人位置
			function = (object_fun)exec_environment[PARAM(3)];
			offset = PARAM(5);
			ADDOP(7);
callback:
			tempval = dynamic_call(function,offset);
			if(!tempval)
			{
				angel_error("动态调用函数参数不符合要求！");
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
			//这里表示用户函数返回过程，其中需要恢复pc，fun，map环境
			offset = exec_fun->localcount;

			//释放栈空间
			for(int i = 0; i<offset; i++)
			{
				exec_environment[i] = angel_uninitial;
			}

			e=env_reg->env_item[--env_reg->len];

			AJUSTSTACKHEAPPTR(-exec_fun->localcount);

			exec_fun = e->bf;
			//取断点
			pc = e->pc_reg;
			//恢复基地址
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
			if(!pc)  //表示线程主函数已经返回
			{
				goto exit;
			}
			angel_runtime->top-=offset;  //注意再系统调用时，这个offset时不变的
			angel_runtime->push_pos = angel_runtime->top;

			continue ;


		/*
		inplace指令
		首先loadaddr系列指令有两个功能：load操作数+loadaddr
		其次loadaddr指令与其后的运算指令结合起来是一个原子操作
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
					angel_error("字典不存在该键！");
					goto exit;
				}
				angel_inplace_addr_reg = &entry->value;
			}
			else
			{
				angel_error("inplace索引前必须为列表或字典类型！");
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
				angel_error("减法运算两边必须是整形变量！");
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
				angel_error("除法运算两边必须是整形变量！");
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
				if(ISSTR(left) && ISLIST(right))  //字符串连接算法,寄存器存的都是暂存数据
				{
					//这里的原则是以reg1为主，如果reg1->count==0才做优化否则直接复制
					//运算结果要创建一个新的对象
					//INPLACE_TEMP((object)strfomat(GETSTR(o),GETLIST(right)));
					;
				}
				else
				{
					angel_error("取模两边的类型不符合要求！");
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
				angel_error("左移两边的类型不符合要求！");
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
				angel_error("右移两边的类型不符合要求！");
				goto exit;
			}
			INPLACE_TEMP(right);
			EXIT_SECURE;
			goto next;
#define BITWISE_BOOL_INPLACE(op) if(ISINT(right) && ISINT(left)){ \
				SECURE; right = (object)(initinteger(GETINT(left)##op##GETINT(right))); INPLACE_TEMP(right) \
				EXIT_SECURE \
			} \
			else{angel_error("位运算类型不符合要求！！"); goto exit;}
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
		自增自减操作
		*/
#define SELF_EXCEPTON if(!ISINT(right)) {angel_error("自增自建操作必须是数字类型！"); goto exit;}
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
			//这里面不需要EXIT_SECURE因为这是暂时变量，后面会自动在迭代结束后ref--
			TEMP(right);
			NEXTOP(5)
		case _iter:
			SECURE 
			angel_iter=(object_iterator)exec_environment[PARAM(3)];
			left = angel_iter->base;
			switch(left->type)//递归调用函数
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
			case LIST:  //这里没有判断越界情况
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
				angel_error("遍历语句的对象必须是列表、字典或集合！");
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
				angel_error("range的起点必须是整数！");
				goto exit;
			}
			if(!ISINT(right))
			{
				angel_error("range的终点必须是整数！");
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
				angel_error("步长左侧应为线性结构！");
				goto exit;
			}
			if(!ISINT(right))
			{
				angel_error("步长应为整数！");
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
			angel_error("无效指令！");
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
//	free(global_value_list->item);  后面交互要访问
//	free(angel_name);
}