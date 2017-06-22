/*************************************************************************
	> File Name: my_hash.h
	> Author: wk
	> Mail: 18402927708@163.com
	> Created Time: Tue 06 Jun 2017 11:08:06 PM CST
 ************************************************************************/

#ifndef _MY_HASH_H
#define _MY_HASH_H



//哈希表结构
typedef struct hash hash_t;
//节点
typedef struct hash_node hash_node_t;
//哈希函数指针
typedef unsigned int (*hashfunc_t)(unsigned int,void *); //桶的大小 关键码


//创建哈希表
hash_t * hash_alloc(unsigned int buckets,hashfunc_t hash_func);
//在哈希表中查找
void* hash_lookuo_enty(hash_t *hash,void *key,unsigned int key_size);
//往哈希表中添加一项
void hash_add_entry(hash_t *hash,void *key,unsigned int key_size,void *value,unsigned int value_size);
//从哈希表中删除一项
void hash_free_entry(hash_t *hash,void *key,unsigned int key_size);
//获取桶地址
hash_node_t ** hash_get_bucket(hash_t *hash,void *key);
//根据key获取哈希表中的一个节点
hash_node_t *hash_get_node_by_key(hash_t *hash,void *key,unsigned int key_size);


#endif
