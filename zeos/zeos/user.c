#include <libc.h>

static int g_parent_pid = -1;
static int g_child_pid = -1;
static int g_mem_probe = 1111;
static int g_total = 0;
static int g_pass = 0;

static void print(char *msg)
{
    write(1, msg, strlen(msg));
}

static void print_int(char *msg, int val)
{
    char buff[16];
    print(msg);
    itoa(val, buff);
    print(buff);
    print("\n");
}

static void test_result(char *name, int ok)
{
    g_total++;
    if (ok) {
        g_pass++;
        print("[PASS] ");
    } else {
        print("[FAIL] ");
    }
    print(name);
    print("\n");
}

static void burn_ticks(int loops)
{
    volatile int i;
    for (i = 0; i < loops; ++i) {
    }
}

static int read_word(char *buffer, int maxchars)
{
    int n = 0;
    char c;

    if (buffer == (char *)0) return -1;
    if (maxchars <= 1) return -1;

    while (1) {
        int r = read(&c, 1);

        if (r <= 0) {
            if (n > 0) break;
            return r;
        }

        if (c == ' ' || c == '\n' || c == '\r') {
            if (n == 0) continue;
            break;
        }

        if (n < (maxchars - 1)) {
            buffer[n++] = c;
        }
    }

    buffer[n] = '\0';
    return n;
}



static void test_write_errors(void)
{
    int r;

    print("\n-- write() validation tests --\n");

    r = write(0, "x", 1);
    test_result("write(fd=0) returns -1", r == -1);
    test_result("errno=EBADF (9)", errno == 9);
    if (r == -1) perror();

    r = write(1, "x", -1);
    test_result("write(size<0) returns -1", r == -1);
    test_result("errno=EINVAL (22)", errno == 22);
    if (r == -1) perror();

    r = write(1, (char *)0, 1);
    test_result("write(NULL) returns -1", r == -1);
    test_result("errno=EFAULT (14)", errno == 14);
    if (r == -1) perror();
}

static void test_read_errors(void)
{
    int r;
    char tmp[8];

    print("\n-- read() validation tests --\n");

    r = read((char *)0, 1);
    test_result("read(NULL) returns -1", r == -1);
    test_result("errno=EFAULT (14)", errno == 14);
    if (r == -1) perror();

    r = read(tmp, -1);
    test_result("read(size<0) returns -1", r == -1);
    test_result("errno=EINVAL (22)", errno == 22);
    if (r == -1) perror();

    r = read(tmp, 0);
    test_result("read(size=0) returns 0", r == 0);
}

static void child_flow(void)
{
    int mypid;
    int t_before;
    int t_after;

    print("\n[Child] starting child flow\n");
    mypid = getpid();
    print_int("[Child] PID: ", mypid);

    test_result("child getpid() is not parent PID", mypid != g_parent_pid);
    test_result("child getpid() > 1", mypid > 1);
    test_result("child sees fork-copied memory value", g_mem_probe == 1111);

    g_mem_probe = 3333;
    test_result("child can modify its own copy", g_mem_probe == 3333);

    test_result("child unblock(self) fails", unblock(mypid) < 0);
    test_result("child unblock(parent) fails", unblock(g_parent_pid) < 0);
    test_result("child unblock(invalid pid) fails", unblock(9999) < 0);

    print("[Child] block #1 should not sleep (pending_unblocks consumed)\n");
    t_before = gettime();
    block();
    t_after = gettime();
    test_result("child block #1 returns quickly", t_after >= t_before);

    test_result("child memory copy keeps its own value", g_mem_probe == 3333);

    print("[Child] block #2 should sleep until parent unblocks\n");
    t_before = gettime();
    block();
    t_after = gettime();
    test_result("child block #2 woke after time advanced", t_after > t_before);

    print_int("[Child] End time: ", gettime());
    print("[Child] exit\n");
    exit();
}

static void parent_flow(int pid)
{
    int t0;
    int t1;

    print("\n[Parent] starting parent flow\n");
    print_int("[Parent] PID: ", g_parent_pid);
    print_int("[Parent] Child PID: ", pid);

    test_result("parent got child pid > 0 from fork", pid > 0);
    test_result("parent child pid != parent pid", pid != g_parent_pid);
    test_result("parent keeps its own memory copy after child fork", g_mem_probe == 2222);

    test_result("parent preemptive unblock(child) succeeds", unblock(pid) == 0);

    print("[Parent] waiting child to reach block #2...\n");
    burn_ticks(1800000);

    t0 = gettime();
    test_result("parent unblock(blocked child) succeeds", unblock(pid) == 0);
    burn_ticks(200000);
    t1 = gettime();
    test_result("gettime advances while parent runs", t1 >= t0);
    test_result("parent memory copy unaffected by child changes", g_mem_probe == 2222);

    burn_ticks(1200000);
}

