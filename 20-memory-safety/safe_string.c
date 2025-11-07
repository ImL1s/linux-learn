/*
 * 文件: safe_string.c
 * 目的: 展示安全的字符串操作和最佳實踐
 *
 * 編譯: gcc -Wall -O2 -o safe_string safe_string.c
 * 運行: ./safe_string
 *
 * 教育目的:
 * - 對比危險函數和安全函數
 * - 展示正確的輸入驗證
 * - 演示安全的字符串處理模式
 * - 提供可重用的安全函數庫
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <stdarg.h>
#include <limits.h>
#include <errno.h>
#include <ctype.h>

#define COLOR_RESET   "\x1b[0m"
#define COLOR_RED     "\x1b[31m"
#define COLOR_GREEN   "\x1b[32m"
#define COLOR_YELLOW  "\x1b[33m"
#define COLOR_CYAN    "\x1b[36m"

/**
 * 安全字符串複製 - 替代 strcpy()
 *
 * @param dest: 目標緩衝區
 * @param src: 源字符串
 * @param dest_size: 目標緩衝區大小
 * @return: 複製的字符數，-1 表示錯誤
 */
int safe_strcpy(char *dest, const char *src, size_t dest_size) {
    if (dest == NULL || src == NULL || dest_size == 0) {
        return -1;
    }

    size_t src_len = strlen(src);

    if (src_len >= dest_size) {
        /* 源字符串太長，進行截斷 */
        strncpy(dest, src, dest_size - 1);
        dest[dest_size - 1] = '\0';
        return dest_size - 1;  /* 返回實際複製的字符數 */
    } else {
        /* 可以完整複製 */
        strcpy(dest, src);
        return src_len;
    }
}

/**
 * 安全字符串連接 - 替代 strcat()
 *
 * @param dest: 目標緩衝區
 * @param src: 源字符串
 * @param dest_size: 目標緩衝區大小
 * @return: 目標字符串的新長度，-1 表示錯誤
 */
int safe_strcat(char *dest, const char *src, size_t dest_size) {
    if (dest == NULL || src == NULL || dest_size == 0) {
        return -1;
    }

    size_t dest_len = strlen(dest);
    size_t src_len = strlen(src);

    if (dest_len >= dest_size) {
        /* 目標已滿或無效 */
        return -1;
    }

    size_t available = dest_size - dest_len - 1;  /* 可用空間（減去 NULL 終止） */

    if (src_len > available) {
        /* 空間不足，截斷 */
        strncat(dest, src, available);
        dest[dest_size - 1] = '\0';
        return dest_size - 1;
    } else {
        /* 可以完整連接 */
        strcat(dest, src);
        return dest_len + src_len;
    }
}

/**
 * 安全格式化字符串 - 替代 sprintf()
 *
 * @param dest: 目標緩衝區
 * @param dest_size: 目標緩衝區大小
 * @param format: 格式字符串
 * @return: 寫入的字符數，-1 表示錯誤或截斷
 */
int safe_sprintf(char *dest, size_t dest_size, const char *format, ...) {
    if (dest == NULL || format == NULL || dest_size == 0) {
        return -1;
    }

    va_list args;
    va_start(args, format);

    int written = vsnprintf(dest, dest_size, format, args);

    va_end(args);

    if (written < 0) {
        /* 編碼錯誤 */
        return -1;
    }

    if ((size_t)written >= dest_size) {
        /* 輸出被截斷 */
        return -1;
    }

    return written;
}

/**
 * 安全讀取一行 - 替代 gets()
 *
 * @param buffer: 目標緩衝區
 * @param size: 緩衝區大小
 * @param fp: 文件指針 (NULL 表示 stdin)
 * @return: 讀取的字符串，NULL 表示錯誤或 EOF
 */
char *safe_getline(char *buffer, size_t size, FILE *fp) {
    if (buffer == NULL || size == 0) {
        return NULL;
    }

    if (fp == NULL) {
        fp = stdin;
    }

    if (fgets(buffer, size, fp) == NULL) {
        return NULL;  /* EOF 或錯誤 */
    }

    /* 移除尾部的換行符 */
    size_t len = strlen(buffer);
    if (len > 0 && buffer[len - 1] == '\n') {
        buffer[len - 1] = '\0';
    }

    return buffer;
}

/**
 * 安全讀取整數
 *
 * @param result: 存儲結果的指針
 * @param prompt: 提示信息
 * @return: 0 成功，-1 失敗
 */
