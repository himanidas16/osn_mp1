#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"

int main() {
    printf("=== FCFS Scheduler Test ===\n");
    
    int start_time = uptime();
    
    for(int i = 1; i <= 3; i++) {
        int pid = fork();
        if(pid == 0) {
            int my_start = uptime();
            printf("Process %d PID=%d started at time %d\n", i, getpid(), my_start);
            
            // Do CPU work
            volatile int sum = 0;
            for(int j = 0; j < 200000; j++) {
                sum += j;
            }
            
            int my_end = uptime();
            printf("Process %d PID=%d finished at time %d (duration=%d)\n", 
                   i, getpid(), my_end, my_end - my_start);
            exit(0);
        }
    }
    
    // Wait for all children
    for(int i = 0; i < 3; i++) {
        wait(0);
    }
    
    int end_time = uptime();
    printf("=== All completed in %d ticks ===\n", end_time - start_time);
    exit(0);
}