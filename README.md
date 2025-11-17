# Linux 系統程式設計學習專案

這是一個系統化的 Linux 系統程式設計學習專案，從基礎到進階，涵蓋所有核心概念，包含詳細的中文註解和完整的範例程式。

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
├── 08-socket/           # TCP Socket 網路編程
├── 09-epoll/            # epoll I/O 多路復用
├── 10-daemon/           # 守護進程
├── 11-semaphore/        # 信號量
├── 12-condition-var/    # 條件變量 🆕
├── 13-rwlock/           # 讀寫鎖 🆕
├── 14-message-queue/    # 消息隊列 🆕
├── 15-mmap/             # 內存映射 🆕
├── 16-timer/            # 定時器 🆕
├── 17-select-poll/      # select/poll 🆕
├── 18-udp-socket/       # UDP Socket 🆕
├── 19-ipc-benchmark/    # IPC 性能對比工具 ⭐
├── 20-memory-safety/    # 內存安全基礎 (教育目的) 🔒
├── 21-file-security/    # 文件權限與訪問控制 (教育目的) 🔒
└── utils/               # 實用工具庫 ⭐
```

## 🎯 學習路徑

### 階段 1：基礎 (01-05)

#### 1. 進程管理 (Process Management)
- **位置**: `01-process/`
- **內容**:
  - 進程創建 (fork)
  - 進程替換 (exec 家族)
  - 進程等待 (wait/waitpid)
  - 殭屍進程與孤兒進程
- **示例**: 3個完整程式

#### 2. 管道通訊 (Pipe)
- **位置**: `02-pipe/`
- **內容**:
  - 匿名管道基礎
  - 父子進程通訊
  - 管道的讀寫操作
- **示例**: 1個程式

#### 3. 命名管道 (FIFO)
- **位置**: `03-fifo/`
- **內容**:
  - FIFO 創建與使用
  - 無親緣關係進程通訊
  - 讀者與寫者程式
- **示例**: 2個程式

#### 4. 信號處理 (Signal)
- **位置**: `04-signal/`
- **內容**:
  - 信號基礎概念
  - 自定義信號處理器
  - 信號發送與接收
- **示例**: 1個程式
- **README**: 316行詳細文檔

#### 5. 文件 I/O (File I/O)
- **位置**: `05-file-io/`
- **內容**:
  - 基本文件操作 (open/read/write/close)
  - 文件描述符
  - 文件鎖 (fcntl)
  - lseek 文件定位
- **示例**: 1個程式
- **README**: 371行詳細文檔

### 階段 2：並發與同步 (06-13)

#### 6. 多線程 (Thread)
- **位置**: `06-thread/`
- **內容**:
  - POSIX 線程基礎
  - 線程創建與同步
  - Mutex 互斥鎖
  - 競爭條件演示
- **示例**: 1個程式（含3個內部demo）
- **README**: 438行詳細文檔

#### 11. 信號量 (Semaphore)
- **位置**: `11-semaphore/`
- **內容**:
  - POSIX 信號量
  - 生產者-消費者問題
  - 互斥與同步
  - P/V 操作
- **示例**: 1個程式（454行）

#### 12. 條件變量 (Condition Variable) 🆕
- **位置**: `12-condition-var/`
- **內容**:
  - pthread_cond_wait/signal/broadcast
  - 虛假喚醒問題
  - 生產者消費者（條件變量實現）
  - 與信號量的性能對比
- **示例**: 3個程式
- **README**: 600+行詳細文檔

#### 13. 讀寫鎖 (Read-Write Lock) 🆕
- **位置**: `13-rwlock/`
- **內容**:
  - pthread_rwlock 讀寫鎖
  - 多讀者單寫者模型
  - 讀者優先 vs 寫者優先
- **示例**: 2個程式

### 階段 3：進程間通訊 (07, 14-15)

#### 7. 共享內存 (Shared Memory)
- **位置**: `07-shared-memory/`
- **內容**:
  - System V 共享內存
  - 共享內存的創建與映射
  - 進程間數據共享
- **示例**: 2個程式
- **README**: 418行詳細文檔

#### 14. 消息隊列 (Message Queue) 🆕
- **位置**: `14-message-queue/`
- **內容**:
  - System V 消息隊列
  - msgget/msgsnd/msgrcv
  - 消息類型和優先級
  - IPC 機制完整對比
- **示例**: 2個程式

#### 15. 內存映射 (Memory Mapping) 🆕
- **位置**: `15-mmap/`
- **內容**:
  - mmap 文件映射
  - 匿名共享映射
  - MAP_SHARED vs MAP_PRIVATE
  - mmap vs read/write 性能對比
- **示例**: 2個程式

### 階段 4：網路編程 (08-09, 17-18)

#### 17. select/poll (I/O Multiplexing) 🆕
- **位置**: `17-select-poll/`
- **內容**:
  - select 實現的服務器
  - poll 實現的服務器
  - select vs poll vs epoll 對比
  - 水平觸發 vs 邊緣觸發
- **示例**: 2個程式

#### 8. TCP Socket 編程
- **位置**: `08-socket/`
- **內容**:
  - TCP 客戶端/服務器
  - socket/bind/listen/accept
  - 網絡字節序轉換
  - 多進程並發處理
- **示例**: 2個程式

#### 18. UDP Socket 編程 🆕
- **位置**: `18-udp-socket/`
- **內容**:
  - UDP 服務器/客戶端
  - sendto/recvfrom
  - TCP vs UDP 詳細對比
  - 數據報邊界
- **示例**: 2個程式

#### 9. epoll (高性能 I/O)
- **位置**: `09-epoll/`
- **內容**:
  - epoll 高性能服務器
  - 邊緣觸發（ET）vs 水平觸發（LT）
  - 非阻塞 I/O
  - 單線程處理高並發
- **示例**: 1個程式

### 階段 5：系統級編程 (10, 16)

#### 10. 守護進程 (Daemon)
- **位置**: `10-daemon/`
- **內容**:
  - 守護進程標準創建流程
  - 會話和進程組
  - PID 文件鎖
  - syslog 日誌系統
  - 兩次 fork() 深入解釋
- **示例**: 1個程式（480行）

#### 16. 定時器 (Timer) 🆕
- **位置**: `16-timer/`
- **內容**:
  - alarm 簡單定時器
  - setitimer 週期定時器
  - timer_create POSIX定時器
  - 三種定時器對比
- **示例**: 2個程式

### 階段 6：增強範例與工具 ⭐

#### 19. IPC 性能對比工具 ⭐
- **位置**: `19-ipc-benchmark/`
- **內容**:
  - 5種IPC機制性能測試（pipe, FIFO, 共享內存, 消息隊列, Unix Socket）
  - 吞吐量測試（MB/s）
  - 延遲測試（微秒級）
  - 命令行參數支持（數據大小、塊大小、測試類型）
  - 詳細性能分析和選擇建議
- **示例**: 1個工具（900+行）
- **README**: 600+行完整文檔

#### 實用工具庫 (Utils) ⭐
- **位置**: `utils/`
- **內容**:
  - **config_parser.h** - INI 格式配置文件解析器（Header-only）
    - 支持 Section/Key/Value 結構
    - 類型轉換（string/int/bool）
    - 默認值支持
    - 易於集成到項目中
  - **config_demo.c** - 配置解析器使用示範
- **示例**: 1個演示程式
- **README**: 500+行 API 文檔

#### 增強版範例 ⭐
以下範例在原有基礎上增加了實用性和靈活性：

**tcp_echo_advanced.c** (08-socket/)
- 命令行參數支持（端口、模式、最大連接數）
- 多工作模式（fork/thread）
- 統計功能（連接數、數據量、運行時間）
- 日誌級別控制
- 660行完整實現

**thread_pool.c** (06-thread/)
- 可配置線程數量
- 任務隊列管理
- 優雅關閉機制
- 統計信息輸出
- 可直接用於實際項目
- 600行專業實現

**http_server_simple.c** (09-epoll/)
- 簡單的HTTP/1.1服務器
- 靜態文件服務
- 12種MIME類型支持
- URL解碼和安全路徑檢查
- 目錄瀏覽功能
- 600行實戰示範

### 階段 7：系統安全基礎 🔒

#### 20. 內存安全基礎 (Memory Safety) 🔒
- **位置**: `20-memory-safety/`
- **⚠️ 重要**: 僅供教育和研究目的使用
- **內容**:
  - Stack 和 Heap 內存布局
  - 緩衝區溢出原理與演示（受控環境）
  - Stack Canary 防護機制
  - ASLR, NX/DEP, PIE 防禦技術
  - 安全的字符串操作
  - CVE 案例研究
- **示例**: 4個教育程式
  - `stack_layout` - 棧結構展示
  - `buffer_overflow_demo` - 緩衝區溢出演示（教育）
  - `canary_demo` - Stack canary 檢測
  - `safe_string` - 安全字符串庫
- **README**: 詳盡的安全教育文檔

#### 21. 文件權限與訪問控制 (File Security) 🔒
- **位置**: `21-file-security/`
- **⚠️ 重要**: 僅供教育和研究目的使用
- **內容**:
  - Linux 權限模型（rwx、owner/group/other）
  - 特殊權限位（SUID/SGID/Sticky Bit）
  - Linux Capabilities 機制
  - 權限提升原理（教育）
  - 安全的臨時文件處理
  - 防止符號鏈接攻擊
- **示例**: 3個教育程式
  - `permission_demo` - 權限檢查和展示
  - `suid_example` - SUID 程式演示（教育）
  - `secure_tempfile` - 安全臨時文件處理
- **README**: CVE案例、最佳實踐

**道德聲明**:
- ✅ 適用於: CTF 競賽、安全研究、授權滲透測試、防禦性安全
- ❌ 禁止用於: 未經授權的系統、惡意攻擊、破壞性行為
- 📚 完整規劃: 見 `SECURITY_PROJECT_PLAN.md`

**安全學習路徑** (進行中 - 2/13 主題完成):
- ✅ 階段1 (部分): 內存安全、文件安全 (已完成)
- 🚧 階段1 (待續): 進程安全 (計劃中)
- 🚧 階段2: 格式化字符串、競爭條件、網絡安全、加密 (計劃中)
- 🚧 階段3: ROP、內核安全、Web安全、逆向工程、滲透測試 (計劃中)
- 📚 詳細規劃見 `SECURITY_PROJECT_PLAN.md`

## 🔨 編譯與運行

### 編譯所有範例

```bash
make all
```

### 編譯特定模塊

```bash
make process        # 編譯進程管理
make thread         # 編譯多線程（包含 thread_pool）
make socket         # 編譯 Socket（包含 tcp_echo_advanced）
make epoll          # 編譯 epoll（包含 http_server_simple）
make condition-var  # 編譯條件變量
make select-poll    # 編譯 select/poll
make udp-socket     # 編譯 UDP socket
make ipc-benchmark  # 編譯 IPC 性能對比工具 ⭐
make utils          # 編譯工具庫範例 ⭐
make memory-safety  # 編譯內存安全範例 (教育目的) 🔒
make file-security  # 編譯文件安全範例 (教育目的) 🔒
# ... 以此類推
```

### 清理編譯文件

```bash
make clean
```

### 運行範例

進入各個目錄，執行對應的可執行文件：

```bash
# 基礎範例
cd 12-condition-var
./cond_basic
./producer_consumer

