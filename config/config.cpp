#include "config.h"

Config::Config(){
    //端口号,默认11111
    PORT = 11111;

    //日志写入方式，默认同步
    LOGWrite = 0;

    //listenfd触发模式，默认LT
    LISTENTrigmode = 0;

    //connfd触发模式，默认LT
    CONNTrigmode = 0;

    //数据库连接池数量,默认8
    sql_num = 8;

    //线程池内的线程数量,默认8
    thread_num = 8;

    // 定时器默认用升序链表
    // timer = 0;
}

void Config::parse_arg(int argc, char*argv[]){
    int opt;
    // const char *str = "p:l:m:c:s:n:t:";
    const char *str = "p:l:m:c:s:n:";
    while ((opt = getopt(argc, argv, str)) != -1)
    {
        switch (opt)
        {
        case 'p':
        {
            PORT = atoi(optarg);
            break;
        }
        case 'l':
        {
            LOGWrite = atoi(optarg);
            break;
        }
        case 'm':
        {
            LISTENTrigmode = atoi(optarg);
            break;
        }
        case 'c':
        {
            CONNTrigmode = atoi(optarg);
            break;
        }
        case 's':
        {
            sql_num = atoi(optarg);
            break;
        }
        case 'n':
        {
            thread_num = atoi(optarg);
            break;
        }
        // case 't':
        // {
        //     timer = atoi(optarg);
        //     break;
        // }
        default:
            break;
        }
    }
}