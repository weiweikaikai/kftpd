/*************************************************************************
	> File Name: config.h
	> Author: wk
	> Mail: 18402927708@163.com
	> Created Time: Sat 10 Jun 2017 09:38:13 AM CST
 ************************************************************************/

#ifndef _CONFIG_H
#define _CONFIG_H
#include<string.h>
#include<stdio.h>


//如果配置文件中没有配置这些值就会使用默认值
extern unsigned int kftpd_pasv_enable;
extern unsigned int kftpd_port_enable;
extern unsigned int kftpd_listen_port;
extern unsigned int kftpd_max_clients;
extern unsigned int kftpd_max_per_ip;
extern unsigned int kftpd_accept_timeout;
extern unsigned int kftpd_connect_timeout;
extern unsigned int kftpd_upload_max_rate; 
extern unsigned int kftpd_download_max_rate;
extern unsigned int kftpd_add_user_enable;
extern unsigned int kftpd_del_user_enable;
extern unsigned int kftpd_session_timeout;


#endif
