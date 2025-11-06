# Socket 網路編程

## 📖 簡介

Socket 是網路編程的基礎，提供進程間的網絡通訊能力。本目錄包含完整的 TCP 客戶端/服務器實現。

## 📁 範例文件

- `tcp_server.c` - TCP 服務器（多進程並發模型）
- `tcp_client.c` - TCP 客戶端

## 🔨 編譯運行

```bash
# 編譯
make socket

# 或單獨編譯
gcc -o tcp_server tcp_server.c
gcc -o tcp_client tcp_client.c

# 運行服務器
./tcp_server 8888

# 另一個終端運行客戶端
./tcp_client localhost 8888

# 或使用 telnet 測試
telnet localhost 8888
```

## 💡 核心知識點

詳細的知識點和註解請查看源代碼：

- TCP socket 編程流程
- socket/bind/listen/accept/connect
- 網絡字節序轉換（htons/ntohs）
- SO_REUSEADDR 選項
- 多進程並發處理
- 信號處理（SIGCHLD）
- 錯誤處理

**📝 所有詳細說明都在源代碼的註解中！**
