#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"

void
cpu_intensive_task(int process_id, int duration)
{
  int start_time = uptime();
  int current_time;
  int count = 0;
  
  printf("Process %d starting at time %d\n", process_id, start_time);
  
  do {
    // CPU intensive loop
    for(int i = 0; i < 1000000; i++) {
      count++;
    }
    current_time = uptime();
  } while(current_time - start_time < duration);
  
  printf("Process %d finished at time %d (ran for %d ticks)\n", 
         process_id, current_time, current_time - start_time);
}

int
main(int argc, char *argv[])
{
  printf("=== FCFS Scheduler Test ===\n");
  printf("Creating 3 processes that should run in creation order\n");
  
  int pid1, pid2, pid3;
  
  // Create first process
  if((pid1 = fork()) == 0) {
    cpu_intensive_task(1, 20);  // Run for 20 ticks
    exit(0);
  }
  
  // Small delay to ensure different creation times
  for(int i = 0; i < 1000000; i++);
  
  // Create second process  
  if((pid2 = fork()) == 0) {
    cpu_intensive_task(2, 20);  // Run for 20 ticks
    exit(0);
  }
  
  // Small delay to ensure different creation times
  for(int i = 0; i < 1000000; i++);
  
  // Create third process
  if((pid3 = fork()) == 0) {
    cpu_intensive_task(3, 20);  // Run for 20 ticks
    exit(0);
  }
  
  // Parent waits for all children
  wait(0);
  wait(0);
  wait(0);
  
  printf("=== Test completed ===\n");
  printf("In FCFS: Process 1 should start first, then 2, then 3\n");
  printf("In Round Robin: Processes should interleave\n");
  
  exit(0);
}