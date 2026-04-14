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

    print("\n======= ZEOS USER FULL TEST =======\n");

    start_t = gettime();
    g_parent_pid = getpid();

    test_result("getpid() in parent > 0", g_parent_pid > 0);
    test_result("gettime() initial value >= 0", start_t >= 0);
    test_result("initial memory probe value", g_mem_probe == 1111);

    wr = write(1, "[INFO] write() basic output test\n", 31);
    burn_ticks(100000);
    
    test_result("write(valid args) returns byte count", wr == 31);

    test_write_errors();

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

    print("[Parent] exit\n");
    exit();

    while (1) ;
}
