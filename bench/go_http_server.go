package main

import (
    "fmt"
    "net/http"
)

// apiHandler 处理 /api 请求，返回 200 OK
func apiHandler(w http.ResponseWriter, r *http.Request) {
    // 只允许 GET 方法
    if r.Method != http.MethodGet {
       	http.Error(w, "Method not allowed", http.StatusMethodNotAllowed)
        return
    }

    w.WriteHeader(http.StatusOK)
    fmt.Fprintf(w, "OK\n")
}

func main() {
    // 设置路由
    http.HandleFunc("/api", apiHandler)

    // 启动 HTTP 服务器
    addr := "0.0.0.0:8080"
    fmt.Printf("Server is listening on %s...\n", addr)
    
    err := http.ListenAndServe(addr, nil)
    if err != nil {
        panic(err)
    }
}