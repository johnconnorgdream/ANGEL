#include <memory.h>
#include <stdlib.h>
#include <string.h>
#include "parse.h"
#include "data.h"
#include "runtime.h"
#include "compilerutil.h"
#include "lib.h"
#include "hash.h"
#include "shell.h"
#include "util.h"
#include "amem.h"
#include "aenv.h"


//数值也是用记号来表示
//getvaluebytoken得到的是变量的值而非引用，并且他返回的类型为整形值。
//因为这里多次调用execord函数。
//除了在赋值行为上其他一切都是返回值而非对象引用
//获得数组和字符串成员变量的操作已经定下来了。
//可以将本系统大致分为可输入和输出模块，可输入模块是需要引用的（比如变量,其中成员变量、普通变量、列表的的子项都有类似的效应），输出模块也是指运算的中间结果，比如函数返回值、四则运算的中间值,
//函数机制和面向对象机制需要注意的问题是背景环境，简单的说就是参数赋值时赋值参数和被赋值参数所处的环境不同。
//私有成员变量机制从this.^x开始
//数据对象只有在使用的时候才放到obj_list,并记录偏移量

/*
这里面的obj_list，global_value，funlist，switch_table,name_pool都是支持2个字节寻址如果必要的话未来会考虑用拓展的四字节操作数，前提条件是列表的长度没有超过65535
其中funlist需要提前将所有重载的函数放在一起，函数映射表中只包含第一个函数的索引
还有要单独设一个迭代器对象，用于记录列表，集合，或字典迭代的当前状态

*/

funlist temp_function;  //这个表示临时定义的函数lambda
object_list global_value_list,obj_list,dynamic_name;  //创建全局变量的列表，这里考虑将全局变量用栈的形式管理   ,建立对象的列表，对象的堆首址与偏移量的关系
codemap brand; //brand是为循环中复杂的跳转程序服务的
funlist global_function;  //函数指令产生的预处理块
classlist clist;
object_set global_value_map,global_function_map,class_map,name_pool;  //这个参数只在编译环节有用，执行时则没用，调试以后再说
; //名字常量主要时用来存储名字的,每个映射表都有一个独立的对象，但字符串是共享的
bytecode main_byte,angel_byte; //这里是定义了一个存储字节码的内存空间,包括分支语句，分支语句要另外存储
tstack ctrl_state;  //控制语句的执行状态。他表示你当前是在哪个分支语句下的
linkcollection global_scope,global_current_scope;
temp_alloc_info angel_temp;
pclass global_class_env;
extern option *angel_option;


switchlist _global_sw_list;

token t[maxtoken];
int islib;
char **directory;
int tokenlen=0;
keyword keywordlib[]={{"if",IF},{"while",LOOP},{"continue",CONTINUE},{"break",BREAK},{"for",FOR},
{"null",NU},{"true",TRUE},{"false",FALSE},{"switch",SWITCH},{"case",CASE},{"default",DEFAULT},
{"==",EQU},{"+",ADD},{"-",SUB},{"*",MUL},{"/",DIV},{"(",LBRACKETL},{")",LBRACKETR},
{"[",MBRACKETL},{"]",MBRACKETR},{"{",BBRACKETL},{"}",BBRACKETR},{";",END},{"<",SMALL},{">",BIG},
{"=",ASSIG},{".",DOT},{"let",VALDEC},{"class",CLASS},{"!=",NOEQU},{"!",NO},
{"else",ELSE},{",",DOUHAO},{"ret",RET},{":",COLON},{"~",WAVEL},{"++",SELFADD},{"--",SELFSUB},
{"?",QUESTION},{"of",OF},{"dec",DECLARE},{"%",MOD},{"<=",SMALLEQUAL},{">=",BIGEQUAL},{"&&",AND},
{"||",OR},{"&",BITAND},{"|",BITOR},{"^",BITXOR},{"&=",INPLACEBITAND},{"|=",INPLACEBITOR},
{"^=",INPLACEBITXOR},{"+=",INPLACEADD},{"-=",INPLACESUB},{"*=",INPLACEMULT},{"/=",INPLACEDIV},
{"%=",INPLACEMOD},{"in",IN},{">>",RSHIFT},{"<<",LSHIFT},{"<<=",INPLACELSHIFT},{">>=",INPLACERSHIFT}
,{"$",MAKEFUN},{NULL,0}};

object_set keytable;

#define VAL_UNDIFINED(error,name) if(!error) \
					{ \
						char errorinfo[100]; \
						sprintf(errorinfo,"变量%s未定义！",name); \
						angel_error(errorinfo); \
						return 0; \
					} 
#define FREEOTSTACK  free(ts); free(os);
#define ISSENTENCEEND(pos) (t[*pos]->id == END || t[*pos]->id == DOUHAO || t[*pos]->id == endid || (!ISKEYWORD(t[*pos]->id)))
#define NOTEND(offset) (offset < tokenlen)
/*

通用接口和脚本语言的预处理

*/



#define ISASSIG(id) ((id<=INPLACEMOD && id>=ASSIG)) 
int findpivot(_switch *a,int low,int high)
{
	_switch pivotval=a[low];
	while(low<high)
	{
		while(low<high && a[high]->hash>pivotval->hash) 
			high--;
		if(low<high) //这是快速排序需要注意的第二个地方。
		{
			if(a[high]->hash==pivotval->hash)
				return -1;
			a[low++] = a[high];
		}
		while(low<high && a[low]->hash<pivotval->hash) 
			low++;
		if(low<high)
		{
			if(a[low]->hash==pivotval->hash)
				return -1;
			a[high--] = a[low];
		}
	}
	a[low]=pivotval;
	return low;
}
int quicksort(_switch *a,int low,int high)//快速排序的核心是每次确定一个位置
{
	int pivot;
	if(low<high)
	{
		pivot=findpivot(a,low,high);//该函数不仅返回了中轴值的位置，还确定了该中轴值应放的值
		if(pivot==-1)
			return 0;
		if(!quicksort(a,low,pivot-1))
			return 0;
		if(!quicksort(a,pivot+1,high))
			return 0;//一定要记住这里是pivot+1而非pivot。
	}
	return 1;
}
fun initfuncontrol(char *funname,int type=0)
{
	fun f=(fun)malloc(sizeof(funnode));
	memset(f,0,sizeof(funnode));
	f->type=type;
	f->local_v_map=init_perpetual_set();
	f->class_context = NULL;
	f->name=funname;
	f->local_scope = f->current_scope = initlink();
	return f;
}
char * multitowide(char *value)
{
	return value;  //有待补充
}









int checkparamfomat(int i);
token gettop(tstack s)
{
	if(!stackempty(s))
		return s->t[s->top];
	else
		return NULL;
}
int stackempty(tstack s)
{
	if(s->top==-1)
		return 1;
	else
		return 0;
}
void push(tstack s,token c)
{
	if(s->top==maxsize)
	{
		printf("栈满了！\n");
		return;
	}
	s->t[++s->top]=c;
}
token pop(tstack s)
{
	if(stackempty(s))
	{
		printf("表达式不符合规则或执行出错！\n");
		return NULL;
	}
	else
	{
		token temp=s->t[s->top];
		s->top--;
		return temp;
	}
}
token copytoken(token p)
{
	token t=(token)malloc(sizeof(tokennode));
	memcpy(t,p,sizeof(tokennode));
	t->first=t->second=NULL;
	return t;
}
void addtoken(token t,token s)
{
	token p;
	if(!s)
		return ;
	if(!t->first)
	{
		t->first=s;
		return ;
	}
	for(p=t->first;p->second;p=p->second);
	//s->second=NULL;
	p->second=s;
}
token maketokenEX(int id,void *attr,token first,token second)
{
	token t=(token)calloc(1,sizeof(tokennode));
	t->id=id;
	t->attr=attr;
	t->first = first;
	t->second = second;
	return t;
}
token maketoken(int id,void *attr)
{
	return maketokenEX(id,attr,NULL,NULL);
}
void deletetoken(token t)   //利用后序遍历将节点均删除掉
{ 
	if(t)
	{
		deletetoken(t->first);
		deletetoken(t->second);
		free(t);
	}
}
object_set initkeylib(keyword lib[])
{
	int i = 0;
	object_set res = initset();
	keyword p = lib[i++];
	while(p.keyword)
	{
		if(!addmap(res,p.keyword,p.id))
		{
			angel_error("关键词定义重复！");
			return NULL;
		}
		p = lib[i++];
	}
	return res;
}
int iswordinlib(char *word,int *id)
{
	int flag = getoffset(keytable,word);
	if(flag == -1)
		return 0;
	*id = flag;
	return 1;
}
int priority(token t)  //以后通过扩展这个函数的功能来增加跟多的运算，并确定优先级
{
	int operation=t->id;
	switch(operation)
	{
	case LBRACKETL:
	case LBRACKETR:
		return 2;
	case ASSIG:
	case INPLACEADD:
	case INPLACESUB:
	case INPLACEMULT:
	case INPLACEDIV:
	case INPLACEBITAND:
	case INPLACEBITOR:
	case INPLACEBITXOR:
	case INPLACEMOD:
		return 5;
	case IN:
		return 5;
	case QUESTION:
		return 6;
	case COLON:
		return 7;
	case OR:
		return 8;
	case AND:
		return 9;
	case BITOR:
		return 10;
	case BITAND:
		return 11;
	case EQU:
	case NOEQU:
	case BIG:
	case BIGEQUAL:
	case SMALL:
	case SMALLEQUAL:
		return 12;
	case WAVEL:
		return 13;
	case LSHIFT:
	case RSHIFT:
		return 14;
	case ADD:
	case SUB:
		return 15;
	case MUL:
	case MOD:
	case DIV:
		return 16;
	case NO:
		return 17;
	case SELFADD:
		return 19;
	case SELFSUB:
		return 19;
	case MAKEFUN:
		return 20;
	case INDEX:
		return 21;
	case DOT:
		return 21;
	default:
		return 0;
	}
}
void addcode(code c,char *s)   //尾插法
{
	code p=(code)malloc(sizeof(codenode));
	p->code=s;
	for(;c->next;c=c->next);  //注意这里
	p->next=NULL;
	c->next=p;
}
int getfilelen(FILE *f)
{
	int len;
	fseek(f,0,SEEK_END);
	len=ftell(f);
	fseek(f,0,0);
	return len;
}
char* filterspace(char *s)  //被过滤的字串的第一个字符必须是空格。即碰到空格需要时才用该过滤。
{
	while(*s==' ' || *s=='\t' || *s=='\n')
		s++;
	return s;
}
int issensitive(char c)  //对于那些不需要空格来分割的单词
{
	char boundary[]={'(',')','[',']','{','}','=','+','-','*','/','<','>',';','&','%','|','!',',','.',':','?','^','~','@','$'};
	int i;
	for(i=0; i<26; i++)
		if(boundary[i]==c)
			return 1;
	return 0;
}
int iscalcsignal(char c)
{
	char calc[]={'+','-','*','/','>','<','%','=','&','|','!',':','{','[','(','^'};
	int i;
	for(i=0; i<15; i++)
		if(calc[i]==c)
			return 1;
	return 0;
}
inline int isnum(char c)
{
	if(c<='9' && c>='0')
		return 1;
	else
		return 0;
}
char *getstrvalue(char **s,char endflag)   //这里是字符串复制的方法
{
	char *res;  
	char *p = *s+1;
	int count=0;
	while(*p)
	{
		if(*p==endflag)
			break ;
		else
		    count++;
		p++;
	}
	res = (char *)calloc(count+1,sizeof(char));
	p = res;
	(*s)++ ; //过滤第一个'号
	while(**s)
	{
		if(**s=='\\')
		{
			char c=*(*s+1);
			switch(c)
			{
			case 't':
				*p++='\t';
				break ;
			case 'n':
				*p++='\n';
				break ;
			case 'r':
				*p++='\r';
				break ;
			case '\'':
				*p++='\'';
				break ;
			case '\\':
				*p++='\\';
				break ;
			default:
				goto common;
			}
			(*s)+=2;
		}
		else if(**s==endflag)
			break ;
		else
common:
		     *p++=*(*s)++;
	}
	*p=0;
	if(**s)
		(*s)++; //过滤掉结束标志
	return res;
}
char *getnumber(char **s)
{
	char *res;
	int count=0,flag = 1;  //表示是整数
	while(**s)
	{
		if(**s == '.' && flag == 1) {
			flag = 2; //表示是浮点数
			count++;
		}
		else if(isnum(**s)){
			count++;
		}
		else
			break ;
		(*s)++;
	}
	res = (char *)calloc(count+2,sizeof(char));
	*res++ = flag;
	memcpy(res,*s-count,count);
	return res-1;
}
inline int isquota(char s)
{
	if(s == '\'' || s == '"')
		return 1;
	return 0;
}
char *getword(char **s)
{
#define SCAN {(*s)++; wordlen++;}
#define BUILDSTR res = (char *)calloc(wordlen+1,sizeof(char));	memcpy(res,*s-wordlen,wordlen);
#define GETTOKEN while(**s!=' ' && **s!='\n' && **s!='\t' && !issensitive(**s) && **s) SCAN
	char *res;
	int wordlen=0;
	*s=filterspace(*s);
	if(!*s)
		return NULL;
	if(issensitive(**s))   //若检测的字符本身就是敏感字符则直接输出。
	{
		SCAN;
		char test = *(*s - 1);
		if(**s == '=')
		{
			if(test=='=' || test=='!' || test=='<' || test=='>'|| test=='&' || test=='|' || test=='^' 
				|| test=='>' || test=='+' || test=='-' || test=='*'|| test=='/' || test=='%') //在这里应添加大于等于和小于等于
				SCAN
		}
		else if(**s == test)
		{
			if(test == '&' || test == '|'|| test == '+' || test == '-')
				SCAN
			else if(test == '<' || test == '>')
			{
				SCAN
				if(**s == '=')  //表示>>=和<<=
					SCAN
			}
		}
		BUILDSTR
		return res;
	}
	else if(**s=='\'' || **s=='"')  //若为字符串变量的的第一个引号
		return getstrvalue(s,**s);
	else if(**s == 'b' && isquota(*((*s)+1)))
		return getstrvalue(s,**s);  //未来待定
	else if(isnum(**s))
	{
		char test = *(*s + 1);
		return getnumber(s);
	}
	//如果什么敏感词汇都没有则直接输出普通标识符即可。
	GETTOKEN
	BUILDSTR
	return res;
}