int __attribute__((__section__(".text.main")))
main(void)
{
    int start_t;
    int end_t;
    int pid;
    int wr;
    int nread;
    char *info_msg;
    char rbuf[16];

    print("\n======= ZEOS USER FULL TEST =======\n");

    start_t = gettime();
    g_parent_pid = getpid();

    test_result("getpid() in parent > 0", g_parent_pid > 0);
    test_result("gettime() initial value >= 0", start_t >= 0);
    test_result("initial memory probe value", g_mem_probe == 1111);

    info_msg = "[INFO] write() basic output test\n";
    wr = write(1, info_msg, strlen(info_msg));
    burn_ticks(100000);

    test_result("write(valid args) returns byte count", wr == (int)strlen(info_msg));

    test_write_errors();
    test_read_errors();

    print("\n-- fork/block/unblock/exit tests --\n");
    pid = fork();

    if (pid < 0) {
        test_result("fork() succeeded", 0);
        perror();
        print("\n[SUMMARY] ");
        print_int("tests passed: ", g_pass);
        print_int("tests total : ", g_total);
        exit();
    }

    if (pid == 0) {
        child_flow();
    }

    g_child_pid = pid;
    g_mem_probe = 2222;
    parent_flow(pid);

    end_t = gettime();
    test_result("global runtime advanced", end_t > start_t);

    print("\n[SUMMARY] ");
    print_int("tests passed: ", g_pass);
    print_int("tests total : ", g_total);
    if (g_pass == g_total) {
        print("ALL TESTS PASS\n");
    } else {
        print("SOME TESTS FAILED\n");
    }

    print("\n-- professor manual checklist --\n");
    print("[STEP 1] Type a short word, then press F1.\n");
    print("[EXPECT] Dump shows the same word in FIFO order.\n");
    print("[STEP 2] Type more than 16 chars, then press F1.\n");
    print("[EXPECT] count stays at 16, new keys are discarded.\n");
    print("[STEP 3] Keep typing and press F1 multiple times.\n");
    print("[EXPECT] System stays stable and responsive.\n");

    print("\n[NOTE] The next demos consume keyboard input with read().\n");
    print("[NOTE] Use F1 before reaching them if you want to inspect the buffer.\n");

    print("\n[STEP 4] read_word() demo: type a word and finish with SPACE or ENTER.\n");
    print("[EXPECT] It waits until delimiter and prints only the word.\n");
    nread = read_word(rbuf, sizeof(rbuf));
    if (nread > 0) {
        write(1, "[READ WORD] ", 12);
        write(1, rbuf, strlen(rbuf));
        write(1, "\n", 1);
    }

    print("\n[STEP 5] Press one button to see read(), it has to block\n");
    nread = read(rbuf, sizeof(rbuf));
    if (nread > 0) {
        write(1, "[READ] ", 7);
        write(1, rbuf, strlen(rbuf));
        write(1, "\n", 1);
    }

    int pid1, pid2;
    char buffer[10];

    write(1, "\n--- LECTURE TEST MULTITASK ---\n", 36);

    pid1 = fork();
    if (pid1 == 0) {
        write(1, "[CHILD 1] I want 5 chars\n", 28);
        read(buffer, 5);
        write(1, "[CHILD 1] I have: ", 19);
        write(1, buffer, 5);
        write(1, "\n", 1);
        exit();
    }

    pid2 = fork();
    if (pid2 == 0) {
        write(1, "[CHILD 2] I want 5 chars\n", 28);
        read(buffer, 5);
        write(1, "[CHILD 2] I have: ", 19);
        write(1, buffer, 5);
        write(1, "\n", 1);
        exit();
    }

    write(1, "[PARENT] I want 5 chars\n", 27);
    read(buffer, 5);
    write(1, "[PARENT]   I have: ", 19);
    write(1, buffer, 5);
    write(1, "\n", 1);

    print("\n[INFO] End of automatic demos. You can keep typing and press F1 anytime.\n");

    while (1) ;
}
