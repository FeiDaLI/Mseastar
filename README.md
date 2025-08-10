# Seastar 学习项目

本项目用于学习 Seastar框架。




## 简化
去掉sstring;
去掉DPDK用户态网络栈;
去掉hwloc，使用/proc代替.



## 环境配置

### 系统依赖安装
```bash
# 安装必要系统依赖
sudo apt update
sudo apt install g++ make
sudo apt install -y libboost-all-dev libsctp-dev lksctp-tools libaio-dev libunwind-dev


### 编译项目

```bash
make
```

### Todo

 **架构改进**
   - 实现与定义分离，遵循清晰的分层设计
      
**使用boost MPI和openMP重构项目**
去掉parrel for each，mapreduce等辅助性功能。

**提高boost无锁队列的性能**

**去掉不必要的功能**
（1）networkstack不需要写这么多抽象，只需要一个posix即可.

（2）使用epoll和aio即可，不需要考虑io_uring.

（3）future和promise只需要实现最基本的功能。目前的future和promise的代码太臃肿了，需要精简。







