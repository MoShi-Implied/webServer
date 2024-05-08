#ifndef TIMER_H
#define TIMER_H

#include <queue>
#include <unordered_map>
#include <time.h>
#include <algorithm>
#include <arpa/inet.h>
#include <functional>
#include <assert.h>
#include <chrono>
#include "../log/log.h"


typedef std::function<void()> TimeoutCallBack;
typedef std::chrono::high_resolution_clock Clock; // 高精度时钟？
typedef std::chrono::milliseconds MS;
typedef Clock::time_point TimeStamp; // 时间节点

/*
  从设计上来说，不是很理解这个结构体的作用
  时间节点
  
  其中有任务的唯一标识和到期时间
*/
struct TimerNode {
  int id;
  TimeStamp expires;
  bool operator<(const TimerNode& t) {
    //时间点之间的比较
    return expires < t.expires; 
  }
};

/*
  用于管理定时任务
    1. 任务调度
    2. 超时处理
    3. 定时任务调度
*/
class HeapTimer {
public:

private:
  void del_(size_t i);
  
  void siftup(size_t i);

  void siftdown(size_t index, size_t n);

  void SwapNode_(size_t i, size_t j);

  // 这两个东西的作用是什么？
  std::vector<TimerNode> heap_;
  std::unordered_map<int, size_t> ref_;
};

#endif