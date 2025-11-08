/*
 * 文件: permission_demo.c
 * 目的: 展示 Linux 文件權限檢查和設置
 *
 * 編譯: gcc -o permission_demo permission_demo.c
 * 運行: ./permission_demo <filename>
 *
 * 教育目的:
 * - 理解 Linux 權限模型
 * - 解析權限位（rwx、SUID、SGID、Sticky）
 * - 檢查文件所有權
 * - 演示權限修改
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#include <pwd.h>
#include <grp.h>
#include <time.h>

#define COLOR_RESET   "\x1b[0m"
#define COLOR_GREEN   "\x1b[32m"
#define COLOR_YELLOW  "\x1b[33m"
#define COLOR_CYAN    "\x1b[36m"
#define COLOR_RED     "\x1b[31m"

/**
 * 將權限模式轉換為字符串表示 (如 rwxr-xr--)
 */
void mode_to_string(mode_t mode, char *str) {
    /* 文件類型 */
    if (S_ISREG(mode))       str[0] = '-';
    else if (S_ISDIR(mode))  str[0] = 'd';
    else if (S_ISLNK(mode))  str[0] = 'l';
    else if (S_ISCHR(mode))  str[0] = 'c';
    else if (S_ISBLK(mode))  str[0] = 'b';
    else if (S_ISFIFO(mode)) str[0] = 'p';
    else if (S_ISSOCK(mode)) str[0] = 's';
    else                     str[0] = '?';

    /* 所有者權限 */
    str[1] = (mode & S_IRUSR) ? 'r' : '-';
    str[2] = (mode & S_IWUSR) ? 'w' : '-';
    if (mode & S_ISUID)
        str[3] = (mode & S_IXUSR) ? 's' : 'S';  /* SUID */
    else
        str[3] = (mode & S_IXUSR) ? 'x' : '-';

    /* 組權限 */
    str[4] = (mode & S_IRGRP) ? 'r' : '-';
    str[5] = (mode & S_IWGRP) ? 'w' : '-';
    if (mode & S_ISGID)
        str[6] = (mode & S_IXGRP) ? 's' : 'S';  /* SGID */
    else
        str[6] = (mode & S_IXGRP) ? 'x' : '-';

    /* 其他用戶權限 */
    str[7] = (mode & S_IROTH) ? 'r' : '-';
    str[8] = (mode & S_IWOTH) ? 'w' : '-';
    if (mode & S_ISVTX)
        str[9] = (mode & S_IXOTH) ? 't' : 'T';  /* Sticky bit */
    else
        str[9] = (mode & S_IXOTH) ? 'x' : '-';

    str[10] = '\0';
}

/**
 * 顯示文件的詳細權限信息
 */
