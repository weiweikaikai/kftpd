/*************************************************************************
	> File Name: reply.h
	> Author: wk
	> Mail: 18402927708@163.com
	> Created Time: Sat 10 Jun 2017 04:48:30 PM CST
 ************************************************************************/

#ifndef _REPLY_H
#define _REPLY_H
#include"net_connect.h"
#include"session.h"


void send_fd(int sock_fd,int fd);
int recv_fd(const int sock_fd);

int lock_file_read(int fd);

int lock_file_write(int fd);
int unlock_file(int fd);

long get_now_time_of_sec();
long get_now_time_of_usec();

void nano_sleep(double sleep_time);

//开启带外数据的传输
void active_oobinline(int fd);
//开启接受带外数据的23号信号
void active_sigurg(int fd);

//连接数的限制
void check_limit_numofclient(session_t *sess);

//给客户端响应
void ftp_reply(session_t *sess,int status,const char *text);
void ftp_reply_(session_t *sess,int status,const char *text);
void ftp_reply_str(session_t *sess,const char*text);

#endif
