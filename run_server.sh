#!/bin/bash

# 定义服务器和目录（使用数组）
SERVER_CMDS=(
    "./build/udp_server_test"
    "./bench/asio_udp_server"
    "./bench/go_udp_echo_server"
    "./bench/node udp_echo_server.js"
)

# 创建日志目录（如果不存在）
LOG_DIR="./logs"
mkdir -p "$LOG_DIR"

# 启动所有服务器并发执行
for i in "${!SERVER_CMDS[@]}"; do
    server_cmd="${SERVER_CMDS[$i]}"
    log_file="$LOG_DIR/server_$i.log"
    echo "启动服务器 $i: $server_cmd，日志将写入 $log_file"
    ./"$server_cmd" > "$log_file" 2>&1 &
done

echo "所有 UDP 服务器已启动！日志在 $LOG_DIR/"
echo "按 Ctrl+C 终止脚本（但不会终止后台服务器）"

# 等待所有后台任务完成（实际上这里可能不需要，因为通常你想让服务器一直运行）
# wait

# 如果你想让脚本一直运行（直到手动终止），可以添加一个无限循环
# trap "echo '收到终止信号，正在停止服务器...'; kill $(jobs -p); exit" SIGINT SIGTERM
# wait