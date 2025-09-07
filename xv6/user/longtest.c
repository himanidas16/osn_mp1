#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"

void
very_long_cpu_process(int id)
{
  printf("=== LONG Process %d: Starting (will reach Queue 3) ===\n", id);
  volatile int dummy = 0;
  
  // Much more intensive work to definitely reach Queue 3
  for(int i = 0; i < 100000000; i++) {  // 100 million iterations
    dummy += i * i;
    for(int j = 0; j < 10; j++) {  // Inner loop for more CPU time
      dummy += j * i;
    }
    
    // Print progress less frequently to reduce output clutter
    if(i % 20000000 == 0) {
      printf("LONG Process %d: %d0 million iterations (should be in lower queue)\n", 
             id, i/10000000);
    }
  }
  
  printf("=== LONG Process %d: FINISHED ===\n", id);
  exit(0);
}

void
short_preempting_process(int id)
{
  printf("*** HIGH PRIORITY Process %d: Starting (should preempt) ***\n", id);
  volatile int dummy = 0;
  
  for(int i = 0; i < 500000; i++) {
    dummy += i;
  }
  
  printf("*** HIGH PRIORITY Process %d: FINISHED QUICKLY ***\n", id);
  exit(0);
}

int
main(int argc, char *argv[])
{
  printf("\n======================================\n");
  printf("MLFQ QUEUE 3 & PREEMPTION TEST\n");
  printf("======================================\n");
  
  // Start multiple long-running processes
  printf("Starting 3 VERY long CPU processes...\n");
  for(int i = 1; i <= 3; i++) {
    if(fork() == 0) {
      very_long_cpu_process(i);
    }
    
    // Small delay between process creation
    for(int j = 0; j < 3000000; j++);
  }
  
  printf("Letting processes run and get demoted to Queue 3...\n");
  
  // Let them run for a while to reach Queue 3
  for(int i = 0; i < 30000000; i++);
  
  printf("\n*** NOW STARTING HIGH PRIORITY PROCESS - Should preempt! ***\n");
  
  // Start high priority process (should preempt)
  if(fork() == 0) {
    short_preempting_process(99);
  }
  
  // Wait a bit more
  for(int i = 0; i < 10000000; i++);
  
  // Start another preempting process
  if(fork() == 0) {
    short_preempting_process(100);
  }
  
  printf("Waiting for all processes to complete...\n");
  
  // Wait for all processes
  for(int i = 0; i < 5; i++) {
    wait(0);
  }
  
  printf("\n======================================\n");
  printf("QUEUE 3 & PREEMPTION TEST COMPLETED\n");
  printf("======================================\n");
  exit(0);
}
