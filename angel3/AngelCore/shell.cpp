#include <windows.h>
#include <tchar.h>
#include "data.h"
#include "runtime.h"
#include "shell.h"
#include "lib.h"
#include "execute.h"
#include "util.h"
#include "amem.h"
#include "compilerutil.h"
#include "aenv.h"


#define MAX_PATH_SIZE 1000
extern int tokenlen;
extern bytecode angel_byte;
extern object_list global_value_list,obj_list;
extern object_set global_value_map,global_function_map;
extern funlist global_function;
extern codemap fm;
extern bytecode main_byte;
extern char **directory;
extern classlist clist;
object *global_check;
option *angel_option;


void printdict(object_dict d);
void printlist(object_list l);
void printset(object_set s);
void init();
void exec_angel();
char **getenvpath(char *path)
{
	char *p = path;
	int count = 1;
	while(*p++)
	{
		if(*p == ',')
		{
			*p = 0;
			count++;
		}
	}
	p = path;
	char **res = (char **)malloc(count+1);
	for(int i=0; i<count; i++)
	{
		res[i] = p;
		p = p+strlen(p)+1;
	}
	res[count] = NULL;
	return res;
}
char *getcurrentdir()
{
	TCHAR filename[MAX_PATH_SIZE];
	GetModuleFileName(NULL,filename,MAX_PATH_SIZE);
	(_tcsrchr(filename,_T('\\')))[1] = 0;
	//获取字节长度   
	int iLength = WideCharToMultiByte(CP_ACP, 0, filename, -1, NULL, 0, NULL, NULL);
	//将tchar值赋给_char
	char *res = (char *)calloc(1,iLength);
	WideCharToMultiByte(CP_ACP, 0, filename, -1, res, iLength, NULL, NULL);
	return res;
}
option *parse_option(int argc,char **argv)
{
	int i = 1;
	option *res = (option *)calloc(1,sizeof(optionnode));
	while(i < argc)
	{
		char *optstr = argv[i++];
		if(*optstr != '-')  //默认是直接执行angel文件
		{
			res->exec_path = optstr;
		}
		else
		{
			optstr++;
			if(strcmp(optstr,"v")==0 || strcmp(optstr,"version") == 0)
			{
				angel_out("angel VM 5.0\n");
				exit(1);
			}
			else if(strcmp(optstr,"env")==0)
			{
				char *paths = argv[i++];
				res->env = getenvpath(paths);
			}
			else if(strcmp(optstr,"sh")==0)
			{
				res->flag |= 1; //表示此时进入checkshell界面
			}
			else if(strcmp(optstr,"c")==0)  //标识编译，但不执行
			{
				res->flag |= 1 << 4;  //写入高4位
			}
		}
	}
	return res;
}
char *read_line(FILE *f)
{
	int len=read_base_size;
	char *line,*p;
	line=(char *)malloc(len);
	p=line;
	char c=fgetc(f);
	if((unsigned char)c==0xef)
	{
		fseek(f,3,0);
		c = fgetc(f);
	}
	if(c==EOF)
		return NULL;
	while(c!=EOF &&  c!='\n')
	{
		if(p-line+3>len)
		{
			int offset=p-line;
			len+=read_base_size;
			line=(char *)realloc(line,len);
			p=offset+line;
		}
		*p++=c;
		c=fgetc(f);
	}
	if(p-line+1>len)
	{
		int offset=p-line;
		len+=1;
		line=(char *)realloc(line,len);
		p=offset+line;
	}
	*p=0;
	return line;
}

