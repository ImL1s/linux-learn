/*
 * 文件: canary_demo.c
 * 目的: 演示 Stack Canary (金絲雀值) 防護機制
 *
 * 編譯對比:
 *   無保護: gcc -fno-stack-protector -o canary_demo_unsafe canary_demo.c
 *   有保護: gcc -fstack-protector-all -o canary_demo_safe canary_demo.c
 *
 * 運行:
 *   ./canary_demo_safe
 *   ./canary_demo_unsafe
 *
 * 教育目的:
 * - 理解 Stack Canary 如何工作
 * - 觀察 canary 檢測溢出的過程
 * - 對比有無保護的區別
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <time.h>

#define COLOR_RESET   "\x1b[0m"
#define COLOR_RED     "\x1b[31m"
#define COLOR_GREEN   "\x1b[32m"
#define COLOR_YELLOW  "\x1b[33m"
#define COLOR_CYAN    "\x1b[36m"

#define BUFFER_SIZE 64
#define CANARY_VALUE 0xDEADBEEF

/**
 * 全局 canary 值（模擬 GCC 的做法）
 */
static uint32_t global_canary = CANARY_VALUE;

/**
 * Canary 檢測失敗處理
 */
void __attribute__((noreturn)) canary_fail_handler(void) {
    fprintf(stderr, "\n");
    fprintf(stderr, COLOR_RED "╔═══════════════════════════════════════════════════════╗\n");
    fprintf(stderr, "║  *** STACK CANARY CORRUPTION DETECTED ***            ║\n");
    fprintf(stderr, "║                                                       ║\n");
    fprintf(stderr, "║  棧金絲雀值被破壞！                                     ║\n");
    fprintf(stderr, "║  檢測到緩衝區溢出攻擊！                                 ║\n");
    fprintf(stderr, "║  程式將終止以防止安全漏洞                               ║\n");
    fprintf(stderr, "╚═══════════════════════════════════════════════════════╝\n" COLOR_RESET);
    fprintf(stderr, "\n");
    abort();
}

/**
 * 手動實現的 Canary 保護函數
 */
void manual_canary_function(const char *input) {
    uint32_t canary = global_canary;  /* 在棧上放置 canary */
    char buffer[BUFFER_SIZE];
    uint32_t canary_copy = canary;    /* 保存 canary 副本 */

    printf("\n" COLOR_CYAN "[手動 Canary 保護函數]" COLOR_RESET "\n");
    printf("  緩衝區地址: %p\n", (void*)buffer);
    printf("  Canary 地址: %p\n", (void*)&canary);
    printf("  Canary 值: 0x%08x\n", canary);
    printf("  輸入長度: %zu 字節\n", strlen(input));

    /* 執行可能導致溢出的操作 */
    printf("\n  執行 strcpy()...\n");
    strcpy(buffer, input);  /* 可能溢出！ */

    /* 檢查 canary 是否被修改 */
    printf("\n  檢查 Canary...\n");
    printf("  原始值: 0x%08x\n", canary_copy);
    printf("  當前值: 0x%08x\n", canary);

    if (canary != canary_copy) {
        printf(COLOR_RED "  ✗ Canary 被破壞！" COLOR_RESET "\n");
        printf("  差異: 0x%08x XOR 0x%08x = 0x%08x\n",
               canary_copy, canary, canary_copy ^ canary);
        canary_fail_handler();
    } else {
        printf(COLOR_GREEN "  ✓ Canary 完整，沒有溢出" COLOR_RESET "\n");
    }

    printf("  緩衝區內容: \"%.64s\"\n", buffer);
}

/**
 * 沒有保護的函數（用於對比）
 */
void unprotected_function(const char *input) {
    char buffer[BUFFER_SIZE];

    printf("\n" COLOR_YELLOW "[無保護函數]" COLOR_RESET "\n");
    printf("  緩衝區地址: %p\n", (void*)buffer);
    printf("  輸入長度: %zu 字節\n", strlen(input));

    printf("\n  執行 strcpy()...\n");
    strcpy(buffer, input);

    printf(COLOR_YELLOW "  ⚠ 沒有 Canary 檢查！" COLOR_RESET "\n");
    printf("  緩衝區內容: \"%.64s\"\n", buffer);
}

/**
 * 演示正常輸入（不溢出）
 */
void demo_normal(void) {
    printf("\n");
    printf("═══════════════════════════════════════════════════════\n");
    printf("  演示 1: 正常輸入（不會溢出）\n");
    printf("═══════════════════════════════════════════════════════\n");

    const char *input = "Hello, Canary Protection!";
    printf("\n輸入: \"%s\" (%zu 字節)\n", input, strlen(input));

    manual_canary_function(input);
}

/**
 * 演示小溢出（不影響 canary）
 */
