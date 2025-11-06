# 內存映射 (Memory Mapping)

## 📖 概念介紹

**mmap** 將文件或設備映射到進程的虛擬地址空間，使文件訪問像訪問內存一樣簡單。

### 兩種模式

1. **文件映射**：將文件內容映射到內存
   - 讀寫內存即讀寫文件
   - 多進程可共享同一文件

2. **匿名映射**：分配內存區域
   - 不關聯文件（MAP_ANONYMOUS）
   - 可用於進程間共享內存

## 🔧 API 說明

```c
#include <sys/mman.h>

void *mmap(void *addr, size_t length, int prot, int flags, int fd, off_t offset);
int munmap(void *addr, size_t length);
int msync(void *addr, size_t length, int flags);
```

### 保護標誌 (prot)

- `PROT_READ` - 可讀
- `PROT_WRITE` - 可寫
- `PROT_EXEC` - 可執行

### 映射標誌 (flags)

- `MAP_SHARED` - 共享映射（修改會寫回文件）
- `MAP_PRIVATE` - 私有映射（寫時複製）
- `MAP_ANONYMOUS` - 匿名映射（無文件）

## 📁 範例程式

1. **mmap_file.c** - 文件映射示例
2. **mmap_shared.c** - 匿名共享映射（父子進程通訊）

編譯：
```bash
make mmap
./mmap_file
./mmap_shared
```

## 💡 mmap vs read/write

| 特性 | mmap | read/write |
|------|------|------------|
| **性能** | 大文件更快 | 小文件更快 |
| **使用** | 像訪問數組 | 需系統調用 |
| **內存** | 佔用虛擬地址空間 | 需緩衝區 |
| **複雜度** | 稍複雜 | 簡單 |

### 適用場景

✅ **使用 mmap**：
- 大文件隨機訪問
- 多進程共享數據
- 數據庫、大文件處理

❌ **使用 read/write**：
- 小文件順序讀寫
- 流式數據處理

## 📚 參考資料

- `man mmap`
- "The Linux Programming Interface" - Chapter 49
