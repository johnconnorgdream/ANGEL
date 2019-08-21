#include "_common.h"
#include "_xmlcompile.h"

void free_xml_node(tag head)
{
	if(!head)
		return ;
	free_xml_node(head->neighbor);
	head->neighbor = NULL;
	free_xml_node(head->firstchild);
	head->firstchild = NULL;
	free(head);
}
object_dict parse_xml_kernel(object_dict ret,tag head)
{
	_adddict(ret,(object)CONST("name"),(object)initstring(head->name));
	char *text = head->c->text;
	if(!text)
		_adddict(ret,(object)CONST("text"),GETNULL);
	else if(strcmp(text,"") != 0)
		_adddict(ret,(object)CONST("text"),(object)initstring(head->c->text));
	else
		_adddict(ret,(object)CONST("text"),GETNULL);
	
	object_dict attrs;
	if(head->c->fistattr)
	{
		attrs = initdictionary();
		_adddict(ret,(object)CONST("attrs"),(object)attrs);
	}
	else
	{
		_adddict(ret,(object)CONST("attrs"),GETNULL);
	}
	for(attr a = head->c->fistattr; a; a = a->next)
	{
		object_string key = initstring(a->key);
		object_string value = initstring(a->value);
		_adddict(attrs,(object)key,(object)value);
	}
	object_list children = NULL;
	object_dict child;
	if(head->firstchild)
	{
		children = initarray();
		_adddict(ret,(object)CONST("children"),(object)children);
	}
	else
	{
		_adddict(ret,(object)CONST("children"),GETNULL);
	}
	
	for(tag p = head->firstchild; p; p = p->neighbor)
	{
		child = initdictionary();
		_addlist(children,(object)child);
		parse_xml_kernel(child,p);
		_adddict(child,(object)CONST("parent"),(object)ret);
	}
	return ret;
}
object parse_xml(object str)
{
	int error;
	char *content = tomult(GETSTR(str));
	xmlres x = xmlanalysis(content,&error);
	object_dict parse_res = initdictionary();

	_adddict(parse_res,(object)CONST("type"),(object)CONST(x->type));
	tag  t = x->res;
	object_dict construct = initdictionary();
	_adddict(parse_res,(object)CONST("head"),(object)construct);
	parse_xml_kernel(construct,t);

	free_xml_node(x->res);
	free(x);

	return (object)parse_res;
}
object sysparse(object str,object mode)
{
	if(!ISSTR(str))
	{
		angel_error("parse第一个参数应该为字符串类型！");
		return GETFALSE;
	}
	if(!ISSTR(mode))
	{
		angel_error("parse第二个参数应该为字符串类型！");
		return GETFALSE;
	}
	if(comparestring(GETSTR(mode),CONST("xml"))==0)  //解析xml文件
	{
		return parse_xml(str);
	}
	return GETNULL;
}
