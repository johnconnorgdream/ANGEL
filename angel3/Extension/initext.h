#ifndef initext_def
#define initext_def
#ifdef _cplusplus
extern "c"{
#endif

#ifndef COMMON_LIB
#define COMMON_LIB
#endif


#include "fs/_filesystem.h"
#include "socket/_socket.h"
#include "time/_time.h"
#include "thread/_thread.h"
#include "xml/_xmlcompile.h"



/*

��дANGEL�⺯����֪
����ڿ����õ�angel_alloc_block�������ڴ棨��Ȼ��ͬ�������в�ͬ�ķ�װ������initinteger��


*/
void initexttype();


#ifdef _cplusplus
}
#endif
#endif