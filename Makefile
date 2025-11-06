# Makefile for Linux System Programming Learning Project
# 用於編譯所有範例程式

CC = gcc
CFLAGS = -Wall -Wextra -O2
PTHREAD_FLAGS = -pthread

# 所有目標
.PHONY: all process pipe fifo signal fileio thread shm clean help

# 默認目標：編譯所有範例
all: process pipe fifo signal fileio thread shm
	@echo ""
	@echo "============================================"
	@echo "  所有範例編譯完成！"
	@echo "============================================"
	@echo ""
	@echo "執行 'make help' 查看使用說明"
	@echo ""

# 1. 進程管理
process:
	@echo "編譯進程管理範例..."
	$(CC) $(CFLAGS) -o 01-process/fork_basic 01-process/fork_basic.c
	$(CC) $(CFLAGS) -o 01-process/exec_demo 01-process/exec_demo.c
	$(CC) $(CFLAGS) -o 01-process/zombie_orphan 01-process/zombie_orphan.c
	@echo "  ✓ fork_basic"
	@echo "  ✓ exec_demo"
	@echo "  ✓ zombie_orphan"

# 2. 管道通訊
pipe:
	@echo "編譯管道範例..."
	$(CC) $(CFLAGS) -o 02-pipe/pipe_demo 02-pipe/pipe_demo.c
	@echo "  ✓ pipe_demo"

# 3. 命名管道
fifo:
	@echo "編譯 FIFO 範例..."
	$(CC) $(CFLAGS) -o 03-fifo/fifo_writer 03-fifo/fifo_writer.c
	$(CC) $(CFLAGS) -o 03-fifo/fifo_reader 03-fifo/fifo_reader.c
	@echo "  ✓ fifo_writer"
	@echo "  ✓ fifo_reader"

# 4. 信號處理
signal:
	@echo "編譯信號處理範例..."
	$(CC) $(CFLAGS) -o 04-signal/signal_demo 04-signal/signal_demo.c
	@echo "  ✓ signal_demo"

# 5. 文件 I/O
fileio:
	@echo "編譯文件 I/O 範例..."
	$(CC) $(CFLAGS) -o 05-file-io/file_operations 05-file-io/file_operations.c
	@echo "  ✓ file_operations"

# 6. 多線程
thread:
	@echo "編譯多線程範例..."
	$(CC) $(CFLAGS) $(PTHREAD_FLAGS) -o 06-thread/thread_demo 06-thread/thread_demo.c
	@echo "  ✓ thread_demo"

# 7. 共享內存
shm:
	@echo "編譯共享內存範例..."
	$(CC) $(CFLAGS) -o 07-shared-memory/shm_writer 07-shared-memory/shm_writer.c
	$(CC) $(CFLAGS) -o 07-shared-memory/shm_reader 07-shared-memory/shm_reader.c
	@echo "  ✓ shm_writer"
	@echo "  ✓ shm_reader"

# 清理所有編譯文件
clean:
	@echo "清理編譯文件..."
	rm -f 01-process/fork_basic 01-process/exec_demo 01-process/zombie_orphan
	rm -f 02-pipe/pipe_demo
	rm -f 03-fifo/fifo_writer 03-fifo/fifo_reader
	rm -f 04-signal/signal_demo
	rm -f 05-file-io/file_operations 05-file-io/test_data.txt
	rm -f 06-thread/thread_demo
	rm -f 07-shared-memory/shm_writer 07-shared-memory/shm_reader
	rm -f pipe/a.out
	@echo "清理完成！"

# 幫助信息
help:
	@echo "============================================"
	@echo "  Linux 系統程式設計學習專案 - Makefile"
	@echo "============================================"
	@echo ""
	@echo "使用方法："
	@echo "  make          - 編譯所有範例"
	@echo "  make all      - 編譯所有範例"
	@echo "  make process  - 只編譯進程管理範例"
	@echo "  make pipe     - 只編譯管道範例"
	@echo "  make fifo     - 只編譯 FIFO 範例"
	@echo "  make signal   - 只編譯信號處理範例"
	@echo "  make fileio   - 只編譯文件 I/O 範例"
	@echo "  make thread   - 只編譯多線程範例"
	@echo "  make shm      - 只編譯共享內存範例"
	@echo "  make clean    - 清理所有編譯文件"
	@echo "  make help     - 顯示此幫助信息"
	@echo ""
	@echo "範例程式位置："
	@echo "  01-process/        - 進程管理"
	@echo "  02-pipe/           - 管道通訊"
	@echo "  03-fifo/           - 命名管道"
	@echo "  04-signal/         - 信號處理"
	@echo "  05-file-io/        - 文件 I/O"
	@echo "  06-thread/         - 多線程"
	@echo "  07-shared-memory/  - 共享內存"
	@echo ""
	@echo "編譯器選項："
	@echo "  CC      = $(CC)"
	@echo "  CFLAGS  = $(CFLAGS)"
	@echo "  PTHREAD = $(PTHREAD_FLAGS)"
	@echo ""