char *checkword(char *s)
{
	return getword(&s);
}
char *getimport(char **s)
{
	char *res=(char *)malloc(100),*p;
	p=res;
	*s=filterspace(*s);
	while(**s && **s!='\n' && **s!='/t' && **s!=' ')
		*p++=*(*s)++;
	*p=0;
	return res;
}
int predeal_s(FILE *f,code c)  //用%import来引入源文件
{
	int filelen=getfilelen(f);
	char *buffer=(char *)malloc(filelen),*p,ch;
	p=buffer;
	ch=fgetc(f);
	if((unsigned char)ch==0xef)
	{
		fseek(f,3,0);
		ch = fgetc(f);
	}
	//先处理第一行
	while(ch!=EOF)
	{
		if(ch=='\'')
		{
			*p++=ch;
			ch=fgetc(f);
			while(ch != '\'' && ch != EOF){
				*p++ = ch;
				ch = fgetc(f);
			}
			if(ch == '\'')
			{
				*p++ = ch;
				ch = fgetc(f);
			}
		}
		else if(ch=='/')  //表示注释
		{
			char temp=ch;
			ch=fgetc(f);
			if(ch=='/')
			{
				while(ch!='\n' && ch!=EOF) ch=fgetc(f);
			}
			else if(ch=='*')
			{
				ch=fgetc(f);
				while(ch!=EOF)
				{
					if(ch=='\'')
					{
						ch = fgetc(f);
						while(ch != '\'' && ch != EOF)
						{
							ch = fgetc(f);
						}
						if(ch == '\'')
						{
							ch = fgetc(f);
						}
					}
					else if(ch == '*' && fgetc(f) == '/')
					{
						ch=fgetc(f);
						break;
					}
					else
						ch=fgetc(f);
				}
			}
			else   //表示并不是注释
			{
				*p++=temp;
				*p++=ch;
				ch=fgetc(f);
			}
		}
		else
		{
			*p++=ch;
			ch=fgetc(f);
		}
	}
	*p=0;
	p=buffer;
	while(*p)
	{
		if(strcmp(checkword(p),"lib")==0)
		{
			FILE *f;
			getword(&p);
			angel_option->exec_path = getimport(&p);
			f=getangelfile();
			if(!f)
				return 0;
			predeal_s(f,c);
			fclose(f);
		}
		else
			break;
	}
	buffer=p;
	addcode(c,buffer);
	return 1;
}
code predeal(FILE *f)
{
	code c=(code)malloc(sizeof(code)); //建立一个头节点
	memset(c,0,sizeof(codenode));
	if(!predeal_s(f,c))
		return NULL;
	return c;
}
int gettoken(char *code)
{
	int id;
	char *p=code,*word;
	if(!*p)
		return 0;
	word=getword(&p);
	while(*word || *p)
	{
		token tk = (token)calloc(1,sizeof(tokennode));
		if(*(p-1)=='\'' || *(p-1)=='\"')
		{
			tk->id=STR;
			tk->attr=getconstbystr(word);
			t[tokenlen++]=tk;
		} 
		else if(iswordinlib(word,&id))  //觉得很多
		{
			tk->id = id;
			t[tokenlen++]=tk;
			if(tk->id == WAVEL && t[tokenlen-2]->id == MUL)
			{
				t[tokenlen - 2]->id = MIN;
			}
			else if(t[tokenlen-2]->id == WAVEL && tk->id == MUL)
			{
				tk->id = MAX;
			}
			else if(tk->id == RET || tk->id == SELFADD || tk->id == SELFSUB)
			{
				char *ts = p;
				while(*ts == '\t' || *ts == '\n' || *ts == '\r' || *ts == ' ')
				{
					if(*ts == '\n')  //表明ret后面有换行符
					{
						t[tokenlen++] = maketoken(END,0);
						break ;
					}
					ts++;
				}
			}
			free(word);
		}
		else
		{
			if(isnum(*(word+1)) && (*word == 1 || *word == 2))   //判断是否是数字类型
			{
				if(*word == 1)  //整数
				{
					tk->id = INT;//该ID表示整形数值
					tk->attr = getconstbyint(toint(word+1));
				}
				else
				{
					tk->id = FLOAT;
					tk->attr = getconstbyfloat(tofloat(word+1));
				}
			}
			else
			{
				tk->id=NAME; //这个表示变量和其他声明关键字
				tk->attr=word;
			}
			t[tokenlen++]=tk;
		}
		next:
		word=getword(&p);
	}
	return 1;
}
int dealcode(code c)
{
	code p;
	for (p=c->next;p->next;p=p->next)
	{
		t[tokenlen++] = maketoken(LIB,0);
		if(!gettoken(p->code))
			return 0;
	}
	t[tokenlen++] = maketoken(MAIN,0);
	gettoken(p->code);  //最后一个主程序
	return 1;
}
void copytree(token src,token *des)  //完整的赋值表达式树
{
	if(src)
	{
		*des=(token)malloc(sizeof(tokennode));   //因为传入的des肯定指向空
		memcpy(*des,src,sizeof(tokennode));   //再将左右子树置空
		copytree(src->first,&(*des)->first);
		copytree(src->second,&(*des)->second);
	}
}
void updatetree(token src,token des) //在调用次函数前必须确定两棵树的结构要完全相同。
{
	if(src)
	{
		des->id=src->id;
		des->attr=src->attr;
		updatetree(src->first,des->first);
		updatetree(src->second,des->second);
	}
}



/*
语法树的创建相关函数
*/
#define ISCALC(id) (id >= ADD && id <= OR)
object_dict getdict(int *pos);
int isnamekeyword(char *name)
{
	int id;
	return iswordinlib(name,&id);
}
int addclass(char *classname)  
{
	if(!addmap(class_map,classname,clist->len))
	{
		angel_error("类名不允许重复！");
		return 0;
	}
	pclass p=initclass(classname);
	clist->c[clist->len++]=p;
	return 1;
}
void addlocalvalue(fun f,char *valuename)
{
	uint16_t offset = f->localcount++;
	/*if(!addmap(f->local_v_map,valuename,offset))
	{
		char errorinfo[errorsize];
		sprintf(errorinfo,"函数%s变量%s重复定义",f->name,valuename);
		angel_error(errorinfo);
		return -1;
	}*/
	addmap(f->local_v_map,valuename,offset);
}
void addglobalvalue(char *name)  
{
	uint16_t offset=global_value_list->len++;
	/*if(!addmap(global_value_map,name,offset))
	{
		char errorinfo[errorsize];
		sprintf(errorinfo,"变量%s重复定义",name);
		angel_error(errorinfo);
		return -1;
	}*/
	addmap(global_value_map,name,offset);
}
void addvalue(char *name,fun f)
{
	if(!f)
		return addglobalvalue(name);
	else
		return addlocalvalue(f,name);
}
token getsentencetree(int *pos,fun f=NULL,int endid = END);
token gettree(int *pos,int *error,int endid = END,fun f=NULL);

int detectbracket(int pos,uint16_t bracketid)  //返回下一个位置
{
	tstack s = (tstack)malloc(sizeof(tstacknode));
	int i = pos;
	s->top = -1;
	push(s,t[pos-1]);
    while(1)                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                       	while(!stackempty(s))   //进行扩号匹配
	{
		if(t[i]->id == bracketid)
		{
			push(s,t[i]);
		}
		else if(t[i]->id == bracketid+1)
		{
			pop(s);
		}
		if(i == tokenlen-1)
			i = i;
		i++;
		if(stackempty(s))
		{
			return i;
		}
		if(i == tokenlen)
		{
			free(s);
			return -1;
		}
	}
	return -1;
}
int dealfunbracket(int pos)
{
	int i = detectbracket(pos,LBRACKETL);
	if(i == -1)
		return i;
	t[i - 1]->id = END;
	return i;
}

int pregetdict(int pos)
{
	int i=pos,isquestion = 0;
	int count = 1;
    while(i < tokenlen) 
	{
		token test = t[i];
		if(test->id==COLON)
		{
			if(isquestion == 1)
				isquestion = 0;
			else
			{
				return 1;
			}
		}
		else if(test->id == QUESTION)
			isquestion = 1;
		else if(test->id == BBRACKETL)
			count++;
		else if(test->id == BBRACKETR)
		{
			count--;
			if(count == 0)
				return 0;
		}
		i++;
	}
	if(i >= tokenlen)
	{
		angel_error("少了一个}!");
		return -1;
	}
	return 0;
}

#define ISCONST(_id) (_id<=STR && _id>=NU)
#define ISCONSTELE(_id,end) (ISCONST(_id)) && (t[(*pos)+1]->id == DOUHAO || t[(*pos)+1]->id == end)

int addconstelementtolist(token base,int *pos) //这里运行结果的pos指向逗号和]结束符
{
	token ret;
	int count = 0;
	int begin = *pos;
	token test;
	while(1)
	{
		if(ISCONSTELE(t[*pos]->id,MBRACKETR))
		{
			count++;
		}
		else
		{
			(*pos) --;
			goto addtoken;
		}
		test = t[(*pos)+1];
		if(test->id!=DOUHAO && test->id!=MBRACKETR)
		{
			return 1;
		}
		else if(test->id==DOUHAO)
			(*pos) += 2;  //过滤掉逗号
		else
		{
			(*pos)++;
			break ;
		}
	}
addtoken:
	object_list head = init_perpetual_list();
	ret = maketokenEX(ORD_SENTEN,NULL,maketoken(LIST,head),NULL);
	for(int i = 0; i < count; i++)
	{
		object item = (object)t[i*2+begin]->attr;
		addlist(head,item);
	}
	base->extra += count;
	addtoken(base,ret);
	return -1;
}
token getindex(int *pos,fun f = NULL)   //获得数组索引表达式树
{
	token result;
	(*pos)++;
	result=getsentencetree(pos,f,MBRACKETR);          //直接索引
	if(result->first->id == COLON)
	{
		result->first->id = RANGE_STEP;
	}
	t[*pos]->id=SUBITEM;
	(*pos)++;
	return result;  //支持切片操作
}
token getarraytoken(int *pos,fun f)   //这个函数将数组定义的]符号给屏蔽掉,这里列表中引用的对象可以不用放在对象池中，因为是受列表引用，
	//不需要对其偏移量进行访问，另外需要将基本数据对象的值的引用计数加2因为这些基本数据是不能够被释放的
{
	int flag=-1;
	token ret = maketoken(LIST,NULL);
	int hasexp=0;
	token test;
	(*pos)++;

	if(t[*pos]->id == END)
	{
		ret->extra = 8;
		//ret->attr = initarray();  //空集合
		goto parsearrayend;
	}
	while(1)
	{
		//一次解析之后出现两种情况：要么在逗号上，要么在最后一个结尾符上
		if(ISCONSTELE(t[*pos]->id,MBRACKETR))
		{
			flag = addconstelementtolist(ret,pos); //这里运行结果的pos指向逗号和]结束符
		}
		else
		{
			addtoken(ret,getsentencetree(pos,f,MBRACKETR));
			hasexp = 1;
			if(t[(*pos)]->id == MBRACKETR)  //表示已经将]结束符给过滤了
				break ;
			ret->extra ++;
		}
		if(t[*pos]->id==DOUHAO)
			(*pos) ++;  //过滤掉逗号
	}
	if(flag!=-1) //说明出错
	{
error:
		angel_error("数组定义缺一个']'\n");
		return NULL;
	}
	test = t[*pos];
	test->id=LIST;
	
parsearrayend:
	(*pos)++;
	ret->extra = ret->extra < 8 ? 8 : ret->extra;
	return ret;
}
token getsettoken(int *pos,fun f)
{
	object_set head = init_perpetual_set();
	token ret = maketoken(SET,head);
	(*pos)++;
	token test;
	if(t[*pos]->id == END)  //表示为空
		goto setparseend;
	while(1)
	{
		test = t[*pos];
		if(ISCONSTELE(test->id,BBRACKETR))
		{
			addset(head,(object)test->attr);
			(*pos)++;
		}
		else
		{
			addtoken(ret,getsentencetree(pos,f,BBRACKETR));
		}
		
		test = t[*pos];
		if(test->id==DOUHAO)
			(*pos) ++;  //过滤掉逗号
		if(test->id == BBRACKETR)  //表示已经将]结束符给过滤了
			break ;
	}
	test->id=SET;
setparseend:
	(*pos)++;
	return ret;
}
token getdicttoken(int *pos,fun f)
{
	object_dict head=init_perpetual_dict();
	token ret = maketoken(DICT,head);
	(*pos)++;
	if(t[*pos]->id == COLON && t[*pos+1]->id == BBRACKETR)
	{
		(*pos) += 2;
		return ret;
	}
	token test;
	while(1)
	{
		token root = getsentencetree(pos,f,BBRACKETR);
		test = t[*pos];
		if(root->first->id == COLON)
		{
			token first = root->first->first,second = root->first->second;
			if(ISCONST(first->id) && ISCONST(second->id))
			{
				object_entry temp = init_perpetual_entry((object)first->attr,(object)second->attr);
				adddict(head,temp);
			}
			else
			{
				addtoken(ret,root);
			}
			if(t[(*pos)]->id == BBRACKETR)  //表示已经将]结束符给过滤了
				break ;
		}
		else
		{
			break ;
		}
		
		if(test->id==DOUHAO)
			(*pos) ++;  //过滤掉逗号
	}
	test->id=DICT;
parsedictend:
	(*pos)++;
	return ret;
}
token dealwithbigbracket(int *pos,fun f)
{
	int flag = pregetdict(*pos+1);
	if(flag < 0)
		return NULL;
	else if(flag == 1)
		return getdicttoken(pos,f);
	else
		return getsettoken(pos,f);
}

/*

在创立语法树的过程中就开始包装数值

*/

