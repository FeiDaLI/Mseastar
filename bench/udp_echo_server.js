const dgram = require('dgram');
// 创建 UDP 服务器
const server = dgram.createSocket('udp4');
// 监听消息事件
server.on('message', (msg, rinfo) => {
    // console.log(`服务器收到来自 ${rinfo.address}:${rinfo.port} 的消息: ${msg.toString()}`);
    // 将接收到的消息原封不动地返回给客户端
    server.send(msg, 0, msg.length, rinfo.port, rinfo.address, (err) => {
        if (err) {
            // console.log(`回复 ${rinfo.address}:${rinfo.port} 失败: ${err.message}`);
        }
    });
});
// 监听服务器启动事件
server.on('listening', () => {
    const address = server.address();
    console.log(`服务器正在监听 ${address.address}:${address.port}`);
});
// 监听错误事件
server.on('error', (err) => {
    console.log(`服务器错误: ${err.message}`);
    server.close();
});
// 绑定端口
const PORT = 41234;
const HOST = '0.0.0.0';
server.bind(PORT, HOST);