/*************************************************************************
	> File Name: string_operate.cpp
	> Author: wk
	> Mail: 18402927708@163.com
	> Created Time: Tue 06 Jun 2017 11:17:39 PM CST
 ************************************************************************/


#include"string_operate.h"
#include<string.h>

//去除\r\n
void str_trim_ctrl(char *str)
{
   if(str == NULL)
   {
	   return;
   }
   char *pcur = &str[strlen(str)-1];
   if(*pcur == '\r' || *pcur == '\n')
	{
	   *pcur--='\0';
	   if(*pcur == '\r' || *pcur == '\n')
		{
	   *pcur='\0';
	    }
    }
	//printf("hahahh: [%s]\n",str);
}

//字符串分割
void str_split(char *str,char *cmd,char *arg,char c)
{
	if(str == NULL)
	{

	 return ;
	}
     char *p = strchr(str,c); //查找出现C字符的位置
	if(p == NULL) //如果没有C字符空格，命令没有参数
	{
	   strcpy(cmd,str); 
	}
	else //找到了C字符出现的字符
	{
	  strncpy(cmd,str,p-str);//拷贝命令
	  strcpy(arg,p+1);//拷贝参数
	}
}
//判断是否全部是空白字符
int isall_space(const char *str)
{
	while(*str != '\0')
	{
	   if(*str != ' ')
		{
	     return 0;
	   }
	   str++;
	}
   return 1;
}
//字符串转化为大写格式
void str_upper(char *str)
{
	if(str == NULL)
	{
	 return ;
	}
   while(*str != '\0')
	{
      *str &=~0x20;
	  str++;
    }
}
//字符串转化为小写格式
void str_lower(char *str)
{
	if(str == NULL)
	{
	 return ;
	}
   while(*str != '\0')
	{
      *str |= 0x20;
	   str++;
    }
}
//将字符串转化为long long 的整数
long long str_to_longlong(const char *str)
{
	if(str == NULL)
	{
		return 0;
	}
    long long result=0;
	unsigned int len = strlen(str);
	unsigned int i=0;
	if(len > 15)
	{
	  return 0;
	}
	for(i=0;i<len;++i)
	{
		if(str[i] <'0' || str[i] >'9')
		{
		 return result;
		}
		
	  result = result*10 +str[i]-'0';
	}
	return result;
}
//将字符串(八进制)转化为无符号正型
unsigned int str_octal_to_uint(const char*str)
{
  if(str == NULL)
	{
    return 0;
    }
    long long result=0;
	unsigned int len = strlen(str);
	unsigned int i=0;
	for(i=1;i<len;++i)
	{
		if(str[i] <'0' || str[i] >'7')
		{
		 return result;
		}
		
	  result = result*8 +str[i]-'0';
	}
	return result;
}