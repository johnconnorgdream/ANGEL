
//正则表达式引擎
/*

难点1：保证最终的匹配符合greedy和lazy模式
难点2：保证


目前在正则引擎上没有做优化，即虚拟机的优化，比如alterlative

*/



#include <stdlib.h>
#include "data.h"
#include "lib.h"
#include "shell.h"
#include "amem.h"


regelement parsereg(object_regular or,wchar end = 0);
#define reg_base_size 4

wchars initwchars()
{
	wchars ret = (wchars)angel_sys_malloc(sizeof(wcharsnode));
	ret->alloc = ret->size = 0;
	ret->alloc = reg_base_size;
	ret->ws = (wchar *)angel_sys_calloc(reg_base_size,sizeof(wchar));
	return ret;
}
void addchar(wchars regarray,wchar el)
{
	if(regarray->size >= regarray->alloc)
	{
		regarray->alloc *= 2;
		regarray->ws = (wchar *)realloc(regarray->ws,regarray->alloc * sizeof(wchar));
	}
	regarray->ws[regarray->size++] = el;
}

#define CUR (*or->pattern)
#define TEST(step) (*(or->pattern+step))
#define SKIP(step) or->pattern += (step);
#define MOVE(offset) (or->pattern+offset)
#define ISWCHARNUM(c) (c >= '0' && c <= '9')
#define ISWCHARWORD(c) (c >= '0' && c <= '9' || c >= 'a' && c <= 'z' || \
						c >= 'A' && c <= 'Z' || c == '_')

