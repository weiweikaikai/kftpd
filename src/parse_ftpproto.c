/*************************************************************************
	> File Name: parse_ftpproto.cpp
	> Author: wk
	> Mail: 18402927708@163.com
	> Created Time: Sat 10 Jun 2017 05:15:51 PM CST
 ************************************************************************/

#include"parse_ftpproto.h"
#include"reply_code.h"
#include"config.h"
#include"ipc.h"
#include<shadow.h>
#include<crypt.h>
#include<dirent.h>
#include"comm.h"
#include"reply.h"
#include"kftpd_log.h"
#include"string_operate.h"
#include"kftpd.h"
#include<signal.h>
#include <sys/time.h>

typedef struct Mapftpcmd
{
    const char*cmd;
    void (*cmd_func)(session_t *sess);
}Mapftp_cmd_t; 

//访问控制执行函数
static void do_user(session_t *sess);
static void do_pass(session_t *sess);
static void do_cwd(session_t *sess); //改变目录功能
static void do_cdup(session_t *sess);
static void do_quit(session_t *sess);//退出功能

//传输参数命令 执行函数
static void do_port(session_t *sess);//主动模式功能
static void do_pasv(session_t *sess);//被动模式功能
static void do_type(session_t *sess);//传输文件格式功能
static void do_stru(session_t *sess);
static void do_mode(session_t *sess);

//服务命令执行函数
static void do_retr(session_t *sess);
static void do_stor(session_t *sess);//上传文件功能
static void do_appe(session_t *sess);
static void do_list(session_t *sess);//显示当前目录列表功能
static void do_nlst(session_t *sess);
static void do_rest(session_t *sess);
static void do_abor(session_t *sess);
static void do_pwd(session_t *sess);//显示当前目录功能
static void do_mkd(session_t *sess);//创建目录功能
static void do_rmd(session_t *sess);//删除目录功能
static void do_dele(session_t *sess);//删除文件功能
static void do_rnfr(session_t *sess);
static void do_rnto(session_t *sess);
static void do_site(session_t *sess);
static void do_syst(session_t *sess);//显示服务端系统属性功能
static void do_feat(session_t *sess);
static void do_size(session_t *sess);
static void do_stat(session_t *sess);
static void do_noop(session_t *sess);
static void do_help(session_t *sess);//显示帮助文档功能

static Mapftp_cmd_t cmd_function[]={

    //命令   执行函数

    //访问控制
    {"USER",do_user}, 
    {"PASS",do_pass}, 
    {"CWD",do_cwd},
    {"XCWD",do_cwd},
    {"CDUP",do_cdup},  
    {"XCUP",do_cdup}, 
    {"QUIT",do_quit}, 
    {"ACCT",NULL},
    {"SMNT",NULL},
    {"REIN",NULL},
    //传输参数命令  
    {"PORT",do_port},
    {"PASV",do_pasv},
    {"TYPE",do_type},
    {"STRU",do_stru},
    {"MODE",do_mode},

    //服务命令
    {"RETR",do_retr},
    {"STOR",do_stor},
    {"APPE",do_appe},
    {"LIST",do_list},
    {"NLIST",do_nlst},
    {"REST",do_rest},
    {"ABOR",do_abor},
    {"\377\364\377\362ABOR",do_abor},
    {"PWD",do_pwd},
    {"XPWD",do_pwd},
    {"MKD",do_mkd},
    {"XMKD",do_mkd},
    {"RMD",do_rmd},
    {"XRMD",do_rmd},
    {"DELE",do_dele},
    {"RNFR",do_rnfr},
    {"RNTO",do_rnto},
    {"SITE",do_site},
    {"SYST",do_syst},
    {"FEAT",do_feat},
    {"SIZE",do_size},
    {"STAT",do_stat},
    {"NOOP",do_noop},
    {"HELP",do_help},
    {"STOU",NULL},
    {"ALLO",NULL}      
};

typedef void (*signal_handler)(int);

void  start_handle_alarm_timeout(int signal_num)
{
    shutdown(p_sess->ctrl_fd,SHUT_RD);
    ftp_reply(p_sess,FTP_IDLE_TIMEOUT,"timeout");
    shutdown(p_sess->ctrl_fd,SHUT_WR);
    exit(0);
}



static void start_alarm()
{
    if(kftpd_session_timeout > 0) 
    {
        //安装信号
	  signal_handler p_signal=start_handle_alarm_timeout;
       signal(SIGALRM,p_signal);
        //启动闹钟
        alarm(kftpd_session_timeout);
    }
} 

void rstart_handle_alarm_timeout(int signal_num)
{
    if(p_sess->isdata_transfer == 0) //当前不处于数据传输状态
    {
        ftp_reply(p_sess,FTP_IDLE_TIMEOUT,"Timeout Reconnect sorry");
        exit(0);
    }
    //处于数据传输的状态，收到超时信号
    p_sess->isdata_transfer =0;
    start_alarm();
}

