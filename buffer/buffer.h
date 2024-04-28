#ifndef BUFFER_H
#define BUFFER_H

#include <cstddef>
#include <cstring> // perror
#include <iostream>
#include <unistd.h> // 意思就是Unix Standrand

#include <sys/uio.h>

#include <vector>
#include <atomic>
#include <assert.h>

/*
  用于数据写入、处理
  性能优化等操作
*/
class Buffer {
public:
  Buffer(int initBufferSize = 1024);
  ~Buffer() = default;

  size_t WriteableBytes() const;  // 写入数据
  size_t ReadableBytes() const ;  // 读取数据

  /*
    这个函数的名称没看出来是什么意思
    GPT：
      返回当前Buffer对象中可预留的字节数
      数据可以从缓冲区的前部（Prepend）或后部（append）进行读取和写入
      这个应该是返回当前缓冲区前部可用的字节数
  */
  size_t PrependableBytes() const;

  const char* Peek() const; // 查看当前Buffer中的数据
  
  /*
    为了高并发预留的两个函数
    这两个函数确保数据目前的状态
    避免出现数据竞争
  */
  void EnsureWriteable(size_t len);
  void HasWritten(size_t len);

  /*
    从缓冲区移除指定长度的数据？？？？？
  */
  void Retrieve(size_t len);
  void RetrieveUntil(const char* end);
  void RetrieveAll();
  
  std::string RetrieveAllToStr();
  
  /*
    这些函数也没看懂是干什么用的？？
    看起来是用于获取底层的两个指针的位置的
    两个指针详细见private
  */
  const char* BeginWriteConst() const;
  char* BeginWrite();

  /*
    向缓冲区中写入数据
  */
  void Append(const std::string& str);
  void Append(const char* str, size_t len);
  void Append(const void* data, size_t len);
  void Append(const Buffer& buff);

  // 不是很明白这个ssize_t是什么数据类型
  ssize_t ReadFd(int fd, int* Errno);
  ssize_t WriteFd(int fd, int* Errno);

private:
  char* BeginPtr_();
  const char* BeginPtr_() const;
  void MakeSpace_(size_t len);

  std::vector<char> buffer_;
  // 原子操作，防止数据出现竞态问题
  std::atomic<std::size_t> readPos_;
  std::atomic<std::size_t> writePos_;
};


#endif // BUFFER_H