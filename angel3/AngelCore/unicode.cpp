#include "typeconfig.h"
#include "unicodeTables.h"
#include <stdlib.h>


int UnicodeToUtf8(char* pInput, char *pOutput)    
{
    int len = 0; //��¼ת�����Utf8�ַ������ֽ��� 
	uint16_t *punicode = (uint16_t *)pInput;
    while (*punicode)  
    {   
        unsigned  wchar = *punicode;//��8λ�͵�8λ���һ��unicode�ַ�,�ӷ����㼶���  
		punicode++;

        if (wchar <= 0x7F ) //Ӣ���ַ�  
        {     
            pOutput[len] = (char)wchar;  //ȡwchar�ĵ�8λ  
            len++;  
        }    
        else if (wchar >=0x80 && wchar <= 0x7FF)  //����ת����˫�ֽ�pOutput�ַ�  
        {    
            pOutput[len] = 0xc0 |((wchar >> 6)&0x1f);  //ȡ��unicode�����6λ���5λ����䵽110yyyyy 10zzzzzz ��yyyyy��  
            len++;  
            pOutput[len] = 0x80 | (wchar & 0x3f);  //ȡ��unicode����ĵ�6λ����䵽110yyyyy 10zzzzzz ��zzzzzz��  
            len++;  
        }    
        else if (wchar >=0x800 && wchar < 0xFFFF)  //����ת����3���ֽڵ�pOutput�ַ�  
        {    
            pOutput[len] = 0xe0 | ((wchar >> 12)&0x0f);  //����λ����1110xxxx 10yyyyyy 10zzzzzz�е�xxxx  
            len++;  
            pOutput[len] = 0x80 | ((wchar >> 6) & 0x3f);  //�м�6λ����1110xxxx 10yyyyyy 10zzzzzz�е�yyyyyy  
            len++;  
            pOutput[len] = 0x80 | (wchar & 0x3f);  //��6λ����1110xxxx 10yyyyyy 10zzzzzz�е�zzzzzz  
            len++;  
        }
        else //���������ֽ�����unicode�ַ������д���  
        {  
            return -1;  
        }
    }  
    //utf8�ַ������棬�и�\0
    pOutput [len]= 0;  
    return len;    
}
//utf8תunicode�����Ͼ�������
int Utf8ToUnicode(char* pInput, char* pOutput)  
{  
    int outputSize = 0; //��¼ת�����Unicode�ַ������ֽ���  
  
    while (*pInput)  
    {
        if (*pInput > 0x00 && *pInput <= 0x7F) //�����ֽ�UTF8�ַ���Ӣ����ĸ�����֣�  
        {  
            *pOutput = *pInput;  
             pOutput++;  
            *pOutput = 0; //С�˷���ʾ���ڸߵ�ַ�0  
        }  
        else if (((*pInput) & 0xE0) == 0xC0) //����˫�ֽ�UTF8�ַ�  
        {  
            char high = *pInput;  
            pInput++;  
            char low = *pInput;  
            if ((low & 0xC0) != 0x80)  //����Ƿ�Ϊ�Ϸ���UTF8�ַ���ʾ���������жϲ�������ȫ������gbk��utf8��Ϊgbk����ǡ��Ҳ��0x80 
            {  
                return -1; //��������򱨴�  
            }
  
            *pOutput = (high << 6) + (low & 0x3F);  
            pOutput++;  
            *pOutput = (high >> 2) & 0x07;  
        }  
        else if (((*pInput) & 0xF0) == 0xE0) //�������ֽ�UTF8�ַ�  
        {  
            char high = *pInput;  
            pInput++;  
            char middle = *pInput;  
            pInput++;  
            char low = *pInput;  
            if (((middle & 0xC0) != 0x80) || ((low & 0xC0) != 0x80))  
            {  
                return -1;  
            }  
            *pOutput = (middle << 6) + (low & 0x3F);//ȡ��middle�ĵ���λ��low�ĵ�6λ����ϳ�unicode�ַ��ĵ�8λ  
            pOutput++;  
            *pOutput = (high << 4) + ((middle >> 2) & 0x0F); //ȡ��high�ĵ���λ��middle���м���λ����ϳ�unicode�ַ��ĸ�8λ  
        }  
        else //���������ֽ�����UTF8�ַ������д���  
        {  
            return -1;  
        }  
        pInput ++;//������һ��utf8�ַ�  
        pOutput ++;  
        outputSize += 2;  
    }  
    //unicode�ַ������棬������\0  
    *pOutput = 0;  
     pOutput++;  
    *pOutput = 0;  
    return outputSize;  
}

