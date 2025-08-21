#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"

int main() {
    printf("=== CFS Scheduler Test ===\n");
    
    for(int i = 1; i <= 3; i++) {
        int pid = fork();
        if(pid == 0) {
            printf("CFS Process %d (PID=%d) starting\n", i, getpid());
            
            // Do CPU work
            volatile int sum = 0;
            for(int j = 0; j < 500000; j++) {
                sum += j;
            }
            
            printf("CFS Process %d (PID=%d) finished\n", i, getpid());
            exit(0);
        }
        
        // Small delay between forks
        volatile int delay = 0;
        for(int k = 0; k < 25000; k++) {
            delay++;
        }
    }
    
    // Wait for all children
    for(int i = 0; i < 3; i++) {
        wait(0);
    }
    
    printf("=== CFS Test completed ===\n");
    exit(0);
}