/*************************************************************************
	> File Name: parse_config.h
	> Author: wk
	> Mail: 18402927708@163.com
	> Created Time: Tue 06 Jun 2017 11:09:33 PM CST
 ************************************************************************/

#ifndef _PARSE_CONFIG_H
#define _PARSE_CONFIG_H



void load_kftpd_conf(const char*path);
void load_add_user_conf(const char*path);
void load_del_user_conf(const char*path);
void parseconf_load_setting( char* setting,int cur);

#endif
