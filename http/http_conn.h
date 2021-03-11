#ifndef HTTPCONNECTION_H
#define HTTPCONNECTION_H

#include <unistd.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/epoll.h>
#include <fcntl.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <assert.h>
#include <sys/stat.h>
#include <string.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/mman.h>
#include <stdarg.h>
#include <errno.h>
#include <sys/wait.h>
#include <sys/uio.h>
#include "../lock/locker.h"
#include "../mysql/sql_connection_pool.h"
#include "../mysql/sql_conn_RAII.h"


class http_conn
{
public:
    static const int FILENAME_LEN = 200;
    static const int READ_BUFFER_SIZE = 2048;
    static const int WRITE_BUFFER_SIZE = 1024;
    enum METHOD
    {
        GET = 0,
        POST,
        HEAD,
        PUT,
        DELETE,
        TRACE,
        OPTIONS,
        CONNECT,
        PATH
    };
    enum CHECK_STATE
    {
        CHECK_STATE_REQUESTLINE = 0,
        CHECK_STATE_HEADER,
        CHECK_STATE_CONTENT
    };
    enum HTTP_CODE
    {
        NO_REQUEST,
        GET_REQUEST,
        BAD_REQUEST,
        NO_RESOURCE,
        FORBIDDEN_REQUEST,
        FILE_REQUEST,
        INTERNAL_ERROR,
        CLOSED_CONNECTION
    };
    enum LINE_STATUS
    {
        LINE_OK = 0,
        LINE_BAD,
        LINE_OPEN
    };

public:
    http_conn() {}
    ~http_conn() {}

public:
    void init(int sockfd, const sockaddr_in &addr, int);
    void close_conn(bool real_close = true);
    void process();
    bool read_once();
    bool write();
    sockaddr_in *get_address()
    {
        return &m_address;
    }
    void initmysql_result(connection_pool *connPool); // 存下数据库中的账户和密码

private:
    // 初始化新的连接
    void init();
    // 解析Http请求
    HTTP_CODE process_read();
    // 填充应答
    bool process_write(HTTP_CODE ret);
    // 不同的函数 分别解析Http请求报文的请求行 头部行 内容
    HTTP_CODE parse_request_line(char *text);
    HTTP_CODE parse_headers(char *text);
    HTTP_CODE parse_content(char *text);
    HTTP_CODE do_request();
    char *get_line() { return m_read_buf + m_start_line; };
    LINE_STATUS parse_line();
    
    // 填充HTTP应答
    void unmap();
    bool add_response(const char *format, ...);
    bool add_content(const char *content);
    bool add_status_line(int status, const char *title);
    bool add_headers(int content_length);
    bool add_content_type();
    bool add_content_length(int content_length);
    bool add_linger();
    bool add_blank_line();

public:
    // 所有socket上的事件都被注册到同一个epoll内科表事件上 将epoll设置为静态的(多个对象共享)
    static int m_epollfd;
    static int m_user_count;
    MYSQL *mysql; // mysql连接

private:
    // HTTP请求的地址
    int m_sockfd;
    sockaddr_in m_address;
    
    // 读缓存区
    char m_read_buf[READ_BUFFER_SIZE];
    int m_read_idx;
    int m_checked_idx;
    int m_start_line;

    // 写缓冲区
    char m_write_buf[WRITE_BUFFER_SIZE];
    int m_write_idx;
    CHECK_STATE m_check_state;
    // 请求方法
    METHOD m_method;
    // 请求文件的完整路径 -> doc_root+m_url
    char m_real_file[FILENAME_LEN];
    // 请求地址
    char *m_url;
    // 版本号
    char *m_version;
    // 主机名
    char *m_host;
    // 消息体长度
    int m_content_length;
    // Http是否保持连接
    bool m_linger;
    // 客户请求的目标文件将被mmap到内存中的其实位置
    char *m_file_address;
    // 目标文件的状态 通过其可以判断文件是否存在 是否为目录 是否可读 并获取文件大小
    struct stat m_file_stat;
    
    /*
    用writev执行写操作 m_iv_count为被写内存块的数目
    iovec是一个结构体(封装了一块内存的起始位置和长度)，里面有两个元素，
    指针成员iov_base指向一个缓冲区，这个缓冲区是存放的是writev将要发送的数据
    成员iov_len表示实际写入的长度
    */
    struct iovec m_iv[2];
    int m_iv_count;
    
    int cgi;        //是否启用POST
    char *m_string; //存储请求头数据

    // 已发送数据和需发生数据
    int bytes_to_send;
    int bytes_have_send;

    // 触发模式
    int m_CONNMode;

};

#endif
