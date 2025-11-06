# 實用工具庫 (Utils)

## 📖 簡介

Utils 目錄包含可重用的工具庫和組件，可以直接應用到實際項目中。這些工具展示了如何構建實用的、可維護的代碼模塊。

## 📁 工具列表

### 1. 配置文件解析器 (Config Parser)

**文件**:
- `config_parser.h` - INI 格式配置文件解析器（Header-only）
- `config_demo.c` - 使用示範程序

**功能特性**:
- ✅ 支持 INI 格式配置文件
- ✅ Section/Key/Value 結構
- ✅ 註釋支持（# 或 ;）
- ✅ 類型轉換（string/int/bool）
- ✅ 默認值支持
- ✅ Header-only 設計，易於集成

**使用方式**:

#### 1. 配置文件格式 (server.conf)

```ini
# Server Configuration
[server]
host = 0.0.0.0
port = 8080
max_connections = 100
timeout = 30

[logging]
level = info
file = /var/log/server.log
rotate = true

[features]
enable_cache = yes
enable_ssl = false
```

#### 2. 代碼中使用

```c
#include "config_parser.h"

int main(void)
{
    // 加載配置文件
    config_t *cfg = config_load("server.conf");
    if (!cfg) {
        fprintf(stderr, "無法加載配置文件\n");
        return 1;
    }

    // 讀取字符串配置
    const char *host = config_get(cfg, "server", "host");
    printf("Host: %s\n", host);

    // 讀取整數配置（帶默認值）
    int port = config_get_int(cfg, "server", "port", 8080);
    printf("Port: %d\n", port);

    // 讀取布爾配置
    int enable_ssl = config_get_bool(cfg, "features", "enable_ssl", 0);
    printf("SSL: %s\n", enable_ssl ? "Enabled" : "Disabled");

    // 釋放配置對象
    config_free(cfg);

    return 0;
}
```

#### 3. 編譯和運行

```bash
# 編譯演示程序
gcc -o config_demo config_demo.c

# 運行（會自動創建示例配置）
./config_demo

# 使用自定義配置文件
./config_demo my_config.ini
```

### API 文檔

#### config_load()
加載配置文件。

```c
config_t* config_load(const char *filename);
```

**參數**:
- `filename` - 配置文件路徑

**返回**: 成功返回配置對象，失敗返回 NULL

---

#### config_get()
獲取字符串配置值。

```c
const char* config_get(config_t *cfg, const char *section, const char *key);
```

**參數**:
- `cfg` - 配置對象
- `section` - Section 名稱（NULL 或 "" 表示全局）
- `key` - 鍵名

**返回**: 找到返回值字符串，未找到返回 NULL

---

#### config_get_default()
獲取字符串配置值（帶默認值）。

```c
const char* config_get_default(config_t *cfg, const char *section,
                               const char *key, const char *default_value);
```

**返回**: 找到返回配置值，未找到返回默認值

---

#### config_get_int()
獲取整數配置值。

```c
int config_get_int(config_t *cfg, const char *section,
                   const char *key, int default_value);
```

**返回**: 找到返回整數值，未找到返回默認值

---

#### config_get_bool()
獲取布爾配置值。

```c
int config_get_bool(config_t *cfg, const char *section,
                    const char *key, int default_value);
```

**支持的布爾值**:
- True: `true`, `yes`, `1` (不區分大小寫)
- False: `false`, `no`, `0` (不區分大小寫)

**返回**: 1 (true) 或 0 (false)

---

#### config_print()
打印所有配置（調試用）。

```c
void config_print(config_t *cfg);
```

---

#### config_free()
釋放配置對象。

```c
void config_free(config_t *cfg);
```

**重要**: 使用完配置後必須調用此函數釋放內存。

---

## 💡 使用場景

### 1. 服務器配置

```c
config_t *cfg = config_load("server.conf");

// 網絡配置
const char *host = config_get(cfg, "server", "host");
int port = config_get_int(cfg, "server", "port", 8080);
int max_conn = config_get_int(cfg, "server", "max_connections", 100);

// 應用配置
start_server(host, port, max_conn);
```

### 2. 日誌配置

```c
config_t *cfg = config_load("app.conf");

// 日誌配置
const char *log_file = config_get(cfg, "logging", "file");
const char *log_level = config_get(cfg, "logging", "level");
int rotate = config_get_bool(cfg, "logging", "rotate", 0);

// 初始化日誌系統
init_logger(log_file, log_level, rotate);
```

### 3. 功能開關

```c
config_t *cfg = config_load("features.conf");

// 功能開關
if (config_get_bool(cfg, "features", "enable_cache", 0)) {
    enable_cache();
}

if (config_get_bool(cfg, "features", "enable_ssl", 0)) {
    setup_ssl();
}
```

