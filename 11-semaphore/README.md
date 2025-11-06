# 信號量 (Semaphore)

## 📖 簡介

信號量是用於進程/線程同步的經典機制，可以解決生產者-消費者、讀者-寫者等經典同步問題。

## 📁 範例文件

- `semaphore_demo.c` - 生產者-消費者問題完整實現

## 🔨 編譯運行

```bash
# 編譯（需要 pthread 庫）
make semaphore

# 或單獨編譯
gcc -o semaphore_demo semaphore_demo.c -pthread

# 運行
./semaphore_demo
```

## 💡 核心知識點

詳細的知識點和註解請查看源代碼：

- POSIX 信號量 API
- sem_init/sem_wait/sem_post
- 生產者-消費者問題
- 二元信號量 vs 計數信號量
- P/V 操作（wait/signal）
- 互斥與同步的區別
- 信號量 vs 互斥鎖
- 避免死鎖的策略
- System V 信號量對比

**📝 所有詳細說明都在源代碼的註解中！**