void demo_small_overflow(void) {
    printf("\n");
    printf("═══════════════════════════════════════════════════════\n");
    printf("  演示 2: 小溢出（可能不影響 Canary）\n");
    printf("═══════════════════════════════════════════════════════\n");

    char input[BUFFER_SIZE + 1];
    memset(input, 'A', BUFFER_SIZE);
    input[BUFFER_SIZE] = '\0';

    printf("\n輸入: %d 個 'A' (%zu 字節)\n", BUFFER_SIZE, strlen(input));

    manual_canary_function(input);
}

/**
 * 演示 Canary 被破壞
 */
void demo_canary_corruption(void) {
    printf("\n");
    printf("═══════════════════════════════════════════════════════\n");
    printf("  演示 3: Canary 被破壞（會被檢測到）\n");
    printf("═══════════════════════════════════════════════════════\n");

    char input[BUFFER_SIZE + 20];
    memset(input, 'B', BUFFER_SIZE + 19);
    input[BUFFER_SIZE + 19] = '\0';

    printf("\n輸入: %d 個 'B' (%zu 字節)\n", BUFFER_SIZE + 19, strlen(input));
    printf(COLOR_RED "⚠️  這會覆蓋 Canary 值！\n" COLOR_RESET);

    manual_canary_function(input);
}

/**
 * 展示 GCC Stack Protector 的工作原理
 */
void explain_gcc_canary(void) {
    printf("\n");
    printf("═══════════════════════════════════════════════════════\n");
    printf("  GCC Stack Protector 原理\n");
    printf("═══════════════════════════════════════════════════════\n");

    printf("\n" COLOR_CYAN "1. 編譯時插入代碼:" COLOR_RESET "\n");
    printf("   函數開始:\n");
    printf("     mov    canary_value, %%rax\n");
    printf("     mov    %%rax, -8(%%rbp)        # 將 canary 放在棧上\n");
    printf("\n");
    printf("   函數結束前:\n");
    printf("     mov    -8(%%rbp), %%rax        # 讀取 canary\n");
    printf("     xor    canary_value, %%rax    # 與原始值比較\n");
    printf("     je     .L_ok                  # 相同則跳轉\n");
    printf("     call   __stack_chk_fail       # 不同則調用失敗處理\n");
    printf("   .L_ok:\n");
    printf("     leave\n");
    printf("     ret\n");

    printf("\n" COLOR_CYAN "2. Canary 值來源:" COLOR_RESET "\n");
    printf("   - 程式啟動時生成隨機值\n");
    printf("   - 存儲在 TLS (Thread-Local Storage)\n");
    printf("   - 每個線程有獨立的 canary\n");
    printf("   - 通常從 /dev/urandom 讀取\n");

    printf("\n" COLOR_CYAN "3. 棧布局（有 Canary）:" COLOR_RESET "\n");
    printf("   高地址\n");
    printf("   +------------------+\n");
    printf("   | 返回地址          |\n");
    printf("   +------------------+\n");
    printf("   | 舊 RBP           |\n");
    printf("   +------------------+\n");
    printf("   | Canary (8字節)   | ← 金絲雀值\n");
    printf("   +------------------+\n");
    printf("   | 局部變量          |\n");
    printf("   | buffer[...]      |\n");
    printf("   +------------------+\n");
    printf("   低地址\n");

    printf("\n" COLOR_CYAN "4. 編譯選項:" COLOR_RESET "\n");
    printf("   -fstack-protector          # 保護有緩衝區的函數\n");
    printf("   -fstack-protector-strong   # 保護更多函數\n");
    printf("   -fstack-protector-all      # 保護所有函數\n");
    printf("   -fno-stack-protector       # 禁用（不安全！）\n");
}

/**
 * Canary 的局限性
 */
void explain_limitations(void) {
    printf("\n");
    printf("═══════════════════════════════════════════════════════\n");
    printf("  Canary 防護的局限性\n");
    printf("═══════════════════════════════════════════════════════\n");

    printf("\n" COLOR_YELLOW "1. 可能被繞過的情況:" COLOR_RESET "\n");
    printf("   ✗ Canary 值泄露（通過格式化字符串漏洞等）\n");
    printf("   ✗ 覆蓋局部變量而不觸及 canary\n");
    printf("   ✗ 堆溢出（canary 只保護棧）\n");
    printf("   ✗ 整數溢出導致的問題\n");
    printf("   ✗ 使用 Canary 前的代碼漏洞\n");

    printf("\n" COLOR_YELLOW "2. 性能開銷:" COLOR_RESET "\n");
    printf("   - 每個函數額外的指令\n");
    printf("   - 增加棧空間使用\n");
    printf("   - 通常 < 5%% 性能影響\n");

    printf("\n" COLOR_GREEN "3. 最佳實踐:" COLOR_RESET "\n");
    printf("   ✓ 結合其他防護機制（ASLR, NX, PIE）\n");
    printf("   ✓ 使用安全的庫函數\n");
    printf("   ✓ 進行代碼審計\n");
    printf("   ✓ 使用靜態分析工具\n");
    printf("   ✓ 輸入驗證和邊界檢查\n");
}

