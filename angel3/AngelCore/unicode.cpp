#include "typeconfig.h"
#include "unicodeTables.h"
#include <stdlib.h>


int UnicodeToUtf8(char* pInput, char *pOutput)    
{
    int len = 0; //记录转换后的Utf8字符串的字节数 
	uint16_t *punicode = (uint16_t *)pInput;
    while (*punicode)  
    {   
        unsigned  wchar = *punicode;//高8位和低8位组成一个unicode字符,加法运算级别高  
		punicode++;

        if (wchar <= 0x7F ) //英文字符  
        {     
            pOutput[len] = (char)wchar;  //取wchar的低8位  
            len++;  
        }    
        else if (wchar >=0x80 && wchar <= 0x7FF)  //可以转换成双字节pOutput字符  
        {    
            pOutput[len] = 0xc0 |((wchar >> 6)&0x1f);  //取出unicode编码低6位后的5位，填充到110yyyyy 10zzzzzz 的yyyyy中  
            len++;  
            pOutput[len] = 0x80 | (wchar & 0x3f);  //取出unicode编码的低6位，填充到110yyyyy 10zzzzzz 的zzzzzz中  
            len++;  
        }    
        else if (wchar >=0x800 && wchar < 0xFFFF)  //可以转换成3个字节的pOutput字符  
        {    
            pOutput[len] = 0xe0 | ((wchar >> 12)&0x0f);  //高四位填入1110xxxx 10yyyyyy 10zzzzzz中的xxxx  
            len++;  
            pOutput[len] = 0x80 | ((wchar >> 6) & 0x3f);  //中间6位填入1110xxxx 10yyyyyy 10zzzzzz中的yyyyyy  
            len++;  
            pOutput[len] = 0x80 | (wchar & 0x3f);  //低6位填入1110xxxx 10yyyyyy 10zzzzzz中的zzzzzz  
            len++;  
        }
        else //对于其他字节数的unicode字符不进行处理  
        {  
            return -1;  
        }
    }  
    //utf8字符串后面，有个\0
    pOutput [len]= 0;  
    return len;    
}
//utf8转unicode本质上就是填零
int Utf8ToUnicode(char* pInput, char* pOutput)  
{  
    int outputSize = 0; //记录转换后的Unicode字符串的字节数  
  
    while (*pInput)  
    {
        if (*pInput > 0x00 && *pInput <= 0x7F) //处理单字节UTF8字符（英文字母、数字）  
        {  
            *pOutput = *pInput;  
             pOutput++;  
            *pOutput = 0; //小端法表示，在高地址填补0  
        }  
        else if (((*pInput) & 0xE0) == 0xC0) //处理双字节UTF8字符  
        {  
            char high = *pInput;  
            pInput++;  
            char low = *pInput;  
            if ((low & 0xC0) != 0x80)  //检查是否为合法的UTF8字符表示，但这种判断并不能完全的区分gbk和utf8因为gbk可能恰好也是0x80 
            {  
                return -1; //如果不是则报错  
            }
  
            *pOutput = (high << 6) + (low & 0x3F);  
            pOutput++;  
            *pOutput = (high >> 2) & 0x07;  
        }  
        else if (((*pInput) & 0xF0) == 0xE0) //处理三字节UTF8字符  
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
            *pOutput = (middle << 6) + (low & 0x3F);//取出middle的底两位与low的低6位，组合成unicode字符的低8位  
            pOutput++;  
            *pOutput = (high << 4) + ((middle >> 2) & 0x0F); //取出high的低四位与middle的中间四位，组合成unicode字符的高8位  
        }  
        else //对于其他字节数的UTF8字符不进行处理  
        {  
            return -1;  
        }  
        pInput ++;//处理下一个utf8字符  
        pOutput ++;  
        outputSize += 2;  
    }  
    //unicode字符串后面，有两个\0  
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
			//将gbk编码的中文字符的两个字节组合成一个    uint16_t word;
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
		word = *(uni_ptr + uni_ind);  //获得int16_t数据。
		uni_ind++;

		if(word == 0x0000)  //字符串结束符
			break;
		
		if(word < 0x80)  /*ASCII不用查表*/
		{
			*(gbk_ptr + gbk_ind) = (unsigned char)word;
			gbk_ind++;
		}
		else
		{
			word_pos = word - unicode_first_code;
			if(word >= unicode_first_code && word <= unicode_last_code && word_pos < gbk_buf_size)
			{
				gbk_word = gbkTables[word_pos];//gbk_word是gbk编码，但是为uint16_t类型，需要拆分成两个字节

				*(gbk_ptr + gbk_ind) = (unsigned char)(gbk_word >> 8);//提取高8位
				gbk_ind++;
				*(gbk_ptr + gbk_ind) = (unsigned char)(gbk_word >> 0);//提取低8位
				gbk_ind++;
				gbk_num +=2;//gbk字符加2个
			}
		}

		if(gbk_num > max_gbk_buf_size - 1)
			break;
	}

    return gbk_num;
}