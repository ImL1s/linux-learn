# 20. 內存安全基礎 (Memory Safety Basics)

## ⚠️ 教育聲明

本章節內容**僅供教育和研究目的**使用。所有範例應在隔離的測試環境中運行，不得用於未經授權的系統。

---

## 📚 目錄

1. [內存布局基礎](#內存布局基礎)
2. [緩衝區溢出原理](#緩衝區溢出原理)
3. [防禦機制](#防禦機制)
4. [安全編程實踐](#安全編程實踐)
5. [範例程式說明](#範例程式說明)
6. [實驗環境設置](#實驗環境設置)
7. [CVE 案例研究](#cve-案例研究)

---

## 1. 內存布局基礎

### 1.1 進程內存空間

Linux 進程的典型內存布局（從高地址到低地址）：

```
高地址
+------------------+
|   內核空間        |  0xFFFFFFFF (32位) / 0x7FFFFFFFFFFF (64位)
+==================+
|   棧 (Stack)     |  ← ESP/RSP 棧指針
|       ↓          |     向下增長
+------------------+
|                  |
|   未使用空間      |
|                  |
+------------------+
|       ↑          |
|   堆 (Heap)      |     向上增長
+------------------+
|   BSS 段         |  未初始化全局變量
+------------------+
|   Data 段        |  初始化全局變量
+------------------+
|   Text 段        |  程式碼（只讀）
+------------------+  0x08048000 (典型起始地址)
低地址
```

### 1.2 棧幀 (Stack Frame) 結構

函數調用時的棧幀布局：

```
高地址
+------------------+
|  參數 n          |
|  參數 2          |
|  參數 1          |
+------------------+
|  返回地址         |  ← 函數返回後要執行的指令地址
+==================+  ← EBP/RBP (舊的基址指針)
|  舊 EBP          |  保存調用者的 EBP
+------------------+
|  局部變量 1       |
|  局部變量 2       |
|  局部變量 n       |
+------------------+  ← ESP/RSP (當前棧指針)
|                  |  ← 棧向下增長
低地址
```

**關鍵點**：
- **返回地址**：函數返回時跳轉的地址
- **舊 EBP**：恢復調用者的棧幀
- **局部變量**：從高地址向低地址分配

---

## 2. 緩衝區溢出原理

### 2.1 什麼是緩衝區溢出？

當程式向緩衝區寫入的數據**超過其分配的大小**時，就會發生緩衝區溢出。這會覆蓋相鄰的內存區域。

### 2.2 棧溢出 (Stack Overflow)

**原理**：
```c
void vulnerable_function(char *input) {
    char buffer[64];      // 64 字節緩衝區
    strcpy(buffer, input); // 沒有邊界檢查！
}
```

**攻擊過程**：
```
正常情況：
+------------------+
|  返回地址: 0x400  |
+------------------+
|  舊 EBP           |
+------------------+
|  buffer[63]      |
|  ...             |
|  buffer[0]       |
+------------------+

溢出情況：
+------------------+
|  0x41414141      |  ← 返回地址被覆蓋！
+------------------+
|  0x41414141      |  ← 舊 EBP 被覆蓋
+------------------+
|  AAAAA...AAAA    |  ← 緩衝區溢出
+------------------+
```

### 2.3 堆溢出 (Heap Overflow)

堆溢出更複雜，可能覆蓋：
- 堆管理元數據（chunk headers）
- 其他堆對象的數據
- 函數指針

### 2.4 利用技術（教育目的）

**控制返回地址**：
```
原返回地址：0x08048abc → main() + 15
修改後：    0xbffff890 → shellcode 起始地址
```

**Shellcode**：
- 小段機器碼，執行特定操作
- 通常啟動 shell (/bin/sh)
- 需要避免 NULL 字節

---

## 3. 防禦機制

### 3.1 Stack Canary (金絲雀值)

**原理**：
在返回地址前放置一個隨機值（canary），函數返回前檢查這個值是否被修改。

```
+------------------+
|  返回地址         |
+------------------+
|  Stack Canary    |  ← 隨機值，如 0xdeadbeef
+==================+
|  舊 EBP           |
+------------------+
|  局部變量         |
+------------------+
```

**檢查代碼**：
```c
if (canary != expected_canary) {
    // 檢測到棧溢出！
    __stack_chk_fail();
}
```

**編譯選項**：
- `-fstack-protector`：保護有緩衝區的函數
- `-fstack-protector-all`：保護所有函數
- `-fno-stack-protector`：禁用（僅用於教學）

### 3.2 ASLR (Address Space Layout Randomization)

**原理**：
每次程式運行時，隨機化內存區域的基址：
- Stack 基址
- Heap 基址
- 共享庫基址
- 可執行文件基址（需要 PIE）

**查看狀態**：
```bash
cat /proc/sys/kernel/randomize_va_space
# 0 = 禁用
# 1 = 部分隨機化（棧、堆、庫）
# 2 = 完全隨機化（包括 VDSO）
```

**效果**：
```
運行 1：Stack at 0xbffff000
運行 2：Stack at 0xbf8a7000  ← 不同地址
運行 3：Stack at 0xbfcde000
```

### 3.3 NX/DEP (Non-Executable Stack)

**原理**：
將棧和堆標記為**不可執行**（NX = No eXecute）。即使注入了 shellcode，也無法執行。

**CPU 支持**：
- Intel: XD (eXecute Disable)
- AMD: NX (No eXecute)

**編譯選項**：
- `-z execstack`：允許棧執行（不安全，僅教學）
- `-z noexecstack`：禁止棧執行（默認）

**繞過技術**（教育）：
- **Return-to-libc**：不執行棧上代碼，而是跳轉到已存在的庫函數
- **ROP (Return-Oriented Programming)**：鏈接多個代碼片段（gadgets）

### 3.4 PIE (Position Independent Executable)

**原理**：
使可執行文件本身的代碼段地址也隨機化。

**編譯選項**：
- `-pie -fPIE`：啟用
- `-no-pie`：禁用（教學用）

### 3.5 RELRO (RELocation Read-Only)

**原理**：
使某些內存區域（如 GOT）在加載後變為只讀。

**選項**：
- `Partial RELRO`：部分保護
- `Full RELRO` (`-Wl,-z,relro,-z,now`)：完全保護

**檢查**：
```bash
checksec --file=./program
```

---

## 4. 安全編程實踐

### 4.1 危險函數與替代方案

| 危險函數 | 問題 | 安全替代 |
|---------|------|---------|
| `strcpy()` | 無邊界檢查 | `strncpy()`, `strlcpy()` |
| `strcat()` | 無邊界檢查 | `strncat()`, `strlcat()` |
| `gets()` | 無法限制長度 | `fgets()` |
| `sprintf()` | 無邊界檢查 | `snprintf()` |
| `scanf("%s")` | 無邊界檢查 | `scanf("%100s")` |

### 4.2 安全函數使用

**fgets() - 安全讀取**：
```c
char buffer[128];
if (fgets(buffer, sizeof(buffer), stdin) != NULL) {
    // 移除換行符
    buffer[strcspn(buffer, "\n")] = '\0';
}
```

**snprintf() - 安全格式化**：
```c
char buffer[128];
int len = snprintf(buffer, sizeof(buffer), "User: %s", username);
if (len >= sizeof(buffer)) {
    // 輸出被截斷
}
```

**strncpy() - 注意事項**：
```c
char dest[64];
strncpy(dest, src, sizeof(dest) - 1);
dest[sizeof(dest) - 1] = '\0';  // 確保 NULL 終止
```

### 4.3 邊界檢查

**總是檢查長度**：
```c
// 錯誤示例
void bad_copy(char *dest, char *src) {
    while (*src) {
        *dest++ = *src++;  // 可能溢出！
    }
}

// 正確示例
void safe_copy(char *dest, char *src, size_t dest_size) {
    size_t i;
    for (i = 0; i < dest_size - 1 && src[i] != '\0'; i++) {
        dest[i] = src[i];
    }
    dest[i] = '\0';  // NULL 終止
}
```

### 4.4 輸入驗證

```c
int safe_read_int(void) {
    char buffer[32];
    long value;
    char *endptr;

    if (fgets(buffer, sizeof(buffer), stdin) == NULL) {
        return -1;
    }

    value = strtol(buffer, &endptr, 10);

    if (endptr == buffer || *endptr != '\n') {
        // 無效輸入
        return -1;
    }

    if (value < INT_MIN || value > INT_MAX) {
        // 超出範圍
        return -1;
    }

    return (int)value;
}
```

---

## 5. 範例程式說明

### 5.1 stack_layout.c
**目的**：展示棧結構和變量地址

**功能**：
- 顯示各類變量的地址
- 展示函數調用時的棧變化
- 計算棧增長方向

**編譯**：
```bash
gcc -o stack_layout stack_layout.c
./stack_layout
```

**輸出示例**：
```
=== 棧布局演示 ===
局部變量 1: 0xbffff7c0
局部變量 2: 0xbffff7bc
參數 1: 0xbffff7e8
返回地址: 0xbffff7e4
棧向下增長
```

### 5.2 buffer_overflow_demo.c
**目的**：演示緩衝區溢出的原理和影響

**功能**：
- 安全版本：有邊界檢查
- 危險版本：使用 strcpy() 無檢查
- 演示返回地址覆蓋

**⚠️ 警告**：此程式故意包含漏洞，僅供教學使用！

**編譯（禁用保護）**：
```bash
# 禁用所有保護（僅用於學習）
gcc -fno-stack-protector -z execstack -no-pie -o buffer_overflow_demo buffer_overflow_demo.c

# 禁用 ASLR（需要 root）
echo 0 | sudo tee /proc/sys/kernel/randomize_va_space
```

**恢復保護**：
```bash
echo 2 | sudo tee /proc/sys/kernel/randomize_va_space
```

### 5.3 canary_demo.c
**目的**：展示 Stack Canary 防護機制

**功能**：
- 手動實現 canary 檢查
- 檢測緩衝區溢出
- 展示 GCC canary 的行為

**編譯對比**：
```bash
# 無保護
gcc -fno-stack-protector -o canary_demo_unsafe canary_demo.c

# 有保護
gcc -fstack-protector-all -o canary_demo_safe canary_demo.c
```

**測試**：
```bash
# 會觸發 canary 檢測
echo "AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA" | ./canary_demo_safe
```

### 5.4 safe_string.c
**目的**：展示安全的字符串操作

**功能**：
- 安全函數庫實現
- 邊界檢查示例
- 性能對比

**編譯**：
```bash
gcc -O2 -o safe_string safe_string.c
./safe_string
```

---

## 6. 實驗環境設置

### 6.1 推薦環境

**虛擬機（推薦）**：
```bash
# 使用 Ubuntu 20.04 或更新版本
# 下載：https://ubuntu.com/download/desktop

# 或使用 Docker
docker run -it --rm ubuntu:22.04 /bin/bash
```

### 6.2 安裝工具

```bash
# 編譯工具
sudo apt-get update
sudo apt-get install build-essential

# 調試和分析工具
sudo apt-get install gdb radare2 objdump

# 檢查保護機制
sudo apt-get install checksec

# Python exploit 開發工具（可選）
pip3 install pwntools
```

### 6.3 GDB 使用

**基本命令**：
```bash
gdb ./program

# 常用命令
(gdb) run                    # 運行程式
(gdb) break main             # 在 main 設斷點
(gdb) info registers         # 查看寄存器
(gdb) x/20x $esp             # 查看棧內容
(gdb) disassemble main       # 反匯編
(gdb) backtrace              # 查看調用棧
```

**查看棧**：
```bash
(gdb) x/32xw $esp
(gdb) x/32xg $rsp  # 64位
```

### 6.4 檢查保護機制

```bash
checksec --file=./program
# 輸出示例：
# RELRO           STACK CANARY      NX            PIE
# Partial RELRO   Canary found      NX enabled    No PIE
```

---

## 7. CVE 案例研究

### 7.1 CVE-2019-14287 (sudo 漏洞)

**描述**：
sudo 的 RunAs 功能中存在緩衝區溢出漏洞。

**影響**：
允許用戶以 root 權限執行命令。

**原理**：
不當的用戶 ID 驗證導致整數溢出。

**修復**：
更新到 sudo 1.8.28 或更高版本。

### 7.2 CVE-2014-0160 (Heartbleed)

**描述**：
OpenSSL TLS heartbeat 擴展中的緩衝區溢出。

**影響**：
洩露服務器內存中的敏感數據（密鑰、密碼）。

**原理**：
```c
// 漏洞代碼簡化
payload_length = incoming_packet->length;  // 未驗證
memcpy(response, memory, payload_length);   // 可能超出範圍
```

**教訓**：
- 始終驗證輸入長度
- 使用安全的內存操作函數
- 代碼審計的重要性

### 7.3 CVE-2021-3156 (Baron Samedit - sudo 堆溢出)

**描述**：
sudo 中的堆溢出漏洞。

**影響**：
本地權限提升到 root。

**原理**：
在 `-s` 選項處理中存在堆溢出。

**利用**：
```bash
sudoedit -s '\' $(python3 -c 'print("A"*1000)')
```

---

## 8. 練習題

### 8.1 基礎題

1. 解釋棧幀中返回地址的作用
2. 為什麼緩衝區溢出會導致安全問題？
3. Stack Canary 如何防止緩衝區溢出？

### 8.2 實踐題

1. 修改 `buffer_overflow_demo.c` 來覆蓋返回地址
2. 使用 GDB 觀察棧溢出過程
3. 編寫一個安全的字符串複製函數

### 8.3 高級題

1. 在啟用 ASLR 的情況下，如何利用緩衝區溢出？
2. 解釋 ROP 攻擊的原理
3. 研究一個真實的 CVE 並分析其利用過程

---

## 9. 參考資料

### 書籍
- **Hacking: The Art of Exploitation** (Jon Erickson)
- **The Shellcoder's Handbook** (3rd Edition)
- **A Guide to Kernel Exploitation** (Enrico Perla)

### 在線資源
- **OWASP**: https://owasp.org/
- **CWE-120**: Buffer Copy without Checking Size of Input
- **Exploit Database**: https://www.exploit-db.com/
- **CTF Wiki**: https://ctf-wiki.org/

### 工具
- **GDB**: GNU Debugger
- **Pwntools**: https://github.com/Gallopsled/pwntools
- **Radare2**: https://rada.re/

---

## 10. 總結

### 關鍵要點

1. **理解內存布局**是所有安全研究的基礎
2. **緩衝區溢出**仍然是最常見的漏洞之一
3. **多層防禦**：Canary + ASLR + NX + PIE
4. **安全編程**：使用安全函數，驗證輸入
5. **持續學習**：新的攻擊和防禦技術不斷湧現

### 下一步

完成本章後，建議學習：
- **21-file-security**：文件權限與訪問控制
- **22-process-security**：進程安全與輸入驗證
- **23-format-string**：格式化字符串漏洞

---

**記住：知識是雙刃劍，請負責任地使用！** 🛡️

---

## 附錄A：編譯選項速查表

| 選項 | 效果 | 用途 |
|-----|------|-----|
| `-fstack-protector` | 啟用 stack canary | 生產環境 |
| `-fno-stack-protector` | 禁用 canary | 教學/調試 |
| `-z execstack` | 棧可執行 | 教學（危險！） |
| `-z noexecstack` | 棧不可執行 | 生產環境（默認） |
| `-pie -fPIE` | 位置無關可執行 | 生產環境 |
| `-no-pie` | 固定地址 | 教學/調試 |
| `-D_FORTIFY_SOURCE=2` | 啟用額外檢查 | 生產環境 |
| `-g` | 包含調試信息 | 調試用 |
| `-O2` | 優化等級 2 | 生產環境 |

## 附錄B：術語表

- **Shellcode**: 用於啟動 shell 的機器碼
- **NOP Sled**: 一系列 NOP 指令，增加跳轉成功率
- **Return Address**: 函數返回後要執行的指令地址
- **Stack Frame**: 函數調用時在棧上分配的內存區域
- **Canary**: 用於檢測棧溢出的隨機值
- **ASLR**: 地址空間布局隨機化
- **NX**: 不可執行保護
- **PIE**: 位置無關可執行
- **Gadget**: ROP 攻擊中使用的代碼片段
- **EIP/RIP**: 指令指針寄存器（x86/x64）
- **ESP/RSP**: 棧指針寄存器（x86/x64）
- **EBP/RBP**: 基址指針寄存器（x86/x64）