#define ISWCHARSPACE(c) (c == ' ')
regelement build()
{
	regelement el = (regelement)angel_sys_calloc(1,sizeof(regelementnode));
	return el;
}
regelement buildescp(char metatype,char flag = 0)
{
	regelement el = build();
	el->type = TYPE_META;
	el->flag = flag;
	el->attr._char = metatype;
	return el;
}
regelement buildchar(wchar wc)
{
	regelement el = build();
	el->type = TYPE_CHAR;
	el->attr._char = wc;
	return el;
}
regelement buildcharset()
{
	regelement el = build();
	el->type = TYPE_CHARSET;
	el->attr.charset.range = initwchars();
	el->attr.charset.set = initwchars();
	return el;
}
regelement buildtogether(char type,collection rp)
{
	regelement el = build();
	el->type = type;
	el->attr.or_list = rp;
	return el;
}
regelement buildref(int ref)
{
	regelement el = build();
	el->type = TYPE_REF;
	el->flag = ref;
	return el;
}
int hextodec(wchar *ptr,int count,int hex = 10)
{
#define TODEC(num) (num - '0')
	int rank = 1;
	char total = 0;
	if(count == 0)
		return -1;
	for(int i = count-1; i >= 0; i--)
	{
		total += TODEC(ptr[i])*rank;
		rank *= hex;
	}
	return total;
}
regelement getrefelement(object_regular or,int count,int bl)
{
	
	regelement el = NULL;
	if(bl) //只可能是引用
	{
		int ref = hextodec(or->pattern,count,10);
		if(ref > or->group_count)
		{
			angel_error("no ref!");
			return NULL;
		}
		el = buildref(ref);
	}
	else
	{
		int ref = hextodec(or->pattern,count,10);
		if(ref > or->group_count)
		{
			el = buildchar(hextodec(or->pattern,count,8));
		}
		else
		{
			el = buildref(ref);
		}
	}
	return el;
}
regelement dealwithescape(object_regular or,int isincharset = 0)  //need是否需要判断向后引用
{
	
	SKIP(1);
	wchar ch = CUR;
	regelement el = NULL;
	if(ISWCHARNUM(ch))
	{
		//backref,或者表示一个8进制
		int count = 0;
		for(wchar *p = or->pattern; (*p && ISWCHARNUM(*p) && count <=3); p++,count++);
		switch(count)
		{
		case 1:
			el = getrefelement(or,count,CUR > '7');
			break ;
		case 2:
			el = getrefelement(or,count,(CUR > '7' || TEST(1) > '7'));
			break ;
		case 3:
			el = getrefelement(or,count,(CUR > '3' || TEST(1) > '7' || TEST(2) > '7'));
			break ;
		}
		SKIP(count);
		return el;
	}
	switch(ch)  //里面部分先不做
	{
	case 'b':
		if(isincharset)
			el = buildchar('\b');
		else
			el = buildescp(META_BOUNDARY_WORD);
		break ;
	case 'B':
		if(isincharset)
			el = buildchar('\B');
		else
			el = buildescp(META_BOUNDARY_WORD,1);
		break ;
	//case 'c':
		//if(bu
		//el = buildmeta(
		//break ;
	case 'd':
		if(isincharset)
			el = buildchar('\d');
		else
			el = buildescp(META_DIGITAL);
		break ;
	case 'D':
		if(isincharset)
			el = buildchar('\D');
		else
			el = buildescp(META_DIGITAL,1);
		break ;
	case 'f':
		el = buildchar('\f');
		break ;
	case 'n':
		el = buildchar('\n');
		break ;
	case 'r':
		el = buildchar('\r');
		break ;
	case 's':
		if(isincharset)
			el = buildchar('\s');
		else
			el = buildescp(META_SPACE);
		break ;
	case 'S':
		if(isincharset)
			el = buildchar('\S');
		else
			el = buildescp(META_SPACE,1);
		break ;
	case 't':
		el = buildchar('\t');
		break ;
	case 'v':
		el = buildchar('\v');
		break ;
	case 'w':
		if(isincharset)
			el = buildchar('\w');
		else
			el = buildescp(META_WORD);
		break ;
	case 'W':
		if(isincharset)
			el = buildchar('\W');
		else
			el = buildescp(META_WORD,1);
		break ;
	case 'u': //unicode
		el = buildchar(hextodec(or->pattern+1,4,16));
		SKIP(4);
		break ;
	case 'x':
		el = buildchar(hextodec(or->pattern+1,2,16));
		SKIP(2);
		break ;
	default:
		el = buildchar(ch);
		break ;
	}
	SKIP(1);
	return el;
}
int getnum(object_regular or)
{
	wchar *pattern = or->pattern;
	int count = 0;
	while(*pattern && ISWCHARNUM(*pattern))
	{
		count++;
		pattern++;
	}
	int res = hextodec(or->pattern,count);
	SKIP(count);
	return res;
}
regelement popreg(collection exp)
{
	if(exp->size == 0)
		return NULL;
	regelement top = (regelement)exp->element[--exp->size];
	return top;
}
void clearregexp(collection rg);
regelement build_or_reg(object_regular or,wchar end = 0)
{
#define CHECKREPEATUNIT regelement unit = popreg(ret); \
						if(unit == NULL) { \
							angel_error("repeat unit cannot be null!"); \
							return NULL; \
						}
#define CHECKPREREPEAT if(unit->type == TYPE_REPEAT) { \
							angel_error("repeat unit cannot be repeat unit!"); \
							return NULL; \
						}
	wchar *pattern  = or->pattern;
	collection ret = initcollection(reg_base_size);
	while(*or->pattern)
	{
		regelement el;
		wchar ch = CUR;
		if(ch == '\\')
		{
			el = dealwithescape(or);
		}
		else if(ch == end)
		{
			break ;
		}
		else if(ch == '^')
		{

		}
		else if(ch == '$')
		{

		}
		else if(ch == '(') //捕获
		{
			SKIP(1);
			int flag = 0;
			if(CUR == '?' && TEST(1) == ':') //不需要引用
			{
				SKIP(2);
			}
			else
			{
				if(or->group_count < 100)
				{
					or->group_count++;
					flag = 1;
				}
				else
				{
					angel_error("too many groups！");
					return NULL;
				}
			}
			el = parsereg(or,')');
			SKIP(1);
			el->type = TYPE_GROUP;
			if(flag)
				el->flag = or->group_count;
		}
		else if(ch == '[')  //-,\d,\s,\w,^,.
		{
			el = buildcharset();
			if(TEST(1) == ']')  //表示非法结束
			{
				angel_error("character set error：[]!");
				return NULL;
			}
			else if(TEST(1) == '^')
			{
				el->flag = 1;
				SKIP(2);
			}
			else
				SKIP(1);
			ch = CUR;
			while(ch != ']')
			{
				if(ch == '\\')
				{
					regelement temp = dealwithescape(or,1);
					if(temp == NULL)
						return NULL;
					ch = temp->attr._char;
					free(temp);
				}
				if(TEST(1) == '-')
				{
					wchar ch1 = TEST(2);
					addchar(el->attr.charset.range,ch);
					addchar(el->attr.charset.range,ch1);
					SKIP(3);
				}
				else
				{
					addchar(el->attr.charset.set,ch);
					SKIP(1);
				}
				ch = CUR;
			}
			SKIP(1);  //过滤掉]
		}
		else if(ch == '|')
		{
			goto rets;
		}		
		else if(ch == '{')
		{
			CHECKREPEATUNIT;
			CHECKPREREPEAT;
			SKIP(1);//过滤{
			int begin = getnum(or);
			wchar inteval = CUR;
			SKIP(1);
			int end = getnum(or);
			SKIP(1); //过滤}
			if(begin == -1 || end == -1 || inteval != ',')
			{
				//直接当正常字符串处理
				goto addchar;
			}
			el = build();
			el->type = TYPE_REPEAT;
			el->attr.repeat.unit = unit;
			el->attr.repeat.repeat_min = begin;
			el->attr.repeat.repeat_max = end;
			el->attr.repeat.record_index = or->repeat_item_count++;
		}
		else if(ch == '*')
		{
			
			CHECKREPEATUNIT
			CHECKPREREPEAT
			el = build();
			el->type = TYPE_REPEAT;
			el->attr.repeat.unit = unit;
			el->attr.repeat.repeat_min = 0;
			el->attr.repeat.repeat_max = LONG_MAX;
			SKIP(1);
		}
		else if(ch == '+')
		{
			
			CHECKREPEATUNIT
			CHECKPREREPEAT
			el = build();
			el->type = TYPE_REPEAT;
			el->attr.repeat.unit = unit;
			el->attr.repeat.repeat_min = 1;
			el->attr.repeat.repeat_max = LONG_MAX;
			el->attr.repeat.record_index = or->repeat_item_count++;
			SKIP(1);
		}
		else if(ch == '?')
		{
			CHECKREPEATUNIT
			SKIP(1);
			if(unit->type == TYPE_REPEAT)
			{
				unit->flag = 1;  //表示非贪婪模式
				el = unit;
			}
			else
			{
				el = build();
				el->type = TYPE_REPEAT;
				el->attr.repeat.unit = unit;
				el->attr.repeat.repeat_min = 0;
				el->attr.repeat.repeat_max = 1;
			}
		}
		else if(ch == '.') //all，除了\n以外的所有字符
		{
			el = build();
			SKIP(1);
			el->type = TYPE_ALL;
		}
		else  //表示一般字符
		{
addchar:
			el = buildchar(CUR);
			SKIP(1);
		}
		addcollection(ret,el);
	}