int safe_read_int(int *result, const char *prompt) {
    char buffer[128];
    char *endptr;
    long value;

    if (result == NULL) {
        return -1;
    }

    if (prompt != NULL) {
        printf("%s", prompt);
        fflush(stdout);
    }

    if (fgets(buffer, sizeof(buffer), stdin) == NULL) {
        return -1;
    }

    errno = 0;
    value = strtol(buffer, &endptr, 10);

    /* 檢查轉換錯誤 */
    if (endptr == buffer) {
        fprintf(stderr, "錯誤: 無效的數字\n");
        return -1;
    }

    if (*endptr != '\n' && *endptr != '\0') {
        fprintf(stderr, "錯誤: 包含非數字字符\n");
        return -1;
    }

    if (errno == ERANGE || value < INT_MIN || value > INT_MAX) {
        fprintf(stderr, "錯誤: 數字超出範圍\n");
        return -1;
    }

    *result = (int)value;
    return 0;
}

/**
 * 字符串清理 - 移除危險字符
 *
 * @param str: 要清理的字符串
 * @return: 清理後的字符串（原地修改）
 */
char *sanitize_string(char *str) {
    if (str == NULL) {
        return NULL;
    }

    char *src = str;
    char *dst = str;

    while (*src) {
        /* 只保留可打印字符和空格 */
        if (isprint((unsigned char)*src) || *src == ' ') {
            *dst++ = *src;
        }
        src++;
    }

    *dst = '\0';
    return str;
}

/**
 * 危險函數演示（僅用於教學對比）
 */
void demonstrate_危險_functions(void) {
    printf("\n");
    printf("═════════════════════════════════════════════════════════════\n");
    printf(COLOR_RED "  危險函數演示（請不要在實際代碼中使用）" COLOR_RESET "\n");
    printf("═════════════════════════════════════════════════════════════\n");

    printf("\n" COLOR_RED "✗ 危險函數列表:" COLOR_RESET "\n\n");

    printf("1. " COLOR_YELLOW "strcpy(dest, src)" COLOR_RESET "\n");
    printf("   問題: 不檢查 dest 大小，可能溢出\n");
    printf("   範例: char buf[10]; strcpy(buf, very_long_string); // 溢出！\n\n");

    printf("2. " COLOR_YELLOW "strcat(dest, src)" COLOR_RESET "\n");
    printf("   問題: 不檢查 dest 剩餘空間\n");
    printf("   範例: char buf[10]=\"Hi\"; strcat(buf, long_string); // 溢出！\n\n");

    printf("3. " COLOR_YELLOW "gets(buffer)" COLOR_RESET "\n");
    printf("   問題: 無法限制讀取長度，已被廢棄\n");
    printf("   範例: gets(buf); // 攻擊者可以輸入任意長度！\n\n");

    printf("4. " COLOR_YELLOW "sprintf(dest, format, ...)" COLOR_RESET "\n");
    printf("   問題: 不檢查 dest 大小\n");
    printf("   範例: sprintf(buf, \"%%s\", long_string); // 可能溢出\n\n");

    printf("5. " COLOR_YELLOW "scanf(\"%%s\", buffer)" COLOR_RESET "\n");
    printf("   問題: 不限制讀取長度\n");
    printf("   範例: scanf(\"%%s\", buf); // 沒有長度限制\n\n");
}

/**
 * 安全函數演示
 */
void demonstrate_safe_functions(void) {
    printf("\n");
    printf("═════════════════════════════════════════════════════════════\n");
    printf(COLOR_GREEN "  安全函數演示" COLOR_RESET "\n");
    printf("═════════════════════════════════════════════════════════════\n");

    printf("\n" COLOR_GREEN "✓ 安全替代方案:" COLOR_RESET "\n\n");

    /* 1. safe_strcpy 演示 */
    printf("1. " COLOR_CYAN "safe_strcpy() - 替代 strcpy()" COLOR_RESET "\n");
    {
        char buffer[20];
        const char *short_str = "Hello";
        const char *long_str = "This is a very long string that will be truncated";

        int result1 = safe_strcpy(buffer, short_str, sizeof(buffer));
        printf("   短字符串: \"%s\" → \"%s\" (複製 %d 字符)\n",
               short_str, buffer, result1);

        int result2 = safe_strcpy(buffer, long_str, sizeof(buffer));
        printf("   長字符串: \"%s\" → \"%s\" (複製 %d 字符，截斷)\n",
               long_str, buffer, result2);
    }

    /* 2. safe_strcat 演示 */
    printf("\n2. " COLOR_CYAN "safe_strcat() - 替代 strcat()" COLOR_RESET "\n");
    {
        char buffer[30] = "Hello, ";
        int result = safe_strcat(buffer, "World!", sizeof(buffer));
        printf("   連接結果: \"%s\" (長度: %d)\n", buffer, result);

        result = safe_strcat(buffer, " This is a test.", sizeof(buffer));
        printf("   再次連接: \"%s\" (長度: %d)\n", buffer, result);
    }

    /* 3. safe_sprintf 演示 */
    printf("\n3. " COLOR_CYAN "safe_sprintf() - 替代 sprintf()" COLOR_RESET "\n");
    {
        char buffer[50];
        int age = 25;
        const char *name = "Alice";

        int result = safe_sprintf(buffer, sizeof(buffer),
                                   "Name: %s, Age: %d", name, age);
        if (result >= 0) {
            printf("   格式化結果: \"%s\" (寫入 %d 字符)\n", buffer, result);
        }
    }

    /* 4. safe_getline 演示 */
    printf("\n4. " COLOR_CYAN "safe_getline() - 替代 gets()" COLOR_RESET "\n");
    printf("   示例: char buf[128];\n");
    printf("         safe_getline(buf, sizeof(buf), stdin);\n");
    printf("   ✓ 自動處理換行符，防止溢出\n");

    /* 5. safe_read_int 演示 */
    printf("\n5. " COLOR_CYAN "safe_read_int() - 安全讀取整數" COLOR_RESET "\n");
    printf("   示例: int num;\n");
    printf("         safe_read_int(&num, \"輸入數字: \");\n");
    printf("   ✓ 完整的錯誤檢查和範圍驗證\n");
}

