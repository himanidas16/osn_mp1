#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"

void
long_running_process(int id)
{
  printf("Long-running process %d starting\n", id);
  volatile int dummy = 0;

  // Long CPU-intensive work to get demoted to lower queues
  for(int i = 0; i < 50000; i++) {
    dummy += i * i;
    if (i % 10000 == 0) {
      printf("Process %d still running (iteration %d)\n", id, i);
    }
  }

  printf("Long-running process %d finished\n", id);
  exit(0);
}

void
short_process(int id)
{
  printf("Short process %d starting (should preempt!)\n", id);
  volatile int dummy = 0;

  // Short work - should stay in high priority queue
  for(int i = 0; i < 1000; i++) {
    dummy += i;
  }

  printf("Short process %d finished\n", id);
  exit(0);
}

int
main(int argc, char *argv[])
{
  printf("=== MLFQ Preemption Test ===\n");

  // Start a long-running process first
  if(fork() == 0) {
    long_running_process(1);
  }

  // Give it time to get demoted to lower queues
  for(int i = 0; i < 5000000; i++);

  // Start a short process - should preempt the long one
  if(fork() == 0) {
    short_process(2);
  }

  // Wait for both to complete
  wait(0);
  wait(0);

  printf("=== Preemption test completed ===\n");
  exit(0);
}
