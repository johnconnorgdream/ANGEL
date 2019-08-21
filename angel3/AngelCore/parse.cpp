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


//��ֵҲ���üǺ�����ʾ
//getvaluebytoken�õ����Ǳ�����ֵ�������ã����������ص�����Ϊ����ֵ��
//��Ϊ�����ε���execord������
//�����ڸ�ֵ��Ϊ������һ�ж��Ƿ���ֵ���Ƕ�������
//���������ַ�����Ա�����Ĳ����Ѿ��������ˡ�
//���Խ���ϵͳ���·�Ϊ����������ģ�飬������ģ������Ҫ���õģ��������,���г�Ա��������ͨ�������б�ĵ���������Ƶ�ЧӦ�������ģ��Ҳ��ָ������м��������纯������ֵ������������м�ֵ,
//�������ƺ�������������Ҫע��������Ǳ����������򵥵�˵���ǲ�����ֵʱ��ֵ�����ͱ���ֵ���������Ļ�����ͬ��
//˽�г�Ա�������ƴ�this.^x��ʼ
//���ݶ���ֻ����ʹ�õ�ʱ��ŷŵ�obj_list,����¼ƫ����

/*
�������obj_list��global_value��funlist��switch_table,name_pool����֧��2���ֽ�Ѱַ�����Ҫ�Ļ�δ���ῼ������չ�����ֽڲ�������ǰ���������б�ĳ���û�г���65535
����funlist��Ҫ��ǰ���������صĺ�������һ�𣬺���ӳ�����ֻ������һ������������
����Ҫ������һ���������������ڼ�¼�б����ϣ����ֵ�����ĵ�ǰ״̬

*/

funlist temp_function;  //�����ʾ��ʱ����ĺ���lambda
object_list global_value_list,obj_list,dynamic_name;  //����ȫ�ֱ������б����￼�ǽ�ȫ�ֱ�����ջ����ʽ����   ,����������б�����Ķ���ַ��ƫ�����Ĺ�ϵ
codemap brand; //brand��Ϊѭ���и��ӵ���ת��������
funlist global_function;  //����ָ�������Ԥ�����
classlist clist;
object_set global_value_map,global_function_map,class_map,name_pool;  //�������ֻ�ڱ��뻷�����ã�ִ��ʱ��û�ã������Ժ���˵
; //���ֳ�����Ҫʱ�����洢���ֵ�,ÿ��ӳ�����һ�������Ķ��󣬵��ַ����ǹ����
bytecode main_byte,angel_byte; //�����Ƕ�����һ���洢�ֽ�����ڴ�ռ�,������֧��䣬��֧���Ҫ����洢
tstack ctrl_state;  //��������ִ��״̬������ʾ�㵱ǰ�����ĸ���֧����µ�
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
						sprintf(errorinfo,"����%sδ���壡",name); \
						angel_error(errorinfo); \
						return 0; \
					} 
#define FREEOTSTACK  free(ts); free(os);
#define ISSENTENCEEND(pos) (t[*pos]->id == END || t[*pos]->id == DOUHAO || t[*pos]->id == endid || (!ISKEYWORD(t[*pos]->id)))
#define NOTEND(offset) (offset < tokenlen)
/*

ͨ�ýӿںͽű����Ե�Ԥ����

*/



