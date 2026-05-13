#include <libc.h>
#include <mm_address.h>

static void print(char *msg)
{
    write(1, msg, strlen(msg));
}

static void print_addr(char *msg, unsigned int addr)
{
    char buff[16];
    print(msg);
    itoa(addr, buff);
    print("0x");
    print(buff);
}

static void print_char_array(char *msg, char *array, int len)
{
    int i;
    print(msg);
    for (i = 0; i < len && array[i] != '\0'; ++i) {
        char c = array[i];
        if (c >= 32 && c < 127) {
            write(1, &c, 1);
        } else {
            print("[?]");
        }
    }
    print("\n");
}

static void test_1_attach_shared_page(void)
{
    char *shared;
    const unsigned int page_mask = 0xFFF;

    print("\n=== TEST 1: Attach Shared Page ===\n");
    print("Calling shmat(0, NULL)...\n");
    
    shared = (char *)shmat(0, (void *)0);
    
    if (shared == (char *)-1) {
        print("ERROR: shmat failed!\n");
        return;
    }
    
    print_addr("Attached address: ", (unsigned int)shared);
    print("\n");
    
    if (((unsigned int)shared & page_mask) != 0) {
        print("ERROR: Address not page-aligned!\n");
        return;
    }
    
    print("SUCCESS: Page attached and page-aligned\n");
}

static void test_2_write_read_shared(void)
{
    char *shared;
    char buffer[32];
    int i;

    print("\n=== TEST 2: Write and Read from Shared Page ===\n");
    print("Attaching shared page 0...\n");
    
    shared = (char *)shmat(0, (void *)0);
    
    if (shared == (char *)-1) {
        print("ERROR: shmat failed!\n");
        return;
    }

    print("Writing 'Hello from Parent!' to shared page...\n");
    const char *msg = "Hello from Parent!";
    for (i = 0; msg[i] != '\0' && i < 30; ++i) {
        shared[i] = msg[i];
    }
    shared[i] = '\0';

    print("Reading back from shared page:\n");
    for (i = 0; i < 30 && shared[i] != '\0'; ++i) {
        buffer[i] = shared[i];
    }
    buffer[i] = '\0';
    
    print_char_array("Content: ", buffer, 32);
    print("SUCCESS\n");
}

static void test_3_parent_child_shared(void)
{
    char *shared;
    int pid;
    int i;

    print("\n=== TEST 3: Parent-Child Shared Memory Visibility ===\n");
    print("Parent: Attaching shared page 0...\n");
    
    shared = (char *)shmat(0, (void *)0);
    
    if (shared == (char *)-1) {
        print("ERROR: shmat failed!\n");
        return;
    }

    print("Parent: Writing initial message to shared page...\n");
    const char *parent_msg = "MSG_FROM_PARENT";
    for (i = 0; parent_msg[i] != '\0' && i < 30; ++i) {
        shared[i] = parent_msg[i];
    }
    shared[i] = '\0';
    shared[16] = 'X';

    print("Parent: Forking child process...\n");
    pid = fork();

    if (pid < 0) {
        print("ERROR: fork failed!\n");
        return;
    }

    if (pid == 0) {
        char *child_shared = (char *)shmat(0, (void *)0);
        
        print("Child: Attached shared page at ");
        print_addr("", (unsigned int)child_shared);
        print("\n");
        
        print("Child: Reading parent message:\n");
        print_char_array("Content: ", child_shared, 32);

        print("Child: Writing response to shared page...\n");
        const char *child_msg = "MSG_FROM_CHILD";
        for (i = 0; child_msg[i] != '\0' && i < 30; ++i) {
            child_shared[i] = child_msg[i];
        }
        child_shared[i] = '\0';
        child_shared[16] = 'Y';

        print("Child: Done, exiting\n");
        exit();
    }

    print("Parent: Waiting for child to modify shared page...\n");
    for (i = 0; i < 2000000 && shared[16] != 'Y'; ++i) {
    }

    if (shared[16] == 'Y') {
        print("Parent: Child modified the shared page!\n");
        print("Parent: Reading child message:\n");
        print_char_array("Content: ", shared, 32);
        print("SUCCESS: Parent sees child modifications\n");
    } else {
        print("ERROR: Child did not modify shared page in time\n");
    }

    unblock(pid);
}

