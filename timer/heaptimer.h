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
  TimeoutCallBack cb;
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
  HeapTimer() { heap_.reserve(64); }
  ~HeapTimer() { clear(); }

  // 延后节点过期时间
  void adjust(int id, int newExpires);

  void add(int id, int timeout, const TimeoutCallBack& cb);

  // 做工作？做什么工作？
  void dowork(int id);

  void clear();

  // 删除过期节点
  void tick();

  void pop();

  int GetNextTick();
  
private:
  // 删除定时任务节点
  void del_(size_t i);
  
  /*
    这是堆排序中两个关键操作，以维护堆的性质
  */
  /*
    当堆中插入一个新的节点后
    将其上移到正确的位置以保持堆的性质
  */
  void siftup(size_t i);
  /*
    当堆删除节点后，将堆的最后一个节点移到根节点位置
    并将其下移到正确的位置以保持堆的性质
  */
  bool siftdown_(size_t index, size_t n);

  void SwapNode_(size_t i, size_t j);

  /*
    heap_：用于存储定时任务节点
  */
  std::vector<TimerNode> heap_;
  /*
    用于存储任务的唯一标识和其在堆中的位置，这个映射关系是为了快速实现任务的删除操作
    
    通过Node的id来找其在heap_中的索引
  */
  std::unordered_map<int, size_t> ref_;
};



#endif