//unicode->gbk
int GbkToUnicode(char *gbk_buf, uint16_t *unicode_buf, int max_unicode_buf_size)
{
    uint16_t word;
    unsigned char *gbk_ptr =(unsigned char *) gbk_buf;
    uint16_t *uni_ptr = unicode_buf;
	unsigned int uni_ind = 0, gbk_ind = 0, uni_num = 0;
	unsigned char ch;
    int word_pos;

    if( !gbk_buf || !unicode_buf )
        return -1;

    while(1)
    {
    	ch = *(gbk_ptr + gbk_ind);

		if(ch == 0x00)
			break;
		
        if( ch > 0x80 )
        {
			//��gbk����������ַ��������ֽ���ϳ�һ��    uint16_t word;
			word = *(gbk_ptr + gbk_ind);
			word <<= 8;
			word += *(gbk_ptr + gbk_ind+1);
			gbk_ind += 2;

            word_pos = word - gbk_first_code;
            if(word >= gbk_first_code && word <= gbk_last_code  && (word_pos < unicode_buf_size))
            {
				*(uni_ptr + uni_ind) = unicodeTables[word_pos];
				uni_ind++;
				uni_num++;
            }
        }
		else
		{
			gbk_ind++;
			*(uni_ptr + uni_ind) = ch;
			uni_ind++;
			uni_num++;
        }
        
        if(uni_num > max_unicode_buf_size - 1)
			break;
    }

    return 2*uni_num;
}

//unicode->gbk
int UnicodeToGbk(uint16_t *unicode_buf, char *gbk_buf, int max_gbk_buf_size)
{
	uint16_t word;
	uint16_t gbk_word;
	unsigned char ch;
    unsigned char *gbk_ptr =(unsigned char *) gbk_buf;
    uint16_t *uni_ptr = unicode_buf;
	unsigned int uni_ind = 0, gbk_ind = 0, gbk_num = 0;
    int word_pos;

    if( !gbk_buf || !unicode_buf )
        return -1;

	while(1)
	{
		word = *(uni_ptr + uni_ind);  //���int16_t���ݡ�
		uni_ind++;

		if(word == 0x0000)  //�ַ���������
			break;
		
		if(word < 0x80)  /*ASCII���ò��*/
		{
			*(gbk_ptr + gbk_ind) = (unsigned char)word;
			gbk_ind++;
		}
		else
		{
			word_pos = word - unicode_first_code;
			if(word >= unicode_first_code && word <= unicode_last_code && word_pos < gbk_buf_size)
			{
				gbk_word = gbkTables[word_pos];//gbk_word��gbk���룬����Ϊuint16_t���ͣ���Ҫ��ֳ������ֽ�

				*(gbk_ptr + gbk_ind) = (unsigned char)(gbk_word >> 8);//��ȡ��8λ
				gbk_ind++;
				*(gbk_ptr + gbk_ind) = (unsigned char)(gbk_word >> 0);//��ȡ��8λ
				gbk_ind++;
				gbk_num +=2;//gbk�ַ���2��
			}
		}

		if(gbk_num > max_gbk_buf_size - 1)
			break;
	}

    return gbk_num;
}