/**
 * 實際案例：檢測真實的溢出
 */
void real_world_example(void) {
    printf("\n");
    printf("═══════════════════════════════════════════════════════\n");
    printf("  實際案例：編譯器 Canary 檢測\n");
    printf("═══════════════════════════════════════════════════════\n");

    printf("\n提示: 使用不同編譯選項重新編譯此程式來觀察區別:\n\n");

    printf(COLOR_GREEN "有保護版本:" COLOR_RESET "\n");
    printf("  gcc -fstack-protector-all -o canary_demo_safe canary_demo.c\n");
    printf("  ./canary_demo_safe\n");
    printf("  → 溢出會被檢測到，程式會終止\n\n");

    printf(COLOR_RED "無保護版本:" COLOR_RESET "\n");
    printf("  gcc -fno-stack-protector -o canary_demo_unsafe canary_demo.c\n");
    printf("  ./canary_demo_unsafe\n");
    printf("  → 溢出不會被檢測，可能導致崩潰或被利用\n\n");

    printf("對比測試:\n");
    printf("  echo \"AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA");
    printf("AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA\" | ./canary_demo_safe\n");
    printf("  → 應該看到 stack smashing detected 錯誤\n");
}

/**
 * 檢查當前程式的保護狀態
 */
void check_protection_status(void) {
    printf("\n");
    printf("═══════════════════════════════════════════════════════\n");
    printf("  當前程式保護狀態\n");
    printf("═══════════════════════════════════════════════════════\n");

    printf("\n檢查方法:\n");
    printf("  1. 使用 checksec:\n");
    printf("     checksec --file=./canary_demo\n");
    printf("\n");
    printf("  2. 使用 readelf:\n");
    printf("     readelf -s ./canary_demo | grep stack_chk\n");
    printf("\n");
    printf("  3. 使用 objdump:\n");
    printf("     objdump -d ./canary_demo | grep stack_chk\n");
    printf("\n");

    #ifdef __SSP__
        printf(COLOR_GREEN "✓ 此程式編譯時啟用了 Stack Protector\n" COLOR_RESET);
    #else
        printf(COLOR_RED "✗ 此程式編譯時未啟用 Stack Protector\n" COLOR_RESET);
    #endif

    #ifdef __SSP_STRONG__
        printf(COLOR_GREEN "✓ 使用了 -fstack-protector-strong\n" COLOR_RESET);
    #endif

    #ifdef __SSP_ALL__
        printf(COLOR_GREEN "✓ 使用了 -fstack-protector-all\n" COLOR_RESET);
    #endif
}

/**
 * 主函數
 */
int main(void) {
    printf("╔═══════════════════════════════════════════════════════════════════╗\n");
    printf("║             Stack Canary 防護機制演示 (教育目的)                   ║\n");
    printf("║             Stack Canary Protection Demonstration                ║\n");
    printf("╚═══════════════════════════════════════════════════════════════════╝\n");

    /* 檢查保護狀態 */
    check_protection_status();

    /* 解釋 GCC Canary */
    explain_gcc_canary();

    /* 演示 1: 正常輸入 */
    demo_normal();

    /* 演示 2: 小溢出 */
    demo_small_overflow();

    /* 詢問是否繼續破壞性測試 */
    printf("\n");
    printf(COLOR_YELLOW);
    printf("下一個演示會故意破壞 Canary，這會導致程式終止！\n");
    printf("是否繼續? (y/n): ");
    printf(COLOR_RESET);

    char choice;
    if (scanf(" %c", &choice) == 1 && (choice == 'y' || choice == 'Y')) {
        /* 演示 3: Canary 被破壞 */
        demo_canary_corruption();
        /* 注意: 如果到達這裡，說明 canary 沒有工作（可能禁用了保護） */
    } else {
        printf("\n跳過破壞性測試。\n");
    }

    /* 解釋局限性 */
    explain_limitations();

    /* 實際案例 */
    real_world_example();

    /* 總結 */
    printf("\n");
    printf("═══════════════════════════════════════════════════════\n");
    printf("  總結\n");
    printf("═══════════════════════════════════════════════════════\n");
    printf("\n");
    printf("關鍵點:\n");
    printf("  1. Canary 是一種運行時檢測機制\n");
    printf("  2. 它在函數返回前檢查棧完整性\n");
    printf("  3. 可以有效防止簡單的棧溢出攻擊\n");
    printf("  4. 但不是萬能的，需要結合其他防護\n");
    printf("  5. 現代編譯器默認啟用此保護\n");
    printf("\n");
    printf(COLOR_GREEN "記住: Canary 是多層防禦的一部分！\n" COLOR_RESET);
    printf("\n");

    return 0;
}