void display_permissions(const char *filename) {
    struct stat st;
    char mode_str[11];
    struct passwd *pwd;
    struct group *grp;
    char time_str[80];

    printf("\n");
    printf("═══════════════════════════════════════════════════════════\n");
    printf(COLOR_CYAN "  文件權限詳細信息" COLOR_RESET "\n");
    printf("═══════════════════════════════════════════════════════════\n");

    /* 獲取文件狀態 */
    if (stat(filename, &st) != 0) {
        perror("stat");
        return;
    }

    /* 轉換權限為字符串 */
    mode_to_string(st.st_mode, mode_str);

    printf("\n文件: %s\n\n", filename);

    /* 基本信息 */
    printf("權限表示: %s\n", mode_str);
    printf("八進制: %04o\n", st.st_mode & 07777);

    /* 所有者信息 */
    pwd = getpwuid(st.st_uid);
    grp = getgrgid(st.st_gid);

    printf("\n所有者: %s (UID: %d)\n",
           pwd ? pwd->pw_name : "unknown", st.st_uid);
    printf("組: %s (GID: %d)\n",
           grp ? grp->gr_name : "unknown", st.st_gid);

    /* 詳細權限解析 */
    printf("\n" COLOR_YELLOW "權限詳解:" COLOR_RESET "\n");
    printf("  所有者 (User):   %c%c%c  (%s%s%s)\n",
           mode_str[1], mode_str[2], mode_str[3],
           (st.st_mode & S_IRUSR) ? "讀 " : "",
           (st.st_mode & S_IWUSR) ? "寫 " : "",
           (st.st_mode & S_IXUSR) ? "執行" : "");

    printf("  組 (Group):      %c%c%c  (%s%s%s)\n",
           mode_str[4], mode_str[5], mode_str[6],
           (st.st_mode & S_IRGRP) ? "讀 " : "",
           (st.st_mode & S_IWGRP) ? "寫 " : "",
           (st.st_mode & S_IXGRP) ? "執行" : "");

    printf("  其他 (Others):   %c%c%c  (%s%s%s)\n",
           mode_str[7], mode_str[8], mode_str[9],
           (st.st_mode & S_IROTH) ? "讀 " : "",
           (st.st_mode & S_IWOTH) ? "寫 " : "",
           (st.st_mode & S_IXOTH) ? "執行" : "");

    /* 特殊權限 */
    printf("\n" COLOR_YELLOW "特殊權限:" COLOR_RESET "\n");

    if (st.st_mode & S_ISUID) {
        printf("  " COLOR_RED "✓ SUID (Set-UID)" COLOR_RESET " - 以所有者權限執行\n");
    } else {
        printf("  ✗ SUID 未設置\n");
    }

    if (st.st_mode & S_ISGID) {
        if (S_ISDIR(st.st_mode)) {
            printf("  " COLOR_RED "✓ SGID" COLOR_RESET " - 目錄中創建的文件繼承組\n");
        } else {
            printf("  " COLOR_RED "✓ SGID (Set-GID)" COLOR_RESET " - 以組權限執行\n");
        }
    } else {
        printf("  ✗ SGID 未設置\n");
    }

    if (st.st_mode & S_ISVTX) {
        if (S_ISDIR(st.st_mode)) {
            printf("  " COLOR_RED "✓ Sticky Bit" COLOR_RESET " - 只有所有者可刪除文件\n");
        } else {
            printf("  ✓ Sticky Bit 已設置\n");
        }
    } else {
        printf("  ✗ Sticky Bit 未設置\n");
    }

    /* 文件類型和大小 */
    printf("\n" COLOR_YELLOW "文件信息:" COLOR_RESET "\n");

    if (S_ISREG(st.st_mode))
        printf("  類型: 普通文件\n");
    else if (S_ISDIR(st.st_mode))
        printf("  類型: 目錄\n");
    else if (S_ISLNK(st.st_mode))
        printf("  類型: 符號鏈接\n");
    else if (S_ISCHR(st.st_mode))
        printf("  類型: 字符設備\n");
    else if (S_ISBLK(st.st_mode))
        printf("  類型: 塊設備\n");
    else if (S_ISFIFO(st.st_mode))
        printf("  類型: FIFO/命名管道\n");
    else if (S_ISSOCK(st.st_mode))
        printf("  類型: Socket\n");

    printf("  大小: %ld 字節\n", st.st_size);
    printf("  硬鏈接數: %ld\n", st.st_nlink);
    printf("  Inode: %ld\n", st.st_ino);

    /* 時間信息 */
    strftime(time_str, sizeof(time_str), "%Y-%m-%d %H:%M:%S",
             localtime(&st.st_mtime));
    printf("  最後修改: %s\n", time_str);

    strftime(time_str, sizeof(time_str), "%Y-%m-%d %H:%M:%S",
             localtime(&st.st_atime));
    printf("  最後訪問: %s\n", time_str);
}

/**
 * 檢查當前用戶對文件的訪問權限
 */
