#ifndef SQLCONNRAII_H
#define SQLCONNRAII_H

#include "./sqlconnpool.h"
#include <assert.h>

class SqlConnRAII {
public:
  // 此处的sql指针为什么这么使用？
  SqlConnRAII(MYSQL** sql, SqlConnPool* connpool) {
    assert(connpool);
    // 获得一个连接
    *sql = connpool->GetConn();
    sql_ = *sql;
    connpool_ = connpool;
  }

  ~SqlConnRAII() {
    if(sql_) {
      connpool_->FreeConn(sql_);
    }
  }
private:
  MYSQL* sql_;
  SqlConnPool* connpool_;
};

#endif