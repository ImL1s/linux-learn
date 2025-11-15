/*
 * 文件: stack_layout.c
 * 目的: 展示 Linux 進程的棧結構和內存布局
 *
 * 編譯: gcc -o stack_layout stack_layout.c
 * 運行: ./stack_layout
 *
 * 教育目的：
 * - 理解棧的增長方向
 * - 觀察變量在內存中的位置
 * - 理解函數調用時的棧變化
 * - 觀察不同類型變量的地址分布
 */

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <stddef.h>

/* 全局變量 - 存儲在 .data 段 */
int g_initialized = 42;

/* 未初始化全局變量 - 存儲在 .bss 段 */
int g_uninitialized;

/* 靜態變量 - 存儲在 .data 段 */
static int s_static_var = 100;

/* 函數前向聲明 */
void level_1(int arg1, int arg2);
void level_2(int arg1, int arg2);
void level_3(int arg1, int arg2);
void analyze_stack_growth(void);
void display_memory_layout(void);

/**
 * 打印地址的輔助函數
 */
void print_address(const char *desc, void *addr) {
    printf("  %-30s: %p\n", desc, addr);
}

/**
 * 打印分隔線
 */
void print_separator(void) {
    printf("\n");
    printf("========================================");
    printf("========================================\n");
}

/**
 * level_3: 最深層的函數調用
 */
void level_3(int arg1, int arg2) {
    int local_var = 300;
    char buffer[64];

    printf("\n[Level 3 函數]:\n");
    print_address("參數 arg1 地址", &arg1);
    print_address("參數 arg2 地址", &arg2);
    print_address("局部變量 local_var 地址", &local_var);
    print_address("局部數組 buffer 地址", buffer);

    printf("\n  棧深度: 3 層\n");
}

/**
 * level_2: 中間層的函數調用
 */
void level_2(int arg1, int arg2) {
    int local_var = 200;
    char small_buffer[16];

    printf("\n[Level 2 函數]:\n");
    print_address("參數 arg1 地址", &arg1);
    print_address("參數 arg2 地址", &arg2);
    print_address("局部變量 local_var 地址", &local_var);
    print_address("局部數組 small_buffer 地址", small_buffer);

    /* 調用下一層函數 */
    level_3(arg1 + 1, arg2 + 1);

    printf("\n[返回到 Level 2]\n");
}

/**
 * level_1: 第一層函數調用
 */
void level_1(int arg1, int arg2) {
    int local_var = 100;
    int another_var = 999;
    char buffer[32];

    printf("\n[Level 1 函數]:\n");
    print_address("參數 arg1 地址", &arg1);
    print_address("參數 arg2 地址", &arg2);
    print_address("局部變量 local_var 地址", &local_var);
    print_address("局部變量 another_var 地址", &another_var);
    print_address("局部數組 buffer 地址", buffer);

    /* 調用下一層函數 */
    level_2(arg1 + 1, arg2 + 1);

    printf("\n[返回到 Level 1]\n");
}

/**
 * 分析棧的增長方向
 * 通過比較不同層次函數中局部變量的地址來確定棧的增長方向
 */
void analyze_stack_growth(void) {
    int var1 = 1;
    int var2 = 2;
    int var3 = 3;

    print_separator();
    printf("棧增長方向分析\n");
    print_separator();

    printf("\n連續聲明的局部變量:\n");
    printf("  int var1 (第一個聲明)  : %p  值: %d\n", (void*)&var1, var1);
    printf("  int var2 (第二個聲明)  : %p  值: %d\n", (void*)&var2, var2);
    printf("  int var3 (第三個聲明)  : %p  值: %d\n", (void*)&var3, var3);

    /* 計算地址差異 */
    ptrdiff_t diff_1_2 = (char*)&var1 - (char*)&var2;
    ptrdiff_t diff_2_3 = (char*)&var2 - (char*)&var3;

    printf("\n地址差異:\n");
    printf("  var1 - var2 = %td 字節\n", diff_1_2);
    printf("  var2 - var3 = %td 字節\n", diff_2_3);

    if (diff_1_2 > 0) {
        printf("\n✓ 棧向低地址增長 (var1 地址 > var2 地址)\n");
        printf("  這是 x86/x64 架構的典型特徵\n");
    } else {
        printf("\n✓ 棧向高地址增長 (罕見)\n");
    }
}

/**
 * 顯示完整的內存布局
 */