rets:
	if(ret->size == 1)
	{
		regelement el = (regelement)ret->element[--ret->size];
		clearregexp(ret);
		return el;
	}
	else
		return buildtogether(TYPE_CAT,ret);
}
void clearwchars(wchars ws)
{
	free(ws->ws);
	free(ws);
}
void clearregel(regelement el)
{
	switch(el->type)
	{

	case TYPE_REPEAT:
		clearregel(el->attr.repeat.unit);
		break ;
	case TYPE_CHARSET:
		clearwchars(el->attr.charset.range);
		clearwchars(el->attr.charset.set);
		break ;
	case TYPE_ALTERNATION :
	case TYPE_GROUP:
		clearregexp(el->attr.or_list);
		break ;
	case TYPE_META:
	case TYPE_ALL:
	default:
		break ;
	}
	free(el);
}
void clearregexp(collection rg)
{
	for(int i = 0; i<rg->size; i++)
		clearregel((regelement)rg->element[i]);
	free(rg);
}
regelement parsereg(object_regular or,wchar end)
{
	collection ret = initcollection(reg_base_size);
	if(CUR == '|')
	{
		angel_error("正则表达式第一个不能为|");
		return NULL;
	}
	while(1)
	{
		or->alternation_count++;
		regelement res = build_or_reg(or,end);
		if(res != NULL)
			addcollection(ret,res);
		else
		{
			clearregexp(ret);
			return NULL;
		}
		if(!CUR || CUR == end)
			break ;
		SKIP(1);
	}
	regelement root = buildtogether(TYPE_ALTERNATION,ret);
	return root;
}
#define ADDBYTE(b) {addbyte(bc,b);}
#define ADDWORD(w) {addtwobyte(bc,w);}
//注意后面的parse部分也要传入regularobject