static void restart_alarm(void)
{
    if(kftpd_connect_timeout > 0)
    {
        //安装信号
		signal_handler p1_signal=rstart_handle_alarm_timeout;
        signal(SIGALRM,p1_signal);
        //启动闹钟
        alarm(kftpd_session_timeout);
    }
    else if(kftpd_session_timeout > 0)//先前安装过信号
    {
        //之前安装过闹钟信号就将先前关闭的闹钟关闭
        alarm(0);
    }
}


static void list_dir(session_t *sess)
{
    DIR *dir = opendir(".");
    if(dir == NULL)
    {
        return ;
    }
    struct dirent *dt;
    struct stat sbuf;
    while((dt =readdir(dir)) != NULL)
    {
        if(lstat(dt->d_name,&sbuf) <0) //lstat 和stat 有对软连接状态显示不同的区别
        {
            continue;
        }
        char perm[]="----------";
        perm[0]='?';
        mode_t mode = sbuf.st_mode;
        switch(mode & S_IFMT) //7个文件类型
        {
            case S_IFREG:
                perm[0]='-';
                break;
            case S_IFDIR:
                perm[0]='d';
                break;
            case S_IFLNK:
                perm[0]='l';
                break;
            case S_IFSOCK:
                perm[0]='s';
                break;
            case S_IFBLK:
                perm[0]='b';
                break;
            case S_IFCHR:
                perm[0]='c';
                break;
            case  S_IFIFO :
                perm[0]='p';
                break;

        }
        //9个权限位
        if(mode & S_IRUSR)  
        {
            perm[1]='r';
        }
        if(mode & S_IWUSR )
        {
            perm[2]='w';
        }
        if(mode & S_IXUSR )
        {
            perm[3]='x';
        }


        if(mode & S_IRGRP)  
        {
            perm[4]='r';
        }
        if(mode & S_IWGRP)
        {
            perm[5]='w';
        }
        if(mode & S_IXGRP)
        {
            perm[6]='x';
        }


        if(mode &  S_IROTH)  
        {
            perm[7]='r';
        }
        if(mode &  S_IWOTH)
        {
            perm[8]='w';
        }
        if(mode &  S_IXOTH)
        {
            perm[9]='x';
        }

        //特殊权限位
        if(mode & S_ISUID)
        {
            perm[3] =(perm[3]=='x' ? 's':'S');  
        }
        if(mode & S_ISGID)
        {
            perm[6] =(perm[6]=='x' ? 's':'S'); 
        }
        if(mode & S_ISVTX)
        {
            perm[9] =(perm[9]=='x' ? 't':'T');  
        }

        //硬链接数

        char buf[4096]={'\0'};
        int offset=0;
        offset += sprintf(buf,"%s ",perm);
        offset += sprintf(buf+offset,"%3d %-3d %-3d ",sbuf.st_nlink,sbuf.st_uid,sbuf.st_gid);
        offset += sprintf(buf+offset,"%8llu ",sbuf.st_size);
        struct timeval tv;
        gettimeofday(&tv,NULL);
        time_t  local_time = tv.tv_sec;
        const char *p_data_format="%b %e %H:%M "; 
        if(sbuf.st_mtime> local_time || local_time - sbuf.st_mtime > 182*24*3600)
            //man strftime中有关于时间的格式化
        {
            p_data_format = "%b   %e  %y ";
        }
        //秒转化为结构体
        struct tm *p_tm=localtime(&local_time);
        char datebuf[64]={'\0'};
        strftime(datebuf,sizeof(datebuf),p_data_format,p_tm);
        offset += sprintf(buf+offset,"%s ",datebuf);
        offset += sprintf(buf+offset,"%s",dt->d_name);
        if(S_ISLNK(sbuf.st_mode))
        {
            char tmp[1024]={'\0'};
            readlink(dt->d_name,tmp,sizeof(tmp));
            offset += sprintf(buf+offset,"->%s",tmp);
        }
        sprintf(buf+offset,"\r\n");
        writen(sess->datasock,buf,strlen(buf));
    }
    closedir(dir);
}

static int pasv_active(session_t *sess);

//主动模式被激活
static int port_active(session_t *sess)
{
    if(sess->port_addr != NULL) //创建了套接字，主动模式处于激活状态
    {
        if(pasv_active(sess))
        {
           exit(EXIT_FAILURE);
        }
        return 1;
    }
    return 0; //没有创建套接字

}
static int pasv_active(session_t *sess)
{
    /* priv_send_cmd(sess->child_fd,PRIV_SOCK_PASV_ACTIVE);
       int active=0;
       priv_sock_get_int(sess->child_fd,&active);
       if(active)
       {
       if(port_active(sess))
       {
       ERR_EXIT(EXIT_FAILURE);
       }
       return 1;
       }*/

    if(sess->pasv_listenfd != -1)
    { //被动模式处于激活状态
        if(port_active(sess))
        {
           exit(0);
        }
        return 1;
    }

    return 0;
}

