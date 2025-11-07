/*
 * 文件: buffer_overflow_demo.c
 * 目的: 演示緩衝區溢出漏洞的原理和影響 (教育目的)
 *
 * ⚠️ 警告: 此程式故意包含漏洞，僅供教育使用！
 *          不得用於未經授權的系統！
 *
 * 編譯 (禁用所有保護機制，僅用於學習):
 *   gcc -fno-stack-protector -z execstack -no-pie -g -o buffer_overflow_demo buffer_overflow_demo.c
 *
 * 禁用 ASLR (需要 root):
 *   echo 0 | sudo tee /proc/sys/kernel/randomize_va_space
 *
 * 恢復 ASLR:
 *   echo 2 | sudo tee /proc/sys/kernel/randomize_va_space
 *
 * 運行:
 *   ./buffer_overflow_demo
 *
 * 教育目的:
 * - 理解緩衝區溢出如何發生
 * - 觀察返回地址被覆蓋的過程
 * - 學習為什麼需要輸入驗證
 * - 對比安全和不安全的函數
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>

/* ANSI 顏色代碼 */
#define COLOR_RESET   "\x1b[0m"
#define COLOR_RED     "\x1b[31m"
#define COLOR_GREEN   "\x1b[32m"
#define COLOR_YELLOW  "\x1b[33m"
#define COLOR_BLUE    "\x1b[34m"
#define COLOR_MAGENTA "\x1b[35m"
#define COLOR_CYAN    "\x1b[36m"

/* 緩衝區大小 */
#define BUFFER_SIZE 64

/**
 * 打印分隔線
 */
void print_separator(void) {
    printf("\n");
    printf("================================================================================\n");
}

/**
 * 打印危險警告
 */
void print_warning(void) {
    printf(COLOR_RED);
    printf("\n");
    printf("  ⚠️  WARNING: 漏洞函數  ⚠️\n");
    printf("  此函數故意包含緩衝區溢出漏洞，僅供教育使用！\n");
    printf(COLOR_RESET);
}

/**
 * 打印安全提示
 */
void print_safe_notice(void) {
    printf(COLOR_GREEN);
    printf("\n");
    printf("  ✓ SAFE: 安全函數\n");
    printf("  此函數包含適當的邊界檢查\n");
    printf(COLOR_RESET);
}

/**
 * 安全版本: 有邊界檢查的字符串複製
 */
void safe_copy(const char *input) {
    char buffer[BUFFER_SIZE];
    size_t input_len = strlen(input);

    print_safe_notice();

    printf("\n[safe_copy 函數開始]\n");
    printf("  緩衝區大小: %d 字節\n", BUFFER_SIZE);
    printf("  輸入長度: %zu 字節\n", input_len);
    printf("  緩衝區地址: %p\n", (void*)buffer);

    /* 邊界檢查 */
    if (input_len >= BUFFER_SIZE) {
        printf(COLOR_YELLOW);
        printf("\n  ⚠ 輸入太長！將被截斷到 %d 字節\n", BUFFER_SIZE - 1);
        printf(COLOR_RESET);

        /* 安全複製：使用 strncpy */
        strncpy(buffer, input, BUFFER_SIZE - 1);
        buffer[BUFFER_SIZE - 1] = '\0';  /* 確保 NULL 終止 */
    } else {
        /* 輸入長度合適，直接複製 */
        strcpy(buffer, input);
    }

    printf("  複製後的內容: \"%s\"\n", buffer);
    printf(COLOR_GREEN);
    printf("  ✓ 成功: 沒有溢出發生\n");
    printf(COLOR_RESET);
    printf("[safe_copy 函數結束]\n\n");
}

/**
 * 危險版本: 沒有邊界檢查的字符串複製
 * ⚠️ 這是一個故意設計的漏洞函數！
 */