inline void insertword(bytecode bc,int offset,int writepos)
{
	int temp = bc->len;
	bc->len = writepos;
	ADDWORD(offset);
	bc->len = temp;
}
int compile_element(regelement el,object_regular or);
void compile_repeat(regelement el,object_regular or)
{
	bytecode bc = or->code;
	int min = el->attr.repeat.repeat_min;
	int max = el->attr.repeat.repeat_max;
	int begin;
	int flag = 0;
	int write;
	begin = bc->len;
	int has_recorded = 0;
	if(el->flag == 0)  //这是贪婪匹配算法
	{
		if(min == 0 && max == LONG_MAX) {
			ADDBYTE(CODE_REPEAT_GREEDY);
		}
		else if(min == 0 && max == 1) {
			ADDBYTE(CODE_REPEAT_GREEDY_CONDITION);
			flag = 1;
		}
		else if(min == 1 && max == LONG_MAX) {
			ADDBYTE(CODE_REPEAT_GREEDY_WITH_ONE);
			ADDWORD(el->attr.repeat.record_index);
			has_recorded = 1;
		}
		else {
			ADDBYTE(CODE_REPEAT_GREEDY_BOTH);
			ADDWORD(el->attr.repeat.record_index);
			ADDWORD(min);
			ADDWORD(max);
			has_recorded = 1;
		}
	}
	else
	{
		if(min == 0 && max == LONG_MAX)
		{
			ADDBYTE(CODE_REPEAT_LAZY);
		}
		else if(min == 0 && max == 1)
		{
			ADDBYTE(CODE_REPEAT_LAZY_CONDITION);
			flag = 1;
		}
		else if(min == 1 && max == LONG_MAX)
		{
			ADDBYTE(CODE_REPEAT_LAZY_WITH_ONE);
			ADDWORD(el->attr.repeat.record_index);
			has_recorded = 1;
		}
		else
		{
			ADDBYTE(CODE_REPEAT_LAZY_BOTH);
			ADDWORD(el->attr.repeat.record_index);
			ADDWORD(min);
			ADDWORD(max);
			has_recorded = 1;
		}
	}
	write = bc->len;
	addcollection(or->repeat_predict_set,bc->code + write);
	bc->len += 2;  //这是填充跳跃的字段
	ADDWORD(or->repeat_count++);
	
	compile_element(el->attr.repeat.unit,or);
	int offset = bc->len - begin;
	if(flag == 0)
	{
		ADDBYTE(CODE_JUMP);
		ADDWORD(offset);
	}
	insertword(bc,bc->len - begin,write);
	if(has_recorded)
	{
		ADDBYTE(CODE_REPEAT_RESET);
		ADDWORD(el->attr.repeat.record_index);
	}
}
void compile_together(regelement el,object_regular or,int flag = 0)
{
	bytecode bc = or->code;
	collection orlist = el->attr.or_list;
	for(int i = 0; i < orlist->size; i++)
	{
		if(flag == 1 && i < orlist->size - 1) //到最后一个可选项之前都要用CODE_ALTERNATION指令
		{
			//这里面到头了就不能再CODE_ALTERNATION了
			int begin = bc->len;
			ADDBYTE(CODE_ALTERNATION);
			bc->len += 2;
			compile_element((regelement)orlist->element[i],or);
			ADDBYTE(CODE_UPDATE_ALTERENATION);
			ADDWORD(or->alternation_count);
			insertword(bc,bc->len-begin,begin+1);
		}
		else
			compile_element((regelement)orlist->element[i],or);
	}
	if(flag == 1 && orlist->size > 1)
	{
		ADDBYTE(CODE_UPDATE_ALTERENATION);
		ADDWORD(or->alternation_count);
		or->or_jump_set[or->alternation_count++] = bc->len;
	}
}
void compile_charset(regelement el,object_regular or)
{
	bytecode bc = or->code;
	wchars range = el->attr.charset.range;
	wchars set = el->attr.charset.set;
	if(or->flag == 1){
		ADDBYTE(CODE_CHECK_NOT_CHARSET);
	}
	else{
		ADDBYTE(CODE_CHECK_CHARSET);
	}
	ADDWORD(range->size);
	ADDWORD(set->size);
	for(int i = 0; i < range->size; i += 2)
	{
		ADDWORD(range->ws[i]);
		ADDWORD(range->ws[i+1]);
	}
	for(int i = 0; i < set->size; i++)
	{
		//准确的说这里应该来一个去重的方法
		ADDWORD(set->ws[i]);  
	}
}
void compile_meta(regelement el,object_regular or)
{
	bytecode bc = or->code;
	switch(el->attr._char)
	{
	case META_BOUNDARY_WORD:
		if(el->flag == 1)
		{
			ADDBYTE(CODE_MATCH_NOT_BOUNDARY);
		}
		else
		{
			ADDBYTE(CODE_MATCH_BOUNDARY);
		}
		break ;
	case META_DIGITAL:
		if(el->flag == 1)
		{
			ADDBYTE(CODE_MATCH_NOT_DIGITAL);
		}
		else
		{
			ADDBYTE(CODE_MATCH_DIGITAL);
		}
		break ;
	case META_WORD:
		if(el->flag == 1)
		{
			ADDBYTE(CODE_MATCH_NOT_WORD);
		}
		else
		{
			ADDBYTE(CODE_MATCH_WORD);
		}
		break ;
	case META_SPACE:
		if(el->flag == 1)
		{
			ADDBYTE(CODE_MATCH_NOT_SPACE);
		}
		else
		{
			ADDBYTE(CODE_MATCH_SPACE);
		}
		break ;
	default:
		angel_error("不是指定的元字符！");
		break ;
	}
}
int compile_element(regelement el,object_regular or)
{
	bytecode bc = or->code;
	switch(el->type)
	{
	case TYPE_CAT:
		compile_together(el,or);
		break ;
	case TYPE_GROUP: //表示是捕获组
		ADDBYTE(CODE_GROUP_CAPTURE_BEGIN);
		ADDBYTE(el->flag-1);
		compile_together(el,or,1);
		ADDBYTE(CODE_GROUP_CAPTURE_END);
		ADDBYTE(el->flag-1);
		break ;
	case TYPE_ALTERNATION:
		compile_together(el,or,1);
		break ;
	case TYPE_REPEAT:
		compile_repeat(el,or);
		break ;
	case TYPE_CHARSET:
		compile_charset(el,or);
		break ;
	case TYPE_META:
		compile_meta(el,or);
		break ;
	case TYPE_CHAR:
		ADDBYTE(CODE_CHAR);
		ADDWORD(el->attr._char);
		break ;
	case TYPE_REF:
		ADDBYTE(CODE_REF);
		ADDBYTE(el->flag);
		break ;
	case TYPE_ALL:
		ADDBYTE(CODE_ALL); //注意执行时去掉换行
		break ;
	default:
		angel_error("正则编译出错：未知元素！");
		return 0;
	}
	return 1;
}


