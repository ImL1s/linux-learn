/*
 * 檔案名稱: config_parser.h
 * 功能說明: INI 格式配置文件解析器 - 可重用的工具庫
 *
 * 支持的 INI 格式:
 *   [section]
 *   key = value
 *   # 註釋
 *
 * 使用方式:
 *   config_t *cfg = config_load("config.ini");
 *   const char *value = config_get(cfg, "section", "key");
 *   config_free(cfg);
 */

#ifndef CONFIG_PARSER_H
#define CONFIG_PARSER_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

/*
 * 配置項結構
 */
typedef struct config_entry_s {
    char *section;
    char *key;
    char *value;
    struct config_entry_s *next;
} config_entry_t;

/*
 * 配置結構
 */
typedef struct {
    config_entry_t *head;
    int entry_count;
} config_t;

/*
 * 移除字符串首尾空白
 */
static char* trim(char *str)
{
    char *end;

    // 移除開頭空白
    while (isspace((unsigned char)*str)) str++;

    if (*str == 0) return str;

    // 移除結尾空白
    end = str + strlen(str) - 1;
    while (end > str && isspace((unsigned char)*end)) end--;
    end[1] = '\0';

    return str;
}

/*
 * 創建新的配置項
 */
static config_entry_t* config_entry_create(const char *section,
                                           const char *key,
                                           const char *value)
{
    config_entry_t *entry = (config_entry_t*)malloc(sizeof(config_entry_t));
    if (!entry) return NULL;

    entry->section = strdup(section);
    entry->key = strdup(key);
    entry->value = strdup(value);
    entry->next = NULL;

    return entry;
}

/*
 * 釋放配置項
 */
static void config_entry_free(config_entry_t *entry)
{
    if (!entry) return;
    free(entry->section);
    free(entry->key);
    free(entry->value);
    free(entry);
}

/*
 * 加載配置文件
 *
 * 參數:
 *   filename - 配置文件路徑
 *
 * 返回:
 *   成功返回配置對象，失敗返回 NULL
 */
static config_t* config_load(const char *filename)
{
    FILE *fp = fopen(filename, "r");
    if (!fp) {
        perror("fopen");
        return NULL;
    }

    config_t *cfg = (config_t*)malloc(sizeof(config_t));
    if (!cfg) {
        fclose(fp);
        return NULL;
    }

    cfg->head = NULL;
    cfg->entry_count = 0;

    char line[1024];
    char current_section[256] = "";
    config_entry_t *tail = NULL;

    while (fgets(line, sizeof(line), fp)) {
        char *p = trim(line);

        // 跳過空行和註釋
        if (*p == '\0' || *p == '#' || *p == ';') {
            continue;
        }

        // 解析 section: [section_name]
        if (*p == '[') {
            char *end = strchr(p, ']');
            if (end) {
                *end = '\0';
                strncpy(current_section, p + 1, sizeof(current_section) - 1);
                trim(current_section);
            }
            continue;
        }

        // 解析 key = value
        char *equal = strchr(p, '=');
        if (equal) {
            *equal = '\0';
            char *key = trim(p);
            char *value = trim(equal + 1);

            config_entry_t *entry = config_entry_create(current_section, key, value);
            if (entry) {
                if (tail) {
                    tail->next = entry;
                } else {
                    cfg->head = entry;
                }
                tail = entry;
                cfg->entry_count++;
            }
        }
    }

    fclose(fp);
    return cfg;
}

/*
 * 獲取配置值
 *
 * 參數:
 *   cfg - 配置對象
 *   section - 區段名（可為 NULL 或 "" 表示全局）
 *   key - 鍵名
 *
 * 返回:
 *   找到返回值字符串，未找到返回 NULL
 */
static const char* config_get(config_t *cfg, const char *section, const char *key)
{
    if (!cfg || !key) return NULL;

    const char *sec = section ? section : "";

    for (config_entry_t *e = cfg->head; e != NULL; e = e->next) {
        if (strcmp(e->key, key) == 0 && strcmp(e->section, sec) == 0) {
            return e->value;
        }
    }

    return NULL;
}

/*
 * 獲取配置值（帶默認值）
 */
static const char* config_get_default(config_t *cfg, const char *section,
                                      const char *key, const char *default_value)
{
    const char *value = config_get(cfg, section, key);
    return value ? value : default_value;
}

/*
 * 獲取整數配置值
 */
static int config_get_int(config_t *cfg, const char *section,
                          const char *key, int default_value)
{
    const char *value = config_get(cfg, section, key);
    return value ? atoi(value) : default_value;
}

/*
 * 獲取布爾配置值
 */
static int config_get_bool(config_t *cfg, const char *section,
                           const char *key, int default_value)
{
    const char *value = config_get(cfg, section, key);
    if (!value) return default_value;

    if (strcasecmp(value, "true") == 0 ||
        strcasecmp(value, "yes") == 0 ||
        strcasecmp(value, "1") == 0) {
        return 1;
    }

    if (strcasecmp(value, "false") == 0 ||
        strcasecmp(value, "no") == 0 ||
        strcasecmp(value, "0") == 0) {
        return 0;
    }

    return default_value;
}

/*
 * 打印所有配置（用於調試）
 */
static void config_print(config_t *cfg)
{
    if (!cfg) return;

    printf("配置項總數: %d\n\n", cfg->entry_count);

    const char *last_section = "";
    for (config_entry_t *e = cfg->head; e != NULL; e = e->next) {
        if (strcmp(e->section, last_section) != 0) {
            printf("\n[%s]\n", e->section);
            last_section = e->section;
        }
        printf("%s = %s\n", e->key, e->value);
    }
}

/*
 * 釋放配置對象
 */
static void config_free(config_t *cfg)
{
    if (!cfg) return;

    config_entry_t *e = cfg->head;
    while (e) {
        config_entry_t *next = e->next;
        config_entry_free(e);
        e = next;
    }

    free(cfg);
}

#endif /* CONFIG_PARSER_H */
