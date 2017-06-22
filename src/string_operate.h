/*************************************************************************
	> File Name: string_operate.h
	> Author: wk
	> Mail: 18402927708@163.com
	> Created Time: Tue 06 Jun 2017 11:17:49 PM CST
 ************************************************************************/

#ifndef _STRING_OPERATE_H
#define _STRING_OPERATE_H

//去除\r\n
void str_trim_ctrl(char *str);
//字符串分割
void str_split(char *str,char *cmd,char *arg,char c);
int isall_space(const char *str);
//字符串转化为大写格式
void str_upper(char *str);
//字符串转化为小写格式
void str_lower(char *str);
//将字符串转化为long long
long long str_to_longlong(const char *str);
//将字符串(八进制)转化为无符号正型
unsigned int str_octal_to_uint(const char*str);

#endif
