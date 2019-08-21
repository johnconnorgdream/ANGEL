#include <stdlib.h>
#include "data.h"
#include "initext.h"


ext_type file_type,socket_type,thread_type,parse_type,time_type;

void initexttype()
{
	file_type = (ext_type)calloc(1,sizeof(ext_typenode));
	socket_type = (ext_type)calloc(1,sizeof(ext_typenode));
	thread_type = (ext_type)calloc(1,sizeof(ext_typenode));
	time_type = (ext_type)calloc(1,sizeof(ext_typenode));
	parse_type = (ext_type)calloc(1,sizeof(ext_typenode));

	file_type->dealloc = filedealloc;
	file_type->type_name = "resouce<file>";
	
	socket_type->dealloc = socketdealloc;
	initsock();
	socket_type->type_name = "resouce<socket>";



	thread_type->type_name = "resouce<thread>";
	thread_type->dealloc = threaddealloc;


	time_type->type_name = "resouce<time>";

	parse_type->type_name = "resouce<parse>";
}