void display_memory_layout(void) {
    int stack_var = 123;           /* 棧變量 */
    int *heap_var = malloc(sizeof(int));  /* 堆變量 */
    static int static_local = 456; /* 靜態局部變量 */

    if (heap_var == NULL) {
        fprintf(stderr, "內存分配失敗\n");
        return;
    }

    *heap_var = 789;

    print_separator();
    printf("完整內存布局 (從低地址到高地址)\n");
    print_separator();

    /* 收集所有地址 */
    typedef struct {
        const char *name;
        void *addr;
        const char *segment;
    } MemRegion;

    /* 教育目的：演示棧地址布局（抑制 array-bounds 警告）*/
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Warray-bounds"
    MemRegion regions[] = {
        {"程式碼 (display_memory_layout 函數)", (void*)display_memory_layout, "TEXT段"},
        {"初始化全局變量 g_initialized", &g_initialized, "DATA段"},
        {"靜態變量 s_static_var", &s_static_var, "DATA段"},
        {"未初始化全局變量 g_uninitialized", &g_uninitialized, "BSS段"},
        {"靜態局部變量 static_local", &static_local, "DATA段"},
        {"堆分配變量 *heap_var", heap_var, "HEAP段"},
        {"棧變量 stack_var", &stack_var, "STACK段"},
        {"函數參數區域 (近似)", ((char*)&stack_var + 100), "STACK段"},
    };
#pragma GCC diagnostic pop

    int count = sizeof(regions) / sizeof(regions[0]);

    /* 按地址排序（冒泡排序） */
    for (int i = 0; i < count - 1; i++) {
        for (int j = 0; j < count - i - 1; j++) {
            if (regions[j].addr > regions[j+1].addr) {
                MemRegion temp = regions[j];
                regions[j] = regions[j+1];
                regions[j+1] = temp;
            }
        }
    }

    /* 打印排序後的結果 */
    printf("\n%-40s %-18s %s\n", "區域", "地址", "段");
    printf("--------------------------------------------------------------------------------\n");

    for (int i = 0; i < count; i++) {
        printf("%-40s %p    %s\n",
               regions[i].name,
               regions[i].addr,
               regions[i].segment);
    }

    printf("\n");
    printf("說明:\n");
    printf("  TEXT段: 程式碼段，只讀，包含可執行指令\n");
    printf("  DATA段: 已初始化數據段，包含初始化的全局和靜態變量\n");
    printf("  BSS段:  未初始化數據段，包含未初始化的全局變量\n");
    printf("  HEAP段: 動態分配的內存 (malloc, new 等)\n");
    printf("  STACK段: 函數調用棧，包含局部變量和函數調用信息\n");

    /* 釋放堆內存 */
    free(heap_var);
}

/**
 * 展示棧幀結構
 */
void demonstrate_stack_frame(void) {
    /* 獲取當前棧指針（近似） */
    void *sp;
    #if defined(__x86_64__)
        __asm__ __volatile__("movq %%rsp, %0" : "=r"(sp));
        printf("  當前 RSP (棧指針): %p\n", sp);
    #elif defined(__i386__)
        __asm__ __volatile__("movl %%esp, %0" : "=r"(sp));
        printf("  當前 ESP (棧指針): %p\n", sp);
    #else
        sp = (void*)&sp;  /* 近似棧指針位置 */
        printf("  當前棧指針 (近似): %p\n", sp);
    #endif
}

/**
 * 展示不同大小緩衝區的內存對齊
 */
void demonstrate_alignment(void) {
    char c1;
    int i1;
    long l1;
    double d1;
    char c2;

    print_separator();
    printf("內存對齊演示\n");
    print_separator();

    printf("\n變量地址:\n");
    print_address("char c1", &c1);
    print_address("int i1", &i1);
    print_address("long l1", &l1);
    print_address("double d1", &d1);
    print_address("char c2", &c2);

    printf("\n變量大小:\n");
    printf("  sizeof(char)   = %zu 字節\n", sizeof(char));
    printf("  sizeof(int)    = %zu 字節\n", sizeof(int));
    printf("  sizeof(long)   = %zu 字節\n", sizeof(long));
    printf("  sizeof(double) = %zu 字節\n", sizeof(double));

    printf("\n說明:\n");
    printf("  編譯器會進行內存對齊以提高訪問效率\n");
    printf("  不同類型的變量可能有對齊要求 (alignment requirement)\n");
}

/**
 * 主函數
 */
int main(int argc, char *argv[]) {
    /* 抑制未使用參數警告 */
    (void)argc;
    (void)argv;

    printf("╔════════════════════════════════════════════════════════════════════════════╗\n");
    printf("║                     Linux 棧結構與內存布局演示                              ║\n");
    printf("║                      Stack Layout Demonstration                           ║\n");
    printf("╚════════════════════════════════════════════════════════════════════════════╝\n");

    /* 1. 顯示完整內存布局 */
    display_memory_layout();

    /* 2. 分析棧增長方向 */
    analyze_stack_growth();

    /* 3. 多層函數調用演示 */
    print_separator();
    printf("多層函數調用棧幀演示\n");
    print_separator();

    printf("\n開始多層函數調用...\n");
    printf("\n[Main 函數]:\n");
    printf("  準備調用 level_1(10, 20)\n");

    level_1(10, 20);

    printf("\n[返回到 Main]\n");
    printf("  所有函數調用完成\n");

    /* 4. 展示棧幀結構 */
    print_separator();
    printf("棧幀結構信息\n");
    print_separator();
    printf("\n");
    demonstrate_stack_frame();

    /* 5. 內存對齊演示 */
    demonstrate_alignment();

    /* 總結 */
    print_separator();
    printf("關鍵要點總結\n");
    print_separator();
    printf("\n");
    printf("1. 棧向低地址增長 (在 x86/x64 架構上)\n");
    printf("2. 每次函數調用都會創建新的棧幀\n");
    printf("3. 棧幀包含: 返回地址、舊 EBP、局部變量、函數參數\n");
    printf("4. 局部變量按聲明順序在棧上分配 (可能因優化而變化)\n");
    printf("5. 不同內存段有不同用途: TEXT, DATA, BSS, HEAP, STACK\n");
    printf("6. 編譯器會進行內存對齊以提高效率\n");
    printf("\n");
    printf("安全提示:\n");
    printf("  ⚠ 理解棧結構對於理解緩衝區溢出漏洞至關重要\n");
    printf("  ⚠ 棧上的返回地址可以被溢出的緩衝區覆蓋\n");
    printf("  ⚠ 這就是為什麼需要 Stack Canary、ASLR、NX 等保護機制\n");

    print_separator();

    return 0;
}
