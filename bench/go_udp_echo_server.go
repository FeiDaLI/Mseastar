package main
import (
	"fmt"
	"net"
)
func main() {
	// 监听地址和端口
	port := 41235
	host := "0.0.0.0"
	addr := fmt.Sprintf("%s:%d", host, port)
	// 创建UDP地址
	udpAddr, err := net.ResolveUDPAddr("udp4", addr)
	if err != nil {
		fmt.Printf("解析地址错误: %v\n", err)
		return
	}
	// 创建UDP连接
	conn, err := net.ListenUDP("udp4", udpAddr)
	if err != nil {
		fmt.Printf("监听错误: %v\n", err)
		return
	}
	defer conn.Close()
	fmt.Printf("服务器正在监听 %s:%d\n", host, port)
	// 缓冲区用于接收数据
	buffer := make([]byte, 1024)
	for {
		// 读取数据
		n, clientAddr, err := conn.ReadFromUDP(buffer)
		if err != nil {
			fmt.Printf("读取错误: %v\n", err)
			continue
		}
		// 打印接收到的消息（注释掉了，与原Node.js代码一致）
		// fmt.Printf("服务器收到来自 %s:%d 的消息: %s\n", clientAddr.IP, clientAddr.Port, string(buffer[:n]))
		// 将接收到的消息原封不动地返回给客户端
		_, err = conn.WriteToUDP(buffer[:n], clientAddr)
		if err != nil {
			// 打印发送错误（注释掉了，与原Node.js代码一致）
			// fmt.Printf("回复 %s:%d 失败: %v\n", clientAddr.IP, clientAddr.Port, err)
		}
	}
}