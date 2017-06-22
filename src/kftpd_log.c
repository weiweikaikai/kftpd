/*************************************************************************
	> File Name: kftpd_log.cpp
	> Author: wk
	> Mail: 18402927708@163.com
	> Created Time: Tue 06 Jun 2017 11:06:37 PM CST
 ************************************************************************/

#include"kftpd_log.h"


#include <time.h>
#include <sys/time.h>
#include<unistd.h>
#include<stdio.h>
#include<sys/types.h>
#include<sys/stat.h>
#include<fcntl.h>
#include<string.h>
#include"memory_pool.h"
#include"comm.h"


void GetTime(char*timestr)
{
	 if (timestr == NULL)
    {
     exit(EXIT_FAILURE); //退出进程
    }
    struct tm      tSysTime     = {0};
    struct timeval tTimeVal     = {0};
    time_t         tCurrentTime = {0};

   char szUsec[20]={'\0'}; // 微秒
   char szMsec[20]={'\0'};; // 毫秒
   
    tCurrentTime = time(NULL);
    localtime_r(&tCurrentTime, &tSysTime);   // localtime_r是线程安全的

    gettimeofday(&tTimeVal, NULL);    
    sprintf(szUsec, "%06d", tTimeVal.tv_usec);  // 获取微秒
    strncpy(szMsec, szUsec, 3);                // 微秒的前3位为毫秒(1毫秒=1000微秒)

    sprintf(timestr, "[%04d.%02d.%02d %02d:%02d:%02d.%3.3s]", 
            tSysTime.tm_year+1900, tSysTime.tm_mon+1, tSysTime.tm_mday,
            tSysTime.tm_hour, tSysTime.tm_min, tSysTime.tm_sec, szMsec);
}

void kftpd_print_log(unsigned int longlevel,char*str,char *file,const char*function,unsigned int line)
{
    char log_str[512]={'\0'}; 
    char timestr[20]={'\0'}; 
    GetTime(timestr);
    sprintf(log_str,"\nSystemTime:%s  FileName:[%s]  FunctionName:[%s]  Line:[%u]\n %s\n",timestr,file, function,line,str);
 
   size_t len=strlen(log_str);
   int fd=open("../log/kftp_log",O_RDWR|O_CREAT|O_APPEND,S_IWUSR|S_IWGRP|S_IRUSR);
   write(fd,log_str,len); //输出到文件中
}
