#ifndef LOG_H
#define LOG_H

#include <memory>
#include <mutex>
#include <string>
#include <sys/time.h>
#include <thread>
#include <string.h>
// 和可变参数有关
#include <stdarg.h>
#include <assert.h>
// 文件状态操作和文件权限等
#include <sys/stat.h>
#include "blockqueue.h"
#include "../buffer/buffer.h"

class Log {
public:
  // 初始化日志对象
  void init(int level, const char* path = "./log", 
            const char* suffix = ".log",
            int maxQueueCapacity = 1024);
  
  // 用于获取Log类的单例实例
  static Log* Instance();

  /*
    (
      是不是其中还有创建的意思？
      因为在实现的时候启动新线程的时候是以这个函数作为线程的入口
      详细见log.cpp::64
    )
   启动异步写入日志的线程
   这个函数还没太搞懂
  */
  static void FlushLogThread();

  // 写入日志消息
  void write(int level, const char* format, ...);
  // 将缓冲区的日志写入文件中
  void flush();

  // 获取当前日志级别
  int GetLevel();
  // 设置目前日志级别
  void SetLevel(int level);
  bool IsOpen() { return isOpen_; }

private:
  Log();
  // 向缓冲区写入日志级别的标题
  void AppendLogLevelTitle_(int level);
  // 为什么单独将析构函数设置为虚函数？
  virtual ~Log();
  /*
    异步写入日志到文件
  */
  void AsyncWrite_();

private:
  static const int LOG_PATH_LEN = 256;
  static const int LOG_NAME_LEN = 256;
  static const int MAX_LINES = 50000;
  
  const char* path_;
  const char* suffix_;
  
  // 日志的最大行数
  int MAX_LINES_;
  
  int lineCount_;
  // 用于标记是否到了新的日志
  int toDay_;
  
  bool isOpen_;

  // 用于储存日志消息
  Buffer buff_;
  // 当前日志的级别
  int level_;
  // 当前的日志是否是异步的
  bool isAsync_;

  // 用于指向当前日志文件的文件描述符
  FILE* fp_;
  // 用于存储待写入的日志
  std::unique_ptr<BlockDeque<std::string>> deque_;
  // 用于异步写入日志的线程
  std::unique_ptr<std::thread> writeThread_;
  std::mutex mtx_;
};

// 如果不使用这种宏展开，就需要使用模板了
#define LOG_BASE(level, format, ...) \
  do { \
    Log* log = Log::Instance(); \
    if(log->IsOpen() && log->GetLevel() <= level) { \
      log->write(level, format, ##__VA_ARGS__); \
      log->flush(); \
    } \
  }while(0);

#define LOG_DEBUG(format, ...) do {LOG_BASE(0, format, ##__VA_ARGS__)} while(0);
#define LOG_INFO(format, ...) do {LOG_BASE(1, format, ##__VA_ARGS__)} while(0);
#define LOG_WARN(format, ...) do {LOG_BASE(1, format, ##__VA_ARGS__)} while(0);
#define LOG_ERROR(format, ...) do {LOG_BASE(2, format, ##__VA_ARGS__)} while(0);

#endif // LOG_H