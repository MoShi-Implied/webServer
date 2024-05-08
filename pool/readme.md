[@TOC]
## 内容概要
这个文件夹负责了两个组件的实现

  1. SQL连接池
  2. 线程池

两者都使用了池化技术，对我来说，这里可能会很有难度

2024.5.8

## 文件编写中遇到的诸多疑惑
### threadpool.h
在其中我又很多地方其实是一知半解，其中的大多数不懂的地方我都使用了备注进行了标记，但是此处我觉得记录一些令我影响深刻的问题还是很有必要的
#### toword
这个东西我还是第一次使用，在AddTask中，但是我到现在都还是没有完全看懂，即使我查看了[cpprefrence](https://zh.cppreference.com/w/cpp/utility/forward)中的相关解释。
根据GPT的解释就是，==它可以将一个函数中的参数进行一个包装，从而使其能够很完美的交付给其它的函数进行使用，这个完美就意味着<font color="red">不会进行隐式转换</font>，同时会保留const等关键字==

#### condition_variable
<font color="red">对于条件变量的理解还是太浅显了</font>
当我一开始看到wait需要传入一把锁的时候我真的是非常的疑惑，不理解这是为什么，于是我询问了GPT。
得到的回答就是：==当wait的时候不能拥有locker的所有权，否则会发生死锁等现象==，于是当线程阻塞的时候释放锁，而收到条件变量通知的时候，我们再次重新获取互斥体的所有权，这样既不会发生死锁现象，也不会有线程安全的烦恼。
也正是由于wait会进行一个临时解锁，因此我们这个传入的锁需要是**unique_lock**而不是**lock_guard**，或者说如果有其它的锁可以支持临时的解锁和上锁也是可以的（比如**多重锁**）。

### sqlconnpool.h
其中有很多设计上的问题不是很了解
同时，对C/C++操作MySQL数据库也不是很了解
前者我觉得我需要回顾代码，而后者我觉得还需要写几个小的项目来加强理解，同时可能需要再补充下MySQL相关的知识。