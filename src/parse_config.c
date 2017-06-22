/*************************************************************************
	> File Name: parse_config.cpp
	> Author: wk
	> Mail: 18402927708@163.com
	> Created Time: Tue 06 Jun 2017 11:09:14 PM CST
 ************************************************************************/


#include"parse_config.h"
#include"kftpd_log.h"
#include"memory_pool.h"
#include"string_operate.h"
#include"config.h"
#include<string.h>
#include"comm.h"
#include<stdlib.h>


void load_kftpd_conf(const char*path)
{
    if(path == NULL)
    {
       kftpd_print_log(LOG_WARN,"path arg is NULL",__FILE__,__FUNCTION__,__LINE__);
       return ;
    }
    FILE *fp = fopen(path,"r");
    if(fp == NULL)
    {
        kftpd_print_log(LOG_WARN,"fopen fucntion fp is NULL",__FILE__,__FUNCTION__,__LINE__);
        exit(0);
    }
	int cur=0;
    char setting_line[1024]={'\0'};
    while( fgets(setting_line,sizeof(setting_line),fp) != NULL)
    {
        if(strlen(setting_line) == 0 || setting_line[0]=='#'
                || isall_space(setting_line) )
        {
            continue;
        }
        str_trim_ctrl(setting_line);
      //  printf("%s--\n",setting_line);
        parseconf_load_setting(setting_line,cur);
		cur++;
        memset(setting_line,0,sizeof(setting_line));
    }
}

void load_add_user_conf(const char*path)
{
    if(path == NULL)
    {
       kftpd_print_log(LOG_WARN,"path arg is NULL",__FILE__,__FUNCTION__,__LINE__);
       return ;
    }
    FILE *fp = fopen(path,"r");
    if(fp == NULL)
    {
        kftpd_print_log(LOG_WARN,"fopen fucntion fp is NULL",__FILE__,__FUNCTION__,__LINE__);
        exit(0);
    }
    char setting_line[1024]={'\0'};
    while( fgets(setting_line,sizeof(setting_line),fp) != NULL)
    {
        if(strlen(setting_line) == 0 || setting_line[0]=='#'
                || isall_space(setting_line) )
        {
            continue;
        }
        //printf("%s\n",setting_line);
        char key[128]={'\0'};
        char value[128]={'\0'};
        str_split(setting_line,key,value,'=');
		char command[1024]={'\0'};
        sprintf(command,"adduser -d /home/%s -p %s %s",key,value,key);
      
		char *cur= command;
		while(*cur!='\0' ){
		   if(*cur == '\n')
			{
			 *cur=' ';
			 break;
			}
		    cur++;
		}
		system(command);
        memset(setting_line,0,sizeof(setting_line));
		memset(key,0,sizeof(key));
		memset(value,0,sizeof(value));
    }

}
void load_del_user_conf(const char*path)
{
    if(path == NULL)
    {
       kftpd_print_log(LOG_WARN,"path arg is NULL",__FILE__,__FUNCTION__,__LINE__);
       return ;
    }
    FILE *fp = fopen(path,"r");
    if(fp == NULL)
    {
        kftpd_print_log(LOG_WARN,"fopen fucntion fp is NULL",__FILE__,__FUNCTION__,__LINE__);
        exit(0);
    }
    char setting_line[1024]={'\0'};
    while( fgets(setting_line,sizeof(setting_line),fp) != NULL)
    {
        if(strlen(setting_line) == 0 || setting_line[0]=='#'
                || isall_space(setting_line) )
        {
            continue;
        }
        //printf("%s\n",setting_line);
       char command[1024]={'\0'};
        sprintf(command,"userdel -rf %s",setting_line);
      
	    //printf("%s\n\n",command);
		system(command);
        printf("----\n");
        memset(setting_line,0,sizeof(setting_line));
	    memset(command,0,sizeof(command));
	
    }

}

void parseconf_load_setting(char* setting,int cur)
{

    if(setting == NULL)
    {
		kftpd_print_log(LOG_WARN,"setting arg is NULL",__FILE__,__FUNCTION__,__LINE__);
        return ;
    }
    char key[128]={'\0'};
    char value[128]={'\0'};
    str_split(setting,key,value,'=');
 //  printf("key:%s  value: %s\n",key,value);
	 int tmp=(int)str_to_longlong(value);
	 switch(cur)
	{
	 case 0:
		 kftpd_pasv_enable=tmp;
		 break;
     case 1:
		 kftpd_port_enable=tmp;
		 break;
     case 2:
		 kftpd_listen_port=tmp;
		 break;
	 case 3:
		 kftpd_max_clients=tmp;
		 break;
	 case 4:
		 kftpd_max_per_ip=tmp;
		 break;
	 case 5:
		 kftpd_accept_timeout=tmp;
		 break;
	 case 6:
		 kftpd_connect_timeout=tmp;
		 break;
	 case 7:
		 kftpd_upload_max_rate=tmp;
		 break;
	 case 8:
		 kftpd_download_max_rate=tmp;
		 break;
	 case 9:
		 kftpd_add_user_enable=tmp;
		 break;
	 case 10:
		 kftpd_del_user_enable=tmp;
		 break;
	 case 11:
		 kftpd_session_timeout=tmp;
		 break;

	 default:
		 break;
	 }
    // printf("--%d\n",kftpd_del_user_enable);
}


