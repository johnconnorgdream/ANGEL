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

编写ANGEL库函数须知
如果在库中用到angel_alloc_block申请了内存（当然不同的类型有不同的封装，比如initinteger）


*/
void initexttype();


#ifdef _cplusplus
}
#endif
#endif