//function:get_port_fd
//success :return 1
//failed :return 0
int get_port_fd(session_t *sess)
{
    unsigned short port = ntohs(sess->port_addr->sin_port);
    char *ip = inet_ntoa(sess->port_addr->sin_addr);
    //向nobody进程发送 PRIV_SOCK_GET_DATA_SOCK命令
    priv_send_cmd(sess->child_fd,PRIV_SOCK_GET_DATA_SOCK);
    //发送端口号
    priv_sock_send_int(sess->child_fd,(int)port);
    //发送ip地址
    priv_sock_send_str(sess->child_fd,ip,strlen(ip));
    //获得nobody响应
    int result=0;
    priv_sock_get_int(sess->child_fd,&result);

    if(result == PRIV_SOCK_RESULT_BAD)
    {
        return 0;
    }
    else if(result == PRIV_SOCK_RESULT_OK)
    {
        //获得nobody进程连接好的文件描述符
        priv_sock_get_fd(sess->child_fd,&(sess->datasock));

    }
    return 1;

}
//function:get_pasv_fd
//success :return 1
//failed :return 0
int get_pasv_fd(session_t *sess)
{

    priv_send_cmd(sess->child_fd,PRIV_SOCK_PASV_ACCEPT);
    int ret=0;
    priv_sock_get_int(sess->child_fd,&ret);
    if(ret == PRIV_SOCK_RESULT_BAD)
    {
        return 0;
    }
    else if(ret == PRIV_SOCK_RESULT_OK)
    {
        priv_sock_get_fd(sess->child_fd,&(sess->datasock));
    }
    return 1;
}

/*
function:get_transport_fd
success retrun 1
failed return 0
*/
static int get_transport_fd(session_t *sess)
{
    //关闭信号量
    restart_alarm();
    //检测是否已经收到PORT 或者PASV
    if(!port_active(sess) && !pasv_active(sess)) //没有创建套接字会返回
    {
        ftp_reply(sess,FTP_BADSENDCONN,"User PORT or PASV first");
        return 0;   //只有主动或者被动模式有一个被激活才可以建立数据连接
    }
    int ret =0;
    //如果是主动模式
    if(port_active(sess))
    {

        ret =get_port_fd(sess);
        if(sess->port_addr) 
        {
            free(sess->port_addr);
            sess->port_addr=NULL;
        }
        if(ret == 0)
        {
            return 0;
        }
    }
    else if(pasv_active(sess))
    {

        int conn=0;
        while((conn=accept_timeout(sess->pasv_listenfd,NULL,kftpd_accept_timeout)) < 0)
        {   
            if(errno == ETIMEDOUT)
            {
                printf("accept_timeout\n");
                continue;
            }
            close(sess->pasv_listenfd);
            return 0;
        }
		
        sess->datasock = conn;
        close(sess->pasv_listenfd);

        /* 
           ret =get_pasv_fd(sess);
           if(ret == 0)
           {
           return 0;
           }*/
    }

    //传输和下载过程中不能超时断开所以要重新安装 sigalarm信号 并启动闹钟
    restart_alarm();
    return 1;

}


//访问控制执行函数
//user指令执行函数
static void do_user(session_t *sess)
{
    //USER weikai
    struct passwd  *pw=getpwnam(sess->arg);
    if(pw == NULL) //用户不存在
    {
        ftp_reply(sess,FTP_LIGINERR,"username is failed");
        return ;
    }
    sess->uid = pw->pw_uid;
    ftp_reply(sess,FTP_GIVEPWORD,"Please specify the password");

}
void handle_sigurg(int signal_num)
{
    if(p_sess->isdata_transfer == 0)
    {
        return ;
    }
    char cmdline[1024]={'\0'};
    int ret =readline(p_sess->ctrl_fd,cmdline,1024);
    if(ret <=0 )
    {
        exit(0);
    }
    //去除\r\n
    str_trim_ctrl(cmdline);
    if(strcasecmp(cmdline,"ABOR") == 0
            || strcasecmp(cmdline,"\377\364\377\362ABOR"))
    {
        p_sess->abor_recived = 1;
        //断开数据连接通道
        shutdown(p_sess->datasock,SHUT_RDWR);;
    }
    else
    {
        ftp_reply(p_sess,FTP_BADCMD,"Unknown command");  
    }

}
//PASS指令执行函数
static void do_pass(session_t *sess)
{
    //PASS 123456
    struct passwd  *pw=getpwuid(sess->uid);
    if(pw == NULL) //用户不存在
    {
        ftp_reply(sess,FTP_LIGINERR,"username is failed");
        return ;
    }
    //获取影子文件信息 shadow
    struct spwd *sp = getspnam(pw->pw_name);
    if(sp == NULL) //影子文件不存在
    {
        ftp_reply(sess,FTP_LIGINERR,"passwd is failed");
        return ;
    }
    //将明文进行加密与影子文件中加密的进行比较
    char *encryted_pass=crypt(sess->arg,sp->sp_pwdp); //进行加密
    if(strcasecmp(encryted_pass,sp->sp_pwdp)==0) //加密后进行比较
    {
        ftp_reply(sess,FTP_LOGINOK,"login successful");
    }
    else
    {
        ftp_reply(sess,FTP_LIGINERR,"passwd is failed");
        return ;
    }
    //能够接受带外数据信号的23号信号
	signal_handler p_signal=handle_sigurg;
    signal(SIGURG,p_signal);
    active_oobinline(sess->ctrl_fd);

    umask(077);
    setegid(pw->pw_gid);
    seteuid(pw->pw_uid);
    chdir(pw->pw_dir);

}