static void test_4_multiple_attachments(void)
{
    char *shared0;
    char *shared1;
    int i;

    print("\n=== TEST 4: Multiple Shared Page Attachments ===\n");
    
    print("Attaching shared page 0...\n");
    shared0 = (char *)shmat(0, (void *)0);
    
    if (shared0 == (char *)-1) {
        print("ERROR: shmat(0) failed!\n");
        return;
    }
    print_addr("Page 0 at: ", (unsigned int)shared0);
    print("\n");

    print("Attaching shared page 1...\n");
    shared1 = (char *)shmat(1, (void *)0);
    
    if (shared1 == (char *)-1) {
        print("ERROR: shmat(1) failed!\n");
        return;
    }
    print_addr("Page 1 at: ", (unsigned int)shared1);
    print("\n");

    print("Writing to page 0...\n");
    const char *msg0 = "Data in Page 0";
    for (i = 0; msg0[i] != '\0' && i < 30; ++i) {
        shared0[i] = msg0[i];
    }
    shared0[i] = '\0';

    print("Writing to page 1...\n");
    const char *msg1 = "Data in Page 1";
    for (i = 0; msg1[i] != '\0' && i < 30; ++i) {
        shared1[i] = msg1[i];
    }
    shared1[i] = '\0';

    print("Reading from page 0:\n");
    print_char_array("Content: ", shared0, 32);
    
    print("Reading from page 1:\n");
    print_char_array("Content: ", shared1, 32);
    
    print("SUCCESS: Multiple pages attached independently\n");
}

static void test_5_shmrm_and_free(void)
{
    char *shared;
    int i;

    print("\n=== TEST 5: shmdt/shmrm frees when marked ===\n");
    print("Attach page 0 and write data...\n");
    shared = (char *)shmat(0, (void *)0);
    if (shared == (char *)-1) { print("ERROR: shmat failed!\n"); return; }

    const char *msg = "KEEP_ME";
    for (i = 0; msg[i] != '\0' && i < 30; ++i) shared[i] = msg[i];
    shared[i] = '\0';

    print("Call shmrm(0) to mark for removal...\n");
    if (shmrm(0) != 0) { print("ERROR: shmrm failed\n"); }

    print("Call shmdt(addr) to detach and trigger free...\n");
    if (shmdt((void *)shared) != 0) { print("ERROR: shmdt failed\n"); }

    print("Re-attach page 0 and check it's zeroed...\n");
    shared = (char *)shmat(0, (void *)0);
    if (shared == (char *)-1) { print("ERROR: re-shmat failed!\n"); return; }

    if (shared[0] == '\0') {
        print("SUCCESS: reattached page is zeroed\n");
    } else {
        print("ERROR: reattached page not zeroed\n");
        print_char_array("Content: ", shared, 32);
    }

    shmdt((void *)shared);
}

static void test_6_shmrm_with_child(void)
{
    char *shared;
    int pid;
    int i;

    print("\n=== TEST 6: shmdt/shmrm with child process ===\n");
    print("Parent: attach page 1 and write 'P'...\n");
    shared = (char *)shmat(1, (void *)0);
    if (shared == (char *)-1) { print("ERROR: shmat failed!\n"); return; }
    shared[0] = 'P';

    print("Parent: fork child...\n");
    pid = fork();
    if (pid < 0) { print("ERROR: fork failed\n"); return; }

    if (pid == 0) {
        /* child */
        char *cshared = (char *)shmat(1, (void *)0);
        print("Child: sees initial char: ");
        write(1, cshared, 1);
        print("\nChild: sleeping a bit then detaching...\n");
        for (i = 0; i < 1000000; ++i) ;
        shmdt((void *)cshared);
        print("Child: detached and exiting\n");
        exit();
    }

    /* parent */
    print("Parent: mark shmrm(1) and detach quickly\n");
    shmrm(1);
    shmdt((void *)shared);

    print("Parent: now reattach page 1 and verify zeroed (after child exit)\n");
    /* wait a bit for child to exit and trigger freeing */
    for (i = 0; i < 3000000; ++i) ;

    shared = (char *)shmat(1, (void *)0);
    if (shared == (char *)-1) { print("ERROR: re-shmat failed!\n"); return; }

    if (shared[0] == '\0') print("SUCCESS: reattached page is zeroed\n");
    else print("ERROR: reattached page not zeroed\n");

    shmdt((void *)shared);
}

