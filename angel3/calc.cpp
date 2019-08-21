#include "angel.h"
//angel����Ĭ������utf-8����ʾ
//towide��ת��Ϊͬ��Ŀ��ַ�unicode��tomult��ת��Ϊͳһ��utf-8��Ȼ���������ʱ��ת��Ϊ���ر��롣
extern list obj_recovery;


int digits(long l)
{
	int i=1;
	long base=10;
	while(l/base)
	{
		i++;
		base*=10;
	}
	return i;
}
long toint(char *a)
{
	long total=0;
	int i=1,singal=1;  //singal��ʾ���ַ���ʾ������
	char *p;
	if(*a=='-')
	{
		p=++a;
		singal=-1;
	}
	else
		p=a;
	while(*p) p++;//��õ�������0
	while(p--!=a)
	{
		total=total+(*p-'0')*i;
		i*=10;
	}
	if(singal==-1)  
		total=-total;	
	return total;
}
double todecimal(char *a)
{
	return 0;
}
_char* tointchar(int a)
{
	char *ret,*p,*temp,*q;
	int b;
	p=(char *)malloc(num_lenth);
	ret=(char *)malloc(num_lenth);
	q=ret;
	temp=p;
	int radix=10; //��ʮ����Ϊ��
	if(a>=0)
		b=a;
	else
		b=-a;
	if(b==0)
		*p++='0';
	while(b)
	{
		int i;
		i=b%radix;//ÿ��ȡ���һλ
		b/=radix; //ÿ�ν�ȥ���һλ
		*p++=i+'0';
	}
	if(a<0)
		*p++='-';
	while(p>temp)
		*q++=*--p;
	*q=0;
	free(temp);
	return ret;
}
 number getinteger(long num)  //С���Ժ���취����,typeΪ0��ʾ��������Ϊ1��ʾ��С��
{
	//������������Ĭ��ֻ֧��һ��int�ͱ���,�����ڲ���������Ҳֻ��int��Χ�ڵ�����������
	number n=(number)malloc(sizeof(numbernode));
	n->type=0;
	n->data.i=num;  //��Ĭ�϶�����������С��ֻ���������ʱ����õõ���
	return n;
}
 number getfloat(float num)
 {
	 number n=(number)malloc(sizeof(numbernode));
	 n->type=1;
	 n->data.f=num;
	 return n;
 }
 object getobject(void *ref,char type)
 {
	 list l;
	 string s;
	 dictionary d;
	 unsigned long i;
	 int common_size=sizeof(objectnode)-sizeof(objectcommon)+4;
	 object o;
#define GETBASEOBJ(res) \
		res=(object)malloc(common_size);\
		memset(res,0,common_size); 
	 switch(type)
	 {
	 case INT:
		 GETBASEOBJ(o);
		 o->data.n=(number)ref;
		 break ;
	 case STR:
		 GETBASEOBJ(o);
		 o->data.s=(string)ref;
		 s=(string)ref;
		 break ;
	 case LIST:
		 l=(list)ref;
		 GETBASEOBJ(o);
		 if(!l)
			 break ;
		 o->data.l=l;
		 //������һ������Ҫ
		 //for(i=0; i<l->len; i++)
			// l->item[i]->count++;
		 break ;
	 case DICT:
		 d=(dictionary)ref;
		 GETBASEOBJ(o);
		 if(!d)
			 break ;
		 o->data.d=d;
		 break ;
	 case SET:
		 GETBASEOBJ(o);
		 if(!ref)
			 break ;
		 o->data._s=(set)ref;
		 break ;
	 case FUN:
		 GETBASEOBJ(o);
		 if(!ref)
			 break ;
		 o->data.f=(fmap)ref;
		 break ;
	 default:
		 o=initobject();
		 break ;
	 }
	 o->type=type;
	 return o;
 }
 number copynumber(number i)
 {
	 number res=(number)malloc(sizeof(numbernode));
	 memcpy(res,i,sizeof(numbernode));
	 return res;
 }
 decimal getdecimal(char *num_char)
 {
	 decimal d=(decimal)malloc(sizeof(decimalnode));
	 d->count=0;
	 d->data=todecimal(num_char);
	 return d;
 }
char *getstrcat(_char *des,_char *src)
{
	_char *res=(_char *)malloc(strlen(des)+strlen(src)+1),*p;   //�ַ����ӷ�֮������������ÿ�ζ�Ҫ����strlen()
	p=res;
	while(*des) *p++=*des++;
	while(*src) *p++=*src++;
	*p=0;
	return res;
}
void _strcat(_char *s1,_char *s2)
{
	strcat(s1,s2); // �������δ�������滻
}
/*void towide(string s)
{
	char *wide,*old,*p;
	old=s->s;
	p=old;
	s->alloc_size=s->len*2+1;
	wide=(char *)malloc(s->alloc_size);
	s->s=wide;
	while(*p)
	{
		if(*p>0)  //��ʾ��ʱ�ǷǺ���,���ﻹ���кܴ�����ġ�
		{
			*wide++=*p++;
			*wide++=*(wide-1);  //�ڲ���һ��0
			s->len++;
		}
		else
			*wide++=*p++;
	}
	*wide=0;
	free(old);
}
char *tomult(string s)
{
	char *mult=(char *)malloc(s->len+1),*q;
	char *p=s->s;
	q=mult;
	while(*p)
	{
		if(*p>0)
		{
			*q++=*p++;
			p++;
		}
		else
			*q++=*p++;
	}
	*q=0;
	return mult;
}
*/
char *tonative(string s)
{
	char *res = (char *)calloc(s->len+1,sizeof(char));
	UnicodeToGbk((unsigned short *)s->s,res,s->len);
	return res;
}
char *tomult(string s)  //��Unicodeת��Ϊutf8
{
	char *res = (char *)calloc(s->len*3/2+1,sizeof(char));
	int len = UnicodeToUtf8(s->s,res);
	return res;
}
void towide(string s)
{
	char *utf8 = s->s;
	s->alloc_size = s->len*2+2;
	s->s = (char *)calloc(s->alloc_size,sizeof(char));
	int len = Utf8ToUnicode(utf8,s->s);
	if(len==-1) //��ʾ��ʱ��gbk����
	{
		len = GbkToUnicode(utf8,(unsigned short *)s->s,s->len);
		if(len==-1)
		{
			angel_error("���뷽ʽ����");
			return ;
		}
		goto save;
	}
save:
	if(2*len < s->alloc_size)
	{
		s->alloc_size=len*2;
		s->s=(char *)realloc(s->s,s->alloc_size*sizeof(char));
	}
	free(utf8);
	s->len=len;
}