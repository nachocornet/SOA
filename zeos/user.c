#include <libc.h>

static volatile int shared_probe = 111;

static void print_text(char *text)
{
  write(1, text, strlen(text));
}

static void print_int_line(char *label, int value)
{
  char n[16];
  write(1, label, strlen(label));
  itoa(value, n);
  write(1, n, strlen(n));
  write(1, "\n", 1);
}

int __attribute__ ((__section__(".text.main")))
  main(void)
{
  int parent_pid = getpid();
  int ret;

  print_text("=== TEST FORK ZEOS ===\n");
  print_int_line("Parent PID: ", parent_pid);

  print_text("[1] Basic fork return test\n");
  ret = fork();
  if (ret < 0) {
    print_text("fork() failed in basic test\n");
    perror();
    for(;;) {}
  }

  if (ret == 0) {
    int child_pid = getpid();
    print_text("Child branch reached\n");
    print_int_line("fork() returned: ", ret);
    print_int_line("Child PID: ", child_pid);
    shared_probe = 222;
    for(;;) {}
  }

  print_text("Parent branch reached\n");
  print_int_line("fork() returned child PID: ", ret);

  for (volatile int k = 0; k < 20000000; ++k) {}
  if (shared_probe == 111) print_text("Memory isolation OK\n");
  else print_text("Memory isolation FAILED\n");

  print_text("[2] Linear multi-fork test (parent-only)\n");
  {
    const int max_children = 3;
    int created = 0;

    for (int i = 0; i < max_children; ++i) {
      int r = fork();
      if (r < 0) {
        print_text("fork() failed in linear test\n");
        perror();
        break;
      }
      if (r == 0) {
        print_text("Linear child started\n");
        for(;;) {}
      }
      created++;
      print_int_line("Created child PID: ", r);
    }

    print_int_line("Total created in linear test: ", created);
  }

  print_text("=== END TEST ===\n");

  for(;;) {}
}
