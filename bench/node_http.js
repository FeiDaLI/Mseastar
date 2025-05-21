const http = require('http');
const cluster = require('cluster');
const numCPUs = require('os').cpus().length; // 8 核

if (cluster.isMaster) {
    console.log(`Master ${process.pid} is running`);
    for (let i = 0; i < numCPUs; i++) {
        cluster.fork();
    }
} else {
    const server = http.createServer((req, res) => {
        if (req.url === '/api' && req.method === 'GET') {
            res.writeHead(200, { 'Content-Type': 'text/plain' });
            res.end('Hello from /api\n');
        } else {
            res.writeHead(404, { 'Content-Type': 'text/plain' });
            res.end('Not Found\n');
        }
    });

    server.maxConnections = 10000; // 增加最大连接数
    const host = '0.0.0.0';
    const port = 8080;

    server.listen(port, host, () => {
        console.log(`Worker ${process.pid} running at http://${host}:${port}/`);
    });
}