void vulnerable_copy(const char *input) {
    char buffer[BUFFER_SIZE];
    char *ret_addr;

    print_warning();

    printf("\n[vulnerable_copy 函數開始]\n");
    printf("  緩衝區大小: %d 字節\n", BUFFER_SIZE);
    printf("  輸入長度: %zu 字節\n", strlen(input));
    printf("  緩衝區地址: %p\n", (void*)buffer);

    /* 獲取返回地址的位置（近似） */
    #if defined(__x86_64__)
        /* 64 位: RBP + 8 是返回地址 */
        __asm__ __volatile__("movq 8(%%rbp), %0" : "=r"(ret_addr));
    #elif defined(__i386__)
        /* 32 位: EBP + 4 是返回地址 */
        __asm__ __volatile__("movl 4(%%ebp), %0" : "=r"(ret_addr));
    #else
        ret_addr = NULL;
    #endif

    if (ret_addr) {
        printf("  返回地址位置: %p\n", (void*)ret_addr);
        printf("  返回地址內容: %p\n", *(void**)ret_addr);
    }

    /* 危險！沒有邊界檢查 */
    printf(COLOR_RED);
    printf("\n  ⚠ 執行 strcpy() - 沒有邊界檢查！\n");
    printf(COLOR_RESET);

    strcpy(buffer, input);  /* 這裡是漏洞所在！ */

    printf("  複製後的內容: \"%s\"\n", buffer);

    /* 檢查是否發生溢出 */
    if (strlen(input) >= BUFFER_SIZE) {
        printf(COLOR_RED);
        printf("\n  ⚠ 危險: 緩衝區溢出發生！\n");
        printf("  溢出字節數: %zu 字節\n", strlen(input) - BUFFER_SIZE + 1);
        printf(COLOR_RESET);
    }

    printf("[vulnerable_copy 函數結束]\n\n");
}

/**
 * 顯示內存內容的輔助函數
 */
void display_memory(const void *addr, size_t size, const char *label) {
    const unsigned char *p = (const unsigned char *)addr;

    printf("\n%s (地址: %p, %zu 字節):\n", label, addr, size);
    printf("  ");

    for (size_t i = 0; i < size; i++) {
        printf("%02x ", p[i]);
        if ((i + 1) % 16 == 0) {
            printf("\n  ");
        }
    }
    printf("\n");
}

/**
 * 演示1: 正常輸入（不會溢出）
 */
void demo_normal_input(void) {
    print_separator();
    printf(COLOR_CYAN "演示 1: 正常輸入（不會溢出）" COLOR_RESET "\n");
    print_separator();

    const char *input = "Hello, World!";
    printf("\n輸入: \"%s\" (長度: %zu)\n", input, strlen(input));

    printf("\n--- 安全版本 ---\n");
    safe_copy(input);

    printf("--- 危險版本 ---\n");
    vulnerable_copy(input);

    printf(COLOR_GREEN "✓ 結果: 兩個版本都正常工作\n" COLOR_RESET);
}

/**
 * 演示2: 邊界輸入（剛好填滿緩衝區）
 */
void demo_boundary_input(void) {
    print_separator();
    printf(COLOR_CYAN "演示 2: 邊界輸入（剛好填滿緩衝區）" COLOR_RESET "\n");
    print_separator();

    char input[BUFFER_SIZE];
    memset(input, 'A', BUFFER_SIZE - 1);
    input[BUFFER_SIZE - 1] = '\0';

    printf("\n輸入: %d 個 'A' (長度: %zu)\n", BUFFER_SIZE - 1, strlen(input));

    printf("\n--- 安全版本 ---\n");
    safe_copy(input);

    printf("--- 危險版本 ---\n");
    vulnerable_copy(input);

    printf(COLOR_YELLOW "⚠ 注意: 邊界情況，兩個版本仍然安全\n" COLOR_RESET);
}

/**
 * 演示3: 輕微溢出
 */
void demo_small_overflow(void) {
    print_separator();
    printf(COLOR_CYAN "演示 3: 輕微溢出（超出 10 字節）" COLOR_RESET "\n");
    print_separator();

    char input[BUFFER_SIZE + 10 + 1];
    memset(input, 'B', BUFFER_SIZE + 10);
    input[BUFFER_SIZE + 10] = '\0';

    printf("\n輸入: %d 個 'B' (長度: %zu)\n", BUFFER_SIZE + 10, strlen(input));
    printf("超出緩衝區: %d 字節\n", 10);

    printf("\n--- 安全版本 ---\n");
    safe_copy(input);

    printf("--- 危險版本 ---\n");
    printf(COLOR_RED);
    printf("⚠️ 警告: 即將發生緩衝區溢出！\n");
    printf(COLOR_RESET);

    vulnerable_copy(input);

    printf(COLOR_RED "✗ 結果: 危險版本發生溢出！" COLOR_RESET "\n");
}

