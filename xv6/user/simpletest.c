#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"

int
main(int argc, char *argv[])
{
  printf("=== Simple Scheduler Test ===\n");
  
  int pid1, pid2, pid3;
  int start_time = uptime();
  
  printf("Creating process 1...\n");
  if((pid1 = fork()) == 0) {
    int my_start = uptime();
    printf("Process 1 (PID %d) started at time %d\n", getpid(), my_start);
    
    // More CPU intensive work
    volatile int dummy = 0;
    for(int i = 0; i < 200000000; i++) {
      // busy work - force more computation
      dummy += i * i;
    }
    // Use dummy to prevent optimization
    dummy = dummy + 1;
    
    int my_end = uptime();
    printf("Process 1 (PID %d) finished at time %d (duration: %d)\n", 
           getpid(), my_end, my_end - my_start);
    exit(0);
  }
  
  // Delay to ensure different creation times
  for(int i = 0; i < 5000000; i++);
  
  printf("Creating process 2...\n");
  if((pid2 = fork()) == 0) {
    int my_start = uptime();
    printf("Process 2 (PID %d) started at time %d\n", getpid(), my_start);
    
    // More CPU intensive work
    volatile int dummy = 0;
    for(int i = 0; i < 200000000; i++) {
      // busy work - force more computation
      dummy += i * i;
    }
    // Use dummy to prevent optimization
    dummy = dummy + 1;
    
    int my_end = uptime();
    printf("Process 2 (PID %d) finished at time %d (duration: %d)\n", 
           getpid(), my_end, my_end - my_start);
    exit(0);
  }
  
  // Delay to ensure different creation times
  for(int i = 0; i < 5000000; i++);
  
  printf("Creating process 3...\n");
  if((pid3 = fork()) == 0) {
    int my_start = uptime();
    printf("Process 3 (PID %d) started at time %d\n", getpid(), my_start);
    
    // More CPU intensive work
    volatile int dummy = 0;
    for(int i = 0; i < 200000000; i++) {
      // busy work - force more computation
      dummy += i * i;
    }
    // Use dummy to prevent optimization
    dummy = dummy + 1;
    
    int my_end = uptime();
    printf("Process 3 (PID %d) finished at time %d (duration: %d)\n", 
           getpid(), my_end, my_end - my_start);
    exit(0);
  }
  
  // Parent waits for all children
  wait(0);
  wait(0);
  wait(0);
  
  int total_time = uptime() - start_time;
  printf("=== All processes completed in %d ticks ===\n", total_time);
  
  exit(0);
}