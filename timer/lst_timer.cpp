#include "lst_timer.h"

sort_timer_lst::~sort_timer_lst(){
        util_timer *tmp = head;
        while (tmp){
            head = tmp->next;
            delete tmp;
            tmp = head;
        }
    }

// 增加定时器
void sort_timer_lst::add_timer(util_timer *timer){
    if (!timer){
        return;
    }
    if (!head){
        head = tail = timer;
        return;
    }
    if (timer->expire < head->expire){
        timer->next = head;
        head->prev = timer;
        head = timer;
        return;
    }
    add_timer(timer, head);
}

// 调整定时器定时时间
void sort_timer_lst::adjust_timer(util_timer *timer){
    if (!timer){
        return;
    }
    util_timer *tmp = timer->next;
    if (!tmp || (timer->expire < tmp->expire)){
        return;
    }
    if (timer == head){
        head = head->next;
        head->prev = NULL;
        timer->next = NULL;
        add_timer(timer, head);
    }
    else{
        timer->prev->next = timer->next;
        timer->next->prev = timer->prev;
        add_timer(timer, timer->next);
    }
}

// 删除定时器
void sort_timer_lst::del_timer(util_timer *timer){
    if (!timer){
        return;
    }
    if ((timer == head) && (timer == tail)){
        delete timer;
        head = NULL;
        tail = NULL;
        return;
    }
    if (timer == head){
        head = head->next;
        head->prev = NULL;
        delete timer;
        return;
    }
    if (timer == tail){
        tail = tail->prev;
        tail->next = NULL;
        delete timer;
        return;
    }
    timer->prev->next = timer->next;
    timer->next->prev = timer->prev;
    delete timer;
}

// 心博函数
void sort_timer_lst::tick(){
    if (!head){
        return;
    }
    //printf( "timer tick\n" );
    LOG_INFO("%s", "timer tick");
    Log::get_instance()->flush();
    time_t cur = time(NULL);
    util_timer *tmp = head;
    while (tmp){
        if (cur < tmp->expire){
            break;
        }
        tmp->cb_func(tmp->user_data);
        head = tmp->next;
        if (head){
            head->prev = NULL;
        }
        delete tmp;
        tmp = head;
    }
}


// 增加定时器的一般函数 从排序链表中找到应该插入的位置 
void sort_timer_lst::add_timer(util_timer *timer, util_timer *lst_head){
    util_timer *prev = lst_head;
    util_timer *tmp = prev->next;
    while (tmp){
        if (timer->expire < tmp->expire){
            prev->next = timer;
            timer->next = tmp;
            tmp->prev = timer;
            timer->prev = prev;
            break;
        }
        prev = tmp;
        tmp = tmp->next;
    }
    if (!tmp){
        prev->next = timer;
        timer->prev = prev;
        timer->next = NULL;
        tail = timer;
    }
}