inline void desccode(int index,char *codetext,char *pc,int opcount)
{
	char codeinfo[maxsize];
	uchar code = *(uchar *)pc++;
	if(code >= _jnp && code <= _jnp_bool_shared)
	{
		sprintf(codeinfo,"%d: %-20s%-20d%-20d\n",index,codetext,*(int16_t *)pc,*(int *)(pc+2));angel_out(codeinfo); 
	}
	else if(code == _jmp)
	{
		sprintf(codeinfo,"%d: %-20s%-20d%\n",index,codetext,*(int *)pc);angel_out(codeinfo); 
	}
	else
	{
		sprintf(codeinfo,"%d: %-20s",index,codetext);
		angel_out(codeinfo);
		for(int i = 0; i<opcount; i++)
		{
			sprintf(codeinfo,"%-20d",*(int16_t *)pc);angel_out(codeinfo); 
			pc += 2;
		}
		angel_out("\n");
	}
}
void print_code_info(char *pc)
{
	int index=1,pcoffset,opcount;
#define DESCCODE(code) desccode(index,code,pc,opcount);
#define ADDOP(op); pc+=op;
#define NEXTOP(op); ADDOP(op); break ;
	angel_out("instruct\t\top1\t\top2\t\t\top3\t\t\top4\n");  //表头
	while(1)
	{
		pcoffset = 2;
		opcount = 0;
		switch((uchar)*pc)  //以下操作都是原子操作要尽可能甚至不出现分支
		{
		case _nop:  //空操作
			DESCCODE("nop");
			NEXTOP(1);
		case _load_dynamic:
			opcount = 2;
			DESCCODE("load_dynamic");
			NEXTOP(5);
		case _load_index_shared_local:
		case _load_index_shared_shared:
		case _load_index_local_shared:
		case _load_index_local_local:
			opcount = 3;
			DESCCODE("load_index");
			NEXTOP(8)
		case _load_member_local:
		case _load_member_shared:
			opcount = 3;
			DESCCODE("load_member");
			NEXTOP(7);
		case _load_static:  //没有操作数所以需要自己处理指针问题
			opcount = 3;
			DESCCODE("load_static");
			NEXTOP(7);
		case _load_static_default:
			opcount = 2;
			DESCCODE("load_static_default");
			NEXTOP(5);
		case _mov_local:
		case _mov_shared:
			opcount = 2;
			DESCCODE("mov");
			NEXTOP(5)
		case _store_global_local:
		case _store_global_shared:
		case _store_global_temp:
			opcount = 2;
			DESCCODE("store_global");
			NEXTOP(5)
		case _store_local_local: 
		case _store_local_shared:
		case _store_local_temp:
			opcount = 2;
			DESCCODE("store_local");
			NEXTOP(5)
		case _store_dynamic_local:
		case _store_dynamic_shared:
		case _store_dynamic_temp:
			opcount = 2;
			DESCCODE("store_dynamic");
			NEXTOP(5)
		case _store_index_shared_local:
		case _store_index_shared_shared:
		case _store_index_local_local:
		case _store_index_local_shared:
		case _store_index_shared_temp:
		case _store_index_local_temp:
			opcount = 3;
			DESCCODE("store_index");
			NEXTOP(7)
		case _store_member_shared_local:
		case _store_member_shared_shared:
		case _store_member_local_local:
		case _store_member_local_shared:
		case _store_member_shared_temp:
		case _store_member_local_temp:
			opcount = 3;
			DESCCODE("store_member");
			NEXTOP(7) 
		case _store_static_local: 
		case _store_static_shared:
		case _store_static_temp:
			opcount = 3;
			DESCCODE("store_static");
			NEXTOP(7)
		case _store_static_default_local:
		case _store_static_default_shared:
		case _store_static_default_temp:
			opcount = 2;
			DESCCODE("store_static_default");
			NEXTOP(5)
		/*
		基本运算指令
		*/
		case _add_shared_local:
		case _add_shared_shared:
		case _add_local_shared:
		case _add_local_local:
			opcount = 3;
			DESCCODE("add");
			NEXTOP(7)
		case _sub_shared_local:
		case _sub_shared_shared:
		case _sub_local_shared:
		case _sub_local_local:
			opcount = 3;
			DESCCODE("sub");
			NEXTOP(7)
		case _mult_shared_local:
		case _mult_shared_shared:
		case _mult_local_shared:
		case _mult_local_local:
			opcount = 3;
			DESCCODE("mult");
			NEXTOP(7)
		case _div_shared_local:
		case _div_shared_shared:
		case _div_local_shared:
		case _div_local_local:
			opcount = 3;
			DESCCODE("div");
			NEXTOP(7)
		case _mod_shared_local:
		case _mod_shared_shared:
		case _mod_local_shared:
		case _mod_local_local:
			opcount = 3;
			DESCCODE("mod");
			NEXTOP(7)
		case _and_direct_local:
		case _and_direct_shared:
		case _and_direct_direct:
		case _and_local_direct:
		case _and_shared_direct:
		case _and_shared_local:
		case _and_shared_shared:
		case _and_local_shared:
		case _and_local_local:
			opcount = 3;
			DESCCODE("and");
			NEXTOP(7);
		case _or_direct_local:
		case _or_direct_shared:
		case _or_direct_direct:
		case _or_local_direct:
		case _or_shared_direct:
		case _or_shared_local:
		case _or_shared_shared:
		case _or_local_shared:
		case _or_local_local:
			opcount = 3;
			DESCCODE("or");
			NEXTOP(7);
		case _big_shared_local:
		case _big_shared_shared:
		case _big_local_shared:
		case _big_local_local:
			opcount = 3;
			DESCCODE("big");
			NEXTOP(7)
		case _small_shared_local:
		case _small_shared_shared:
		case _small_local_shared:
		case _small_local_local:
			opcount = 3;
			DESCCODE("small");
			NEXTOP(7)
		case _equal_shared_local:
		case _equal_shared_shared:
		case _equal_local_shared:
		case _equal_local_local:
			opcount = 3;
			DESCCODE("equal");
			NEXTOP(7)
		case _noequal_shared_local:
		case _noequal_shared_shared:
		case _noequal_local_shared:
		case _noequal_local_local:
			opcount = 3;
			DESCCODE("noequal");
			NEXTOP(7)
		case _small_equal_shared_local:
		case _small_equal_shared_shared:
		case _small_equal_local_shared:
		case _small_equal_local_local:
			opcount = 3;
			DESCCODE("small_equal");
			NEXTOP(7)
		case _big_equal_shared_local:
		case _big_equal_shared_shared:
		case _big_equal_local_shared:
		case _big_equal_local_local:
			opcount = 3;
			DESCCODE("big_equal");
			NEXTOP(7)
		case _not_local:
		case _not_shared:
		case _not_direct:
			opcount = 2;
			DESCCODE("not");
			NEXTOP(5)
		case _bool_local:
		case _bool_shared:
			opcount = 2;
			DESCCODE("bool");
			NEXTOP(5)
		case _is_item_shared_local:
		case _is_item_shared_shared:
		case _is_item_local_shared:
		case _is_item_local_local: 
			opcount = 3;
			DESCCODE("is_item");
			NEXTOP(7)
		case _bitwise_and_shared_local:
		case _bitwise_and_shared_shared:
		case _bitwise_and_local_shared:
		case _bitwise_and_local_local:
			opcount = 3;
			DESCCODE("bitwise_add");
			NEXTOP(7)
		case _bitwise_or_shared_local:
		case _bitwise_or_shared_shared:
		case _bitwise_or_local_shared:
		case _bitwise_or_local_local:
			opcount = 3;
			DESCCODE("bitwise_or");
			NEXTOP(7)
		case _bitwise_xor_shared_local:
		case _bitwise_xor_shared_shared:
		case _bitwise_xor_local_shared:
		case _bitwise_xor_local_local:
			opcount = 3;
			DESCCODE("bitwise_xor");
			NEXTOP(7)
		case _jnp_bool_local: 
		case _jnp_bool_shared:
		case _jnp:
			opcount = 3;
			DESCCODE("jnp");
			NEXTOP(7);
		case _jmp:
			DESCCODE("jmp");
			NEXTOP(5);
		case _switch_case_local: 
		case _switch_case_shared:
			opcount = 2;
			DESCCODE("swith_case");
			NEXTOP(5);
		/*
		函数调用指令
		*/
		case _push_local:
		case _push_shared:
			opcount = 1;
			DESCCODE("push");
			NEXTOP(3)
		case _call:
			opcount = 3;
			DESCCODE("call");
			NEXTOP(7)
		case _sys_call:
			opcount = 3;
			DESCCODE("sys_call");
			NEXTOP(7);
		//注意这里需要动态编译指令并执行,这里只是第一次用动态编译的方法，后面可以想办法将这个默认参数代码首地址想办法保存起来到控制块中
		case _call_member_local: 
		case _call_member_shared:
			opcount = 4;
			DESCCODE("call_member"); 
			NEXTOP(9)
		case _call_static:
			opcount = 4;
			DESCCODE("call_static");
			NEXTOP(9)
		case _call_static_default:
			opcount = 3;
			DESCCODE("call_static_default");
			NEXTOP(7);
		case _call_default:
		case _call_back:
			DESCCODE("call_back");
			NEXTOP(1);
		case _ret:
			DESCCODE("ret");
			NEXTOP(1)
		case _ret_obj:
			DESCCODE("ret_object");
			NEXTOP(1)
		case _ret_with_local:  
		case _ret_with_shared: 
			opcount = 1;
			DESCCODE("ret_with_value");
			NEXTOP(3)
		/*
		inplace指令
		首先loadaddr系列指令有两个功能：load操作数+loadaddr
		其次loadaddr指令与其后的运算指令结合起来是一个原子操作
		*/
		case _loadaddr_index_shared_local:  
		case _loadaddr_index_shared_shared:  
		case _loadaddr_index_local_shared: 
		case _loadaddr_index_local_local:
			opcount = 2;
			DESCCODE("load_index_addr");
			pcoffset = 0;
			NEXTOP(5);
		case _loadaddr_dynamic:
			opcount = 1;
			DESCCODE("load_dynamic_addr");
			pcoffset = 0;
			NEXTOP(3);
		case _loadaddr_member_local:
		case _loadaddr_member_shared: 
			opcount = 2;
			DESCCODE("load_member_addr");
			pcoffset = 0;
			NEXTOP(5);
		case _loadaddr_static:
			opcount = 2;
			DESCCODE("load_static_addr");
			pcoffset = 0;
			NEXTOP(5);
		case _loadaddr_static_default:
			opcount = 1;
			DESCCODE("load_static_default_addr");
			pcoffset = 0;
			NEXTOP(3);
		case _inplace_add_global_local:
		case _inplace_add_global_shared:
		case _inplace_add_local_shared:
		case _inplace_add_local_local:
			opcount = 1+pcoffset/2;
			DESCCODE("inplace_add");
			NEXTOP(3+pcoffset);
		case _inplace_sub_global_local:
		case _inplace_sub_global_shared:
		case _inplace_sub_local_shared:
		case _inplace_sub_local_local:
			opcount = 1+pcoffset/2;
			DESCCODE("inplace_sub");
			NEXTOP(3+pcoffset);
		case _inplace_mult_global_local:
		case _inplace_mult_global_shared:
		case _inplace_mult_local_shared:
		case _inplace_mult_local_local:
			opcount = 1+pcoffset/2;
			DESCCODE("inplace_mult");
			NEXTOP(3+pcoffset);
		case _inplace_div_global_local:
		case _inplace_div_global_shared:
		case _inplace_div_local_shared:
		case _inplace_div_local_local:
			opcount = 1+pcoffset/2;
			DESCCODE("inplace_div");
			NEXTOP(3+pcoffset);
		case _inplace_mod_global_local:
		case _inplace_mod_global_shared:
		case _inplace_mod_local_shared:
		case _inplace_mod_local_local:
			opcount = 1+pcoffset/2;
			DESCCODE("inplace_mod");
			NEXTOP(3+pcoffset);
		case _inplace_bitwise_and_global_local:
		case _inplace_bitwise_and_global_shared:
		case _inplace_bitwise_and_local_shared:
		case _inplace_bitwise_and_local_local:
			opcount = 1+pcoffset/2;
			DESCCODE("inplace_bitwise_and");
			NEXTOP(3+pcoffset);
		case _inplace_bitwise_or_global_local:
		case _inplace_bitwise_or_global_shared:
		case _inplace_bitwise_or_local_shared:
		case _inplace_bitwise_or_local_local:
			opcount = 1+pcoffset/2;
			DESCCODE("inplace_bitwise_or");
			NEXTOP(3+pcoffset);
		case _inplace_bitwise_xor_global_local:
		case _inplace_bitwise_xor_global_shared:
		case _inplace_bitwise_xor_local_shared:
		case _inplace_bitwise_xor_local_local:
			opcount = 1+pcoffset/2;
			DESCCODE("inplace_bitwise_xor");
			NEXTOP(3+pcoffset);
		/*
		自增自减操作
		*/
		case _self_ladd_local:
		case _self_ladd_shared:
			opcount = 1+pcoffset/2;
			DESCCODE("self_add_left");
			NEXTOP(3+pcoffset);
		case _self_radd_local:
		case _self_radd_shared:
			opcount = 1+pcoffset/2;
			DESCCODE("self_add_right");
			NEXTOP(3+pcoffset);
		case _self_lsub_local:
		case _self_lsub_shared:
			opcount = 1+pcoffset/2;
			DESCCODE("self_sub_left");
			NEXTOP(3+pcoffset);
		case _self_rsub_local:
		case _self_rsub_shared:
			opcount = 1+pcoffset/2;
			DESCCODE("self_sub_right");
			NEXTOP(3+pcoffset);
		case _build_list:
			opcount = 2;
			DESCCODE("build_list");
			NEXTOP(5);
		case _append_list_local:
		case _append_list_shared:
			opcount = 2;
			DESCCODE("append_list");
			NEXTOP(5);
		case _extend_list_local:
		case _extend_list_shared:
			opcount = 2;
			DESCCODE("extend_list");
			NEXTOP(5);
		case _build_set:
			opcount = 2;
			DESCCODE("build_set");
			NEXTOP(5);
		case _add_set_local: 
		case _add_set_shared:
			opcount = 2;
			DESCCODE("add_set");
			NEXTOP(5);
		case _build_dict:
			opcount = 2;
			DESCCODE("extend_dict");
			NEXTOP(5);
		case _init_iter_local:
		case _init_iter_shared: 
			opcount = 2;
			DESCCODE("init_iter");
			NEXTOP(5)
		case _iter:
			opcount = 1;
			DESCCODE("iter");
			NEXTOP(3)
		case _init_range_shared_local:
		case _init_range_shared_shared:
		case _init_range_local_local:
		case _init_range_local_shared:
			opcount = 3;
			DESCCODE("init_range");
			NEXTOP(7)
		case _range_step:
			opcount = 2;
			DESCCODE("_range_step");
			NEXTOP(5)
		case _end:
			DESCCODE("end");
			angel_out("\n\n");
			ADDOP(1)
			return ;
		case _asc_ref:
			opcount = 1;
			DESCCODE("_asc_ref");
			NEXTOP(3)
		case _dec_ref:
			opcount = 1;
			DESCCODE("_dec_ref");
			NEXTOP(3)
		case _ret_anyway:
			opcount = 1;
			DESCCODE("final_ret");
			ADDOP(1);
			return ;
		case _init_class:
			opcount = 1;
			DESCCODE("init_class");
			NEXTOP(3);
		default:
			angel_error("无效指令！");
			goto exit;
		}
		index++;
	}
exit:
	printf("\n");
}
void transcode(bytecode bc)
{
	char *pc = bc->code;
	angel_out("test code:\n");
	print_code_info(pc);
	for(int i = 0; i < global_function->len; i++)
	{
		char funcodeinfo[errorsize];
		fun f = global_function->fun_item[i];
		sprintf(funcodeinfo,"function[名字：%s，参数个数：%d，默认参数个数：%d]：\n",
			f->name,f->paracount,f->default_paracount);
		angel_out(funcodeinfo);
		print_code_info(bc->code + *f->base_addr);
	}
	angel_out("\n");
}


