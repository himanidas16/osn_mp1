#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"

void
simple_work(int process_id)
{
  int start_time = uptime();
  volatile int dummy = 0;

  printf("Process %d starting\n", process_id);

  // Simple, finite workload
  for(int i = 0; i < 5000000; i++) {
    dummy += i;
  }

  int end_time = uptime();
  printf("Process %d finished (duration: %d ticks)\n", 
         process_id, end_time - start_time);
  exit(0);
}

int
main(int argc, char *argv[])
{
  printf("=== Simple MLFQ Test ===\n");

  for(int i = 1; i <= 3; i++) {
    if(fork() == 0) {
      simple_work(i);
    }
  }

  // Wait for all children
  for(int i = 0; i < 3; i++) {
    wait(0);
  }

  printf("=== All processes completed ===\n");
  exit(0);
}
