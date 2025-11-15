/*
 * 文件: secure_tempfile.c
 * 目的: 展示安全的臨時文件處理
 *
 * 編譯: gcc -o secure_tempfile secure_tempfile.c
 * 運行: ./secure_tempfile
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <errno.h>
#include <time.h>

#define COLOR_RESET   "\x1b[0m"
#define COLOR_RED     "\x1b[31m"
#define COLOR_GREEN   "\x1b[32m"
#define COLOR_YELLOW  "\x1b[33m"
#define COLOR_CYAN    "\x1b[36m"

/* 不安全的方法 (示例) */
void unsafe_temp_method(void) {
    const char *filename = "/tmp/unsafe_temp.txt";
    FILE *fp;

    printf("\n" COLOR_RED "✗ 不安全方法演示" COLOR_RESET "\n");
    printf("文件名: %s\n", filename);

    /* 危險：可預測的文件名 */
    fp = fopen(filename, "w");
    if (fp) {
        fprintf(fp, "不安全的數據\n");
        fclose(fp);
        printf(COLOR_YELLOW "⚠ 問題:\n");
        printf("  1. 文件名可預測，攻擊者可以提前創建符號鏈接\n");
        printf("  2. 可能存在競爭條件\n");
        printf("  3. 權限可能不正確\n" COLOR_RESET);

        unlink(filename);
    }
}

/* 安全的方法 1: mkstemp() */
void safe_method_mkstemp(void) {
    char template[] = "/tmp/secure_temp.XXXXXX";
    int fd;
    struct stat st;

    printf("\n" COLOR_GREEN "✓ 安全方法 1: mkstemp()" COLOR_RESET "\n");
    printf("模板: %s\n", template);

    fd = mkstemp(template);
    if (fd == -1) {
        perror("mkstemp");
        return;
    }

    printf("生成的文件名: %s\n", template);

    /* 寫入數據 */
    const char *data = "安全的臨時數據\n";
    if (write(fd, data, strlen(data)) == -1) {
        perror("write");
    }

    /* 設置正確的權限 */
    if (fchmod(fd, 0600) == 0) {
        printf(COLOR_GREEN "✓ 權限設置為 0600 (只有所有者可訪問)\n" COLOR_RESET);
    }

    /* 檢查文件狀態 */
    if (fstat(fd, &st) == 0) {
        printf("文件所有者: UID %d\n", st.st_uid);
        printf("文件權限: %04o\n", st.st_mode & 07777);
    }

    printf(COLOR_GREEN "✓ 優點:\n");
    printf("  1. 文件名不可預測\n");
    printf("  2. 原子創建，避免競爭條件\n");
    printf("  3. 返回文件描述符，立即可用\n" COLOR_RESET);

    close(fd);
    unlink(template);
    printf("臨時文件已刪除\n");
}

/* 安全的方法 2: open() with O_EXCL */
void safe_method_open_excl(void) {
    char filename[256];
    int fd;

    /* 使用進程 ID 和時間戳生成唯一文件名 */
    snprintf(filename, sizeof(filename), "/tmp/secure_%d_%ld.tmp",
             getpid(), (long)time(NULL));

    printf("\n" COLOR_GREEN "✓ 安全方法 2: open() with O_EXCL" COLOR_RESET "\n");
    printf("文件名: %s\n", filename);

    /* O_EXCL 確保文件不存在時才創建 */
    fd = open(filename, O_CREAT | O_EXCL | O_WRONLY, 0600);
    if (fd == -1) {
        if (errno == EEXIST) {
            printf(COLOR_RED "✗ 文件已存在（可能是攻擊）\n" COLOR_RESET);
        } else {
            perror("open");
        }
        return;
    }

    /* 寫入數據 */
    const char *data = "使用 O_EXCL 標誌的數據\n";
    if (write(fd, data, strlen(data)) == -1) {
        perror("write");
    }

    printf(COLOR_GREEN "✓ 優點:\n");
    printf("  1. 原子操作，如果文件存在則失敗\n");
    printf("  2. 可以檢測競爭條件攻擊\n" COLOR_RESET);

    close(fd);
    unlink(filename);
    printf("臨時文件已刪除\n");
}