/**
 * 演示4: 嚴重溢出（可能覆蓋返回地址）
 */
void demo_severe_overflow(void) {
    print_separator();
    printf(COLOR_CYAN "演示 4: 嚴重溢出（可能覆蓋返回地址）" COLOR_RESET "\n");
    print_separator();

    char input[BUFFER_SIZE + 64 + 1];
    memset(input, 'C', BUFFER_SIZE + 64);
    input[BUFFER_SIZE + 64] = '\0';

    printf("\n輸入: %d 個 'C' (長度: %zu)\n", BUFFER_SIZE + 64, strlen(input));
    printf("超出緩衝區: %d 字節\n", 64);

    printf(COLOR_RED);
    printf("\n⚠️⚠️⚠️ 極度危險: 可能覆蓋返回地址和其他重要數據！⚠️⚠️⚠️\n");
    printf(COLOR_RESET);

    printf("\n--- 安全版本 ---\n");
    safe_copy(input);

    printf("--- 危險版本 ---\n");
    printf(COLOR_RED);
    printf("⚠️ 嚴重警告: 即將發生嚴重緩衝區溢出！\n");
    printf("   這可能導致程式崩潰或安全漏洞！\n");
    printf(COLOR_RESET);

    /* 注意: 這裡可能導致程式崩潰 */
    vulnerable_copy(input);

    printf(COLOR_RED "✗ 結果: 如果你看到這條消息，說明程式還沒崩潰（很幸運）\n" COLOR_RESET);
}

/**
 * 展示棧結構
 */
void show_stack_structure(void) {
    print_separator();
    printf(COLOR_CYAN "棧結構可視化" COLOR_RESET "\n");
    print_separator();

    printf("\n函數調用時的棧布局:\n\n");
    printf("  高地址\n");
    printf("  +------------------+\n");
    printf("  | 函數參數          |\n");
    printf("  +------------------+\n");
    printf("  | 返回地址          | ← 如果被覆蓋，程式會跳到錯誤地址！\n");
    printf("  +==================+\n");
    printf("  | 舊 EBP/RBP       |\n");
    printf("  +------------------+\n");
    printf("  | 局部變量          |\n");
    printf("  | buffer[63]       |\n");
    printf("  | ...              |\n");
    printf("  | buffer[0]        | ← strcpy() 從這裡開始寫入\n");
    printf("  +------------------+ ← 如果輸入太長，會向上覆蓋！\n");
    printf("  | ...              |\n");
    printf("  低地址\n");

    printf("\n緩衝區溢出過程:\n");
    printf("  1. buffer[0..63] ← 正常寫入 64 字節\n");
    printf("  2. buffer[64+]   ← 溢出！開始覆蓋其他數據\n");
    printf("  3. 舊 EBP        ← 可能被覆蓋\n");
    printf("  4. 返回地址       ← 如果到這裡被覆蓋，攻擊者可控制程式流程！\n");
}

/**
 * 展示防禦機制
 */
void show_defense_mechanisms(void) {
    print_separator();
    printf(COLOR_CYAN "防禦機制說明" COLOR_RESET "\n");
    print_separator();

    printf("\n現代編譯器和操作系統提供多層防禦:\n\n");

    printf(COLOR_GREEN "1. Stack Canary (金絲雀值)" COLOR_RESET "\n");
    printf("   - 在返回地址前放置隨機值\n");
    printf("   - 函數返回前檢查這個值\n");
    printf("   - 編譯選項: -fstack-protector\n\n");

    printf(COLOR_GREEN "2. ASLR (地址空間布局隨機化)" COLOR_RESET "\n");
    printf("   - 每次運行時隨機化內存地址\n");
    printf("   - 使攻擊者難以預測目標地址\n");
    printf("   - 系統設置: /proc/sys/kernel/randomize_va_space\n\n");

    printf(COLOR_GREEN "3. NX/DEP (不可執行棧)" COLOR_RESET "\n");
    printf("   - 將棧標記為不可執行\n");
    printf("   - 防止執行注入的代碼\n");
    printf("   - 編譯選項: -z noexecstack (默認)\n\n");

    printf(COLOR_GREEN "4. PIE (位置無關可執行)" COLOR_RESET "\n");
    printf("   - 可執行文件本身也隨機化\n");
    printf("   - 編譯選項: -pie -fPIE\n\n");

    printf(COLOR_GREEN "5. FORTIFY_SOURCE" COLOR_RESET "\n");
    printf("   - 編譯時檢查緩衝區大小\n");
    printf("   - 編譯選項: -D_FORTIFY_SOURCE=2\n\n");
}

