#ifndef SQLCONNPOOL_H
#define SQLCONNPOOL_H

#include <mysql/mysql.h>
#include <string>
#include <queue>
#include <mutex>
#include <thread>
// 这个头文件提供了信号量相关的数据结构和函数
// 这个头文件是POSIX标准下的，而condition_variable是C++官方提出的
// 两种头文件的应用场景有些许的差别
#include <semaphore.h>

/*
  也是使用了单例模式
*/
class SqlConnPool {
public:
  static SqlConnPool* Instance();
  
  MYSQL* GetConn();
  void FreeConn(MYSQL* conn);
  int GetFreeConnCount();

  void Init(const char* host, int port,
            const char* user, const char* pwd,
            const char* dbName, 
            int connSize);
  void ClosePool();

private:
  SqlConnPool();
  ~SqlConnPool();

  int MAX_CONN_;
  int useCount_;
  int freeCount_;

  std::queue<MYSQL*> connQue_;
  std::mutex mtx_;
  // 信号量
  sem_t semId_;
};

#endif