/*************************************************************************
	> File Name: kftpd.h
	> Author: wk
	> Mail: 18402927708@163.com
	> Created Time: Sat 10 Jun 2017 04:32:12 PM CST
 ************************************************************************/

#ifndef _KFTPD_H
#define _KFTPD_H
#include"my_hash.h"
#include"session.h"





static session_t *p_sess;
static unsigned int num_children;   //连接数
static hash_t *ip_num_clients_hash;//哈希表
static hash_t *pid_ip_hash;

unsigned int drop_ip_count(void *ip);
void  handle_sigchld(int sig);
unsigned int hash_ip_function(unsigned int buckets,void *key); //桶的大小 关键码
unsigned int handle_ip_count(void *ip);//返回该IP的连接数
#endif
