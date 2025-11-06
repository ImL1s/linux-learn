/*
 * 檔案名稱: config_demo.c
 * 功能說明: 配置文件解析器使用示範
 *
 * 編譯方式: gcc -o config_demo config_demo.c
 * 執行方式: ./config_demo [config_file]
 */

#include "config_parser.h"
#include <stdio.h>

int main(int argc, char *argv[])
{
    const char *config_file = "server.conf";

    if (argc > 1) {
        config_file = argv[1];
    }

    printf("========================================\n");
    printf("  配置文件解析器演示\n");
    printf("========================================\n");
    printf("加載配置文件: %s\n\n", config_file);

    // 如果配置文件不存在，創建示例配置
    FILE *fp = fopen(config_file, "r");
    if (!fp) {
        printf("配置文件不存在，創建示例配置...\n\n");

        fp = fopen(config_file, "w");
        if (fp) {
            fprintf(fp,
                "# Server Configuration File\n"
                "# Lines starting with # are comments\n"
                "\n"
                "[server]\n"
                "host = 0.0.0.0\n"
                "port = 8080\n"
                "max_connections = 100\n"
                "timeout = 30\n"
                "\n"
                "[logging]\n"
                "level = info\n"
                "file = /var/log/server.log\n"
                "rotate = true\n"
                "max_size = 10M\n"
                "\n"
                "[database]\n"
                "host = localhost\n"
                "port = 3306\n"
                "username = root\n"
                "password = secret\n"
                "database = mydb\n"
                "pool_size = 10\n"
                "\n"
                "[features]\n"
                "enable_cache = yes\n"
                "enable_compression = true\n"
                "enable_ssl = false\n"
            );
            fclose(fp);
            printf("示例配置已創建！\n\n");
        }
    } else {
        fclose(fp);
    }

    // 加載配置
    config_t *cfg = config_load(config_file);
    if (!cfg) {
        fprintf(stderr, "錯誤: 無法加載配置文件\n");
        return 1;
    }

    printf("配置加載成功！\n\n");

    // 打印所有配置
    printf("========================================\n");
    printf("  所有配置項\n");
    printf("========================================\n");
    config_print(cfg);

    printf("\n========================================\n");
    printf("  讀取特定配置\n");
    printf("========================================\n\n");

    // 讀取服務器配置
    printf("[Server]\n");
    printf("  Host: %s\n",
           config_get_default(cfg, "server", "host", "0.0.0.0"));
    printf("  Port: %d\n",
           config_get_int(cfg, "server", "port", 8080));
    printf("  Max Connections: %d\n",
           config_get_int(cfg, "server", "max_connections", 100));
    printf("  Timeout: %d seconds\n\n",
           config_get_int(cfg, "server", "timeout", 30));

    // 讀取日誌配置
    printf("[Logging]\n");
    printf("  Level: %s\n",
           config_get_default(cfg, "logging", "level", "info"));
    printf("  File: %s\n",
           config_get_default(cfg, "logging", "file", "server.log"));
    printf("  Rotate: %s\n",
           config_get_bool(cfg, "logging", "rotate", 0) ? "Yes" : "No");
    printf("  Max Size: %s\n\n",
           config_get_default(cfg, "logging", "max_size", "10M"));

    // 讀取數據庫配置
    printf("[Database]\n");
    printf("  Host: %s\n",
           config_get_default(cfg, "database", "host", "localhost"));
    printf("  Port: %d\n",
           config_get_int(cfg, "database", "port", 3306));
    printf("  Username: %s\n",
           config_get_default(cfg, "database", "username", "root"));
    printf("  Password: %s\n",
           config_get_default(cfg, "database", "password", "****"));
    printf("  Database: %s\n",
           config_get_default(cfg, "database", "database", "mydb"));
    printf("  Pool Size: %d\n\n",
           config_get_int(cfg, "database", "pool_size", 10));

    // 讀取功能開關
    printf("[Features]\n");
    printf("  Enable Cache: %s\n",
           config_get_bool(cfg, "features", "enable_cache", 0) ? "Yes" : "No");
    printf("  Enable Compression: %s\n",
           config_get_bool(cfg, "features", "enable_compression", 0) ? "Yes" : "No");
    printf("  Enable SSL: %s\n\n",
           config_get_bool(cfg, "features", "enable_ssl", 0) ? "Yes" : "No");

    // 測試不存在的配置（使用默認值）
    printf("[測試默認值]\n");
    printf("  不存在的鍵: %s\n",
           config_get_default(cfg, "server", "nonexistent", "default_value"));
    printf("  不存在的整數: %d\n",
           config_get_int(cfg, "server", "nonexistent_int", 999));
    printf("  不存在的布爾: %s\n\n",
           config_get_bool(cfg, "server", "nonexistent_bool", 1) ? "Yes" : "No");

    printf("========================================\n\n");

    // 釋放配置
    config_free(cfg);

    printf("✓ 配置解析器演示完成！\n");
    printf("\n使用要點:\n");
    printf("  1. config_load() - 加載配置文件\n");
    printf("  2. config_get() - 獲取字符串值\n");
    printf("  3. config_get_int() - 獲取整數值\n");
    printf("  4. config_get_bool() - 獲取布爾值\n");
    printf("  5. config_free() - 釋放配置對象\n\n");

    return 0;
}