void addthistofun(fun f)
{
	addlocalvalue(f,"this");
}
token dealtoken(int flag,int *i,fun f=NULL);
void updateoverload(funlist fl)
{
	for(int i=0; i<fl->len; i++)
	{
		fun item = fl->fun_item[i];
		item->index = i;
		for(fun p = item->overload; p; p=p->overload)
		{
			p->index = fl->len;
			_addfun(fl,p);
		}
	}
}
 int checkparamfomat(int i)
{
	int error=dealfunbracket(i); 
	if(error==-1)
	{
		angel_error("函数参数格式出错！");
		return 0;
	}
	if(error==-2)
	{
		angel_error("函数参数列表后少一个）");
		return 0;
	}
	return error;
}
int getparameter(fun f,int *i)  //这里的pref在一般函数中肯定为空在成员函数中可能来自于
{
	int flag=0,error; //表示此时有没有默认参数
	while(*i<tokenlen)   //自增操作不能在这里进行，这里先不急着实现默认参数机制。
	{
		if(t[(*i)]->id==NAME)
		{
			if(flag && t[(*i)+1]->id!=ASSIG)
			{
				angel_error("函数的默认参数应该放在最后定义！");
				return 0;
			}
			f->paracount++;  //只要记录传入参数的个数就行了，
			addlocalvalue(f,(char *)t[*i]->attr);
		}
		else if(t[(*i)]->id==ASSIG)  //表明是默认参数
		{
			f->default_paracount++;
			flag=1;
			(*i)++;
			if(!f->default_para)
				f->default_para=maketoken(0,0);
			addtoken(f->default_para,getsentencetree(i));
			(*i)--; 
			if(t[*i]->id==END)
				break ;
		}
		else if(t[(*i)]->id==DOUHAO)
		{
			(*i)++;
			continue ;
		}
		else if(t[*i]->id==END)
			break ;
		else
		{
			angel_error("函数参数只能为普通标识符！");
			return 0;
		}
		(*i)++;
	}
	(*i)++;   //去掉后面的}
}
fun dealfuncdef(int *i,int flag = 0)
{
	funlist head;
	object_set fmap;
	char *funname = (char *)t[(*i)++]->attr;
	if(!funname)
	{
		(*i)-- ;
	}

	fun f=initfuncontrol(funname);  //i到参数括号
	if(!f)
		return NULL;
	f->type = flag;
	//给f设定类型，并初始化head和
	if(f->name && strcmp(f->name,"this")==0)  //这个肯定是类成员
	{
		//表示此时是构造函数
		if(!global_class_env)
		{
			angel_error("全局函数不能命名为this");
			return NULL;
		}
		f->type = 1; //覆盖
	}

	if(global_class_env)
	{
		f->class_context = global_class_env;
		if(f->type == 2)
		{
			head = global_class_env->mem_f;
			fmap = global_class_env->mem_f_map;
		}
		else  //这里包括this函数
		{
			head = global_class_env->static_f;
			fmap = global_class_env->static_f_map;
		}
	}
	else  //表示这里是一般函数的定义处
	{
		head = global_function;
		fmap = global_function_map;
	}


	if(t[(*i)++]->id==LBRACKETL)   //取出参数
	{
		if(!getparameter(f,i))  //取参数失败
			return NULL;
	}
	if(t[*i]->id==BBRACKETL)
	{
		(*i)++; //过滤
		int scope_num=0;
		int flag = 2;
		token root = dealtoken(flag,i,f);
		if(root)
			f->grammar = root;
	}
	if(f->name == NULL)
	{
		_addfun(temp_function,f);
		return f;
	}
	if(!addfun(head,fmap,f))
		return 0;
	return f;
}

int dealfunccall(token ft,int *pos,fun f)  //返回成功或失败
{
	
	//每次解析完之后要将这个角色放到最后一个解析的token上
	(*pos) += 2;  //过滤掉函数名和第一个小括号
	while(t[*pos-1]->id!=END)   
	{
		addtoken(ft,getsentencetree(pos,f));   //当遇到结束符时会自动将其过滤掉
		if(t[*pos-1]->id == END )      //因为这是虚拟结束符
		{
			t[*pos-1]->id = EXECFUN; //为后面的数组和索引表达式的识别提供便捷。
			break ;
		}
		(*pos)++;  //过滤掉逗号，虚假的结束符已在构建语法树的时候给过滤掉了,索引需要在循环体中另外判断虚假结束符。
	}
	ft->id = EXECFUN;
	return 1;
}
void inserttoken(token root1,token root2)
{
	if(!root2)
		return ;
	else
	{
		addtoken(root1,root2->first);
		free(root2);
	}

}
fun definefunction(int *i,fun f)
{
	int fun_flag = 0;
	if(f)
	{
		if(f->type == 1)  //表示此时是
			fun_flag = 2;
	}
	else
	{
		if(global_class_env)
		{
			fun_flag = 3;
		}
	}

	fun deff = dealfuncdef(i,fun_flag);  //此时已经将右括号给过滤掉了，因为创建语法树时每次都要过滤掉结束符（包括虚假的结束符）
	if(!deff)
	{
		angel_error("函数定义出错！");
		return NULL;
	}
	return deff;
}
int isfunctiondef(int pos,int *util = NULL)
{
	int next = checkparamfomat(pos);
	if(next == 0)
		return -1;
					
	int fun_flag = 0;
	if(util)
		*util = next;
	if(t[next] && t[next]->id == BBRACKETL)  //表示函数的的定义
		return 1;
	return 0;
}
int islambda(int pos)
{
	int next = detectbracket(pos,LBRACKETL);
	if(next == -1)
		return 0;
	if(t[next]->id == BBRACKETL)
	{
		t[next-1]->id = END;
		return 1;
	}
	return 0;
}
token gettree(int *pos,int *error,int endid,fun f)   //可以将点看做是运算符，若语法树建立过程中有错误则尽量进项到底，其他的异常信息交给执行树解决
{
	token temp;
	tstack os,ts;
	os=(tstack)malloc(sizeof(tstacknode));
	ts=(tstack)malloc(sizeof(tstacknode));
	ts->top=os->top=-1;

#define GETOPERSUBTREE  \
	token temp=pop(os); \
	temp->second=pop(ts); \
	if(temp->id != NO && temp->id != MAKEFUN) {\
		if(stackempty(ts)) \
		{ \
			FREEOTSTACK; *error = 1;\
			angel_error("缺少一个操作数！\n"); \
			return NULL; \
		} \
		else \
			temp->first=pop(ts);\
	}\
	push(ts,temp);

#define DEALOPER if(ispreoper == 1) break ; ispreoper = 1;
#define ISKEYWORD(id) ((id) < IF || (id) > RET)

	int ispreoper=0,isglobal=0;

	while(!ISSENTENCEEND(pos))  //这里表征了获取表达式可以以分号结束也可以以逗号结束。
	{
		//这里面的分支及其条件是核心，分类主要以其行为为标准分的
		if(t[*pos]->id==NAME || t[*pos]->id==TRUE || t[*pos]->id==FALSE ||(t[*pos]->id<= BOOLEAN && t[*pos]->id>= NU)|| t[*pos]->id==CLASS || t[*pos]->id==GLOBAL || t[*pos]->id==ORD_SENTEN || t[*pos]->id==GRAMROOT)  //这里需要扩展字符串的操作，而字符串目前只有+和*（必须和整形数字）
		{
			//错误提示功能
			DEALOPER
			if(t[*pos]->id == NAME)
			{
				if(NOTEND(*pos+1) && t[(*pos)+1]->id == LBRACKETL) //call function
				{
					token ft=t[*pos];

					if(ft->extra == 1)
						goto callfun;
					int funflag = isfunctiondef((*pos)+2);
					if(funflag == -1)
						return 0;
					if(funflag == 1)
					{
fundef:
						ft=t[*pos];
						fun deff = definefunction(pos,f);
						if(!deff)//此时已经将右括号给过滤掉了，因为创建语法树时每次都要过滤掉结束符（包括虚假的结束符）
						{
							angel_error("函数定义出错！");
							return NULL;
						}

						object_fun of = init_function(deff,CERTAIN_USER);
						of->refcount = 2;  //perpetual
						ft->id = FUNP;
						ft->attr = of;
					}
					else
					{
callfun:
						if(!dealfunccall(ft,pos,f))  //此时已经将右括号给过滤掉了，因为创建语法树时每次都要过滤掉结束符（包括虚假的结束符）
						{
							angel_error("调用函数时参数格式出错！");
							goto reterror;
						}

						if(isglobal)
							ft->extra=GLOBAL;
						isglobal=0;
					}
					push(ts,ft);
				}
				else
				{
					char *name=(char *)t[*pos]->attr;
					if(strcmp(name,"this")==0)  //这里是this指针,在函数运行的时候在动态的将其赋值为每个对象的引用，不过首先得动态建立一个this变量
					{
						if(!f || !global_class_env)
						{
							angel_error("this变量必须出现在成员函数中代码中！");
							goto reterror;
						}
						if(t[*pos+1]->id==ASSIG)
						{
							angel_error("this指针是不可写的！");
							goto reterror;
						}
					}
					push(ts,t[(*pos)++]);
				
					if(isglobal)
						t[(*pos)-1]->extra=GLOBAL;
					isglobal=0;
				}
			}
			else if(t[*pos]->id==CLASS)
			{
				if(!global_class_env)
				{
					angel_error("class关键字必须定义在类中！");
					goto reterror;
				}
				if(t[*pos+1]->id!=DOT)
				{
					angel_error("class关键字不可读写！");
					goto reterror;
				}
				push(ts,t[(*pos)++]);
			}
			else
				push(ts,t[(*pos)++]);

		}
		else if(t[*pos]->id==MBRACKETL)  
		{
			token res,test;
			test=t[(*pos)-1];
			if(ispreoper == 0)  //前面不是操作数，所以只能是list
			{
				if(test->id == INDEX)
				{
					res=getindex(pos,f);
					if(!res)
					{
						goto reterror;
					}
				}
				else
				{
					res = getarraytoken(pos,f);
					if(!res)
					{
						goto reterror;
					}
				}
				push(ts,res);
				ispreoper = 1;
			}
			else   //前面是操作数，这个是一个索引操作符
			{
				t[--(*pos)]=maketoken(INDEX,0);  //建立索引运算符,并让POS在倒退一步
			}
		}
		else if(t[*pos]->id == BBRACKETL)  //说明这是字典的初始化定义
		{
			DEALOPER
			token ret;
			ret = dealwithbigbracket(pos,f);
			
			if(!ret)
			{
				goto reterror;
			}
			push(ts,ret);
		}
		else  //这里是入栈运算符
		{
newbeg:
			int isright=0;
			if(!ispreoper)  //即上一次读入的是运算符
			{
				if(t[*pos]->id == DOT)  //表示此时是全局调用
				{
					(*pos)++;
					if(t[*pos]->id!=NAME)
					{
						angel_error("点运算符右边必须为变量或函数调用！");
						goto reterror;
					}
					isglobal=1;
					goto checktokenend ;
				}
				else
				{
					int negative = 0;
					while(t[*pos]->id == ADD || t[*pos]->id == SUB || t[*pos]->id == SELFADD || t[*pos]->id == SELFSUB)
					{
						if(t[*pos]->id == ADD)
							negative += 2;
						else if(t[*pos]->id == SUB)
							negative ++;
						else
						{
							if(!ISCALC(t[(*pos)+1]->id))
							{
								push(ts,maketoken(NOOPER,0));
								break ;
							}
						}
						(*pos)++;
					}
					if(negative != 0)
					{
						push(ts,maketoken(INT,getconstbyint(negative % 2 ? -1 : 1)));
						t[(*pos)-1] = maketoken(MUL,0);
						(*pos)--;
						goto checktokenend ;
					}
				}
			}
			else if(t[*pos]->id==SELFADD || t[*pos]->id==SELFSUB)
			{
				isright = 1;
			}
			if(stackempty(os)  ||  priority(t[*pos]) > priority(gettop(os)) || t[*pos]->id==LBRACKETL || t[*pos]->id==ASSIG && gettop(os)->id==ASSIG)  //多个复合赋值运算需要做一些改变
			{
				if(t[*pos]->id==LBRACKETL)  //可能是lambada函数
				{
					if(islambda((*pos)+1))  //肯定是函数定义了
					{
						DEALOPER;
						goto fundef;
					}

				}
				if(stackempty(os) && t[*pos]->id==LBRACKETR)  //如果出现括号不匹配，右括号多一个
				{
					angel_error("多出一个(符号");
					goto reterror;
				}
				push(os,t[(*pos)++]);
			}
			else
			{
				if(priority(t[*pos])<=priority(gettop(os)) || t[*pos]->id==LBRACKETR && !stackempty(os) && gettop(os)->id!=LBRACKETL)
				{
					if(t[*pos]->id==LBRACKETR && !stackempty(os) && gettop(os)->id==LBRACKETL) //左右括号抵消
					{
						pop(os);
						(*pos)++;
						goto checktokenend ;
					}
					else
					{
						//这里不能将ispreoper置为0，因为这样会打乱后面的操作
						GETOPERSUBTREE;
						goto newbeg;
					}
				}
				else if(t[*pos]->id==LBRACKETR && stackempty(os))  //这钟情况下栈顶肯定为左括号,将（与）抵消掉
				{
					angel_error("多出一个)符号");
					goto reterror;
				}
			}
			/*if(t[(*pos)-1]->id==INDEX)
				t[(*pos)-1]=maketoken(SUBITEM,0);*/
			if(isright)
				push(ts,maketoken(NOOPER,0));

			ispreoper=0;
		}
checktokenend:
		if(!NOTEND(*pos)) goto getfinaloper;
	}

	if(t[*pos]->id == END)    //逗号不能被过滤
	{
		
		if(t[(*pos)-1]->id!=DOUHAO)
		{
			(*pos)++; //过滤掉结束符
		}
		else
		{
			angel_error("逗号不能在结束符后！\n");
reterror:
			*error = 1;
			FREEOTSTACK
			return NULL;
		}
	}
getfinaloper:
	if(!stackempty(os))
	{
		while(!stackempty(os))
		{
			GETOPERSUBTREE;
		}
	}
	token root;
	if(stackempty(ts))   
	{
		FREEOTSTACK
		return NULL;
	}
	root=pop(ts);
	FREEOTSTACK;
	return root;
}
token getsentencetree(int *pos,fun f,int endid)      //执行表达式树，表达式树的建立最复杂的成分即是根据运算符的优先级来进行出入栈操作，这里主要将情况分为三类：栈顶和输入均不为括号的一般运算符、栈顶为扩号输入不为括号、栈顶不为括号输入为括号、栈顶和输入都为括号
{
	//条件模型，在处理表达式优先级时尤为重要
	token root,tree;
	int error = 0;
	tree=gettree(pos,&error,endid,f);
	if(error)
		return NULL;
	root=maketoken(ORD_SENTEN,0);
	root->first=tree;
	return root;
}

int dealforbracket(int pos)
{
	tstack s=(tstack)malloc(sizeof(tstacknode));
	int i=pos;
	s->top=-1;
	push(s,t[pos-1]);
    while(!stackempty(s))                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                       	while(!stackempty(s))   //进行扩号匹配
	{
		if(t[i]->id==LBRACKETL)
			push(s,t[i]);
		else if(t[i]->id==LBRACKETR)
			pop(s);
		i++;
	}
	if(i < tokenlen && t[i]->id==LBRACKETR)
		return -1;
	t[i-1]->id=END;
	free(s);
	return i-1;
}

token getexptree(int *pos,fun f=NULL)    //条件表达式树,此时传入的指针包含左括号
{
	(*pos)++;
	if(dealforbracket(*pos) < 0)
		return NULL;
	return getsentencetree(pos,f);   //若结果栈里不止一个结果则以第一个结果为准。
}

