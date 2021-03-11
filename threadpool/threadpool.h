#ifndef THREADPOOL_H
#define THREADPOOL_H

#include <list>
#include <cstdio>
#include <exception>
#include <pthread.h>
#include "../lock/locker.h"
#include "../mysql/sql_connection_pool.h"
#include "../mysql/sql_conn_RAII.h"

template <typename T>
class threadpool
{
public:
    /*thread_number是线程池中线程的数量，max_requests是请求队列中最多允许的、等待处理的请求的数量*/
    threadpool(connection_pool *connPool, int thread_number = 8, int max_request = 10000);
    ~threadpool();
    bool append(T *request);

private:
    /*工作线程运行的函数，它不断从工作队列中取出任务并执行之*/
    static void *worker(void *arg);
    void run();

private:
    int m_thread_number;        //线程池中的线程数
    int m_max_requests;         //请求队列中允许的最大请求数
    pthread_t *m_threads;       //描述线程池的数组，其大小为m_thread_number
    std::list<T *> m_workqueue; //请求队列
    locker m_queuelocker;       //保护请求队列的互斥锁
    sem m_queuestat;            //是否有任务需要处理
    bool m_stop;                //是否结束线程
    connection_pool *m_connPool;  //数据库
};

// 构造函数
template <typename T>
threadpool<T>::threadpool( connection_pool *connPool, int thread_number, int max_requests) : m_thread_number(thread_number), m_max_requests(max_requests), m_stop(false), m_threads(NULL),m_connPool(connPool)
{
    if (thread_number <= 0 || max_requests <= 0)
        throw std::exception();
    m_threads = new pthread_t[m_thread_number];
    if (!m_threads)
        throw std::exception();

    // 创建thread_number个守护线程(即后台线程)
    for (int i = 0; i < thread_number; ++i)
    {
        //printf("create the %dth thread\n",i);
        if (pthread_create(m_threads + i, NULL, worker, this) != 0)
        {
            delete[] m_threads;
            throw std::exception();
        }
        if (pthread_detach(m_threads[i]))
        {
            delete[] m_threads;
            throw std::exception();
        }
    }
}

template <typename T>
threadpool<T>::~threadpool()
{
    delete[] m_threads;
    m_stop = true;
}

template <typename T>
bool threadpool<T>::append(T *request)
{
    m_queuelocker.lock();
    if (m_workqueue.size() > m_max_requests)
    {
        m_queuelocker.unlock();
        return false;
    }
    m_workqueue.push_back(request);
    m_queuelocker.unlock();
    m_queuestat.post(); // append即增加信号量 唤醒阻塞的线程 让任务得到处理
    return true;
}

template <typename T>
void *threadpool<T>::worker(void *arg)
{
    threadpool *pool = (threadpool *)arg;
    pool->run();
    return pool;
}

template <typename T>
void threadpool<T>::run()
{
    while (!m_stop)
    {
        // 所有线程均需要访问/操作工作队列 用信号量m_queuestat完成同步互斥
        m_queuestat.wait(); // 阻塞线程
        m_queuelocker.lock();
        if (m_workqueue.empty())
        {
            m_queuelocker.unlock();
            continue;
        }

        // 从工作队列中拿出一个任务出来进行处理 由于工作队列是list 调度是先来先服务
        T *request = m_workqueue.front();
        m_workqueue.pop_front();
        m_queuelocker.unlock();
        if (!request)
            continue;

        /*
        当线程需要run的时候 说明就是有连接到来 必须要连接数据库 
        因此需要从连接池中拿出一个数据库连接（数据库连接池在main函数里已经初始化了）
        看函数connectionRAII::connectionRAII(MYSQL **SQL, connection_pool *connPool)即可知道 
        */

        /*
        这里初始化一个connectionRAII对象 以资源管理对象 不用写成下面这样子
        1.从连接池中取出一个数据库连接
            request->mysql = m_connPool->GetConnection();
        2.process(模板类中的方法,这里是http类)进行处理
            request->process();
        3.将数据库连接放回连接池
            m_connPool->ReleaseConnection(request->mysql);
        // 直接RAII方式即可 需要新定义一个connectionRAII类
        */
        connectionRAII mysqlcon(&request->mysql, m_connPool);
        
        // 线程开始处理此任务
        request->process();
    }
}
#endif
