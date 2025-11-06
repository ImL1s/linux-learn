# Linux 系統程式設計學習專案 Makefile

CC = gcc
CFLAGS = -Wall -Wextra -O2
PTHREAD = -pthread

# 所有目標
.PHONY: all clean help process pipe fifo signal fileio thread shm socket epoll daemon semaphore \
        condition-var rwlock message-queue mmap select-poll udp-socket timer ipc-benchmark utils

# 默認目標
all: process pipe fifo signal fileio thread shm socket epoll daemon semaphore \
     condition-var rwlock message-queue mmap select-poll udp-socket timer ipc-benchmark utils
	@echo ""
	@echo "============================================"
	@echo "  所有範例編譯完成！"
	@echo "============================================"
	@echo ""
	@echo "執行 'make help' 查看使用說明"

# 01-進程管理
process:
	@echo "編譯進程管理範例..."
	@$(CC) $(CFLAGS) -o 01-process/fork_basic 01-process/fork_basic.c
	@$(CC) $(CFLAGS) -o 01-process/exec_demo 01-process/exec_demo.c
	@$(CC) $(CFLAGS) -o 01-process/zombie_orphan 01-process/zombie_orphan.c
	@echo "  ✓ fork_basic"
	@echo "  ✓ exec_demo"
	@echo "  ✓ zombie_orphan"

# 02-管道
pipe:
	@echo "編譯管道範例..."
	@$(CC) $(CFLAGS) -o 02-pipe/pipe_demo 02-pipe/pipe_demo.c
	@echo "  ✓ pipe_demo"

# 03-FIFO
fifo:
	@echo "編譯 FIFO 範例..."
	@$(CC) $(CFLAGS) -o 03-fifo/fifo_writer 03-fifo/fifo_writer.c
	@$(CC) $(CFLAGS) -o 03-fifo/fifo_reader 03-fifo/fifo_reader.c
	@echo "  ✓ fifo_writer"
	@echo "  ✓ fifo_reader"

# 04-信號處理
signal:
	@echo "編譯信號處理範例..."
	@$(CC) $(CFLAGS) -o 04-signal/signal_demo 04-signal/signal_demo.c
	@echo "  ✓ signal_demo"

# 05-文件I/O
fileio:
	@echo "編譯文件 I/O 範例..."
	@$(CC) $(CFLAGS) -o 05-file-io/file_operations 05-file-io/file_operations.c
	@echo "  ✓ file_operations"

# 06-多線程
thread:
	@echo "編譯多線程範例..."
	@$(CC) $(CFLAGS) $(PTHREAD) -o 06-thread/thread_demo 06-thread/thread_demo.c
	@$(CC) $(CFLAGS) $(PTHREAD) -o 06-thread/thread_pool 06-thread/thread_pool.c
	@echo "  ✓ thread_demo"
	@echo "  ✓ thread_pool"

# 07-共享內存
shm:
	@echo "編譯共享內存範例..."
	@$(CC) $(CFLAGS) -o 07-shared-memory/shm_writer 07-shared-memory/shm_writer.c
	@$(CC) $(CFLAGS) -o 07-shared-memory/shm_reader 07-shared-memory/shm_reader.c
	@echo "  ✓ shm_writer"
	@echo "  ✓ shm_reader"

# 08-Socket
socket:
	@echo "編譯 Socket 範例..."
	@$(CC) $(CFLAGS) -o 08-socket/tcp_server 08-socket/tcp_server.c
	@$(CC) $(CFLAGS) -o 08-socket/tcp_client 08-socket/tcp_client.c
	@$(CC) $(CFLAGS) $(PTHREAD) -o 08-socket/tcp_echo_advanced 08-socket/tcp_echo_advanced.c
	@echo "  ✓ tcp_server"
	@echo "  ✓ tcp_client"
	@echo "  ✓ tcp_echo_advanced"

