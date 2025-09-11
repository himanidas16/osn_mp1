#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"
#include "kernel/fcntl.h"
/* ############## LLM Generated Code Begins ############## */
int
main(int argc, char *argv[])
{
  int initial_count, final_count;
  int fd;
  char buffer[100];
  int bytes_read;
  
  printf("=== getreadcount() System Call Test ===\n");
  
  // Get initial read count
  initial_count = getreadcount();
  printf("Initial read count: %d\n", initial_count);
  
  // Create a test file with exactly 100 bytes
  fd = open("testfile.txt", O_CREATE | O_WRONLY);
  if(fd < 0) {
    printf("ERROR: Failed to create test file\n");
    exit(1);
  }
  
  // Write exactly 100 'A' characters to the file
  for(int i = 0; i < 100; i++) {
    if(write(fd, "A", 1) != 1) {
      printf("ERROR: Failed to write to test file\n");
      close(fd);
      exit(1);
    }
  }
  close(fd);
  printf("Created test file with 100 bytes\n");
  
  // Read 100 bytes from the file
  fd = open("testfile.txt", O_RDONLY);
  if(fd < 0) {
    printf("ERROR: Failed to open test file for reading\n");
    exit(1);
  }
  
  bytes_read = read(fd, buffer, 100);
  close(fd);
  
  if(bytes_read < 0) {
    printf("ERROR: Failed to read from test file\n");
    exit(1);
  }
  
  printf("Successfully read %d bytes from test file\n", bytes_read);
  
  // Get final read count
  final_count = getreadcount();
  printf("Final read count: %d\n", final_count);
  
  // Verify the increase
  int increase = final_count - initial_count;
  printf("Increase in read count: %d\n", increase);
  
  if(increase == bytes_read) {
    printf("✓ SUCCESS: Read count increased by exactly %d bytes\n", bytes_read);
  } else {
    printf("✗ FAILURE: Expected increase of %d, but got %d\n", bytes_read, increase);
  }
  
  // Clean up
  unlink("testfile.txt");
  
  printf("=== Test completed ===\n");
  exit(0);
}
/* ############## LLM Generated Code Ends ################ */