# 讀寫鎖 (Read-Write Lock)

## 📖 概念介紹

**讀寫鎖 (Read-Write Lock)** 是一種允許多個讀者同時訪問共享資源，但只允許一個寫者獨占訪問的同步機制。它是對互斥鎖的優化，適用於讀多寫少的場景。

### 核心特性

- **多讀並發**：多個讀者可以同時持有讀鎖
- **單寫獨占**：只有一個寫者可以持有寫鎖
- **讀寫互斥**：讀鎖和寫鎖互斥
- **寫寫互斥**：多個寫者之間互斥

### 適用場景

✅ 讀操作遠多於寫操作
✅ 數據結構較大，讀取耗時較長
✅ 緩存系統、配置管理
✅ 數據庫索引、路由表

❌ 寫操作頻繁（不如mutex）
❌ 臨界區非常短（開銷不值得）

## 🔧 API 說明

### 基本操作

```c
pthread_rwlock_t rwlock = PTHREAD_RWLOCK_INITIALIZER;

pthread_rwlock_rdlock(&rwlock);   // 讀鎖定
pthread_rwlock_wrlock(&rwlock);   // 寫鎖定
pthread_rwlock_unlock(&rwlock);   // 解鎖
pthread_rwlock_destroy(&rwlock);  // 銷毀
```

### vs Mutex

| 特性 | Mutex | Read-Write Lock |
|------|-------|-----------------|
| 並發讀 | ❌ 不支持 | ✅ 支持 |
| 性能 | 讀少時更好 | 讀多時更好 |
| 開銷 | 小 | 略大 |
| 複雜度 | 簡單 | 稍複雜 |

## 📁 範例程式

1. **rwlock_basic.c** - 基礎演示
2. **rwlock_demo.c** - 多讀者單寫者
3. **rwlock_vs_mutex.c** - 性能對比

編譯運行：
```bash
make rwlock
cd 13-rwlock
./rwlock_basic
```

## 💡 重要知識點

### 讀者優先 vs 寫者優先

**讀者優先**：只要有讀者，寫者就要等待
- 優點：讀吞吐量高
- 缺點：可能餓死寫者

**寫者優先**：有寫者等待時，新的讀者要等待
- 優點：避免寫者飢餓
- 缺點：讀吞吐量下降

POSIX默認實現依賴於系統。

## 📚 參考資料

- `man pthread_rwlock_rdlock`
- "The Linux Programming Interface" - Chapter 30
