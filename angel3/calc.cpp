#include "angel.h"
//angel语言默认是用utf-8来显示
//towide是转化为同意的宽字符unicode，tomult是转化为统一的utf-8，然后在输出的时候转化为本地编码。
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
	int i=1,singal=1;  //singal表示该字符表示的正负
	char *p;
	if(*a=='-')
	{
		p=++a;
		singal=-1;
	}
	else
		p=a;
	while(*p) p++;//获得的是最后的0
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
	int radix=10; //以十进制为例
	if(a>=0)
		b=a;
	else
		b=-a;
	if(b==0)
		*p++='0';
	while(b)
	{
		int i;
		i=b%radix;//每次取最后一位
		b/=radix; //每次截去最后一位
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
 number getinteger(long num)  //小数以后想办法在做,type为0表示是整数，为1表示是小数
{
	//这里我们首先默认只支持一个int型变量,而且在测试是我们也只用int范围内的数字来测试
	number n=(number)malloc(sizeof(numbernode));
	n->type=0;
	n->data.i=num;  //先默认都放在整数，小数只有在运算的时候采用得到。
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
		 //下面这一步很重要
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
	_char *res=(_char *)malloc(strlen(des)+strlen(src)+1),*p;   //字符串加法之所以慢是这里每次都要计算strlen()
	p=res;
	while(*des) *p++=*des++;
	while(*src) *p++=*src++;
	*p=0;
	return res;
}
void _strcat(_char *s1,_char *s2)
{
	strcat(s1,s2); // 这个函数未来考虑替换
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
		if(*p>0)  //表示此时是非汉字,这里还是有很大争议的。
		{
			*wide++=*p++;
			*wide++=*(wide-1);  //在补填一个0
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
char *tomult(string s)  //将Unicode转化为utf8
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
	if(len==-1) //表示此时按gbk编码
	{
		len = GbkToUnicode(utf8,(unsigned short *)s->s,s->len);
		if(len==-1)
		{
			angel_error("编码方式出错！");
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