#define PARAM(no)  *(int16_t *)(code+no)
#define ADDOP(op) code += op;
#define NEXTOP(op) ADDOP(op) goto next;
char *_initpredict(char *code)
{
next:
	switch(*code)
	{
	case CODE_REPEAT_RESET:
		NEXTOP(3);
	case CODE_UPDATE_ALTERENATION:
		NEXTOP(3);
	case CODE_ALTERNATION:
		NEXTOP(5);
	case CODE_REPEAT_GREEDY:
	case CODE_REPEAT_GREEDY_CONDITION:
		NEXTOP(5);
	case CODE_REPEAT_GREEDY_WITH_ONE:
		NEXTOP(7);
	case CODE_REPEAT_GREEDY_BOTH:
		NEXTOP(11);
	case CODE_REPEAT_LAZY:
	case CODE_REPEAT_LAZY_CONDITION:
		NEXTOP(5);
	case CODE_REPEAT_LAZY_WITH_ONE:
		NEXTOP(7);
	case CODE_REPEAT_LAZY_BOTH:
		NEXTOP(11);
	case CODE_GROUP_CAPTURE_BEGIN://捕获字符串
		NEXTOP(2);
	case CODE_GROUP_CAPTURE_END://捕获字符串
		NEXTOP(2);
	default:
		return code;
	}
}
void initpredict(object_regular or)
{
	int arg1,arg2;
	wchar *charset;
	state current;
	//这个是在做repeat匹配的时候判断是否要做pushstate
	collection predict = or->repeat_predict_set;
	for(int i = 0; i < predict->size; i++){
		predict->element[i] = _initpredict((char *)predict->element[i]);
	}
}
object_regular are_compile(wchar *pattern) //编译模式串
{
	bytecode bc = initbytearray();
	object_regular ret = (object_regular)angel_alloc_block(sizeof(object_regularnode));
	ret->type = REGULAR;
	ret->code = bc;
	ret->pattern = pattern;
	ret->repeat_count = 0;
	ret->alternation_count = 0;
	ret->repeat_item_count = 0;
	ret->group_count = 0;
	regelement root = parsereg(ret,0);
	if(root == NULL) return NULL;
	if(ret->alternation_count > 0)
		ret->or_jump_set = (int16_t *)angel_sys_calloc(ret->alternation_count,sizeof(int16_t));
	ret->alternation_count = 0;
	ret->repeat_predict_set = initcollection();
	compile_element(root,ret);
	ADDBYTE(CODE_EXIT);
	ret->match_record = (int *)angel_sys_calloc(ret->repeat_item_count,sizeof(int));
	ret->repeat_for_duplicate_record = (int *)angel_sys_calloc(ret->repeat_count,sizeof(int));
	initpredict(ret);
	ret->group = (state *)angel_sys_calloc(100,sizeof(state));
	void *res = angel_sys_calloc(100,sizeof(statenode));
	for(int i = 0; i < 100; i++){
		ret->group[i] = &((statenode *)res)[i];
	}
	ret->match_state = initcollection();
	return ret;
}

inline void pushstate(collection g_state,int index,char *code,int isgreedy = 1)
{
	state s = NULL;
	if(g_state->size > 0) {
		state top = (state)g_state->element[g_state->size - 1];
		if(top->index == index && top->pos == code) {
			return ;
		}
	}
	if(g_state->size < g_state->alloc) {
		s = (state)g_state->element[g_state->size++];
		if(!s) {
			s = (state)angel_sys_malloc(sizeof(statenode));
			g_state->element[g_state->size-1] = s;
		}
	}
	else {
		s = (state)angel_sys_malloc(sizeof(statenode));
		addcollection(g_state,s);
	}
	s->index = index;
	s->pos = code;
	s->isgreedy = isgreedy;
}
inline state popstate(collection g_state)
{
	state s = NULL;
	if(g_state->size > 0)
		return (state)g_state->element[--g_state->size];
	return NULL;
		
}

inline int compare_w_char(wchar *pattern,wchar *source,int len)
{
	for(int i = 0; i < len; i++)
	{
		if(pattern[i] != source[i])
			return 0;
	}
	return 1;
}

inline int test_next(char *code,wchar check,state *group_record)
{
	int arg1,arg2;
	wchar *charset;
	state current;
	//这个是在做repeat匹配的时候判断是否要做pushstate
	switch(*code)
	{
	case CODE_JUMP:
	case CODE_EXIT:
		return 1;
	case CODE_CHECK_CHARSET:
		//CODE_CHECK_CHARSET匹配到以后要判断CODE_CHECK_RANGE是否有匹配
		arg1 = PARAM(1);  //range size
		arg2 = PARAM(3);  //charset size
		charset = (wchar *)(code + 5);
		for(int i = 0; i < arg1; i += 2)
		{
			if(check >= charset[i] && check <= charset[i+1])
			{
_success:
				return 1;
			}
		}
		charset = (wchar *)(charset + arg1);
		for(int j = 0; j < arg2; j++)
		{
			if(check == charset[j])
			{
				goto _success;
			}
		}
		return 0;
	case CODE_CHECK_NOT_CHARSET:
		//CODE_CHECK_CHARSET匹配到以后要判断CODE_CHECK_RANGE是否有匹配
		arg1 = PARAM(1);  //range size
		arg2 = PARAM(3);  //charset size
		charset = (wchar *)(code + 5);
		for(int i = 0; i < arg1; i += 2)
		{
			if(check >= charset[i] && check <= charset[i+1])
			{
				return 0;
			}
		}
		charset = (wchar *)(charset + arg1);
		for(int j = 0; j < arg2; j++)
		{
			if(check == charset[j])
			{
				return 0;
			}
		}
		goto _success;
	case CODE_CHAR:
		if(PARAM(1) != check)
		{
			return 0;
		}
		goto _success;
	case CODE_REF:
		arg1 = *(code+1)-1;  //向后引用
		current = group_record[arg1];
		if(*(wchar *)current->pos != check)
		{
			return 0;
		}
		goto _success;
	case CODE_ALL:
		if(check == '\n')
		{
			return 0;
		}
		goto _success;
	case CODE_MATCH_BOUNDARY:
		if(ISWCHARWORD(check))
		{
			return 0;
		}
		goto _success;
	case CODE_MATCH_NOT_BOUNDARY:
		if(!ISWCHARWORD(check))
		{
			return 0;
		}
		goto _success;
	case CODE_MATCH_DIGITAL:
		if(!ISWCHARNUM(check))
		{
			return 0;
		}
		goto _success;
	case CODE_MATCH_NOT_DIGITAL:
		if(ISWCHARNUM(check))
		{
			return 0;
		}
		goto _success;
	case CODE_MATCH_WORD:
		if(!ISWCHARWORD(check))
		{
			return 0;
		}
		goto _success;
	case CODE_MATCH_NOT_WORD:
		if(ISWCHARWORD(check))
		{
			return 0;
		}
		goto _success;
	case CODE_MATCH_SPACE:
		if(!ISWCHARSPACE(check))
		{
			return 0;
		}
		goto _success;
	case CODE_MATCH_NOT_SPACE:
		if(ISWCHARSPACE(check))
		{
			return 0;  //可以偷懒了
		}
		goto _success;
	}
	return 1;
}

