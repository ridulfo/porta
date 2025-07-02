#include "pty_test.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>
#include <signal.h>

#define CTRL_KEY(k) ((k) & 0x1f)

static void cleanup_processes(void) {
    /* Kill any remaining porta processes */
    system("pkill -f './build/debug/porta' 2>/dev/null");
    sleep(1);
}

static void test_basic_functionality(void) {
    printf("Testing basic terminal interaction...\n");
    
    cleanup_processes();
    
    pty_test_t *pty = pty_test_create();
    pty_assert(pty != NULL, "Failed to create PTY");
    
    /* Set terminal size */
    pty_test_set_winsize(pty, 24, 80);
    
    /* Spawn porta with a test file */
    char *argv[] = {"./build/debug/porta", "test_basic.txt", NULL};
    int pid = pty_test_spawn_process(pty, "./build/debug/porta", argv);
    pty_assert(pid > 0, "Failed to spawn porta process");
    
    /* Wait for initial output - read in stages */
    for (int i = 0; i < 10; i++) {
        pty_test_read(pty, 500);
        usleep(200000);
        if (pty_test_contains(pty, "Welcome")) break;
    }
    
    printf("Got output (%zu bytes): '%.200s...'\n", pty->output_len, 
           pty->output_len > 0 ? pty->output_buffer : "(empty)");
    
    /* Check we got some output */
    pty_assert(pty->output_len > 0, "Should have some output");
    
    /* Check for alternate screen buffer */
    pty_assert(pty_test_contains_escape_seq(pty, "\033[?1049h"), "Should use alternate screen buffer");
    
    /* Check for welcome message */
    pty_assert(pty_test_contains(pty, "Welcome to PORTA"), "Should show welcome message");
    
    /* Dismiss splash screen */
    pty_test_send_key(pty, ' ');
    usleep(500000);
    pty_test_read(pty, 500);
    
    /* Type a character */
    pty_test_send_key(pty, 'A');
    usleep(200000);
    pty_test_read(pty, 200);
    
    /* Should contain the character somewhere */
    pty_assert(pty_test_contains(pty, "A"), "Should contain typed character");
    
    /* Send quit command */
    pty_test_send_ctrl_key(pty, 'q');
    usleep(500000);
    
    /* Wait for process to exit */
    int status;
    waitpid(pid, &status, 0);
    
    pty_test_destroy(pty);
    
    printf("✓ Basic functionality test passed\n");
}

static void test_save_functionality(void) {
    printf("Testing save functionality...\n");
    
    cleanup_processes();
    
    const char *test_file = "test_save_simple.txt";
    
    pty_test_t *pty = pty_test_create();
    pty_assert(pty != NULL, "Failed to create PTY");
    
    pty_test_set_winsize(pty, 24, 80);
    
    char *argv[] = {"./build/debug/porta", (char*)test_file, NULL};
    int pid = pty_test_spawn_process(pty, "./build/debug/porta", argv);
    pty_assert(pid > 0, "Failed to spawn porta process");
    
    /* Skip splash */
    sleep(1);
    pty_test_read(pty, 1000);
    pty_test_send_key(pty, ' ');
    usleep(500000);
    pty_test_read(pty, 500);
    
    /* Type some content */
    const char *content = "Test content";
    for (const char *c = content; *c; c++) {
        pty_test_send_key(pty, *c);
        usleep(50000);
    }
    
    /* Save */
    pty_test_send_ctrl_key(pty, 's');
    sleep(2); /* Wait for save to complete */
    
    /* Quit */
    pty_test_send_ctrl_key(pty, 'q');
    
    /* Wait for process to exit */
    int status;
    waitpid(pid, &status, 0);
    
    pty_test_destroy(pty);
    
    /* Verify file exists and has content */
    FILE *f = fopen(test_file, "r");
    pty_assert(f != NULL, "Saved file should exist");
    
    char buffer[256];
    fgets(buffer, sizeof(buffer), f);
    fclose(f);
    
    pty_assert(strstr(buffer, "Test content") != NULL, "File should contain saved content");
    
    /* Clean up test file */
    unlink(test_file);
    
    printf("✓ Save functionality test passed\n");
}

int main(void) {
    printf("Running simplified UI tests...\n\n");
    
    /* Ensure debug build exists */
    system("make debug >/dev/null 2>&1");
    
    test_basic_functionality();
    test_save_functionality();
    
    cleanup_processes();
    
    printf("\n✓ All simplified UI tests passed!\n");
    return 0;
}