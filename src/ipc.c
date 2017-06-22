/*************************************************************************
	> File Name: ipc.c
	> Author: wk
	> Mail: 18402927708@163.com
	> Created Time: Sat 10 Jun 2017 05:17:14 PM CST
 ************************************************************************/


#include"ipc.h"
#include"comm.h"
#include"reply.h"
#include"kftpd_log.h"
#include"parent_ipc.h"


//初始化内部进程间通信通道
void priv_sock_init(session_t *sess)
{
    //内部进程间通道的创建
    int sockfds[2]; //进程间通信用socketpair 
    if(socketpair(PF_UNIX,SOCK_STREAM,0,sockfds) <0)
    {
        //TODO log
        kftpd_print_log(LOG_ERROR,"socketpair failed",__FILE__,__FUNCTION__,__LINE__);
       exit(EXIT_FAILURE);
    }
    sess->parent_fd = sockfds[0];
    sess->child_fd = sockfds[1];
}
//关闭内部进程间通信通道
void priv_sock_close(session_t *sess)
{
    if(sess->parent_fd != -1)
    {
        close(sess->parent_fd);
        sess->parent_fd = -1;
    }
    if(sess->child_fd != -1)
    {
        close(sess->child_fd);
        sess->child_fd = -1;
    }

}
//设置父进程环境
void priv_sock_set_parent_context(session_t *sess)
{         
    if(sess->child_fd != -1)
    {
        close(sess->child_fd);
        sess->child_fd = -1;
    }

}
//设置子进程环境
void priv_sock_set_child_context(session_t *sess)
{
    if(sess->parent_fd != -1)
    {
        close(sess->parent_fd);
        sess->parent_fd = -1;
    }
}

//子进程-->父进程发送命令  ftp->nobody
void priv_send_cmd(int fd,char cmd)
{
    ssize_t ret=0;
    ret = writen(fd,&cmd,sizeof(cmd));
    if(ret != sizeof(cmd))
    {
        //TODO log
        kftpd_print_log(LOG_ERROR,"priv_send_cmd() failed",__FILE__,__FUNCTION__,__LINE__);
        exit(EXIT_FAILURE);
    }

}
//父进程<--子进程接受命令  nobody<-ftp
void priv_get_cmd(int fd,char *cmd)
{
    ssize_t ret=0;
    ret = readn(fd,cmd,sizeof(char));
    if(ret == 0)
    {
        //TODO log
        kftpd_print_log(LOG_ERROR,"kftpd and kftpd's father process is closeing",__FILE__,__FUNCTION__,__LINE__);
        exit(0);
    }
    else if(ret != sizeof(char))
    {
        //TODO log
        kftpd_print_log(LOG_ERROR,"priv_get_cmd() failed",__FILE__,__FUNCTION__,__LINE__);
        exit(EXIT_FAILURE);
    }
}
//父进程-->子进程发送结果  nobody->ftp
void priv_send_result(int fd,char result)
{
    ssize_t ret=0;
    ret = writen(fd,&result,sizeof(result));
    if(ret != sizeof(result))
    {
        fprintf(stderr,"priv_send_result eror\n");
        exit(EXIT_FAILURE);
    }
}
//子进程<--父进程接受结果  ftp<-nobody
void priv_get_result(int fd,char *result)
{
    ssize_t ret=0;
    ret = readn(fd,result,sizeof(char));
    if(ret != sizeof(char))
    {
        fprintf(stderr,"priv_get_result eror\n");
        exit(EXIT_FAILURE);
    }

}

//发送一个整数
void priv_sock_send_int(int fd,int the_int)
{
    printf("int :%d\n",the_int);
    ssize_t ret=0;
    ret = writen(fd,&the_int,sizeof(the_int));
    if(ret != sizeof(the_int))
    {
        fprintf(stderr,"priv_sock_send_int eror\n");
        exit(EXIT_FAILURE);
    }
}
//接受一个整数
void priv_sock_get_int(int fd,int *the_int)
{
    ssize_t ret=0;
    ret = readn(fd,the_int,sizeof(int));
    if(ret != sizeof(int))
    {
        fprintf(stderr,"priv_sock_get_int eror\n");
        exit(EXIT_FAILURE);
    }

}
//发送一个字符串
void priv_sock_send_str(int fd,const char*str,unsigned int strlen)
{
    //发送一个长度
    priv_sock_send_int(fd,(int)strlen);
    //发送实际字符串
    ssize_t ret=0;
    ret = writen(fd,str,strlen);
    if(ret != (int)strlen)
    {
        fprintf(stderr,"priv_sock_send_str eror\n");
       exit(EXIT_FAILURE);
    } 

}
//接受一个字符串
void priv_sock_get_str(int fd,char*str,unsigned int strlen)
{
    //接受一个长度
    int len=0;
    priv_sock_get_int(fd,&len);
    if((unsigned int)len > strlen)
    {
        fprintf(stderr,"too length\n");
        exit(EXIT_FAILURE);
    }
    //接受字符串
    ssize_t ret=0;
    ret = readn(fd,str,len);
    if(ret !=len)
    {
        fprintf(stderr,"priv_sock_get_str eror\n");
        exit(EXIT_FAILURE);
    }

}
//发送文件描述符
void priv_sock_send_fd(int fd,int descfd)
{
    send_fd(fd,descfd);
}
//接受文件描述符
void priv_sock_get_fd(int fd,int *descfd)
{
    *descfd = recv_fd(fd);
}

