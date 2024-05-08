#ifndef THREADPOOL_H
#define THREADPOOL_H

#include <stdlib.h>
#include <assert.h>
#include <mutex>
#include <thread>
#include <queue>
#include <functional>
#include <condition_variable>
#include <memory>
#include <utility>

class ThreadPool {
public:
  explicit ThreadPool(size_t threadCount = 8) {
    assert(threadCount > 0);
    for(size_t i = 0; i < threadCount; i++) {
      std::thread([pool = pool_](){
        std::unique_lock<std::mutex> locker(pool->mtx);
        while(true) {
          if(!pool->tasks.empty()) {
            // 取出来的就是一个函数
            std::function<void()> task = std::move(pool->tasks.front());
            pool->tasks.pop();
            locker.unlock();
            // 执行这个函数，不会有临界问题
            task();
            locker.lock();
          }
          else if(pool->isClosed) 
            break;
          else {
            /*
              对于条件变量还是有很多疑惑
              就比如这里我就还是不知道为什么需要传入一个锁

              条件变量在使用wait的时候
              传入的这个锁首先会被“释放”
              当收到通知的时候，锁的所有权又会再次被获取，这是为了线程安全

              现在似乎理解了
            */
            pool->cond.wait(locker);
          }
        }
      }).detach(); // 变成守护进程
    }
  }

  ThreadPool() = default;
  ThreadPool(ThreadPool&&) = default;

  ~ThreadPool() {
    // 首先检查这个线程池是否已经被初始化，若是没被初始化就会是nullptr
    if(static_cast<bool>(pool_)) {
      {
        std::lock_guard<std::mutex> locker(pool_->mtx);
        // 关闭线程池的标识设置为true
        pool_->isClosed = true;
      }
      
      /*
        这一步需要翻看上面启动守护线程的代码
        在while循环中，它会检查isClosed标识的状态
        若是发现线程池被关闭，就会退出线程，这应该就是起到了这个作用
        先停止各个线程的阻塞状态，进入下次循环中，就可以进入到关闭线程的条件分支了
      */
      pool_->cond.notify_all();
    }
  }

/*
  可以看看这个函数的使用，我这里还是没有太看懂
*/
template<class F>
void AddTask(F&& task) {
  {
    std::lock_guard<std::mutex> locker(pool_->mtx);
    /*
      这个又没有见过了
      这个forword又是什么？

      forword：一种包装器
      这种包装器可以将一个函数的参数原封不动的传递给另一个参数，同时保证参数的原有属性
    */
    pool_->tasks.emplace(std::forward<F>(task));
  }
  pool_->cond.notify_one();
}

private:
  struct Pool {
    std::mutex mtx;
    // 通知什么时候会有任务
    std::condition_variable cond;
    bool isClosed;

    /*
      关于function的使用我毫无了解，需要去学一下
      每一个Pool都有一个任务队列
    */
    std::queue<std::function<void()>> tasks;
  };
  std::shared_ptr<Pool> pool_;
};
#endif