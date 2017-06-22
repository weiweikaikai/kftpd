/*************************************************************************
	> File Name: memory_pool.c
	> Author: wk
	> Mail: 18402927708@163.com
	> Created Time: Thu 01 Jun 2017 02:09:21 PM CST
 ************************************************************************/

#include"memory_pool.h"
#include<string.h>
#include"kftpd_log.h"

#define _ALIGN 16 //排列基准值
#define _MAX_BYTES  1024 //最大值
#define _NFREELISTS _MAX_BYTES/_ALIGN //排列链大小


typedef union Obj
{
    union Obj* freelistlink;//指向下一个内存块的指针
    char clientData[1];    //数据
}Obj;

static Obj* volatile freelist[_NFREELISTS]; //自由链表
static char* startfree;                     //内存池水位线开始
static char* endfree;                       //内存池水位线结束
static size_t heapSize;                     //内存池的总大小

//获取大块内存插入自由链表中
static void* Refill(size_t n);
//从内存池中分配大块内存
static char* ChunkAlloc(size_t size,int nobjs);

static void* Allocate_1(size_t n);
static void Deallocate_1(void*p,size_t n);
static void* Reallocate_1(void*p,size_t old_sz,size_t new_sz);



static size_t Round_up(size_t bytes)
{
    //对齐
    return ((bytes +_ALIGN-1 ) &~(_ALIGN-1));
}
static size_t FreeList_Index(size_t bytes)
{
    return ((bytes+_ALIGN-1)/_ALIGN -1);
}


static void* OomMalloc(size_t n)
{

    void *result;
    /*
       1.分配内存成功，则直接返回
       2.若分配失败，则检查是否设置处理的handler,希望这个handler中会有用户自定义的释放空间的函数
       有处理函数则调用以后再分配，不断重复这个过程，直到分配成功为止
       没有设置处理handler 则直接结束程序
       */

    for(;;)
    {
        result = malloc(n);
        if(result)
        {
            return result;
        }
    }
}
static void* OomRealloc(void*p,size_t n)
{
    void *result;

    for(;;)
    {
        result = realloc(p,n);
        if(result)
        {
            return result;
        }
    }

}

static void* Allocate_1(size_t n)
{
    void *result = malloc(n);
    if(0 == result)
    {
        result = OomMalloc(n);
    }
    return result;
}
static void Deallocate_1(void *p,size_t n)
{
    free(p);
}
static void* Reallocate_1(void *p,size_t old_sz,size_t new_sz)
{
    void *result = realloc(p,new_sz);
    if(0 == result)
    {
        result=OomRealloc(p,new_sz);
    }
    return result;
}

static void* Refill(size_t n)
{
    //分配 20个n  bytes 的内存
    //如果分配不够则能分配多少就分配多少
	kftpd_print_log(LOG_TRACE,"using Alloc Refill",__FILE__,__FUNCTION__,__LINE__);
    int nobjs = 20;
    char * chunk = ChunkAlloc(n,nobjs);
    //如果只分配到一块，则直接用这块内存
    if(nobjs == 1)
    {
        return chunk;
    } 

    Obj* result ;
    Obj* cur;
    size_t index = FreeList_Index(n);
    result = (Obj*)chunk;  //把用户使用的空间留给用户
    //把剩余的块链接到自由链表上面
    cur =(Obj*)(chunk+n); 
    freelist[index] = cur;
     int i=0;
    for( i=2;i<nobjs;++i)
    {
        cur->freelistlink = (Obj*)(chunk+n*i); //强行将下一个内存块的地址转化为Obj类型的方便管理
        cur = cur->freelistlink;  //尾插
    }
    cur->freelistlink=NULL;
    return result;
}
static char*ChunkAlloc(size_t size, int nobjs)
{
    char *result;
    size_t bytesNeed = size*nobjs; //nobjs个size大小的内存 20*8 = 16   20*128=2560
    size_t bytesLeft = endfree-startfree;
	kftpd_print_log(LOG_TRACE,"using Alloc ChunkAlloc",__FILE__,__FUNCTION__,__LINE__);
    /*
       1.内存池中的内存足够，bytesleft >= bytesNeed 则直接从内存中取
       2.内存池中的内存不足，但是够一个bytesleft >=size  则直接取
       3.内存池中的内存不足，则从系统堆分配大块内存到内存池中
       */

    //能够从内存池中分配出20块出来就改变startfree
    if(bytesLeft >= bytesNeed)
    {
		char heapsize[64]={'\0'};
		sprintf(heapsize,"heapsize: %u  bytes :%u big need\n",heapSize,bytesLeft);
		 kftpd_print_log(LOG_TRACE,(char*)heapsize,__FILE__,__FUNCTION__,__LINE__);
        
        //内存足够分配nobjs个对象
        result = startfree;
        startfree += bytesNeed; //可以使用的空闲内存区
    }
    else if(bytesLeft >= size)  //不足以分配20块就分配一些块
    {
        char heapsize[64]={'\0'};
		sprintf(heapsize,"heapsize: %u  bytes :%u big need\n",heapSize,bytesLeft);
		 kftpd_print_log(LOG_TRACE,(char*)heapsize,__FILE__,__FUNCTION__,__LINE__);
        //内存不足分配nobjs个对象
        result = startfree;
        nobjs = bytesLeft /size; //可能内存不足会导致nobjs比较小
        startfree += nobjs *size;
    }
    else
    {
        //再试一下若内存池中还有小块剩余内存
        //将他头插到合适的自由链表中
        if(bytesLeft > 0)
        {
            size_t index = FreeList_Index(bytesLeft);
            ((Obj*)startfree)->freelistlink= freelist[index];
            freelist[index]=(Obj*)startfree;
            startfree=NULL;
        }

        //从系统堆分配两倍+已分配的heapSize/8 的内存到内存池中
        size_t bytesToGet = 2*bytesNeed + Round_up(heapSize>>4); //2560*2 + 0>>4 =5120
        //内存池内存不足，系统堆重新分配
        startfree = (char*)malloc(bytesToGet);

        //无奈之举
        //如果在系统堆中内存分配失败，
        //则尝试到自由链表中更大的节点中分配

        if(startfree == NULL)
        {
            //系统堆中没有足够，无奈只能到自由链表中看看
           int i=0; 
            for(i=size;i<_MAX_BYTES;i+=_ALIGN)
            {
                Obj* head = freelist[FreeList_Index(size)];
                if(head)
                {
                    startfree = (char*)head;
                    freelist[FreeList_Index(size)]=head->freelistlink;
                    endfree = startfree+i;
                    return ChunkAlloc(size,nobjs);
                }
            }
            //自由链表中也没有分配到内存，则再到一级配置器中分配内存
            //一级配置器中可能有设置的处理内存，或许能分配到内存
            startfree=(char*)Allocate_1(bytesToGet);
        }

        //从系统堆中分配的总字节数（可用于下次分配时进行调节）
        heapSize += bytesToGet;
        endfree = startfree + bytesToGet;

        //递归调用获取内存
        return ChunkAlloc(size,nobjs);
    }
    return result;
}

