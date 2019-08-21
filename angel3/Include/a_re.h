#ifndef regular_def
#define regular_def


#ifdef _cplusplus
extern "c"{
#endif

#include "astring.h"
#include "bytecode.h"
#include "data.h"

#define TYPE_REPEAT 1
#define TYPE_GROUP 2
#define TYPE_CAT 3
#define TYPE_CHARSET 4
#define TYPE_ALTERNATION 5
#define TYPE_META 6
#define TYPE_CHAR 7
#define TYPE_REF 8
#define TYPE_ALL 9

#define CODE_ALTERNATION 1
#define CODE_REPEAT_GREEDY 2
#define CODE_REPEAT_GREEDY_WITH_ONE 3
#define CODE_REPEAT_GREEDY_BOTH 4
#define CODE_REPEAT_GREEDY_CONDITION 5
#define CODE_REPEAT_LAZY 6
#define CODE_REPEAT_LAZY_WITH_ONE 7
#define CODE_REPEAT_LAZY_BOTH 8
#define CODE_REPEAT_LAZY_CONDITION 9
#define CODE_CHECK_CHARSET 10
#define CODE_JUMP 11
#define CODE_CHAR 12
#define CODE_REF 13
#define CODE_ALL 14
#define CODE_GROUP_CAPTURE_BEGIN 15
#define CODE_GROUP_CAPTURE_END 16
#define CODE_MATCH_BOUNDARY 17
#define CODE_MATCH_NOT_BOUNDARY 18
#define CODE_MATCH_DIGITAL 19
#define CODE_MATCH_NOT_DIGITAL 20
#define CODE_MATCH_WORD 21
#define CODE_MATCH_NOT_WORD 22
#define CODE_MATCH_SPACE 23
#define CODE_MATCH_NOT_SPACE 24
#define CODE_EXIT 25
#define CODE_UPDATE_ALTERENATION 26
#define CODE_REPEAT_GREEDY_BEGIN 27
#define CODE_CHECK_NOT_CHARSET 28



#define META_BOUNDARY_WORD 1
#define META_BOUNDARY_LINE_BEGIN 2
#define META_BOUNDARY_LINE_END 3
#define META_DIGITAL 4
#define META_WORD 5
#define META_SPACE 6
typedef struct wcharsnode{
	wchar *ws;
	int size,alloc;
}*wchars;
typedef struct regelementnode{
	char type,flag,extra; //正则表达式元素的类型
	union{
		struct{
			wchars range;
			wchars set;
		}charset;
		wchar _char;
		collection or_list;
		struct{
			regelementnode *unit;
			int repeat_min,repeat_max,record_index; //重复次数
		}repeat;
	}attr;
}*regelement;
typedef struct object_regularnode{
	BASEDEF;
	//上下行运行环境
	//编译环境
	int16_t group_count;
	bytecode code;
	wchar *pattern;
	int16_t record_count,alternation_count,repeat_history_for_duplicate;
}*object_regular;
object_regular are_compile(wchar *pattern);
void clearregel(regelement el);
int reg_match(object_regular or,wchar *source,int begin,int end);
object reg_find(object_regular or,wchar *source,int begin,int end);
object reg_findall(object_regular or,wchar *source,int begin,int end);

object syscode_regular(object o);



#ifdef _cplusplus
}
#endif
#endif