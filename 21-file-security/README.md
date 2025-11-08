# 21. 文件權限與訪問控制 (File Permissions & Access Control)

## ⚠️ 教育聲明

本章節內容**僅供教育和研究目的**使用。所有範例應在合法授權的測試環境中運行。

---

## 📚 目錄

1. [Linux 權限模型](#linux-權限模型)
2. [特殊權限位](#特殊權限位-suidsgidsticky-bit)
3. [Capabilities 機制](#capabilities-機制)
4. [權限提升原理](#權限提升原理教育)
5. [安全編程實踐](#安全編程實踐)
6. [範例程式說明](#範例程式說明)

---

## 1. Linux 權限模型

### 1.1 基本權限 (rwx)

Linux 使用 9 位權限模型：

```
-rwxr-xr--
 │││││││││
 ││││││││└─ 其他用戶 (others) - 可讀
 │││││││└── 其他用戶 - 不可寫
 ││││││└─── 其他用戶 - 不可執行
 │││││└──── 組用戶 (group) - 可讀
 ││││└───── 組用戶 - 不可寫
 │││└────── 組用戶 - 可執行
 ││└─────── 所有者 (owner) - 可讀
 │└──────── 所有者 - 可寫
 └───────── 所有者 - 可執行
```

**權限含義**:
- **r (read, 4)**: 讀取文件內容或列出目錄
- **w (write, 2)**: 修改文件或在目錄中創建/刪除文件
- **x (execute, 1)**: 執行文件或進入目錄

**八進制表示**:
```
rwxr-xr-- = 754
7 (rwx) = 4+2+1 = 所有者
5 (r-x) = 4+0+1 = 組
4 (r--) = 4+0+0 = 其他
```

### 1.2 文件所有權

每個文件有三個關鍵屬性：
- **Owner (UID)**: 文件所有者
- **Group (GID)**: 文件所屬組
- **Others**: 其他所有用戶

**查看**:
```bash
ls -l file.txt
-rw-r--r-- 1 user group 1024 Nov 7 10:00 file.txt
              │    │
              │    └─ 組
              └────── 所有者
```

### 1.3 目錄權限特殊含義

對於目錄：
- **r**: 可以列出目錄內容 (ls)
- **w**: 可以在目錄中創建/刪除文件（需要配合 x）
- **x**: 可以進入目錄 (cd)，訪問目錄中的文件

**重要**:
- 沒有 x 權限，即使有 r 也無法讀取目錄內容的詳細信息
- 沒有 w 權限，無法創建或刪除文件

---

## 2. 特殊權限位 (SUID/SGID/Sticky Bit)

### 2.1 SUID (Set User ID) - 4000

**作用**: 執行文件時以文件所有者的權限運行，而非執行者的權限

**符號**: `s` 在所有者的執行位
```bash
-rwsr-xr-x  # SUID 設置
```

**典型應用**:
```bash
ls -l /usr/bin/passwd
-rwsr-xr-x 1 root root 59640 Mar 22  2019 /usr/bin/passwd
```
- `passwd` 需要修改 `/etc/shadow`（只有 root 可寫）
- 普通用戶執行時臨時獲得 root 權限

**安全風險**:
- SUID 程式如果有漏洞，可能導致權限提升
- 攻擊者可以通過 SUID 程式獲得 root shell

**查找系統中的 SUID 文件**:
```bash
find / -perm -4000 -type f 2>/dev/null
```

### 2.2 SGID (Set Group ID) - 2000

**文件**: 執行時以文件所屬組的權限運行
**目錄**: 在目錄中創建的文件繼承目錄的組

**符號**: `s` 在組的執行位
```bash
-rwxr-sr-x  # SGID 設置
drwxrws---  # 目錄 SGID
```

**應用場景**:
```bash
# 共享目錄，所有文件屬於同一個組
mkdir /shared
chgrp developers /shared
chmod 2775 /shared  # SGID + rwxrwxr-x
```

### 2.3 Sticky Bit - 1000

**作用**: 在目錄上設置，只有文件所有者才能刪除或重命名自己的文件

**符號**: `t` 在其他用戶的執行位
```bash
drwxrwxrwt  # Sticky bit 設置
```

**典型應用**: `/tmp` 目錄
```bash
ls -ld /tmp
drwxrwxrwt 20 root root 4096 Nov 7 10:00 /tmp
```
- 任何人都可以在 /tmp 創建文件
- 但只能刪除自己的文件

### 2.4 設置特殊權限

**使用 chmod**:
```bash
chmod u+s file      # 設置 SUID
chmod g+s file      # 設置 SGID
chmod +t directory  # 設置 Sticky Bit

# 或使用數字
chmod 4755 file     # SUID + rwxr-xr-x
chmod 2755 file     # SGID + rwxr-xr-x
chmod 1777 dir      # Sticky + rwxrwxrwx
```

---

## 3. Capabilities 機制

### 3.1 什麼是 Capabilities?

傳統 Unix 只有兩種用戶：root (全能) 和普通用戶（受限）。Capabilities 將 root 權限**細分**為多個獨立的能力。

**優勢**:
- 避免給程式完整的 root 權限
- 實現最小權限原則
- 更細粒度的安全控制

### 3.2 常用 Capabilities

| Capability | 描述 | 替代的權限 |
|-----------|------|----------|
| `CAP_NET_BIND_SERVICE` | 綁定 < 1024 的端口 | 不需要 root 即可綁定特權端口 |
| `CAP_NET_RAW` | 使用 RAW 和 PACKET socket | ping, tcpdump 等 |
| `CAP_SYS_ADMIN` | 系統管理操作 | mount, sethostname 等 |
| `CAP_SYS_TIME` | 修改系統時間 | 設置時鐘 |
| `CAP_CHOWN` | 更改文件所有權 | chown |
| `CAP_SETUID` | 設置 UID | su, sudo |
| `CAP_KILL` | 發送信號給任意進程 | kill 任意進程 |

**完整列表**: `man capabilities`

### 3.3 使用 Capabilities

**查看**:
```bash
# 查看文件的 capabilities
getcap /usr/bin/ping
/usr/bin/ping = cap_net_raw+ep

# 查看進程的 capabilities
getpcaps <PID>
```

**設置**:
```bash
# 賦予 capability
sudo setcap cap_net_bind_service=+ep ./my_server

# 移除 capability
sudo setcap -r ./my_server

# 多個 capabilities
sudo setcap 'cap_net_bind_service,cap_net_raw=+ep' ./my_tool
```

**Capability 標誌**:
- `e` (Effective): 當前生效
- `p` (Permitted): 允許使用
- `i` (Inheritable): 可繼承

### 3.4 實際應用

**案例 1**: Web 服務器綁定端口 80
```bash
# 傳統方法：需要 root
sudo ./webserver

# 使用 capability：不需要 root
sudo setcap cap_net_bind_service=+ep ./webserver
./webserver  # 以普通用戶運行
```

**案例 2**: Ping 程式
```bash
# ping 需要創建 RAW socket
getcap /bin/ping
/bin/ping = cap_net_raw+ep
```

---

## 4. 權限提升原理（教育）

### 4.1 常見提權路徑

**⚠️ 以下內容僅供理解漏洞原理，禁止用於未授權系統**

1. **SUID 程式漏洞**:
   - 找到有漏洞的 SUID root 程式
   - 利用緩衝區溢出、命令注入等
   - 獲得 root shell

2. **文件權限錯誤**:
   - `/etc/passwd` 或 `/etc/shadow` 可寫
   - SSH 私鑰文件權限過鬆
   - Sudoers 配置錯誤

3. **環境變量劫持**:
   - SUID 程式使用相對路徑調用命令
   - 修改 PATH 環境變量
   - 執行惡意程式

4. **競爭條件** (TOCTOU):
   - 檢查時間 vs 使用時間的間隙
   - 在檢查後、使用前更換文件
   - 符號鏈接攻擊

### 4.2 防禦措施

**最佳實踐**:
```c
✓ 檢查 SUID/SGID 程式的必要性
✓ 使用絕對路徑調用外部程式
✓ 清理環境變量
✓ 使用安全函數檢查權限
✓ 實現最小權限原則
✓ 定期審計文件權限
```

**安全檢查示例**:
```c
// 檢查文件所有權
struct stat st;
if (stat(filename, &st) != 0) {
    perror("stat");
    return -1;
}

if (st.st_uid != expected_uid) {
    fprintf(stderr, "文件所有者不正確\n");
    return -1;
}

// 檢查文件權限
if ((st.st_mode & 0022) != 0) {
    fprintf(stderr, "文件權限過於寬鬆（組或其他可寫）\n");
    return -1;
}
```

---

## 5. 安全編程實踐

### 5.1 安全的臨時文件處理

**危險做法**:
```c
// ✗ 不安全：可預測的文件名
char tmpfile[] = "/tmp/myapp.tmp";
int fd = open(tmpfile, O_CREAT | O_WRONLY, 0644);
```

**安全做法**:
```c
// ✓ 安全：使用 mkstemp()
char tmpfile[] = "/tmp/myapp.XXXXXX";
int fd = mkstemp(tmpfile);  // 自動創建唯一文件名
if (fd == -1) {
    perror("mkstemp");
    return -1;
}

// 設置正確權限
fchmod(fd, 0600);  // 只有所有者可讀寫

// 使用完後刪除
unlink(tmpfile);
```

### 5.2 權限檢查

**檢查有效 UID vs 真實 UID**:
```c
// 獲取真實 UID (執行者的 UID)
uid_t real_uid = getuid();

// 獲取有效 UID (當前生效的 UID，可能是 SUID 賦予的)
uid_t effective_uid = geteuid();

if (effective_uid == 0) {
    printf("當前以 root 權限運行\n");
}

if (real_uid != 0 && effective_uid == 0) {
    printf("這是一個 SUID root 程式\n");
}
```

**放棄特權**:
```c
// SUID 程式在完成特權操作後應放棄特權
if (seteuid(getuid()) != 0) {
    perror("seteuid");
    exit(1);
}
```

### 5.3 安全的文件操作

**使用 O_NOFOLLOW 防止符號鏈接攻擊**:
```c
// 打開文件時不跟隨符號鏈接
int fd = open(filename, O_RDONLY | O_NOFOLLOW);
if (fd == -1) {
    if (errno == ELOOP) {
        fprintf(stderr, "檢測到符號鏈接攻擊\n");
    }
    return -1;
}
```

**原子操作**:
```c
// 使用 O_EXCL 確保文件不存在時才創建
int fd = open(filename, O_CREAT | O_EXCL | O_WRONLY, 0600);
if (fd == -1) {
    if (errno == EEXIST) {
        fprintf(stderr, "文件已存在\n");
    }
    return -1;
}
```

---

## 6. 範例程式說明

### 6.1 permission_demo.c
**目的**: 展示權限檢查和設置

**功能**:
- 顯示文件的詳細權限信息
- 解析權限位（rwx、SUID、SGID、Sticky）
- 演示權限修改
- 檢查有效/真實 UID

**編譯**:
```bash
gcc -o permission_demo permission_demo.c
./permission_demo /path/to/file
```

### 6.2 suid_example.c
**目的**: 演示 SUID 程式的工作原理和安全考慮

**⚠️ 警告**: 這是教育範例，不要在生產環境設置 SUID！

**功能**:
- 顯示真實 UID 和有效 UID
- 演示特權操作
- 演示權限提升風險
- 展示安全的特權放棄

**編譯並設置 SUID (僅測試環境)**:
```bash
gcc -o suid_example suid_example.c
sudo chown root:root suid_example
sudo chmod 4755 suid_example
./suid_example
```

**移除 SUID**:
```bash
sudo chmod 0755 suid_example
```

### 6.3 capability_demo.c
**目的**: 演示 Linux Capabilities 的使用

**功能**:
- 檢查進程的 capabilities
- 嘗試綁定特權端口（<1024）
- 演示 CAP_NET_BIND_SERVICE 的作用

**編譯**:
```bash
gcc -o capability_demo capability_demo.c
```

**不使用 capability（失敗）**:
```bash
./capability_demo 80
# 錯誤: Permission denied
```

**使用 capability（成功）**:
```bash
sudo setcap cap_net_bind_service=+ep ./capability_demo
./capability_demo 80
# 成功綁定端口 80
```

**清理**:
```bash
sudo setcap -r ./capability_demo
```

### 6.4 secure_tempfile.c
**目的**: 展示安全的臨時文件處理

**功能**:
- 對比不安全 vs 安全的臨時文件創建
- 演示 mkstemp() 使用
- 展示權限設置
- 防止符號鏈接攻擊

**編譯**:
```bash
gcc -o secure_tempfile secure_tempfile.c
./secure_tempfile
```

---

## 7. CVE 案例研究

### 7.1 Dirty COW (CVE-2016-5195)

**描述**: Linux 內核競爭條件漏洞

**原理**:
- 利用內存映射的 COW (Copy-On-Write) 機制
- 在寫保護檢查和實際寫入之間存在競爭窗口
- 可以修改只讀文件（包括 SUID 二進制文件）

**影響**: 本地權限提升到 root

**教訓**:
- 競爭條件難以檢測和修復
- 即使是核心系統機制也可能有漏洞

### 7.2 Sudo 權限繞過 (CVE-2019-14287)

**描述**: Sudo RunAs 權限繞過

**原理**:
```bash
# 如果 sudoers 配置為：
# user ALL=(ALL, !root) /bin/bash

# 攻擊者可以通過指定 UID -1 繞過限制
sudo -u#-1 /bin/bash
# 獲得 root shell（UID 0）
```

**修復**: 升級到 sudo 1.8.28+

**教訓**: 配置文件的邏輯漏洞同樣危險

---

## 8. 最佳實踐總結

### ✓ 應該做的:

1. **最小權限原則**
   - 只授予必要的權限
   - 使用 capabilities 替代完整 root 權限
   - 及時放棄不需要的特權

2. **安全的文件操作**
   - 使用 mkstemp() 創建臨時文件
   - 使用 O_NOFOLLOW 防止符號鏈接攻擊
   - 使用 O_EXCL 確保原子創建

3. **權限檢查**
   - 檢查文件所有權和權限
   - 驗證 UID/GID
   - 使用 access() 檢查實際用戶的權限

4. **定期審計**
   - 檢查 SUID/SGID 文件
   - 審查文件權限
   - 監控權限變更

### ✗ 不應該做的:

1. ❌ 不要過度使用 SUID root
2. ❌ 不要使用可預測的臨時文件名
3. ❌ 不要忽略競爭條件風險
4. ❌ 不要信任環境變量
5. ❌ 不要在 SUID 程式中使用 system()

---

## 9. 練習題

### 基礎題

1. 解釋 `chmod 755` 和 `chmod 4755` 的區別
2. 為什麼 /tmp 目錄需要 sticky bit?
3. Capabilities 相比 SUID 有什麼優勢？

### 實踐題

1. 查找系統中所有 SUID root 文件
2. 創建一個只有所有者可讀寫的安全臨時文件
3. 使用 capabilities 讓普通程式綁定端口 80

### 高級題

1. 設計一個安全的 SUID 程式框架
2. 解釋並演示符號鏈接攻擊
3. 實現一個權限審計工具

---

## 10. 參考資料

### 文檔
- `man chmod`
- `man capabilities`
- `man setcap`
- `man 2 access`

### 書籍
- **The Linux Programming Interface** (Michael Kerrisk)
- **Secure Programming for Linux and Unix HOWTO**

### 在線資源
- [Linux Capabilities Wiki](https://wiki.archlinux.org/title/Capabilities)
- [OWASP - File System Security](https://owasp.org/)

---

**記住**: 文件權限是系統安全的第一道防線！🛡️
