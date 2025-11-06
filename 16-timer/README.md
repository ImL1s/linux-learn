# 定時器 (Timer)

## 📖 概念介紹

Linux 提供多種定時器機制，用於定期執行任務或實現超時控制。

## 🔧 API 說明

### 1. alarm() - 簡單定時器

```c
unsigned int alarm(unsigned int seconds);
```

- 在指定秒數後發送 SIGALRM 信號
- 只能設置一次
- 秒級精度

### 2. setitimer() - 週期定時器

```c
int setitimer(int which, const struct itimerval *new_value,
              struct itimerval *old_value);
```

**類型**：
- `ITIMER_REAL` - 實際時間（SIGALRM）
- `ITIMER_VIRTUAL` - 用戶態CPU時間（SIGVTALRM）
- `ITIMER_PROF` - 用戶態+內核態CPU時間（SIGPROF）

**精度**：微秒級

### 3. timer_create() - POSIX定時器

```c
int timer_create(clockid_t clockid, struct sigevent *sevp, timer_t *timerid);
int timer_settime(timer_t timerid, int flags, const struct itimerspec *new_value,
                  struct itimerspec *old_value);
```

- 更靈活、精度更高
- 支持納秒級
- 可創建多個定時器

## 📁 範例程式

1. **alarm_demo.c** - alarm簡單定時
2. **setitimer_demo.c** - 週期定時器
3. **timer_create_demo.c** - POSIX定時器（可選）

編譯運行：
```bash
make timer
./alarm_demo
./setitimer_demo
```

## 💡 對比總結

| 特性 | alarm | setitimer | timer_create |
|------|-------|-----------|--------------|
| **精度** | 秒 | 微秒 | 納秒 |
| **週期** | ❌ | ✅ | ✅ |
| **多實例** | ❌ | ❌ (3種) | ✅ 無限 |
| **複雜度** | 簡單 | 中等 | 複雜 |

## 📚 參考資料

- `man alarm`
- `man setitimer`  
- `man timer_create`
