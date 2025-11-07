# Linux 系統安全與攻防教育專案規劃

## ⚠️ 重要聲明

**本專案僅供教育和研究目的使用**

- ✅ 合法用途：CTF 競賽、安全研究、滲透測試授權環境、防禦性安全
- ❌ 禁止用途：未經授權的系統訪問、惡意攻擊、破壞性行為
- 📜 使用者責任：所有使用者必須遵守當地法律和道德規範

---

## 🎯 專案目標

1. **教育性**：幫助學習者理解系統安全的基本原理
2. **實踐性**：提供可執行的範例和實驗環境
3. **防禦性**：強調如何防禦各種攻擊
4. **道德性**：培養負責任的安全研究態度

---

## 📚 學習路徑（基礎 → 進階 → 深入）

### 階段 1：基礎安全概念 (Foundation)

#### 20. 內存安全基礎 (Memory Safety)
**目錄**: `20-memory-safety/`

**內容**:
- Stack 和 Heap 布局原理
- Buffer Overflow 原理與演示（受控環境）
- Stack Canary 防禦機制
- ASLR (Address Space Layout Randomization)
- NX/DEP (Non-Executable Stack)
- 安全編程實踐

**範例程式**:
1. `stack_layout.c` - 展示棧結構（200行）
2. `buffer_overflow_demo.c` - Buffer overflow 教學範例（300行）
3. `canary_demo.c` - Stack canary 演示（250行）
4. `safe_string.c` - 安全字符串操作（200行）

**README**: 500+ 行，包含：
- 內存布局圖解
- 漏洞原理詳解
- 防禦技術說明
- CVE 案例研究

---

#### 21. 文件權限與訪問控制 (File Permissions & Access Control)
**目錄**: `21-file-security/`

**內容**:
- Linux 權限模型（rwx, owner/group/other）
- SUID/SGID/Sticky Bit
- POSIX Capabilities
- ACL (Access Control Lists)
- 權限提升原理（教育）
- Secure coding for file operations

**範例程式**:
1. `permission_demo.c` - 權限檢查和設置（200行）
2. `suid_example.c` - SUID 程式範例（300行）
3. `capability_demo.c` - Capabilities 使用（250行）
4. `secure_tempfile.c` - 安全臨時文件（200行）

**README**: 450+ 行

---

#### 22. 進程安全 (Process Security)
**目錄**: `22-process-security/`

**內容**:
- 環境變量安全
- Command Injection 原理與防禦
- Path Traversal 攻擊
- Secure execution (execve vs system)
- Input validation
- Sandboxing 基礎

**範例程式**:
1. `env_injection.c` - 環境變量注入演示（250行）
2. `command_injection.c` - Command injection 範例（300行）
3. `safe_exec.c` - 安全執行外部命令（250行）
4. `input_validation.c` - 輸入驗證範例（200行）

**README**: 500+ 行

---

### 階段 2：進階攻防技術 (Intermediate)

#### 23. 格式化字符串漏洞 (Format String Vulnerabilities)
**目錄**: `23-format-string/`

**內容**:
- printf 系列函數原理
- 格式化字符串漏洞原理
- 內存讀寫技術（教育）
- 防禦措施（編譯器警告、安全函數）
- 靜態分析工具

**範例程式**:
1. `format_vuln_demo.c` - 格式化漏洞演示（300行）
2. `format_exploit.c` - 利用技術（教育）（400行）
3. `secure_format.c` - 安全格式化輸出（200行）
4. `format_scanner.c` - 簡單掃描工具（300行）

**README**: 600+ 行

---

#### 24. 競爭條件 (Race Conditions)
**目錄**: `24-race-conditions/`

**內容**:
- TOCTOU (Time-of-Check-Time-of-Use) 漏洞
- 文件競爭攻擊
- Symlink 攻擊
- 防禦技術（原子操作、文件鎖、O_NOFOLLOW）
- 多線程競爭條件

**範例程式**:
1. `toctou_demo.c` - TOCTOU 漏洞演示（300行）
2. `symlink_attack.c` - Symlink 攻擊（250行）
3. `secure_file_ops.c` - 安全文件操作（300行）
4. `race_detector.c` - 競爭條件檢測工具（350行）

