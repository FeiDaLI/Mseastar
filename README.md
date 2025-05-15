# Seastar 学习项目

本项目用于学习 Seastar框架。

## 环境配置

### 系统依赖安装
```bash
# 安装必要系统依赖
sudo apt update
sudo apt install -y libboost-all-dev libsctp-dev lksctp-tools libaio-dev libunwind-dev
# hwloc 安装请参考: https://www.open-mpi.org/software/hwloc/v1.11/


### 编译项目

```bash
make
```

### Todo

1. **UDP Server 性能优化**
   - 当前状态: 需改进压测结果
     ![QQ_1747317232904](https://github.com/user-attachments/assets/24e6e1a6-f744-41bd-9b16-d12352363ea3)

   - 目标: 优化现有实现，提升吞吐量和延迟表现

2. **TCP Server 实现**
   - 当前状态: 待实现
   - 目标: 完成基础 TCP 服务端功能开发

3. **架构改进**
   - 实现与定义分离，遵循清晰的分层设计

4. **HTTP Server 实现**
   - 当前状态: 待实现
   - 目标: 开发完整的 HTTP 服务端实现

5. **RPC Server 实现**
   - 当前状态: 待实现
   - 目标: 构建 RPC 服务框架