void check_access(const char *filename) {
    printf("\n");
    printf("═══════════════════════════════════════════════════════════\n");
    printf(COLOR_CYAN "  當前用戶訪問權限" COLOR_RESET "\n");
    printf("═══════════════════════════════════════════════════════════\n");

    printf("\n當前用戶: UID=%d, GID=%d\n", getuid(), getgid());
    printf("有效用戶: UID=%d, GID=%d\n", geteuid(), getegid());

    printf("\n權限檢查:\n");

    if (access(filename, F_OK) == 0) {
        printf("  " COLOR_GREEN "✓" COLOR_RESET " 文件存在\n");
    } else {
        printf("  " COLOR_RED "✗" COLOR_RESET " 文件不存在\n");
        return;
    }

    if (access(filename, R_OK) == 0) {
        printf("  " COLOR_GREEN "✓" COLOR_RESET " 可讀\n");
    } else {
        printf("  " COLOR_RED "✗" COLOR_RESET " 不可讀\n");
    }

    if (access(filename, W_OK) == 0) {
        printf("  " COLOR_GREEN "✓" COLOR_RESET " 可寫\n");
    } else {
        printf("  " COLOR_RED "✗" COLOR_RESET " 不可寫\n");
    }

    if (access(filename, X_OK) == 0) {
        printf("  " COLOR_GREEN "✓" COLOR_RESET " 可執行\n");
    } else {
        printf("  " COLOR_RED "✗" COLOR_RESET " 不可執行\n");
    }
}

/**
 * 演示權限修改
 */
void demonstrate_chmod(void) {
    const char *test_file = "/tmp/permission_test.txt";
    FILE *fp;
    struct stat st;
    char mode_str[11];

    printf("\n");
    printf("═══════════════════════════════════════════════════════════\n");
    printf(COLOR_CYAN "  權限修改演示" COLOR_RESET "\n");
    printf("═══════════════════════════════════════════════════════════\n");

    /* 創建測試文件 */
    fp = fopen(test_file, "w");
    if (!fp) {
        perror("fopen");
        return;
    }
    fprintf(fp, "測試內容\n");
    fclose(fp);

    printf("\n創建測試文件: %s\n", test_file);

    /* 演示不同的權限設置 */
    mode_t modes[] = {
        0644,  /* rw-r--r-- */
        0755,  /* rwxr-xr-x */
        0600,  /* rw------- */
        0777,  /* rwxrwxrwx */
        04755, /* rwsr-xr-x (SUID) */
        02755, /* rwxr-sr-x (SGID) */
        01777, /* rwxrwxrwt (Sticky) */
    };

    const char *descriptions[] = {
        "所有者可讀寫，其他人只讀",
        "所有者全權限，其他人讀執行",
        "只有所有者可讀寫",
        "所有人全權限（危險！）",
        "SUID: 以所有者權限執行",
        "SGID: 以組權限執行",
        "Sticky Bit: 防止隨意刪除",
    };

    int count = sizeof(modes) / sizeof(modes[0]);

    for (int i = 0; i < count; i++) {
        if (chmod(test_file, modes[i]) != 0) {
            perror("chmod");
            continue;
        }

        stat(test_file, &st);
        mode_to_string(st.st_mode, mode_str);

        printf("\n%d. chmod %04o  →  %s\n",
               i + 1, modes[i], mode_str);
        printf("   說明: %s\n", descriptions[i]);
    }

    /* 清理 */
    unlink(test_file);
    printf("\n測試文件已刪除\n");
}

/**
 * 安全檢查示例
 */