void *Allocate(size_t n)
{
	kftpd_print_log(LOG_TRACE,"using Alloc Allocate",__FILE__,__FUNCTION__,__LINE__);
    //若n > _MAX_BYTES则直接在一级配置器中获取
    //否则在二级配置器中获取
    if(n >_MAX_BYTES) //大于_MAX_BYTES  这里也就是大于1024
    {
        return Allocate_1(n); //调用一级内存池也就是封装了malloc
    }
    size_t index = FreeList_Index(n);//将申请到的内存块大小映射到FreeArry数组中

    /*
       1.如果自由链表中没有内存则通过Refill进行填充
       2.如果自由链表中有责直接返回一个节点块内存
       3.多线程环境需要考虑加锁
       */
    Obj* head = freelist[index];//获取该FreeArry节点的地址
    if(head == NULL)
    {
        return Refill(Round_up(n)); //调整为8字节内存对齐再填充内存池
    }
    else 
    {
        //自由链表获取内存  //多线程环境需要考虑加锁
        freelist[index] = head->freelistlink;//相当于链表的头删
        return head;//将该内存块反馈给用户
    }
}
void *Reallocate(void *p, size_t old_sz,size_t new_sz)
{
   kftpd_print_log(LOG_TRACE,"using Alloc reallocate",__FILE__,__FUNCTION__,__LINE__);
    void *result;
    size_t copy_sz;

    if(old_sz > (size_t)_MAX_BYTES && new_sz > (size_t)_MAX_BYTES)
    {
        return (realloc(p,new_sz));
    }
    if(Round_up(old_sz) == Round_up(new_sz))
    {
        return p;
    }
    result = Allocate(new_sz);
    copy_sz  = new_sz > old_sz ? old_sz :new_sz;
    memcpy(result,p,copy_sz);
    Deallocate(p,old_sz);

    return result;
}
void Deallocate(void *p, size_t n)
{
     kftpd_print_log(LOG_TRACE,"using Alloc Deallocate",__FILE__,__FUNCTION__,__LINE__);
    //若 n> _MAX_BYTES 则直接归还给一级配置器
    //否则放回二级配置器的自由链表

    if(n>_MAX_BYTES) //大小大于1024就采用一级内存池进行回收
    {
        Deallocate_1(p,n);
    }
    else
    {
        size_t index = FreeList_Index(n);
        //头插回自由链表
        Obj* tmp = (Obj*)p;
        tmp->freelistlink = freelist[index];
        freelist[index]=tmp;
    }
}

void* kftp_malloc(size_t sz)
	{
	   return Allocate(sz);
	}
void kftp_free(void*p,size_t sz)
	{
	     Deallocate(p,sz);
	}


/*
内存池的使用用例

int main()
{

char *p=kftp_malloc(20); //申请char这种类型的20个
kftp_free(p,20);

}


*/



