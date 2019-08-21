#include <stdlib.h>
#include <time.h>
#include "parse.h"
#include "lib.h"
#include "hash.h"
#include "execute.h"
#include "compilerutil.h"
#include "util.h"
#include "shell.h"
#include "amem.h"

#include "../Extension/initext.h"

//这个模块不能出现具体的类型



//这种动态添加常量会导致

object_set global_func_map,array_func_map,bytes_func_map,
	string_func_map,set_func_map,dict_func_map,regular_func_map;
extern linkcollection global_scope,global_current_scope;

/*

系统库函数
*/
object systype(object o)
{
	return (object)CONST(gettypedesc(o));
}
object sysset(object init)
{
	object_set ret = initset();
	if(ISNOTYPE(init))//表示此时是默认参数
		return (object)ret;
	else if(ISLIST(init))
	{
		translisttoset(ret, GETLIST(init));
	}
	else if(ISRANGE(init))
	{
		transrangetoset(ret,GETRANGE(init));
	}
	else
	{
		angel_error("function[list] argument 1 must be integer");
		return GETNULL;
	}
	return (object)ret;
}
object syslist(object o,object init)
{
	ARG_CHECK(o,INT,"list",1)
	if(ISNOTYPE(init))//表示此时是默认参数
		init = INTCONST(0);
 	object_list res = initarray(GETINT(o));
	for(int i = 0; i<res->alloc_size; i++)
	{
		addlist(res,init);
	}
	return (object)res;
}
object sysprint(object o)
{
	char *buf;
	if(o->type!=STR)
	{
		_print(o);
	}
	else //格式化输出字符串
	{
		char *print = tonative(GETSTR(o)); 
		angel_out(print);
		free(print);
	}
	return GETNULL;
}
object sysprintl(object o)
{
	//涉及到io操作的不要用锁
	//critical_enter();
	sysprint(o);
	angel_out("\n");
	//critical_leave();
	return GETNULL;
}
object sysscan()
{
	char s[read_base_size];
	scanf("%s",s);
	return (object)initstring(s);
}
object sysid(object o)
{
	return (object)initinteger((long)o);
}
object sysaddr(object valuename, object funname, object o)
{
	linkcollection link;
	ARG_CHECK(valuename,STR,"addr",1);
	char *value = tonative(GETSTR(valuename));
	if(!ISNOTYPE(funname))
	{
		char *funn = tonative(GETSTR(funname));
		if(!ISSTR(funname))
		{
			angel_error("addr函数的第二个参数必须是字符串类型！");
			return GETNULL;
		}
		fun f;
		if(!ISNOTYPE(o))
			f = getobjmemfun(o,funn);
		else
			f = getglobalfun(funn);
		link = f->local_scope;
		free(funn);
	}
	else
	{
		link = global_scope;
	}
	object_list ret = initarray();
	linkcollection head = link;
	for(linkcollection p = link->next; p != head; p = p->next)
	{
		object_set map = (object_set)p->data;
		if(!map) continue ;
		int offset;
		if(-1 != (offset = getoffset(map,value)))
			_addlist(ret,(object)initinteger(offset));
	}
	free(value);
	return (object)ret;
}
object sysheap()
{
	object_dict dict = initdictionary();
	_adddict(dict,(object)CONST("data"),(object)initfloat((double)get_data_heap()->free_size / get_data_heap()->total_size));
	_adddict(dict,(object)CONST("object"),(object)initfloat((double)get_object_heap()->free_size / get_object_heap()->total_size));
	return (object)dict;
}
object sysref(object o)
{
	return (object)initinteger(o->refcount);
}
object syshash(object o)
{
	return (object)initinteger(globalhash(o));
}



object sysregular(object s)
{
	ARG_CHECK(s,STR,"regular",1);
	object res = checkpatternparam((object)s);
	if(!res)
	{
		return GETNULL;
	}
return (object)res;
}
//系统函数不打算用类的形式提供，所以需要考虑系统资源表示数目问题
angel_buildin_func angel_build_in_def[] = 
{
	{"type",systype,1,0,0},
	{"list",syslist,2,1,0},
	{"set",sysset,1,1,0},
	{"print",sysprint,1,0,0},
	{"printl",sysprintl,1,0,0},
	{"scan",sysscan,0,0,0},
	{"id",sysid,1,0,0},
	{"addr",sysaddr,3,2,0},
	{"heap",sysheap,0,0,0},
	{"ref",sysref,1,0,0},
	{"hash",syshash,1,0,0},
	{"regular",sysregular,1,0,0},

	/*
	文件操作
	*/
	{"fopen",sysfopen,2,0,0},
	{"fread",sysfread,1,0,0},
	{"fgets",sysfgets,1,0,0},
	{"fwrite",sysfwrite,1,0,0},
	{"fclose",sysfclose,1,0,0},
	{"finfo",sysfinfo,1,0,0},
	{"fls",sysfls,1,0,0},


	/*
	格式化解析
	*/
	{"parse",sysparse,2,0,0},


	/*
	时间类
	*/
	{"clock",sysclock,1,1,0},
	{"sleep",syssleep,2,1,0},


	/*
	线程相关
	*/
	{"startthread",sysstartthread,4,2,0},



	/*
	网络相关
	*/
	//{"netname",sysnetaddr,1,0,0},
	{"netaddr",sysnetaddr,1,0,0},
	{"socket",syssocket,1,0,0},
	{"bind",sysbind,2,0,0},
	{"listen",syslisten,2,1,0},
	{"accept",sysaccept,2,0,0},
	{"connect",sysconnect,2,0,0},
	{"recv",sysrecv,1,0,0},
	{"send",syssend,2,0,0},
	{"sclose",syssclose,1,0,0},



	{NULL,NULL,0,0}
};