## 🎨 設計特點

### 1. Header-Only 設計

**優點**:
- 無需編譯成獨立的庫文件
- 直接 `#include "config_parser.h"` 即可使用
- 便於集成到項目中

**使用**:
```c
#include "config_parser.h"  // 就這麼簡單！
```

### 2. 內存管理

**自動管理**:
- 所有內存由 config_t 對象管理
- 調用 `config_free()` 自動釋放所有資源

**注意事項**:
```c
config_t *cfg = config_load("config.ini");

// ✅ 正確：保存配置對象
const char *value = config_get(cfg, "section", "key");
printf("%s\n", value);

// ❌ 錯誤：不要釋放返回的字符串
// free(value);  // 千萬不要這樣做！

// ✅ 正確：統一釋放
config_free(cfg);
```

### 3. 錯誤處理

```c
config_t *cfg = config_load("config.ini");
if (!cfg) {
    // 文件不存在或解析失敗
    fprintf(stderr, "無法加載配置文件\n");
    return 1;
}

// 使用默認值處理缺失的配置
int port = config_get_int(cfg, "server", "port", 8080);
```

## 📚 完整示例

### 示例 1: Web 服務器配置

**config.ini**:
```ini
[server]
host = 0.0.0.0
port = 8080
workers = 4
max_connections = 1000
timeout = 30

[ssl]
enable = true
cert_file = /etc/ssl/server.crt
key_file = /etc/ssl/server.key

[logging]
level = info
access_log = /var/log/access.log
error_log = /var/log/error.log
```

**main.c**:
```c
#include "config_parser.h"
#include <stdio.h>

int main(void)
{
    config_t *cfg = config_load("config.ini");
    if (!cfg) return 1;

    // 服務器配置
    const char *host = config_get_default(cfg, "server", "host", "0.0.0.0");
    int port = config_get_int(cfg, "server", "port", 8080);
    int workers = config_get_int(cfg, "server", "workers", 4);
    int max_conn = config_get_int(cfg, "server", "max_connections", 1000);

    // SSL 配置
    int ssl_enabled = config_get_bool(cfg, "ssl", "enable", 0);
    const char *cert_file = config_get(cfg, "ssl", "cert_file");
    const char *key_file = config_get(cfg, "ssl", "key_file");

    // 日誌配置
    const char *log_level = config_get_default(cfg, "logging", "level", "info");

    printf("Starting server on %s:%d\n", host, port);
    printf("Workers: %d, Max connections: %d\n", workers, max_conn);
    printf("SSL: %s\n", ssl_enabled ? "Enabled" : "Disabled");
    printf("Log level: %s\n", log_level);

    // TODO: 啟動服務器
    // start_server(host, port, workers, max_conn, ssl_enabled, cert_file, key_file);

    config_free(cfg);
    return 0;
}
```

## 🚀 擴展方向

### 1. 添加更多類型支持

```c
// 添加浮點數支持
double config_get_double(config_t *cfg, const char *section,
                         const char *key, double default_value);

// 添加數組支持
const char** config_get_array(config_t *cfg, const char *section,
                               const char *key, int *count);
```

### 2. 配置寫入支持

```c
// 修改配置值
int config_set(config_t *cfg, const char *section,
               const char *key, const char *value);

// 保存到文件
int config_save(config_t *cfg, const char *filename);
```

### 3. 配置驗證

```c
// 驗證必需的配置項
int config_require(config_t *cfg, const char *section, const char *key);

// 驗證配置值範圍
int config_validate_int(config_t *cfg, const char *section,
                        const char *key, int min, int max);
```

## 📝 學習要點

### 1. INI 格式

INI 是一種簡單的配置文件格式：
- **Section**: `[section_name]`
- **Key-Value**: `key = value`
- **註釋**: `# comment` 或 `; comment`

### 2. 解析技術

- 逐行讀取文件
- 字符串處理（trim、split）
- 鏈表數據結構
- 內存管理

### 3. Header-Only 設計

- 所有實現都在 .h 文件中
- 使用 `static` 關鍵字避免重複定義
- 便於集成和分發

## 🎯 總結

### 優點

✅ 簡單易用 - 幾行代碼即可使用
✅ Header-Only - 無需編譯成庫
✅ 類型安全 - 支持 string/int/bool
✅ 默認值 - 處理缺失配置
✅ 跨平台 - 純 C 實現

### 適用場景

- 小型到中型項目配置
- 命令行工具配置
- 服務器應用配置
- 學習配置文件解析

---

**下一步**: 在你的項目中試用這個配置解析器！