typedef struct codeinfo{
	char *name;
	int argoffset,unit;
};

codeinfo regcode[] = {
{"CODE_ALTERNATION",3,2},
{"CODE_REPEAT_GREEDY",5,2},
{"CODE_REPEAT_GREEDY_WITH_ONE",7,2},
{"CODE_REPEAT_GREEDY_BOTH",11,2},
{"CODE_REPEAT_GREEDY_CONDITION",5,2},
{"CODE_REPEAT_LAZY",5,2},
{"CODE_REPEAT_LAZY_WITH_ONE",7,2},
{"CODE_REPEAT_LAZY_BOTH",11,2},
{"CODE_REPEAT_LAZY_CONDITION",5,2},
{"CODE_CHECK_CHARSET",-1,2}, //5+(arg1+arg2)*2
{"CODE_JUMP",3,2},
{"CODE_CHAR",3,2},
{"CODE_REF",2,1},
{"CODE_ALL",1,0},
{"CODE_GROUP_CAPTURE_BEGIN",2,1},
{"CODE_GROUP_CAPTURE_END",2,1},
{"CODE_MATCH_BOUNDARY",1,0},
{"CODE_MATCH_NOT_BOUNDARY",1,0},
{"CODE_MATCH_DIGITAL",1,0},
{"CODE_MATCH_NOT_DIGITAL",1,0},
{"CODE_MATCH_WORD",1,0},
{"CODE_MATCH_NOT_WORD",1,0},
{"CODE_MATCH_SPACE",1,0},
{"CODE_MATCH_NOT_SPACE",1,0},
{"CODE_EXIT",1,0},
{"CODE_UPDATE_ALTERENATION",3,2},
{"CODE_REPEAT_RESET",3,2},
{"CODE_CHECK_NOT_CHARSET",-1,2}
};
//在repeat指令处理中，要尽可能的减少pushstate通过预执行下一条指令（如果吓一条是简单的比较指令则不需要pushstate）