static void do_cwd(session_t *sess) //更改目录
{
    char dest_path[1024]={'\0'};
    char *cur=sess->arg;
    int i=0;
    while(*cur != '\0')
    {
        if(*cur != ' ')
        {
            dest_path[i++]=*cur;
        }
        cur++;
    }
    if(chdir(dest_path) <0)
    {
        ftp_reply(sess,FTP_FILEFAILD,"Failed  Directory changed");
        return ;
    }
    ftp_reply(sess,FTP_CWDOK,"Directory successfully changed");
}
static void do_cdup(session_t *sess) //返回上层目录
{
    if(chdir("..") <0)
    {
        ftp_reply(sess,FTP_FILEFAILD,"Failed  Directory changed");
        return ;
    }
    ftp_reply(sess,FTP_CWDOK,"Directory successfully changed");
}
static void do_quit(session_t *sess)
{
    ftp_reply(sess,FTP_GOODBY,"Goodbye");
    exit(0);
}

//传输参数命令 执行函数 主动模式
static void do_port(session_t *sess)
{
    unsigned int v[6];
	printf("----%s---\n",sess->arg);
    sscanf(sess->arg,"%u,%u,%u,%u,%u,%u",&v[2],&v[3],&v[4],&v[5],&v[0],&v[1]);
    sess->port_addr = (struct sockaddr_in *)malloc(sizeof(struct sockaddr));

    memset(sess->port_addr,'\0',sizeof(struct sockaddr_in));
    sess->port_addr->sin_family = AF_INET;
    unsigned char *p = (unsigned char*)&(sess->port_addr->sin_port);
    p[0]=v[0];
    p[1]=v[1];
   // printf("port=%d\n",ntohs(sess->port_addr->sin_port));
    p= (unsigned char*)&(sess->port_addr->sin_addr);
    p[0]=v[2];
    p[1]=v[3];
    p[2]=v[4];
    p[3]=v[5];
    //sess->port_addr =NULL;
    // printf("%s\n",inet_ntoa(sess->port_addr->sin_addr));
    ftp_reply(sess,FTP_PORTOK,"PORT command successful Consider using PASV");
}
//被动模式
static void do_pasv(session_t *sess)
{
    char ip[16]={'\0'};
    getlocalip(ip);

    int fd= tcp_server(ip,0);
    struct sockaddr_in addr;
    socklen_t addrlen =sizeof(addr);
    if(getsockname(fd,(struct sockaddr*)&addr,&addrlen) <0 )
    {
       exit(EXIT_FAILURE);
    }
    sess->pasv_listenfd=fd;
    unsigned short port = ntohs(addr.sin_port);
    /*
       priv_send_cmd(sess->child_fd,PRIV_SOCK_PASV_LISTEN);
       int tmp=0;
       priv_sock_get_int(sess->child_fd,&tmp);
       unsigned short port=(unsigned short)tmp;
       */
    unsigned int v[4];
    sscanf(ip,"%u.%u.%u.%u",&v[0],&v[1],&v[2],&v[3]);
    char text[1024]={'\0'};
    sprintf(text,"Entering Passsive Mode (%u,%u,%u,%u,%u,%u).\r\n",v[0],v[1],v[2],v[3],port>>8,port&0xFF);
    //printf("%s\n",text);
    ftp_reply(sess,FTP_PASVOK,text);
}
static void do_type(session_t *sess)
{
    if(strcasecmp(sess->arg,"A") == 0) //转为ASCII
    {
        sess->is_ascii=1;
        ftp_reply(sess,FTP_TYPEOK,"Swiching to ASCII mode");
    }
    else if(strcasecmp(sess->arg,"I") == 0)//转为 二进制
    {
        sess->is_ascii=0;
        ftp_reply(sess,FTP_TYPEOK,"Swiching to Binary mode");
    }
    else
    {
        ftp_reply(sess,FTP_BADCMD,"Bad cmd");
    }
}
static void do_stru(session_t *sess){}
static void do_mode(session_t *sess){}

