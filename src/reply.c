/*************************************************************************
	> File Name: reply.cpp
	> Author: wk
	> Mail: 18402927708@163.com
	> Created Time: Sat 10 Jun 2017 04:48:37 PM CST
 ************************************************************************/
 
#include"reply.h"
#include"comm.h"
#include"reply_code.h"
#include <sys/time.h>
#include"config.h"

void send_fd(int sock_fd,int fd)
{
    int ret = 0;
    struct msghdr msg;
    struct cmsghdr *p_cmsg;
    struct iovec vec;
    char cmsgbuf[CMSG_SPACE(sizeof(fd))];
    int *p_fds;
    char sendchar = 0;

    msg.msg_control = cmsgbuf;
    msg.msg_controllen = sizeof(cmsgbuf);
    p_cmsg= CMSG_FIRSTHDR(&msg);
    p_cmsg->cmsg_level = SOL_SOCKET;
    p_cmsg->cmsg_type = SCM_RIGHTS;
    p_cmsg->cmsg_len = CMSG_LEN(sizeof(fd));
    p_fds = (int*)CMSG_DATA(p_cmsg);
    *p_fds =fd;

    msg.msg_name =NULL;
    msg.msg_namelen = 0;
    msg.msg_iov = &vec;
    msg.msg_iovlen = 1;
    msg.msg_flags = 0;

    vec.iov_base = &sendchar;
    vec.iov_len = sizeof(sendchar);
    ret = sendmsg(sock_fd,&msg,0);
    if(ret != 1)
    {
       exit(0);
    } 
}
int recv_fd(const int sock_fd)
{
    int ret =0;
    struct msghdr msg;
    char recvchar;
    struct iovec vec;
    int recvfd;
    char cmsgbuf[CMSG_SPACE(sizeof(recvfd))];
    struct cmsghdr *p_cmsg;
    int *p_fd;
    vec.iov_base = &recvchar;
    vec.iov_len = sizeof(recvchar);
    msg.msg_name = NULL;
    msg.msg_namelen =0;
    msg.msg_iov = &vec;
    msg.msg_iovlen =-1;
    msg.msg_control = cmsgbuf;
    msg.msg_controllen = sizeof(cmsgbuf);
    msg.msg_flags=0;

    p_fd = (int*)CMSG_DATA(CMSG_FIRSTHDR(&msg));
    *p_fd = -1;

    ret = recvmsg(sock_fd,&msg,0);
    if(ret != 1)
    {
          exit(0);
    }
    return recvfd;
}

static int lock_internal(int fd,int locktype)
{
    struct flock the_lock;
    memset(&the_lock,'\0',sizeof(the_lock));
    the_lock.l_type = locktype;
    the_lock.l_whence =SEEK_SET;//从哪开始
    the_lock.l_start =0; //加锁开始的偏移
    the_lock.l_len =0; //加锁的字节数
    int flag;
    do
    {
        flag = fcntl(fd,F_SETLKW,&the_lock);
    }
    while ( flag < 0 && errno == EINTR);
    return flag;
}
int lock_file_read(int fd)
{
    return lock_internal(fd,F_RDLCK);
}
int lock_file_write(int fd)
{
    return lock_internal(fd,F_WRLCK);
}
int unlock_file(int fd)
{
    struct flock the_lock;
    memset(&the_lock,'\0',sizeof(the_lock));
    the_lock.l_type = F_UNLCK;
    the_lock.l_whence =SEEK_SET;//从哪开始
    the_lock.l_start =0; //加锁开始的偏移
    the_lock.l_len =0; //加锁的字节数
    int flag=fcntl(fd,F_SETLK,&the_lock);
    return flag;
}

static struct timeval curr_time;
long get_now_time_of_sec()
{
    if(gettimeofday(&curr_time,NULL) < 0)
    {
          exit(0);
    }
    return curr_time.tv_sec;
}
long get_now_time_of_usec()
{
    return curr_time.tv_usec;
}


void nano_sleep(double sleep_time)
{
    time_t secs = (time_t)sleep_time; //整数部分
    double fractional = sleep_time -(double)secs; //小数部分
    struct timespec ts;
    ts.tv_sec = secs;
    ts.tv_nsec =(long)(fractional * (double)1000000000);
    int ret=0;
    do
    {
        ret =nanosleep(&ts,&ts);

    }while(ret == -1 && errno == EINTR);
}

//开启套接字fd接受带外数据的功能
void active_oobinline(int fd)
{
    int on =1;
    int ret = setsockopt(fd,SOL_SOCKET,SO_OOBINLINE,(const char*)&on,sizeof(on));
    if(ret == -1)
    {
          exit(0);
    }
}


//开启接受带外数据的23号信号 
//设定当前进程可以接受fd文件描述符所产生的信号
void active_sigurg(int fd)
{
    //F_SETOWN
    int ret = fcntl(fd,F_SETOWN,getpid());
    if(ret == -1)
    {
         exit(0);
    }
}
//连接数的限制
void check_limit_numofclient(session_t *sess)
{
    if(kftpd_max_clients > 0)
    {
        if(sess->num_clients > kftpd_max_clients)
        {
            ftp_reply(sess,FTP_TOO_MANY_USERS,"two many connected user please try later\n");
            exit(0);
        }
    }

    if(kftpd_max_per_ip > 0)
    {
        if(kftpd_max_per_ip > kftpd_max_clients)
        {
            ftp_reply(sess,FTP_TOO_MANY_USERS,"two many connected user please try later\n");
            exit(0);
        }
        if(sess->num_ip >kftpd_max_per_ip )
        {
            ftp_reply(sess,FTP_TOO_MANY_IP,"two many connected user form your internet address");
            exit(0);
        }
    }

}

//返回响应码函数1
void ftp_reply(session_t *sess,int status,const char *text)
{
    char buf[1024]={'\0'};
    sprintf(buf,"%d %s\r\n",status,text);
    writen(sess->ctrl_fd,buf,strlen(buf));
}
//返回响应码函数2
void ftp_reply_(session_t *sess,int status,const char *text)
{
    char buf[1024]={'\0'};
    sprintf(buf,"%d-%s\r\n",status,text);
    writen(sess->ctrl_fd,buf,strlen(buf));
}
void ftp_reply_str(session_t *sess,const char*text)
{
    char buf[1024]={'\0'};
    sprintf(buf,"%s\r\n",text);
    writen(sess->ctrl_fd,buf,strlen(buf));
}