int getvaluewithscopeenv(char *valuename,fun f,uint16_t *offset,int isglobal = 0);
void addscopevalue(char *name,fun f)
{
	uint16_t offset;
	object_set ml;
	if(getvaluewithscopeenv(name,f,&offset))  //需要修复
	{
		//表示此时
		return ;
	}
	if(f)
	{
		ml = (object_set)f->current_scope->data;
		offset = f->localcount++;
	}
	else
	{
		ml = (object_set)global_current_scope->data;
		offset = global_value_list->len++;
	}
	addmap(ml,name,offset);
}
/*
void updatescopesize(fun f,token scopet,token scopep,int *parallel,int parallelold)
{
	scope sp;
	if(!f)
	{
		sp = global_scope;
	}
	else
	{
		sp = f->local_scope;
	}
	if(scopep)
		scopep->extra += parallelold; //这里是叠加了scopet子控制语句的空间
	
	*parallel = *parallel< scopet->extra ? scopet->extra : *parallel;   //这里都是形同虚设
	sp->alloc++;
}
*/
void addsetencetoroot(token root,token exp,fun f)
{
	token roottemp;
	if(global_class_env && !f)
	{
		roottemp = global_class_env->entry_token;
	}
	else
	{
		roottemp = root;
	}
	addtoken(roottemp,exp);
}
token dealtoken(int flag,int *i,fun f)  //这仅仅是主程序语法产生的语法树。
{ 
//scope_parallel_num是控制语句中的局部变量的水平空间，而每个token里放的是垂直空间，每个垂直空间呈递给上一级的水平空间	
//#define SET_SCOPE_VALUE_INDEX_BEGIN(st) if(scopet) st->s_extra = scopet->s_extra; else st->s_extra = -1;
#define ISPARSEASLIB (!global_class_env && !f && islib == 1)
	token root=(token)malloc(sizeof(tokennode));  //总的语法根节点
	memset(root,0,sizeof(tokennode));
	root->id=GRAMROOT;   //对于语法根节点
	int scope_parallel_num = 0;
	if((*i) >= tokenlen) {
		return root;
	}
	token temp=t[(*i)++];
	int declare=0;
	while(1)  //总的循环
	{
		//以下实现每个语句的解析,每个分支解决后将指针放在下一个模块解析的位置

		if(temp->id == LIB)
		{
			islib = 1;
		}
		else if(temp->id == MAIN)
		{
			islib = 0;
		}
		else if(temp->id == ORD_SENTEN) //表示预处理的
		{
			addsetencetoroot(root,temp,f);
		}
		else if(temp->id==NAME && (NOTEND(*i) && t[*i]->id==LBRACKETL))  //这个表明此时已经是函数，可能是函数调用或者函数的定义
		{
			(*i)--;
			int util;
			int funflag = isfunctiondef((*i)+2,&util);

			if(funflag == -1)
				return 0;

			if(funflag == 1)  //表示函数的的定义
			{
				if(!definefunction(i,f))
					return NULL;
			}
			else
			{
				temp->extra = 1;  //表示已经check过了
				if(ISPARSEASLIB)
				{
					*i = util;
					continue ;
				}
				else
					goto getexptree; //表示调用函数
			}
		}
		else if(temp->id == CLASS && !global_class_env)
		{
			if(t[(*i)]->id != NAME)
			{
				angel_error("class关键字只能在类里使用！");
				return NULL;
			}
			char *classname = (char *)t[(*i)++]->attr;
			if(!addclass(classname))
			{
				angel_error("添加类失败！");
				return 0;
			}
			if(t[(*i)]->id != BBRACKETL)  //这个表示是正常的类，未来可能要增加新的东西（比如继承）
			{
				angel_error("类定义失败！");
				return 0;
			}
			else
				(*i)++;

			uint16_t offset = getoffset(class_map,classname);
			global_class_env = clist->c[offset];
			if(!dealtoken(2,i))
				return NULL;
			global_class_env = NULL;
		}
		else
		{
			if(ISPARSEASLIB) {
				temp = t[(*i)++];
				continue ;
			}

			if(temp->id == SWITCH)
			{
				if(t[*i]->id!=LBRACKETL)
				{
					angel_error("switch语句出错！");
					return NULL;
				}
				token switchtoken,temp;
				switchtoken=maketoken(SWITCH_SENTEN,NULL);
				temp=getexptree(i,f);  //switch的条件变量
				addtoken(switchtoken,temp);
				if(t[(*i)++]->id==BBRACKETL)
				{
					int tablelen=10,isdefault=0;
					switchtable st_table=(switchtable)malloc(sizeof(switchtable));
					st_table->sw_item=(_switch *)calloc(tablelen,sizeof(_switchnode));
					st_table->len=0;
					token mergetoken = maketoken(0,0);
					while(1)
					{
						if(t[*i]->id==CASE || t[*i]->id==DEFAULT)
						{				
	tocase:
							token casetoken=maketoken(CASE_SENTEN,0);
							_switch st=(_switch)malloc(sizeof(_switchnode));
							if(t[(*i)++]->id==CASE)
							{
								if(t[*i]->id==STR)
								{
									st->hash=stringhash(GETSTR(t[*i]->attr)->s,GETSTR(t[*i]->attr)->len);
									st->type=STR;
								}
								else if(t[*i]->id==INT)  //这里需要包装数值
								{
									st->hash = GETINT(t[*i]->attr);
									st->type=INT;
								}
								else
								{
									angel_error("case后必须接数字或字符串常量");
									return NULL;
								}
								(*i)++;
							}
							else
							{
								if(isdefault)  //已经有了一个default
								{
									angel_error("一个switch-case语句只能有一个default条件！");
									return NULL;
								}
								isdefault=1;
								st->type=0;
							}
							st_table->sw_item[st_table->len++]=st;
							if(st_table->len>=tablelen)
							{
								tablelen+=10;
								st_table->sw_item=(_switch *)realloc(st_table->sw_item,tablelen*sizeof(_switchnode));
							}
							if(t[(*i)++]->id!=COLON)
							{
								angel_error("case条件后应接冒号！");
								return NULL;
							}
							addtoken(mergetoken,casetoken);
							if(t[*i]->id == CASE)
							{
								goto tocase;
							}
							else
							{
								token root=dealtoken(3,i,f);
							
 								root->attr = casetoken->attr;
								root->extra = casetoken->extra;

								for(token p = mergetoken->first; p; p = p->second)
								{
									addtoken(p,root);
								}
								addtoken(switchtoken,mergetoken->first);
								mergetoken->first = NULL;
								if(t[(*i)-1]->id==BBRACKETR)
									break ;
								else
									(*i)--;
							}
						}
					}
					if(!isdefault && st_table->len>=tablelen)  //如果没有default则预留一个保存在后的首地址
						st_table->sw_item=(_switch *)realloc(st_table->sw_item,(tablelen+1)*sizeof(_switchnode));
					switchtoken->extra=_global_sw_list->len;   //表示
					_global_sw_list->st_table[_global_sw_list->len++]=st_table;
				}
				else
				{
					angel_error("switch语句后要接一个左大括号！");
					return NULL;
				}
				addsetencetoroot(root,switchtoken,f);
			}
			else if(temp->id==IF)  //if语句
			{
				if(t[*i]->id!=LBRACKETL)
				{
					angel_error("if语句出错！");
					return NULL;
				}
				token iftoken,temp;
				iftoken = maketoken(IF_SENTEN,0);
				temp=getexptree(i,f);
				if(!temp->first)
				{
					angel_error("if语句条件不能为空！");
					return NULL;
				}
				addtoken(iftoken,temp);
				//此时有的字iftoken会没有maplist这说明此时不需要申请内存
				if(ISEND(*i) || t[*i]->id!=BBRACKETL)
				{
					temp=dealtoken(1,i,f);
				}
				else
				{
					(*i)++;
					temp = dealtoken(2,i,f);
				}
				if(!temp)
					return NULL;

				int ifalloc = iftoken->extra;
				iftoken->extra = 0;
				temp->attr = iftoken->attr;
				iftoken->attr = NULL;
				addtoken(iftoken,temp);
				if(!ISEND(*i)) 
				{
					if(t[*i]->id==ELSE)
					{
						if(t[++(*i)]->id!=BBRACKETL)  //这是过滤掉else关键字
						{
							temp = dealtoken(1,i,f);
						}
						else
						{
							(*i)++;
							temp=dealtoken(2,i,f);
						}
						if(!temp)
							return NULL;
						temp->attr=iftoken->attr;
						int elsealloc = iftoken->extra;
						temp->extra = ifalloc < elsealloc ? elsealloc : ifalloc;
						addtoken(iftoken,temp);
					}
				}
				addsetencetoroot(root,iftoken,f);
			}
			else if(temp->id==LOOP)
			{
				token looptoken,temp;
				looptoken=(token)malloc(sizeof(tokennode));
				memset(looptoken,0,sizeof(tokennode));
				looptoken->id=LOOP_SENTEN;


				temp=getexptree(i,f);
				if(!temp->first)
				{
					angel_error("while语句条件不能为空！");
					return NULL;
				}
				addtoken(looptoken,temp);
			
			
				if(ISEND(*i) || t[(*i)]->id!=BBRACKETL)
				{
					temp = dealtoken(1,i,f);
				}
				else
				{
					(*i)++;
					temp = dealtoken(2,i,f);
				}
				if(!temp)
					return NULL;

				temp->attr = looptoken->attr;
				temp->extra = looptoken->extra;
				addtoken(looptoken,temp);
				addsetencetoroot(root,looptoken,f);
			}
			else if(temp->id == FOR)
			{
				(*i)++;
				int error=dealforbracket(*i);
				if(error == -1)
				{
					angel_error("for循环多一个)");
					return NULL;
				}
				if(error==-2)
				{
					angel_error("for循环少一个）");
					return NULL;
				}
				token looptoken,temp,selfadd,scope_in;
				looptoken=maketoken(FOR_SENTEN,0);
			
				temp=dealtoken(4,i,f);

				if(!temp) return NULL;

				token tmp = temp->first;
				if(!tmp->first)
				{
					angel_error("for语句条件不能为空！");
					return NULL;
				}
				if(tmp->first->id == IN)  //表示此时是遍历的语句
				{
					addtoken(looptoken,tmp);
				}
				else  //这时正常的for语句
				{
					//能否支持for(;;;) for() ,for里的三个语句可以用;
					//隔开也可以用空格隔开,可以隔开子语句
					//主要的逻辑是用空格或;先分开每个语句，解析前三个就好，多出来的就不解析了，如果烧出来了，就默认没有对应的语句，但是第二个语句必须为true
					addtoken(looptoken,temp);
					if(error == (*i)-1)  //表示for循环中就一个语句
					{
						temp = maketoken(ORD_SENTEN,0);
						temp->first = maketoken(TRUE,0);
						selfadd = maketoken(ORD_SENTEN,0);
					}
					else
					{
						temp = getsentencetree(i,f);
						if(!temp)
							return NULL;
						if(error == (*i)-1)
						{
							selfadd = maketoken(GRAMROOT,0);
							selfadd->first = maketoken(ORD_SENTEN,0);
						}
						else
						{
							selfadd = dealtoken(4, i, f);
							if(!selfadd)
								return NULL;
						}
					}
					addtoken(looptoken,temp);
					addtoken(looptoken,selfadd);
				}
				*i = error+1;
				

				if(ISEND(*i) || t[(*i)]->id != BBRACKETL)
					temp = dealtoken(1,i,f);
				else
				{
					(*i)++;
					temp = dealtoken(2,i,f);
				}
				if(!temp)
					return NULL;

				addtoken(looptoken,temp);
				addsetencetoroot(root,looptoken,f);
				//loop
				//	|-init
				//	|-cond
				//  |-iteration
				//  |-body
			}
			else if(temp->id==BREAK || temp->id==CONTINUE)   //表示循环跳出语句。
			{
				token breaktoken = maketoken(0,0);
				if(temp->id==BREAK)
					breaktoken->id=BREAK_SENTEN;
				else
					breaktoken->id=CONTINUE_SENTEN;
				addsetencetoroot(root,breaktoken,f);
			}
			else if(temp->id==RET)
			{
				if(f)
				{
					if(f->name && strcmp(f->name,"this") == 0)
					{
						angel_error("构造函数this中不能有ret语句！");
						return NULL;
					}
				}
				else if(flag == 6)
				{
					angel_error("class初始化代码中不能有ret语句！");
					return NULL;
				}
				token rettoken=maketoken(RET_SENTEN,0),temp;
				temp=getsentencetree(i,f,BBRACKETR);
				if(!temp)
					return NULL;
				addtoken(rettoken,temp);
				addsetencetoroot(root,rettoken,f);
			}
			else if(temp->id==DOUHAO)  //逗号语句
			{
				temp=t[(*i)++];
				continue ;
			}
			else if(temp->id == DECLARE)
			{
				(*i)+=2;
				token roottemp=dealtoken(2,i,f);
				if(!roottemp)
					return NULL;
				addsetencetoroot(root,roottemp->first,f);
			}
 			else   //这时一般赋值或表达式语句,这个条件只有在有声明var时才有效果除此之外指针就会出错
			{
				(*i)--;//因为每一次运行到这里前都要执行一次（*i）++
	getexptree:
				token texp = getsentencetree(i,f,BBRACKETR);
				if(!texp)
					return NULL;
				addsetencetoroot(root,texp,f);
			}
		}
		if(*i>=tokenlen)
		{
			if(flag==2)
			{
				char errorinfo[errorsize];
				if(global_class_env)
				{
					if(f)
						sprintf(errorinfo,"类%s成员函数%s定义缺少}号",global_class_env->name,f->name);
					else
						sprintf(errorinfo,"类%s定义缺少}号",global_class_env->name);
				}
				else
				{
					if(f)
						sprintf(errorinfo,"函数%s定义缺少}号",f->name);
					else
						sprintf(errorinfo,"语句块缺少}号");
				}
				angel_error(errorinfo);
				return NULL;
			}
			break ;
		}
		if(flag == 4 || flag == 1)  //如果是逗号则还需要继
		{
			if(t[*i]->id==DOUHAO)
				temp=t[(*i)++];
			else
				break ;
		}
		temp=t[(*i)++];
		if(flag == 3)
		{
			if(temp->id == CASE || temp->id == DEFAULT || temp->id == BBRACKETR)
				break ;
		}
		else if((flag==2) && temp->id==BBRACKETR)
			break ;
	}
	return root;
}  //处理主程序或代码快的
void init()   //所有的销毁和初始化代码都在这里
{
	obj_list=init_perpetual_list();  //建立对象的列表，保存字面量，是只读的
	global_value_list=init_perpetual_list();  //创建全局变量的列表
	global_function=initfunlist();
	temp_function = initfunlist();
	global_value_map=init_perpetual_set();
	global_function_map=init_perpetual_set();
	class_map = init_perpetual_set();

	init_const_pool();
	init_lib_func_map();
	name_pool = init_perpetual_set();
	keytable = initkeylib(keywordlib);

	

	clist=(classlist)calloc(1,sizeof(classlistnode));
	_global_sw_list=(switchlist)calloc(1,sizeof(switchlistnode));


	
	addlist(obj_list,angel_null);
	addlist(obj_list,angel_true);
	addlist(obj_list,angel_false);
	addlist(obj_list,getconstbyint(LONG_MAX));
	addlist(obj_list,getconstbyint(LONG_MIN));
}
token grammartree_entry()    //总程序的入口,创建整个语法树
{
	int point=0;
	token grammarroot;
	//可以在这个地方添加一些内置的对象和方法，比如字符串和列表。
	if(tokenlen>0)
		grammarroot=dealtoken(0,&point,NULL); //0表示主程序，1表示单个语句，2表示函数语句块，3表示switch结束，4表示强制结束，5表示while语句块，6代表的是类的定义
	else
		grammarroot=NULL;

	//更新函数的重载
	updateoverload(global_function);
	for(int i = 0; i<clist->len; i++)
	{
		pclass item = clist->c[i];
		updateoverload(item->mem_f);
		updateoverload(item->static_f);
	}
	return grammarroot;
}









