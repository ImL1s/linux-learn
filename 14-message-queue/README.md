# 消息隊列 (Message Queue)

## 📖 概念介紹

System V 消息隊列是一種進程間通訊 (IPC) 機制，允許進程通過消息進行異步通訊。

### 核心特性

- **異步通訊**：發送者和接收者不需要同時運行
- **消息類型**：支持消息優先級和分類
- **持久性**：消息隊列在進程退出後仍然存在
- **有序性**：FIFO 或按類型接收

## 🔧 API 說明

```c
#include <sys/msg.h>

int msgget(key_t key, int msgflg);           // 創建/獲取隊列
int msgsnd(int msqid, const void *msgp, size_t msgsz, int msgflg);
ssize_t msgrcv(int msqid, void *msgp, size_t msgsz, long msgtyp, int msgflg);
int msgctl(int msqid, int cmd, struct msqid_ds *buf);
```

### 消息結構

```c
struct msg_buffer {
    long msg_type;        // 消息類型 (必須 > 0)
    char msg_text[512];   // 消息內容
};
```

## 📁 範例程式

1. **msg_sender.c** - 消息發送端
2. **msg_receiver.c** - 消息接收端

編譯運行：
```bash
make message-queue
# 終端1
./msg_receiver
# 終端2  
./msg_sender
```

## 💡 IPC 機制對比

| IPC | 優點 | 缺點 | 適用場景 |
|-----|------|------|----------|
| **Pipe** | 簡單 | 只能親緣進程 | 父子進程 |
| **FIFO** | 無親緣限制 | 無結構 | 簡單通訊 |
| **消息隊列** | 有結構、異步 | 需手動清理 | 多進程協作 |
| **共享內存** | 最快 | 需同步機制 | 大量數據 |

## ⚠️ 常見問題

### Q: 如何清理消息隊列？

```bash
# 查看
ipcs -q
# 刪除
ipcrm -q <msqid>
```

### Q: 消息類型有什麼用？

- 實現優先級隊列
- 多個接收者接收不同類型
- 消息分類過濾

## 📚 參考資料

- `man msgget`
- "UNIX Network Programming Vol.2" - Chapter 6