# 09-epoll
epoll:
	@echo "編譯 epoll 範例..."
	@$(CC) $(CFLAGS) -o 09-epoll/epoll_server 09-epoll/epoll_server.c
	@$(CC) $(CFLAGS) -o 09-epoll/http_server_simple 09-epoll/http_server_simple.c
	@echo "  ✓ epoll_server"
	@echo "  ✓ http_server_simple"

# 10-守護進程
daemon:
	@echo "編譯守護進程範例..."
	@$(CC) $(CFLAGS) -o 10-daemon/daemon_demo 10-daemon/daemon_demo.c
	@echo "  ✓ daemon_demo"

# 11-信號量
semaphore:
	@echo "編譯信號量範例..."
	@$(CC) $(CFLAGS) $(PTHREAD) -o 11-semaphore/semaphore_demo 11-semaphore/semaphore_demo.c
	@echo "  ✓ semaphore_demo"

# 12-條件變量
condition-var:
	@echo "編譯條件變量範例..."
	@$(CC) $(CFLAGS) $(PTHREAD) -o 12-condition-var/cond_basic 12-condition-var/cond_basic.c
	@$(CC) $(CFLAGS) $(PTHREAD) -o 12-condition-var/producer_consumer 12-condition-var/producer_consumer.c
	@$(CC) $(CFLAGS) $(PTHREAD) -o 12-condition-var/cond_vs_semaphore 12-condition-var/cond_vs_semaphore.c
	@echo "  ✓ cond_basic"
	@echo "  ✓ producer_consumer"
	@echo "  ✓ cond_vs_semaphore"

# 13-讀寫鎖
rwlock:
	@echo "編譯讀寫鎖範例..."
	@$(CC) $(CFLAGS) $(PTHREAD) -o 13-rwlock/rwlock_basic 13-rwlock/rwlock_basic.c
	@$(CC) $(CFLAGS) $(PTHREAD) -o 13-rwlock/rwlock_demo 13-rwlock/rwlock_demo.c
	@echo "  ✓ rwlock_basic"
	@echo "  ✓ rwlock_demo"

# 14-消息隊列
message-queue:
	@echo "編譯消息隊列範例..."
	@$(CC) $(CFLAGS) -o 14-message-queue/msg_sender 14-message-queue/msg_sender.c
	@$(CC) $(CFLAGS) -o 14-message-queue/msg_receiver 14-message-queue/msg_receiver.c
	@echo "  ✓ msg_sender"
	@echo "  ✓ msg_receiver"

# 15-內存映射
mmap:
	@echo "編譯內存映射範例..."
	@$(CC) $(CFLAGS) -o 15-mmap/mmap_file 15-mmap/mmap_file.c
	@$(CC) $(CFLAGS) -o 15-mmap/mmap_shared 15-mmap/mmap_shared.c
	@echo "  ✓ mmap_file"
	@echo "  ✓ mmap_shared"

# 16-定時器
timer:
	@echo "編譯定時器範例..."
	@$(CC) $(CFLAGS) -o 16-timer/alarm_demo 16-timer/alarm_demo.c
	@$(CC) $(CFLAGS) -o 16-timer/setitimer_demo 16-timer/setitimer_demo.c
	@echo "  ✓ alarm_demo"
	@echo "  ✓ setitimer_demo"

# 17-select/poll
select-poll:
	@echo "編譯 select/poll 範例..."
	@$(CC) $(CFLAGS) -o 17-select-poll/select_server 17-select-poll/select_server.c
	@$(CC) $(CFLAGS) -o 17-select-poll/poll_server 17-select-poll/poll_server.c
	@echo "  ✓ select_server"
	@echo "  ✓ poll_server"

# 18-UDP Socket
udp-socket:
	@echo "編譯 UDP Socket 範例..."
	@$(CC) $(CFLAGS) -o 18-udp-socket/udp_server 18-udp-socket/udp_server.c
	@$(CC) $(CFLAGS) -o 18-udp-socket/udp_client 18-udp-socket/udp_client.c
	@echo "  ✓ udp_server"
	@echo "  ✓ udp_client"

