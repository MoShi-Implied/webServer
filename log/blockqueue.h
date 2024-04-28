#ifndef BLOCKQUEUE_H
#define BLOCKQUEUE_H

#include <mutex>
#include <deque>
#include <assert.h>
#include <condition_variable>
#include <sys/time.h>
#include <sys/types.h>

template<class T>
class BlockDeque {
public:
  explicit BlockDeque(size_t MaxCapacity = 1000);
  
  ~BlockDeque();

  void clear();
  
  bool empty();

  bool full();

  void Close();
  
  size_t size();
  
  size_t capacity();

  T front();
  
  T back();
  
  void push_back(const T& item);
  
  void push_front(const T& item);

  /*
    这两个参数其实都是无参的
    传入的item其实是用于获取pop所取出的元素的值
  */
  bool pop(T& item);
  bool pop(T& item, int timeout);

  // 将数据写入缓冲区
  void flush();

private:
  std::deque<T> deq_;
  
  size_t capacity_;

  std::mutex mtx_;
  
  // 用于表示该队列是否已经被关闭
  // 若是为true，就表示队列不能再接收新的元素了
  bool isClose_;

  std::condition_variable condConsumer_;
  std::condition_variable condProducer_;
};

template<class T>
BlockDeque<T>::BlockDeque(size_t MaxCapacity) : capacity_(MaxCapacity) {
  assert(MaxCapacity > 0);
  isClose_ = false;
}

template<class T>
BlockDeque<T>::~BlockDeque() {
  Close();
}

/*
  对这个函数的实现有很大的疑惑
*/
template<class T>
void BlockDeque<T>::Close() {
  {
    std::lock_guard<std::mutex> locker(mtx_);
    // 为什么在此处需要一个清空队列的操作？
    // 队列满了，我不完成就要清空吗？
    deq_.clear();
    isClose_ = true;
  }
  // 为什么是通知所有线程？
  condProducer_.notify_all();
  condConsumer_.notify_all();
}

template<class T>
void BlockDeque<T>::flush() {
  // 为什么仅仅是通知消费者前来消费？
  condConsumer_.notify_one();
}

template<class T>
void BlockDeque<T>::clear() {
  std::lock_guard<std::mutex> locker(mtx_);
  deq_.clear();
}

template<class T>
T BlockDeque<T>::front() {
  std::lock_guard<std::mutex> locker(mtx_);
  return deq_.front();
}

template<class T>
T BlockDeque<T>::back() {
  std::lock_guard<std::mutex> locker(mtx_);
  return deq_.back();
}

template<class T>
size_t BlockDeque<T>::size() {
  std::lock_guard<std::mutex> locker(mtx_);
  return deq_.size();
}

template<class T>
size_t BlockDeque<T>::capacity() {
  std::lock_guard<std::mutex> locker(mtx_);
  return capacity_;
}

template<class T>
void BlockDeque<T>::push_back(const T& item) {
  std::unique_lock<std::mutex> locker(mtx_);
  /*
    为什么使用while循环？
    ：“虚假唤醒”
  */
  while(deq_.size() >= capacity_) {
    // 这个wait做了些什么事情？
    /*
      wait先释放locker的所有权
      因为条件变量在等待的时候持有锁的所有权也没用
      若是一直锁上还不解锁，就可能会有死锁问题
      然后当条件变量被通知的时候，会重新获取锁的所有权
    */
    // 在等待条件变量的时候，还会阻塞线程
    condProducer_.wait(locker);
  }
  // 有位置了，可以添加任务了
  deq_.push_back(item);
  // 通知消费者有任务了
  condConsumer_.notify_one();
}

template<class T>
void BlockDeque<T>::push_front(const T& item) {
  std::unique_lock<std::mutex> locker(mtx_);
  while(deq_.size() >= capacity_) {
    condProducer_.wait(locker);
  }
  deq_.push_front(item);
  condConsumer_.notify_one();
}

template<class T>
bool BlockDeque<T>::empty() {
  std::lock_guard<std::mutex> locker(mtx_);
  return deq_.empty();
}

template<class T>
bool BlockDeque<T>::full() {
  std::lock_guard<std::mutex> locker(mtx_);
  // 此处的>=是用于维护代码的健壮性，即使>在正常情况下是不可能存在的
  return deq_.size() >= capacity_;
}

template<class T>
bool BlockDeque<T>::pop(T& item) {
  std::unique_lock<std::mutex> locker(mtx_);
  // 不能是空的，如果为空，就说明程序出错了
  while(deq_.empty()) {
    // 没有数据可以消耗
    // 等通知，等需要消费者的时候再唤醒
    condConsumer_.wait(locker);
    // 如果队列处于关闭状态
    if(isClose_) {
      return false;
    }
  }
  item = deq_.front();
  deq_.pop_front();
  condProducer_.notify_one();
  return true;
}

template<class T>
bool BlockDeque<T>::pop(T& item, int timeout) {
  std::unique_lock<std::mutex> locker(mtx_);
  while(deq_.empty()) {
    // 超时了，还没有能进行消费的数据
    if(condConsumer_.wait_for(locker, std::chrono::seconds(timeout)) == std::cv_status::timeout) {
      return false;
    }
  }
  item = deq_.front();
  deq_.pop_front();
  condProducer_.notify_one();
  return true;
}

#endif // BLOCKDEQUE_H