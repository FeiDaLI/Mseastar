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








```
Mseastar
├─ README.md
├─ bench
│  ├─ asio_http_Server.cc
│  ├─ asio_udp_server
│  ├─ go_http_server.go
│  ├─ go_udp_echo_server
│  ├─ go_udp_echo_server.go
│  ├─ http_test.py
│  ├─ kill_port.sh
│  ├─ node_http.js
│  └─ udp_echo_server.js
├─ condition_variable_state_diagram.puml
├─ coroutine_test
├─ demo_test
│  ├─ a.out
│  ├─ apply_demo.cc
│  ├─ future_demo.cc
│  ├─ lambda_list
│  ├─ lambda_list.cc
│  ├─ rvalue
│  ├─ rvalue_demo.cc
│  ├─ task_demo.cc
│  ├─ uni
│  └─ unique_ptr_demo.cc
├─ future_state_test
├─ include
│  ├─ HTTP
│  │  ├─ api_docs.hh
│  │  ├─ common.hh
│  │  ├─ exception.hh
│  │  ├─ file_handler.hh
│  │  ├─ function_handlers.hh
│  │  ├─ handlers.hh
│  │  ├─ http_response_parser.rl
│  │  ├─ httpd.hh
│  │  ├─ json_path.hh
│  │  ├─ matcher.hh
│  │  ├─ matchrules.hh
│  │  ├─ mime_types.hh
│  │  ├─ ragel.hh
│  │  ├─ reply.hh
│  │  ├─ request.hh
│  │  ├─ request_parser.hh
│  │  ├─ request_parser.rl
│  │  ├─ routes.hh
│  │  └─ transformers.hh
│  ├─ app
│  │  ├─ app-template.cc
│  │  └─ app-template.hh
│  ├─ fd
│  │  ├─ posix.cc
│  │  └─ posix.hh
│  ├─ future
│  │  ├─ bitset.h
│  │  ├─ chunked_fifo.hh
│  │  ├─ deleter.hh
│  │  ├─ do_with.hh
│  │  ├─ expiring_fifo.hh
│  │  ├─ file_desc.hh
│  │  ├─ future.hh
│  │  ├─ future_all12.cc
│  │  ├─ future_all12.hh
│  │  ├─ io_queue.cc
│  │  ├─ io_queue.hh
│  │  ├─ memory.cc
│  │  ├─ memory.hh
│  │  ├─ net.cc
│  │  ├─ net.hh
│  │  ├─ network_stack.hh
│  │  ├─ packet.cc
│  │  ├─ packet.hh
│  │  ├─ semaphore.cc
│  │  ├─ semaphore.hh
│  │  ├─ signals.hh
│  │  ├─ socket_api.hh
│  │  ├─ stream.cc
│  │  ├─ stream.hh
│  │  ├─ temp_buffer.cc
│  │  ├─ temp_buffer.hh
│  │  ├─ timer.cc
│  │  └─ timer.hh
│  ├─ json
│  │  ├─ formatter.cc
│  │  ├─ formatter.hh
│  │  ├─ json2code.py
│  │  ├─ json_elements.cc
│  │  └─ json_elements.hh
│  ├─ log
│  │  ├─ log.cc
│  │  └─ log.hh
│  ├─ memcached
│  │  ├─ ascii.hh
│  │  ├─ ascii.rl
│  │  ├─ memcache.cc
│  │  ├─ memcached.hh
│  │  └─ slab.hh
│  ├─ resource
│  │  ├─ resource.cc
│  │  └─ resource.hh
│  ├─ task
│  │  ├─ continuation.hh
│  │  ├─ reactor_demo.cc
│  │  ├─ reactor_demo.hh
│  │  └─ task.hh
│  ├─ test
│  │  ├─ exchanger.hh
│  │  ├─ test-utils.cc
│  │  ├─ test-utils.hh
│  │  ├─ test_runner.cc
│  │  └─ test_runner.hh
│  ├─ timer
│  │  └─ bitset-iter.hh
│  └─ util
│     ├─ align.hh
│     ├─ backtrace.hh
│     ├─ bitops.hh
│     ├─ bool_class.hh
│     ├─ const.hh
│     ├─ defer.hh
│     ├─ deleter.hh
│     ├─ eclipse.hh
│     ├─ shared_ptr.hh
│     ├─ shared_ptr_debug_helper.hh
│     ├─ spinlock.hh
│     ├─ temporary_buffer.hh
│     ├─ tuple_utils.hh
│     └─ unaligned.hh
├─ logs
│  ├─ server_0.log
│  ├─ server_1.log
│  ├─ server_2.log
│  └─ server_3.log
├─ main.cpp
├─ makefile
├─ reactor_demo_test
├─ run_server.sh
├─ script
│  ├─ __init__.py
│  ├─ all_results.csv
│  ├─ bench.sh
│  ├─ com.png
│  ├─ http_client.py
│  ├─ node_seastar_test_results.csv
│  ├─ plot.py
│  ├─ results.csv
│  ├─ run_udp_tests.py
│  ├─ seastar_test_results.csv
│  ├─ throughput_comparison.png
│  └─ udp_tester.py
├─ shared_ptr_class_diagram.puml
├─ test
│  ├─ fiber_test
│  │  └─ fiber.cc
│  ├─ http_test
│  │  ├─ http_server.cc
│  │  └─ http_test.cc
│  ├─ load_balance_test
│  │  ├─ client
│  │  ├─ client.c
│  │  ├─ server
│  │  ├─ server.c
│  │  ├─ tcp_client
│  │  ├─ tcp_client.c
│  │  ├─ tcp_server
│  │  └─ tcp_server.c
│  ├─ mem_test
│  │  └─ mem_test.cc
│  ├─ memcached
│  │  ├─ test.py
│  │  ├─ test_ascii_parser.cc
│  │  └─ test_memcached.py
│  ├─ net_test
│  │  ├─ packet_test.cc
│  │  ├─ udp_client.cc
│  │  ├─ udp_server.cc
│  │  └─ udp_zero_copy.cc
│  ├─ new_tests
│  │  ├─ expiring_fifo_test.cc
│  │  ├─ future_promise_test.cc
│  │  ├─ reactor_demo_test.cc
│  │  ├─ smp_test.cc
│  │  └─ thread_test.cc
│  ├─ old_tests
│  │  ├─ future_promise_test.cc
│  │  ├─ future_test.cc
│  │  ├─ mem_test.cc
│  │  ├─ resource_test.cc
│  │  ├─ shared_ptr_test.cc
│  │  ├─ task_test.cc
│  │  ├─ temporary_buffer_test.cc
│  │  └─ timer_test.cc
│  ├─ others
│  │  ├─ MESI_test
│  │  ├─ MESI_test.cc
│  │  ├─ atomic_test
│  │  ├─ atomic_test.cc
│  │  ├─ benchmark
│  │  ├─ coroutine.cc
│  │  ├─ coroutine_test.cc
│  │  ├─ lock_test.cc
│  │  ├─ rw_lock_test
│  │  ├─ rw_lock_test.cc
│  │  ├─ shared
│  │  ├─ shared_ptr_test.cc
│  │  ├─ sys_call.cc
│  │  ├─ thread_test.cc
│  │  ├─ virtual_test
│  │  └─ virtual_test.cc
│  ├─ posix_test
│  │  └─ condition_v.c
│  └─ timer_test
│     └─ timer_demo.cc
└─ thread_test

```