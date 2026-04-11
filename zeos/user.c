#include <libc.h>

void print(char* msg) {
    write(1, msg, strlen(msg));
}

void print_int(char* msg, int val) {
    char buff[16];
    print(msg);
    itoa(val, buff);
    print(buff);
    print("\n");
}

int __attribute__ ((__section__(".text.main")))
  main(void)
{
    print("\n------- ZEOS USER TEST SUITE -------\n");

    int time1 = gettime();
    print_int("Start time: ", time1);

    print("Testing write & getpid...\n");
    print_int("My PID: ", getpid());

    print("Testing fork...\n");
    int pid = fork();

    if (pid < 0) {
        print("Fork failed!\n");
        perror();
    }
    else if (pid == 0) {
        // Processo Hijo
        print_int("[Child] Hello! My PID is: ", getpid());
        // Caso 1: Testeando 'pending_unblocks'
        print("[Child] Delaying to let parent unblock me first...\n");
        for (int i = 0; i < 1000000; i++) {}
        print("[Child] Now calling block(). I should NOT sleep because of pending_unblocks!\n");
        block();
        print("[Child] Survived block() successfully!\n");

        // Caso 2: Bloqueo normal
        print("[Child] Testing normal block(). Sleeping...\n");
        block(); 
        
        print("[Child] Woke up from normal block! Succesfully unblocked.\n");
        print_int("[Child] End time: ", gettime());
        print("[Child] Testing exit(). Goodbye!\n");
        exit();
    }
    else {
        // Processo Padre
        print_int("[Parent] I created a child with PID: ", pid);
        
        // Caso 1: Testeando 'pending_unblocks'
        print("[Parent] Testing unblock() on child before it blocks (pending_unblock)...\n");
        unblock(pid);

        // Caso 2: Bloqueo normal
        print("[Parent] Delaying to give child time to block normally...\n");
        for (int i = 0; i < 1000000; i++) {}
        
        print("[Parent] Testing unblock() on blocked child...\n");
        unblock(pid);
        
        print("[Parent] Delaying to let child finish...\n");
        for (int i = 0; i < 1000000; i++) {}
        
        print_int("[Parent] End time: ", gettime());
        print("[Parent] Testing exit(). Goodbye!\n");
        exit();
    }
    
    while(1);
}
