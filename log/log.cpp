/*
  对于Log还有很多设计上的不理解的地方
  可以单独实现一个小型的日志类去理解一下
*/
#include "log.h"
#include <cstdio>
#include <mutex>
using namespace std;

Log::Log() {
  lineCount = 0;
  isAsync_ = false;
  writeThread_ = nullptr;
  deque_ = nullptr;
  toDay_ = 0;
  fp_ = nullptr;
}

Log::~Log() {
  // 如果异步线程启用了，并且这个异步线程还未汇入
  // 需要进行汇入，并且将队列中的数据全部写入缓冲区
  if(writeThread_ && writeThread_->joinable()) {
    while(!deque_->empty()) {
      deque_->flush();
    }
    deque_->Close();
    writeThread_->join();
  }
  if(fp_) {
    lock_guard<mutex> locker(mtx_);
    flush();
    // 这个头文件就在stdio.h中
    fclose(fp_);
  }
}

int Log::GetLevel() {
  lock_guard<mutex> locker(mtx_);
  return level_;
}

void Log::SetLevel(int level) {
  lock_guard<mutex> locker(mtx_);
  level_ = level;
}

void Log::init(int level = 1, const char* path, const char* suffix, int maxQueueSize) {
  isOpen_ = true;
  level_ = level;
  // 这个MaxQueueSize到底是什么意思？
  if(maxQueueSize > 0) {
    
  }
}