/**
 * 安全編程建議
 */
void show_secure_coding_tips(void) {
    print_separator();
    printf(COLOR_CYAN "安全編程建議" COLOR_RESET "\n");
    print_separator();

    printf("\n" COLOR_GREEN "✓ 推薦做法:" COLOR_RESET "\n");
    printf("  1. 使用安全函數: strncpy(), snprintf(), fgets()\n");
    printf("  2. 總是檢查輸入長度\n");
    printf("  3. 確保字符串 NULL 終止\n");
    printf("  4. 使用邊界檢查工具 (valgrind, AddressSanitizer)\n");
    printf("  5. 啟用所有編譯器保護機制\n\n");

    printf(COLOR_RED "✗ 避免做法:" COLOR_RESET "\n");
    printf("  1. 避免使用 strcpy(), strcat(), sprintf(), gets()\n");
    printf("  2. 不要假設輸入長度\n");
    printf("  3. 不要禁用安全機制（除非教學需要）\n");
    printf("  4. 不要信任用戶輸入\n");
    printf("  5. 不要在生產環境使用不安全代碼\n");
}

/**
 * 主函數
 */
int main(void) {
    printf("╔════════════════════════════════════════════════════════════════════════════╗\n");
    printf("║                       緩衝區溢出漏洞演示 (教育目的)                          ║\n");
    printf("║                    Buffer Overflow Demonstration                          ║\n");
    printf("╚════════════════════════════════════════════════════════════════════════════╝\n");

    printf(COLOR_YELLOW);
    printf("\n⚠️  重要提示:\n");
    printf("   本程式僅供教育目的，用於理解緩衝區溢出的原理\n");
    printf("   請在隔離的測試環境中運行\n");
    printf("   不得用於攻擊未經授權的系統！\n");
    printf(COLOR_RESET);

    /* 顯示棧結構 */
    show_stack_structure();

    /* 演示1: 正常輸入 */
    demo_normal_input();

    /* 演示2: 邊界輸入 */
    demo_boundary_input();

    /* 演示3: 輕微溢出 */
    demo_small_overflow();

    /* 詢問是否繼續嚴重溢出演示 */
    print_separator();
    printf(COLOR_YELLOW);
    printf("\n下一個演示可能導致程式崩潰！\n");
    printf("是否繼續? (y/n): ");
    printf(COLOR_RESET);

    char choice;
    scanf(" %c", &choice);

    if (choice == 'y' || choice == 'Y') {
        /* 演示4: 嚴重溢出 */
        demo_severe_overflow();
    } else {
        printf("\n跳過嚴重溢出演示。\n");
    }

    /* 顯示防禦機制 */
    show_defense_mechanisms();

    /* 顯示安全編程建議 */
    show_secure_coding_tips();

    /* 總結 */
    print_separator();
    printf(COLOR_CYAN "總結" COLOR_RESET "\n");
    print_separator();
    printf("\n");
    printf("1. 緩衝區溢出是最常見的安全漏洞之一\n");
    printf("2. 它可以導致程式崩潰、數據損壞或任意代碼執行\n");
    printf("3. 攻擊者可以利用它來控制程式流程\n");
    printf("4. 現代系統有多層防禦機制\n");
    printf("5. 安全編程實踐是最好的防禦\n");
    printf("\n");
    printf(COLOR_GREEN);
    printf("記住: 永遠不要信任用戶輸入，總是進行邊界檢查！\n");
    printf(COLOR_RESET);

    print_separator();

    return 0;
}
