#include "_common.h"
#include "_filesystem.h"


extern ext_type file_type;


void filedealloc(object o)
{
	FILE *f = GETFILE(o)->f;
	fclose(f);
}
object_file initfile(FILE *f)
{
	object_file res = (object_file)initext(APPLYSIZE(sizeof(object_filenode)));
	res->_type = file_type;
	res->f = f;
	return res;
}
inline int getfilesize(FILE *fp)
{
	int orign =  ftell(fp);
	fseek(fp,0L,SEEK_END);
	int filesize = ftell(fp);
	fseek(fp,0L,orign);
	return filesize;
}
__forceinline int encode_format(char *buffer,char **decode_entry)
{
	//优先判断是否是utf8或unicode，如果不是默认按
	char *p = buffer;
	int encodetype;
	*decode_entry = p;
	if((uchar)*p == 0xfe || (uchar)*p == 0xff)  //带BOM的utf-8
	{
		*decode_entry += 2;
		return UTF8_TYPE; //utf-8 with BOM
	}
	else
	{
		if(*p > 0)  
		{
			if(*p > 0x7f)
			{
				if(((*p) & 0xE0) == 0xC0 || ((*p) & 0xF0) == 0xE0)
					return UTF8_TYPE; //utf-8
				else
					return UNICODE_TYPE; //
			}
			return UTF8_TYPE; //utf-8
		}
		else
		{
			return NATIVE_TYPE; //native编码
		}
	}
}
int fread_bytes(FILE *fp,void *buffer,int filesize)
{
	int cow = fread(buffer,filesize,1,fp);
	if(1 < cow)
	{
		angel_error("文件读取出错！");
		return -1;
	}
	return filesize;
}
object sysfopen(object filename,object mode)
{
	ARG_CHECK(filename,STR,"fopen",1)
	ARG_CHECK(mode,STR,"fopen",2)
	char *cmode;
	//模式有rewrite，read，!rewrite,!read,write,!write
	if(!comparestring(GETSTR(mode),CONST("write")))
	{
		cmode = "ab+";
	}
	else if(!comparestring(GETSTR(mode),CONST("!write")))
	{
		cmode = "ab";
	}
	else if(!comparestring(GETSTR(mode),CONST("rewrite")))
	{
		cmode = "wb+";
	}
	else if(!comparestring(GETSTR(mode),CONST("!rewrite")))
	{
		cmode = "wb";
	}
	else if(!comparestring(GETSTR(mode),CONST("read")))
	{
		cmode = "rb+";
	}
	else if(!comparestring(GETSTR(mode),CONST("!read")))
	{
		cmode = "rb";
	}
	else
	{
		angel_error("文件打开模式出错！（请使用rewrite，read，!rewrite,!read,write,!write）");
		return GETNULL;
	}
	char *cfilename = tonative(GETSTR(filename));
	FILE *cf = fopen(cfilename,cmode);
	free(cfilename);
	if(!cf)
	{
	//	angel_error("文件打开失败！");
		return GETNULL;
	}
	return (object)initfile(cf);
}
object sysfread(object file)
{
	FILE *fp = GETFILE(file)->f;
	int filesize = getfilesize(fp);
	void *buffer = (void *)calloc(1,filesize);
	if(!fread_bytes(fp,buffer,filesize))
		return GETNULL;
	object_bytes res = initbytes(filesize);
	memcpy(res->bytes,buffer,filesize);
	free(buffer);
	return (object)res;
}
object sysfgets(object file)
{
	ARG_CHECK(file,FILE,"fgets",1)
	FILE *f = GETFILE(file)->f;
	char *encode_res;
	int filesize = getfilesize(f);
	void *buffer = (void *)calloc(1,filesize+2);
	if(!fread_bytes(f,buffer,filesize))
		return GETNULL;
	int encode = encode_format((char *)buffer,&encode_res);
	object res=NULL;
	switch(encode)
	{
	case UNICODE_TYPE:
		res = (object)copystring_str(encode_res,filesize);
		break ;
	case UTF8_TYPE:
		res = (object)initstring(encode_res);
		break ;
	case NATIVE_TYPE:
		res = (object)initstring_n(encode_res);
		break ;
	}
	//这里需要对buffer做后期处理，这里只做简单的尾零
	free(buffer);
	return res;
}
object sysfwrite(object file,object content)
{
	ARG_CHECK(file,FILE,"fwrite",1)
	if(ISSTR(content))
	{
		char *buffer = tonative(GETSTR(content));
		FILE *f = GETFILE(file)->f;
		if(fputs(buffer,f)<0)
			return GETFALSE;
	}
	else
	{
		angel_error("function[fwrite] argument 2 must be string、bytes、number！");
		return GETFALSE;
	}
	return GETTRUE;
}
object sysfclose(object file)
{
	ARG_CHECK(file,FILE,"fclose",1)
	FILE *f = GETFILE(file)->f;
	if(fclose(f)<0)
		return GETFALSE;
	return GETTRUE;
}

object sysfinfo(object filename)
{
	ARG_CHECK(filename,STR,"finfo",1)
	object_dict ret = _getfileinfo(GETSTR(filename)->s);
	return (object)ret;
}
object sysfls(object filename)
{
	ARG_CHECK(filename,STR,"fls",1)
	return (object)_getls(GETSTR(filename)->s);
}