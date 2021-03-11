// config
#ifndef CONFIG_H
#define CONFIG_H
#include <string.h>
#include <stdlib.h>
#include <unistd.h>

class Config
{
public:
    Config();
    ~Config(){};

    void parse_arg(int argc, char*argv[]);

    //端口号
    int PORT;
    
    //日志写入方式
    int LOGWrite;

    //listenfd触发模式
    int LISTENTrigmode;

    //connfd触发模式
    int CONNTrigmode;

    //数据库连接池数量
    int sql_num;

    //线程池内的线程数量
    int thread_num;

    // 定时器的类型
    // int timer;
};

#endif