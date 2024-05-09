#include "heaptimer.h"
#include <cassert>
#include <chrono>

/*
  这个堆是个小顶堆
*/
// 向上堆化操作
void HeapTimer::siftup(size_t i) {
  // i是TimeNode的下标，所以一定要在heap_中
  assert(i >= 0 && i < heap_.size());
  size_t j = (i - 1) / 2;
  while(j >= 0) {
    // 有一个满足性质了，所有的都是满足性质的
    if(heap_[j] < heap_[i]) {
      break;
    }
    SwapNode_(i, j);
    i = j;
    // j是父节点的索引计算方法
    // i 的父节点就是 (i-1)/2
    j = (i-1) / 2; 
  }
}

void HeapTimer::SwapNode_(size_t i, size_t j) {
  assert(i >= 0 && i < heap_.size());
  assert(j >= 0 && j < heap_.size());
  std::swap(heap_[i], heap_[j]);

  // 对这个映射的更新我还需要再想想
  ref_[heap_[i].id] = i;
  ref_[heap_[j].id] = j;
}

/*
  为什么siftup_的返回值是void
  而这里的是bool？

  index是待进行下沉操作的节点在堆中的索引
  n是当前堆中有效元素的个数
  
  当堆顶元素被删除的时候
  一般会使用堆中的最后一个元素，将其放至堆顶
  因此，堆的性质就会被破坏，此时就需要使用下沉操作，将该节点下沉至合适的位置

  这个函数还是有很多疑惑
*/
bool HeapTimer::siftdown_(size_t index, size_t n) {
  // 确保索引在合法范围内
  assert(index >= 0 && index < heap_.size());
  assert(n >= 0 && n < heap_.size());

  size_t i = index; // 当前节点的索引
  // 计算左子节点的索引
  size_t j = i * 2 + 1;
  // 当左子节点存在时继续循环
  // 若是一直没有符合条件的，会一直下沉到堆底
  while(j < n) {
    // 如果右子节点存在且右子节点的值小于左子节点的值，则选择右子节点
    if(j + 1 < n &&  heap_[j + 1] < heap_[j]) {
      j++;
    }
    // 如果当前节点的值小于等于选择的子节点的值，则停止循环（已经符合堆的性质了）
    if(heap_[i] < heap_[j]) {
      break;
    }
    // 否则，交换当前节点和选择的子节点，并更新当前节点的索引
    SwapNode_(i, j);
    i = j;
    j = i * 2 + 1;
  }
  // 返回是否有节点进行了下沉操作
  return i > index;
}

void HeapTimer::add(int id, int timeout, const TimeoutCallBack& cb) {
  assert(id >= 0);
  size_t i;
  if(ref_.count(id) == 0) {
    /*
      这是一个新节点，堆尾插入后，向上堆化
    */
    i = heap_.size(); // 这个是什么意思
    ref_[id] = i; // id所对应的下标为i
    heap_.push_back({id, Clock::now() + MS(timeout), cb});
    siftup(i); // 从底向上
  }
  else {
    /* 
      已有节点，调整堆
    */
    i = ref_[id];
    // 更新节点的过期时间和回调函数
    heap_[i].expires = Clock::now() + MS(timeout);
    heap_[i].cb = cb;
    /*
      这两步堆化的操作实际上就是为了维护小顶堆的性质
      仔细想想其实就能够理解的
    */
    // 若是没法向下堆化
    if(!siftdown_(i, heap_.size())) {
      // 那就向上堆化
      siftup(i);
    }
  }
}

void HeapTimer::dowork(int id) {
  /*
    删除指定ide节点，并触发回调函数
  */
  if(heap_.empty() || ref_.count(id) == 0) {
    return;
  }

  // 找到该回调函数在堆中的索引
  size_t i = ref_[id];
  TimerNode node = heap_[i];
  node.cb();
  del_(i);
}

void HeapTimer::del_(size_t index) {
  /*
    删除指定位置的节点
  */
  assert(!heap_.empty() && index >= 0 && index < heap_.size());
  /*
    将要删除的节点换到队尾，然后调整堆
  */
  size_t i = index;
  // -1是因为这是一个删除操作，在删除之后有效个数就减少了
  size_t n = heap_.size() - 1;
  assert(i <= n);
  if(i < n) {
    SwapNode_(i, n);
    if(!siftdown_(i, n)) {
      siftup(i);
    }
  }
  /*
    删除队尾元素
    同时映射也需要删除
  */
   ref_.erase(heap_.back().id);
   heap_.pop_back();
}

void HeapTimer::adjust(int id, int timeout) {
  /*
    调整指定id的节点
  */
  assert(!heap_.empty() && ref_.count(id) > 0);
  // 演唱过期时间，根据之前定义的比较函数，节点值变大了
  heap_[ref_[id]].expires = Clock::now() + MS(timeout);
  // 上面说到，由于节点值是变大的，因此只需要向下堆化就行了
  siftdown_(ref_[id], heap_.size());
}

void HeapTimer::tick() {
  /*
    清除超时节点
  */
  if(heap_.empty()) {
    return;
  }
  while(!heap_.empty()) {
    /*
      在小顶堆中，堆顶是最小的，因此也是最早过期的节点
    */
    TimerNode node = heap_.front();
    if(std::chrono::duration_cast<MS>(node.expires - Clock::now()).count() > 0) {
      break;
    }
    // 超时直接执行回调函数
    node.cb();
    pop();
  }
}

// 删除堆顶
void HeapTimer::pop() {
  assert(!heap_.empty());
  del_(0);
}

void HeapTimer::clear() {
  ref_.clear();
  heap_.clear();
}

/*
  获得下一个超时节点的距离超时的时间

  返回-1说明堆中为空
  返回0说明已经超时了
  返回>0说明还未超时
*/
int HeapTimer::GetNextTick() {
  // 首先清除已经超时的节点
  tick();
  size_t res = -1;
  if(!heap_.empty()) {
    // res就是过期时间
    res = std::chrono::duration_cast<MS>(heap_.front().expires - Clock::now()).count();
    if(res < 0) {
      res = 0;
    }
  }
  return res;
}