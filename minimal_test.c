#include "pty_test.h"
#include <stdio.h>
#include <unistd.h>
#include <sys/wait.h>
#include <signal.h>

int main(void) {
    printf("Testing PTY framework with simple commands...\n");
    
    /* Test 1: Basic PTY creation */
    pty_test_t *pty = pty_test_create();
    if (!pty) {
        printf("❌ PTY creation failed\n");
        return 1;
    }
    printf("✓ PTY creation works\n");
    
    /* Test 2: Simple echo command */
    char *argv[] = {"echo", "Hello PTY", NULL};
    int pid = pty_test_spawn_process(pty, "echo", argv);
    if (pid <= 0) {
        printf("❌ Process spawn failed\n");
        pty_test_destroy(pty);
        return 1;
    }
    printf("✓ Process spawn works\n");
    
    /* Read output */
    sleep(1);
    pty_test_read(pty, 1000);
    
    printf("Echo output (%zu bytes): '%s'\n", pty->output_len, pty->output_buffer);
    
    if (pty_test_contains(pty, "Hello PTY")) {
        printf("✓ Echo test passed\n");
    } else {
        printf("❌ Echo test failed\n");
    }
    
    /* Wait for process to finish */
    wait(NULL);
    pty_test_destroy(pty);
    
    /* Test 3: Try running porta briefly */
    printf("\nTesting porta startup...\n");
    pty = pty_test_create();
    pty_test_set_winsize(pty, 24, 80);
    
    char *porta_argv[] = {"./build/debug/porta", "test_minimal.txt", NULL};
    pid = pty_test_spawn_process(pty, "./build/debug/porta", porta_argv);
    
    if (pid <= 0) {
        printf("❌ Porta spawn failed\n");
        pty_test_destroy(pty);
        return 1;
    }
    
    printf("✓ Porta spawned (PID: %d)\n", pid);
    
    /* Try to read some initial output */
    for (int i = 0; i < 5; i++) {
        pty_test_read(pty, 200);
        usleep(200000);
        if (pty->output_len > 0) break;
    }
    
    printf("Porta initial output (%zu bytes): '%.100s'\n", 
           pty->output_len, pty->output_len > 0 ? pty->output_buffer : "(none)");
    
    /* Send quit command quickly */
    pty_test_send_ctrl_key(pty, 'q');
    usleep(500000);
    
    /* Force kill if still running */
    kill(pid, SIGTERM);
    wait(NULL);
    
    pty_test_destroy(pty);
    
    printf("✓ Minimal tests complete\n");
    return 0;
}