/**
 * 實際應用案例
 */
void demonstrate_real_world_usage(void) {
    printf("\n");
    printf("═════════════════════════════════════════════════════════════\n");
    printf(COLOR_CYAN "  實際應用案例" COLOR_RESET "\n");
    printf("═════════════════════════════════════════════════════════════\n");

    printf("\n案例 1: 構建安全的用戶信息字符串\n");
    {
        char result[256] = "";
        const char *username = "john_doe";
        const char *email = "john@example.com";
        int age = 30;

        safe_strcpy(result, "User: ", sizeof(result));
        safe_strcat(result, username, sizeof(result));
        safe_strcat(result, ", Email: ", sizeof(result));
        safe_strcat(result, email, sizeof(result));

        char age_str[20];
        snprintf(age_str, sizeof(age_str), ", Age: %d", age);
        safe_strcat(result, age_str, sizeof(result));

        printf("結果: %s\n", result);
        printf(COLOR_GREEN "✓ 所有操作都有邊界檢查\n" COLOR_RESET);
    }

    printf("\n案例 2: 清理用戶輸入\n");
    {
        char user_input[] = "Hello\x01\x02World\t\n!";  /* 包含控制字符 */
        printf("原始輸入: ");
        for (const char *p = user_input; *p; p++) {
            printf("\\x%02x ", (unsigned char)*p);
        }
        printf("\n");

        sanitize_string(user_input);
        printf("清理後: %s\n", user_input);
        printf(COLOR_GREEN "✓ 危險字符已移除\n" COLOR_RESET);
    }

    printf("\n案例 3: 安全的配置文件解析\n");
    {
        printf("示例代碼:\n");
        printf(COLOR_YELLOW);
        printf("  char key[64], value[256];\n");
        printf("  if (sscanf(line, \"%%63[^=]=%%255[^\\n]\", key, value) == 2) {\n");
        printf("      // 使用限制長度的 scanf，防止溢出\n");
        printf("      sanitize_string(key);\n");
        printf("      sanitize_string(value);\n");
        printf("      // 現在可以安全使用 key 和 value\n");
        printf("  }\n");
        printf(COLOR_RESET);
        printf(COLOR_GREEN "✓ 限制讀取長度，清理輸入\n" COLOR_RESET);
    }
}

/**
 * 性能對比
 */
void performance_comparison(void) {
    printf("\n");
    printf("═════════════════════════════════════════════════════════════\n");
    printf(COLOR_CYAN "  性能考量" COLOR_RESET "\n");
    printf("═════════════════════════════════════════════════════════════\n");

    printf("\n安全函數的開銷:\n");
    printf("  1. 邊界檢查: < 1%% 額外開銷（現代 CPU 分支預測良好）\n");
    printf("  2. strlen() 調用: 可能是主要開銷，但對於安全性是值得的\n");
    printf("  3. 編譯器優化: -O2 可以優化掉大部分開銷\n");

    printf("\n優化建議:\n");
    printf("  ✓ 如果已知長度，直接傳入而不是調用 strlen()\n");
    printf("  ✓ 批量操作時重用緩衝區\n");
    printf("  ✓ 使用編譯器優化選項 (-O2, -O3)\n");
    printf("  ✓ 考慮使用更高效的庫 (如 libbsd 的 strlcpy)\n");

    printf("\n記住: " COLOR_GREEN "安全性 > 微小的性能差異" COLOR_RESET "\n");
}

/**
 * 最佳實踐總結
 */
