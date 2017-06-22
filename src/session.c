/*************************************************************************
	> File Name: session.cpp
	> Author: wk
	> Mail: 18402927708@163.com
	> Created Time: Sat 10 Jun 2017 08:43:18 AM CST
 ************************************************************************/

#include"session.h"
#include<stdio.h>
#include<string.h>
#include<unistd.h>
#include<sys/types.h>
#include"parent_ipc.h"
#include"ipc.h"
#include"kftpd_log.h"
#include"reply.h"
#include"parse_ftpproto.h"


////会话变量
//session_t sess =
//{
//    //uid
//    0,
//    //控制连接套接字
//    -1,
//    //命令行字符串  命令 参数
//    "","","",
//    //数据套接字
//    NULL,-1,-1,
//    //限速
//    0,0,0,0,
//    //父子进程通道
//    -1,-1,
//    //FTP协议状态
//    0, 0,NULL,0,0,
//    //连接数限制
//    0,0,
//    //已经连接客户端的ip
//    ""
//};
void init_session(session_t *sess)
{
    //uid
    sess->uid=0;
    //控制连接套接字
    sess->ctrl_fd=-1; 
    //控制连接
    strcpy(sess->cmdline,""); //命令行
    strcpy(sess->cmd,"");//命令行
    strcpy(sess->arg,"");//参数
    //限速
    sess->upload_max_rate=0;
    sess->dowload_max_rate=0;
    sess->start_transfer_time_sec=0;
    sess->start_transfer_time_usec=0;
    //数据连接套接字
    sess->port_addr=NULL;
    sess->pasv_listenfd=-1;
    sess->datasock=-1;
    //父子进程通道
    sess->parent_fd=-1; //unix域的套接字
    sess->child_fd=-1;
    //FTP协议状态
    sess->is_ascii=0;
    sess->restart_pos=0;//用于断点续传
    sess->rnfr_name=NULL;
    sess->abor_recived=0;
    //是否传输数据 下载或者上传
    sess->isdata_transfer=0;
    //连接数限制
    sess->num_clients=0;
    sess->num_ip=0;
    //客户端的ip地址
    strcpy(sess->online_client_ip,"");
}

void begin_session(session_t *sess)
{
    priv_sock_init(sess); //父子之间进行通信的socketpair 
    active_oobinline(sess->ctrl_fd);
    pid_t pid =fork();
    if(pid < 0)
    {
        //TODO log
        kftpd_print_log(LOG_ERROR,"fork failed",__FILE__,__FUNCTION__,__LINE__);
        exit(0);
    }
    if(pid == 0) //子进程   
    {
        //ftp服务进程 处理FTP相关协议的进程 控制连接和数据连接
        priv_sock_set_child_context(sess);
        handle_child(sess);  
    }
    else //父进程
    {
        //协助子进程进行相关权限的管理
        priv_sock_set_parent_context(sess);
			
        handle_parent(sess);
		//printf("-------\n");
    }
    priv_sock_close(sess);
}

