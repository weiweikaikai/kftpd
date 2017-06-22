/*************************************************************************
	> File Name: session.h
	> Author: wk
	> Mail: 18402927708@163.com
	> Created Time: Sat 10 Jun 2017 08:43:09 AM CST
 ************************************************************************/

#ifndef _SESSION_H
#define _SESSION_H
#include<unistd.h>
#include<sys/types.h>

typedef struct session
{
    //uid
    uid_t uid;
    //控制连接套接字
    int ctrl_fd; 
    //控制连接
    char cmdline[512]; //命令行
    char cmd[32];//命令行
    char arg[128];//参数
    //限速
    unsigned int upload_max_rate;
    unsigned int dowload_max_rate;
    long start_transfer_time_sec;
    long start_transfer_time_usec;
    //数据连接套接字
    struct sockaddr_in  *port_addr;
    int pasv_listenfd;
    int datasock;
    //父子进程通道
    int parent_fd; //unix域的套接字
    int child_fd;
    //FTP协议状态
    int is_ascii;
    long long restart_pos;//用于断点续传
    char *rnfr_name;
    int abor_recived;
    //是否传输数据 下载或者上传
    int isdata_transfer;
    //连接数限制
    unsigned int num_clients;
    unsigned int num_ip;
    //客户端的ip地址
    char online_client_ip[20];
}session_t;

void init_session(session_t *sess);   
void begin_session(session_t *sess);



#endif