//reg_match中的malloc和free函数不要有，这个这个很拖慢速度的
int reg_match(object_regular or,wchar *source,int begin,int end)
{
//设置一个
#define _TEST *(source + scan)
#define _SKIP(step) scan += step;
#define RECALL \
	do { \
		stack_pop_flag = 1; \
		current = popstate(match_state);  \
		if(current){ \
			isgreedy = current->isgreedy; code = current->pos; \
			scan = current->index; goto backtobegin; \
		} \
		else goto exit; \
	}while(0);
#define IFEND if(scan > end) {RECALL;}
#define NEXT  IFEND check = source[scan++];
#define PUSHSTATE(offset,isgreedy,backup)  \
			do{ \
				char *test = repeat_backup_set[backup]; \
				if(test_next(test,source[scan],group_record)) { \
					pushstate(match_state,scan,code + offset,isgreedy); \
				} \
			}while(0);
#define PUSHSTATE_REPEAT(offset,duplicate,isgreedy) \
			do{/*只有在一轮repeat后scan有移动才考虑pushstate，这是整个正则引擎中最有疑点的*/\
				if(repeat_for_duplicate_record[duplicate] < scan) { \
					repeat_for_duplicate_record[duplicate] = scan; \
					PUSHSTATE(offset,isgreedy,duplicate); \
				}\
			}while(0);
	char *code = or->code->code;
	int scan = begin;
	int max_match_pos = begin;
	int repeat_times = 0;
	int arg1,arg2,arg3;
	int *match_record = or->match_record;
	int *repeat_for_duplicate_record = or->repeat_for_duplicate_record;
	for(int i = 0; i < or->repeat_item_count; i++)	match_record[i] = 0;
	
	char **repeat_backup_set = (char **)or->repeat_predict_set->element;

	for(int i = 0; i < or->repeat_count; i++) repeat_for_duplicate_record[i] = 0;

	wchar check;
	wchar *charset;
	collection match_state = or->match_state;
	match_state->size = 0;
	state *group_record = or->group;

	//标志着当前是从stack中pop出来的repeat单元，主要是为了防止次数限制重复的repeat操作
	int stack_pop_flag = 0;
	int isgreedy = 1;

	//匹配只有两个准则，对于repeat要将整个匹配结果尽可能长，而alternation是需要将选择最长的item即可
	//current_sate即表示当前在哪个最外层的repeat context下的
	state current;
	char *codebase = or->code->code;
	int16_t *jump_set = or->or_jump_set;

	int match_size;
	int ret = 0;
	int isjumpflag = 0;
	//只要匹配失败就pop重来，到每次repeat匹配成功都要push一次，因为repeat一般有两种匹配方向
	//这两种匹配方向分别是：继续repeat还是跳出repeat
	//对于greedy再次数允许范围内尽可能尝试继续repeat，lazy模式在次数允许范围内尽可能尝试跳出repeat
	//所以他们pushstate的操作相差很大。

	while(1)
	{
backtobegin:
		switch(*code)
		{
		case CODE_UPDATE_ALTERENATION:
			//更新选择匹配的最大值
			arg1 = PARAM(1);
			arg2 = jump_set[arg1];
			code = codebase + arg2;
			continue ;
		case CODE_ALTERNATION:
			//保护下一次选择匹配的位置
			pushstate(match_state,scan,code+PARAM(1),0);//将下一个入栈
			NEXTOP(3);
		case CODE_JUMP:
			code -= PARAM(1);  //跳转
			isjumpflag = 1;
			continue ;
		case CODE_REPEAT_RESET:
			match_record[PARAM(1)] = 0;
			NEXTOP(3);
		case CODE_REPEAT_GREEDY:
		case CODE_REPEAT_GREEDY_CONDITION:
			//每次将repeat的匹配历史保存以免每次都是在同样的context下repeat
			PUSHSTATE_REPEAT(PARAM(1),PARAM(3),1)
			//由于是无上界的贪婪匹配，所以每次不需要判断直接进入循环
			NEXTOP(5);
		case CODE_REPEAT_GREEDY_WITH_ONE:
			arg1 = PARAM(1);  //recordindex
			if(!stack_pop_flag && match_record[arg1] == 0) 
			{
				match_record[arg1] = 1;
			}
			else
			{
				PUSHSTATE_REPEAT(PARAM(3),PARAM(5),1);
			}
			NEXTOP(7);
		case CODE_REPEAT_GREEDY_BOTH:
			arg3 = PARAM(1);
			repeat_times = match_record[arg3];
			arg1 = PARAM(3);
			arg2 = PARAM(5);
			if(!stack_pop_flag) {
				if(repeat_times >= arg1 && repeat_times <= arg2)
				{
					match_record[arg3] = repeat_times+1;
				}
				else if(repeat_times > arg2)
				{
					code += PARAM(7);
					continue ;
				}
				else
				{
					PUSHSTATE_REPEAT(PARAM(7),PARAM(9),1);
				}
			}
			else
				PUSHSTATE_REPEAT(PARAM(7),PARAM(9),1);
			NEXTOP(11);
		case CODE_REPEAT_LAZY:
		case CODE_REPEAT_LAZY_CONDITION:
			arg1 = PARAM(1);
			PUSHSTATE_REPEAT(0,PARAM(3),0);
			if(isjumpflag) {
				isjumpflag = 0;
				code += arg1;
				continue ;
			}
			NEXTOP(5);
		case CODE_REPEAT_LAZY_WITH_ONE:
			arg1 = PARAM(1);  //recordindex
			if(!stack_pop_flag && match_record[arg1] == 0)
			{
				match_record[arg1] = 1;
			}
			else
			{
				PUSHSTATE_REPEAT(0,PARAM(5),0);
				if(isjumpflag) {
					isjumpflag = 0;
					code += PARAM(3);
					continue ;
				}
			}
			NEXTOP(7);
		case CODE_REPEAT_LAZY_BOTH:
			arg3 = PARAM(1);
			repeat_times = match_record[arg3];
			arg1 = PARAM(3);
			arg2 = PARAM(5);
			if(repeat_times >= arg2)
			{
				code += PARAM(7);
				continue ;
			}
			else if(repeat_times >= arg1)
			{
				PUSHSTATE_REPEAT(0,PARAM(9),0);
				if(isjumpflag) {
					isjumpflag = 0;
					code += PARAM(7);
					continue ;
				}
			}
			match_record[arg3] = repeat_times+1;
			NEXTOP(11);
		case CODE_CHECK_NOT_CHARSET:
			NEXT;
			arg1 = PARAM(1); //range size
			arg2 = PARAM(3); //charset size
			charset = (wchar *)(code + 5);
			for(int i = 0; i < arg1; i += 2)
			{
				if(check < charset[i] || check > charset[i+1])
				{
					goto complete_notcharset;
				}
			}
			charset = (wchar *)(charset + arg1);
			for(int j = 0; j < arg2; j++)
			{
				if(check != charset[j])
				{
					goto complete_notcharset;
				}
			}
			RECALL;
complete_notcharset:
			NEXTOP(5 + (arg1 + arg2) * 2);
		case CODE_CHECK_CHARSET:
			//CODE_CHECK_CHARSET匹配到以后要判断CODE_CHECK_RANGE是否有匹配
			NEXT;
			arg1 = PARAM(1); //range size
			arg2 = PARAM(3); //charset size
			charset = (wchar *)(code + 5);
			for(int i = 0; i < arg1; i += 2)
			{
				if(check >= charset[i] && check <= charset[i+1])
				{
					goto complete_charset;
				}
			}
			charset = (wchar *)(charset + arg1);
			for(int j = 0; j < arg2; j++)
			{
				if(check == charset[j])
				{
					goto complete_charset;
				}
			}
			RECALL;
complete_charset:
			NEXTOP(5+(arg1*2+arg2)*2);
		case CODE_CHAR:
			NEXT;
			if(PARAM(1) != check)
			{
				RECALL;
			}
			NEXTOP(3);
		case CODE_REF:
			arg1 = *(code+1)-1;  //向后引用
			current = group_record[arg1];
			if(!compare_w_char((wchar *)current->pos,source+scan,current->index))
			{
				RECALL;
			}
			_SKIP(current->index);
			NEXTOP(2);
		case CODE_ALL:
			NEXT;
			if(check == '\n')
			{
				RECALL;
			}
			NEXTOP(1);
		case CODE_GROUP_CAPTURE_BEGIN://捕获字符串
			current = group_record[*(code+1)];
			current->index = 0;
			current->pos = (char *)(source+scan);
			NEXTOP(2);
		case CODE_GROUP_CAPTURE_END://捕获字符串
			current = group_record[*(code+1)];
			current->index = scan - ((wchar *)current->pos-source);
			NEXTOP(2);
		case CODE_MATCH_BOUNDARY:
			IFEND;
			check = source[scan];
			if(ISWCHARWORD(check))
			{
				RECALL;
			}
			NEXTOP(1);
		case CODE_MATCH_NOT_BOUNDARY:
			IFEND;
			check = source[scan];
			if(!ISWCHARWORD(check))
			{
				RECALL;
			}
			NEXTOP(1);
		case CODE_MATCH_DIGITAL:
			NEXT;
			if(!ISWCHARNUM(check))
			{
				RECALL;
			}
			NEXTOP(1);
		case CODE_MATCH_NOT_DIGITAL:
			NEXT;
			if(ISWCHARNUM(check))
			{
				RECALL;
			}
			NEXTOP(1);
		case CODE_MATCH_WORD:
			NEXT;
			if(!ISWCHARWORD(check))
			{
				RECALL;
			}
			NEXTOP(1);
		case CODE_MATCH_NOT_WORD:
			NEXT;
			if(ISWCHARWORD(check))
			{
				RECALL;
			}
			NEXTOP(1);
		case CODE_MATCH_SPACE:
			NEXT;
			if(!ISWCHARSPACE(check))
			{
				RECALL;
			}
			NEXTOP(1);
		case CODE_MATCH_NOT_SPACE:
			NEXT;
			if(ISWCHARSPACE(check))
			{
				RECALL;
			}
			NEXTOP(1);
		case CODE_EXIT:
			max_match_pos = max_match_pos > scan ? max_match_pos : scan;
			if(max_match_pos > end)  //表示此时是已经是最大的了
			{
				goto exit;
			}
			stack_pop_flag = 1; 
			current = popstate(match_state);
			while(current){ 
				scan = current->index; 
				code = current->pos; 
				isgreedy = current->isgreedy; 
				if(isgreedy)
					goto backtobegin;
				current = popstate(match_state);
			}
			goto exit; 
		default:
			angel_error("正则引擎---未知指令！");
			goto exit;
		}
next:
		;
	}
exit:
	//清理各种环境
	return max_match_pos - begin;
}

