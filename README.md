# Linux 系統程式設計學習專案

這是一個系統化的 Linux 系統程式設計學習專案，包含各種核心概念的範例程式和詳細說明。

## 📚 專案結構

```
linux-learn/
├── 01-process/          # 進程管理
├── 02-pipe/             # 管道通訊
├── 03-fifo/             # 命名管道
├── 04-signal/           # 信號處理
├── 05-file-io/          # 文件 I/O 操作
├── 06-thread/           # 多線程編程
├── 07-shared-memory/    # 共享內存
├── 08-socket/           # Socket 網路編程
├── 09-epoll/            # epoll I/O 多路復用
├── 10-daemon/           # 守護進程
└── 11-semaphore/        # 信號量
```

## 🎯 學習主題

### 1. 進程管理 (Process Management)
- **位置**: `01-process/`
- **內容**:
  - 進程創建 (fork)
  - 進程替換 (exec 家族)
  - 進程等待 (wait/waitpid)
  - 殭屍進程與孤兒進程

### 2. 管道通訊 (Pipe)
- **位置**: `02-pipe/`
- **內容**:
  - 匿名管道基礎
  - 父子進程通訊
  - 管道的讀寫操作

### 3. 命名管道 (FIFO)
- **位置**: `03-fifo/`
- **內容**:
  - FIFO 創建與使用
  - 無親緣關係進程通訊
  - 讀者與寫者程式

### 4. 信號處理 (Signal)
- **位置**: `04-signal/`
- **內容**:
  - 信號基礎概念
  - 自定義信號處理器
  - 信號發送與接收

### 5. 文件 I/O (File I/O)
- **位置**: `05-file-io/`
- **內容**:
  - 基本文件操作 (open/read/write/close)
  - 文件描述符
  - 文件鎖 (fcntl)

### 6. 多線程 (Thread)
- **位置**: `06-thread/`
- **內容**:
  - POSIX 線程基礎
  - 線程創建與同步
  - Mutex 與條件變量

### 7. 共享內存 (Shared Memory)
- **位置**: `07-shared-memory/`
- **內容**:
  - System V 共享內存
  - 共享內存的創建與映射
  - 進程間數據共享

### 8. Socket 網路編程 (Socket Programming)
- **位置**: `08-socket/`
- **內容**:
  - TCP 客戶端/服務器
  - socket/bind/listen/accept
  - 網絡字節序轉換
  - 多進程並發處理

### 9. epoll I/O 多路復用 (epoll)
- **位置**: `09-epoll/`
- **內容**:
  - epoll 高性能服務器
  - 邊緣觸發 vs 水平觸發
  - 非阻塞 I/O
  - 單線程處理高並發

### 10. 守護進程 (Daemon)
- **位置**: `10-daemon/`
- **內容**:
  - 守護進程標準創建流程
  - 會話和進程組
  - PID 文件鎖
  - syslog 日誌系統

### 11. 信號量 (Semaphore)
- **位置**: `11-semaphore/`
- **內容**:
  - POSIX 信號量
  - 生產者-消費者問題
  - 互斥與同步
  - P/V 操作

## 🔨 編譯與運行

### 編譯所有範例
```bash
make all
```

### 編譯特定模塊
```bash
make process   # 編譯進程管理範例
make pipe      # 編譯管道範例
make fifo      # 編譯 FIFO 範例
# ... 以此類推
```

### 清理編譯文件
```bash
make clean
```

### 運行範例
進入各個目錄，執行對應的可執行文件：
```bash
cd 01-process
./fork_basic
./exec_demo
```

## 📖 學習建議

1. **按順序學習**: 建議按照目錄編號順序學習，因為後面的主題會用到前面的知識
2. **閱讀代碼**: 每個範例都有詳細的中文註解，請仔細閱讀
3. **動手實踐**: 不要只看代碼，一定要自己編譯運行
4. **修改實驗**: 試著修改參數或邏輯，觀察不同的結果
5. **閱讀文檔**: 配合 `man` 命令查看系統調用的詳細說明

## 🛠️ 環境需求

- **作業系統**: Linux (建議 Ubuntu 18.04 或更新版本)
- **編譯器**: GCC 7.0 或更新版本
- **Make**: GNU Make
- **庫**: pthread 庫 (多線程範例需要)

## 📚 參考資料

- Linux Manual Pages (man pages)
- Advanced Programming in the UNIX Environment (APUE)
- The Linux Programming Interface (TLPI)

## 🤝 貢獻

歡迎提交 Issue 和 Pull Request 來改進這個學習專案！

## 📝 授權

本專案採用 MIT 授權條款。

---

**開始你的 Linux 系統程式設計之旅吧！** 🚀