/*

字节码生成过程

所有的表达式语句从原则上说都应该将结果写入寄存器1

//全局变量的临时记录在list中，局部变量的临时记录在map中，因为此时还没有分配空间


编译环节以后不需要再报什么运行时的类型问题，而直接产生字节码，运行时错误有解释运行时才报这时直接将运行错误的行号贴出来，编译时只报定义错误

所以这里要建立一个字节码范围与源程序文本的对应表。


未来所有的指令都不需要直接编译操作数节点，操作数节点一般都直接做立即数寻址，只有再双元运算情况下才需要将其中一个放到寄存器
*/
//ISASSIG宏是探测是否为逻辑运算或bool常量，否则一律要用translate进行转化
void adddynamicname(char *name)
{
	uint16_t offset = dynamic_name->len;
	addmap(name_pool,name,offset);
	addlist(dynamic_name,checkobjbystr(name_pool,name));
	ADDWORD(offset);
}
void initangeltemp(int initval)
{
	angel_temp->top = 0;
	angel_temp->inuse = initval;  //可以随时改动
}
void releasetempnum(int num,fun f)
{
	angel_temp->stack[angel_temp->top++] = num;
}
int gettempnum(fun f)
{
	if(angel_temp->top == 0)  //说明此时没有回收的寄存器
	{
		if(angel_temp->inuse > temp_max_size)
		{
			angel_error("中间变量溢出！！！");
			return -1;
		}
		if(f)
			return f->localcount++;
		else
			return angel_temp->inuse++;
	}
	return angel_temp->stack[--angel_temp->top];
}
token gettopctrl()
{
	int i;
	/*for(i=0; i<=ctrl_state->top; i++)
		return ctrl_state->t[i];
	return NULL;*/
	return ctrl_state->t[ctrl_state->top];
}
int isidnum(uint16_t id)
{
	uint16_t lib[]={INT};   //这里后面需要进行扩充
	int i;
	for(i=0; i<1; i++)
	{
		if(lib[i]==id)
			return 1;
	}
	return 0;
}
void insert_pc(unsigned long begin,unsigned long offset)
{
	angel_byte->len=begin;
	
	addfourbyte(angel_byte,offset);
	angel_byte->len=offset;   //还原
}
void addbreakoffset(unsigned long addr)
{
	int i;
	for(i=brand->length-1; brand->head_addr[i]; i--)
		insert_pc(brand->head_addr[i],addr);
	brand->length = i;
}
int parse_kernel(token t,fun);  //其中这里的flag只会影响成员变量的获取。最后一个参数表示返回的寄存器号
int parse_ord(token t,fun f);  //这里的int *参数是返回一个语句执行结果之后的结果寄存器号，一般再外部我们直接调用该函数而非parse_kernel函数
int parse_grammar(token t,fun f);
int operationdata(token t,fun f);
int parse_function(fun p);
int parse_if(token t,fun f);
int assignment(token t,fun f);
int genopercodewithres(uchar inst,fun f);
int boolcalc_u(uint16_t inst,token t,fun f);
int genbinarycodewithnoresEX(token root,uchar inst,fun pref);
int genunarycodewithres(token root,uchar inst,fun pref);
int genunarycodewithnores(token root,uchar inst,fun pref);
int getoperationinfo(token root,uint16_t *offsetres,fun pref);
int gentempunarycode(token index,uchar inst,fun f);
int genreleasetempcode(token index,int offset2);
int pushdefaultparam(fun f,int count);

int genopercodewithnores(uchar inst)  //返回的是真正的索引号+1
{
	ADDBYTE(inst);
	return 1;
}

int isoperationdata(uint16_t id)
{
	return ((id >= NU && id <= SET) || (id == NAME) || (id == CLASS) || (id == EXECFUN));
}
int16_t getconstindex(token t)
{
	int16_t offset = obj_list->len;
	object o = (object)t->attr;
	addlist(obj_list,o);
	return offset;
}
int dynamic_list(uchar inst,token t,fun f)
{
	int flag = genopercodewithres(inst,f);
	ADDWORD(t->extra);
	for(token p = t->first; p; p=p->second)
	{
		int insttemp;
		token parse = p->first;
		if(!parse)
			continue ;
		if(parse->id == LIST && !parse->first)  //有p->first表示此元素需要动态生成
		{
			insttemp = _extend_list_local;
		}
		else
		{
			insttemp = inst+1;
		}
		if(!genunarycodewithnores(parse,insttemp,f))
			return -1; 
		ADDWORD(flag);
	}
	t->extra = 200;
	return flag;
}
int dynamic_set(uchar inst,token t,fun f)
{
	int flag = genopercodewithres(inst,f);

	ADDWORD(-getconstindex(t)-1);
	for(token p = t->first; p; p=p->second)
	{
		int insttemp;
		token parse = p->first;
		
		insttemp = inst+1;
		parse = p->first;
		if(!parse)
			continue ;
		if(!genunarycodewithnores(parse,insttemp,f))
			return -1; 
		ADDWORD(flag);
	}
	t->extra = 200;
	return flag;
}
int dynamic_dictionary(token t,fun f)
{
	int flag = genopercodewithres(_build_dict,f);
	ADDWORD(-getconstindex(t)-1);
	for(token p = t->first; p; p=p->second)
	{
		token parse = p->first;
		if(!parse)
			continue ;
		int16_t offset = genunarycodewithres(parse->first,_mov_local,f);  //index
		if(offset == -1)
			return 0;
		parse->first->id = COMPUTED;
		parse->first->extra = flag;
		if(!genbinarycodewithnoresEX(parse,_store_index_shared_local,f))
			return 0;

		ADDWORD(offset);  //index
		releasetempnum(offset,f);
	}
	t->extra = 200;
	return flag;
}
int immediate_data(token t,fun f=NULL)
{
#define EXTRA_FIELD *(int16_t *)((char *)o+sizeof(object_intnode))
	object o;
	object_set test;
	object_string stest;
	int16_t offset;
	uchar dynamic_inst;
	int16_t argcount;
	switch(t->id)
	{
	case FUNP:
		o = (object)t->attr;
		offset =  obj_list->len;
		addlist(obj_list,o);
		break ;
	case INT:
	case FLOAT:
		o=(object)t->attr;
		if(EXTRA_FIELD == 0)   //说明还没有放入到obj_list中。
		{
			EXTRA_FIELD = obj_list->len;
			addlist(obj_list,o);
		}
		offset = EXTRA_FIELD;
		break ;
	case STR:
		stest=(object_string)t->attr;
		if(stest->extra == 0)   //说明还没有放入到obj_list中。
		{
			stest->extra = obj_list->len;
			addlist(obj_list,(object)stest);
		}
		offset = stest->extra;
		break ;
	case LIST:
		if(!t->attr)  //注意这里的条件与set和dict有所不同
			return dynamic_list(_build_list,t,f);
		else
			offset = getconstindex(t);
		break ;
	case DICT:
		return dynamic_dictionary(t,f);
	case SET:
		return dynamic_set(_build_set,t,f);
	case TRUE:
		offset = 1;
		break ;
	case FALSE:
		offset = 2;
		break ;
	case NU:
		offset = 0;
		break ;
	case MAX:
		offset = 3;
		break ;
	case MIN:
		offset = 4;
		break ;
	}
	return -offset-1;
}
int getvaluewithscopeenv(char *valuename,fun f,uint16_t *offset,int isglobal)
{
	linkcollection current,head;
	object_set g_map;
	if(f)
	{
		head = f->local_scope;
		current = f->current_scope;
		g_map = f->local_v_map;
	}
	else
	{
		head = global_scope;
		current = global_current_scope;
		g_map = global_value_map;
	}
	int res;
	int flag = 0;
	for(linkcollection p = current; p->data; p = p->next)
	{
		object_set map = (object_set)p->data;
		res = getoffset(map,valuename);
		*offset = res;
		if(res != -1) return 1;
	}

	for(linkcollection p = current->pre; p != head; p = p->pre)
	{
		object_set map = (object_set)p->data;
		if(!map)
			continue ;
		res = getoffset(map,valuename);
		*offset = res;
		if(res != -1) return 1;
	}
	
	res = getoffset(g_map,valuename);
	if(res == -1) return 0;
	*offset = res;
	return 1;
}
uint16_t getvalueoffset_s(char *valuename,fun f,int *flag)  //在这里做文章
{
	int error;
	uint16_t offset;
	*flag=1;
	int i;
	error = getvaluewithscopeenv(valuename,f,&offset,1);
	if(!f)
	{
		if(!error)
			*flag = 0;
		else
			*flag = 2;
		return offset;
	}
	//if(strcmp(valuename,"this")==0 && f->type!=0)
		//return f->localcount-1;
	if(!error)
	{
		char errorinfo[errorsize];
		if(f->type != 0) //表示函数为构造函数或者为成员函数
		{
			*flag = 3;  //需要动态确定变量位置。
			return 0;//无意义
		}
		error = getoffset(global_value_map,valuename);
		if(error != -1)
		{
			*flag=2;  //表示此时是函数访问全局变量
			return error;
		}
		*flag=0;
	}
	else
		return offset;
}
uint16_t getvalueoffset(char *valuename,fun f,int *flag)
{
	uint16_t offset;
	char errorinfo[errorsize];
	offset=getvalueoffset_s(valuename,f,flag);
	if(*flag==0)
	{
		if(f)
			sprintf(errorinfo,"函数%s中变量%s未定义",f->name,valuename);
		else
			sprintf(errorinfo,"变量%s未定义",valuename);
		angel_error(errorinfo);
	}
	else
		return offset;
}
uint16_t getobjoffset(char *valuename,fun f,int *flag)
{
	int offset;
	char errorinfo[errorsize];
	offset=getvalueoffset_s(valuename,f,flag);
	if(*flag==0 || *flag==3)
	{
		offset=getclassoffset(valuename);
		if(offset!=-1)
		{
			*flag=4;
			return offset;
		}
		if(*flag==3)
			return 0; //无意义
		if(f)
			sprintf(errorinfo,"函数%s中变量%s未定义",f->name,valuename);
		else
			sprintf(errorinfo,"变量%s未定义",valuename);
		angel_error(errorinfo);
	}
	else
		return offset;
}
int indirect_addr(token t,fun f,uint16_t *offset_ret)  //这里都是直接默认是全局变量
{
	int error;
	uint16_t offset;
	char code=0;
	char *valuename=(char *)t->attr;


	if(t->extra==GLOBAL) //表示是全局变量
	{
		offset=getvalueoffset(valuename,NULL,&error);
		if(error != 0) //如果变量valuename找到了。
			error=2;
	}
	else
		offset=getvalueoffset(valuename,f,&error);
	*offset_ret = offset;
	return error;
}

/*
指令中所谓的操作数

*/
void mergesharedmemory()
{
	//所有确定下来的静态内存都应该用系统申请，这样避免了不必要的错误
	object *addr = dynamic_name->item;
	sys_realloc_list(dynamic_name,dynamic_name->len);
	memcpy(dynamic_name->item,addr,dynamic_name->len*sizeof(object));
	sys_realloc_list(global_value_list,obj_list->len+global_value_list->len);
	addr = global_value_list->item+obj_list->len;
	for(int i = 0; i < global_value_list->len; i++)
	{
	//	memset(global_value_list->item+obj_list->len,0,global_value_list->len*sizeof(object));
		addr[i] = angel_uninitial;
	}

	for(int i = 0; i<obj_list->len; i++)
	{
		global_value_list->item[obj_list->len-1-i] = obj_list->item[i];
	}
	global_value_list->alloc_size = obj_list->len;
	obj_list->refcount = 0;
	global_value_list->len += obj_list->len;
}
int pushparam(token t,fun pref)
{
	//push参数的时候直接操作栈顶指针，因为栈顶指针代表着局部变量域，但值得注意的是当所有的参数指令都处理完后再设置当前栈底并保存前次栈顶后全局基地址
	int count=0;
	token p;
	for(p=t; p; p=p->second)
	{
		if(!genunarycodewithnores(p->first,_push_local,pref))
			return 0;
		count ++; //这里是计算传入参数的个数
	}
	return count;
}
int function_call(token t,fun pref)  //它与mem_call最大的不同之一是他是一个原子操作
{
	fun f;
	uint16_t offset,count=0,i,defaultindex;
	bytecode temp;
	token p;
	char *funname=(char *)t->attr;
	int error,callnum=-1;
	//处理传入的参数，包括默认参数，这个以后再想办法
	//这里首先用nop代替，因为不知道该函数是否是成员函数
	
	//对于call_d的方式可以尝试用exec_enviromnet-1的形式得到this对象，因为我们有严格的方式来保证this在栈顶位置
	count=pushparam(t->first,pref);
	if(count == -1)
		return 0;

	if(pref && t->extra!=GLOBAL)  //这里需要建立一个token列表保存这里的调用语句以便实现动态编译，最大可能的减少字节码的个数
	{
		if(pref->type!=0) //如果是特殊函数
		{
			t->extra = genopercodewithres(_call_default,pref);
			adddynamicname(funname);
			addtwobyte(angel_byte,count); //运行一次最终都会变为静态代码
			return 1;
		}
	}


	if((callnum=isglobalsyscall(funname,count))>=0)  
	{
		t->extra = genopercodewithres(_sys_call,pref); 
		addtwobyte(angel_byte,callnum);  
		addtwobyte(angel_byte,count);
		return 1; 
	}


	int foffset=getfunoffset(global_function,global_function_map,funname,count);
	
	if(foffset==-1)
	{
		//调用回掉函数
		t->id = NAME;
		foffset = genunarycodewithres(t,_mov_local,pref);
		if(foffset == -1)
			return 0;
		t->extra = genopercodewithres(_call_back,pref);
		ADDWORD(foffset);
		releasetempnum(foffset,pref);
		ADDWORD(count);
		t->id = EXECFUN;
		return 1;
	}
	t->extra = genopercodewithres(_call,pref);
	ADDWORD(foffset);
	ADDWORD(count);
	return 1;
}

