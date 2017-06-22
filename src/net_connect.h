/*************************************************************************
	> File Name: net_connect.h
	> Author: wk
	> Mail: 18402927708@163.com
	> Created Time: Tue 06 Jun 2017 11:15:55 PM CST
 ************************************************************************/

#ifndef _NET_CONNECT_H
#define _NET_CONNECT_H

#include<unistd.h>
#include<errno.h>
#include<stdio.h>
#include<stdlib.h>
#include<sys/socket.h>
#include<sys/types.h>
#include<sys/stat.h>
#include<netinet/in.h>
#include<netdb.h>
#include<fcntl.h>
#include<arpa/inet.h>
#include<string.h>
#include<signal.h>
#include<pwd.h>
#include<time.h>
#include<sys/sendfile.h>

int tcp_server(const char *ip,unsigned short port);

int getlocalip(char *ip);  //获取本地IP

void activate_nonblock(int fd); //设置文件描述符为非阻塞
void deactivate_nonblock(int fd);//设置去掉非阻塞属性


//几个阻塞函数的超时函数
int read_timeout(int fd,unsigned int wait_seconds);
int write_timeout(int fd,unsigned int wait_seconds);
int accept_timeout(int fd,struct sockaddr_in *addr,unsigned int wait_seconds);
int connect_timeout(int fd,struct sockaddr_in *addr,unsigned int wait_seconds);


ssize_t readn(int fd,void *buf,size_t count);
ssize_t writen(int fd,const void *buf,size_t count);
ssize_t recv_peek(int sockfd,void *buf,size_t len);
ssize_t readline(int sockfd,void *buf,size_t maxline);

#endif
