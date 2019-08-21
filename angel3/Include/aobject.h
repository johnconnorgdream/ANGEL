#ifndef object_def
#define object_def
#ifdef _cplusplus
extern "c"{
#endif

typedef struct object_dictnode _object_dict;
typedef struct object_setnode _object_set;
typedef struct tokennode _token;
typedef struct funnode _fun;
typedef struct object_entrynode entry;
typedef struct object_stringnode _string;

typedef struct classnode{
	//��ļ̳���Ϣȫ���������¼
	char *name;
	_token *entry_token;
	_object_dict* static_value; //�����Ǿ�̬��Ա�������б�
	struct funlistnode *static_f,*mem_f;
	_object_set* static_f_map,*mem_f_map;
}*pclass;
typedef struct objectnode{  
	//����ʱ�������ڲ���ϵͳ�Ķ��ڴ������ĸ�������16λ���ܱ�ʾ�������ǲű�ϵͳ����
	BASEDEF
	pclass c;
	_object_dict *mem_value,*pri_mem_value;  //��Ա�������б�ķ���ʵ��,֮���Բ��þ�̬��������Ϊ����Ҫ�����Զ�����������ᣬ����ڳ���������ȷ������
	//pri_mem_value��˽�г�Ա����
}*object;
typedef struct classlistnode{
	pclass c[runtime_max_size];
	uint16_t len;
}*classlist;

object initobject();
object stacktoheap(object i);

//���ҳ�Ա����
_fun * getobjmemfun(object o,char *name,int count = -1);
_fun * getclassfun(pclass c,char *name,uint16_t count);
_fun * getobjmemfun(object o,char *name,int count);
_fun * getmemfun(object o,char *name,uint16_t count);
entry * getobjmember(object o,_string * name);
entry * getclassmember(pclass c,_string * name);
entry * getmember(object o,_string * name);

#ifdef _cplusplus
}
#endif
#endif