static void limit_rate(session_t *sess,int nbytes,int isupload)
{
    long cur_sec=get_now_time_of_sec();
    long cur_usec=get_now_time_of_usec();

    //当前传输时间
    double elapsed=(double)(cur_sec - sess->start_transfer_time_sec);
    elapsed += (double)(cur_usec-sess->start_transfer_time_usec)/(double)1000000;
    if(elapsed <= (double)0) //突然速度加快
    {
        elapsed=0.01;
    }
    //计算当前传输速度
    unsigned int cur_rate =(unsigned int)((double)nbytes/elapsed);
    //当前传输速度取整所以变小，所以导致睡眠时间变小

    double  rate_ratio;
    if(isupload)
    {
        if(cur_rate <= sess->upload_max_rate)
        {
            //不需要限速
            sess->start_transfer_time_sec=get_now_time_of_sec();
            sess->start_transfer_time_usec=get_now_time_of_usec();
            return;
        }
        rate_ratio = cur_rate/sess->upload_max_rate;//最大速度和当前速度的比率
    }

    //限速最大睡眠时间
    //睡眠时间=(当前速度/最大速度   -1) *当前传输时间
    double sleep_time=(rate_ratio -(double)1 )* elapsed;

    nano_sleep(sleep_time);//睡眠一定时间

    //sleep 参数必须为整数 这个不是整数 内部会使用时钟信号实现会出现错误
    sess->start_transfer_time_sec=get_now_time_of_sec();
    sess->start_transfer_time_usec=get_now_time_of_usec();
}

//服务命令执行函数
static void do_retr(session_t *sess)
{
    //下载 retr   断点下载 rest  retr
    char dest_path[1024]={'\0'};
    char *cur=sess->arg;
    int i=0;
    while(*cur != '\0')
    {
        if(*cur != ' ')
        {
            dest_path[i++]=*cur;
        }
        cur++;
    }
    //创建数据连接
    if(get_transport_fd(sess)==0)
    {
        return ;
    }
    long long offset = sess->restart_pos;
    sess->restart_pos=0;
    //下载文件首先要打开文件
    int fd = open(dest_path,O_RDONLY);
    if(fd == -1)
    {
        ftp_reply(sess,FTP_FILEFAILD,"File open failed");
        return;
    }
    //加读锁  在读的过程中其他进程可以读但是不可以写
    int ret =lock_file_read(fd);
    if(ret < 0)
    {
        ftp_reply(sess,FTP_FILEFAILD,"Lock failed");
        return ;
    }
    //判断是否是普通文件
    struct stat sbuf;
    ret = fstat(fd,&sbuf);
    if(!S_ISREG(sbuf.st_mode))
    {
        ftp_reply(sess,FTP_FILEFAILD,"Is not Regular file");
        return ;
    }
    if(offset != 0)  //断点续载
    {
        ret = lseek(fd,offset,SEEK_SET);
        if(ret == -1)
        {
            ftp_reply(sess,FTP_FILEFAILD,"lseek failed");
            return ;
        }
    }
    char text[1024]={'\0'};
    if(sess->is_ascii)
    {
        sprintf(text,"Open Ascii mode data for %s (%lld)bytes",dest_path,(long long)sbuf.st_size);
    }
    else
    {
        sprintf(text,"Open Binary mode data for %s (%lld)bytes",dest_path,(long long)sbuf.st_size);
    }
    ftp_reply(sess,FTP_DATACONN,text);
    //下载文件
    long long bytes_to_send = (long long )sbuf.st_size;
    if(offset >  bytes_to_send)
    {
        bytes_to_send = 0;
    }
    else
    {
        bytes_to_send -= offset;
    }
    sess->start_transfer_time_sec=get_now_time_of_sec();
    sess->start_transfer_time_usec=get_now_time_of_usec();
    int flag =0;
    while(bytes_to_send > 0)
    {
        int num_this=bytes_to_send > 65536 ? 65536: bytes_to_send;
        ret = sendfile(sess->datasock,fd,NULL,bytes_to_send); //内核中进行不会被信号中断
        if(ret == -1)
        {
            flag = 1;
            break;
        }
        //下载限速
        limit_rate(sess,ret,0);
        if(sess->abor_recived) //abor命令
        {
            flag = 1;
            break;
        }
        bytes_to_send -= ret;
    }

    /*//此处系统调用多次进行内核陷入会影响传输的效率，用户态不需要这些数据,只需要知道是否传输完毕
      char buf[4096]={'\0'};
      while(1)
      {
      ret = readn(fd,buf,sizeof(buf));
      if(ret == 0)
      {
      break;
      }
      int nwrite = writen(sess->datasock,buf,ret);
      if(nwrite != ret)
      {
      flag =1;
      break;
      }

      }*/
    //关闭数据套接字
    close(sess->datasock); //告诉对方数据传输结束
    sess->datasock=-1;
    close(fd);

    if(flag == 0)
    {
        ftp_reply(sess,FTP_TRANSFEROK,"Teansfer complete");
    }
    else
    {
        ftp_reply(sess,FTP_BADSENDFAILED,"Write failed");
    }
    //判读abor是不是刚好在数据传输完毕的时候到达
    if(sess->abor_recived)
    {
        sess->abor_recived =0;
        ftp_reply(sess,FTP_TRANSFEROK,"Teansfer complete");
    }
    //上传完成之后重新开启控制连接通道
    start_alarm();
}


