/* 파일을 0바이트에서 2,134바이트까지 37바이트씩 늘리면서,
   단계마다 파일 크기가 제대로 보고되는지 확인함. */

#include <syscall.h>
#include "tests/filesys/seq-test.h"
#include "tests/lib.h"
#include "tests/main.h"

static char buf[2134];

static size_t
return_block_size (void) 
{
  return 37;
}

static void
check_file_size (int fd, long ofs) 
{
  long size = filesize (fd);
  if (size != ofs)
    fail ("filesize not updated properly: should be %ld, actually %ld",
          ofs, size);
}

void
test_main (void) 
{
  seq_test ("testfile",
            buf, sizeof buf, 0,
            return_block_size, check_file_size);
}