void best_practices_summary(void) {
    printf("\n");
    printf("═════════════════════════════════════════════════════════════\n");
    printf(COLOR_CYAN "  安全編程最佳實踐" COLOR_RESET "\n");
    printf("═════════════════════════════════════════════════════════════\n");

    printf("\n" COLOR_GREEN "DO (推薦做法):" COLOR_RESET "\n");
    printf("  ✓ 總是檢查緩衝區大小\n");
    printf("  ✓ 使用 strncpy(), strncat(), snprintf(), fgets()\n");
    printf("  ✓ 確保字符串 NULL 終止\n");
    printf("  ✓ 驗證所有輸入\n");
    printf("  ✓ 使用常量表達式表示緩衝區大小: sizeof(buffer)\n");
    printf("  ✓ 檢查函數返回值\n");
    printf("  ✓ 使用靜態分析工具 (clang-tidy, cppcheck)\n");
    printf("  ✓ 啟用編譯器警告 (-Wall -Wextra)\n");

    printf("\n" COLOR_RED "DON'T (避免做法):" COLOR_RESET "\n");
    printf("  ✗ 不要使用 strcpy(), strcat(), gets(), sprintf()\n");
    printf("  ✗ 不要假設輸入長度\n");
    printf("  ✗ 不要忽略編譯器警告\n");
    printf("  ✗ 不要禁用安全機制\n");
    printf("  ✗ 不要使用魔數，使用 sizeof()\n");
    printf("  ✗ 不要信任用戶輸入\n");

    printf("\n" COLOR_CYAN "工具推薦:" COLOR_RESET "\n");
    printf("  • Valgrind: 內存錯誤檢測\n");
    printf("  • AddressSanitizer (-fsanitize=address): 編譯時檢測\n");
    printf("  • Clang Static Analyzer: 靜態分析\n");
    printf("  • Coverity: 商業靜態分析工具\n");
    printf("  • Fuzzing: AFL, libFuzzer (自動化測試)\n");
}

/**
 * 互動測試
 */
void interactive_test(void) {
    printf("\n");
    printf("═════════════════════════════════════════════════════════════\n");
    printf(COLOR_CYAN "  互動測試" COLOR_RESET "\n");
    printf("═════════════════════════════════════════════════════════════\n");

    char buffer[64];
    int number;

    printf("\n測試 1: 安全字符串輸入\n");
    printf("請輸入您的名字 (最多 63 字符): ");
    fflush(stdout);

    if (safe_getline(buffer, sizeof(buffer), stdin) != NULL) {
        sanitize_string(buffer);
        printf("您好, %s!\n", buffer);
    }

    printf("\n測試 2: 安全整數輸入\n");
    if (safe_read_int(&number, "請輸入您的年齡: ") == 0) {
        printf("您輸入的年齡是: %d\n", number);

        if (number < 0 || number > 150) {
            printf(COLOR_YELLOW "警告: 這個年齡看起來不太對勁\n" COLOR_RESET);
        }
    } else {
        printf(COLOR_RED "輸入無效\n" COLOR_RESET);
    }
}

/**
 * 主函數
 */
int main(void) {
    printf("╔═══════════════════════════════════════════════════════════════════╗\n");
    printf("║              安全字符串操作演示                                     ║\n");
    printf("║           Safe String Operations Demonstration                    ║\n");
    printf("╚═══════════════════════════════════════════════════════════════════╝\n");

    /* 1. 危險函數演示 */
    demonstrate_危險_functions();

    /* 2. 安全函數演示 */
    demonstrate_safe_functions();

    /* 3. 實際應用案例 */
    demonstrate_real_world_usage();

    /* 4. 性能對比 */
    performance_comparison();

    /* 5. 最佳實踐 */
    best_practices_summary();

    /* 6. 互動測試 */
    printf("\n");
    printf(COLOR_YELLOW "是否進行互動測試? (y/n): " COLOR_RESET);
    char choice;
    if (scanf(" %c", &choice) == 1 && (choice == 'y' || choice == 'Y')) {
        /* 清空輸入緩衝 */
        while (getchar() != '\n');
        interactive_test();
    }

    /* 總結 */
    printf("\n");
    printf("═════════════════════════════════════════════════════════════\n");
    printf(COLOR_CYAN "  總結" COLOR_RESET "\n");
    printf("═════════════════════════════════════════════════════════════\n");
    printf("\n");
    printf("關鍵點:\n");
    printf("  1. 字符串操作是緩衝區溢出的主要來源\n");
    printf("  2. 始終使用安全的替代函數\n");
    printf("  3. 驗證所有輸入，不要信任用戶\n");
    printf("  4. 使用工具檢測潛在問題\n");
    printf("  5. 安全性的小成本遠小於被攻擊的代價\n");
    printf("\n");
    printf(COLOR_GREEN "記住: 寫安全的代碼是每個程序員的責任！\n" COLOR_RESET);
    printf("\n");

    return 0;
}