static void upload_comm(session_t *sess,int is_append)
{
    char dest_path[1024]={'\0'};
    char *cur=sess->arg;
    int i=0;
    while(*cur != '\0')
    {
        if(*cur != ' ')
        {
            dest_path[i++]=*cur;
        }
        cur++;
    }
    //创建数据连接
    if(get_transport_fd(sess) == 0)
    {
        return ;
    }
    long long offset = sess->restart_pos;
    sess->restart_pos=0;
    //给客户端传送文件首先要打开文件
    int fd = open(sess->arg,O_CREAT | O_WRONLY,0666);
    if(fd == -1)
    {
        ftp_reply(sess,FTP_UPLODEFAILED,"File open failed");
        return;
    }
    //加写锁  上传文件不允许读或者写
    int ret =lock_file_write(fd);
    if(ret < 0)
    {
        ftp_reply(sess,FTP_UPLODEFAILED,"Lock failed");
        return ;
    }
    if(!is_append && offset == 0) //stor
    {
        ftruncate(fd,0) ;//文件长度清零
        ret =lseek(fd,0,SEEK_SET);
        if(ret == -1)
        {
            ftp_reply(sess,FTP_FILEFAILD,"lseek failed");
            return ;
        }
    }
    else if(!is_append && offset != 0)//REST + stor
    {
        ret =lseek(fd,offset,SEEK_SET);
        if(ret == -1)
        {
            ftp_reply(sess,FTP_FILEFAILD,"lseek failed");
            return ;
        }
    }
    else if(is_append)
    {
        ret =lseek(fd,0,SEEK_END);
        if(ret == -1)
        {
            ftp_reply(sess,FTP_FILEFAILD,"lseek failed");
            return ;
        }
    }
    struct stat sbuf;
    ret = fstat(fd,&sbuf);
    char text[1024]={'\0'};
  //  if(sess->is_ascii)
  // {
   //     sprintf(text,"Open Ascii mode data for %s (%lld)bytes",dest_path,(long long)sbuf.st_size);
  //  }
  //  else
  //  {
  //      sprintf(text,"Open Binary mode data for %s (%lld)bytes",dest_path,(long long)sbuf.st_size);
 //   }
    ftp_reply(sess,FTP_DATACONN,text);
    //sess->start_transfer_time_sec=get_now_time_of_sec();
    //sess->start_transfer_time_usec=get_now_time_of_usec();
    char buf[65536]={'\0'}; 
    int flag=0;
    while(1)
    {
		printf("-------\n");
        ret = readn(sess->datasock,buf,sizeof(buf));
        if(ret == 0)
        {
            break;
        }
      //  limit_rate(sess,ret,1);
        if(sess->abor_recived) //abor命令
        {
            flag = 1;
            break;
        }
        int nwrite = writen(fd,buf,ret);
        if(nwrite != ret)
        {
            flag =1;
            break;
        }

    }
    //关闭数据套接字
    close(sess->datasock); //告诉对方数据传输结束
    sess->datasock=-1;
    close(fd);
    //226 return 
    if(flag == 0)
    {
        ftp_reply(sess,FTP_TRANSFEROK,"Teansfer complete");
    }
    else// 426 return
    {
        ftp_reply(sess,FTP_BADSENDFAILED,"Write failed");
    }  
    //判读abor是不是刚好在数据传输完毕的时候到达
    if(sess->abor_recived)
    {
        sess->abor_recived =0;
        ftp_reply(sess,FTP_TRANSFEROK,"Teansfer complete");
    }
    //上传完成之后重新开启控制连接通道
    start_alarm();
}
static void do_stor(session_t *sess)
{
    //上传 stor   断点上传  rest  stor
    upload_comm(sess,0);

}
static void do_appe(session_t *sess)
{
    upload_comm(sess,1);
}
static void do_list(session_t *sess)
{
    //创建数据连接
    if(get_transport_fd(sess)==0)
    {
        return ;
    }
    ftp_reply(sess,FTP_DATACONN,"Here comes the directory list");
    //传输列表
    list_dir(sess);
    //关闭数据套接字
    close(sess->datasock); //告诉对方数据传输结束
    sess->datasock=-1;
    //226 return 
    ftp_reply(sess,FTP_TRANSFEROK,"Directory send OK");

}
static void do_nlst(session_t *sess){} //不实现
static void do_rest(session_t *sess) //和断点续传有关系
{
    sess->restart_pos = str_to_longlong(sess->arg);
    char text[1024]={'\0'};
    sprintf(text,"Restart position acccepted (%lld)",sess->restart_pos);
    ftp_reply(sess,FTP_RESTOK,"rest OK");
}
static void do_abor(session_t *sess)
    //数据连接通道创建的时候断开数据连接但是不断开控制连接
{
    //处于数据传输过程中 通过紧急模式传输命令
    //发送命令时候刚好数据传输完毕  226

    //发送命令时候数据没有传输完毕,数据正在传输  426 226

    //没有处于数据传输中会通过正常的模式来传输指令  225
    ftp_reply(sess,FTP_ABOR_NOCONN,"No transfet to abor");

}
static void do_pwd(session_t *sess)
{
    char path[1024]={'\0'};
    char dir[4096]={'\0'};
    getcwd(dir,4096);
    sprintf(path,"\"%s\"",dir);
    ftp_reply(sess,FTP_PWDOK,path);
}
static void do_mkd(session_t *sess)
{
    if(mkdir(sess->arg,0777) < 0)
    {
        ftp_reply(sess,FTP_FILEFAILD,"Created Failed" );
        return ;
    }
    char text[1024]={'\0'};
    if(sess->arg[0] == '/')
    {
        sprintf(text,"%s Created successful",sess->arg);
    }
    else
    {
        char dir[1024]={'\0'};
        getcwd(dir,1024);
        if(dir[strlen(dir)-1] != '/')
        {
            dir[strlen(dir)]='/';
        }
        sprintf(text,"%s%s Created successful",dir,sess->arg);
    }

    ftp_reply(sess,FTP_PWDOK,text);

}
static void do_rmd(session_t *sess) //删除文件夹
{
    if(rmdir(sess->arg) <0)
    {
        ftp_reply(sess,FTP_FILEFAILD,"Delete operation Failed");
        return ;
    }
    ftp_reply(sess,FTP_FILEFAILD,"Delete operation Successful");
}
static void do_dele(session_t *sess) //删除文件
{
    char dest_path[1024]={'\0'};
    char *cur=sess->arg;
    int i=0;
    while(*cur != '\0')
    {
        if(*cur != ' ')
        {
            dest_path[i++]=*cur;
        }
        cur++;
    }
    if(unlink(dest_path) < 0)
    {
        ftp_reply(sess,FTP_FILEFAILD,"Delete operation Failed");
        return ;
    }
    ftp_reply(sess,FTP_DELETEOK,"Delete operation Successful");
}
static void do_rnfr(session_t *sess)
{
    sess->rnfr_name= (char*)malloc(strlen(sess->arg)+1);
    memset(sess->rnfr_name,'\0',strlen(sess->arg)+1);
    char dest_path[1024]={'\0'};
    char *cur=sess->arg;
    int i=0;
    while(*cur != '\0')
    {
        if(*cur != ' ')
        {
            dest_path[i++]=*cur;
        }
        cur++;
    }
    strcpy(sess->rnfr_name,dest_path);
    ftp_reply(sess,FTP_RNFROK,"Reday for RNTO");  
}

