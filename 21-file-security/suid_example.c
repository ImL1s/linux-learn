/*
 * 文件: suid_example.c
 * 目的: 演示 SUID 程式的工作原理和安全考慮（教育目的）
 *
 * ⚠️ 警告: 這是教育範例，不要在生產環境設置 SUID！
 *
 * 編譯: gcc -o suid_example suid_example.c
 * 設置SUID (僅測試): sudo chown root:root suid_example && sudo chmod 4755 suid_example
 * 運行: ./suid_example
 * 清理: sudo chmod 0755 suid_example
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <pwd.h>

#define COLOR_RESET   "\x1b[0m"
#define COLOR_RED     "\x1b[31m"
#define COLOR_GREEN   "\x1b[32m"
#define COLOR_YELLOW  "\x1b[33m"
#define COLOR_CYAN    "\x1b[36m"

void display_ids(const char *stage) {
    uid_t ruid = getuid();
    uid_t euid = geteuid();
    gid_t rgid = getgid();
    gid_t egid = getegid();

    struct passwd *rpwd = getpwuid(ruid);
    struct passwd *epwd = getpwuid(euid);

    printf("\n[%s]\n", stage);
    printf("  真實 UID: %d (%s)\n", ruid, rpwd ? rpwd->pw_name : "unknown");
    printf("  有效 UID: %d (%s)\n", euid, epwd ? epwd->pw_name : "unknown");
    printf("  真實 GID: %d\n", rgid);
    printf("  有效 GID: %d\n", egid);

    if (ruid != euid) {
        printf(COLOR_RED "  ⚠ 這是一個 SUID 程式！\n" COLOR_RESET);
    }
}

void safe_privilege_drop(void) {
    uid_t ruid = getuid();

    printf("\n" COLOR_YELLOW "正在放棄特權..." COLOR_RESET "\n");

    if (seteuid(ruid) != 0) {
        perror("seteuid");
        exit(1);
    }

    printf(COLOR_GREEN "✓ 特權已放棄\n" COLOR_RESET);
}

void demonstrate_privilege_operation(void) {
    FILE *fp;
    const char *test_file = "/tmp/suid_test.txt";

    printf("\n" COLOR_CYAN "嘗試創建特權文件..." COLOR_RESET "\n");

    /* 嘗試以當前有效 UID 創建文件 */
    fp = fopen(test_file, "w");
    if (fp) {
        fprintf(fp, "由 UID %d 創建\n", geteuid());
        fclose(fp);
        printf(COLOR_GREEN "✓ 文件創建成功: %s\n" COLOR_RESET, test_file);

        /* 顯示文件所有者 */
        char cmd[256];
        snprintf(cmd, sizeof(cmd), "ls -l %s", test_file);
        system(cmd);

        /* 清理 */
        unlink(test_file);
    } else {
        printf(COLOR_RED "✗ 文件創建失敗\n" COLOR_RESET);
    }
}

int main(void) {
    printf("╔═══════════════════════════════════════════════════════════╗\n");
    printf("║            SUID 程式演示（教育目的）                       ║\n");
    printf("║         SUID Program Demonstration                       ║\n");
    printf("╚═══════════════════════════════════════════════════════════╝\n");

    printf(COLOR_YELLOW "\n⚠️  重要提示:\n");
    printf("   本程式用於演示 SUID 機制，僅供教育目的\n");
    printf("   不要在生產環境設置 SUID！\n" COLOR_RESET);

    /* 1. 顯示初始 UID */
    display_ids("程式啟動");

    /* 2. 檢查是否為 SUID */
    if (getuid() != geteuid()) {
        printf("\n" COLOR_RED "檢測到 SUID 設置！" COLOR_RESET "\n");
        printf("這個程式正在以提升的權限運行。\n");

        /* 3. 執行特權操作 */
        demonstrate_privilege_operation();

        /* 4. 放棄特權 */
        safe_privilege_drop();

        /* 5. 再次顯示 UID */
        display_ids("放棄特權後");

    } else {
        printf("\n" COLOR_GREEN "這個程式沒有 SUID 設置\n" COLOR_RESET);
        printf("以普通權限運行。\n");

        printf("\n" COLOR_CYAN "如何設置 SUID（僅測試環境）:" COLOR_RESET "\n");
        printf("  sudo chown root:root suid_example\n");
        printf("  sudo chmod 4755 suid_example\n");
        printf("  ./suid_example\n");
        printf("\n清理:\n");
        printf("  sudo chmod 0755 suid_example\n");
    }

    printf("\n" COLOR_CYAN "安全要點:" COLOR_RESET "\n");
    printf("  1. SUID 程式以文件所有者權限運行\n");
    printf("  2. SUID root 程式極其危險，需要仔細審計\n");
    printf("  3. 使用後應立即放棄不需要的特權\n");
    printf("  4. 避免在 SUID 程式中調用 system() 或 popen()\n");
    printf("  5. 始終使用絕對路徑調用外部程式\n");
    printf("\n");

    return 0;
}
