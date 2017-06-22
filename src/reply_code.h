/*************************************************************************
	> File Name: reply_code.h
	> Author: wk
	> Mail: 18402927708@163.com
	> Created Time: Tue 06 Jun 2017 11:14:58 PM CST
 ************************************************************************/

#ifndef _REPLY_CODE_H
#define _REPLY_CODE_H


#define FTP_DATACONN 150

#define FTP_TYPEOK 200
#define FTP_PORTOK  200
#define FTP_NOOPOK  200
#define FTP_FEAT  211
#define FTP_STATOK  211
#define FTP_SIZEOK  213
#define FTP_HELP    214
#define FTP_SYSTOK 215
#define FTP_GREET   220
#define FTP_GOODBY  221
#define FTP_TRANSFEROK 226
#define FTP_ABOR_NOCONN  225
#define FTP_PASVOK 227
#define FTP_LOGINOK 230
#define FTP_CWDOK   250
#define FTP_DELETEOK  250
#define FTP_RENAMEOK  250
#define FTP_PWDOK  257

#define FTP_GIVEPWORD 331
#define FTP_RESTOK    350
#define FTP_RNFROK 350

#define FTP_IDLE_TIMEOUT 421
#define FTP_TOO_MANY_USERS 421
#define FTP_TOO_MANY_IP    421
#define FTP_BADSENDCONN 425
#define FTP_BADSENDFAILED 426

#define FTP_BADCMD     500
#define FTP_COMMANDNOTOMPL 502
#define FTP_NEEDRNFR   503
#define FTP_LIGINERR  530
#define FTP_FILEFAILD        550
#define FTP_UPLODEFAILED    553


#endif