int genopercodewithres(uchar inst,fun f)  //返回的是真正的索引号+1
{
	int regnum = gettempnum(f);
	if(regnum == -1)
		return -1;
	ADDBYTE(inst);
	ADDWORD(regnum);
	return regnum;
}
int genopercodewithspecres(uchar inst,int num)
{
	ADDBYTE(inst);
	ADDWORD(num);
	return num;
}
#define ISBOOLEANCONST(id) (id == TRUE || id == FALSE)
#define ISLOGICAL(id) (id>=EQU && id <= OR)
int getoperationinfo(token root,uint16_t *offsetres,fun pref)  //返回：1为local，2为shared，3为regnum
{
	token t = root;
	if(!root)
		return 0;
	if(isoperationdata(t->id))
	{
		uint16_t offset;
		if(t->id==NAME)
		{
			char type = indirect_addr(t,pref,offsetres);
			if(type == 3)
			{
				//此时算运算指令
				*offsetres = genopercodewithres(_load_dynamic,pref);
				if(*offsetres == -1)
					return 0;
				adddynamicname((char *)t->attr);
			}
			return type;
		}
		else if(t->id==EXECFUN)
		{
			//此为运算指令
			if(!function_call(t,pref))
				return 0;
			*offsetres = t->extra;
			return 3;
		}
		else
		{
immediate:
			int flag = immediate_data(t,pref);
			*offsetres = flag;
			if(flag >= 0)
			{
				if(t->extra == 200)
					return 3;
				else
					return 0;
			}
			return 2;
		}
	}
	else if(t->id == COMPUTED)
	{
		*offsetres = t->extra;
		return 1;
	}
	else
	{
kernel:
		if(!parse_kernel(t,pref))
			return 0;
		if(ISASSIG(t->id))  //本身并没有返回值
		{
			return getoperationinfo(t->first,offsetres,pref);
		}
		*offsetres = t->extra;
		return 3;
	}
}

/*
代码生成函数系列
*/
#define ISARITHMETIC(id) (id >= ADD && id <= MOD)
#define ISFUNCALL(t) (t->id==EXECFUN || (t->id == DOT && t->second->id == EXECFUN)) 
#define ISSTACKCHECKNEEDED(t) (ISARITHMETIC(t->id) || ISFUNCALL(t) || t->id == INDEX)
int genunarycodewithnores(token root,uchar inst,fun pref)  //生成指令形如：inst|operand
{
	uint16_t res ;
	int flag = getoperationinfo(root, &res, pref);
	switch(flag)
	{
	case 0:
		return 0;
	case 1:  //表示local
		ADDBYTE(inst);
		break ;
	case 2:  //表示shared
		ADDBYTE(inst+1);
		break ;
	case 3:
		ADDBYTE(inst);
		releasetempnum(res,pref);
		break ;
	}
	ADDWORD(res);
	return 1;
}
int genunarycodewithnoresEX(token root,uchar inst,fun pref)  //生成指令形如：inst|operand
{
	uint16_t res ;
	int flag = getoperationinfo(root, &res, pref);
	switch(flag)
	{
	case 0:
		return 0;
	case 1:  //表示local
		ADDBYTE(inst);
		break ;
	case 2:  //表示shared
		ADDBYTE(inst+1);
		break ;
	case 3:
		if(ISSTACKCHECKNEEDED(root)){
			ADDBYTE(inst+2);
		}
		else{
			ADDBYTE(inst);
		}
		releasetempnum(res,pref);
		break ;
	}
	ADDWORD(res);
	return 1;
}
int genunarycodewithres(token root,uchar inst,fun pref) //生成指令形如：inst|resreg|operand
{
	uint16_t res ;
	int flag = getoperationinfo(root, &res, pref);
	switch(flag)
	{
	case 0:
		return -1;
	case 1:  //表示local
		break ;
	case 2:  //表示shared
		inst++;
		break ;
	case 3:
		releasetempnum(res,pref);
		break ;
	}
	//if(root->s_extra == 1)  //这为默认参数设计的
		//flag = genopercodewithspecres(inst,root->extra);
	//else
	flag = genopercodewithres(inst,pref);
	ADDWORD(res);
	return flag;
}
int gencodewithspecres(token root,int16_t offset,fun pref)   //目的只有一个让指令的最终结果放到指定的寄存器中
{
	int flag;
	uint16_t res;
	//root->s_extra = 1;  //表示自己指定位置
	//root->extra = offset;
	//flag = getoperationinfo(root, &res, pref);
	
	//按照赋值语句的标准来
	if(!genunarycodewithnoresEX(root,_store_local_local,pref))
		return 0;
	ADDWORD(offset);
	/*if(flag != 3)
	{
		flag = genunarycodewithnores(root,_store_local_local,pref);
		ADDWORD(offset);
		if(flag == 0 || flag == -1)
			return 0;
	}*/
	return 1;
}
int genbinarycodewithnores(token root,uchar inst,fun pref)
{
	uint16_t operoffset1,operoffset2;
	int operinfo1 = getoperationinfo(root->first,&operoffset1,pref);
	int operinfo2 = getoperationinfo(root->second,&operoffset2,pref);
	//1/3local 2shared
	switch(operinfo1)
	{
	case 0:
		return 0;
	case 1:
		switch(operinfo2)
		{
		case 0:
			return 0;
		case 1: //local-local
			ADDBYTE(inst+2);
			break ;
		case 2: //local-shared
			ADDBYTE(inst+3);
			break ;
		case 3:
			ADDBYTE(inst+2);
			releasetempnum(operoffset2,pref);
			break ;
		}
		break ;
	case 2:
		switch(operinfo2)
		{
		case 0:
			return 0;
		case 1: //shared-local
			ADDBYTE(inst);
			break ;
		case 2: //shared-shared
			ADDBYTE(inst+1);
			break ;
		case 3:
			ADDBYTE(inst);
			releasetempnum(operoffset2,pref);
			break ;
		}
		break ;
	case 3:  
		switch(operinfo2)
		{
		case 0:
			return 0;
		case 1: //local-local
			ADDBYTE(inst+2);
			break ;
		case 2: //local-shared
			ADDBYTE(inst+3);
			break ;
		case 3:
			ADDBYTE(inst+2);
			releasetempnum(operoffset2,pref);
			break ;
		}
		releasetempnum(operoffset1,pref);
		break ;
	}
	ADDWORD(operoffset1);
	ADDWORD(operoffset2);
	return 1;
}
int genbinarycodewithres(token root,uchar inst,fun pref)
{
	uint16_t operoffset1,operoffset2;
	int res;
	int operinfo1 = getoperationinfo(root->first,&operoffset1,pref);
	int operinfo2 = getoperationinfo(root->second,&operoffset2,pref);
	switch(operinfo1)
	{
	case 0:
		return -1;
	case 1:
		switch(operinfo2)
		{
		case 0:
			return -1;
		case 1: //local-local
			inst += 2;
			break ;
		case 2: //local-shared
			inst += 3;
			break ;
		case 3:
			releasetempnum(operoffset2,pref);
			inst += 2;
			break ;
		}
		break ;
	case 2:
		switch(operinfo2)
		{
		case 0:
			return -1;
		case 1: //shared-local
			break ;
		case 2: //shared-shared
			inst++;
			break ;
		case 3:
			releasetempnum(operoffset2,pref);
			break ;
		}
		break ;
	case 3:  
		releasetempnum(operoffset1,pref);
		switch(operinfo2)
		{
		case 0:
			return -1;
		case 1: //local-local
			inst += 2;
			break ;
		case 2: //local-shared
			inst += 3;
			break ;
		case 3:
			releasetempnum(operoffset2,pref);
			inst += 2;
			break ;
		}
		break ;
	}
	if(root->s_extra == 1)
		res = genopercodewithspecres(inst,root->extra);
	else
		res = genopercodewithres(inst,pref);
	ADDWORD(operoffset1);
	ADDWORD(operoffset2);
	return res;
}
int genbinarycodewithnoresEX(token root,uchar inst,fun pref)
{
	uint16_t operoffset1,operoffset2;
	token first = root->first,second = root->second;
	int operinfo1 = getoperationinfo(first,&operoffset1,pref);
	int operinfo2 = getoperationinfo(second,&operoffset2,pref);
	switch(operinfo1)
	{
	case 0:
		return 0;
	case 1:
		switch(operinfo2)
		{
		case 0:
			return 0;
		case 1: //local-local
			ADDBYTE(inst+3);
			break ;
		case 2: //local-shared
			ADDBYTE(inst+4);
			break ;
		case 3:
			if(ISSTACKCHECKNEEDED(second)) {
				ADDBYTE(inst+5);
			}
			else {
				ADDBYTE(inst+3);
			}
			releasetempnum(operoffset2,pref);
			break ;
		}
		break ;
	case 2:
		switch(operinfo2)
		{
		case 0:
			return 0;
		case 1: //shared-local
			ADDBYTE(inst);
			break ;
		case 2: //shared-shared
			ADDBYTE(inst+1);
			break ;
		case 3://shared_temp
			if(ISSTACKCHECKNEEDED(second)){
				ADDBYTE(inst+2);
			}
			else{
				ADDBYTE(inst);
			}
			releasetempnum(operoffset2,pref);
			break ;
		}
		break ;
	case 3:  
		switch(operinfo2)
		{
		case 0:
			return 0;
		case 1: //local-local
			ADDBYTE(inst+3);
			break ;
		case 2: //local-shared
			ADDBYTE(inst+4);
			break ;
		case 3: //local_temp
			if(ISSTACKCHECKNEEDED(second)){
				ADDBYTE(inst+5);
			}
			else{
				ADDBYTE(inst+3);
			}
			releasetempnum(operoffset2,pref);
			break ;
		}
		releasetempnum(operoffset1,pref);
		break ;
	}
	ADDWORD(operoffset1);
	ADDWORD(operoffset2);
	return 1;
}
int genopercodewithparams(token root,uchar inst,fun pref)
{
	uint16_t operoffset1,operoffset2;
	token first = root->first,second = root->second;
	int operinfo1 = getoperationinfo(first,&operoffset1,pref);
	int operinfo2 = getoperationinfo(second,&operoffset2,pref);

	if(operinfo1 == 2)  //此时表示是exec_environment
	{
		operoffset1 = genunarycodewithres(first,_mov_local,pref);
		if(operoffset1 == -1)
			return 0;

	}
	if(operinfo2 == 2)  //此时表示是exec_environment
	{
		operoffset2 = genunarycodewithres(second,_mov_local,pref);
		if(operoffset2 == -1)
			return 0;
	}
	if(!genopercodewithnores(inst))
		return 0;
	ADDWORD(operoffset1);
	ADDWORD(operoffset2);
	releasetempnum(operoffset2,pref);
	return operoffset1;
}
int gentempunarycode(token index,uchar inst,fun f)  //生成需要对特定运算结果做缓存的字节码，主要为了避免特定中间结果可能会因为gc而造成数据破坏
{
	uint16_t offset2;
	if(index->id == WAVEL)
	{
		//之所以感withnores是WAVEL结果一定是中间变量
		getoperationinfo(index, &offset2, f);
		ADDBYTE(_asc_ref);
		ADDWORD(offset2);
	}
	else
	{
		offset2 = genunarycodewithres(index,inst,f);
	}
	return offset2;
}
int genreleasetempcode(token index,int offset2)
{
	if(index->id == WAVEL)
	{
		ADDBYTE(_dec_ref);
		ADDWORD(offset2);
	}
	return 1;
}

#define ISCOLLECTIONEX(o) (o->id <= SET && o->id >= STR)
int pushdefaultparam(fun f)
{
	token p;
	//处理并传入默认参数
	unsigned long i=f->default_paracount;
	int default_index = f->paracount - f->default_paracount;
	f->base_addr=(unsigned long *)calloc(f->default_paracount+1,sizeof(unsigned long));
	if(!f->default_para)
	{
		f->base_addr[0]=angel_byte->len;
		return 1;
	}
	for(p=f->default_para->first; p; p=p->second)
	{
		f->base_addr[i--]=angel_byte->len;
		if(!gencodewithspecres(p->first,default_index++,f))
			return 0;
	}
	f->base_addr[i]=angel_byte->len;
	/*
	*/
	return 1;
}
int getmembase(token t,fun f,uint16_t *offset_ret)  //这个函数要与indirect_addr分开看待
{
	int flag;
	char *name=(char *)t->attr;
	uint16_t offset;
	if(t->id==NAME)
	{
		offset=getobjoffset(name,f,&flag);  //这里存在一个重复运行的问题，即checkstaticmem和indirect_addr都运行了一遍
		if(flag==4) //classname.的形式
		{
			*offset_ret = offset;
			return 0;
		}
	}
	else if(t->id==CLASS)
		return 1;
	return 2;
}
int definevalue(char *name,fun f)
{
	if(getoffset(keytable,name)>0)
	{
		angel_error("变量名与关键字冲突！");
		return 0;
	}
	if(-1 != getoffset(class_map,name))
	{
		angel_error("变量不能与模块重名！");
		return -1;
	}
	linkcollection head,current;
	if(f)
	{
		head = f->current_scope;
		current = f->local_scope;
	}
	else
	{
		head = global_current_scope;
		current = global_scope;
	}
	if(head != current)
	{
		addscopevalue(name,f);
	}
	else
	{
		addvalue(name,f);
	}
	return 1;
}
int assignment(token t,fun f)  //这里记住赋值之后要将所有用到的寄存器都回收,jibe
{
	object *ov;
	uchar typecode=0,typecode_index = 0;
	char type;
	uint16_t offset,offset1,offset2;
	token first,second,temp,index;
	char *name;
	token exp=t->second,assign=t->first;
	if(exp->id == ASSIG)
	{
		if(!assignment(exp,f))
			return 0;
		exp = exp->first;
	}
assign:
	switch(assign->id)  
	{
	case NAME:  //普通的赋值
		//先申请变量空间
		if(!definevalue((char *)assign->attr,f))
			return 0;
		type = indirect_addr(assign,f,&offset);
		switch(type)
		{
			case 0:
				return 0;
			case 1:
				if(!genunarycodewithnoresEX(exp,_store_local_local,f))
					return 0;
				ADDWORD(offset);
				break ;
			case 2:
				if(!genunarycodewithnoresEX(exp,_store_global_local,f))
					return 0;
				ADDWORD(offset);
				break ;
			case 3:
				if(!genunarycodewithnoresEX(exp,_store_dynamic_local,f))
					return 0;
				adddynamicname((char *)assign->attr);
				break ;
		}
		break ;
	case DOT:
		first=assign->first;
		second = assign->second;
		type = getmembase(first,f,&offset);  //这里主要判断点运算符左边的结果是什么：一般的会有以下几种情况：CLASS、一般变量或运算的中间形式、类名（类名一般都是以变量的形式展示的，一般判断其是否是一般变量，如果不是在看是否是类名）
		if(second->id!=NAME)
		{
			angel_error("赋值对象必须为可变的成员变量！");
			return 0;
		}
		switch(type)
		{
		case 0:
			if(!genunarycodewithnoresEX(exp,_store_static_local,f))
				return 0;
			ADDWORD(offset);
			break ;
		case 1:
			if(!genunarycodewithnoresEX(exp,_store_static_default_local,f))
				return 0;
			break ;
		case 2:
			temp = t->first;
			t->first = first;
			if(!genbinarycodewithnoresEX(t,_store_member_shared_local,f))
				return 0;
			t->first = temp;
			break ;
		}
		//这里是动态的名字，供运行时确定地址
		adddynamicname((char *)second->attr);

		break ;
	case INDEX:
		//基地址的确定
		first = assign->first;
		second = assign->second;
		
		index = second->first;
		
		offset2 = genunarycodewithres(index,_mov_local,f);
		if(offset2 == -1)
			return 0;


		temp = t->first;
		t->first = first;    //基地址
		if(exp->id == INDEX) //表示可能要切片复制
			exp->s_extra = 2;
		if(!genbinarycodewithnoresEX(t,_store_index_shared_local,f))
			return 0;
		ADDWORD(offset2);
		
		//genreleasetempcode(index,offset2);

		releasetempnum(offset2,f);
		t->first = temp;
		break ;
	default:
		angel_error("赋值语句左值是可变对象！");
		return 0;
	}
	return 1;
}
int addscopeparse(fun f)
{
	linkcollection item = initlink();
	item->data = init_perpetual_set();
	linkcollection head;

	if(f)
	{
		head = f->current_scope;
		f->current_scope = item;
	}
	else
	{
		head = global_current_scope;
		global_current_scope = item;
	}
	addlink(head, item);
	return 0;
}
void releasescopeparse(fun f)
{
	linkcollection head,current;
	if(f)
	{
		head = f->local_scope;
		current = f->current_scope;
		if(head == current)
		{
			linkcollection p = head->pre; //最后一个元素
			linkcollection newhead = initlink();
			addlink(p,newhead);

			f->current_scope = newhead;
		}
		else
			f->current_scope = current->pre;
	}
	else
	{
		head = global_scope;
		current = global_current_scope;
		if(head == current)
		{
			linkcollection p = head->pre; //最后一个元素
			linkcollection newhead = initlink();
			addlink(p,newhead);

			global_current_scope = newhead;
		}
		else
			global_current_scope = current->pre;
	}
}