void setangelenv()
{
	int len=0,alloc=0;
	char *line;
	FILE *f=fopen(getstrcat(getcurrentdir(),"angel.dir"),"r");
	if(!f)
	{
		alloc=1;
		directory=(char **)calloc(alloc,sizeof(char *));
		angel_error("初始配置失败，看是否丢失angel.dir文件！");
		return ;
	}
	else
	{
		alloc=10;
		directory=(char **)calloc(alloc,sizeof(char *));
	}
	line=read_line(f);
	while(line)
	{
		if(len>=alloc-1)
		{
			alloc*=2;
			directory=(char **)realloc(directory,alloc*sizeof(char *));
		}
		directory[len++]=line;
		line=read_line(f);
	}
}
FILE *getangelfile()
{
	FILE *f;
	char **p;
	option *opt = angel_option;
	char *filename=opt->exec_path,*path;
	if(strcmp(filename + strlen(filename)-6,".angel")!=0)  //表明这不是完整的文件名
	{
		filename = getstrcat(filename,".angel");
	}
	f = fopen(filename,"r");
	if(f) return f;
	f = fopen(getstrcat(getcurrentdir(),filename),"r");
	if(f) return f;
	int i = 0;
	if(opt->env)
	{
		while(opt->env[i])
		{
			path=getstrcat(opt->env[i],filename);
			f = fopen(path,"r");
			if(f) return f;
			i++;
		}
	}
	for(p=directory; *p; p++)
	{
		path=getstrcat(*p,filename);
		f=fopen(path,"r");
		if(f)
			return f;
	}
	char errorinfo[errorsize];
	sprintf(errorinfo,"文件%s不存在",filename);
	angel_error(errorinfo);
	return NULL;
}
void angel_error(char *errorinfo)
{
	printf("Internal Error：");
	printf(errorinfo);
	printf("\n");
}
void angel_out(char *out,FILE *f)  //这里的out是本地编码。
{
	fprintf(f,out);
}
void angel_outc(char c,FILE *f)
{
	fputc(c,f);
}
void printbyte(bytecode bc)
{
	int i;
	for(i=0; i<bc->len; i=i+1)
		printf("%x ",bc->code[i]);
	printf("\n%d\n",bc->len);
}

