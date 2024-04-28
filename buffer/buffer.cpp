#include "buffer.h"

// 规定缓冲区大小、读写指针的位置
Buffer::Buffer(int initBufferSize) : buffer_(initBufferSize), readPos_(0), writePos_(0) {}

/*
  可读字节的大小
  也就是已写入但是还未读的元素的个数
*/
size_t Buffer::ReadableBytes() const {
  return writePos_ - readPos_;
}

size_t Buffer::WriteableBytes() const {
  return buffer_.size() - writePos_;
}

/*
  从缓冲区已读取的字节数量
*/
size_t Buffer::PrependableBytes() const {
  return readPos_;
}

/*
  获取readPos_处的数据
*/
const char* Buffer::Peek() const {
  return BeginPtr_() + readPos_;
}

/*
  改变读指针的位置，使得中间的某块区域不可取
*/
void Buffer::Retrieve(size_t len) {
  assert(len <= ReadableBytes());
  readPos_ += len;
}

void Buffer::RetrieveUntil(const char* end) {
  assert(Peek() <= end);
  Retrieve(end - Peek());
}

void Buffer::RetrieveAll() {
  bzero(&buffer_[0], buffer_.size());
  readPos_ = 0;
  writePos_ = 0;
}

/*
  将所有可读对象转换成string
  然后从缓冲区中移除
*/
std::string Buffer::RetrieveAllToStr() {
  std::string str(Peek(), ReadableBytes());
  RetrieveAll();
  return str;
}

/*
  写入数据的位置
  这两个函数好像十分重复啊
*/
const char* Buffer::BeginWriteConst() const {
  return BeginPtr_() + writePos_;
}
char* Buffer::BeginWrite() {
  return BeginWrite() + writePos_;
}

void Buffer::HasWritten(size_t len) {
  writePos_ += len;
}

void Buffer::Append(const std::string& str) {
  // 这个data()有必要说一下
  // data()的返回值是字符串首元素的指针
  Append(str.data(), str.size());
}

// 后面会有，主要于备用的buff的填充
void Buffer::Append(const char* str, size_t len) {
  assert(str);
  /*
    在写入数据之前，确保有这么多空间写入数据
    疑惑：这个函数的返回值为什么不设置为bool？
  */
  EnsureWriteable(len);
  std::copy(str, str + len, BeginWrite());
  HasWritten(len);
}

/*
  将另一个缓冲区的未读数据添加至本缓冲区
*/
void Buffer::Append(const Buffer& buff) {
  Append(buff.Peek(), buff.ReadableBytes());
}

void Buffer::EnsureWriteable(size_t len) {
  // 若是可写数据区大小不够
  // 进行扩容
  if(WriteableBytes() < len){
    MakeSpace_(len);
  }
  assert(WriteableBytes() >= len);
}

/*
  ssize_t 就是Linux中使用read, write这类函数的返回值类型
  size_t 的最大值是 SIZE_MAX
  ssize_t 的存储范围是 [-SIZE_MAX, SIZE_MAX]
*/
ssize_t Buffer::ReadFd(int fd, int* saveErrno) {
  // 为什么这个选择这个数字？
  // 其中一点：在TCP传输中，一个字节流的最大编号是65535
  char buff[65535];
  /*
    struct iovec
    {
      void *iov_base;	 // Pointer to data. 
      size_t iov_len;	 // Length of data.
    };
  */
  struct iovec iov[2];
  const size_t writable = WriteableBytes();
  /*
    分散读，保证数据全部读完
    buff是备用的数据缓冲区
    就是为了应对buffer_大小不够的时候
    所以65535这个数字很有讲究
  */
  iov[0].iov_base = BeginPtr_() + writePos_;
  iov[0].iov_len = writable;
  iov[1].iov_base = buff;
  iov[1].iov_len = sizeof(buff);

  const ssize_t len = readv(fd, iov, 2);
  if(len < 0) {
    // errno就是一个整数
    /* 
      当发生错误的时候，使用saveErrno将errno保存起来
      然后进行进一步的错误处理
    */
    *saveErrno = errno;
  }
  else if(static_cast<size_t>(len) <= writable) {
    writePos_ += len;
  }
  else {
    // buffer_满了
    writePos_ = buffer_.size();
    // 需要启用buff
    Append(buff, len - writable);
  }
  return len;
}

/*
  将buff中的数据写入文件中
  算是读取数据
*/
ssize_t Buffer::WriteFd(int fd, int* saveErrno) {
  size_t readSize = ReadableBytes();
  ssize_t len = write(fd, Peek(), readSize);
  if(len < 0) {
    *saveErrno = errno;
    return len;
  }
  readPos_ += len;
  return len;
}

char* Buffer::BeginPtr_() {
  // return &(*buffer_.begin());
  // 上面的是原版
  // 我自作主张将其更换为buffer_.data()
  return buffer_.data();
}

const char* Buffer::BeginPtr_() const {
  return buffer_.data();
}

// 这个len是需要存储的数据的长度
void Buffer::MakeSpace_(size_t len) {
  // 为什么是可写+已读？
  if(WriteableBytes() + PrependableBytes() < len) {
    // 这个+1应该是为了存储\0
    buffer_.resize(writePos_ + len + 1);
  }
  else {
    size_t readable = ReadableBytes();
    std::copy(BeginPtr_() + readPos_, BeginPtr_() + writePos_, BeginPtr_());
    readPos_ = 0;
    writePos_ = readable;
    assert(readable == ReadableBytes());
  }
}