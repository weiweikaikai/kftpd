/*************************************************************************
	> File Name: ipc.h
	> Author: wk
	> Mail: 18402927708@163.com
	> Created Time: Sat 10 Jun 2017 05:17:23 PM CST
 ************************************************************************/

#ifndef _IPC_H
#define _IPC_H


#include"session.h"
#include"net_connect.h"

//#define str_cmp(m, c0, c1, c2, c3)\ 
 //    *(unsigned int *)m == ((c3 << 24) | (c2 << 16) | (c1 << 8) | c0)

//内部进程自定义通信协议
//用于ftp进程和nobody进程进行通信

//FTP服务器向nobody进程请求的命令
#define PRIV_SOCK_GET_DATA_SOCK    1

#define PRIV_SOCK_PASV_ACTIVE      2
#define PRIV_SOCK_PASV_LISTEN      3
#define PRIV_SOCK_PASV_ACCEPT      4

//nobody进程对FTP服务进程的应答
#define PRIV_SOCK_RESULT_OK        1
#define PRIV_SOCK_RESULT_BAD       2


//初始化内部进程间通信通道
void priv_sock_init(session_t *sess);
//关闭内部进程间通信通道
void priv_sock_close(session_t *sess);
//设置父进程环境
void priv_sock_set_parent_context(session_t *sess);
//设置子进程环境
void priv_sock_set_child_context(session_t *sess);

//子进程-->父进程发送命令  ftp->nobody
void priv_send_cmd(int fd,char cmd);
//父进程<--子进程接受命令  nobody<-ftp
void priv_get_cmd(int fd,char *cmd);
//父进程-->子进程发送结果  nobody->ftp
void priv_send_result(int fd,char result);
//子进程<--父进程接受结果  ftp<-nobody
void priv_get_result(int fd,char *result);

//发送一个整数
void priv_sock_send_int(int fd,int the_int);
//接受一个整数
void priv_sock_get_int(int fd,int *the_int);
//发送一个字符串
void priv_sock_send_str(int fd,const char*str,unsigned int strlen);
//接受一个字符串
void priv_sock_get_str(int fd,char*str,unsigned int strlen);
//发送文件描述符
void priv_sock_send_fd(int fd,int descfd);
//接受文件描述符
void priv_sock_get_fd(int fd,int *descfd);


#endif
