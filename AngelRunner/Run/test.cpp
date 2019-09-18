#include <locale.h>
#include <tchar.h>
#include "angel.h"
#include "stringutil.h"
//本系统基本模块包括运算模块和赋值操作，运算模块包括基本的算数逻辑运算、变量取值、函数运算、点运算、索引取项运算其中前两种返回值或自定义对象的引用，后两种是返回变量的地址，为了更广泛的操作。其中变量取值运算视具体情况而定不能一概而论。
//本系统将运算分为一元运算和二元运算，其中二元运算需要先将两个运算因子分别做一元运算化为统一的运算结果再进行二元运算。所以一般单目运算中变量或成员变量的取值运算的运算结果是不统一的，需要单独讨论。
//本系统中一元运算包括函数的执行，数据、变量、成员变量、索引中间结果的取值，还有空类型。
#include <stdio.h>
#include <stdlib.h>



int main(int argc ,char **argv)
{
	//int len;
	//char *pattern = towide("(a+?)",&len);
	//regelement reg = are_compile((wchar *)pattern);
	setlocale(LC_ALL,"");
	//wchar_t wstr[] = L"他们就是啥";
	//wchar_t fomat[] = L"%s";
	//printf("%ls",wstr);
	shell(argc,argv);
	return 1;
}