#define ISASSIG(id) ((id<=INPLACEMOD && id>=ASSIG)) 
int findpivot(_switch *a,int low,int high)
{
	_switch pivotval=a[low];
	while(low<high)
	{
		while(low<high && a[high]->hash>pivotval->hash) 
			high--;
		if(low<high) //���ǿ���������Ҫע��ĵڶ����ط���
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
int quicksort(_switch *a,int low,int high)//��������ĺ�����ÿ��ȷ��һ��λ��
{
	int pivot;
	if(low<high)
	{
		pivot=findpivot(a,low,high);//�ú�����������������ֵ��λ�ã���ȷ���˸�����ֵӦ�ŵ�ֵ
		if(pivot==-1)
			return 0;
		if(!quicksort(a,low,pivot-1))
			return 0;
		if(!quicksort(a,pivot+1,high))
			return 0;//һ��Ҫ��ס������pivot+1����pivot��
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
	return value;  //�д�����
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
		printf("ջ���ˣ�\n");
		return;
	}
	s->t[++s->top]=c;
}
token pop(tstack s)
{
	if(stackempty(s))
	{
		printf("���ʽ�����Ϲ����ִ�г���\n");
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
void deletetoken(token t)   //���ú���������ڵ��ɾ����
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
			angel_error("�ؼ��ʶ����ظ���");
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
int priority(token t)  //�Ժ�ͨ����չ��������Ĺ��������Ӹ�������㣬��ȷ�����ȼ�
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
void addcode(code c,char *s)   //β�巨
{
	code p=(code)malloc(sizeof(codenode));
	p->code=s;
	for(;c->next;c=c->next);  //ע������
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
char* filterspace(char *s)  //�����˵��ִ��ĵ�һ���ַ������ǿո񡣼������ո���Ҫʱ���øù��ˡ�
{
	while(*s==' ' || *s=='\t' || *s=='\n')
		s++;
	return s;
}
int issensitive(char c)  //������Щ����Ҫ�ո����ָ�ĵ���
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
char *getstrvalue(char **s,char endflag)   //�������ַ������Ƶķ���
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
	(*s)++ ; //���˵�һ��'��
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
		(*s)++; //���˵�������־
	return res;
}
char *getnumber(char **s)
{
	char *res;
	int count=0,flag = 1;  //��ʾ������
	while(**s)
	{
		if(**s == '.' && flag == 1) {
			flag = 2; //��ʾ�Ǹ�����
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
	if(issensitive(**s))   //�������ַ�������������ַ���ֱ�������
	{
		SCAN;
		char test = *(*s - 1);
		if(**s == '=')
		{
			if(test=='=' || test=='!' || test=='<' || test=='>'|| test=='&' || test=='|' || test=='^' 
				|| test=='>' || test=='+' || test=='-' || test=='*'|| test=='/' || test=='%') //������Ӧ��Ӵ��ڵ��ں�С�ڵ���
				SCAN
		}
		else if(**s == test)
		{
			if(test == '&' || test == '|'|| test == '+' || test == '-')
				SCAN
			else if(test == '<' || test == '>')
			{
				SCAN
				if(**s == '=')  //��ʾ>>=��<<=
					SCAN
			}
		}
		BUILDSTR
		return res;
	}
	else if(**s=='\'' || **s=='"')  //��Ϊ�ַ��������ĵĵ�һ������
		return getstrvalue(s,**s);
	else if(**s == 'b' && isquota(*((*s)+1)))
		return getstrvalue(s,**s);  //δ������
	else if(isnum(**s))
	{
		char test = *(*s + 1);
		return getnumber(s);
	}
	//���ʲô���дʻ㶼û����ֱ�������ͨ��ʶ�����ɡ�
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
int predeal_s(FILE *f,code c)  //��%import������Դ�ļ�
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
	//�ȴ����һ��
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
		else if(ch=='/')  //��ʾע��
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
			else   //��ʾ������ע��
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
	code c=(code)malloc(sizeof(code)); //����һ��ͷ�ڵ�
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
		else if(iswordinlib(word,&id))  //���úܶ�
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
					if(*ts == '\n')  //����ret�����л��з�
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
			if(isnum(*(word+1)) && (*word == 1 || *word == 2))   //�ж��Ƿ�����������
			{
				if(*word == 1)  //����
				{
					tk->id = INT;//��ID��ʾ������ֵ
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
				tk->id=NAME; //�����ʾ���������������ؼ���
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
	gettoken(p->code);  //���һ��������
	return 1;
}
void copytree(token src,token *des)  //�����ĸ�ֵ���ʽ��
{
	if(src)
	{
		*des=(token)malloc(sizeof(tokennode));   //��Ϊ�����des�϶�ָ���
		memcpy(*des,src,sizeof(tokennode));   //�ٽ����������ÿ�
		copytree(src->first,&(*des)->first);
		copytree(src->second,&(*des)->second);
	}
}
void updatetree(token src,token des) //�ڵ��ôκ���ǰ����ȷ���������ĽṹҪ��ȫ��ͬ��
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
�﷨���Ĵ�����غ���
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
		angel_error("�����������ظ���");
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
		sprintf(errorinfo,"����%s����%s�ظ�����",f->name,valuename);
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
		sprintf(errorinfo,"����%s�ظ�����",name);
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

int detectbracket(int pos,uint16_t bracketid)  //������һ��λ��
{
	tstack s = (tstack)malloc(sizeof(tstacknode));
	int i = pos;
	s->top = -1;
	push(s,t[pos-1]);
    while(1)                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                       	while(!stackempty(s))   //��������ƥ��
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
		angel_error("����һ��}!");
		return -1;
	}
	return 0;
}

#define ISCONST(_id) (_id<=STR && _id>=NU)
#define ISCONSTELE(_id,end) (ISCONST(_id)) && (t[(*pos)+1]->id == DOUHAO || t[(*pos)+1]->id == end)

int addconstelementtolist(token base,int *pos) //�������н����posָ�򶺺ź�]������
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
			(*pos) += 2;  //���˵�����
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
token getindex(int *pos,fun f = NULL)   //��������������ʽ��
{
	token result;
	(*pos)++;
	result=getsentencetree(pos,f,MBRACKETR);          //ֱ������
	if(result->first->id == COLON)
	{
		result->first->id = RANGE_STEP;
	}
	t[*pos]->id=SUBITEM;
	(*pos)++;
	return result;  //֧����Ƭ����
}
token getarraytoken(int *pos,fun f)   //������������鶨���]���Ÿ����ε�,�����б������õĶ�����Բ��÷��ڶ�����У���Ϊ�����б����ã�
	//����Ҫ����ƫ�������з��ʣ�������Ҫ���������ݶ����ֵ�����ü�����2��Ϊ��Щ���������ǲ��ܹ����ͷŵ�
{
	int flag=-1;
	token ret = maketoken(LIST,NULL);
	int hasexp=0;
	token test;
	(*pos)++;

	if(t[*pos]->id == END)
	{
		ret->extra = 8;
		//ret->attr = initarray();  //�ռ���
		goto parsearrayend;
	}
	while(1)
	{
		//һ�ν���֮��������������Ҫô�ڶ����ϣ�Ҫô�����һ����β����
		if(ISCONSTELE(t[*pos]->id,MBRACKETR))
		{
			flag = addconstelementtolist(ret,pos); //�������н����posָ�򶺺ź�]������
		}
		else
		{
			addtoken(ret,getsentencetree(pos,f,MBRACKETR));
			hasexp = 1;
			if(t[(*pos)]->id == MBRACKETR)  //��ʾ�Ѿ���]��������������
				break ;
			ret->extra ++;
		}
		if(t[*pos]->id==DOUHAO)
			(*pos) ++;  //���˵�����
	}
	if(flag!=-1) //˵������
	{
error:
		angel_error("���鶨��ȱһ��']'\n");
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
	if(t[*pos]->id == END)  //��ʾΪ��
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
			(*pos) ++;  //���˵�����
		if(test->id == BBRACKETR)  //��ʾ�Ѿ���]��������������
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
			if(t[(*pos)]->id == BBRACKETR)  //��ʾ�Ѿ���]��������������
				break ;
		}
		else
		{
			break ;
		}
		
		if(test->id==DOUHAO)
			(*pos) ++;  //���˵�����
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

�ڴ����﷨���Ĺ����оͿ�ʼ��װ��ֵ

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
		angel_error("����������ʽ����");
		return 0;
	}
	if(error==-2)
	{
		angel_error("���������б����һ����");
		return 0;
	}
	return error;
}
int getparameter(fun f,int *i)  //�����pref��һ�㺯���п϶�Ϊ���ڳ�Ա�����п���������
{
	int flag=0,error; //��ʾ��ʱ��û��Ĭ�ϲ���
	while(*i<tokenlen)   //��������������������У������Ȳ�����ʵ��Ĭ�ϲ������ơ�
	{
		if(t[(*i)]->id==NAME)
		{
			if(flag && t[(*i)+1]->id!=ASSIG)
			{
				angel_error("������Ĭ�ϲ���Ӧ�÷�������壡");
				return 0;
			}
			f->paracount++;  //ֻҪ��¼��������ĸ��������ˣ�
			addlocalvalue(f,(char *)t[*i]->attr);
		}
		else if(t[(*i)]->id==ASSIG)  //������Ĭ�ϲ���
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
			angel_error("��������ֻ��Ϊ��ͨ��ʶ����");
			return 0;
		}
		(*i)++;
	}
	(*i)++;   //ȥ�������}
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

	fun f=initfuncontrol(funname);  //i����������
	if(!f)
		return NULL;
	f->type = flag;
	//��f�趨���ͣ�����ʼ��head��
	if(f->name && strcmp(f->name,"this")==0)  //����϶������Ա
	{
		//��ʾ��ʱ�ǹ��캯��
		if(!global_class_env)
		{
			angel_error("ȫ�ֺ�����������Ϊthis");
			return NULL;
		}
		f->type = 1; //����
	}

	if(global_class_env)
	{
		f->class_context = global_class_env;
		if(f->type == 2)
		{
			head = global_class_env->mem_f;
			fmap = global_class_env->mem_f_map;
		}
		else  //�������this����
		{
			head = global_class_env->static_f;
			fmap = global_class_env->static_f_map;
		}
	}
	else  //��ʾ������һ�㺯���Ķ��崦
	{
		head = global_function;
		fmap = global_function_map;
	}


	if(t[(*i)++]->id==LBRACKETL)   //ȡ������
	{
		if(!getparameter(f,i))  //ȡ����ʧ��
			return NULL;
	}
	if(t[*i]->id==BBRACKETL)
	{
		(*i)++; //����
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

int dealfunccall(token ft,int *pos,fun f)  //���سɹ���ʧ��
{
	
	//ÿ�ν�����֮��Ҫ�������ɫ�ŵ����һ��������token��
	(*pos) += 2;  //���˵��������͵�һ��С����
	while(t[*pos-1]->id!=END)   
	{
		addtoken(ft,getsentencetree(pos,f));   //������������ʱ���Զ�������˵�
		if(t[*pos-1]->id == END )      //��Ϊ�������������
		{
			t[*pos-1]->id = EXECFUN; //Ϊ�����������������ʽ��ʶ���ṩ��ݡ�
			break ;
		}
		(*pos)++;  //���˵����ţ���ٵĽ��������ڹ����﷨����ʱ������˵���,������Ҫ��ѭ�����������ж���ٽ�������
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
		if(f->type == 1)  //��ʾ��ʱ��
			fun_flag = 2;
	}
	else
	{
		if(global_class_env)
		{
			fun_flag = 3;
		}
	}

	fun deff = dealfuncdef(i,fun_flag);  //��ʱ�Ѿ��������Ÿ����˵��ˣ���Ϊ�����﷨��ʱÿ�ζ�Ҫ���˵���������������ٵĽ�������
	if(!deff)
	{
		angel_error("�����������");
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
	if(t[next] && t[next]->id == BBRACKETL)  //��ʾ�����ĵĶ���
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
token gettree(int *pos,int *error,int endid,fun f)   //���Խ��㿴��������������﷨�������������д�����������ף��������쳣��Ϣ����ִ�������
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
			angel_error("ȱ��һ����������\n"); \
			return NULL; \
		} \
		else \
			temp->first=pop(ts);\
	}\
	push(ts,temp);

#define DEALOPER if(ispreoper == 1) break ; ispreoper = 1;
#define ISKEYWORD(id) ((id) < IF || (id) > RET)

	int ispreoper=0,isglobal=0;

	while(!ISSENTENCEEND(pos))  //��������˻�ȡ���ʽ�����ԷֺŽ���Ҳ�����Զ��Ž�����
	{
		//������ķ�֧���������Ǻ��ģ�������Ҫ������ΪΪ��׼�ֵ�
		if(t[*pos]->id==NAME || t[*pos]->id==TRUE || t[*pos]->id==FALSE ||(t[*pos]->id<= BOOLEAN && t[*pos]->id>= NU)|| t[*pos]->id==CLASS || t[*pos]->id==GLOBAL || t[*pos]->id==ORD_SENTEN || t[*pos]->id==GRAMROOT)  //������Ҫ��չ�ַ����Ĳ��������ַ���Ŀǰֻ��+��*��������������֣�
		{
			//������ʾ����
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
						if(!deff)//��ʱ�Ѿ��������Ÿ����˵��ˣ���Ϊ�����﷨��ʱÿ�ζ�Ҫ���˵���������������ٵĽ�������
						{
							angel_error("�����������");
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
						if(!dealfunccall(ft,pos,f))  //��ʱ�Ѿ��������Ÿ����˵��ˣ���Ϊ�����﷨��ʱÿ�ζ�Ҫ���˵���������������ٵĽ�������
						{
							angel_error("���ú���ʱ������ʽ����");
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
					if(strcmp(name,"this")==0)  //������thisָ��,�ں������е�ʱ���ڶ�̬�Ľ��丳ֵΪÿ����������ã��������ȵö�̬����һ��this����
					{
						if(!f || !global_class_env)
						{
							angel_error("this������������ڳ�Ա�����д����У�");
							goto reterror;
						}
						if(t[*pos+1]->id==ASSIG)
						{
							angel_error("thisָ���ǲ���д�ģ�");
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
					angel_error("class�ؼ��ֱ��붨�������У�");
					goto reterror;
				}
				if(t[*pos+1]->id!=DOT)
				{
					angel_error("class�ؼ��ֲ��ɶ�д��");
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
			if(ispreoper == 0)  //ǰ�治�ǲ�����������ֻ����list
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
			else   //ǰ���ǲ������������һ������������
			{
				t[--(*pos)]=maketoken(INDEX,0);  //�������������,����POS�ڵ���һ��
			}
		}
		else if(t[*pos]->id == BBRACKETL)  //˵�������ֵ�ĳ�ʼ������
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
		else  //��������ջ�����
		{
newbeg:
			int isright=0;
			if(!ispreoper)  //����һ�ζ�����������
			{
				if(t[*pos]->id == DOT)  //��ʾ��ʱ��ȫ�ֵ���
				{
					(*pos)++;
					if(t[*pos]->id!=NAME)
					{
						angel_error("��������ұ߱���Ϊ�����������ã�");
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
			if(stackempty(os)  ||  priority(t[*pos]) > priority(gettop(os)) || t[*pos]->id==LBRACKETL || t[*pos]->id==ASSIG && gettop(os)->id==ASSIG)  //������ϸ�ֵ������Ҫ��һЩ�ı�
			{
				if(t[*pos]->id==LBRACKETL)  //������lambada����
				{
					if(islambda((*pos)+1))  //�϶��Ǻ���������
					{
						DEALOPER;
						goto fundef;
					}

				}
				if(stackempty(os) && t[*pos]->id==LBRACKETR)  //����������Ų�ƥ�䣬�����Ŷ�һ��
				{
					angel_error("���һ��(����");
					goto reterror;
				}
				push(os,t[(*pos)++]);
			}
			else
			{
				if(priority(t[*pos])<=priority(gettop(os)) || t[*pos]->id==LBRACKETR && !stackempty(os) && gettop(os)->id!=LBRACKETL)
				{
					if(t[*pos]->id==LBRACKETR && !stackempty(os) && gettop(os)->id==LBRACKETL) //�������ŵ���
					{
						pop(os);
						(*pos)++;
						goto checktokenend ;
					}
					else
					{
						//���ﲻ�ܽ�ispreoper��Ϊ0����Ϊ��������Һ���Ĳ���
						GETOPERSUBTREE;
						goto newbeg;
					}
				}
				else if(t[*pos]->id==LBRACKETR && stackempty(os))  //���������ջ���϶�Ϊ������,�����룩������
				{
					angel_error("���һ��)����");
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

	if(t[*pos]->id == END)    //���Ų��ܱ�����
	{
		
		if(t[(*pos)-1]->id!=DOUHAO)
		{
			(*pos)++; //���˵�������
		}
		else
		{
			angel_error("���Ų����ڽ�������\n");
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
token getsentencetree(int *pos,fun f,int endid)      //ִ�б��ʽ�������ʽ���Ľ�����ӵĳɷּ��Ǹ�������������ȼ������г���ջ������������Ҫ�������Ϊ���ࣺջ�����������Ϊ���ŵ�һ���������ջ��Ϊ�������벻Ϊ���š�ջ����Ϊ��������Ϊ���š�ջ�������붼Ϊ����
{
	//����ģ�ͣ��ڴ�����ʽ���ȼ�ʱ��Ϊ��Ҫ
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
    while(!stackempty(s))                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                       	while(!stackempty(s))   //��������ƥ��
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

token getexptree(int *pos,fun f=NULL)    //�������ʽ��,��ʱ�����ָ�����������
{
	(*pos)++;
	if(dealforbracket(*pos) < 0)
		return NULL;
	return getsentencetree(pos,f);   //�����ջ�ﲻֹһ��������Ե�һ�����Ϊ׼��
}

int getvaluewithscopeenv(char *valuename,fun f,uint16_t *offset,int isglobal = 0);
void addscopevalue(char *name,fun f)
{
	uint16_t offset;
	object_set ml;
	if(getvaluewithscopeenv(name,f,&offset))  //��Ҫ�޸�
	{
		//��ʾ��ʱ
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
		scopep->extra += parallelold; //�����ǵ�����scopet�ӿ������Ŀռ�
	
	*parallel = *parallel< scopet->extra ? scopet->extra : *parallel;   //���ﶼ����ͬ����
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
token dealtoken(int flag,int *i,fun f)  //��������������﷨�������﷨����
{ 
//scope_parallel_num�ǿ�������еľֲ�������ˮƽ�ռ䣬��ÿ��token��ŵ��Ǵ�ֱ�ռ䣬ÿ����ֱ�ռ�ʵݸ���һ����ˮƽ�ռ�	
//#define SET_SCOPE_VALUE_INDEX_BEGIN(st) if(scopet) st->s_extra = scopet->s_extra; else st->s_extra = -1;
#define ISPARSEASLIB (!global_class_env && !f && islib == 1)
	token root=(token)malloc(sizeof(tokennode));  //�ܵ��﷨���ڵ�
	memset(root,0,sizeof(tokennode));
	root->id=GRAMROOT;   //�����﷨���ڵ�
	int scope_parallel_num = 0;
	if((*i) >= tokenlen) {
		return root;
	}
	token temp=t[(*i)++];
	int declare=0;
	while(1)  //�ܵ�ѭ��
	{
		//����ʵ��ÿ�����Ľ���,ÿ����֧�����ָ�������һ��ģ�������λ��

		if(temp->id == LIB)
		{
			islib = 1;
		}
		else if(temp->id == MAIN)
		{
			islib = 0;
		}
		else if(temp->id == ORD_SENTEN) //��ʾԤ�����
		{
			addsetencetoroot(root,temp,f);
		}
		else if(temp->id==NAME && (NOTEND(*i) && t[*i]->id==LBRACKETL))  //���������ʱ�Ѿ��Ǻ����������Ǻ������û��ߺ����Ķ���
		{
			(*i)--;
			int util;
			int funflag = isfunctiondef((*i)+2,&util);

			if(funflag == -1)
				return 0;

			if(funflag == 1)  //��ʾ�����ĵĶ���
			{
				if(!definefunction(i,f))
					return NULL;
			}
			else
			{
				temp->extra = 1;  //��ʾ�Ѿ�check����
				if(ISPARSEASLIB)
				{
					*i = util;
					continue ;
				}
				else
					goto getexptree; //��ʾ���ú���
			}
		}
		else if(temp->id == CLASS && !global_class_env)
		{
			if(t[(*i)]->id != NAME)
			{
				angel_error("class�ؼ���ֻ��������ʹ�ã�");
				return NULL;
			}
			char *classname = (char *)t[(*i)++]->attr;
			if(!addclass(classname))
			{
				angel_error("�����ʧ�ܣ�");
				return 0;
			}
			if(t[(*i)]->id != BBRACKETL)  //�����ʾ���������࣬δ������Ҫ�����µĶ���������̳У�
			{
				angel_error("�ඨ��ʧ�ܣ�");
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
					angel_error("switch������");
					return NULL;
				}
				token switchtoken,temp;
				switchtoken=maketoken(SWITCH_SENTEN,NULL);
				temp=getexptree(i,f);  //switch����������
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
								else if(t[*i]->id==INT)  //������Ҫ��װ��ֵ
								{
									st->hash = GETINT(t[*i]->attr);
									st->type=INT;
								}
								else
								{
									angel_error("case���������ֻ��ַ�������");
									return NULL;
								}
								(*i)++;
							}
							else
							{
								if(isdefault)  //�Ѿ�����һ��default
								{
									angel_error("һ��switch-case���ֻ����һ��default������");
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
								angel_error("case������Ӧ��ð�ţ�");
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
					if(!isdefault && st_table->len>=tablelen)  //���û��default��Ԥ��һ�������ں���׵�ַ
						st_table->sw_item=(_switch *)realloc(st_table->sw_item,(tablelen+1)*sizeof(_switchnode));
					switchtoken->extra=_global_sw_list->len;   //��ʾ
					_global_sw_list->st_table[_global_sw_list->len++]=st_table;
				}
				else
				{
					angel_error("switch����Ҫ��һ��������ţ�");
					return NULL;
				}
				addsetencetoroot(root,switchtoken,f);
			}
			else if(temp->id==IF)  //if���
			{
				if(t[*i]->id!=LBRACKETL)
				{
					angel_error("if������");
					return NULL;
				}
				token iftoken,temp;
				iftoken = maketoken(IF_SENTEN,0);
				temp=getexptree(i,f);
				if(!temp->first)
				{
					angel_error("if�����������Ϊ�գ�");
					return NULL;
				}
				addtoken(iftoken,temp);
				//��ʱ�е���iftoken��û��maplist��˵����ʱ����Ҫ�����ڴ�
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
						if(t[++(*i)]->id!=BBRACKETL)  //���ǹ��˵�else�ؼ���
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
					angel_error("while�����������Ϊ�գ�");
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
					angel_error("forѭ����һ��)");
					return NULL;
				}
				if(error==-2)
				{
					angel_error("forѭ����һ����");
					return NULL;
				}
				token looptoken,temp,selfadd,scope_in;
				looptoken=maketoken(FOR_SENTEN,0);
			
				temp=dealtoken(4,i,f);

				if(!temp) return NULL;

				token tmp = temp->first;
				if(!tmp->first)
				{
					angel_error("for�����������Ϊ�գ�");
					return NULL;
				}
				if(tmp->first->id == IN)  //��ʾ��ʱ�Ǳ��������
				{
					addtoken(looptoken,tmp);
				}
				else  //��ʱ������for���
				{
					//�ܷ�֧��for(;;;) for() ,for���������������;
					//����Ҳ�����ÿո����,���Ը��������
					//��Ҫ���߼����ÿո��;�ȷֿ�ÿ����䣬����ǰ�����ͺã�������ľͲ������ˣ�����ճ����ˣ���Ĭ��û�ж�Ӧ����䣬���ǵڶ���������Ϊtrue
					addtoken(looptoken,temp);
					if(error == (*i)-1)  //��ʾforѭ���о�һ�����
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
			else if(temp->id==BREAK || temp->id==CONTINUE)   //��ʾѭ��������䡣
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
						angel_error("���캯��this�в�����ret��䣡");
						return NULL;
					}
				}
				else if(flag == 6)
				{
					angel_error("class��ʼ�������в�����ret��䣡");
					return NULL;
				}
				token rettoken=maketoken(RET_SENTEN,0),temp;
				temp=getsentencetree(i,f,BBRACKETR);
				if(!temp)
					return NULL;
				addtoken(rettoken,temp);
				addsetencetoroot(root,rettoken,f);
			}
			else if(temp->id==DOUHAO)  //�������
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
 			else   //��ʱһ�㸳ֵ����ʽ���,�������ֻ����������varʱ����Ч������֮��ָ��ͻ����
			{
				(*i)--;//��Ϊÿһ�����е�����ǰ��Ҫִ��һ�Σ�*i��++
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
						sprintf(errorinfo,"��%s��Ա����%s����ȱ��}��",global_class_env->name,f->name);
					else
						sprintf(errorinfo,"��%s����ȱ��}��",global_class_env->name);
				}
				else
				{
					if(f)
						sprintf(errorinfo,"����%s����ȱ��}��",f->name);
					else
						sprintf(errorinfo,"����ȱ��}��");
				}
				angel_error(errorinfo);
				return NULL;
			}
			break ;
		}
		if(flag == 4 || flag == 1)  //����Ƕ�������Ҫ��
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
}  //����������������
void init()   //���е����ٺͳ�ʼ�����붼������
{
	obj_list=init_perpetual_list();  //����������б���������������ֻ����
	global_value_list=init_perpetual_list();  //����ȫ�ֱ������б�
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
token grammartree_entry()    //�ܳ�������,���������﷨��
{
	int point=0;
	token grammarroot;
	//����������ط����һЩ���õĶ���ͷ����������ַ������б�
	if(tokenlen>0)
		grammarroot=dealtoken(0,&point,NULL); //0��ʾ������1��ʾ������䣬2��ʾ�������飬3��ʾswitch������4��ʾǿ�ƽ�����5��ʾwhile���飬6���������Ķ���
	else
		grammarroot=NULL;

	//���º���������
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

�ֽ������ɹ���

���еı��ʽ����ԭ����˵��Ӧ�ý����д��Ĵ���1

//ȫ�ֱ�������ʱ��¼��list�У��ֲ���������ʱ��¼��map�У���Ϊ��ʱ��û�з���ռ�


���뻷���Ժ���Ҫ�ٱ�ʲô����ʱ���������⣬��ֱ�Ӳ����ֽ��룬����ʱ�����н�������ʱ�ű���ʱֱ�ӽ����д�����к�������������ʱֻ���������

��������Ҫ����һ���ֽ��뷶Χ��Դ�����ı��Ķ�Ӧ��


δ�����е�ָ�����Ҫֱ�ӱ���������ڵ㣬�������ڵ�һ�㶼ֱ����������Ѱַ��ֻ����˫Ԫ��������²���Ҫ������һ���ŵ��Ĵ���
*/
//ISASSIG����̽���Ƿ�Ϊ�߼������bool����������һ��Ҫ��translate����ת��
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
	angel_temp->inuse = initval;  //������ʱ�Ķ�
}
void releasetempnum(int num,fun f)
{
	angel_temp->stack[angel_temp->top++] = num;
}
int gettempnum(fun f)
{
	if(angel_temp->top == 0)  //˵����ʱû�л��յļĴ���
	{
		if(angel_temp->inuse > temp_max_size)
		{
			angel_error("�м�������������");
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
	uint16_t lib[]={INT};   //���������Ҫ��������
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
	angel_byte->len=offset;   //��ԭ
}
void addbreakoffset(unsigned long addr)
{
	int i;
	for(i=brand->length-1; brand->head_addr[i]; i--)
		insert_pc(brand->head_addr[i],addr);
	brand->length = i;
}
int parse_kernel(token t,fun);  //���������flagֻ��Ӱ���Ա�����Ļ�ȡ�����һ��������ʾ���صļĴ�����
int parse_ord(token t,fun f);  //�����int *�����Ƿ���һ�����ִ�н��֮��Ľ���Ĵ����ţ�һ�����ⲿ����ֱ�ӵ��øú�������parse_kernel����
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

int genopercodewithnores(uchar inst)  //���ص���������������+1
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
		if(parse->id == LIST && !parse->first)  //��p->first��ʾ��Ԫ����Ҫ��̬����
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
		if(EXTRA_FIELD == 0)   //˵����û�з��뵽obj_list�С�
		{
			EXTRA_FIELD = obj_list->len;
			addlist(obj_list,o);
		}
		offset = EXTRA_FIELD;
		break ;
	case STR:
		stest=(object_string)t->attr;
		if(stest->extra == 0)   //˵����û�з��뵽obj_list�С�
		{
			stest->extra = obj_list->len;
			addlist(obj_list,(object)stest);
		}
		offset = stest->extra;
		break ;
	case LIST:
		if(!t->attr)  //ע�������������set��dict������ͬ
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
uint16_t getvalueoffset_s(char *valuename,fun f,int *flag)  //������������
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
		if(f->type != 0) //��ʾ����Ϊ���캯������Ϊ��Ա����
		{
			*flag = 3;  //��Ҫ��̬ȷ������λ�á�
			return 0;//������
		}
		error = getoffset(global_value_map,valuename);
		if(error != -1)
		{
			*flag=2;  //��ʾ��ʱ�Ǻ�������ȫ�ֱ���
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
			sprintf(errorinfo,"����%s�б���%sδ����",f->name,valuename);
		else
			sprintf(errorinfo,"����%sδ����",valuename);
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
			return 0; //������
		if(f)
			sprintf(errorinfo,"����%s�б���%sδ����",f->name,valuename);
		else
			sprintf(errorinfo,"����%sδ����",valuename);
		angel_error(errorinfo);
	}
	else
		return offset;
}
int indirect_addr(token t,fun f,uint16_t *offset_ret)  //���ﶼ��ֱ��Ĭ����ȫ�ֱ���
{
	int error;
	uint16_t offset;
	char code=0;
	char *valuename=(char *)t->attr;


	if(t->extra==GLOBAL) //��ʾ��ȫ�ֱ���
	{
		offset=getvalueoffset(valuename,NULL,&error);
		if(error != 0) //�������valuename�ҵ��ˡ�
			error=2;
	}
	else
		offset=getvalueoffset(valuename,f,&error);
	*offset_ret = offset;
	return error;
}

/*
ָ������ν�Ĳ�����

*/
void mergesharedmemory()
{
	//����ȷ�������ľ�̬�ڴ涼Ӧ����ϵͳ���룬���������˲���Ҫ�Ĵ���
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
	//push������ʱ��ֱ�Ӳ���ջ��ָ�룬��Ϊջ��ָ������žֲ������򣬵�ֵ��ע����ǵ����еĲ���ָ�������������õ�ǰջ�ײ�����ǰ��ջ����ȫ�ֻ���ַ
	int count=0;
	token p;
	for(p=t; p; p=p->second)
	{
		if(!genunarycodewithnores(p->first,_push_local,pref))
			return 0;
		count ++; //�����Ǽ��㴫������ĸ���
	}
	return count;
}
int function_call(token t,fun pref)  //����mem_call���Ĳ�֮ͬһ������һ��ԭ�Ӳ���
{
	fun f;
	uint16_t offset,count=0,i,defaultindex;
	bytecode temp;
	token p;
	char *funname=(char *)t->attr;
	int error,callnum=-1;
	//������Ĳ���������Ĭ�ϲ���������Ժ�����취
	//����������nop���棬��Ϊ��֪���ú����Ƿ��ǳ�Ա����
	
	//����call_d�ķ�ʽ���Գ�����exec_enviromnet-1����ʽ�õ�this������Ϊ�������ϸ�ķ�ʽ����֤this��ջ��λ��
	count=pushparam(t->first,pref);
	if(count == -1)
		return 0;

	if(pref && t->extra!=GLOBAL)  //������Ҫ����һ��token�б�������ĵ�������Ա�ʵ�ֶ�̬���룬�����ܵļ����ֽ���ĸ���
	{
		if(pref->type!=0) //��������⺯��
		{
			t->extra = genopercodewithres(_call_default,pref);
			adddynamicname(funname);
			addtwobyte(angel_byte,count); //����һ�����ն����Ϊ��̬����
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
		//���ûص�����
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

int genopercodewithres(uchar inst,fun f)  //���ص���������������+1
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
int getoperationinfo(token root,uint16_t *offsetres,fun pref)  //���أ�1Ϊlocal��2Ϊshared��3Ϊregnum
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
				//��ʱ������ָ��
				*offsetres = genopercodewithres(_load_dynamic,pref);
				if(*offsetres == -1)
					return 0;
				adddynamicname((char *)t->attr);
			}
			return type;
		}
		else if(t->id==EXECFUN)
		{
			//��Ϊ����ָ��
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
		if(ISASSIG(t->id))  //����û�з���ֵ
		{
			return getoperationinfo(t->first,offsetres,pref);
		}
		*offsetres = t->extra;
		return 3;
	}
}

/*
�������ɺ���ϵ��
*/
#define ISARITHMETIC(id) (id >= ADD && id <= MOD)
#define ISFUNCALL(t) (t->id==EXECFUN || (t->id == DOT && t->second->id == EXECFUN)) 
#define ISSTACKCHECKNEEDED(t) (ISARITHMETIC(t->id) || ISFUNCALL(t) || t->id == INDEX)
int genunarycodewithnores(token root,uchar inst,fun pref)  //����ָ�����磺inst|operand
{
	uint16_t res ;
	int flag = getoperationinfo(root, &res, pref);
	switch(flag)
	{
	case 0:
		return 0;
	case 1:  //��ʾlocal
		ADDBYTE(inst);
		break ;
	case 2:  //��ʾshared
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
int genunarycodewithnoresEX(token root,uchar inst,fun pref)  //����ָ�����磺inst|operand
{
	uint16_t res ;
	int flag = getoperationinfo(root, &res, pref);
	switch(flag)
	{
	case 0:
		return 0;
	case 1:  //��ʾlocal
		ADDBYTE(inst);
		break ;
	case 2:  //��ʾshared
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
int genunarycodewithres(token root,uchar inst,fun pref) //����ָ�����磺inst|resreg|operand
{
	uint16_t res ;
	int flag = getoperationinfo(root, &res, pref);
	switch(flag)
	{
	case 0:
		return -1;
	case 1:  //��ʾlocal
		break ;
	case 2:  //��ʾshared
		inst++;
		break ;
	case 3:
		releasetempnum(res,pref);
		break ;
	}
	//if(root->s_extra == 1)  //��ΪĬ�ϲ�����Ƶ�
		//flag = genopercodewithspecres(inst,root->extra);
	//else
	flag = genopercodewithres(inst,pref);
	ADDWORD(res);
	return flag;
}
int gencodewithspecres(token root,int16_t offset,fun pref)   //Ŀ��ֻ��һ����ָ������ս���ŵ�ָ���ļĴ�����
{
	int flag;
	uint16_t res;
	//root->s_extra = 1;  //��ʾ�Լ�ָ��λ��
	//root->extra = offset;
	//flag = getoperationinfo(root, &res, pref);
	
	//���ո�ֵ���ı�׼��
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

	if(operinfo1 == 2)  //��ʱ��ʾ��exec_environment
	{
		operoffset1 = genunarycodewithres(first,_mov_local,pref);
		if(operoffset1 == -1)
			return 0;

	}
	if(operinfo2 == 2)  //��ʱ��ʾ��exec_environment
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
int gentempunarycode(token index,uchar inst,fun f)  //������Ҫ���ض���������������ֽ��룬��ҪΪ�˱����ض��м������ܻ���Ϊgc����������ƻ�
{
	uint16_t offset2;
	if(index->id == WAVEL)
	{
		//֮���Ը�withnores��WAVEL���һ�����м����
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
	//��������Ĭ�ϲ���
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
int getmembase(token t,fun f,uint16_t *offset_ret)  //�������Ҫ��indirect_addr�ֿ�����
{
	int flag;
	char *name=(char *)t->attr;
	uint16_t offset;
	if(t->id==NAME)
	{
		offset=getobjoffset(name,f,&flag);  //�������һ���ظ����е����⣬��checkstaticmem��indirect_addr��������һ��
		if(flag==4) //classname.����ʽ
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
		angel_error("��������ؼ��ֳ�ͻ��");
		return 0;
	}
	if(-1 != getoffset(class_map,name))
	{
		angel_error("����������ģ��������");
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
int assignment(token t,fun f)  //�����ס��ֵ֮��Ҫ�������õ��ļĴ���������,jibe
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
	case NAME:  //��ͨ�ĸ�ֵ
		//����������ռ�
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
		type = getmembase(first,f,&offset);  //������Ҫ�жϵ��������ߵĽ����ʲô��һ��Ļ������¼��������CLASS��һ�������������м���ʽ������������һ�㶼���Ա�������ʽչʾ�ģ�һ���ж����Ƿ���һ���������������ڿ��Ƿ���������
		if(second->id!=NAME)
		{
			angel_error("��ֵ�������Ϊ�ɱ�ĳ�Ա������");
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
		//�����Ƕ�̬�����֣�������ʱȷ����ַ
		adddynamicname((char *)second->attr);

		break ;
	case INDEX:
		//����ַ��ȷ��
		first = assign->first;
		second = assign->second;
		
		index = second->first;
		
		offset2 = genunarycodewithres(index,_mov_local,f);
		if(offset2 == -1)
			return 0;


		temp = t->first;
		t->first = first;    //����ַ
		if(exp->id == INDEX) //��ʾ����Ҫ��Ƭ����
			exp->s_extra = 2;
		if(!genbinarycodewithnoresEX(t,_store_index_shared_local,f))
			return 0;
		ADDWORD(offset2);
		
		//genreleasetempcode(index,offset2);

		releasetempnum(offset2,f);
		t->first = temp;
		break ;
	default:
		angel_error("��ֵ�����ֵ�ǿɱ����");
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
			linkcollection p = head->pre; //���һ��Ԫ��
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
			linkcollection p = head->pre; //���һ��Ԫ��
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
	//��ʾ��ʱ�Ƿ���slice
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
	char type = getmembase(first,f,&offset);  //������Ҫ�жϵ��������ߵĽ����ʲô��һ��Ļ������¼��������CLASS��һ�������������м���ʽ������������һ�㶼���Ա�������ʽչʾ�ģ�һ���ж����Ƿ���һ���������������ڿ��Ƿ���������
	
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
		//�����Ƕ�̬�����֣�������ʱȷ����ַ
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
		angel_error("��������ұߵĲ����������Ϲ淶��");
		return 0;
	}
	return 1;
}
int loadfunction(token t,fun f)  //���ڲ�֧��ָ���������������ľ�ȷdy_load��ʽ
{
	uchar typecode;
	/*if(t->id == COLON)
	{
		token dot = t->first;
		if(dot->id == DOT)
		{
			typecode = 3;
			if(dot->second->id == EXECFUN) //ִ�к����ͱ���$��������Ч
				return loaddot(dot,f);
			if(dot->second->id != NAME)
			{
				angel_error("��̬������ȡ��ʽ����");
				return 0;	
			}
			genopercodewithnores(_dynamic_get_function);
			if(!genbinarycodewithres(t,0,f))  //ע���������0����typcode
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
			angel_error("��̬������ȡ��ʽ����");
			return 0;
		}
	}
	else
	{*/
	if(t->id == NAME)  //ֱ��
	{
		typecode = 0;
		genopercodewithnores(_dynamic_get_function);
		genopercodewithres(typecode,f);
		adddynamicname((char *)t->attr);
	}
	else if(t->id == DOT)
	{
		typecode = 1;
		if(t->second->id == EXECFUN) //ִ�к����ͱ���$��������Ч
			return loaddot(t,f);
		if(t->second->id != NAME)
		{
			angel_error("��̬������ȡ��ʽ����");
			return 0;	
		}
		genopercodewithnores(_dynamic_get_function);
		if(!genunarycodewithres(t,0,f))  //ע���������0����typcode
			return 0;
		angel_byte->code[angel_byte->len - 1 - 2 * 2] = typecode;
	}
	else
	{
		angel_error("��̬������ȡ��ʽ����");
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
		if(t->first->id == NAME)  //���ǳ�Ա�����������ǿɱ����
			return 1;
		else
			return 0;
	default:
		return 0;
	}
}

//�߼�����֮��������������Ϊ�˽�directֱ����Ϊlocal��ͬʱ����Ҫ����ת��
int boolcalc_b(uint16_t inst,token t,fun f)
{
	uint16_t offset1,offset2;
	int flag;
	//��һ����ڶ����Ѷ���ͨ��gencodeϵ�еĴ����߼���inst��������������Ӧ����
	if(ISLOGICAL(t->first->id))  
	{
		if(ISLOGICAL(t->second->id))   //ֻҪ��logical�ʹ���direct
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
		if(ISLOGICAL(t->second->id))  //��һ����ȷ���ڶ�����direct
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
int boolcalc_u(uint16_t inst,token t,fun f)  //local��shared��direct
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
		angel_error("�������");
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
			//��ʱ������ָ��
			if(!genopercodewithnores(_loadaddr_dynamic))
				return 0;
			adddynamicname((char *)t->attr);
			return 1;//��ʾ��loadaddrָ�ͷ
		}
		return -1;
	case INDEX:
		t->second = t->second->first;
		if(!genbinarycodewithnores(t,_loadaddr_index_shared_local,f))
			return 0;
		return 1;
	case DOT:
		type = getmembase(t->first,f,&offset);  //������Ҫ�жϵ��������ߵĽ����ʲô��һ��Ļ������¼��������CLASS��һ�������������м���ʽ������������һ�㶼���Ա�������ʽչʾ�ģ�һ���ж����Ƿ���һ���������������ڿ��Ƿ���������
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
	if(type > 0) //��loadaddrǰ׺
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
	if(type > 0) //��loadaddrǰ׺
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
int parse_kernel(token t,fun f)  //�����﷨������Ҫ��ȷ���ֲ�ͬ�Ľڵ㣨���������ֲ�ͬ�����㣩��һ��ʱ�Զ��������һ�����Ե�������
{
	//ע������һ��˫Ŀ���㶼�ǽ���������һ���������ļĴ����У���Ŀ����������������ǼĴ��������޲�������ֱ����ȥ�Ĵ���
	int typeres,flag,typeres1;
	uchar typecode=0;
	uint16_t offset,offset1;
	token first=t->first,second = t->second;
	
	if(t->id==ITER)  //���ָ����û���κβ������ģ��������ⲻ��г�Ժ�Ҫ��
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
	else if(!isoperationdata(t->id))  //���������Ŀ����������Ǻ�
	{
		int error=1;
		uint16_t offset;

		if(t->id<=SELFSUB && t->id>=ASSIG)  //�ำֵ����
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
			//ֱ�ӱ��룬���ð���������if���﷨���Ľṹ
			
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

		//�����﷨���е�ÿһ���ڵ㣬���Ƚ���������ջ,�����������ָ��
		else if(t->id == DOT)  //�����ǵ��������ȡֵ����ϵͳ
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
int parse_ord(token t,fun f)  //�ȿ���ִ���﷨��,����ǵ���Ԫ����ֱ�ӷ��ز�ִ��
{
	token root=t;
	if(!root)
	{
		return 1;
	}
	else
	{
		if(!isoperationdata(root->id))  //���ʽ����Ҫ����
		{
			if(!parse_kernel(root,f))
				return 0;
			//����Ĵ���
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
		//����ǲ����������κβ���
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
	//����if����ж�����Ҫô��ȫ�ֱ���Ҫô�Ǿֲ�������Ҫô��ֱ��������
	if(elsetoken)  //�����else�Ļ�ֻ��if�Ӿ��л����jmpָ��
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
		angel_byte->len+=4;  //����if�Ӿ�ִ��������������else�Ӿ�

		//��ʱ���else��ƫ�ƣ�������if����������������
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

	//��ʾ��ʱ��һ��while���
	brand->head_addr[brand->length++]=0;

	condition=t->first,body=condition->second;

	bodyoffset=angel_byte->len;

	
	temp=maketoken(LOOP,0);
	temp->extra=bodyoffset;
	push(ctrl_state,temp);

	if(condition->first->id == TRUE)
	{
		//������ֱ������ر���
		if(!parse_block(body,f))
			return 0;
		ADDBYTE(_jmp);
		addfourbyte(angel_byte,bodyoffset);
		goto end;
	}
	else if(condition->first->id == FALSE)
		return 1; //ֱ�ӾͲ���������
	CONDITIONCODE;

	//�������������ط�����ѭ������������ʱ�˳���λ��
	ifend=angel_byte->len; 
	angel_byte->len+=4;


	//ѭ����
	if(!parse_block(body,f))
		return 0;


	//ѭ�������ж����
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
	brand->head_addr[brand->length++]=0;   //��ʾ��ʱ��һ��while���
	addscopeparse(f);
	//���ȱ���������֣�init��cond��iteration
	if(init->first->id == IN)  //���Ǳ�ʾcollection����
	{
		//����iterationӦ�÷����м������
		//����ֻ����������ʼ�����͵������̣��ж��˳�ʱ�ڵ��������н��е�
		token iter = init->first;
		body = init->second;
		//��ʼ��������
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

		//��ʼ����䣬һ��ֻ��forѭ����
		if(!parse_grammar(init->first,f))
			return 0;

		//��ʼ�����Ҫ�����ж����
		ADDBYTE(_jmp);
		int initend=angel_byte->len; 
		angel_byte->len+=4;
		
		bodyoffset = angel_byte->len;
		if(!parse_grammar(iteration->first,f))
			return 0;
		insert_pc(initend,angel_byte->len);
		
		CONDITIONCODE;
		//�������������ط�����ѭ������������ʱ�˳���λ��
		ifend=angel_byte->len;
		angel_byte->len+=4;
	}
	temp=maketoken(LOOP,0);
	temp->extra=bodyoffset;
	push(ctrl_state,temp);
	
	//ѭ����
	if(!parse_grammar(body->first,f))
		return 0;
	releasescopeparse(f);  //break���ܳ�������

	//ѭ�������ж����
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
	brand->head_addr[brand->length++]=0;   //��ʾ��ʱ��һ��while���

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
		if(!swb->sw_item[i]->type) //��ʾ��ʱ��default,Ҫ�������
		{
			_switch temp=swb->sw_item[i];
			swb->sw_item[i]=swb->sw_item[swb->len-1];
			swb->sw_item[swb->len-1]=temp;
			swb->len--;
		}
	}
	if(!swb->sw_item[swb->len]) //˵����default
	{
		_switch s=(_switch)malloc(sizeof(_switchnode));
		s->offset=angel_byte->len;
		swb->sw_item[swb->len]=s;
	}
	if(!quicksort(swb->sw_item,0,swb->len-1))
	{
		angel_error("case��ĳ��������ظ���");
		return 0;
	}

	addbreakoffset(angel_byte->len);
	pop(ctrl_state);
	return 1;
}
int parse_break(token t,fun f) //���ﴫ���t��ָѭ����ĸ��ڵ�
{
	token loop=gettopctrl();
	int scopelen = (uint16_t)loop->first;
	if(!loop)
	{
		angel_error("break��������loopѭ������switch-case�У�");
		return 0;
	}
	ADDBYTE(_jmp);
	brand->head_addr[brand->length++]=angel_byte->len;
	angel_byte->len+=4;
	return 1;
}
int parse_continue(token t,fun f)  //���break��continue���ʱ��Ҫ����һ������ÿ������λ��ƫ�����������������в��������Թ��Ϊ��������ָ��
{
	token loop=gettopctrl();
	if(!loop || loop && loop->id==SWITCH)
	{
		angel_error("continue��������loopѭ������У�");
		return 0;
	}
	ADDBYTE(_jmp);
	addfourbyte(angel_byte,loop->extra);
	return 1;
}
int parse_ret(token t,fun f)  //Ŀ�ľ��ǽ�ret�������Ľ����ŷŵ�,ret��ֵһ�ɷ��ڽ������
{
	uint16_t offset;
	int typeres;
	uchar typecode=0;
	if(!f)
	{
		angel_error("ret���ֻ�����ں����У�");
		return 0;
	}
	else if(f->type == 1)
	{
		char errorinfo[errorsize];
		sprintf(errorinfo,"���캯��%s������ret���",f->name);
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
int parse_grammar(token t,fun f)  //����flag=0����������1������
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
		if(!error)  //����һ�����������ⶼֱ���˳���
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
	//�����м����
	initangeltemp(0); //Ϊ��collection_base
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
	
	//�����м����
	initangeltemp(0);

	if(p->type ==1 || p->type == 2)
		addthistofun(p);

	if(!pushdefaultparam(p))
		return 0;

	if(!parse_grammar(p->grammar->first,p))
		return 0;

	p->temp_var_num = angel_temp->inuse;
	//p->localcount += p->temp_var_num; //


    //ÿ������ִ�����Ҫ��һ��ret��䣬����ϵͳ����ϵġ�
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
		//ע��������Ҫ��thisָ����ջ��Ȼ����
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
	//������Ҫ�ǽ��﷨���������м���룬һ���Ƚ�������������ڽ����������롣
	object o;
	angel_temp = (temp_alloc_info)calloc(1,sizeof(temp_alloc_infonode));

	dynamic_name = init_perpetual_list();
	dynamic_name->len++;
	main_byte = initbytearray();
	angel_byte = main_byte;
	brand = (codemap)calloc(1,sizeof(codemapnode));
	ctrl_state = (tstack)calloc(1,sizeof(tstacknode));  //���������ջҲ����Ϊδ���ı��������÷�Χ��ȷ������
	ctrl_state->top = -1;
	global_scope = initlink();
	global_current_scope = global_scope;

	if(!parse_class())
		return 0;

	if(!parse_main(root))
		return 0;  //�ܹ������һ��angel_temp��Ϣ����

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

	mergesharedmemory();//һ��Ҫ��֤global���ڴ�λ��ʼ�ղ���

	extent_const_pool();
	lock_const_sector();  //��ס��̬�ڴ�
	merge_page();
	return 1;
}

//����ִ��Ҫ�������£�һ���ò���������ִ�к���
//�����ӵĹ���ת��Ϊ�򵥵ļ��������
//0�żĴ����д洢�����ķ���ֵÿ�η���ʱ��Ҫ������д��NULL�򷵻�ֵ