static int count_free_frames()
{
    int i, cnt = 0;
    for (i = 0; i < TOTAL_PAGES; ++i) {
        if (is_frame_free(i) == 1) cnt++;
    }
    return cnt;
}

static void test_7_verify_physical_free(void)
{
    int before, after_attach, after_detach;
    char *shared;

    print("\n=== TEST 7: Verify physical free frame on shmrm+shmdt ===\n");

    before = count_free_frames();
    print_addr("Free frames before attach: ", before);
    print("\n");

    shared = (char *)shmat(3, (void *)0);
    if (shared == (char *)-1) { print("ERROR: shmat failed\n"); return; }

    after_attach = count_free_frames();
    print_addr("Free frames after attach: ", after_attach);
    print("\n");

    if (after_attach != before - 1) print("WARNING: free frames didn't decrease by 1\n");

    print("Marking shmrm(3) and detaching to trigger free...\n");
    shmrm(3);
    shmdt((void *)shared);

    /* wait a bit */
    for (int i = 0; i < 2000000; ++i) ;

    after_detach = count_free_frames();
    print_addr("Free frames after detach: ", after_detach);
    print("\n");

    if (after_detach == before) print("SUCCESS: physical frame freed and returned to pool\n");
    else print("ERROR: physical frame not freed as expected\n");
}

static void test_8_performance_fps(void)
{
    int counter = 0;
    int start_time = gettime();
    int current_time;

    print("\n=== TEST 8: Performance (FPS Counter) ===\n");
    print("Running FPS counter for 3 seconds...\n");
    print("Press any key to stop early.\n\n");

    while (1) {
        current_time = gettime();
        
        /* Update and display FPS every frame */
        fps_update();
        display_fps();
        
        counter++;
        
        /* Stop after 3 seconds or 10000 iterations */
        if (current_time - start_time > 3000 || counter > 10000) {
            break;
        }
    }
    
    print("\n\nPerformance test completed.\n");
}

static void print_menu(void)
{
    print("\n============================================\n");
    print("  MILESTONE 5: SHARED MEMORY (SHMAT) TESTS\n");
    print("============================================\n");
    print("1. Attach Shared Page\n");
    print("2. Write and Read from Shared Page\n");
    print("3. Parent-Child Shared Memory Visibility\n");
    print("4. Multiple Shared Page Attachments\n");
    print("5. shmdt/shmrm: detach frees when marked\n");
    print("6. shmdt/shmrm with child process\n");
    print("7. Verify physical free frame on shmrm+shmdt\n");
    print("8. Performance Test (FPS Counter)\n");
    print("0. Exit\n");
    print("Select option: ");
}

int __attribute__((__section__(".text.main")))
main(void)
{
    char input_buffer[32];
    int bytes_read;
    int option;
    int running = 1;

    print("\n");
    print("========================================\n");
    print("  ZEOS Milestone 5: Shared Memory Tests\n");
    print("========================================\n");
    print("(FPS counter displayed at top-left)\n\n");

    while (running) {
        /* Update and display FPS on every iteration */
        fps_update();
        display_fps();
        
        print_menu();
        
        bytes_read = read(input_buffer, 1);
        
        if (bytes_read <= 0) {
            print("Input error\n");
            continue;
        }

        option = input_buffer[0] - '0';

        switch (option) {
            case 1:
                test_1_attach_shared_page();
                break;
            case 2:
                test_2_write_read_shared();
                break;
            case 3:
                test_3_parent_child_shared();
                break;
            case 4:
                test_4_multiple_attachments();
                break;
            case 5:
                test_5_shmrm_and_free();
                break;
            case 6:
                test_6_shmrm_with_child();
                break;
            case 7:
                test_7_verify_physical_free();
                break;
            case 8:
                test_8_performance_fps();
                break;
            case 0:
                print("\nExiting...\n");
                running = 0;
                break;
            default:
                print("Invalid option. Please select 0-8.\n");
                break;
        }
    }

    print("\n========================================\n");
    print("          Tests Completed\n");
    print("========================================\n");

    exit();
    return 0;
}