static void do_rnto(session_t *sess)
{
    if(sess->rnfr_name == NULL)
    {
        ftp_reply(sess,FTP_NEEDRNFR,"Need Rnfr");
        return ;
    }
    rename(sess->rnfr_name,sess->arg);
    ftp_reply(sess,FTP_RENAMEOK,"Rename successful");
    free(sess->rnfr_name);
    sess->rnfr_name=NULL;
}
static void do_site_chmod(session_t *sess,char *arg)
{
    if(strlen(arg)==0)
    {
        ftp_reply(sess,FTP_BADCMD,"SITE CHOMD needs 2 argument.");  
    }
    else
    {
        char perm[100]={'\0'};
        char file[100]={'\0'};
        str_split(arg,perm,file,' ');
        if(strlen(file) == 0)
        {
            ftp_reply(sess,FTP_BADCMD,"SITE CHOMD needs 2 argument."); 
        }
        else
        {
            unsigned int mode = str_octal_to_uint(perm);
            if(chmod(file,mode) < 0)
            {
                ftp_reply(sess,FTP_TYPEOK,"chmod falied"); 
            }
            else
            {
                ftp_reply(sess,FTP_TYPEOK,"chmod ok"); 
            }

        }
    }
}
static void do_site_umask(session_t *sess,char *arg)
{
    char text[1024]={'\0'};
    if(strlen(arg)==0)
    {
        ftp_reply(sess,FTP_TYPEOK,"you current umask is 644");  
    }
    else
    {
        unsigned int um= str_octal_to_uint(arg);
        umask(um);
        sprintf(text,"umask set to %s",arg);
        ftp_reply(sess,FTP_TYPEOK,text);  
    }

}