**README**: 550+ 行

---

#### 25. 網絡安全基礎 (Network Security Basics)
**目錄**: `25-network-security/`

**內容**:
- Packet Sniffing 原理（教育）
- Port Scanning 技術
- TCP/IP 安全問題
- 基本防火牆實現
- 加密通訊（TLS基礎）
- Man-in-the-Middle 攻擊原理

**範例程式**:
1. `packet_sniffer.c` - 簡單抓包工具（教育）（400行）
2. `port_scanner.c` - 端口掃描器（300行）
3. `simple_firewall.c` - 基本防火牆（500行）
4. `secure_socket.c` - 加密通訊範例（400行）

**README**: 650+ 行

---

#### 26. 加密基礎 (Cryptography Basics)
**目錄**: `26-cryptography/`

**內容**:
- 對稱加密（AES）
- 非對稱加密（RSA）
- Hash 函數（SHA-256）
- 數字簽名
- 密鑰管理
- 常見加密錯誤

**範例程式**:
1. `aes_encrypt.c` - AES 加密實現（400行）
2. `rsa_demo.c` - RSA 加密演示（500行）
3. `hash_demo.c` - Hash 函數應用（300行）
4. `secure_password.c` - 安全密碼存儲（300行）

**README**: 700+ 行
**需要**: OpenSSL 庫

---

### 階段 3：深入高級技術 (Advanced)

#### 27. Return-Oriented Programming (ROP)
**目錄**: `27-rop-techniques/`

**內容**:
- ROP 基本原理
- Gadget 查找技術
- ROP 鏈構造（教育）
- 現代防禦機制（PIE, CFI）
- ROP 檢測技術

**範例程式**:
1. `rop_basics.c` - ROP 原理演示（300行）
2. `gadget_finder.c` - Gadget 搜索工具（400行）
3. `rop_chain_builder.c` - ROP 鏈構造器（教育）（500行）
4. `rop_detector.c` - ROP 檢測工具（350行）

**README**: 700+ 行

---

#### 28. 內核安全 (Kernel Security)
**目錄**: `28-kernel-security/`

**內容**:
- Linux 內核模塊基礎
- System call 機制
- Kernel exploit 原理（教育）
- SELinux/AppArmor 基礎
- Seccomp 系統調用過濾
- 內核調試技術

**範例程式**:
1. `simple_lkm.c` - 簡單內核模塊（300行）
2. `syscall_monitor.c` - 系統調用監控（400行）
3. `seccomp_demo.c` - Seccomp 演示（300行）
4. `kernel_hardening.c` - 內核加固範例（350行）

**README**: 800+ 行
**警告**: 需要 root 權限，僅在測試環境使用

---

#### 29. Web 安全基礎 (Web Security)
**目錄**: `29-web-security/`

**內容**:
- SQL Injection 原理與防禦
- XSS (Cross-Site Scripting) 攻擊
- CSRF (Cross-Site Request Forgery)
- Path Traversal
- 安全的 Web 編程
- Input sanitization

**範例程式**:
1. `sql_injection_demo.c` - SQL 注入演示（400行）
2. `xss_demo.c` - XSS 範例（350行）
3. `secure_web_server.c` - 安全 Web 服務器（600行）
4. `web_scanner.c` - 簡單 Web 掃描器（500行）

**README**: 750+ 行
**需要**: SQLite 庫

---

#### 30. 逆向工程基礎 (Reverse Engineering)
**目錄**: `30-reverse-engineering/`

**內容**:
- ELF 文件格式
- 反匯編基礎
- 動態分析技術
- Debugger 原理（ptrace）
- Anti-debugging 技術
- Binary patching

**範例程式**:
1. `elf_parser.c` - ELF 解析器（500行）
2. `simple_debugger.c` - 簡單調試器（600行）
3. `disassembler.c` - 基本反匯編器（450行）
4. `binary_patcher.c` - 二進制補丁工具（400行）

**README**: 800+ 行

---

#### 31. 滲透測試工具開發 (Pentesting Tools)
**目錄**: `31-pentest-tools/`

