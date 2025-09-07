#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"

void
cpu_intensive_process(int id)
{
  printf("CPU-intensive process %d starting\n", id);
  volatile int dummy = 0;
  
  // Very heavy workload to ensure processes get demoted and stay in lower queues
  for(int i = 0; i < 5000000; i++) {
    dummy += i * i;
    for(int j = 0; j < 50; j++) {
      dummy += j * i;
    }
    
    // Print progress every 1 million iterations
    if(i % 1000000 == 0) {
      printf("Process %d: iteration %d million\n", id, i/1000000);
    }
  }
  
  printf("CPU-intensive process %d finished\n", id);
  exit(0);
}

// Add this function to user/starvationtest.c
void test_48_tick_timing() {
  printf("=== 48-Tick Starvation Prevention Verification ===\n");
  
  int start_time = uptime();
  printf("Test started at tick: %d\n", start_time);
  
  // Create processes that will demonstrate starvation prevention
  for(int i = 1; i <= 3; i++) {
    if(fork() == 0) {
      // Long CPU work to get demoted and trigger starvation prevention
      for(volatile long j = 0; j < 50000000L; j++) {
        if(j % 10000000 == 0) {
          printf("Process %d: %ldM iterations at tick %d\n", i, j/1000000, uptime());
        }
      }
      exit(0);
    }
  }
  
  // Parent waits and monitors
  for(int i = 0; i < 3; i++) {
    wait(0);
  }
  
  int end_time = uptime();
  printf("Test completed at tick: %d (duration: %d ticks)\n", end_time, end_time - start_time);
  printf("Expected to see starvation prevention every 48 ticks\n");
}

int main(int argc, char *argv[]) {
  if(argc > 1 && strcmp(argv[1], "timing") == 0) {
    test_48_tick_timing();
  } else {
    printf("=== Starvation Prevention Test ===\n");
    printf("Starting multiple CPU-intensive processes to trigger starvation prevention...\n");
    
    // Start 3 CPU-intensive processes
    for(int i = 1; i <= 3; i++) {
      if(fork() == 0) {
        cpu_intensive_process(i);
      }
    }
    
    // Wait for all children to complete
    for(int i = 0; i < 3; i++) {
      wait(0);
    }
    
    printf("=== Starvation test completed ===\n");
  }
  exit(0);
}