/*
成员库函数
*/



angel_buildin_func angel_array_func_def[] = 
{
	{"size",syssize_list,1,0,0},
	{"add",sysadd_list,2,0,0},
	{"extend",sysextend_list,2,0,0},
	{"pop",syspop_list,1,0,0},
	{NULL,NULL,0,0}
};
angel_buildin_func angel_set_func_def[] = 
{
	{"size",syssize_set,1,0,0},
	{"add",sysadd_set,2,0,0},
	{"isexist",sysisexist_set,2,0,0},
	{"remove",sysremove_set,2,0,0},
	{NULL,NULL,0,0}
};
angel_buildin_func angel_dict_func_def[] = 
{
	{"size",syssize_dict,1,0,0},
	{"iskey",sysiskey_dict,2,0,0},
	{"keys",syskeys_dict,1,0,0},
	{NULL,NULL,0,0}
};
angel_buildin_func angel_string_func_def[] = 
{
	{"size",syssize_string,1,0,0},
	{"join",sysjoin_string,2,0,0},
	{"upper",sysupper_string,1,0,0},
	{"lower",syslower_string,1,0,0},
	{"find",sysfind_string,3,1,0},
	{"findall",sysfindall_string,3,1,0},
	{"match",sysmatch_string,3,1,0},
	{NULL,NULL,0,0}
};
angel_buildin_func angel_bytes_func_def[] = 
{
	{"size",syssize_bytes,1,0,0},
	{NULL,NULL,0,0}
};
angel_buildin_func angel_regular_func_def[] = 
{
	{"code",syscode_regular,1,0,0},
	{NULL,NULL,0,0}
};
void build_sys_func_map(object_set lib_function_map,angel_buildin_func lib_fun[])
{
	int i=0;
	angel_buildin_func func = lib_fun[i];
	while(func.name)
	{
		if(!addmap(lib_function_map,func.name,i++))
		{
			angel_error("编译出错：系统内置函数定义重复");
			return ;
		}
		func = lib_fun[i];
	}
}
void init_lib_func_map()
{
	initexttype();

	global_func_map = init_perpetual_set();
	bytes_func_map = init_perpetual_set();
	string_func_map = init_perpetual_set();
	array_func_map = init_perpetual_set();
	set_func_map = init_perpetual_set();
	dict_func_map = init_perpetual_set();
	regular_func_map = init_perpetual_set();

	build_sys_func_map(global_func_map,angel_build_in_def);
	build_sys_func_map(array_func_map,angel_array_func_def);
	build_sys_func_map(set_func_map,angel_set_func_def);
	build_sys_func_map(dict_func_map,angel_dict_func_def);
	build_sys_func_map(string_func_map,angel_string_func_def);
	build_sys_func_map(bytes_func_map,angel_bytes_func_def);
	build_sys_func_map(regular_func_map,angel_regular_func_def);
}
int issysfunbyname(char *funname)
{
	int i=0;
	
	return -1;
}
int _issysfun(fun f,object_set lib_function_map)
{
	int offset = getoffset(lib_function_map,f->name);
	if(offset == -1)
		return 0;
	angel_buildin_func fun = angel_build_in_def[offset];
	if(f->paracount - f->default_paracount <= fun.argcount && fun.argcount <= f->paracount)
		return 1;
	return 0;
}
int issysfun(fun f)
{
	return _issysfun(f,global_func_map);
}
int issyscall(char *funname,int count,angel_buildin_func libfun[],object_set lib_function_map)
{
	int offset = getoffset(lib_function_map,funname);
	angel_buildin_func fun = libfun[offset]; 
	if(count == -1)  //表示此时只是向获取同名库函数
		return offset;
	if(offset == -1)
		return -1;
	if(isparamvalid(fun.argcount,fun.argdefaultcount,count))
		return offset;
	return -1;
}
int isglobalsyscall(char *funname,int count)
{
	return issyscall(funname,count,angel_build_in_def,global_func_map);
}
angel_buildin_func *getglobalsyscall(char *funname,int count)
{
	int index = isglobalsyscall(funname,count);
	if(index == -1)
		return NULL;
	return &angel_build_in_def[index];
}
angel_buildin_func *getsysmembercall(object o,char *funname,int count)
{
	angel_buildin_func *witch;
	object_set map;
	switch(o->type)
	{
	case BYTES:
		witch = angel_bytes_func_def;
		map = bytes_func_map;
		break ; 
	case STR:
		witch = angel_string_func_def;
		map = string_func_map;
		break ;
	case LIST:
		witch = angel_array_func_def;
		map = array_func_map;
		break ;
	case SET:
		witch = angel_set_func_def;
		map = set_func_map;
		break ;
	case DICT:
		witch = angel_dict_func_def;
		map = dict_func_map;
		break ;
	case REGULAR:
		witch = angel_regular_func_def;
		map = regular_func_map;
		break ;
	default:
		angel_error("目前调试阶段还不支持一般类型的成员函数！");
		return NULL;
	}
	int index = issyscall(funname,count,witch,map);
	if(index == -1)
		return NULL;
	return &witch[index];
}


int checkrangeparam(object range,int size,int *res)
{
	int begin,end;
	if(ISNOTYPE(range))
	{
		begin = 0;
		end = size;
	}
	else if(ISRANGE(range))
	{
		begin = GETRANGE(range)->begin > 0 ? GETRANGE(range)->begin : 0;
		end = GETRANGE(range)->end < size ? GETRANGE(range)->end : size;
	}
	else
	{
		return 0;
	}
	res[0] = begin;
	res[1] = end;
	return 1;
}