object reg_find(object_regular or,wchar *source,int begin,int end)
{
	int i = begin;
	while(i < end)
	{
		int matchsize = reg_match(or,source,i,end);
		if(matchsize > 0)  //表示没有匹配到
		{
			return (object)initrange(i,i+matchsize-1);
		}
		i++;
	}
	return GETNULL;
}
object reg_findall(object_regular or,wchar *source,int begin,int end)
{
	int i = begin;
	object_list ret = initarray();

	while(i < end)
	{
		//为了考虑减少时间开销，可以通过各种方式减少reg_match函数的调用次数
		int matchsize = reg_match(or,source,i,end);
		if(matchsize > 0)  //表示没有匹配到
		{
			object_range item = initrange(i,i+matchsize-1);
			//
			_addlist(ret,(object)item);
		}
		i += (matchsize == 0 ? 1 : matchsize);
		//printf("%d\n",i);
	}
	
	return (object)ret;
}


/*
regular库
*/
object syscode_regular(object o)
{
	bytecode b = GETREGULAR(o)->code;
	int scan = 0;
	char info[errorsize];
	sprintf(info,"正则表达式的字节码总共%d个字节\n",b->len);
	angel_out(info);
	char *code = b->code;
	while(code < b->code + b->len)
	{
		uchar inst = *code - 1;
		char *name = regcode[inst].name;
		int offset = regcode[inst].argoffset;
		int unit = regcode[inst].unit;

		if(offset == -1) {
			offset = 5 + (PARAM(1) * 2 + PARAM(3)) * 2;
		}
		sprintf(info, "%d:\t%s\n", scan, name);
		angel_out(info);
		code += offset;
	}
	return GETNULL;
}