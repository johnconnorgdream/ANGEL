#ifndef parse_def
#define parse_def

#ifdef _cplusplus
extern "c"{
#endif



#include <stdio.h>
#include "typeconfig.h"

#define maxsize 50000
#define maxtoken 1000000
#define temp_max_size 256


#define GRAMROOT 0
#define ADD 1
#define SUB 2
#define MUL 3
#define DIV 4
#define BITAND 5
#define BITOR 6
#define BITXOR 7
#define LSHIFT 8
#define RSHIFT 9
#define MOD 10
#define EQU 11
#define NOEQU 12
#define BIG 13
#define SMALL 14
#define NO 15
#define SMALLEQUAL 16
#define BIGEQUAL 17
#define AND 18
#define OR 19
#define DOT 20


#define NUM 29  //组合类型
#define NOTYPE 30
#define ENTRY 31
#define NU 32
#define FUNP 33
#define FALSE 34
#define TRUE  35
//INT以后如果要改的化一定要大于12
#define INT 36
#define FLOAT 37
#define STR 38
#define BYTES 39
#define LIST 40
#define DICT 41
#define MIN 42
#define MAX 43
#define SET 45
#define REGULAR 46
#define OBJECT 47
#define BOOLEAN 48
//下面一些类型是资源类型
#define ITERATOR 49
#define RANGE 50
#define SLICE 51
#define EXT_TYPE 53


#define IF  60
#define ELSE 61
#define LOOP 62
#define BREAK 63
#define CONTINUE 64
#define SWITCH 65
#define CASE 66
#define DEFAULT 67
#define FOR 68
#define	RET 69
#define LBRACKETL 70
#define LBRACKETR 71
#define MBRACKETL 72
#define MBRACKETR 73
#define BBRACKETL 74
#define BBRACKETR 75
#define NAME 76 //包括其他声明标识符
#define INDEX 77
#define ORD_SENTEN 78
#define IF_SENTEN  79
#define LOOP_SENTEN 80
#define BREAK_SENTEN 81
#define CONTINUE_SENTEN 82
#define RET_SENTEN 83
#define SWITCH_SENTEN 84
#define CASE_SENTEN 85
#define NO_SENTEN 86
#define FUN 87
#define EXECFUN 89
#define CLASS 90
#define MEMVAL 92
#define PRIVATE 93
#define OBJFUN 94
#define GLOBAL 97
#define SUBITEM 98
#define VALDEC 101
#define QUESTION 102
#define IN 103
#define FOR_SENTEN 104
#define ITER 105
#define NOOPER 106 //表示空操作数
#define UNCERTAIN 107  //变量不确定的注记符
#define DECLARE 108
#define DECFUN 109
#define COMPUTED 110
#define WAVEL 111
#define RANGE_STEP 112
#define OF 113
#define COLON 114
#define END  115
#define DOUHAO 116
#define IN 117


//下面时赋值类指令
#define ASSIG 120  //赋值
#define INPLACEADD 121
#define INPLACESUB 122
#define INPLACEMULT 123
#define INPLACEDIV 124
#define INPLACEBITAND 125
#define INPLACEBITOR 126
#define INPLACEBITXOR 127
#define INPLACELSHIFT 128
#define INPLACERSHIFT 129
#define INPLACEMOD 130
#define SELFADD 131
#define SELFSUB 132


#define LOWESTPRIORITY 140 
#define LIB 141
#define MAIN 142
#define MAKEFUN 143  //获取函数的操作符
/*
这两个标记是为了区分寄存器的。
*/
//#define size 50
#define errorsize 200


#define ADDBYTE(b) {addbyte(angel_byte,b);}
#define ADDWORD(w) {addtwobyte(angel_byte,w);}
#define ISEND(id) (id >= tokenlen)


typedef struct temp_alloc_infonode{
	int stack[temp_max_size];
	int inuse,top;
}*temp_alloc_info;
typedef struct tokennode{
	uint16_t id;  //reg寄存器选择标记
	int s_extra;
	unsigned long extra;
	void *attr; //这是针对变量名、值或引用。
	tokennode *first,*second;   //这里的reg存储的是语法树计算出来的中间值
}* token;
typedef struct codenode{
	char *code;
	codenode *next;
}* code;
typedef struct {
	char *keyword;
	uint16_t id;
} keyword;
typedef struct tstacknode {
	token t[maxsize];
	int top;
}* tstack;

typedef struct optionnode{
	char flag;
	char *exec_path;
	char **env;  //考虑用链表，所有的环境相关的东西都放到这个option结构中
}option;


#define UTF8_TYPE 1
#define UNICODE_TYPE 2
#define NATIVE_TYPE 3  //本地类型
#define CONST(s) GETSTR(getconstbystr(s))
#define INTCONST(l) getconstbyint(l)


token gettop(tstack s);
int stackempty(tstack s);
void push(tstack s,token c);
token pop(tstack s);


code predeal(FILE *f);
int dealcode(code c);
token grammartree_entry();

token maketoken(int id,void *attr);
void deletetoken(token t);


int parse_angel(token root);



#ifdef _cplusplus
}
#endif
#endif