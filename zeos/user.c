#include <libc.h>

static int child_pid_global = -1;

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

int __attribute__((__section__(".text.main")))
main(void)
{
    print("\n------- ZEOS USER TEST SUITE -------\n");

    int time1 = gettime();
    print_int("Start time: ", time1);

    print("Testing write & getpid...\n");
    print_int("My PID: ", getpid());

    print("Testing fork/block/unblock edge cases...\n");
    int pid = fork();

    if (pid < 0) {
        print("Fork failed!\n");
        perror();
    }
    else if (pid == 0) {
        print_int("[Child] PID: ", getpid());

        if (unblock(getpid()) < 0) print("[Child] unblock(self) should fail: PASS\n");
        else print("[Child] unblock(self) should fail: FAIL\n");

        if (unblock(1) < 0) print("[Child] unblock(parent) should fail: PASS\n");
        else print("[Child] unblock(parent) should fail: FAIL\n");

        if (unblock(9999) < 0) print("[Child] unblock(invalid pid) should fail: PASS\n");
        else print("[Child] unblock(invalid pid) should fail: FAIL\n");

        print("[Child] Delay before block #1 (preemptive unblock expected)...\n");
        for (int i = 0; i < 1200000; i++) {}

        print("[Child] block #1 should NOT sleep\n");
        block();
        print("[Child] block #1 PASS\n");

        print("[Child] block #2 should sleep until parent unblocks\n");
        int t_before = gettime();
        block();
        int t_after = gettime();

        if (t_after > t_before) print("[Child] block #2 slept and woke: PASS\n");
        else print("[Child] block #2 slept and woke: FAIL\n");

        print_int("[Child] End time: ", gettime());
        print("[Child] exit\n");
        exit();
    }
    else {
        child_pid_global = pid;
        print_int("[Parent] Child PID: ", child_pid_global);

        if (unblock(child_pid_global) == 0) print("[Parent] preemptive unblock should pass: PASS\n");
        else print("[Parent] preemptive unblock should pass: FAIL\n");

        print("[Parent] Delay to let child reach block #2...\n");
        for (int i = 0; i < 1800000; i++) {}

        if (unblock(child_pid_global) == 0) print("[Parent] unblock(blocked child) should pass: PASS\n");
        else print("[Parent] unblock(blocked child) should pass: FAIL\n");

        print("[Parent] Final delay...\n");
        for (int i = 0; i < 2000000; i++) {}

        print_int("[Parent] End time: ", gettime());
        print("[Parent] exit\n");
        exit();
    }

    while (1)
        ;
}