void security_checks(const char *filename) {
    struct stat st;

    printf("\n");
    printf("═══════════════════════════════════════════════════════════\n");
    printf(COLOR_CYAN "  安全檢查" COLOR_RESET "\n");
    printf("═══════════════════════════════════════════════════════════\n");

    if (stat(filename, &st) != 0) {
        perror("stat");
        return;
    }

    printf("\n執行安全檢查...\n\n");

    /* 檢查1: 文件是否為符號鏈接 */
    if (S_ISLNK(st.st_mode)) {
        printf(COLOR_YELLOW "⚠ 警告: 這是一個符號鏈接\n" COLOR_RESET);
        printf("  符號鏈接可能被用於攻擊（如符號鏈接競爭）\n");
    } else {
        printf(COLOR_GREEN "✓" COLOR_RESET " 不是符號鏈接\n");
    }

    /* 檢查2: 其他用戶是否可寫 */
    if (st.st_mode & S_IWOTH) {
        printf(COLOR_RED "✗ 危險: 其他用戶可寫！\n" COLOR_RESET);
        printf("  任何人都可以修改此文件\n");
    } else {
        printf(COLOR_GREEN "✓" COLOR_RESET " 其他用戶不可寫\n");
    }

    /* 檢查3: 組用戶是否可寫 */
    if (st.st_mode & S_IWGRP) {
        printf(COLOR_YELLOW "⚠ 注意: 組用戶可寫\n" COLOR_RESET);
        printf("  組內成員可以修改此文件\n");
    } else {
        printf(COLOR_GREEN "✓" COLOR_RESET " 組用戶不可寫\n");
    }

    /* 檢查4: SUID/SGID */
    if (st.st_mode & (S_ISUID | S_ISGID)) {
        printf(COLOR_RED "⚠ 警告: 設置了 SUID 或 SGID\n" COLOR_RESET);
        printf("  這可能是安全風險，需要仔細審查\n");

        if (st.st_uid == 0) {
            printf(COLOR_RED "✗ 嚴重: SUID root 文件！\n" COLOR_RESET);
            printf("  如果此文件有漏洞，可能導致權限提升\n");
        }
    } else {
        printf(COLOR_GREEN "✓" COLOR_RESET " 未設置 SUID/SGID\n");
    }

    /* 檢查5: 文件所有者 */
    if (st.st_uid == 0) {
        printf(COLOR_YELLOW "⚠ 注意: 文件屬於 root\n" COLOR_RESET);
    } else if (st.st_uid == getuid()) {
        printf(COLOR_GREEN "✓" COLOR_RESET " 文件屬於當前用戶\n");
    }

    /* 總結 */
    int security_score = 100;
    if (S_ISLNK(st.st_mode)) security_score -= 10;
    if (st.st_mode & S_IWOTH) security_score -= 40;
    if (st.st_mode & S_IWGRP) security_score -= 10;
    if ((st.st_mode & S_ISUID) && st.st_uid == 0) security_score -= 30;

    printf("\n" COLOR_CYAN "安全評分: ");
    if (security_score >= 80)
        printf(COLOR_GREEN);
    else if (security_score >= 60)
        printf(COLOR_YELLOW);
    else
        printf(COLOR_RED);
    printf("%d/100" COLOR_RESET "\n", security_score);
}

/**
 * 主函數
 */
int main(int argc, char *argv[]) {
    printf("╔═══════════════════════════════════════════════════════════╗\n");
    printf("║          Linux 文件權限檢查與設置演示                      ║\n");
    printf("║        File Permissions Demonstration                    ║\n");
    printf("╚═══════════════════════════════════════════════════════════╝\n");

    if (argc < 2) {
        printf("\n使用方法: %s <filename>\n", argv[0]);
        printf("\n示例:\n");
        printf("  %s /etc/passwd\n", argv[0]);
        printf("  %s /usr/bin/passwd\n", argv[0]);
        printf("  %s /tmp\n", argv[0]);
        printf("\n或運行演示模式:\n");
        printf("  %s --demo\n", argv[0]);
        return 1;
    }

    if (strcmp(argv[1], "--demo") == 0) {
        /* 演示模式 */
        demonstrate_chmod();
    } else {
        /* 檢查指定文件 */
        display_permissions(argv[1]);
        check_access(argv[1]);
        security_checks(argv[1]);
    }

    printf("\n");
    printf("═══════════════════════════════════════════════════════════\n");
    printf(COLOR_CYAN "  提示" COLOR_RESET "\n");
    printf("═══════════════════════════════════════════════════════════\n");
    printf("\n常用命令:\n");
    printf("  ls -l <file>          # 查看權限\n");
    printf("  chmod 644 <file>      # 設置權限\n");
    printf("  chown user:group <f>  # 更改所有者\n");
    printf("  find / -perm -4000    # 查找 SUID 文件\n");
    printf("\n");

    return 0;
}