int loadindex(token t,fun f)
{
	uint16_t offset,offset1;
	uchar typecode=0;
	token first = t->first,second = t->second;


	token temp = t->second;
	t->second = second->first;

	int res= genbinarycodewithres(t,_load_index_shared_local,f);
	//表示此时是否是slice
	if(res == -1)
		return 0;
	ADDBYTE(t->s_extra);
	t->extra = res;
	t->second = temp;
	return 1;
}
int loaddot(token t,fun f)
{
	int res,flag;
	uint16_t offset;
	token first=t->first,second = t->second;
	char type = getmembase(first,f,&offset);  //这里主要判断点运算符左边的结果是什么：一般的会有以下几种情况：CLASS、一般变量或运算的中间形式、类名（类名一般都是以变量的形式展示的，一般判断其是否是一般变量，如果不是在看是否是类名）
	
	if(second->id==NAME)
	{
		switch(type)
		{
		case 0:
			res = genopercodewithres(_load_static,f);
			addtwobyte(angel_byte,offset);
			break ;
		case 1:
			res = genopercodewithres(_load_static_default,f);
			break ;
		case 2:
			res = genunarycodewithres(first,_load_member_local,f);
			break ;
		}
		if(res == -1) return 0;
		//这里是动态的名字，供运行时确定地址
		t->extra = res;
		adddynamicname((char *)second->attr);
		return 1;
	}
	else if(second->id==EXECFUN)
	{
		int count=pushparam(t->second->first,f);
		if(count==-1)
			return 0;
		switch(type)
		{
		case 0:
			res = genopercodewithres(_call_static,f);
			addtwobyte(angel_byte,offset);
			break ;
		case 1:
			res = genopercodewithres(_call_static_default,f);
			break ;
		case 2:
			t->first = first;
			res = genunarycodewithres(t->first,_call_member_local,f);
			break ;
		}
		if(res == -1) return 0; 
		adddynamicname((char *)second->attr);
		addtwobyte(angel_byte,count);
		t->extra = res;
		return 1;
	}
	else
	{
		angel_error("点运算符右边的操作数不符合规范！");
		return 0;
	}
	return 1;
}
int loadfunction(token t,fun f)  //现在不支持指定函数参数个数的精确dy_load方式
{
	uchar typecode;
	/*if(t->id == COLON)
	{
		token dot = t->first;
		if(dot->id == DOT)
		{
			typecode = 3;
			if(dot->second->id == EXECFUN) //执行函数就表明$操作符无效
				return loaddot(dot,f);
			if(dot->second->id != NAME)
			{
				angel_error("动态函数获取格式出错！");
				return 0;	
			}
			genopercodewithnores(_dynamic_get_function);
			if(!genbinarycodewithres(t,0,f))  //注意这里面的0就是typcode
				return 0;
			angel_byte->code[angel_byte->len - 1 - 2 * 3] = typecode;
		}
		else if(dot->id == NAME)
		{
			typecode = 2;
			genopercodewithnores(_dynamic_get_function);
			if(!genunarycodewithres(t->second,typecode,f))
				return 0;
			adddynamicname((char *)dot->attr);
		}
		else if(dot->id == EXECFUN)
		{
			if(!parse_ord(dot,f))
				return 0;
		}
		else
		{
			angel_error("动态函数获取格式出错！");
			return 0;
		}
	}
	else
	{*/
	if(t->id == NAME)  //直接
	{
		typecode = 0;
		genopercodewithnores(_dynamic_get_function);
		genopercodewithres(typecode,f);
		adddynamicname((char *)t->attr);
	}
	else if(t->id == DOT)
	{
		typecode = 1;
		if(t->second->id == EXECFUN) //执行函数就表明$操作符无效
			return loaddot(t,f);
		if(t->second->id != NAME)
		{
			angel_error("动态函数获取格式出错！");
			return 0;	
		}
		genopercodewithnores(_dynamic_get_function);
		if(!genunarycodewithres(t,0,f))  //注意这里面的0就是typcode
			return 0;
		angel_byte->code[angel_byte->len - 1 - 2 * 2] = typecode;
	}
	else
	{
		angel_error("动态函数获取格式出错！");
		return 0;
	}
	//}
	return 1;
}
int isaddr(token t)
{
	uint16_t id = t->id;
	switch(id)
	{
	case NAME:
	case INDEX:
		return 1;
	case DOT:
		if(t->first->id == NAME)  //这是成员变量，所以是可变对象
			return 1;
		else
			return 0;
	default:
		return 0;
	}
}

//逻辑运算之所以这样折腾是为了将direct直接视为local，同时不需要布尔转换
int boolcalc_b(uint16_t inst,token t,fun f)
{
	uint16_t offset1,offset2;
	int flag;
	//第一个或第二个已定，通过gencode系列的代码逻辑对inst做算数运算以适应调用
	if(ISLOGICAL(t->first->id))  
	{
		if(ISLOGICAL(t->second->id))   //只要是logical就代表direct
		{
			return genbinarycodewithres(t,inst,f);
		}
		else
		{
			return genbinarycodewithres(t,inst-2,f);

		}
	}
	else
	{ 
		if(ISLOGICAL(t->second->id))  //第一个不确定第二个是direct
		{
			uint16_t offset;
			if(!getoperationinfo(t->second,&offset,f))
				return -1;
			flag = genunarycodewithres(t->first,inst+3,f);
			ADDWORD(offset);
			releasetempnum(offset,f);
			return flag;
		}
		else
			return genbinarycodewithres(t,inst+5,f);
	}
}
int boolcalc_u(uint16_t inst,token t,fun f)  //local、shared、direct
{
	uint16_t offset;
	int flag;
	if(ISLOGICAL(t->id))
	{
		return genunarycodewithres(t,inst+2,f);
	}
	else{
		return genunarycodewithres(t,inst,f);
	}
}
int calccore(token t,fun f)
{
	int res;
	switch(t->id)
	{
	case ADD:
		res = genbinarycodewithres(t,_add_shared_local,f);
		break ;
	case SUB:
		res = genbinarycodewithres(t,_sub_shared_local,f);
		break ;
	case MUL:
		res = genbinarycodewithres(t,_mult_shared_local,f);
		break ;
	case MOD:
		res = genbinarycodewithres(t,_mod_shared_local,f);
		break ;
	case DIV:
		res = genbinarycodewithres(t,_div_shared_local,f);
		break ;
	case RSHIFT:
		res = genbinarycodewithres(t,_rshift_shared_local,f);
		break ;
	case LSHIFT:
		res = genbinarycodewithres(t,_lshift_shared_local,f);
		break ;
	case WAVEL:
		res = genbinarycodewithres(t,_init_range_shared_local,f);
		break ;
	case RANGE_STEP:
		res = genopercodewithparams(t,_range_step,f);
		break ;
	case BIG:
		res = genbinarycodewithres(t,_big_shared_local,f);
		break ;
	case SMALL:
		res = genbinarycodewithres(t,_small_shared_local,f);
		break ;
	case BIGEQUAL:
		res = genbinarycodewithres(t,_big_equal_shared_local,f);
		break ;
	case SMALLEQUAL:
		res = genbinarycodewithres(t,_small_equal_shared_local,f);
		break ;
	case AND:
		res = boolcalc_b(_and_direct_local,t,f);
		break ;
	case OR:
		res = boolcalc_b(_or_direct_local,t,f);
		break ;
	case EQU:
		res = genbinarycodewithres(t,_equal_shared_local,f);
		break ;
	case NOEQU:
		res = genbinarycodewithres(t,_noequal_shared_local,f);
		break ;
	case NO:
		res = boolcalc_u(_not_local,t->second,f);
		break ;
	case IN:
		res = genbinarycodewithres(t,_is_item_shared_local,f);
		break ;
	case BITAND:
		res = genbinarycodewithres(t,_bitwise_and_shared_local,f);
		break ;
	case BITOR:
		res = genbinarycodewithres(t,_bitwise_or_shared_local,f);
		break ;
	case BITXOR:
		res = genbinarycodewithres(t,_bitwise_xor_shared_local,f);
		break ;
	default:
		res = 0;
		angel_error("编译出错！");
	}
	if(res == -1)
		return 0;
	t->extra = res ;
	return 1;
}
int dealwithinplaceleft(token t,fun f)
{
	int type;
	uint16_t offset;
	switch(t->id)
	{
	case NAME:
		type = indirect_addr(t,f,&offset);
		if(type == 3)
		{
			//此时算运算指令
			if(!genopercodewithnores(_loadaddr_dynamic))
				return 0;
			adddynamicname((char *)t->attr);
			return 1;//表示是loadaddr指令开头
		}
		return -1;
	case INDEX:
		t->second = t->second->first;
		if(!genbinarycodewithnores(t,_loadaddr_index_shared_local,f))
			return 0;
		return 1;
	case DOT:
		type = getmembase(t->first,f,&offset);  //这里主要判断点运算符左边的结果是什么：一般的会有以下几种情况：CLASS、一般变量或运算的中间形式、类名（类名一般都是以变量的形式展示的，一般判断其是否是一般变量，如果不是在看是否是类名）
		if(!type)
			return 0;
		switch(type)
		{
		case 0:
			if(!genopercodewithnores(_loadaddr_static))
				return 0;
			addtwobyte(angel_byte,offset);
			break ;
		case 1:
			if(!genopercodewithnores(_loadaddr_static_default))
				return 0;
			break ;
		case 2:
			if(!genunarycodewithnores(t->first,_loadaddr_member_local,f))
				return 0;
			break ;
		}
		adddynamicname((char *)t->second->attr);
		return 1;
	default:
		return 0;
	}
}
int inplace_calc(token t,fun f)
{
	uint16_t offset,offset1;
	token exp=t->second,assign=t->first;
	int type = dealwithinplaceleft(assign,f);
	uchar inst;
	switch(t->id)
	{
	case INPLACEADD:
		inst = _inplace_add_global_local;
		break ;
	case INPLACESUB:
		inst = _inplace_sub_global_local;
		break ;
	case INPLACEMULT:
		inst = _inplace_mult_global_local;
		break ;
	case INPLACEDIV:
		inst = _inplace_div_global_local;
		break ;
	case INPLACEMOD:
		inst = _inplace_mod_global_local;
		break ;
	case INPLACEBITAND:
		inst = _inplace_bitwise_and_global_local;
		break ;
	case INPLACEBITOR:
		inst = _inplace_bitwise_or_global_local;
		break ;
	case INPLACEBITXOR:
		inst = _inplace_bitwise_xor_global_local;
		break ;
	case INPLACELSHIFT:
		inst = _inplace_lshift_global_local;
		break ;
	case INPLACERSHIFT:
		inst = _inplace_rshift_global_local;
		break ;
	}
	if(type > 0) //有loadaddr前缀
	{
		if(!genunarycodewithnores(t->second,inst,f))
			return 0;
	}
	else
	{
		if(!genbinarycodewithnores(t,inst,f))
			return 0;
	}
	return 1;
}
int self_calc(token t,fun f)
{
	uint16_t offset,offset1;
	token exp=t->second,assign=t->first;
	int type;
	uchar inst;
	token parse;
	if(t->id == SELFADD)
	{
		if(t->first->id == NOOPER)
		{
			parse = t->second;
			inst = _self_ladd_local;
		}
		else
		{
			parse = t->first; 
			inst = _self_radd_local;
		}
	}
	else
	{
		if(t->first->id == NOOPER)
		{
			parse = t->second;
			inst = _self_lsub_local;
		}
		else
		{
			parse = t->first;
			inst = _self_rsub_local;
		}
	}
	int flag;
	type = dealwithinplaceleft(parse,f);
	if(type == 0)
	{
		flag = genunarycodewithres(parse,_mov_local,f);
		goto ret; 
	}
	if(type > 0) //有loadaddr前缀
	{
		flag = genopercodewithres(inst,f);
	}
	else
	{
		flag = genunarycodewithres(parse,inst,f);
	}
ret:
	if(flag == -1)
		return 0;
	t->extra = flag;
	return 1;
}
int parse_kernel(token t,fun f)  //解析语法树就是要明确两种不同的节点（即代表两种不同的运算），一个时自顶向底运算一个是自底向顶运算
{
	//注意这里一般双目运算都是将结果存入第一个操作数的寄存器中，单目操作如果操作数不是寄存器或者无操作数则直接令去寄存器
	int typeres,flag,typeres1;
	uchar typecode=0;
	uint16_t offset,offset1;
	token first=t->first,second = t->second;
	
	if(t->id==ITER)  //这个指令是没有任何操作数的，但放在这不和谐以后要改
	{
		int flag = genopercodewithres(_iter,f);
		if(flag == -1)
			return 0;
		ADDWORD(t->extra);
		t->extra = flag;
		t->s_extra = angel_byte->len;
		angel_byte->len += 4;
		return 1;
	}
	else if(t->id == EXECFUN)
		return 1;
	else if(!isoperationdata(t->id))  //这里包含单目运算符包括非和
	{
		int error=1;
		uint16_t offset;

		if(t->id<=SELFSUB && t->id>=ASSIG)  //类赋值操作
		{
			if(t->id == ASSIG)
				return assignment(t,f);
			else if(t->id == SELFADD || t->id == SELFSUB)
				return self_calc(t,f);
			else
				return inplace_calc(t,f);
		}
		else if(t->id==QUESTION)
		{
			//直接编译，不用把他解析成if的语法树的结构
			
		}
		else if(t->id==INDEX)
		{
			return loadindex(t,f);
		}

		/*if(second)
		{
			if(!parse_kernel(second,f))
				return 0;
		}
		if(first)
		{
			if(!parse_kernel(first,f))
				return 0;
		}*/

		//解析语法树中的每一个节点，首先将操作数入栈,下面均是运算指令
		else if(t->id == DOT)  //这里是点运算符的取值编译系统
			return loaddot(t,f);
		else if(t->id == MAKEFUN)
			return loadfunction(t->second,f);
		else if(t->id==COLON) 
			return 1;
		else
			return calccore(t,f);
	}
	return 1;
}
int parse_ord(token t,fun f)  //既可以执行语法树,如果是单个元素则直接返回不执行
{
	token root=t;
	if(!root)
	{
		return 1;
	}
	else
	{
		if(!isoperationdata(root->id))  //表达式不需要运行
		{
			if(!parse_kernel(root,f))
				return 0;
			//清理寄存器
			if(!ISASSIG(root->id))
				releasetempnum(root->extra,f);
		}
		else if(root->id == EXECFUN)
		{
			uint16_t offset;
			if(!getoperationinfo(root,&offset,f))
				return 0;
			releasetempnum(offset,f);
		}
		//如果是操作数则不做任何操作
	}
	return 1;
}
#define CONDITIONCODE if(!ISLOGICAL(condition->first->id)){ flag = genunarycodewithnores(condition->first,_jnp_bool_local,f); if(flag == -1)return 0;}\
						else if(!genunarycodewithnores(condition->first,_jnp,f)) return 0;
