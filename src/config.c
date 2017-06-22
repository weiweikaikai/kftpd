/*************************************************************************
	> File Name: config.c
	> Author: wk
	> Mail: 18402927708@163.com
	> Created Time: Sat 10 Jun 2017 08:38:24 PM CST
 ************************************************************************/


//如果配置文件中没有配置这些值就会使用默认值
 unsigned int kftpd_pasv_enable=1;
 unsigned int kftpd_port_enable=1;
 unsigned int kftpd_listen_port=9001;
 unsigned int kftpd_max_clients=200;
 unsigned int kftpd_max_per_ip=50;
 unsigned int kftpd_accept_timeout=60;
 unsigned int kftpd_connect_timeout=60;
 unsigned int kftpd_upload_max_rate=102400; //字节为单位 100k
 unsigned int kftpd_download_max_rate=204800;//200k
 unsigned int kftpd_add_user_enable=0;
 unsigned int kftpd_del_user_enable=0;
 unsigned int kftpd_session_timeout=300;