static void do_site(session_t *sess)
{
    char cmd[100]={'\0'};
    char arg[100]={'\0'};

    str_split(sess->arg,cmd,arg,' ');
    if(strcasecmp(cmd,"CHMOD") == 0)
    {
        do_site_chmod(sess,arg);
    }
    else if(strcasecmp(cmd,"HELP") == 0)
    {
        ftp_reply(sess,FTP_HELP,"CHMOD　HELP UMASK");
    }
    else if(strcasecmp(cmd,"UMASK") == 0)
    {
        do_site_umask(sess,arg);
    }
    else
    {
        ftp_reply(sess,FTP_BADCMD,"Bad cmd");
    }

}
static void do_syst(session_t *sess)
{
    ftp_reply(sess,FTP_SYSTOK,"UNIX Type: L8");
}
static void do_feat(session_t *sess)
{
    ftp_reply_(sess,FTP_FEAT,"Features:");
    ftp_reply_str(sess,"EPRT");
    ftp_reply_str(sess,"EPSV");
    ftp_reply_str(sess,"MDTM");
    ftp_reply_str(sess,"PASV");
    ftp_reply_str(sess,"REST STREAM");
    ftp_reply_str(sess,"SIZE");
    ftp_reply_str(sess,"TVFS");
    ftp_reply_str(sess,"UTF8");
    ftp_reply(sess,FTP_FEAT,"END");
}
static void do_size(session_t *sess)
{
    struct stat buf;
    if(stat(sess->arg,&buf) < 0)
    {
        ftp_reply(sess,FTP_FILEFAILD,"SIZE operation failed");
        return;
    }
    if( !S_ISREG(buf.st_mode))
    {
        ftp_reply(sess,FTP_FILEFAILD,"Can not get file size");
        return ;
    }
    char text[1024]={'\0'};
    sprintf(text,"%lld",(long long)buf.st_size);
    ftp_reply(sess,FTP_SIZEOK,text);

}
static void do_stat(session_t *sess)
{
    ftp_reply_(sess,FTP_STATOK,"server status:");

    ftp_reply(sess,FTP_STATOK,"End of status");

}
static void do_noop(session_t *sess) //防止空闲断开
{
    ftp_reply(sess,FTP_NOOPOK,"NOOP OK");
}
static void do_help(session_t *sess)
{
    ftp_reply_(sess,FTP_HELP,"The following commands are recognized.");
    ftp_reply_str(sess,"ABOR ACCT ALLO APPE CDUP CWD  DELE EPRT EPSV FEAT HELP LIST MDTM MKD");
    ftp_reply_str(sess,"MODE NLST NOOP OPTS PASS PASV PORT PWD  QUIT REIN REST RETR RMD  RNFR");
    ftp_reply_str(sess,"RNTO SITE SIZE SMNT STAT STOR STOU STRU SYST TYPE USER XCUP XCWD XMKD");
    ftp_reply_str(sess,"XPWD XRMD");
    ftp_reply(sess,FTP_HELP,"Help OK.");
}


void handle_child(session_t *sess)
{
    ftp_reply(sess,FTP_GREET,"myftp 1.0  welcome to myftp");
    while(1)
    {
        memset(sess->cmdline,0,sizeof(sess->cmdline));
        memset(sess->cmd,0,sizeof(sess->cmd));
        memset(sess->arg,0,sizeof(sess->arg));
        start_alarm();
        int ret =readline(sess->ctrl_fd,sess->cmdline,64); //接受客户端命令请求
        if(ret == -1)
        {
            //TODO log
            kftpd_print_log(LOG_ERROR,"accept failed",__FILE__,__FUNCTION__,__LINE__);
           exit(EXIT_FAILURE);
        }	
        else if(ret == 0) //客户端关闭
        {
            printf("ip=%s client is closeing",sess->online_client_ip);
            exit(EXIT_FAILURE);
        }
        //去除\r\n
        str_trim_ctrl(sess->cmdline);
        printf("cmdline: [%s]\n",sess->cmdline);
        //解析和处理FTP命令 与参数
        str_split(sess->cmdline,sess->cmd,sess->arg,' ');
        printf("cmd: [%s]  arg: [%s]\n",sess->cmd,sess->arg);
        //将命令转换为大写
        str_upper(sess->cmd);
        //处理FTP命令
        int i=0;
        int size=sizeof(cmd_function)/sizeof(cmd_function[0]);
        for(;i<size;++i)
        {
            if(strcasecmp(cmd_function[i].cmd,sess->cmd) == 0)
            {
                if(cmd_function[i].cmd_func != NULL)
                {
                    cmd_function[i].cmd_func(sess);
                }
                else
                {
                    ftp_reply(sess,FTP_COMMANDNOTOMPL,"Unimplement cmd"); 
                }
                break;
                printf("handle\n");
            }
        }
        if(i == size)
        {
            ftp_reply(sess,FTP_BADCMD,"Unknown cmd"); 
        }

    }
}