int parse_block(token t,fun f)
{
	int scopeindex = addscopeparse(f);
	if(!parse_grammar(t->first,f))
		return 0;
	releasescopeparse(f);
	return 1;
}
int parse_if(token t,fun f)
{
	int scopeindex,flag;
	uchar typecode=0;
	uint16_t offset;
	token condition=t->first,iftoken=condition->second,elsetoken=iftoken->second;
	if(condition->first->id == TRUE)
	{
		if(!parse_block(iftoken,f))
			return 0;
		return 1;
	}
	if(condition->first->id == FALSE)
	{
		if(!elsetoken)
			return 1;
		if(!parse_block(elsetoken,f))
			return 0;
		return 1;
	}
	//至此if语句判断条件要么是全局变量要么是局部变量，要么是直接运算结果
	if(elsetoken)  //如果有else的话只有if子句中会产生jmp指令
	{
		unsigned long ifend,conditionend;
		CONDITIONCODE;
		conditionend = angel_byte->len;
		angel_byte->len += 4;
		
		addscopeparse(f);

		if(!parse_grammar(iftoken->first,f))
			return 0;

		ADDBYTE(_jmp);
		ifend=angel_byte->len;
		angel_byte->len+=4;  //用于if子句执行完后跳出下面的else子句

		//这时添加else的偏移，即上面if条件不满足的情况下
		insert_pc(conditionend,angel_byte->len);  
		
		if(!parse_grammar(elsetoken->first,f))
			return 0;
		
		releasescopeparse(f);
		
		insert_pc(ifend,angel_byte->len);
	}
	else
	{
		unsigned long conditionend;
		CONDITIONCODE;
		conditionend=angel_byte->len;
		angel_byte->len+=4;
		
		if(!parse_block(iftoken,f))
			return 0;
		
		insert_pc(conditionend,angel_byte->len);
	}
	return 1;
}
int parse_loop(token t,fun f)
{
	int scopeindex,flag;
	uchar typecode=0;
	unsigned long bodyoffset,ifend,offsettemp,tranvoffset;
	uint16_t offset;
	token condition,body,temp;

	//表示此时是一个while界点
	brand->head_addr[brand->length++]=0;

	condition=t->first,body=condition->second;

	bodyoffset=angel_byte->len;

	
	temp=maketoken(LOOP,0);
	temp->extra=bodyoffset;
	push(ctrl_state,temp);

	if(condition->first->id == TRUE)
	{
		//这里面直接做相关编译
		if(!parse_block(body,f))
			return 0;
		ADDBYTE(_jmp);
		addfourbyte(angel_byte,bodyoffset);
		goto end;
	}
	else if(condition->first->id == FALSE)
		return 1; //直接就不跟你玩啦
	CONDITIONCODE;

	//后面可以在这个地方插入循环条件不满足时退出的位置
	ifend=angel_byte->len; 
	angel_byte->len+=4;


	//循环体
	if(!parse_block(body,f))
		return 0;


	//循环跳到判断语句
	ADDBYTE(_jmp);
	addfourbyte(angel_byte,bodyoffset);
	insert_pc(ifend,angel_byte->len);
end:
	addbreakoffset(angel_byte->len);
	pop(ctrl_state);
	return 1;
}
int parse_for(token t,fun f)
{
	int scopeindex,flag,itertemp = -1;
	unsigned long bodyoffset,ifend,offsettemp,tranvoffset;
	token init,condition,body,iteration,temp;
	init = t->first;
	brand->head_addr[brand->length++]=0;   //表示此时是一个while界点
	addscopeparse(f);
	//首先编译迭代部分，init，cond，iteration
	if(init->first->id == IN)  //这是表示collection迭代
	{
		//首先iteration应该放在中间变量中
		//迭代只有两步：初始化，和迭代过程，判断退出时在迭代过程中进行的
		token iter = init->first;
		body = init->second;
		//初始化迭代器
		itertemp = genunarycodewithres(iter->second,_init_iter_local,f);
		if(itertemp == -1)
			return 0;

		ADDBYTE(_jmp);
		int initend=angel_byte->len; 
		angel_byte->len+=4;

		iter->id = ASSIG;
		iter->second->id = ITER;
		iter->second->extra = itertemp;
		bodyoffset = angel_byte->len;
		insert_pc(initend,bodyoffset);
		if(!parse_ord(iter,f))
			return 0;
		ifend = iter->second->s_extra;
	}
	else
	{
		condition=init->second,iteration = condition->second;body=iteration->second;

		//初始化语句，一般只有for循环有
		if(!parse_grammar(init->first,f))
			return 0;

		//初始化语句要跳过判断语句
		ADDBYTE(_jmp);
		int initend=angel_byte->len; 
		angel_byte->len+=4;
		
		bodyoffset = angel_byte->len;
		if(!parse_grammar(iteration->first,f))
			return 0;
		insert_pc(initend,angel_byte->len);
		
		CONDITIONCODE;
		//后面可以在这个地方插入循环条件不满足时退出的位置
		ifend=angel_byte->len;
		angel_byte->len+=4;
	}
	temp=maketoken(LOOP,0);
	temp->extra=bodyoffset;
	push(ctrl_state,temp);
	
	//循环体
	if(!parse_grammar(body->first,f))
		return 0;
	releasescopeparse(f);  //break可能出问题了

	//循环跳到判断语句
	ADDBYTE(_jmp);
	addfourbyte(angel_byte,bodyoffset);
	
	insert_pc(ifend,angel_byte->len);

	if(itertemp != -1)
		releasetempnum(itertemp,f);
	addbreakoffset(angel_byte->len);
	pop(ctrl_state);
	return 1;
}
int parse_switch(token t,fun f)
{
	int i,typeres;
	uchar typecode=0;
	uint16_t offset;
	token cases;
	brand->head_addr[brand->length++]=0;   //表示此时是一个while界点

	switchtable swb=_global_sw_list->st_table[t->extra];
	token condition = t->first;

	
	if(!genunarycodewithnores(condition->first,_switch_case_local,f)) return 0;
	addtwobyte(angel_byte,t->extra);

	token temp=maketoken(SWITCH,0);
	push(ctrl_state,temp);


	for(i=0,cases=t->first->second; i<swb->len; i++,cases=cases->second)
	{
		swb->sw_item[i]->offset=angel_byte->len;
		if(!parse_block(cases->first,f))
			return 0;
		if(!swb->sw_item[i]->type) //表示此时是default,要换到最后
		{
			_switch temp=swb->sw_item[i];
			swb->sw_item[i]=swb->sw_item[swb->len-1];
			swb->sw_item[swb->len-1]=temp;
			swb->len--;
		}
	}
	if(!swb->sw_item[swb->len]) //说明有default
	{
		_switch s=(_switch)malloc(sizeof(_switchnode));
		s->offset=angel_byte->len;
		swb->sw_item[swb->len]=s;
	}
	if(!quicksort(swb->sw_item,0,swb->len-1))
	{
		angel_error("case后的常量不能重复！");
		return 0;
	}

	addbreakoffset(angel_byte->len);
	pop(ctrl_state);
	return 1;
}
int parse_break(token t,fun f) //这里传入的t是指循环体的根节点
{
	token loop=gettopctrl();
	int scopelen = (uint16_t)loop->first;
	if(!loop)
	{
		angel_error("break语句必须在loop循环语句或switch-case中！");
		return 0;
	}
	ADDBYTE(_jmp);
	brand->head_addr[brand->length++]=angel_byte->len;
	angel_byte->len+=4;
	return 1;
}
int parse_continue(token t,fun f)  //多个break和continue语句时需要建立一个关于每个所在位置偏移量的数组这样所有操作都可以归结为批量插入指令
{
	token loop=gettopctrl();
	if(!loop || loop && loop->id==SWITCH)
	{
		angel_error("continue语句必须在loop循环语句中！");
		return 0;
	}
	ADDBYTE(_jmp);
	addfourbyte(angel_byte,loop->extra);
	return 1;
}
int parse_ret(token t,fun f)  //目的就是将ret语句运算的结果存放放到,ret的值一律放在结果集中
{
	uint16_t offset;
	int typeres;
	uchar typecode=0;
	if(!f)
	{
		angel_error("ret语句只能用在函数中！");
		return 0;
	}
	else if(f->type == 1)
	{
		char errorinfo[errorsize];
		sprintf(errorinfo,"构造函数%s不能有ret语句",f->name);
		return 0;
	}
	else
	{
		token condition=t->first->first;
		if(!condition){
			ADDBYTE(_ret);
		}
		else 
		{
			if(!genunarycodewithnores(condition,_ret_with_local,f)) 
				return 0;
		}
	}
	return 1;
}
int parse_grammar(token t,fun f)  //其中flag=0代表主程序，1代表函数
{
	token p;
	int error=1;
	for(p=t; p; p=p->second)
	{
		switch(p->id)
		{
		case ORD_SENTEN:
			error=parse_ord(p->first,f);
			break ;
		case IF_SENTEN:
			error=parse_if(p,f);
			break ;
		case LOOP_SENTEN:
			error=parse_loop(p,f);
			break ;
		case FOR_SENTEN:
			error=parse_for(p,f);
			break ;
		case BREAK_SENTEN:
			error=parse_break(p,f);
			break ;
		case CONTINUE_SENTEN:
			error=parse_continue(p,f);
			break ;
		case SWITCH_SENTEN:
			error=parse_switch(p,f);
			break ;
		case RET_SENTEN:
			error=parse_ret(p,f);
			break ;
		}
		if(!error)  //若有一个解析出问题都直接退出。
			return 0;
		token test=p->second;
	}
	return 1;
}
int parse_main(token root)
{
	int flag;
	angel_byte=main_byte;
//	global_value_list->len = global_value_list->len > global_scope->scope_value_num ? global_value_list->len : global_scope->scope_value_num;
	//重置中间变量
	initangeltemp(0); //为了collection_base
	if(root)
		flag=parse_grammar(root->first,0);
	else
		return 0;
	ADDBYTE(_end);
	return flag;
}
int parse_function(fun p)
{
	int count=0;
	int map_init_len=p->local_v_map->len;
	
	//重置中间变量
	initangeltemp(0);

	if(p->type ==1 || p->type == 2)
		addthistofun(p);

	if(!pushdefaultparam(p))
		return 0;

	if(!parse_grammar(p->grammar->first,p))
		return 0;

	p->temp_var_num = angel_temp->inuse;
	//p->localcount += p->temp_var_num; //


    //每个函数执行完后都要有一个ret语句，这是系统添加上的。
	if(p->type==1){
		ADDBYTE(_ret_obj);
	}
	ADDBYTE(_ret_anyway);
	return 1;
}
int parse_functions(funlist fl)
{
	int count;
	fun p;
	for(int i=0; i<fl->len; i++)
	{
		p=fl->fun_item[i];
		if(!parse_function(p))
			return 0;
	}
	return 1;
}
int parse_memfun()
{
	int i;
	for(i=0; i<clist->len; i++)
	{
		fun p;
		funlist fl=clist->c[i]->mem_f;
		//注意这里需要将this指针入栈，然后用
		if(!parse_functions(fl))
			return 0;
		fl=clist->c[i]->static_f;
		if(!parse_functions(fl))
			return 0;
	}
	return 1;
}

int parse_class()
{
	for(int i = 0; i<clist->len; i++)
	{
		if(!genopercodewithnores(_init_class))
			return 0;
		ADDWORD(i);
		pclass pc = clist->c[i];
		if(!parse_grammar(pc->entry_token->first,NULL))
			return 0;
	}
	return 1;
}
int parse_angel(token root)
{
	//这里主要是将语法树解析成中间代码，一般先解析主程序代码在解析函数代码。
	object o;
	angel_temp = (temp_alloc_info)calloc(1,sizeof(temp_alloc_infonode));

	dynamic_name = init_perpetual_list();
	dynamic_name->len++;
	main_byte = initbytearray();
	angel_byte = main_byte;
	brand = (codemap)calloc(1,sizeof(codemapnode));
	ctrl_state = (tstack)calloc(1,sizeof(tstacknode));  //这个控制流栈也可以为未来的变量的作用范围的确定服务
	ctrl_state->top = -1;
	global_scope = initlink();
	global_current_scope = global_scope;

	if(!parse_class())
		return 0;

	if(!parse_main(root))
		return 0;  //能够将最后一个angel_temp信息保存

	temp_alloc_infonode tempinfo;
	tempinfo.inuse = angel_temp->inuse;
	tempinfo.top = angel_temp->top;

	if(!parse_functions(global_function))
		return 0;

	if(!parse_memfun())
		return 0;

	if(!parse_functions(temp_function))
		return 0;

	angel_temp->inuse = tempinfo.inuse;
	angel_temp->top = tempinfo.top;

	mergesharedmemory();//一定要保证global的内存位置始终不变

	extent_const_pool();
	lock_const_sector();  //锁住静态内存
	merge_page();
	return 1;
}

//函数执行要做两件事，一是置参数，二是执行函数
//将复杂的工作转化为简单的计算机规则。
//0号寄存器中存储函数的返回值每次返回时都要向其中写，NULL或返回值