**內容**:
- 漏洞掃描器架構
- Exploit 開發框架（教育）
- Payload 生成器
- Post-exploitation 技術（教育）
- 報告生成
- 工具集成

**範例程式**:
1. `vuln_scanner.c` - 漏洞掃描框架（700行）
2. `exploit_framework.c` - Exploit 框架（教育）（800行）
3. `payload_generator.c` - Payload 生成器（500行）
4. `report_generator.c` - 報告生成工具（400行）

**README**: 900+ 行

---

## 🛠️ 輔助工具

#### 32. 安全測試環境 (Security Lab)
**目錄**: `32-security-lab/`

**內容**:
- Vulnerable 測試程式集
- Docker 容器配置
- 虛擬化環境設置
- 網絡隔離配置
- 日誌和監控工具

**工具**:
1. 自動化測試腳本
2. 環境重置工具
3. 漏洞環境部署
4. 練習平台

---

## 📊 專案統計預估

- **新增主題**: 13 個（20-32）
- **源文件**: 50+ 個程式
- **代碼行數**: 18,000+ 行
- **文檔行數**: 8,000+ 行
- **README 平均**: 600+ 行/個

---

## 🎓 學習建議

### 前置知識
1. 完成 01-19 基礎 Linux 系統編程
2. 熟悉 C 語言和指針
3. 了解計算機組成原理
4. 基本的網絡知識

### 學習路徑
1. **階段1（20-22）**: 2-3 週，建立安全基礎
2. **階段2（23-26）**: 3-4 週，掌握進階技術
3. **階段3（27-31）**: 4-6 週，深入高級主題
4. **實踐（32）**: 持續練習和測試

### 道德準則
1. 僅在授權環境中測試
2. 不要攻擊生產系統
3. 負責任地披露漏洞
4. 遵守法律和道德規範
5. 尊重他人隱私和財產

---

## 📚 參考資料

### 書籍
- **The Art of Exploitation** (Jon Erickson)
- **Hacking: The Art of Exploitation** (2nd Edition)
- **The Shellcoder's Handbook** (3rd Edition)
- **Gray Hat Hacking** (5th Edition)
- **Linux Kernel Development** (Robert Love)

### 在線資源
- **OWASP Top 10**
- **CVE Database** (cve.mitre.org)
- **Exploit Database** (exploit-db.com)
- **CTF Time** (ctftime.org)
- **HackTheBox / TryHackMe** (練習平台)

### 工具
- **GDB** - GNU Debugger
- **Radare2** - 逆向工程框架
- **pwntools** - Exploit 開發
- **Wireshark** - 網絡分析
- **Metasploit** - 滲透測試框架

---

## ⚖️ 法律聲明

本專案中的所有內容：
1. 僅供教育和研究目的
2. 不應用於未經授權的系統
3. 使用者須自行承擔法律責任
4. 作者不對濫用行為負責
5. 所有範例都在隔離環境中測試

**在進行任何安全測試前，請確保獲得明確的書面授權！**

---

## 🚀 開始使用

### 環境準備
```bash
# 安裝必要工具
sudo apt-get install build-essential gdb radare2 netcat nmap wireshark

# 創建隔離測試環境（推薦使用 Docker 或 VM）
docker pull ubuntu:22.04

# 禁用 ASLR（僅在測試環境）
echo 0 | sudo tee /proc/sys/kernel/randomize_va_space

# 編譯範例（加上必要的不安全標誌，僅用於教學）
gcc -fno-stack-protector -z execstack -no-pie -o vuln vuln.c
```

### 學習流程
1. 閱讀每個主題的 README
2. 理解漏洞原理
3. 編譯並運行範例
4. 嘗試防禦技術
5. 完成每個章節的練習

---

## 🤝 貢獻指南

歡迎貢獻，但請遵守：
1. 所有內容必須有教育目的
2. 包含詳細的註解和說明
3. 提供防禦方法和最佳實踐
4. 標註任何潛在危險
5. 不提交 0-day exploits

---

**記住：With great power comes great responsibility!** 💪🛡️

使用這些知識來構建更安全的系統，而不是破壞系統！
