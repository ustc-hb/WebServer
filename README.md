Linux Webserve
===============
Linux下C++轻量级Web服务器.

* 使用**线程池 + epoll(ET和LT均实现) + 模拟Proactor模式**的并发模型
* 使用**数据库连接池**实现数据库连接
* 使用**状态机**解析HTTP请求报文，支持解析**GET和POST**请求
* 通过访问服务器数据库实现web端用户**注册、登录**功能，可以请求服务器**图片和视频文件**
* 实现**同步/异步日志系统**，记录服务器运行状态
* 实现**升序链表/时间堆定时器**， 处理超时事件
* 经Webbench压力测试可以实现**5000+的并发连接**数据交换

基础测试
------------
* 服务器测试环境
	* Ubuntu版本16.04
	* MySQL版本5.7.33

* 运行前需安装MySQL数据库并修改main.cpp中的数据库初始化信息
    ```C++
    // 建立数据库
    create database dbname;
    USE dbname;
    // 创建表
    CREATE TABLE user(
        username char(50) NULL,
        passwd char(50) NULL
    )ENGINE=InnoDB;
    // 添加数据
    INSERT INTO user(username, passwd) VALUES('name', 'passwd');
    ```

* 编译

    ```C++
    make all
    ```

* 运行

    ```C++
    ./webserver
    ```
    或者
    ```C++
    sh ./run.sh
    ```
    
* 个性化运行
    ```C++
    ./webserver -p [port] -l[LOGWrite]  -m[LISTENTrigmode]  -c[CONNTrigmode]  -s[sql_num]  -n[thread_num]
    ```
  其中
    ```C++
    port: 端口号 
    LOGWrite: 日志模式 同步: 0(默认) 异步: 1 
    LISTENTrigmode: listenfd模式 LT模式: 0(默认) ET模式: 1
    CONNTrigmode: connfd模式 LT模式: 0(默认) ET模式: 1  c
    sql_num: sql连接池数目 默认8 
    thread_num: 线程池数目 默认8
    ```
* 浏览器端

    ```C++
    ip:port
    ```

webbench个性化测试
------

> * I/O复用方式 listenfd和connfd可以使用不同的触发模式 代码中使用LT + LT模式 可以自由修改与搭配.

- [x] LT + LT模式
    ```C++
    ./webserver -p 12345 -m 0 -c 0
    ```
    ![image](https://github.com/ustc-hb/WebServer/blob/main/resource/webbench-res/LT%2BLT.jpg)

- [ ] LT + ET模式
    ```C++
    ./webserver -p 12345 -m 0 -c 1
    ```
    ![image](https://github.com/ustc-hb/WebServer/blob/main/resource/webbench-res/LT%2BET.jpg)

- [ ] ET + LT模式
    ```C++
    ./webserver -p 12345 -m 1 -c 0
    ```
    ![image](https://github.com/ustc-hb/WebServer/blob/main/resource/webbench-res/ET%2BLT.jpg)    
    
- [ ] ET + ET模式
    ```C++
    ./webserver -p 12345 -m 1 -c 1
    ```
     ![image](https://github.com/ustc-hb/WebServer/blob/main/resource/webbench-res/ET%2BET.jpg)     
> * 日志写入方式，代码中使用同步日志，可以修改为异步写入.

- [x] 同步写入日志
* 默认模式	
	

- [ ] 异步写入日志
    ```C++
    ./webserver -p 12345 -l 1
    ```
     ![image](https://github.com/ustc-hb/WebServer/blob/main/resource/webbench-res/LT%2BLT_log1.jpg)  

> * 选择定时器类型，默认选择升序链表定时器，可以选择使用最小堆定时器。
- [x] 升序链表定时器
* 默认模式

- [ ] 最小堆定时器
* 需要在程序中更改定时器模式

    ```C++
    main.cpp
    26 // #define TIME_LIST // 排序定时器链表
    27 #define TIME_HEAP // 时间堆
    ```
     ![image](https://github.com/ustc-hb/WebServer/blob/main/resource/webbench-res/LT%2BLT_timer1.jpg)  
