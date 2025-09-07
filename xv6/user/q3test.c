#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"

void
extreme_cpu_hog(int id)
{
  printf("CPU HOG %d: Starting - will definitely reach Queue 3\n", id);
  volatile long long dummy = 0;
  
  // Extreme CPU usage to guarantee Queue 3
  for(long long i = 0; i < 200000000LL; i++) {
    dummy += i * i * i;
    
    if(i % 50000000 == 0) {
      printf("CPU HOG %d: %lld0 million iterations\n", id, i/10000000);
    }
  }
  
  printf("CPU HOG %d: COMPLETED\n", id);
  exit(0);
}

int
main(int argc, char *argv[])
{
  printf("\n=== QUEUE 3 ROUND-ROBIN VERIFICATION ===\n");
  
  // Start 4 extreme CPU hogs
  for(int i = 1; i <= 4; i++) {
    printf("Starting CPU HOG %d\n", i);
    if(fork() == 0) {
      extreme_cpu_hog(i);
    }
  }
  
  printf("All CPU hogs started - they should reach Queue 3 and show round-robin\n");
  
  for(int i = 0; i < 4; i++) {
    wait(0);
  }
  
  printf("=== VERIFICATION COMPLETE ===\n");
  exit(0);
}
