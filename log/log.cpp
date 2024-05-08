#include "log.h"
#include "blockqueue.h"
#include <cstdarg>
#include <cstdio>
#include <ctime>
#include <memory>
#include <mutex>
#include <sys/select.h>

using namespace std;

Log::Log() {
  // 初始化行数计数器
  lineCount_ = 0;
  // 当前的日志记录是否是异步的？
  isAsync_ = false;
  writeThread_ = nullptr;
  deque_ = nullptr;
  toDay_ = 0;
  fp_ = nullptr;
}

Log::~Log() {
  if(writeThread_ && writeThread_ -> joinable()) {
    while(!deque_->empty()) {
      deque_->flush();
    }
    deque_->Close();
    writeThread_->join();
  }
  if(fp_) {
    lock_guard<mutex> locker(mtx_);
    flush();
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

void Log::init(int level = 1, const char* path, const char* suffix, 
              int maxQueueSize) {
  isOpen_ = true;
  level_ = level;

  /* 
    这个队列的大小和异步有什么关系？？？？
  */
  if(maxQueueSize > 0) {
    isAsync_ = true;
    /*
      这段代码有很多疑惑，还有很多地方没有太理解
    */
    if(!deque_) {
      unique_ptr<BlockDeque<std::string>> newDeque(new BlockDeque<std::string>);
      /*
        为什么这里要使用std::move，这是不是一种优化？
      */
      deque_ = std::move(newDeque);

      std::unique_ptr<std::thread> NewThread(new thread(FlushLogThread));
      writeThread_ = std::move(NewThread);
    }
  }
  else {
    isAsync_ = false;
  }

  lineCount_ = 0;

  time_t timer = time(nullptr);
  struct tm* sysTime = localtime(&timer);
  struct tm t = *sysTime;
  path_ = path;
  suffix_ = suffix;
  char fileName[LOG_NAME_LEN] = {0};
  snprintf(fileName, LOG_NAME_LEN - 1, "%s%04d_%02d_%02d%s",
    path_, t.tm_year + 1900, t.tm_mon + 1, t.tm_mday, suffix_);
  toDay_ = t.tm_mday;

  {
    lock_guard<mutex> locker(mtx_);
    // 清除缓冲区中的所有数据
    buff_.RetrieveAll();
    if(fp_) {
      // 将缓冲区的数据写入文件中
      flush();
      fclose(fp_);
    }

    fp_ = fopen(fileName, "a");
    if(fp_ == nullptr) {
      mkdir(path, 0777);
      fp_ = fopen(fileName, "a");
    }
    assert(fp_ != nullptr);
  }
}


void Log::write(int level, const char* format, ...) {
  struct timeval now = {0, 0};
  gettimeofday(&now, nullptr);
  time_t tSec = now.tv_sec;
  struct tm* sysTime = localtime(&tSec);
  struct tm t = *sysTime;
  // 用于处理可变数量参数·的函数，通常和stdarg.h一起使用
  va_list vaList;

  /*
    日期日志，日志行数
  */
  if(toDay_ != t.tm_mday || (lineCount_ && (lineCount_ % MAX_LINES == 0))) {
    unique_lock<mutex> locker(mtx_);

    /*
      为什么在这里不需要上锁了？
      是不是因为这个写入操作本身就不会出现并发问题？
    */
    locker.unlock();

    char newFile[LOG_NAME_LEN];
    char tail[36] = {0};
    snprintf(tail, 36, "%04d_%02d_%02d", t.tm_year + 1900, t.tm_mon + 1, t.tm_mday);

    if(toDay_ != t.tm_mday) {
      snprintf(newFile, LOG_NAME_LEN - 72, "%s/%s%s", path_, tail, suffix_);
      toDay_ = t.tm_mday;
      lineCount_ = 0;
    } else {
      snprintf(newFile, LOG_NAME_LEN - 72, "%s/%s-%d%s", path_, tail, (lineCount_  / MAX_LINES), suffix_);
    }

    locker.lock();
    flush();
    fclose(fp_);
    fp_ = fopen(newFile, "a");
    assert(fp_ != nullptr);
  }

  {
    unique_lock<mutex> locker(mtx_);
    lineCount_++;
    int n = snprintf(buff_.BeginWrite(), 128, "%d-%02d-%02d %02d:%02d:%02d.%06ld ",
                    t.tm_year + 1900, t.tm_mon + 1, t.tm_mday,
                    t.tm_hour, t.tm_min, t.tm_sec, now.tv_usec);
    buff_.HasWritten(n);
    AppendLogLevelTitle_(level);

    /*
      确定可变参数列表的起始位置
      一边之后使用va_arg来逐个获取和处理可变参数列表中的参数值

      这个是C语言的写法
      在C++中有更现代化的写法，就是使用C++的模板
      （一开始我是想使用initializer_list的，但是看了下好像没法很好的满足类型的要求）
      我希望在后续重写该项目的时候用上这个
    */
    va_start(vaList, format);
    int m = vsnprintf(buff_.BeginWrite(), buff_.WriteableBytes(), format, vaList);
    va_end(vaList);

    buff_.HasWritten(m);
    buff_.Append("\n\0, 2");

    if(isAsync_ && deque_ && !deque_->full()) {
      deque_->push_back(buff_.RetrieveAllToStr());
    } else {
      fputs(buff_.Peek(), fp_);
    }

    buff_.RetrieveAll();
  }
}

void Log::AppendLogLevelTitle_(int level) {
  switch(level) {
    case 0:
      buff_.Append("[debug]: ", 9);
      break;

    case 1:
      buff_.Append("[info]: ", 9);
      break;

    case 2:
      buff_.Append("[warn]: ", 9);
      break;
    
    case 3:
      buff_.Append("[error]: ", 9);
      break;

    default:
      buff_.Append("[info]: ", 9);
      break;
  }
}

void Log::flush() {
  if(isAsync_) {
    deque_->flush();
  }
  // 这个函数也是没见过的
  fflush(fp_);
}

// 异步写入？？？
// 这个异步究竟是什么异步？
void Log::AsyncWrite_() {
  string str = "";
  // 删除列表中第一个元素的同时，使用str对其进行获取
  while(deque_->pop(str)) {
    lock_guard<mutex> locker(mtx_);
    /*
      转换为C风格的字符串数组
      需要这么写的原因是：fputs的接口需要const char*
      而string本身是不能直接转换成cons char*的
    */
    fputs(str.c_str(), fp_);
  }
}

Log* Log::Instance() {
  // 初始化这个静态变量
  // 单例模式所需要
  static Log inst;
  return &inst;
}

// 
void Log::FlushLogThread() {
  Log::Instance()->AsyncWrite_();
}