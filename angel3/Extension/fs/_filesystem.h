#ifndef filesystem_def
#define filesystem_def


#ifdef _cplusplus
extern "c"{
#endif

#ifndef COMMON_LIB

//这里面定义与具体领域相关的
typedef struct object_filenode{
	EXT_HEAD
	FILE *f;
}*object_file;
#define GETFILE(o) ((object_file)o)

#endif

//外面定义统一接口
#define ISFILE(o) (GETEXT(o)->_type == file_type)
#define FILENAME "resource<file>"


object_dict _getfileinfo(char* pathname);
object_list _getls(char* pathname);

void filedealloc(object o);
/*
库函数
*/
object sysfopen(object filename,object mode);
object sysfread(object file);
object sysfgets(object file);
object sysfwrite(object file,object content);
object sysfclose(object file);
object sysfinfo(object filename);
object sysfls(object filename);


#ifdef _cplusplus
}
#endif

#endif