/*************************************************************************
	> File Name: kftpd.cpp
	> Author: wk
	> Mail: 18402927708@163.com
	> Created Time: Tue 06 Jun 2017 11:06:03 PM CST
 ************************************************************************/

#include <unistd.h>
#include <sys/types.h>
#include"memory_pool.h"
#include"comm.h"
#include"kftpd_log.h"
#include"config.h"
#include"session.h"
#include"parse_config.h"
#include"session.h"
#include"kftpd.h"
#include<signal.h>
#include <sys/wait.h>
#include"net_connect.h"
#include"reply.h"


int main()
{
   if(getuid() != 0) //必须是root用户才可以开启服务器，一定程度上保证安全性
    {
        //TODO log  打印日志信息
        kftpd_print_log(LOG_WARN,"kftpd must be start by root user",__FILE__,__FUNCTION__,__LINE__);
       exit(EXIT_FAILURE); //退出进程
    }
    load_kftpd_conf("../config/kftpd.conf"); //加载配置文件
    session_t *p_sess=kftp_malloc(sizeof(session_t));
    init_session(p_sess);//初始化session 
   
	if(kftpd_add_user_enable !=0)
	{
	     load_add_user_conf("../config/add_user_list.conf"); //加载添加用户列表
	}

	if(kftpd_del_user_enable !=0)
	{
         load_del_user_conf("../config/del_user_list.conf"); //加载删除用户列表
	}
	p_sess->upload_max_rate=kftpd_upload_max_rate;
    p_sess->dowload_max_rate=kftpd_download_max_rate;
	//daemon(1,1); //守护进程

	 signal(SIGCHLD,handle_sigchld); //避免僵尸进程的出现
     signal(SIGPIPE,SIG_IGN);
	 ip_num_clients_hash=hash_alloc(256,hash_ip_function);//创建ip和连接数映射的哈希表
     pid_ip_hash=hash_alloc(256,hash_ip_function); //创建进程id和ip映射的哈希表
   

	//TODO log  
    char*str=(char*)kftp_malloc(256); //调用内存池分配内存
    sprintf(str,"service_ip:[121.142.180.114]service_bind_port:[%d] max_client_per_ip_connect:[%d] max_client_total_connect:%d",kftpd_listen_port,kftpd_max_per_ip,kftpd_max_clients);
	kftpd_print_log(LOG_INFO,str,__FILE__,__FUNCTION__,__LINE__); 
    printf("service_ip:[121.142.180.114]service_bind_port:[%d] max_client_per_ip_connect:[%d] max_client_total_connect:%d\n",kftpd_listen_port,kftpd_max_per_ip,kftpd_max_clients);
    int listenfd = tcp_server(NULL,kftpd_listen_port); //bind port 
    int conn=0;
    pid_t pid=0;

    while(1)
    {
        int waittime=5; //timeout 5s
        struct sockaddr_in clientaddr;
        memset(&clientaddr,0,sizeof(clientaddr));
        conn=accept_timeout(listenfd,&clientaddr,waittime);
        if(conn == -1 && errno ==ETIMEDOUT )
        {
			printf("accept timeout\n");
            //TODO log
            kftpd_print_log(LOG_TRACE,"accept_timeout",__FILE__,__FUNCTION__,__LINE__);
            continue;
        }
        else if(conn == -1)
        {
            //TODO log
            kftpd_print_log(LOG_ERROR,"accept failed",__FILE__,__FUNCTION__,__LINE__);
            exit(EXIT_FAILURE);
        }
        unsigned int ip= clientaddr.sin_addr.s_addr;
        p_sess->num_ip=handle_ip_count(&ip);

        ++num_children;
        p_sess->num_clients=num_children;
        pid = fork();
        if(pid == -1)
        {
            --num_children;

            //TODO log
            kftpd_print_log(LOG_ERROR,"fork error",__FILE__,__FUNCTION__,__LINE__); 
            exit(EXIT_FAILURE);
        }
        if(pid == 0) //子进程开启会话
        {
            char*str1=kftp_malloc(64); //调用内存池分配内存
			//TODO log
            sprintf(str1,"client:%s  online\n",inet_ntoa(clientaddr.sin_addr));
            strcpy(p_sess->online_client_ip,(char*)inet_ntoa(clientaddr.sin_addr));
            kftpd_print_log(LOG_ERROR,str1,__FILE__,__FUNCTION__,__LINE__); 
           
            close(listenfd);
            p_sess->ctrl_fd=conn; 
			printf("max_clients:%d  mx_per_ip:%d  num_connecter:%d\n",kftpd_max_clients,kftpd_max_per_ip,num_children);
            check_limit_numofclient(p_sess); //客户端连接数的限制
            signal(SIGCHLD,SIG_IGN);
            begin_session(p_sess);  //将与客户端的连接抽象为一个会话
        }
        else //父进程
        {   
            //ip  和进程id的映射
            hash_add_entry(pid_ip_hash,&pid,sizeof(unsigned int),&ip,sizeof(unsigned int));
            close(conn);
        }

    }

    return 0;
}




//减少每个ip连接数的接口
unsigned int drop_ip_count(void *ip) 
{
    unsigned int count=0;
    unsigned int *conn_count =(unsigned int *)hash_lookuo_enty(ip_num_clients_hash,ip,sizeof(unsigned int));
    if(conn_count == NULL)
    {
        return 0;
    }
    else
    {
        if(--(*conn_count) == 0)
        {
            hash_free_entry(ip_num_clients_hash,ip,sizeof(unsigned int));
        }
    }
    return 0;
}
//SIGCHID信号捕捉函数
void  handle_sigchld(int sig)
{
    pid_t pid;
    while((pid=waitpid(-1,NULL,WNOHANG)) > 0) //非阻塞式
    {
        --num_children;
        unsigned int *ip =(unsigned int *)hash_lookuo_enty(pid_ip_hash,&pid,sizeof(pid));
        if(ip == NULL)
        {
            continue;
        }
        drop_ip_count(ip);
        hash_free_entry(pid_ip_hash,&pid,sizeof(unsigned int));
    }

}
//哈希函数
unsigned int hash_ip_function(unsigned int buckets,void *key) //桶的大小 关键码
{
    unsigned int *number =(unsigned int*)key;
    return(*number) %buckets;
}

//增加和获得单ip连接数的接口
unsigned int handle_ip_count(void *ip) //返回该IP的连接数
{
    unsigned int count=0;
    unsigned int *conn_count =(unsigned int *)hash_lookuo_enty(ip_num_clients_hash,ip,sizeof(unsigned int));
    if(conn_count == NULL)
    {
        count=1;
        hash_add_entry(ip_num_clients_hash,ip,sizeof(unsigned int),&count,sizeof(unsigned int));
    }
    else
    {
        count=*conn_count;
        ++count;
        *conn_count=count;;
    }
    return count;
}