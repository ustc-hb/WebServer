// 实现一个时间堆

#ifndef MIN_HEAP
#define MIN_HEAP

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
#include <errno.h>
#include <iostream>
#include <time.h>

#include "../log/log.h"

using std::exception;

class heap_timer; // 时间堆的声明

// 绑定socket与定时器
struct client_data_heap
{
    sockaddr_in address;
    int sockfd;
    heap_timer *timer;
};


// 定时器类
class heap_timer{
public:
	heap_timer(){};
	heap_timer(int delay){
		expire = time(NULL) + delay;
	}
public:
	time_t expire;
	void (*cb_func) (client_data_heap*); // 定时器回调函数
	client_data_heap* user_data; // 用户数据
};


// 时间堆类
class time_heap{
public:
	// 构造函数之一 初始化一个大小为cap的空堆
	time_heap(int ) throw (std::exception);

	// 构造函数之二 用已有的数组来初始化堆
	time_heap(heap_timer**, int , int ) throw (std::exception);

	// 销毁时间堆
	~time_heap();

public:
	// 添加目标定时器timer
	void add_timer(heap_timer* ) throw (std::exception) ;

	// 删除目标定时器timer
	void del_timer(heap_timer* );

	// 调整定时器时间
	void adjust_timer(heap_timer*);


	// 获取堆顶部的定时器
	heap_timer* top() const;

	// 删除堆顶定时器
	void pop_heap();

	// 心博函数
	void tick();

	bool empty() const {return cur_size == 0;}

private:
	// 最小堆下虑操作
	void percolate_down(int );

	// 扩容堆 扩大一倍
	void resize() throw (std::exception);

private:
	heap_timer** array; // 堆数组 指针的指针
	int capacity; // 对数组的容量
	int cur_size; // 目前堆里的元素数目
};


#endif