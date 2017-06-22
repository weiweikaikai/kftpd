/*************************************************************************
	> File Name: memory_pool.h
	> Author: wk
	> Mail: 18402927708@163.com
	> Created Time: Thu 01 Jun 2017 02:09:36 PM CST
 ************************************************************************/

#ifndef _MEMORY_POOL_H
#define _MEMORY_POOL_H
#include<stdio.h>
#include<stdlib.h>

void *Allocate(size_t n); //申请内存
void *Reallocate(void *p, size_t old_sz,size_t new_sz);//改变申请的大小
void Deallocate(void *p, size_t n);//释放内存
void* kftp_malloc(size_t sz);
void kftp_free(void*p,size_t sz);
#endif
