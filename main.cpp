#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <stdio.h>
#include <unistd.h>
#include <errno.h>
#include <fcntl.h>
#include <stdlib.h>
#include <cassert>
#include <sys/epoll.h>

#include "./lock/locker.h"
#include "./threadpool/threadpool.h"
#include "./timer/lst_timer.h"
#include "./timer/heap_timer.h"
#include "./http/http_conn.h"
#include "./log/log.h"
#include "./mysql/sql_connection_pool.h"
#include "./mysql/sql_conn_RAII.h"
#include "./config/config.h"

#define MAX_FD 65536           //最大文件描述符
#define MAX_EVENT_NUMBER 10000 //最大事件数
#define TIMESLOT 5             //最小超时单位

#define TIME_LIST // 排序定时器链表
// #define TIME_HEAP // 时间堆

//这三个函数在http_conn.cpp中定义，改变链接属性
extern int addfd(int epollfd, int fd, bool one_shot, int mode);
extern int removefd(int epollfd, int fd);
extern int setnonblocking(int fd);

//设置定时器相关参数
static int pipefd[2];
static int epollfd = 0;

//信号处理函数 发送到管道即可
void sig_handler(int sig){
    //为保证函数的可重入性，保留原来的errno
    int save_errno = errno;
    int msg = sig;
    send(pipefd[1], (char *)&msg, 1, 0);
    errno = save_errno;
}

//设置信号函数
void addsig(int sig, void(handler)(int), bool restart = true){
    struct sigaction sa;
    memset(&sa, '\0', sizeof(sa));
    sa.sa_handler = handler;
    if (restart)
        sa.sa_flags |= SA_RESTART;
    sigfillset(&sa.sa_mask);
    assert(sigaction(sig, &sa, NULL) != -1);
}

//定时处理任务，重新定时以不断触发SIGALRM信号
// 需要区别不同的定时器
template<class T>
void timer_handler(T* timer_process){
    timer_process->tick();
    alarm(TIMESLOT);
}


//定时器回调函数，删除非活动连接在socket上的注册事件，并关闭
template<class T>
void cb_func(T* user_data){
    epoll_ctl(epollfd, EPOLL_CTL_DEL, user_data->sockfd, 0);
    assert(user_data);
    close(user_data->sockfd);
    http_conn::m_user_count--;
    LOG_INFO("close fd %d", user_data->sockfd);
    Log::get_instance()->flush();
}


void show_error(int connfd, const char *info){
    printf("%s", info);
    send(connfd, info, strlen(info), 0);
    close(connfd);
}

