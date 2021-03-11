// 自实现一个升序的定时器链表

#ifndef LST_TIMER
#define LST_TIMER

#include <unistd.h>
#include <sys/types.h>
#include <fcntl.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <assert.h>
#include <sys/stat.h>
#include <string.h>
#include <stdlib.h>
#include <sys/mman.h>

#include <time.h>
#include "../log/log.h"

class util_timer;

struct client_data{
    sockaddr_in address;
    int sockfd;
    util_timer *timer;
};

class util_timer{
public:
    util_timer() : prev(NULL), next(NULL) {}

public:
    time_t expire;
    void (*cb_func)(client_data *);
    client_data *user_data;
    util_timer *prev;
    util_timer *next;
};

// 排序链表定时器
class sort_timer_lst{
public:
    sort_timer_lst() :head(NULL), tail(NULL){}
    ~sort_timer_lst();

    // 增加定时器
    void add_timer(util_timer*);

    // 调整定时器定时时间
    void adjust_timer(util_timer*);

    // 删除定时器
    void del_timer(util_timer*);

    // 心博函数
    void tick();

private:
    // 增加定时器的一般函数 从排序链表中找到应该插入的位置 
    void add_timer(util_timer *, util_timer *);

private:
    util_timer *head;
    util_timer *tail;
};

#endif
