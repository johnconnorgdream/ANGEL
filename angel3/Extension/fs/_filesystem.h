#ifndef filesystem_def
#define filesystem_def


#ifdef _cplusplus
extern "c"{
#endif

#ifndef COMMON_LIB

//�����涨�������������ص�
typedef struct object_filenode{
	EXT_HEAD
	FILE *f;
}*object_file;
#define GETFILE(o) ((object_file)o)

#endif

//���涨��ͳһ�ӿ�
#define ISFILE(o) (GETEXT(o)->_type == file_type)
#define FILENAME "resource<file>"


object_dict _getfileinfo(char* pathname);
object_list _getls(char* pathname);

void filedealloc(object o);
/*
�⺯��
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