char *getstrfomat(char *s,int len)
{
	char *p=s,*res=(char *)calloc(len+1,sizeof(char));
	char *q=res;
	char c=*p++;
	while(c!=0)
	{
		switch(c)
		{
		case '\\':
			*q++='\\';
			break ;
		case '\n':
			*q++='\\';
			*q++='n';
			goto nextchar;
		case '\t':
			*q++='\\';
			*q++='t';
			goto nextchar;
		case '\r':
			*q++='\\';
			*q++='r';
			goto nextchar; //.....
		}
		*q++=c;
nextchar:
		c=*p++;
	}
	*q=0;
	free(s);
	return res;
}
int compiler(FILE *f)
{
	token res;
	tokenlen=0;
	init();
	code c=predeal(f);  //是做一些编码测试和引入库的工作
	if(!c)
		return 0;
	dealcode(c);
	
	res=grammartree_entry();
	if(!parse_angel(res))  //表示编译失败
		return 0;
	return 1;
}
void execparse(FILE *f)
{
	init_stack_list();

	if(!compiler(f))
		return ;
	
	//对全聚的资源进行初始化
	
	linkcollection main = alloc_thread();
	runable run = (runable)main->data;
	run->argc = NULL;
	run->func = NULL;
	run->pthread = getcurrentthread();

	exec(main);  //exec可以重新设计返回值（返回环境参数）


	global_check = &global_value_list->item[global_value_list->alloc_size];
	

	/*
	
	while(forehead->next) ; //前台没结束则等待
	while(backhead->next)  //后台还包括未完成线程
		thread_cmd=1;
		
	*/
}
void printbytes(object_bytes b)
{
	angel_out("bytes\"");
	for(int i = 0; i<b->len; i++)
	{
		printf("%02x ",(uchar)b->bytes[i]);
	}
	angel_out("\"");
}
void printstr(object_string s)  //这里是将所有的转移字符都打印出来
{
	int i;
	char *out=tonative(s);
	printf("\'");
	char *print=getstrfomat(out,s->len*2);
	printf(print);
	printf("\'");
	free(print);
}
void printint(object_int i)
{
	printf("%lld",i->val);
}
void printfloat(object_float f)
{
	printf("%f",f->val);
}
void printobject(object o)
{
	printf("object[%s]",o->c->name);
}
void printbool(object o)
{
	if(o == angel_false)
		printf("false");
	else
		printf("true");
}
void printlist(object_list l)
{
	int i;
	if(ISPRINTED(l))
	{
		angel_out("[...]");
		return ;
	}
	printf("[");
	l->extra_flag = IS_PRINTED;
	for(i=0; i<l->len; i++)
	{
		object o=l->item[i];
		
		_print(o);
		
		if(i!=l->len-1)
			printf(", ");
	}
	l->extra_flag = 0;
	printf("]");
}
void printset(object_set s)
{
	int i,start = 1;
	if(ISPRINTED(s))
	{
		angel_out("{...}");
		return ;
	}
	printf("{");
	s->extra_flag = IS_PRINTED;
	for(i=0; i<s->alloc_size; i++)
	{
		object o=s->element[i];
		if(o)
		{
			if(start == 0)
				printf(", ");
			start = 0;
			_print(o);
		}
	}
	s->extra_flag = 0;
	printf("}");
}
void printentry(object_entry entry)
{
	angel_out("entry<");
	_print(entry->key);
	angel_out(",");
	_print(entry->value);
	angel_out(">");
}
void printdict(object_dict d)
{
	int i;
	int start=0;
	if(ISPRINTED(d))
	{
		angel_out("{...}");
		return ;
	}
	printf("{");
	d->extra_flag = IS_PRINTED;
	for(i=0; i<d->alloc_size; i++)
	{
		object_entry entry=d->hashtable[i];
		if(entry)
		{
			if(start)
				angel_out(", ");
			_print(entry->key);
			angel_out(": ");

			_print(entry->value);
			start=1;
		}
	}
	if(d->len == 0) angel_out(":");  //表明这是一个空的字典
	
	d->extra_flag = 0;
	angel_out("}");
}
void printrange(object_range r)
{
	char info[errorsize];
	sprintf(info,"range<%d~%d : %d>",r->begin,r->end,r->step);
	angel_out(info);
}
void printextobj(object o)
{
	object_ext ext = (object_ext)o;
	angel_out(ext->_type->type_name);
}
char *getcommand(char **cmd)
{
	char *c=*cmd,*b;
	if(*c==' ')
		while(*c==' ' && *c) c++;
	b=c;
	while(*c && *c!=' ') c++;
	//前后都是过滤掉空格
	*c=0;
	if(*c==' ')
		while(*c==' ') c++;
	*cmd=c+1;
	return b;
}
void _print(object o)  //这个函数的代码需要做邻接保护
{
	switch(o->type)
	{
	case BOOLEAN :
		printbool(o);
		break ;
	case INT:
		printint((object_int)o);
		break ;
	case FLOAT:
		printfloat((object_float)o);
		break ;
	case STR:
		printstr(GETSTR(o));
		break ;
	case LIST:
		printlist(GETLIST(o));
		break ;
	case OBJECT:
		printobject(o);
		break ;
	case DICT:
		printdict(GETDICT(o));
		break;
	case SET:
		printset(GETSET(o));
		break ;
	case ENTRY:
		printentry(GETENTRY(o));
		break ;
	case FUNP:
		angel_out("[function]");
		break ;
	case NU:
		angel_out("<null>");
		break ;
	case BYTES:
		printbytes(GETBYTES(o));
		break ;
	case REGULAR:
		angel_out("resource<regular>");
		break ;
	case RANGE:
		printrange(GETRANGE(o));
		break ;
	case EXT_TYPE:
		printextobj(o);
	default:
		angel_error("不合法的数据类型");
		break ;
	}
}
void print(object o)
{
	_print(o);
	printf("\n");
}
void exec_shell()
{
	token res;
	option *op = angel_option;
	char *cmd=(char *)malloc(100);


	init_heap();

	init_test_obj();
	reset_heap();


	if(!op->exec_path)
		return ;
	FILE *f = getangelfile();
	if(!f)
		return ;
	if(op->flag >> 4 == 1) //标识此时是编译
		compiler(f);
	else
		execparse(f);
		
	if((op->flag & 0x0f) == 0)  //表示此时不进入checkshell界面
		return ;

	angel_out("checkshell：\n");
	angel_out("查看全局变量或使用-命令：\n");
	angel_out("checker : ");
	gets(cmd);
	while(strcmp(cmd,"-exit")!=0)
	{
		//开始对命令进行解析
		char *param=getcommand(&cmd);
		char *param2;
		uint16_t offset;
		if(strcmp(param,"-exit")==0)
			exit(0) ;
		else if(strcmp(param,"-cls")==0)
			system("cls");
		else if(strcmp(param,"-code")==0)
		{
			angel_out("test code:\n");
			print_code_info(main_byte->code);
		}
		else if(strcmp(param,"-bytec")==0)
		{
			for(int i = 0; i<angel_byte->len; i++)
			{
				printf("%02x ",(uchar)angel_byte->code[i]);
			}
			angel_out("\n");
		}
		else if(strcmp(param,"-fcode")==0)
		{
			param=getcommand(&cmd);
			int16_t offset = getoffset(global_function_map,param);
			if(-1 == offset)
			{
				angel_error("待查函数不存在！");
				goto newbegin;
			}
			fun fp = global_function->fun_item[offset];
			print_code_info(main_byte->code + *fp->base_addr);
		}
		else if(strcmp(param,"-mfcode")==0)
		{
			param = getcommand(&cmd);
			param2 = getcommand(&cmd);
			int16_t offset = getclassoffset(param);
			if(-1 == offset)
			{
				angel_error("待查类不存在！");
				goto newbegin;
			}
			pclass pc = clist->c[offset];
			fun fp = getfun(pc->mem_f,pc->mem_f_map,param2,-1);
			if(-1 == offset)
			{
				angel_error("待查成员函数不存在！");
				goto newbegin;
			}
			print_code_info(main_byte->code + *fp->base_addr);
		}
		else   //这里面可以直接对其进行编译和运行，其中将变量，函数执行数字等要提取出来作为结果输出
		{
			int flag = getoffset(global_value_map,param);
			if(flag == -1)
			{
				char errorinfo[errorsize];
				sprintf(errorinfo,"变量%s未定义！",param);
				angel_error(errorinfo);
				goto newbegin;
			}
			object o=global_check[flag];
			if(!o)
				goto newbegin;
			print(o);
		}
		newbegin:
		angel_out("checker : ");
		gets(cmd);
	}
}
void shell(int argc,char **argv)
{
	critical_init();


	setangelenv();
	if(argc == 1)  //表示此时是双击打开模式或者
	{
		angel_out("angel VM 5.0 适用32位以上系统\n");
		angel_out("作者：邵贤鹏（john connor）\n");
		angel_out("命令格式为：\n");
		angel_out("angel [opts] [angel文件(不需要后缀)]\n");
		angel_out("选项opts包括如下：\n");
		angel_out("-sh 表示执行后进入check交互界面\n");
		angel_out("-env 指定运行环境，系统优先查找指定的环境，在查找配置的环境\n");
		angel_out("-v/-version 查看当前版本\n");
		exit(1);
	}
	angel_option = parse_option(argc,argv);

	exec_shell();
}