cd ../17-select-poll
./select_server  # 終端1
./poll_server    # 或終端1
telnet localhost 9000  # 終端2

# 增強版範例 ⭐
cd ../08-socket
./tcp_echo_advanced --port 9999 --mode thread --max-conn 100 --verbose

cd ../06-thread
./thread_pool  # 線程池演示

cd ../09-epoll
./http_server_simple --port 8080 --root ./www

# IPC 性能對比工具 ⭐
cd ../19-ipc-benchmark
./ipc_benchmark                    # 測試所有 IPC 機制（默認 1MB）
./ipc_benchmark --size 10M         # 測試 10MB 數據
./ipc_benchmark pipe shm socket    # 只測試特定機制

# 配置解析器 ⭐
cd ../utils
./config_demo                      # 使用示範
./config_demo my_config.ini        # 自定義配置文件
```

## 📊 專案統計

- **主題數量**: 21個（11個基礎 + 7個進階 + 工具 + 2個安全）
  - 基礎模塊: ✅ 100% 完成 (01-19)
  - 安全模塊: 🚧 15% 完成 (2/13 主題，計劃中)
- **源文件**: 44個 C 程式 + 1個頭文件
  - 基礎範例: 34個
  - 增強範例: 3個（thread_pool, tcp_echo_advanced, http_server_simple）
  - 工具程式: 2個（ipc_benchmark, config_demo）
  - 安全教育: 7個
    - 內存安全: 4個（stack_layout, buffer_overflow_demo, canary_demo, safe_string）
    - 文件安全: 3個（permission_demo, suid_example, secure_tempfile）
- **代碼行數**: 11,439 行（C + H，含詳細註解）
- **文檔行數**: 11,649 行（23個 README）
- **編譯質量**: ✅ 100% 成功 (0 錯誤, 0 警告)
- **README平均**: 500+ 行/個
- **最大單文件**: 900+ 行（ipc_benchmark.c）

## ✨ 專案特色

### 1. 完整的學習路徑

- ✅ 由淺入深：從進程到網路編程
- ✅ 循序漸進：每個階段建立在前一階段之上
- ✅ 實戰導向：所有示例都可直接運行

### 2. 詳盡的中文註解

- ✅ 每個概念都有詳細解釋
- ✅ 60% 註解密度
- ✅ 包含「為什麼」不只是「怎麼做」

### 3. 完整的文檔

- ✅ 每個主題都有詳細 README
- ✅ API 說明、使用示例、常見問題
- ✅ 性能對比、適用場景分析

### 4. 對比性學習

- ✅ 條件變量 vs 信號量
- ✅ select vs poll vs epoll
- ✅ TCP vs UDP
- ✅ mmap vs read/write
- ✅ 各種 IPC 機制對比

### 5. 實用性增強 ⭐

- ✅ **IPC 性能對比工具** - 幫助選擇最適合的 IPC 機制
- ✅ **配置文件解析器** - 可直接用於實際項目的 header-only 庫
- ✅ **線程池實現** - 專業級線程池，可重用組件
- ✅ **增強版網絡服務器** - 支持命令行參數和統計功能
- ✅ **簡易 HTTP 服務器** - 實戰示範，組合多種技術

### 6. 靈活可配置 ⭐

- ✅ 支持命令行參數（端口、模式、大小等）
- ✅ 配置文件支持（INI 格式）
- ✅ 統計信息輸出（性能監控）
- ✅ 多種工作模式（fork/thread/epoll）

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

## 🎓 知識點覆蓋

### 進程管理
- fork, exec, wait, 殭屍/孤兒進程

### 進程間通訊 (IPC)
- pipe, FIFO, 共享內存, 消息隊列, mmap

### 線程同步
- mutex, 信號量, 條件變量, 讀寫鎖

### 信號
- signal, kill, alarm, setitimer

### 文件I/O
- open, read, write, lseek, fcntl, mmap

### 網路編程
- TCP/UDP socket, select/poll/epoll

### 系統編程
- daemon, timer, syslog

## 📚 參考資料

- **Linux Manual Pages** (man pages)
- **Advanced Programming in the UNIX Environment** (APUE)
- **The Linux Programming Interface** (TLPI)
- **UNIX Network Programming** (UNP Vol.1 & Vol.2)

## 🤝 貢獻

歡迎提交 Issue 和 Pull Request 來改進這個學習專案！

## 📝 授權

本專案採用 MIT 授權條款。

---

**開始你的 Linux 系統程式設計之旅吧！** 🚀

從最簡單的 `01-process/fork_basic.c` 開始，一步步深入到高級的並發網路編程。
