# epoll I/O 多路復用

## 📖 簡介

epoll 是 Linux 下高性能的 I/O 多路復用機制，適合處理大量並發連接。單線程即可處理數萬個並發連接。

## 📁 範例文件

- `epoll_server.c` - 高性能 epoll 服務器

## 🔨 編譯運行

```bash
# 編譯
make epoll

# 或單獨編譯
gcc -o epoll_server epoll_server.c

# 運行
./epoll_server 9999

# 測試（多個終端）
telnet localhost 9999
```

## 💡 核心知識點

詳細的知識點和註解請查看源代碼：

- epoll_create1/epoll_ctl/epoll_wait
- 邊緣觸發（ET）vs 水平觸發（LT）
- 非阻塞 I/O（O_NONBLOCK）
- epoll vs select/poll 性能對比
- 事件類型（EPOLLIN/EPOLLOUT/EPOLLET）
- 高並發服務器架構

**📝 所有詳細說明都在源代碼的註解中！**
