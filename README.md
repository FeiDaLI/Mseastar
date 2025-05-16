# Seastar 学习项目

本项目用于学习 Seastar框架。




## 简化
去掉sstring;

去掉hwloc，使用/proc代替.



## 环境配置

### 系统依赖安装
```bash
# 安装必要系统依赖
sudo apt update
sudo apt install -y libboost-all-dev libsctp-dev lksctp-tools libaio-dev libunwind-dev


### 编译项目

```bash
make
```

### Todo

1. **UDP Server 性能优化**
   - 当前状态: 需改进压测结果
     	server_name	server_port	num_clients	num_packets	packet_size	total_packets_sent	total_errors	success_rate	throughput_mbps	duration_seconds
1	Mseastar	10000	500	5000	2048	2500000	0	100.00%	387.10	105.81
2	Node	41234	500	5000	2048	2500000	0	100.00%	446.71	91.69
3	Go	41235	500	5000	2048	2500000	0	100.00%	434.89	94.18

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