/* 演示符號鏈接攻擊防護 */
void demonstrate_symlink_protection(void) {
    const char *filename = "/tmp/symlink_test";
    int fd;

    printf("\n" COLOR_CYAN "符號鏈接攻擊防護演示" COLOR_RESET "\n");

    /* 創建一個測試符號鏈接 */
    unlink(filename);  /* 清理可能存在的文件 */
    if (symlink("/etc/passwd", filename) == -1) {
        perror("symlink");
        return;
    }
    printf("創建符號鏈接: %s -> /etc/passwd\n", filename);

    /* 嘗試打開（不跟隨符號鏈接） */
    fd = open(filename, O_RDONLY | O_NOFOLLOW);
    if (fd == -1) {
        if (errno == ELOOP) {
            printf(COLOR_GREEN "✓ 檢測到符號鏈接，拒絕打開\n" COLOR_RESET);
            printf("  O_NOFOLLOW 標誌阻止了符號鏈接攻擊\n");
        } else {
            perror("open");
        }
    } else {
        printf(COLOR_RED "✗ 意外：文件被打開了\n" COLOR_RESET);
        close(fd);
    }

    unlink(filename);
}

/* 最佳實踐總結 */
void show_best_practices(void) {
    printf("\n");
    printf("═══════════════════════════════════════════════════════════\n");
    printf(COLOR_CYAN "  臨時文件安全最佳實踐" COLOR_RESET "\n");
    printf("═══════════════════════════════════════════════════════════\n");

    printf("\n" COLOR_GREEN "✓ 推薦做法:" COLOR_RESET "\n");
    printf("  1. 使用 mkstemp() 或 tmpfile()\n");
    printf("  2. 使用 O_EXCL 標誌創建文件\n");
    printf("  3. 設置正確的權限 (0600)\n");
    printf("  4. 使用 O_NOFOLLOW 防止符號鏈接攻擊\n");
    printf("  5. 盡快刪除臨時文件\n");
    printf("  6. 考慮使用 tmpfile()（自動刪除）\n");

    printf("\n" COLOR_RED "✗ 避免做法:" COLOR_RESET "\n");
    printf("  1. 使用可預測的文件名\n");
    printf("  2. 在 /tmp 使用簡單文件名\n");
    printf("  3. 不檢查文件是否已存在\n");
    printf("  4. 使用不安全的權限（如 0666）\n");
    printf("  5. 忘記刪除臨時文件\n");

    printf("\n" COLOR_CYAN "代碼示例:" COLOR_RESET "\n");
    printf(COLOR_YELLOW);
    printf("  // 安全的臨時文件創建\n");
    printf("  char template[] = \"/tmp/myapp.XXXXXX\";\n");
    printf("  int fd = mkstemp(template);\n");
    printf("  if (fd == -1) {\n");
    printf("      perror(\"mkstemp\");\n");
    printf("      return -1;\n");
    printf("  }\n");
    printf("  fchmod(fd, 0600);  // 設置安全權限\n");
    printf("  // ... 使用文件 ...\n");
    printf("  close(fd);\n");
    printf("  unlink(template);  // 刪除文件\n");
    printf(COLOR_RESET);
}

int main(void) {
    printf("╔═══════════════════════════════════════════════════════════╗\n");
    printf("║            安全臨時文件處理演示                            ║\n");
    printf("║         Secure Temporary File Handling                   ║\n");
    printf("╚═══════════════════════════════════════════════════════════╝\n");

    /* 演示不安全方法 */
    unsafe_temp_method();

    /* 演示安全方法 */
    safe_method_mkstemp();
    safe_method_open_excl();

    /* 符號鏈接防護 */
    demonstrate_symlink_protection();

    /* 最佳實踐 */
    show_best_practices();

    printf("\n" COLOR_CYAN "參考:" COLOR_RESET "\n");
    printf("  man mkstemp\n");
    printf("  man tmpfile\n");
    printf("  man 2 open  (查看 O_EXCL, O_NOFOLLOW)\n");
    printf("\n");

    return 0;
}