int main(int argc, char *argv[]){

    Config config;
    config.parse_arg(argc, argv);

    // 选择相应的定时器
    
#ifdef TIME_LIST
    static sort_timer_lst* timer_process = new sort_timer_lst();
#endif
#ifdef TIME_HEAP
    static time_heap* timer_process = new time_heap(10);
#endif

	// 设置日志系统 若设置日志的数组长度>0 则是异步日志
    if (config.LOGWrite == 1)
        Log::get_instance()->init("ServerLog", 2000, 800000, 8); //异步日志模型
    else if (config.LOGWrite == 0)
        Log::get_instance()->init("ServerLog", 2000, 800000, 0); //同步日志模型

    int port = config.PORT;

    addsig(SIGPIPE, SIG_IGN);

    // 创建数据库连接池
    // 后面要把数据库连接池作为参数传入线程池
    connection_pool *connPool = connection_pool::GetInstance();
    connPool->init("localhost", "root", "111111", "hbdb", 3306, config.sql_num);

    //创建线程池
    threadpool<http_conn> *pool = NULL;
    try{
        pool = new threadpool<http_conn>(connPool, config.thread_num);
    }
    catch (...){
        return 1;
    }

    http_conn *users = new http_conn[MAX_FD];
    assert(users);

    //初始化数据库读取表
    // 将数据库上的数据存入一个map中 在http_conn中定义了
    users->initmysql_result(connPool);

    int listenfd = socket(PF_INET, SOCK_STREAM, 0);
    assert(listenfd >= 0);

    //struct linger tmp={1,0};
    //SO_LINGER若有数据待发送，延迟关闭
    //setsockopt(listenfd,SOL_SOCKET,SO_LINGER,&tmp,sizeof(tmp));

    int ret = 0;
    struct sockaddr_in address;
    bzero(&address, sizeof(address));
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = htonl(INADDR_ANY); // 本地地址
    address.sin_port = htons(port); // htons、htonl将主机字节顺序转换为网络字节顺序

    // 重用socket地址
    int flag = 1;
    setsockopt(listenfd, SOL_SOCKET, SO_REUSEADDR, &flag, sizeof(flag));
    ret = bind(listenfd, (struct sockaddr *)&address, sizeof(address));
    assert(ret >= 0);
    ret = listen(listenfd, 5);
    assert(ret >= 0);

    //创建内核事件表
    epoll_event events[MAX_EVENT_NUMBER];
    epollfd = epoll_create(5);
    assert(epollfd != -1);

    addfd(epollfd, listenfd, false, config.LISTENTrigmode);
    http_conn::m_epollfd = epollfd;

    //创建管道 信号管道
    ret = socketpair(PF_UNIX, SOCK_STREAM, 0, pipefd);
    assert(ret != -1);
    setnonblocking(pipefd[1]);
    addfd(epollfd, pipefd[0], false, 0);

    addsig(SIGALRM, sig_handler, false);
    addsig(SIGTERM, sig_handler, false);
    bool stop_server = false;

    // 用户数据数组

#ifdef TIME_LIST
    client_data *users_timer = new client_data[MAX_FD];
#endif
#ifdef TIME_HEAP
    client_data_heap *users_timer = new client_data_heap[MAX_FD];
#endif

    bool timeout = false;
    alarm(TIMESLOT); // 定时

    while (!stop_server){
        int number = epoll_wait(epollfd, events, MAX_EVENT_NUMBER, -1);
        if (number < 0 && errno != EINTR){
            LOG_ERROR("%s", "epoll failure");
            break;
        }

        for (int i = 0; i < number; i++){
            int sockfd = events[i].data.fd;

            //处理新到的客户连接
            if (sockfd == listenfd){
                struct sockaddr_in client_address;
                socklen_t client_addrlength = sizeof(client_address);

                // 这里是listen下的LT模式和ET模式
                // LT模式 会多次触发 一次连接一个
                if (config.LISTENTrigmode == 0){
                    int connfd = accept(listenfd, (struct sockaddr *)&client_address, &client_addrlength);
                    if (connfd < 0){
                        LOG_ERROR("%s:errno is:%d", "accept error", errno);
                        continue;
                    }
                    if (http_conn::m_user_count >= MAX_FD){
                        show_error(connfd, "Internal server busy");
                        LOG_ERROR("%s", "Internal server busy");
                        continue;
                    }
                    users[connfd].init(connfd, client_address, config.CONNTrigmode);

                    //初始化client_data数据
                    //创建定时器，设置回调函数和超时时间，绑定用户数据，将定时器添加到链表中
                    users_timer[connfd].address = client_address;
                    users_timer[connfd].sockfd = connfd;
                    
                    
    				#ifdef TIME_LIST
                        util_timer *timer = new util_timer;
                    #endif
                    #ifdef TIME_HEAP
                        heap_timer *timer = new heap_timer;
                    #endif

                    timer->user_data = &users_timer[connfd];

                    timer->cb_func = cb_func;

                    time_t cur = time(NULL);
                    timer->expire = cur + 3 * TIMESLOT;
                    users_timer[connfd].timer = timer;
                    timer_process->add_timer(timer);
                }
                else if (config.LISTENTrigmode == 1){
                // ET模式 只会触发一次 要用循环来处理接收连接的代码 确保程序进行了连接
                    while (1){
                        int connfd = accept(listenfd, (struct sockaddr *)&client_address, &client_addrlength);
                        if (connfd < 0)	{
                            LOG_ERROR("%s:errno is:%d", "accept error", errno);
                            break;
                        }
                        if (http_conn::m_user_count >= MAX_FD){
                            show_error(connfd, "Internal server busy");
                            LOG_ERROR("%s", "Internal server busy");
                            break;
                        }
                        users[connfd].init(connfd, client_address, config.CONNTrigmode);

                        //初始化client_data数据
                        //创建定时器，设置回调函数和超时时间，绑定用户数据，将定时器添加到链表中
                        users_timer[connfd].address = client_address;
                        users_timer[connfd].sockfd = connfd;
                        
                        #ifdef TIME_LIST
                            util_timer *timer = new util_timer;
                        #endif
                        #ifdef TIME_HEAP
                            heap_timer *timer = new heap_timer;
                        #endif
                        
                        timer->user_data = &users_timer[connfd];


                        timer->cb_func = cb_func;

                        time_t cur = time(NULL);
                        timer->expire = cur + 3 * TIMESLOT;
                        users_timer[connfd].timer = timer;
                        timer_process->add_timer(timer);
                    }
                    continue;
                }

            }

            // 出错了 异常情况发生
            else if (events[i].events & (EPOLLRDHUP | EPOLLHUP | EPOLLERR)){
                //服务器端关闭连接，移除对应的定时器

				auto *timer = users_timer[sockfd].timer;
                timer->cb_func(&users_timer[sockfd]);

                if (timer){
                    timer_process->del_timer(timer);
                }
            }

            // 信号到来 处理信号
            else if ((sockfd == pipefd[0]) && (events[i].events & EPOLLIN)){
                int sig;
                char signals[1024];
                ret = recv(pipefd[0], signals, sizeof(signals), 0);
                if (ret == -1){
                    continue;
                }
                else if (ret == 0){
                    continue;
                }
                else{
                    for (int i = 0; i < ret; ++i){
                        switch (signals[i]){
                        	case SIGALRM:{
                            	timeout = true;
                            	break;
                        	}
                        	case SIGTERM:{
                            	stop_server = true;
                        	}
                    	}
                    }
                }
            }

            //处理客户连接上接收到的数据
            else if (events[i].events & EPOLLIN){

				auto timer = users_timer[sockfd].timer;

                // 根据读的结果 决定是否将任务添加到任务队列中
                // 如果读取成功 将此任务加入到任务队列(数据结构为list)中
                // 因为调用append函数 threadpool中的信号量post 线程池中wait的线程便开始处理此客户
                // 程序由此展开
                if (users[sockfd].read_once()){
                    LOG_INFO("deal with the client(%s)", inet_ntoa(users[sockfd].get_address()->sin_addr));
                    Log::get_instance()->flush();
                    //若监测到读事件，将该事件放入请求队列
                    pool->append(users + sockfd);

                    //若有数据传输，则将定时器往后延迟3个单位
                    //并对新的定时器在链表上的位置进行调整
                    if (timer){
                        time_t cur = time(NULL);
                        timer->expire = cur + 3 * TIMESLOT;
                        LOG_INFO("%s", "adjust timer once");
                        Log::get_instance()->flush();
                        timer_process->adjust_timer(timer);
                    }
                }

                else{
                    timer->cb_func(&users_timer[sockfd]);
                    if (timer){
                        timer_process->del_timer(timer);
                    }
                }
            }

            // 子线程调用process_write完成响应报文，随后注册epollout事件
            // 服务器主线程检测写事件，并调用http_conn::write函数将响应报文发送给浏览器端
            // 写数据 发送http处理报文
            else if (events[i].events & EPOLLOUT){

				auto timer = users_timer[sockfd].timer;
                // 根据写的结果(write()失败返回false) 决定是否关闭连接
                if (users[sockfd].write()){
                    LOG_INFO("send data to the client(%s)", inet_ntoa(users[sockfd].get_address()->sin_addr));
                    Log::get_instance()->flush();

                    //若有数据传输，则将定时器往后延迟3个单位
                    //并对新的定时器在链表上的位置进行调整
                    if (timer){
                        time_t cur = time(NULL);
                        timer->expire = cur + 3 * TIMESLOT;
                        LOG_INFO("%s", "adjust timer once");
                        Log::get_instance()->flush();
                        timer_process->adjust_timer(timer);
                    }
                }
                else{
                    timer->cb_func(&users_timer[sockfd]);
                    if (timer){
                        timer_process->del_timer(timer);
                    }
                }
            }
        }

        // 最后处理定时超出事件
        if (timeout){
            timer_handler<> (timer_process);
            timeout = false;
        }
    }

    close(epollfd);
    close(listenfd);
    close(pipefd[1]);
    close(pipefd[0]);
    delete[] users;
    delete[] users_timer;
    delete pool;
    return 0;
}
