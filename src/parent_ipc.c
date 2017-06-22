/*************************************************************************
	> File Name: parent_ipc.cpp
	> Author: wk
	> Mail: 18402927708@163.com
	> Created Time: Sat 10 Jun 2017 05:18:44 PM CST
 ************************************************************************/

#include"parent_ipc.h"
#include"config.h"
#include<sys/types.h>
#include<pwd.h>
#include"ipc.h"
#include <unistd.h>
#include"kftpd_log.h"
#include <linux/capability.h>

/*
fuction: tcp_client
success return 0
failed   return -1
*/
static int tcp_client(unsigned short port)
{
    int sock=0;
    if((sock = socket(AF_INET,SOCK_STREAM,0)) < 0)
    {
        exit(EXIT_FAILURE);;
    }
    if(port > 0)
    {
        int on=1;
        if(setsockopt(sock,SOL_SOCKET,SO_REUSEADDR,(const char*)&on,sizeof(on)) <0)
        {
           exit(EXIT_FAILURE);
        }
        //char ip[16]={'\0'};
        //getlocalip(ip);
        char ip[16]="121.42.180.114 ";
        struct sockaddr_in localaddr;
        memset(&localaddr,0,sizeof(localaddr));
        localaddr.sin_family = AF_INET;
        localaddr.sin_port=htons(port);
        localaddr.sin_addr.s_addr=inet_addr(ip);
        if(bind(sock,(struct sockaddr*)&localaddr,sizeof(localaddr)) <0)
        {
            exit(EXIT_FAILURE);
        }
    }
    return sock;
}


static void privparent_port_get_data_sock(session_t *sess)
{

    //nobody进程接受ftp进程发送过来的端口号
    int tmp_fd;
    priv_sock_get_int(sess->parent_fd,&tmp_fd);
    unsigned short port = (unsigned short)tmp_fd;
   // printf("port=%u\n",port);
    //接受ip
    char ip[16]={'\0'};
    priv_sock_get_str(sess->parent_fd,ip,sizeof(ip));

  //  printf("port=%u  ip=%s\n",port,ip);
    //创建数据连接
    struct sockaddr_in addr;
    memset(&addr,0,sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);
    addr.sin_addr.s_addr = inet_addr(ip);
    //socket->connect
    int fd =tcp_client(20); //连接20端口此时没有权限，需要nobody进程协助绑定20端口 服务器作为客户端连接
    if(fd == -1)
    {
        priv_send_result(sess->parent_fd,PRIV_SOCK_RESULT_BAD);
        return ;
    }
    int ret=0;
    while((ret=connect_timeout(fd,sess->port_addr,kftpd_connect_timeout))< 0)
    {
        if(errno == ETIMEDOUT)
        {
            printf("connect_timeout\n");
            continue;
        }
        priv_send_result(sess->parent_fd,PRIV_SOCK_RESULT_BAD);
        close(fd);
        return;
    }
    priv_send_result(sess->parent_fd,PRIV_SOCK_RESULT_OK);
    priv_sock_send_fd(sess->parent_fd,fd);
    close(fd);
}
static void privparent_pasv_active(session_t *sess)
{
    int active=0;
    if(sess->pasv_listenfd != -1)
    {
        active = 1;
    }
    priv_sock_send_int(sess->parent_fd,active);
}
static void privparent_pasv_listen(session_t *sess)
{
    char ip[16]={'\0'};
    getlocalip(ip);
    int fd= tcp_server(ip,0);
    struct sockaddr_in addr;
    socklen_t addrlen =sizeof(addr);
    if(getsockname(fd,(struct sockaddr*)&addr,&addrlen) <0 )
    {
        exit(EXIT_FAILURE);;
    }
    sess->pasv_listenfd=fd;
    unsigned short port = ntohs(addr.sin_port);
    priv_sock_send_int(sess->parent_fd,(int)port);

}
static void privparent_pasv_accept(session_t *sess)
{
    int conn=0;
    while((conn=accept_timeout(sess->pasv_listenfd,NULL,kftpd_accept_timeout)) < 0)
    {   
        if(errno == ETIMEDOUT)
        {
            printf("accept_timeout\n");
            continue;
        }
        priv_send_result(sess->parent_fd,PRIV_SOCK_RESULT_BAD);
        close(sess->parent_fd);
        sess->parent_fd=-1;
        close(conn);
        return ;
    }
    priv_send_result(sess->parent_fd,PRIV_SOCK_RESULT_OK);
    priv_sock_send_fd(sess->parent_fd,conn);
    close(sess->parent_fd);
    sess->parent_fd=-1;
    close(conn);

}

static void minimize_privilege()
{
    //nobody 进程  辅助ftp服务进程与客户端建立的数据连接 nobody只是协助在内部使用
    //在于客户端建立连接之后会将连接成功的文件描述符传给ftp服务器进程
    //比如为了绑定20端口 普通用户是没有权限绑定的
    struct passwd *pw = getpwnam("nobody"); //父进程改名为nassistant

	if(pw == NULL)
    {	
        //TODO log
        kftpd_print_log(LOG_ERROR,"getpwnam() failed",__FILE__,__FUNCTION__,__LINE__);
        exit(EXIT_FAILURE);; 
    }
    if(setegid(pw->pw_gid)<0)//有效组id改成  pw->pw_gid
    {
        //TODO log
        kftpd_print_log(LOG_ERROR,"setegid() failed",__FILE__,__FUNCTION__,__LINE__);
        exit(EXIT_FAILURE);
    }
    if(seteuid(pw->pw_uid)<0)//有效用户id改成  pw->pw_uid
    {
        //TODO log
        kftpd_print_log(LOG_ERROR,"seteuid() failed",__FILE__,__FUNCTION__,__LINE__);
      exit(EXIT_FAILURE);
    }
    //让进程拥有绑定众所周知端口的方法
    struct __user_cap_header_struct  cap_header;
    struct __user_cap_data_struct  cap_data;

    cap_header.version = _LINUX_CAPABILITY_VERSION_2;
    cap_header.pid = 0;
    __u32 cap_mask=0;
    cap_mask |= (1<<CAP_NET_BIND_SERVICE);
    cap_data.effective = cap_data.permitted = cap_mask;
    cap_data.inheritable = 0;
   //capset(&cap_header,&cap_data);
}

void handle_parent(session_t *sess)
{
    minimize_privilege();

    char cmd='\0';
    while(1)
    {
        //解析子进程发送过来的内部命令协助子进程完成
        priv_get_cmd(sess->parent_fd,&cmd);
        switch(cmd)
        {
            case PRIV_SOCK_GET_DATA_SOCK:
                privparent_port_get_data_sock(sess);
                break;
            case PRIV_SOCK_PASV_ACTIVE:
                privparent_pasv_active(sess);
                break;
            case PRIV_SOCK_PASV_LISTEN:
                privparent_pasv_listen(sess);
                break;
            case PRIV_SOCK_PASV_ACCEPT:
                privparent_pasv_accept(sess);
                break;
        }
    }
}
