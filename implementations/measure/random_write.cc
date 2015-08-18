#include <fcntl.h>
#include <stdbool.h>
#include <stdio.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#include "lib.hh"

int main(int argc, char* argv[]) {
  if (argc != 4 && argc != 5) {
    printf("Usage: ./random_write [file] [count] [block size] ([O_DIRECT?])\n");
    return EXIT_FAILURE;
  }
  bool odirect = argc == 5;
  int flags = odirect ? O_DIRECT : 0;

  // setup file
  const char* file_name = argv[1];
  size_t num_block = atol(argv[2]);
  size_t block_size = atol(argv[3]);
  size_t file_size = num_block * block_size;

  int fd = open(file_name, O_CREAT | O_WRONLY | flags, 00644);
  if (fd < 0) {
    perror("Failed when opening file.");
    return EXIT_FAILURE;
  }

  // setup data to write
  char * data = (char *) aligned_alloc(ALIGN, block_size);
  for (size_t i = 0; i < block_size; ++i) {
    data[i] = i % 256;
  }
  size_t* random_block = new size_t[num_block];
  FillRandomPermutation(random_block, num_block);

  // write data
  struct timespec time_start, time_end;
  clock_gettime(CLOCK_MONOTONIC, &time_start);
  for (size_t i = 0; i < num_block; ++i) {
    ssize_t r = pwrite(fd, data, block_size, random_block[i] * block_size);
    if (r == -1 || (size_t) r != block_size) {
      perror("Failed when writing.");
    }
  }

  // sync data to disk
  if (fdatasync(fd) != 0) {
    perror("Failed when syncing.");
    return EXIT_FAILURE;
  }
  if (close(fd) != 0) {
    perror("Failed when closing.");
    return EXIT_FAILURE;
  }

  // time took?
  clock_gettime(CLOCK_MONOTONIC, &time_end);
  double dur = time_diff(time_start, time_end);
  double mbs = file_size / dur / MB;
  // read?, sync?, random?, block, depth, odirect, MB/s
  printf("%c, %c, %c, %ld, %d, %d, %f\n", 'w', 's', 'r', block_size, 1, odirect, mbs);

  return EXIT_SUCCESS;
}
