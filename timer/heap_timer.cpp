#include "heap_timer.h"

// 构造函数之一 初始化一个大小为cap的空堆
time_heap::time_heap(int cap) throw (std::exception) : capacity(cap), cur_size(0){
	array = new heap_timer* [capacity]; // 创建堆数组
	if(!array){
		throw std::exception();
	}
	for (int i = 0; i < capacity; ++i){
		array[i] = NULL;
	}
}

// 构造函数之二 用已有的数组来初始化堆
time_heap::time_heap(heap_timer** init_array, int size, int capacity) throw (std::exception): cur_size(size), capacity(capacity){
	if (capacity < size){
		throw std::exception();
	}
	array = new heap_timer*[capacity]; // 创建堆数组
	if(!array){
		throw std::exception();
	}
	for (int i = 0; i < capacity; ++i){
		array[i] = NULL;
	}
	if (size != 0){
		// 初始化堆数组
		for (int i = 0; i < size; ++i){
			array[i] = init_array[i];
		}
		for (int i = (cur_size-1)/2; i >= 0; --i){
			// 构造堆
			percolate_down(i);
		}
	}
} 

// 销毁时间堆
time_heap::~time_heap(){
	for(int i = 0; i < cur_size; ++i){
		delete array[i];
	}
	delete[] array;
}

// 添加目标定时器timer
void time_heap::add_timer(heap_timer* timer) throw (std::exception) {
	if (!timer){
		return ;
	}
	if (cur_size >= capacity){
		resize(); // 容量不够大时 进行扩容 直接扩大一倍
	}

	// 插入一个元素 当前堆大小+1 hole是新位置
	int hole = cur_size++;
	int parent = 0;
	// 对新加入的节点路径上的所有节点 上虑
	for (; hole > 0; hole = parent){
		parent = (hole-1)/2;
		if (array[parent]->expire <= timer->expire){
			break;
		}
		array[hole] = array[parent];
	}
	array[hole] = timer;
}

// 删除目标定时器timer
void time_heap::del_timer(heap_timer* timer){
	if (!timer){
		return ;
	}
	// 下一句：仅仅将目标定时器的回调函数设置为空 即所谓的延迟销毁 这将节省真正删除该定时器的开销 但是容易使得堆数组膨胀
	timer->cb_func = NULL;
}

// 调整定时器时间
void time_heap::adjust_timer(heap_timer* timer){
	if (!timer){
		return ;
	}
	// 将此定时器进行下虑
	// 需要计算此定时器在堆数组中的位置
	int i = 0;
	for (; i < cur_size; ++i){
		if (array[i] == timer){
			break;
		}
	}
	percolate_down(i);
}


// 获取堆顶部的定时器
heap_timer* time_heap::top() const{
	if (empty()){
		return NULL;
	}
	return array[0];
}

// 删除堆顶定时器
void time_heap::pop_heap(){
	if (empty()){
		return ;
	}
	if (array[0]){
		delete array[0];
		// 将堆顶元素替换为数组中的最后一个 再进行重新排列
		array[0] = array[--cur_size];
		percolate_down(0); // 堆顶元素 下虑
	}
}

// 心博函数
void time_heap::tick(){
	heap_timer* tmp = array[0];
	time_t cur = time(NULL);

	LOG_INFO("%s", "timer tick");
    Log::get_instance()->flush();

	while(!empty()){
		if (!tmp){
			break;
		}

		// 如果堆顶元素没到期 直接退出
		if (tmp->expire > cur){
			break;
		}

		// 如果到期了 就要执行回调函数
		if (array[0]->cb_func){
			array[0]->cb_func(array[0]->user_data);
		}
		// 删除堆顶元素 生成新的堆顶定时器
		pop_heap();
		tmp = array[0];
	}
}

// 最小堆下虑操作
void time_heap::percolate_down(int hole){
	heap_timer* temp = array[hole];
	int child = 0;
	for(; ((hole*2+1) <= (cur_size-1)); hole = child){
		child = hole*2 + 1;
		// 看左右子节点 哪个小
		if ((child < (cur_size-1)) && (array[child+1]->expire < array[child]->expire)){
			++child;
		}
		if (array[child]->expire < temp->expire){
			array[hole] = array[child];
		}
		else{
			break;
		}
	}
	array[hole] = temp;
}

// 扩容堆 扩大一倍
void time_heap::resize() throw (std::exception){
	heap_timer** temp = new heap_timer*[2*cur_size];
	for (int i = 0; i < 2*capacity; ++i){
		temp[i] = NULL;
	}
	if (!temp){
		throw std::exception();
	}

	capacity = 2*capacity;
	for (int i = 0; i < cur_size; ++i){
		temp[i] = array[i];
	}
	delete[] array;
	array = temp;
}
