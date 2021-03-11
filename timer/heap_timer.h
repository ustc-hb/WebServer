// ʵ��һ��ʱ���

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

class heap_timer; // ʱ��ѵ�����

// ��socket�붨ʱ��
struct client_data_heap
{
    sockaddr_in address;
    int sockfd;
    heap_timer *timer;
};


// ��ʱ����
class heap_timer{
public:
	heap_timer(){};
	heap_timer(int delay){
		expire = time(NULL) + delay;
	}
public:
	time_t expire;
	void (*cb_func) (client_data_heap*); // ��ʱ���ص�����
	client_data_heap* user_data; // �û�����
};


// ʱ�����
class time_heap{
public:
	// ���캯��֮һ ��ʼ��һ����СΪcap�Ŀն�
	time_heap(int ) throw (std::exception);

	// ���캯��֮�� �����е���������ʼ����
	time_heap(heap_timer**, int , int ) throw (std::exception);

	// ����ʱ���
	~time_heap();

public:
	// ���Ŀ�궨ʱ��timer
	void add_timer(heap_timer* ) throw (std::exception) ;

	// ɾ��Ŀ�궨ʱ��timer
	void del_timer(heap_timer* );

	// ������ʱ��ʱ��
	void adjust_timer(heap_timer*);


	// ��ȡ�Ѷ����Ķ�ʱ��
	heap_timer* top() const;

	// ɾ���Ѷ���ʱ��
	void pop_heap();

	// �Ĳ�����
	void tick();

	bool empty() const {return cur_size == 0;}

private:
	// ��С�����ǲ���
	void percolate_down(int );

	// ���ݶ� ����һ��
	void resize() throw (std::exception);

private:
	heap_timer** array; // ������ ָ���ָ��
	int capacity; // �����������
	int cur_size; // Ŀǰ�����Ԫ����Ŀ
};


#endif