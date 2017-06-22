/*************************************************************************
	> File Name: kftpd_log.h
	> Author: wk
	> Mail: 18402927708@163.com
	> Created Time: Tue 06 Jun 2017 11:06:56 PM CST
 ************************************************************************/

#ifndef _KFTPD_LOG_H
#define _KFTPD_LOG_H


#define LOG_FATAL       0     // 严重错误
#define LOG_ERROR       1     // 一般错误
#define LOG_WARN        2     // 警告
#define LOG_INFO        3     // 一般信息
#define LOG_TRACE       4     // 跟踪信息
#define LOG_DEBUG       5     // 调试信息
#define LOG_ALL         6     // 所有信息

void kftpd_print_log(unsigned int longlevel, char*str,char *file,const char*function,unsigned int line);
void GetTime(char *timestr);


#endif
