#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"

void
very_long_process(int id)
{
  printf("Very long process %d starting\n", id);
  volatile int dummy = 0;
  
  // Much more intensive work to reach Queue 3
  for(int i = 0; i < 2000000; i++) {  // Increased from 200000
    dummy += i * i;
    for(int j = 0; j < 100; j++) {    // Added inner loop
      dummy += j;
    }
    if (i % 500000 == 0) {
      printf("Process %d still running (iteration %d)\n", id, i);
    }
  }
  
  printf("Very long process %d finished\n", id);
  exit(0);
}

void
medium_process(int id)
{
  printf("Medium process %d starting\n", id);
  volatile int dummy = 0;
  
  // Medium work to reach Queue 2
  for(int i = 0; i < 100000; i++) {
    dummy += i * i;
  }
  
  printf("Medium process %d finished\n", id);
  exit(0);
}

void
short_process(int id)
{
  printf("Short process %d starting\n", id);
  volatile int dummy = 0;
  
  for(int i = 0; i < 10000; i++) {
    dummy += i;
  }
  
  printf("Short process %d finished\n", id);
  exit(0);
}

int
main(int argc, char *argv[])
{
  printf("=== Complete MLFQ Feature Test ===\n");
  
  // Start multiple long processes to fill lower queues
  for(int i = 1; i <= 2; i++) {
    if(fork() == 0) {
      very_long_process(i);
    }
  }
  
  // Start medium processes
  for(int i = 3; i <= 4; i++) {
    if(fork() == 0) {
      medium_process(i);
    }
  }
  
  // Give time for processes to get demoted
  for(int i = 0; i < 10000000; i++);
  
  // Start high-priority process to demonstrate preemption
  if(fork() == 0) {
    short_process(5);
  }
  
  // Wait for all processes
  for(int i = 0; i < 5; i++) {
    wait(0);
  }
  
  printf("=== Complete test finished ===\n");
  exit(0);
}