# 19-IPC 性能對比
ipc-benchmark:
	@echo "編譯 IPC 性能對比工具..."
	@$(CC) $(CFLAGS) -o 19-ipc-benchmark/ipc_benchmark 19-ipc-benchmark/ipc_benchmark.c -lrt
	@echo "  ✓ ipc_benchmark"

# 工具庫
utils:
	@echo "編譯工具庫範例..."
	@$(CC) $(CFLAGS) -o utils/config_demo utils/config_demo.c
	@echo "  ✓ config_demo"

# 清理
clean:
	@echo "清理編譯文件..."
	@rm -f 01-process/fork_basic 01-process/exec_demo 01-process/zombie_orphan
	@rm -f 02-pipe/pipe_demo
	@rm -f 03-fifo/fifo_writer 03-fifo/fifo_reader
	@rm -f 04-signal/signal_demo
	@rm -f 05-file-io/file_operations 05-file-io/test_data.txt
	@rm -f 06-thread/thread_demo 06-thread/thread_pool
	@rm -f 07-shared-memory/shm_writer 07-shared-memory/shm_reader
	@rm -f 08-socket/tcp_server 08-socket/tcp_client 08-socket/tcp_echo_advanced
	@rm -f 09-epoll/epoll_server 09-epoll/http_server_simple
	@rm -f 10-daemon/daemon_demo
	@rm -f 11-semaphore/semaphore_demo
	@rm -f 12-condition-var/cond_basic 12-condition-var/producer_consumer 12-condition-var/cond_vs_semaphore
	@rm -f 13-rwlock/rwlock_basic 13-rwlock/rwlock_demo
	@rm -f 14-message-queue/msg_sender 14-message-queue/msg_receiver
	@rm -f 15-mmap/mmap_file 15-mmap/mmap_shared 15-mmap/test.txt
	@rm -f 16-timer/alarm_demo 16-timer/setitimer_demo
	@rm -f 17-select-poll/select_server 17-select-poll/poll_server
	@rm -f 18-udp-socket/udp_server 18-udp-socket/udp_client
	@rm -f 19-ipc-benchmark/ipc_benchmark
	@rm -f utils/config_demo
	@rm -f pipe/a.out
	@echo "清理完成！"

# 幫助
help:
	@echo "Linux 系統程式設計學習專案"
	@echo ""
	@echo "使用方式："
	@echo "  make all            - 編譯所有範例"
	@echo "  make clean          - 清理所有編譯文件"
	@echo "  make help           - 顯示此幫助信息"
	@echo ""
	@echo "編譯特定主題："
	@echo "  make process        - 01-進程管理"
	@echo "  make pipe           - 02-管道"
	@echo "  make fifo           - 03-FIFO命名管道"
	@echo "  make signal         - 04-信號處理"
	@echo "  make fileio         - 05-文件I/O"
	@echo "  make thread         - 06-多線程"
	@echo "  make shm            - 07-共享內存"
	@echo "  make socket         - 08-TCP Socket"
	@echo "  make epoll          - 09-epoll"
	@echo "  make daemon         - 10-守護進程"
	@echo "  make semaphore      - 11-信號量"
	@echo "  make condition-var  - 12-條件變量 🆕"
	@echo "  make rwlock         - 13-讀寫鎖 🆕"
	@echo "  make message-queue  - 14-消息隊列 🆕"
	@echo "  make mmap           - 15-內存映射 🆕"
	@echo "  make timer          - 16-定時器 🆕"
	@echo "  make select-poll    - 17-select/poll 🆕"
	@echo "  make udp-socket     - 18-UDP Socket 🆕"
	@echo "  make ipc-benchmark  - 19-IPC 性能對比 🆕"
	@echo "  make utils          - 工具庫範例 🆕"
